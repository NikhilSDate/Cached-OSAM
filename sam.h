#ifndef SAM_H__
#define SAM_H__

#include "types.h"
#include <cassert>
#include <iostream>
#include <unordered_map>

// Initialize SAM with `n` slots.
void init(int n);

// Clear the SAM.
void clear();

// Reset SAM usage counters.
void debug_reset();

// Print SAM usage header.
void debug_header();

// Print SAM usage.
void debug();

/**
 * Set aside a pointer with no pointee.
 */
int alloc();

/**
 * Write block `b` to pointer `p`.
 */
void write(int p, block b);

/**
 * Copy pointer `p`, invalidating `p` but yielding two fresh
 * pointer `p'` and `p''`.
 */
std::pair<int, int> copy(int p);

/**
 * Destroy pointer `p`.
 */
void destroy(int p);

/**
 * Dereference pointer `p` yielding block `b`.
 * Pointer `p` is implicitly destroyed.
 * This block is "moved" from the server, so it is not yet safe to
 * dereference another pointer to `b`.
 *
 * Also yields
 * (1) pointer `p'`, which can be used to replace `p` and
 * (2) pointer `q`, which is where one should write back an
 * updated version of `b` when one is done with it.
 * Output is in order `[p', q, b]`.
 */
std::tuple<int, int, block> deref(int p);

inline void save(int &p, block data) {
  auto [p_, where, b] = deref(p);
  p = p_;
  write(where, data);
}

class CacheObj;
class CachePtr;

CacheObj deref_cache(int *x);

class CacheObj {
public:
  // Default constructor
  CacheObj();

  CacheObj(int addr);

  // Copy constructor
  CacheObj(const CacheObj &other);

  // Copy assignment
  CacheObj &operator=(const CacheObj &other);

  int at(int idx);

  void set(int idx, int val);

  CachePtr ptr_at(int idx);

  // Destructor
  ~CacheObj();

private:
  int addr_;

  void release();
};

class CachePtr {
  public:
  CachePtr(int addr, int idx);

  // Copy constructor
  CachePtr(const CachePtr &other);

  // Copy assignment
  CachePtr &operator=(const CachePtr &other);

  CacheObj deref();

  void set(CachePtr other);

  CacheObj alloc_object();

  ~CachePtr();
  private:
    void destroy();

    int addr_;
    int idx_;
};

#endif
