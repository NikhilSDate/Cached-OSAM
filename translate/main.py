"""
Main program - matches main.cc behavior
"""
import sam
import graph
import tree
import c_random

def main():
    sam_size = 1 << 26

    c_random.srand(123)

    sam.debug_header()

    for lgn in range(8, 16):
        sam.init(sam_size)

        graph_size = 1 << lgn
        d = 100

        t = tree.Ref(graph.make_random_graph(graph_size, d))

        # PRIME THE GRAPH
        for i in range(graph_size):
            graph.random_walk_cache(t, 20)

        sam.debug_reset()

        for i in range(50):
            graph.random_walk_cache(t, 50)

        sam.debug()
        sam.clear()

if __name__ == "__main__":
    main()
