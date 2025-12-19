from typing import List
import sam
from sam_types import Block, BlockType, BLOCK_SIZE
import c_random

FAN_OUT = 6

def make_inner(height: int, n_children: int, n_leaves: int, children: List[int]) -> int:
    """
    Create an inner tree node (DATA block in tree context).
    Block structure:
    - b[0] = DATA type
    - b[1] = height
    - b[2] = n_children
    - b[3] = n_leaves
    - b[4..4+n_children-1] = child pointers
    """
    assert n_children <= FAN_OUT

    b: Block = [0] * BLOCK_SIZE
    b[0] = BlockType.DATA.value
    b[1] = height
    b[2] = n_children
    b[3] = n_leaves

    for i in range(n_children):
        b[i + 4] = children[i]

    out = sam.alloc()
    sam.write(out, b)
    return out

def make_tree(leaves: List[int]) -> int:
    """
    Build a tree from a list of leaf pointers.
    Returns the root pointer.
    """
    leaf_counts = [1] * len(leaves)
    height = 1

    while len(leaves) > 1:
        m = (len(leaves) + FAN_OUT - 1) // FAN_OUT
        level = []
        lcs = []

        for i in range(m):
            n_children = min(FAN_OUT, len(leaves) - i * FAN_OUT)
            n_leaves = sum(leaf_counts[i * FAN_OUT : i * FAN_OUT + n_children])
            lcs.append(n_leaves)

            children = leaves[i * FAN_OUT : i * FAN_OUT + n_children]
            level.append(make_inner(height, n_children, n_leaves, children))

        leaves = level
        leaf_counts = lcs
        height += 1

    return leaves[0]

def get_leaf(which_leaf: int, write_back: int, b: Block) -> int:
    """
    Recursively traverse tree to get a specific leaf.
    Returns a pointer to the leaf.
    """
    height = b[1]

    if height == 1:
        # At leaf level, copy the child pointer
        p, q = sam.copy(b[which_leaf + 4])
        b[which_leaf + 4] = p
        sam.write(write_back, b)
        return q
    else:
        # Calculate which child to descend into
        max_leaves_per_child = 1
        for i in range(height - 1):
            max_leaves_per_child *= FAN_OUT

        child = which_leaf // max_leaves_per_child

        # Deref the child
        child_ptr, wb, the_child = sam.deref(b[child + 4])
        b[child + 4] = child_ptr
        sam.write(write_back, b)

        # Recurse
        return get_leaf(which_leaf % max_leaves_per_child, wb, the_child)

class Ref:
    """Simple reference wrapper to simulate C++ reference semantics"""
    def __init__(self, value: int):
        self.value = value

def get_leaf_from_ref(which: int, t: Ref) -> int:
    """
    Get a specific leaf from tree pointer t (modifies t in place).
    Returns the leaf pointer.
    """
    t_, wb, b = sam.deref(t.value)
    t.value = t_
    return get_leaf(which, wb, b)

def uniform_leaf(t: Ref) -> int:
    """
    Get a random leaf from tree pointer t (modifies t in place).
    Returns the leaf pointer.
    """
    t_, wb, b = sam.deref(t.value)
    t.value = t_

    which_leaf = c_random.rand() % b[3]  # b[3] is n_leaves
    return get_leaf(which_leaf, wb, b)

def get_leaf_cache(which_leaf: int, b: sam.CacheObj) -> sam.CacheObj:
    """
    Get a specific leaf from a cached tree node.
    Works by descending the tree using CacheObj.deref_at().
    """
    while True:
        height = b.at(1)
        if height == 1:
            break

        max_leaves_per_child = 1
        for i in range(height - 1):
            max_leaves_per_child *= FAN_OUT

        child = which_leaf // max_leaves_per_child
        b = b.deref_at(child)
        # Here the old b is destroyed (via __del__), so it should get evicted

        which_leaf = which_leaf % max_leaves_per_child

    return b.deref_at(which_leaf)

def uniform_leaf_cache(b: sam.CacheObj) -> sam.CacheObj:
    """
    Get a random leaf from a cached tree node.
    """
    which_leaf = c_random.rand() % b.at(3)  # b.at(3) is n_leaves
    return get_leaf_cache(which_leaf, b)
