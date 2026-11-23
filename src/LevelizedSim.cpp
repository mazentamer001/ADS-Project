#include "LevelizedSim.hpp"
#include <cstdio>

//prints a net name without the "$out" the parser adds to output probes
static void printNetName(const char* name) {
    for (int i = 0; name[i] != '\0' && name[i] != '$'; i++)
        putchar(name[i]);
}

//find the output probes once so we know which values to save
LevelizedSim::LevelizedSim(const Graph& graph) : g(graph), state(graph) {
    for (int i = 0; i < g.numGates(); i++)
        if (g.gate(i).type == GATE_OUTPUT)
            outs.push_back(i);
}

void LevelizedSim::run(const DynArray<StimEvent>& stim, const DynArray<int>& order) {
    state.reset();
    vecTime.clear();
    results.clear();

    int i = 0; //next stimulus line
    while (i < stim.size()) {
        int t = stim[i].time;

        //1. apply every input change of this vector
        while (i < stim.size() && stim[i].time == t) {
            state.value[stim[i].gate] = stim[i].val;
            i++;
        }

        //2. evaluate every gate once in topological order, inputs are skipped and DFFs arent supported yet
        for (int k = 0; k < order.size(); k++) {
            int id = order[k];
            GateType type = g.gate(id).type;
            if (type == GATE_INPUT || type == GATE_DFF)
                continue;
            state.value[id] = state.evaluate(id);
        }

        //3. save the outputs of this vector
        vecTime.push_back(t);
        for (int k = 0; k < outs.size(); k++)
            results.push_back(state.value[outs[k]]);
    }
}

void LevelizedSim::print() const {
    printf("Levelized simulation (%d vectors):\n  time", numVectors());
    for (int k = 0; k < outs.size(); k++) {
        printf("\t");
        printNetName(g.gate(outs[k]).name);
    }
    printf("\n");
    for (int v = 0; v < numVectors(); v++) {
        printf("  %d", vecTime[v]);
        for (int k = 0; k < outs.size(); k++)
            printf("\t%c", outputValue(v, k));
        printf("\n");
    }
}

//design decisions:
//topological order means every gate's drivers are done before it, so one pass gives the settled result
//the order is passed in, not computed here, so this file only simulates (it can come from levelize() or the TimingAnalyzer)
//every gate is evaluated even if nothing changed, thats what the comparison with the event driven sim shows
//results are one flat array (v * numOutputs + k), one allocation and easy to index

//complexity:
//O(V + E) per vector, O(vectors * (V + E)) in total
//space O(V + vectors * outputs)
