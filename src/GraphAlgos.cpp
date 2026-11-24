// =============================================================================
// GraphAlgos.cpp -- Milestone 2: levelize() and findLoop()
// =============================================================================
#include "GraphAlgos.hpp"
#include "Queue.h"
#include "Stack.h"

bool levelize(const Graph& g, DynArray<int>& topoOrder, DynArray<int>& levels) {
    const int n = g.numGates();

    // Assumes topoOrder and levels are empty when passed in.
    DynArray<int> indeg;  // holds  number  of  gates  of  current gate "id"
    /// intializing
    for (int i = 0; i < n; i++) {
        levels.push_back(0);
        indeg.push_back(g.combInDegree(i));
    }

    Queue q; // ready  to  use  (source  gates)
    for (int i = 0; i < n; i++)
        if (indeg[i] == 0) q.enqueue(i);

    int u;
    while (q.front(u)) {
        q.dequeue();
        topoOrder.push_back(u);

        const Gate& gu = g.gate(u);
        for (int k = 0; k < gu.fanout.size(); k++) {
            int eid = gu.fanout[k];
            if (!g.isCombinationalEdge(eid)) continue;   // wire into a DFF (source)
            int v = g.edge(eid).to;
            if (levels[u] + 1 > levels[v]) levels[v] = levels[u] + 1;
            if (--indeg[v] == 0) q.enqueue(v);
        }
    }
    return topoOrder.size() == n;   // false => combinational loop
}

bool findLoop(const Graph& g, DynArray<int>& cycleGates) {
    const int n = g.numGates();
    enum { WHITE = 0, GRAY = 1, BLACK = 2 };

    DynArray<int> color, parent, nextEdge;
    for (int i = 0; i < n; i++) {
        color.push_back(WHITE);
        parent.push_back(-1);
        nextEdge.push_back(0);
    }

    Stack st;
    for (int root = 0; root < n; root++) {
        if (color[root] != WHITE) continue;
        color[root] = GRAY;
        st.push(root);

        int u;
        while (st.top(u)) {
            const Gate& gu = g.gate(u);
            if (nextEdge[u] < gu.fanout.size()) {
                int eid = gu.fanout[nextEdge[u]++];
                if (!g.isCombinationalEdge(eid)) continue;
                int v = g.edge(eid).to;

                if (color[v] == WHITE) {
                    color[v] = GRAY;
                    parent[v] = u;
                    st.push(v);
                } else if (color[v] == GRAY) {
                    // back edge u -> v: cycle is v -> ... -> u -> v
                    DynArray<int> rev;
                    for (int x = u; x != v; x = parent[x]) rev.push_back(x);
                    rev.push_back(v);
                    for (int i = rev.size() - 1; i >= 0; i--)
                        cycleGates.push_back(rev[i]);
                    return true;
                }
            } else {
                color[u] = BLACK;
                st.pop();
            }
        }
    }
    return false;
}
