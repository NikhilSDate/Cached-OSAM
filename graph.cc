#include "graph.h"
#include "sam.h"
#include "tree.h"
#include <iostream>

int make_random_graph(int n, int d) {

  std::vector<int> vertices(n);

  for (int i = 0; i < n; ++i) {
    vertices[i] = alloc();
  }

  int t = make_tree(vertices);

  for (int i = 0; i < n; ++i) {
    std::vector<int> out_edges(d);
    for (int j = 0; j < d; ++j) {
      out_edges[j] = uniform_leaf(t);
    }
    int out_tree = make_tree(out_edges);
    auto [t_, wb, b] = deref(out_tree);
    write(vertices[i], b);
  }

  return t;
}

CacheObj make_random_graph_cache(int n, int d) {
  int t = make_random_graph(n, d);
  return deref_cache(&t);
}

void random_walk(int &t, int steps) {
  int n = uniform_leaf(t);

  for (int i = 0; i < steps; ++i) {
    int m = uniform_leaf(n);
    n = m;
  }
}

void random_walk_cache(CacheObj n, int steps) {
  n = uniform_leaf_cache(n);
  for (int i = 0; i < steps; ++i) {
    n = uniform_leaf_cache(n);
  }
}
