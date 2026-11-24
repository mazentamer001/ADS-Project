
// GraphAlgos.hpp -- Milestone 2: Graph algorithms for circuit preprocessing
//
// Provides two combinational graph analyses built directly on the custom
// Graph, DynArray, Queue, and Stack data structures (no STL):
//   1. levelize() -- Kahn's algorithm using Queue and combInDegree 
//   2. findLoop() -- Iterative 3-color (White/Gray/Black) DFS using Stack to
//      locate and return the exact sequence of gates forming a combinational
//      feedback cycle so error reporting can name them.

#ifndef GRAPHALGOS_HPP
#define GRAPHALGOS_HPP

#include "Graph.hpp"
#include "DynArray.hpp"


// Outputs:
//   topoOrder -- gate IDs ordered so every combinational driver precedes its
//                consumers. Incomplete if a combinational cycle exists.
//   levels    -- resized to g.numGates(); levels[id] is the longest
//                combinational path length from any level-0 source (INPUT/DFF)
//                to gate 'id'.

bool levelize(const Graph& g, DynArray<int>& topoOrder, DynArray<int>& levels);


// Outputs:
//   cycleGates --  populated with the gate IDs on 
// Returns true if a combinational cycle is found, or false if the circuit is

bool findLoop(const Graph& g, DynArray<int>& cycleGates);

#endif
