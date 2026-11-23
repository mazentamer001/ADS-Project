// =============================================================================
// GraphAlgos.hpp -- Milestone 2: Graph algorithms for circuit preprocessing
//
// Provides two combinational graph analyses built directly on the custom
// Graph, DynArray, Queue, and Stack data structures (no STL):
//   1. levelize() -- Kahn's algorithm using Queue and combInDegree to compute
//      a valid topological evaluation order, assign logic levels, and detect
//      whether a combinational cycle exists.
//   2. findLoop() -- Iterative 3-color (White/Gray/Black) DFS using Stack to
//      locate and return the exact sequence of gates forming a combinational
//      feedback cycle so error reporting can name them.
//
// Wires terminating at sequential elements (GATE_DFF) are ignored by both
// algorithms so synchronous feedback paths (e.g., counters) are permitted.
// =============================================================================
#ifndef GRAPHALGOS_HPP
#define GRAPHALGOS_HPP

#include "Graph.hpp"
#include "DynArray.hpp"

// Computes a topological ordering and combinational logic depth for every gate
// in 'g' using Kahn's algorithm backed by the custom Queue.
//
// Outputs:
//   topoOrder -- gate IDs ordered so every combinational driver precedes its
//                consumers. Incomplete if a combinational cycle exists.
//   levels    -- resized to g.numGates(); levels[id] is the longest
//                combinational path length from any level-0 source (INPUT/DFF)
//                to gate 'id'.
//
// Returns true if the combinational graph is acyclic (DAG), or false if a
// combinational loop prevents full levelization.
// Complexity: O(V + E) time, O(V) auxiliary space.
bool levelize(const Graph& g, DynArray<int>& topoOrder, DynArray<int>& levels);

// Searches 'g' for a combinational feedback cycle using iterative three-color
// depth-first search (White = unvisited, Gray = active path, Black = finished)
// backed by the custom Stack.
//
// Outputs:
//   cycleGates -- cleared, then populated with the gate IDs on the detected
//                 cycle in signal-flow order (g0 -> g1 -> ... -> gk -> g0).
//
// Returns true if a combinational cycle is found, or false if the circuit is
// combinationally acyclic.
// Complexity: O(V + E) time, O(V) auxiliary space.
bool findLoop(const Graph& g, DynArray<int>& cycleGates);

#endif