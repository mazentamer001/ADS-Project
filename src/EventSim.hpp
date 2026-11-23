#ifndef EVENTSIM_HPP
#define EVENTSIM_HPP

#include "Graph.hpp"
#include "DynArray.hpp"
#include "SimState.hpp"
#include "SimParser.hpp"
#include "EventQueue.h"
#include "Hashset.h"

//event driven simulation with gate delays, an event means at time t net n becomes v
//each time step: apply all events at t and mark the gates they feed (once, using the hash set), then evaluate each marked gate once and push its new value at t + delay

//size of the event queue = stimulus lines + sum of (delay + 1) over all gates
//all stimulus events are pushed at the start, and a gate with delay d has at most d events waiting at once (+1 to be safe)
int eventQueueCapacity(const Graph& g, int numStim);

//one value change on a net, saved for the waveform and for checking
class ValueChange {
private:
    int time;
    int gate; //gate id == net id
    char val;

public:
    //default values so DynArray can make an array of them
    ValueChange(int t = 0, int g = -1, char v = 'X') {
        time = t;
        gate = g;
        val = v;
    }

    int getTime() const { return time; }
    int getGate() const { return gate; }
    char getVal() const { return val; }
};

class EventSim {
private:
    const Graph& g;
    SimState state; //net values + evaluation counter
    GateHashSet scheduled; //gates already marked in this time step
    DynArray<int> affected; //the same gates as a list so we can loop over them
    DynArray<char> projected; //value each gate will have after its waiting events
    DynArray<ValueChange> log; //every real value change, in time order
    int processed; //events popped so far
    int lastTime; //time of the last real change
    int lastCapacity; //capacity the queue was made with
    int peakSize; //most events waiting at once

public:
    EventSim(const Graph& graph);

    EventSim(const EventSim&) = delete;
    EventSim& operator=(const EventSim&) = delete;

    //runs the whole stimulus, the queue is made inside with eventQueueCapacity()
    //returns false if it hit maxEvents or the queue was full
    bool run(const DynArray<StimEvent>& stim, int maxEvents = 1000000);

    int evaluations() const { return state.evalCount; }
    int eventsProcessed() const { return processed; }
    int queueCapacity() const { return lastCapacity; }
    int maxQueueSize() const { return peakSize; }
    int endTime() const { return lastTime; }
    const DynArray<ValueChange>& changes() const { return log; }

    //value of a net at a given time, read from the log
    char valueAt(int gate, int time) const;

    //prints every change on an output, in time order
    void print() const;
};

#endif
