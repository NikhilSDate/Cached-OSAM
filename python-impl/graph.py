"""
Graph operations - random graph construction and random walks
"""
from typing import List
import sam
from sam import _sam
import tree
from sam_types import Block

def make_random_graph(n: int, d: int) -> int:
    """
    Create a random graph with n vertices and d out-edges per vertex.
    Returns a pointer to a tree containing all vertices.
    """
    # Create pointers for all vertices
    vertices = [sam.alloc() for _ in range(n)]

    # Build a tree of vertices
    t = tree.Ref(tree.make_tree(vertices))

    # For each vertex, create d random out-edges
    for i in range(n):
        out_edges = []
        for j in range(d):
            # Pick a random leaf from the tree (a random vertex)
            leaf = tree.uniform_leaf(t)
            out_edges.append(leaf)

        # Build a tree of out-edges for this vertex
        out_tree = tree.make_tree(out_edges)

        # Deref the out_tree to get the block data
        t_, wb, b = sam.deref(out_tree)

        # Write this block as the vertex data
        sam.write(vertices[i], b)

    return t.value

def make_random_graph_cache(n: int, d: int) -> sam.CacheObj:
    t = [make_random_graph(n, d)]
    return _sam.deref_cache(t, 0)


def random_walk(t: tree.Ref, steps: int):
    """
    Perform a random walk on the graph for 'steps' steps.
    Modifies t in place (via Ref).
    """
    # Start at a random vertex
    n = tree.Ref(tree.uniform_leaf(t))

    # Perform the walk
    for i in range(steps):
        # n is a pointer to a vertex (which contains a tree of out-edges)
        # Pick a random out-edge
        m = tree.uniform_leaf(n)
        n.value = m

def random_walk_cache(n: sam.CacheObj, steps: int):
    """
    Perform a random walk using cached objects.
    Modifies t in place (via Ref).
    """
    # deref_cache needs to modify t, so we create a temporary single-element list
    # Get first vertex
    n = tree.uniform_leaf_cache(n)

    # Perform the walk
    for i in range(steps):
        n = tree.uniform_leaf_cache(n)
