#ifndef TREE_H__
#define TREE_H__

#include "sam.h"
#include <vector>

constexpr std::size_t fan_out = 6;

int make_tree(std::vector<int> leaves);
int make_tree(int where, std::vector<int> leaves);

void print_tree(int indent, int tree);

/**
 * This procedure copies the accessed leaf node of the tree, such that the tree
 * can be traversed to the same leaf again.
 */
int get_leaf(int which, int& t);
int uniform_leaf(int& t);
block* uniform_leaf_cache(block* b);

#endif
