#include "sam.h"
#include "types.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <utility>

int size;
int next;
block *sam;
bool *used;
std::unordered_map<int, std::pair<block *, size_t>> cache;

int reads = 0;
int writes = 0;
int allocs = 0;

void init(int n) {
  size = n;
  reads = 0;
  writes = 0;
  allocs = 0;

  next = 0;
  sam = new block[n];
  used = new bool[n];
  memset(used, 0, n * sizeof(bool));
}

void clear() {
  size = 0;
  delete[] sam;
  delete[] used;
}

void debug_header() {
  std::cout << "READS" << "\t" << "WRITES" << "\t" << "ALLOCS" << '\n';
}

void debug() { std::cout << reads << "\t" << writes << "\t" << allocs << '\n'; }

void debug_reset() {
  reads = 0;
  writes = 0;
  allocs = 0;
}

void write(int i, block x) {
  ++writes;
  sam[i] = x;
}

block read(int i) {
  ++reads;
  if (used[i]) {
    int x = -1;
    std::cerr << "ADDRESS " << i << " ALREADY READ!\n";
    std::cerr << "SAM SIZE " << size << '\n';
    std::cerr << "ALLOCATIONS " << allocs << '\n';
    std::exit(1);
  } else {
    used[i] = true;
    return sam[i];
  }
}

int alloc() {
  ++allocs;
  return next++;
}

void link(int l, int r, int p) {
  write(l, {(int)block_type::INNER, p, r});
  write(r, {(int)block_type::INNER, p, l});
}

std::pair<int, int> copy(int p) {
  int l = alloc();
  int r = alloc();
  link(l, r, p);
  return {l, r};
}

int copy_cache(int* p) {
  int l = alloc();
  int r = alloc();
  link(l, r, *p);
  *p = r;
  return l;
}

void destroy(int x) {
  if (cache.contains(x)) {
    return;
  }
  const auto bl = read(x);
  if ((block_type)bl[0] == block_type::DATA) {
    // DO NOTHING
  } else {
    const auto p = bl[1];
    const auto s = bl[2];

    if (cache.contains(p)) {
      int dummy = alloc();
      write(dummy, {(int)block_type::INNER, p, s});
      write(s, {(int)block_type::INNER, p, dummy});
      return;
    }

    const auto bl2 = read(p);
    if ((block_type)bl2[0] == block_type::DATA) {
      write(s, bl2);
    } else {
      const auto p_ = bl2[1];
      const auto s_ = bl2[2];
      link(s, s_, p_);
    }
  }
}

void destroy_cache(int x) {

}

/**
 * Chase pointer x, splaying the tree as it goes.
 * As input take (1) a pointer x and (2) two elements that should point to the
 * new version of x.
 *
 * Return (1) the pointer where pointed-to data should be written back and (2)
 * the two fields of the pointee.
 */

int counter = 0;
std::pair<int, block> splay(int a, int b, int x) {
  while (true) {
    ++counter;

    const auto bl = read(x);
    const auto tx = (block_type)bl[0];
    const auto y = bl[1];
    const auto c = bl[2];

    if (tx == block_type::DATA) {
      int x_ = alloc();
      link(a, b, x_);
      return {x_, bl};
    }

    const auto bl2 = read(y);
    const auto ty = (block_type)bl2[0];
    const auto z = bl2[1];
    const auto d = bl2[2];
    if (ty == block_type::DATA) {
      /**
       * zig splay:
       *
       *     y            x
       *    / \          / \
       *   x   c  ==>   a   y
       *  / \              / \
       * a   b            b   c
       */

      int x_ = alloc();
      int y_ = alloc();

      link(b, c, y_);
      link(a, y_, x_);

      return {x_, bl2};
    }

    /**
     * zig-zag splay:
     *       z               x
     *      / \             / \
     *     y   d           /   \
     *    / \      ==>    y     z
     *   c   x           / \   / \
     *      / \         a   b c   d
     *     a   b
     */

    int x_ = z;
    int y_ = alloc();
    int z_ = alloc();

    link(a, b, y_);
    link(c, d, z_);

    a = y_;
    b = z_;
    x = x_;
  }
}

// splays tree, but returns from cache at last level if hit
// else puts into cache
// in either case, returns the writeback address/cache tag
int counter_cache = 0;
CacheObj splay_cache(int a, int b, int x) {
  while (true) {
    ++counter;

    if (cache.contains(x)) {
      link(a, b, x);
      return CacheObj(x);
    }

    auto bl = read(x);
    const auto tx = (block_type)bl[0];
    const auto y = bl[1];
    const auto c = bl[2];

    if (tx == block_type::DATA) {
      int x_ = alloc();
      link(a, b, x_);

      // put data into the cache
      bl[15] = x_;
      cache[x_] = {new block(bl), 0};
      return CacheObj(x_);
    }

    if (cache.contains(y)) {
      int x_ = alloc();
      link(a, b, x_);
      link(x_, c, y);

      return CacheObj(y);
    }

    auto bl2 = read(y);
    const auto ty = (block_type)bl2[0];
    const auto z = bl2[1];
    const auto d = bl2[2];
    if (ty == block_type::DATA) {
      /**
       * zig splay:
       *
       *     y            x
       *    / \          / \
       *   x   c  ==>   a   y
       *  / \              / \
       * a   b            b   c
       */

      int x_ = alloc();
      int y_ = alloc();

      link(b, c, y_);
      link(a, y_, x_);

      bl2[15] = x_;
      cache[x_] = {new block(bl2), 0};
      return CacheObj(x_);
    }

    /**
     * zig-zag splay:
     *       z               x
     *      / \             / \
     *     y   d           /   \
     *    / \      ==>    y     z
     *   c   x           / \   / \
     *      / \         a   b c   d
     *     a   b
     */

    int x_ = z;
    int y_ = alloc();
    int z_ = alloc();

    link(a, b, y_);
    link(c, d, z_);

    a = y_;
    b = z_;
    x = x_;
  }
}

