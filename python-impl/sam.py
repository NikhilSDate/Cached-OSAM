from typing import Tuple, Dict, Optional, List
from sam_types import Block, BlockType, make_block, BLOCK_SIZE
import sys

class SAM:
    def __init__(self):
        self.size = 0
        self.next = 0
        self.sam: Dict[int, Block] = {}
        self.used: set[int] = set()
        self.cache: Dict = {}

        # Statistics
        self.reads = 0
        self.writes = 0
        self.allocs = 0

    def init(self, n: int):
        """Initialize SAM with n slots"""
        self.size = n
        self.reads = 0
        self.writes = 0
        self.allocs = 0
        self.next = 0
        # Use a dictionary instead of a list to avoid preallocating huge arrays
        self.sam = {}
        self.used = set()  # Use a set to track used addresses
        self.cache = {}

    def clear(self):
        """Clear the SAM"""
        self.size = 0
        self.sam = {}
        self.used = set()
        self.cache = {}

    def debug_reset(self):
        """Reset SAM usage counters"""
        self.reads = 0
        self.writes = 0
        self.allocs = 0

    def debug_header(self):
        print("READS\tWRITES\tALLOCS")

    def debug(self):
        """Print SAM usage"""
        print(f"{self.reads}\t{self.writes}\t{self.allocs}")

    def alloc(self) -> int:
        """Set aside a pointer with no pointee"""
        self.allocs += 1
        result = self.next
        self.next += 1
        return result

    def write(self, p: int, b: Block):
        """Write block b to pointer p"""
        self.writes += 1
        self.sam[p] = b.copy()

    def read(self, i: int) -> Block:
        """Read from address i (can only be done once per address)"""
        self.reads += 1
        if i in self.used:
            print(f"ADDRESS {i} ALREADY READ!", file=sys.stderr)
            print(f"SAM SIZE {self.size}", file=sys.stderr)
            print(f"ALLOCATIONS {self.allocs}", file=sys.stderr)
            sys.exit(1)
        else:
            self.used.add(i)
            return self.sam[i].copy()

    def link(self, l: int, r: int, p: int):
        """Create linked INNER blocks"""
        self.write(l, [BlockType.INNER.value, p, r] + [-1] * (BLOCK_SIZE - 3))
        self.write(r, [BlockType.INNER.value, p, l] + [-1] * (BLOCK_SIZE - 3))

    def copy(self, p: int) -> Tuple[int, int]:
        """
        Copy pointer p, invalidating p but yielding two fresh pointers p' and p''
        """
        l = self.alloc()
        r = self.alloc()
        self.link(l, r, p)
        return (l, r)

    def destroy(self, x: int):
        """Destroy pointer x"""
        if x in self.cache:
            return

        bl = self.read(x)
        if BlockType(bl[0]) == BlockType.DATA:
            # DO NOTHING
            pass
        else:
            p = bl[1]
            s = bl[2]

            if p in self.cache:
                dummy = self.alloc()
                self.write(dummy, [BlockType.INNER.value, p, s] + [-1] * (BLOCK_SIZE - 3))
                self.write(s, [BlockType.INNER.value, p, dummy] + [-1] * (BLOCK_SIZE - 3))
                return

            bl2 = self.read(p)
            if BlockType(bl2[0]) == BlockType.DATA:
                self.write(s, bl2)
            else:
                p_ = bl2[1]
                s_ = bl2[2]
                self.link(s, s_, p_)

    def splay(self, a: int, b: int, x: int) -> Tuple[int, Block]:
        """
        Chase pointer x, splaying the tree as it goes.
        Return the writeback address and the block data.
        """
        while True:
            bl = self.read(x)
            tx = BlockType(bl[0])
            y = bl[1]
            c = bl[2]

            if tx == BlockType.DATA:
                x_ = self.alloc()
                self.link(a, b, x_)
                return (x_, bl)

            bl2 = self.read(y)
            ty = BlockType(bl2[0])
            z = bl2[1]
            d = bl2[2]

            if ty == BlockType.DATA:
                # zig splay
                x_ = self.alloc()
                y_ = self.alloc()

                self.link(b, c, y_)
                self.link(a, y_, x_)

                return (x_, bl2)

            # zig-zag splay
            x_ = z
            y_ = self.alloc()
            z_ = self.alloc()

            self.link(a, b, y_)
            self.link(c, d, z_)

            a = y_
            b = z_
            x = x_

    def deref(self, x: int) -> Tuple[int, int, Block]:
        """
        Dereference pointer x yielding block b.
        Returns [p', q, b] where:
        - p' can be used to replace p
        - q is where to write back an updated version of b
        - b is the block data
        """
        bl = self.read(x)
        t = BlockType(bl[0])
        p = bl[1]
        s = bl[2]

        x_ = self.alloc()
        if t == BlockType.DATA:
            return (x_, x_, bl)
        else:
            where, b = self.splay(x_, s, p)
            return (x_, where, b)

    def save(self, p: int, data: Block) -> int:
        """Helper function to dereference, then write back data"""
        p_, where, b = self.deref(p)
        self.write(where, data)
        return p_

    def cache_evict(self, x: int):
        """Evict address x from cache and write it back to SAM"""
        if x not in self.cache:
            return
        block_ptr, refcount = self.cache[x]
        self.write(x, block_ptr)
        del self.cache[x]

    def splay_cache(self, a: int, b: int, x: int):
        """
        Splay operation that returns a CacheObj.
        Like splay(), but returns cached objects and checks cache at each step.
        """
        while True:
            if x in self.cache:
                self.link(a, b, x)
                return CacheObj(x)

            bl = self.read(x)
            tx = BlockType(bl[0])
            y = bl[1]
            c = bl[2]

            if tx == BlockType.DATA:
                x_ = self.alloc()
                self.link(a, b, x_)

                # put data into the cache
                bl[15] = x_
                self.cache[x_] = [bl, 0]  # [block, refcount]
                return CacheObj(x_)

            if y in self.cache:
                x_ = self.alloc()
                self.link(a, b, x_)
                self.link(x_, c, y)
                return CacheObj(y)

            bl2 = self.read(y)
            ty = BlockType(bl2[0])
            z = bl2[1]
            d = bl2[2]

            if ty == BlockType.DATA:
                # zig splay
                x_ = self.alloc()
                y_ = self.alloc()

                self.link(b, c, y_)
                self.link(a, y_, x_)

                bl2[15] = x_
                self.cache[x_] = [bl2, 0]
                return CacheObj(x_)

            # zig-zag splay
            x_ = z
            y_ = self.alloc()
            z_ = self.alloc()

            self.link(a, b, y_)
            self.link(c, d, z_)

            a = y_
            b = z_
            x = x_

    def deref_cache(self, block_ref: Block, idx: int):
        """
        Dereference a pointer stored in block_ref[idx], updating it in place.
        Returns a CacheObj.
        """
        x = block_ref[idx]

        if x in self.cache:
            return CacheObj(x)

        bl = self.read(x)
        t = BlockType(bl[0])
        p = bl[1]
        s = bl[2]

        x_ = self.alloc()
        block_ref[idx] = x_  # Update the pointer in place

        if t == BlockType.DATA:
            self.cache[x_] = [bl, 0]
            return CacheObj(x_)
        else:
            return self.splay_cache(x_, s, p)

