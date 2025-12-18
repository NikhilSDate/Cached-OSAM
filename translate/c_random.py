"""
C-compatible rand() implementation using ctypes to call actual C library
"""
import ctypes

# Load the C standard library
libc = ctypes.CDLL(None)

# Get the actual C rand() and srand() functions
_c_rand = libc.rand
_c_rand.restype = ctypes.c_int

_c_srand = libc.srand
_c_srand.argtypes = [ctypes.c_uint]

def srand(seed):
    """Set random seed (calls actual C srand())"""
    _c_srand(seed)

def rand():
    """Get random number (calls actual C rand())"""
    return _c_rand()
