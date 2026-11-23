#ifndef SIMSTATE_HPP
#define SIMSTATE_HPP

#include "Graph.hpp"
#include "DynArray.hpp"
#include "Logic.hpp"

//the state both simulators share: the value on every net and the evaluation counter, so both sims count evaluations the same way
//net == gate id (every gate drives one net), so the value of net n is just value[n]

class SimState {
private:
    const Graph& g; //the circuit, only read

public:
    DynArray<char> value; //value[id] = current value of the net driven by gate id
    int evalCount; //number of gate evaluations since the last reset

    //every net starts as X (unknown)
    SimState(const Graph& graph) : g(graph), evalCount(0) {
        value.resize(g.numGates(), 'X');
    }

    //holds a reference to the graph so copying is turned off
    SimState(const SimState&) = delete;
    SimState& operator=(const SimState&) = delete;

    //back to all X and zero evaluations so it can run again
    void reset() {
        for (int i = 0; i < value.size(); i++)
            value[i] = 'X';
        evalCount = 0;
    }

    //value arriving at pin "pin" of gate id, found through the gate's fanin edges, X if nothing is connected
    char inputValue(int id, int pin) const {
        const Gate& gate = g.gate(id);
        for (int k = 0; k < gate.fanin.size(); k++) {
            const Edge& e = g.edge(gate.fanin[k]);
            if (e.pin == pin)
                return value[e.from];
        }
        return 'X';
    }

    //new output of gate id from its current inputs, counts the evaluation but doesnt store the result
    char evaluate(int id) {
        evalCount++;
        const Gate& gate = g.gate(id);
        return evalGate(gate.type, inputValue(id, 0), inputValue(id, 1));
    }
};

//design decisions:
//both sims evaluate through evaluate() so the evaluation counts in the report are fair
//evaluate() returns the value instead of storing it, levelized stores it right away and event driven schedules it after the delay

//complexity:
//inputValue and evaluate O(1), a gate has at most 2 fanin edges
//reset O(V), space O(V)
#endif