# Global SAM instance
_sam = SAM()

# Expose module-level functions
def init(n: int):
    _sam.init(n)

def clear():
    _sam.clear()

def debug_reset():
    _sam.debug_reset()

def debug_header():
    _sam.debug_header()

def debug():
    _sam.debug()

def alloc() -> int:
    return _sam.alloc()

def write(p: int, b: Block):
    _sam.write(p, b)

def read(i: int) -> Block:
    return _sam.read(i)

def copy(p: int) -> Tuple[int, int]:
    return _sam.copy(p)

def destroy(x: int):
    _sam.destroy(x)

def deref(x: int) -> Tuple[int, int, Block]:
    return _sam.deref(x)

def save(p: int, data: Block) -> int:
    return _sam.save(p, data)

# Cache-related classes and functions

class CacheObj:
    """
    Wrapper for cached blocks with automatic reference counting.
    When all references are released, the block is evicted from cache.
    """
    def __init__(self, addr: Optional[int] = None):
        if addr is None:
            # Default constructor - allocate new object
            self.addr_ = _sam.alloc()
            b = [BlockType.DATA.value] + [-1] * (BLOCK_SIZE - 1)
            _sam.cache[self.addr_] = [b, 1]  # refcount starts at 1
        else:
            # Constructor from address
            self.addr_ = addr
            _sam.cache[self.addr_][1] += 1  # Increment refcount

    def __del__(self):
        self.release()

    def destroy(self):
        pass

    def release(self):
        """Explicitly decrement reference count and evict if zero (call instead of relying on __del__)"""
        if self.addr_ is not None and self.addr_ in _sam.cache:
            _sam.cache[self.addr_][1] -= 1
            if _sam.cache[self.addr_][1] == 0:
                _sam.cache_evict(self.addr_)
            self.addr_ = None  # Prevent double-destroy

    def at(self, idx: int) -> int:
        """Read value at index (for first 4 fields of the block)"""
        assert idx < 4, f"Index {idx} out of range, must be < 4"
        block_ref = _sam.cache[self.addr_][0]
        return block_ref[idx]

    def set(self, idx: int, val: int):
        """Write value at index (for first 4 fields of the block)"""
        block_ref = _sam.cache[self.addr_][0]
        block_ref[idx] = val

    def deref_at(self, idx: int):
        """Dereference pointer at field idx+4, returns new CacheObj"""
        block_ref = _sam.cache[self.addr_][0]
        return _sam.deref_cache(block_ref, idx + 4)

    def __copy__(self):
        """Copy constructor"""
        raise ValueError

    def __deepcopy__(self, memo):
        """Deep copy - same as shallow copy for CacheObj"""
        raise ValueError

    def ptr_at(self, idx: int):
        """Get a CachePtr pointing to field idx+4 of this object"""
        return CachePtr(self.addr_, idx)

