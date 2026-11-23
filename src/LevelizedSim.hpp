#ifndef LEVELIZEDSIM_HPP
#define LEVELIZEDSIM_HPP

#include "Graph.hpp"
#include "DynArray.hpp"
#include "SimState.hpp"
#include "SimParser.hpp"

//levelized simulation: for every input vector it evaluates every gate once in topological order and ignores delays
//an input vector = all the input changes that share one time in the .stim file

class LevelizedSim {
private:
    const Graph& g;
    SimState state; //net values + evaluation counter
    DynArray<int> outs; //ids of the output probes, in file order
    DynArray<int> vecTime; //vecTime[v] = time of vector v
    DynArray<char> results; //outputs after each vector, results[v * outs.size() + k]

public:
    LevelizedSim(const Graph& graph);

    LevelizedSim(const LevelizedSim&) = delete;
    LevelizedSim& operator=(const LevelizedSim&) = delete;

    //runs every vector in stim, order must be a topological order with every gate once
    void run(const DynArray<StimEvent>& stim, const DynArray<int>& order);

    int evaluations() const { return state.evalCount; }
    int numVectors() const { return vecTime.size(); }
    int numOutputs() const { return outs.size(); }
    int vectorTime(int v) const { return vecTime[v]; }
    int outputGate(int k) const { return outs[k]; }

    //value of output k after vector v
    char outputValue(int v, int k) const { return results[v * outs.size() + k]; }

    //prints one row per vector with the time and every output
    void print() const;
};

#endif
