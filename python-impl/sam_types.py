from enum import Enum
from typing import List

BLOCK_SIZE = 16

class BlockType(Enum):
    INNER = 0
    DATA = 1

# A block is a list of BLOCK_SIZE integers
Block = List[int]

def make_block() -> Block:
    """Create a new block initialized to zeros"""
    return [0] * BLOCK_SIZE