class CachePtr:
    def __init__(self, addr: int, idx: int):
        self.addr_ = addr
        self.idx_ = idx
        _sam.cache[addr][1] += 1  # Increment refcount of parent CacheObj

    def destroy(self):
        pass

    def __del__(self):
        self.release()

    def release(self):
        """Explicitly release reference to parent CacheObj (call instead of relying on __del__)"""
        if self.addr_ is not None and self.addr_ in _sam.cache:
            _sam.cache[self.addr_][1] -= 1
            if _sam.cache[self.addr_][1] == 0:
                _sam.cache_evict(self.addr_)
            self.addr_ = None  # Prevent double-destroy
    
    def deref(self) -> CacheObj:
        """Dereference this pointer, returning the pointed-to CacheObj"""
        block_ref = _sam.cache[self.addr_][0]
        return _sam.deref_cache(block_ref, self.idx_ + 4)

    def set(self, other: 'CachePtr'):
        """
        Set this pointer to point to the same thing as other.
        Calls copy_cache to duplicate the pointer.
        """
        self.destroy_slot()
        b1 = _sam.cache[self.addr_][0]
        b2 = _sam.cache[other.addr_][0]

        # p2 points to b2[other.idx_ + 4]
        # We need to call copy_cache which takes a pointer and modifies it
        # copy_cache(p2) will modify b2[other.idx_ + 4] and return a new pointer

        # Simulate copy_cache(&(*b2)[other.idx_ + 4])
        p_val = b2[other.idx_ + 4]
        l = _sam.alloc()
        r = _sam.alloc()
        _sam.link(l, r, p_val)
        b2[other.idx_ + 4] = r  # Modify b2's pointer

        # Set p1 to the returned value
        b1[self.idx_ + 4] = l

    def alloc_object(self) -> CacheObj:
        """
        Allocate a new CacheObj and make this pointer point to it.
        """
        self.destroy_slot()
        block_ref = _sam.cache[self.addr_][0]

        a = _sam.alloc()
        block_ref[self.idx_ + 4] = a

        b = [BlockType.DATA.value] + [-1] * (BLOCK_SIZE - 1)
        _sam.cache[a] = [b, 0]
        return CacheObj(a)

    def destroy_slot(self):
        """Destroy the pointer stored at this location (if any)"""
        block_ref = _sam.cache[self.addr_][0]
        p = block_ref[self.idx_ + 4]
        if p == -1:
            # slot is empty
            return
        # destroy_cache is a no-op in the C++ code, so we do nothing
