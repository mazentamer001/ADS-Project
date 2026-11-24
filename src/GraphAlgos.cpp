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

    Queue q; // ready  to  use gates (source  gates)
    for (int i = 0; i < n; i++)
        if (indeg[i] == 0) q.enqueue(i);

    int u;  // upstream  gate
    while (q.front(u)) {
       // pushing  ready  gates  to  topology order
        q.dequeue();
        topoOrder.push_back(u);

        const Gate& gu = g.gate(u);
        for (int k = 0; k < gu.fanout.size(); k++) {
            int eid = gu.fanout[k];
            if (!g.isCombinationalEdge(eid)) continue;   // wire into a DFF (source)
            int v = g.edge(eid).to; // downstream  gates 
            //depth evaluation 
            if (levels[u] + 1 > levels[v]) levels[v] = levels[u] + 1;
            if (--indeg[v] == 0) q.enqueue(v);
        }
    }
    return topoOrder.size() == n;   // false => combinational loop
}

bool findLoop(const Graph& g, DynArray<int>& cycleGates) {
    const int n = g.numGates();
    enum { WHITE = 0, GRAY = 1, BLACK = 2 }; // gate states white= unvisited , gray= working on it, black= finished

    DynArray<int> color, parent, nextEdge; 
    // init
    
    for (int i = 0; i < n; i++) {
        color.push_back(WHITE);
        parent.push_back(-1);
        nextEdge.push_back(0);
    }

    Stack st; // dfs  stack  search
    
    for (int root = 0; root < n; root++) {
        if (color[root] != WHITE) continue;
        color[root] = GRAY; // currenlty  workin  on it 
        st.push(root);

        int u;
        while (st.top(u)) {
            const Gate& gu = g.gate(u);
            // keeping  track  of  edges unil  we over  them  all
            if (nextEdge[u] < gu.fanout.size()) {
                int eid = gu.fanout[nextEdge[u]++];
                if (!g.isCombinationalEdge(eid)) continue;
                int v = g.edge(eid).to;
                // cheking down stream  gates 
                if (color[v] == WHITE) {
                    color[v] = GRAY;
                    parent[v] = u;
                    st.push(v);
                } else if (color[v] == GRAY) {
                    // back edge u -> v: goes bakc to  v
                    DynArray<int> rev;
                 int x = u;
                    // cycle  detectoin backwardly
        while (x != v) {
            rev.push_back(x);
                x = parent[x];
                            }
                    rev.push_back(v);
                    // feeding  gates
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