std::tuple<int, int, block> deref(int x) {
  const auto bl = read(x);
  const auto t = (block_type)bl[0];
  const auto p = bl[1];
  const auto s = bl[2];

  const auto x_ = alloc();
  if (t == block_type::DATA) {
    return {x_, x_, bl};
  } else {
    const auto [where, b] = splay(x_, s, p);
    return {x_, where, b};
  }
}

CacheObj deref_cache(int *x) {
  if (cache.contains(*x)) {
    return CacheObj(*x);
  }

  auto bl = read(*x);
  const auto t = (block_type)bl[0];
  const auto p = bl[1];
  const auto s = bl[2];

  const auto x_ = alloc();
  *x = x_;

  if (t == block_type::DATA) {
    cache[x_] = {new block(bl), 0};
    return CacheObj(x_);
  } else {
    return splay_cache(x_, s, p);
  }
}

void cache_evict(int x) {
  if (!cache.contains(x)) {
    return;
  }
  auto data = cache.at(x);
  write(x, *(data.first));
  delete data.first;
  cache.erase(x);
}

CacheObj deref_cache(int *x);

block make_data_block() {
  block b;
  b[0] = (int)block_type::DATA;
  for (int i = 1; i < b.size(); i++) {
    b[i] = -1;
  }
  return b;
}


CacheObj::CacheObj() {
  int addr = alloc();
  block b = make_data_block();

  cache[addr] = {new block(b), 1}; // one reference
  addr_ = addr;
}

// Default constructor
CacheObj::CacheObj(int addr) : addr_{addr} { cache.at(addr).second += 1; }

// Copy constructor
CacheObj::CacheObj(const CacheObj &other) : addr_(other.addr_) {
  cache.at(addr_).second += 1;
}

// Copy assignment
CacheObj &CacheObj::operator=(const CacheObj &other) {
  if (this != &other) {
    // Decrement old reference
    release();

    // Copy from other
    addr_ = other.addr_;

    // Increment new reference
    cache.at(addr_).second += 1;
  }
  return *this;
}

int CacheObj::at(int idx) {
  assert(idx < 4);
  block *b = cache.at(addr_).first;
  return (*b)[idx];
}

void CacheObj::set(int idx, int val) {
  block *b = cache.at(addr_).first;
  (*b)[idx] = val;
}

CachePtr CacheObj::ptr_at(int idx) {
  return CachePtr(addr_, idx);
}

// Destructor
CacheObj::~CacheObj() { release(); }

void CacheObj::release() {
  cache.at(addr_).second -= 1;
  if (cache.at(addr_).second == 0) {
    cache_evict(addr_);
  }
}

// Default constructor
CachePtr::CachePtr(int addr, int idx) : addr_{addr}, idx_{idx} {
  cache.at(addr).second += 1;
}

// Copy constructor
CachePtr::CachePtr(const CachePtr &other) : addr_(other.addr_), idx_(other.idx_) {
  cache.at(addr_).second += 1;
}

CachePtr &CachePtr::operator=(const CachePtr &other) {
  if (this != &other) {
    // Decrement old reference
    cache.at(addr_).second -= 1;
    if (cache.at(addr_).second == 0) {
      cache_evict(addr_);
    }

    // Copy from other
    addr_ = other.addr_;
    idx_ = other.idx_;

    // Increment new reference
    cache.at(addr_).second += 1;
  }
  return *this;
}

CacheObj CachePtr::deref() {
  block *b = cache.at(addr_).first;
  return deref_cache(&(*b)[idx_ + 4]);
}

void CachePtr::set(CachePtr other) {
  destroy();
  block *b1 = cache.at(addr_).first;
  block* b2 = cache.at(other.addr_).first;
  int* p1 = &(*b1)[idx_ + 4];
  int* p2 = &(*b2)[other.idx_ + 4];
  *p1 = copy_cache(p2);
}

CacheObj CachePtr::alloc_object() {
  destroy();
  block *b1 = cache.at(addr_).first;
  int* p1 = &(*b1)[idx_ + 4];
  int a = alloc();
  *p1 = a;
  block b = make_data_block();
  cache[a] = {new block(b), 0};
  return CacheObj(a);
}

void CachePtr::destroy() {
  block *b = cache.at(addr_).first;
  int* p = &(*b)[idx_ + 4];
  if (*p == -1) {
    // slot is empty
    return;
  }
  destroy_cache(*p); // handle case where slot is not empty
}

// Destructor
CachePtr::~CachePtr() {
  cache.at(addr_).second -= 1;
  if (cache.at(addr_).second == 0) {
    cache_evict(addr_);
  }
}
