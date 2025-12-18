#ifndef GRAPH__
#define GRAPH__

#include "sam.h"
#include "tree.h"

int make_random_graph(int n, int d); 
CacheObj make_random_graph_cache(int n, int d);
void random_walk_cache(CacheObj n, int steps);
void random_walk(int& t, int steps);

#endif
