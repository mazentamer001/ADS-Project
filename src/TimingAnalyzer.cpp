// =============================================================================
// TimingAnalyzer.cpp -- see TimingAnalyzer.hpp
// =============================================================================
#include "TimingAnalyzer.hpp"
#include "Queue.h"
#include <cstdio>

// Kahn's algorithm. Cost O(V + E).
bool TimingAnalyzer::buildOrder(const Graph& g) {
    int n = g.numGates();
    order_.clear();
    loop_.clear();

    DynArray<int> indeg;
    indeg.resize(n, 0);
    Queue q;
    for (int i = 0; i < n; i++) {
        indeg[i] = g.combInDegree(i);
        if (indeg[i] == 0) q.enqueue(i);
    }

    int u;
    while (q.front(u)) {
        q.dequeue();
        order_.push_back(u);
        const Gate& gt = g.gate(u);
        for (int k = 0; k < gt.fanout.size(); k++) {
            int e = gt.fanout[k];
            if (!g.isCombinationalEdge(e)) continue;
            int v = g.edge(e).to;
            if (--indeg[v] == 0) q.enqueue(v);
        }
    }

    if (order_.size() == n) return true;
    for (int i = 0; i < n; i++)           // gates still waiting = on a loop or downstream of one
        if (indeg[i] > 0) loop_.push_back(i);
    return false;
}

// arrival[g] = delay[g] + max(arrival[driver]); walking in topological order
// guarantees every driver is finished before g is looked at.
void TimingAnalyzer::runDP(const Graph& g) {
    int n = g.numGates();
    arrival_.clear();  arrival_.resize(n, 0);
    pred_.clear();     pred_.resize(n, INVALID);

    for (int i = 0; i < order_.size(); i++) {
        int u = order_[i];
        const Gate& gt = g.gate(u);
        int best = 0, bestDriver = INVALID;
        for (int k = 0; k < gt.fanin.size(); k++) {
            int e = gt.fanin[k];
            if (!g.isCombinationalEdge(e)) continue;      // DFF inputs start no path through the DFF
            int d = g.edge(e).from;
            if (bestDriver == INVALID || arrival_[d] > best) { best = arrival_[d]; bestDriver = d; }
        }
        arrival_[u] = gt.delay + (bestDriver == INVALID ? 0 : best);
        pred_[u] = bestDriver;
    }

    // latest endpoint: prefer primary outputs; fall back to all gates
    endpoint_ = INVALID; delay_ = 0;
    bool hasOutput = false;
    for (int i = 0; i < n; i++) if (g.gate(i).type == GATE_OUTPUT) { hasOutput = true; break; }
    for (int i = 0; i < n; i++) {
        if (hasOutput && g.gate(i).type != GATE_OUTPUT) continue;
        if (endpoint_ == INVALID || arrival_[i] > delay_) { endpoint_ = i; delay_ = arrival_[i]; }
    }
}

bool TimingAnalyzer::analyze(const Graph& g) {
    if (!buildOrder(g)) return false;
    runDP(g);
    return true;
}

bool TimingAnalyzer::analyze(const Graph& g, const DynArray<int>& order) {
    if (order.size() != g.numGates()) return false;
    order_ = order;
    loop_.clear();
    runDP(g);
    return true;
}

void TimingAnalyzer::criticalPath(DynArray<int>& path) const {
    path.clear();
    DynArray<int> rev;
    for (int u = endpoint_; u != INVALID; u = pred_[u]) rev.push_back(u);
    for (int i = rev.size() - 1; i >= 0; i--) path.push_back(rev[i]);
}

void TimingAnalyzer::print(const Graph& g) const {
    if (endpoint_ == INVALID) { printf("Timing: circuit is empty\n"); return; }
    DynArray<int> path;
    criticalPath(path);
    printf("Critical path (delay %d):\n  ", delay_);
    for (int i = 0; i < path.size(); i++) {
        printf("%s(%d)%s", g.gate(path[i]).name, arrival_[path[i]], i + 1 < path.size() ? " -> " : "\n");
    }
}
