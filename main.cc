#include "sam.h"
#include "graph.h"
#include <random>
#include <iostream>

int main() {
  constexpr int sam_size = 1 << 26;

  srand(time(NULL));

  debug_header();
  for (int lgn = 8; lgn < 16; ++lgn) {
    init(sam_size);

    int graph_size = 1 << lgn;
    int d = 100;

    int t = make_random_graph(graph_size, d);

    /**
     * PRIME THE GRAPH
     */
    for (int i = 0; i < graph_size; ++i) {
      random_walk(t, 20);
    }

    debug_reset();

    for (int i = 0; i < 50; ++i) {
      random_walk(t, 50);
    }

    debug();
    clear();
  }
}
