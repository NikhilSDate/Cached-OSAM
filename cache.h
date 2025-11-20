#pragma once

#include "types.h"
#include <unordered_map>
#include <iostream>
#include <cassert>


std::unordered_map<int, std::pair<block*, size_t>> cache;

void cache_evict(int x) {
  if (!cache.contains(x)) {
    return;
  }
  auto data = cache.at(x);
  write(x, *(data.first));
  delete data.first;
  cache.erase(x);
}

class CachePtr {
public:
    // Default constructor
    CachePtr(int addr): addr_{addr} {
        cache.at(addr).second += 1;
    }

    // Copy constructor
    CachePtr(const CachePtr& other)
        : addr_(other.addr_)
    {
        cache.at(addr_).second += 1;
    }

    // Copy assignment
    CachePtr& operator=(const CachePtr& other)
    {
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

    CachePtr deref_at(int idx) {
        block* b = cache.at(addr_).first;
        return CachePtr((*b)[idx + 4]);
    }

    int at(int idx) {
        assert(idx < 4);
        block* b = cache.at(addr_).first;
        return (*b)[idx];
    }

    // Destructor
    ~CachePtr()
    {
        release();
    }

private:
    int addr_;

    void release()
    {
        cache.at(addr_).second -= 1;
        if (cache.at(addr_).second == 0) {
            std::cout << "evicting " << addr_ << std::endl;
            cache_evict(addr_);
        }
    }
};
