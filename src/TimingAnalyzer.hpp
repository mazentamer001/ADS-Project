// =============================================================================
// TimingAnalyzer.hpp -- Person 4: static timing analysis (longest path)
//
// Longest-path DP over a topological order of the circuit graph:
//     arrival[g] = delay[g] + max(arrival[d])  over the combinational drivers d of g
// (a gate with no drivers, e.g. a primary INPUT, has arrival[g] = delay[g]).
// pred[g] remembers which driver gave the maximum, so the critical path can be
// walked backwards from the latest endpoint.
//
// Endpoints are the primary OUTPUT nodes (if the circuit has none, every gate).
// Wires into a DFF are not combinational (Graph::isCombinationalEdge), so a DFF
// starts a new path: this is already correct for the sequential milestone.
//
// Two ways to call analyze():
//   analyze(g)         builds its own topological order (Kahn: Queue + combInDegree)
//   analyze(g, order)  uses an order that was computed elsewhere (e.g. levelize())
// Both return false if the graph has a combinational loop.
// =============================================================================
#ifndef TIMINGANALYZER_HPP
#define TIMINGANALYZER_HPP

#include "Graph.hpp"
#include "DynArray.hpp"

class TimingAnalyzer {
public:
    TimingAnalyzer() : endpoint_(INVALID), delay_(0) {}

    // Builds the topological order itself. False = combinational loop found
    // (then loopGates() lists the gates that could not be ordered).
    bool analyze(const Graph& g);

    // Uses a topological order computed by someone else (must hold every gate once).
    bool analyze(const Graph& g, const DynArray<int>& order);

    int criticalDelay() const { return delay_; }
    int criticalEndpoint() const { return endpoint_; }       // gate id, INVALID if no gates
    int arrival(int id) const { return arrival_[id]; }
    const DynArray<int>& topoOrder() const { return order_; }
    const DynArray<int>& loopGates() const { return loop_; }

    // Critical path from the startpoint to the endpoint (gate ids, in signal order).
    void criticalPath(DynArray<int>& path) const;

    // Prints the critical path and its delay.
    void print(const Graph& g) const;

private:
    DynArray<int> order_;     // topological order
    DynArray<int> arrival_;   // per gate
    DynArray<int> pred_;      // per gate: driver on the longest path, or INVALID
    DynArray<int> loop_;      // gates left over when a loop blocks Kahn's algorithm
    int endpoint_;
    int delay_;

    bool buildOrder(const Graph& g);
    void runDP(const Graph& g);
};

#endif
