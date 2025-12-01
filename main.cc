#include "sam.h"
#include "graph.h"
#include <random>
#include <iostream>

int main() {
  constexpr int sam_size = 1 << 26;

  srand(123);

  debug_header();
  // for (int lgn = 8; lgn < 16; ++lgn) {
  //   init(sam_size);

  //   int graph_size = 1 << lgn;
  //   int d = 100;

  //   int t = make_random_graph(graph_size, d);

  //   /**
  //    * PRIME THE GRAPH
  //    */
  //   for (int i = 0; i < graph_size; ++i) {
  //     random_walk(t, 20);
  //   }

  //   debug_reset();

  //   for (int i = 0; i < 50; ++i) {
  //     // std::cout << i << std::endl;
  //     random_walk_cache(t, 50);
  //   }

  //   debug();
  //   clear();
  // }

  init(sam_size);
  {
    // A -> B -> B
    CacheObj obj1; // root object
    {
      CachePtr p = obj1.ptr_at(0);    
      CacheObj obj2 = p.alloc_object();
      CachePtr p2 = obj2.ptr_at(0);
      p2.set(p);
    }
    CachePtr p = obj1.ptr_at(0);
    CacheObj o2 = p.deref();
  }
  debug();
  // clear();
}
