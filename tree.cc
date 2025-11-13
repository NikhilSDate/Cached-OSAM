#include "tree.h"
#include <cassert>
#include <iostream>


int make_inner(
    int height,
    int n_children,
    int n_leaves,
    int* children) {
  assert (n_children <= fan_out);

  block b;
  b[0] = (int)block_type::DATA;
  b[1] = height;
  b[2] = n_children;
  b[3] = n_leaves;

  for (int i = 0; i < n_children; ++i) {
    b[i + 4] = children[i];
  }
  int out = alloc();
  write(out, b);
  return out;
}

int make_tree(std::vector<int> leaves) {
  std::vector<int> leaf_counts(leaves.size());
  for (auto & l : leaf_counts) { l = 1; }
  int height = 1;

  while (leaves.size() > 1) {
    const auto m = (leaves.size() + fan_out - 1) / fan_out;
    std::vector<int> level(m);
    std::vector<int> lcs(m);
    for (int i = 0; i < m; ++i) {
      int n_children = std::min(fan_out, leaves.size() - i*fan_out);
      int n_leaves = 0;
      for (int j = 0; j < n_children; ++j) {
        n_leaves += leaf_counts[i*fan_out+j];
      }
      lcs[i] = n_leaves;
      level[i] = make_inner(height, n_children, n_leaves, leaves.data() + i*fan_out);
    }
    leaves = level;
    leaf_counts = lcs;
    ++height;
  }
  return leaves[0];
}

void print_tree(int indent, int tree) {
  const auto [tree_, node_, b] = deref(tree);
  const auto height = b[1];
  const auto n_children = b[2];
  const auto n_leaves = b[3];
  for (int i = 0; i < indent; ++i) {
    std::cout << "  ";
  }
  std::cout << height << ' ' << n_leaves << '\n';

  if (n_leaves > fan_out) {
    for (std::size_t i = 0; i < n_children; ++i) {
      print_tree(indent+1, b[i + 4]);
    }
  }
}

// get_leaf pseudocode
// p = root
// while () {
//  node = deref(p)
//  if node is leaf:
//    break
//  node = deref(node.child[i])
//}


int get_leaf(int which_leaf, int write_back, block& b) {
  int height = b[1];
  if (height == 1) {
    const auto [p, q] = copy(b[which_leaf + 4]);
    b[which_leaf + 4] = p;
    write(write_back, b);
    return q;
  } else {

    int max_leaves_per_child = 1;
    for (int i = 0; i < height - 1; ++i) {
      max_leaves_per_child *= fan_out;
    }

    int child = which_leaf / max_leaves_per_child;
    
    auto [child_, wb, the_child] = deref(b[child + 4]);
    b[child + 4] = child_;
    write(write_back, b);

    return get_leaf(
        which_leaf % max_leaves_per_child,
        wb,
        the_child);
  }
}

int get_leaf(int which, int& t) {
  auto [t_, wb, b] = deref(t);
  t = t_;
  return get_leaf(which, wb, b);
}

int uniform_leaf(int& t) {
  auto [t_, wb, b] = deref(t);
  t = t_;

  int which_leaf = rand() % b[3];
  return get_leaf(which_leaf, wb, b);
}
