#include "EventSim.hpp"
#include <cstdio>

//prints a net name without the "$out" the parser adds to output probes
static void printNetName(const char* name) {
    for (int i = 0; name[i] != '\0' && name[i] != '$'; i++)
        putchar(name[i]);
}

int eventQueueCapacity(const Graph& g, int numStim) {
    int cap = numStim;
    for (int i = 0; i < g.numGates(); i++)
        cap += g.gate(i).delay + 1;
    return cap;
}

EventSim::EventSim(const Graph& graph)
    : g(graph), state(graph), processed(0), lastTime(0), lastCapacity(0), peakSize(0) {
    projected.resize(g.numGates(), 'X');
}

bool EventSim::run(const DynArray<StimEvent>& stim, int maxEvents) {
    //start clean so run() can be called again
    state.reset();
    scheduled.clear();
    affected.clear();
    log.clear();
    for (int i = 0; i < projected.size(); i++)
        projected[i] = 'X';
    processed = 0;
    lastTime = 0;
    peakSize = 0;

    //the queue is made here since its capacity depends on the stimulus
    lastCapacity = eventQueueCapacity(g, stim.size());
    EventQueue q(lastCapacity);

    //every input change becomes an event, fifo keeps the file order for equal times
    for (int i = 0; i < stim.size(); i++)
        q.push(stim[i].time, stim[i].gate, stim[i].val);
    peakSize = q.size();

    while (!q.isEmpty()) {
        int t = q.nextTime(); //current time step

        //step 1: apply every event at time t
        while (!q.isEmpty() && q.nextTime() == t) {
            Event e = q.pop();
            processed++;
            if (processed > maxEvents) {
                printf("warning: event limit (%d) reached at time %d, simulation stopped\n", maxEvents, t);
                return false;
            }

            int net = e.getNetId();
            if (state.value[net] == e.getVal())
                continue; //no change so nothing downstream changes

            state.value[net] = e.getVal();
            log.push_back(ValueChange(t, net, e.getVal()));
            lastTime = t;

            //mark every gate this net feeds, insert() returns false if it was already marked
            const Gate& driver = g.gate(net);
            for (int k = 0; k < driver.fanout.size(); k++) {
                int to = g.edge(driver.fanout[k]).to;
                if (scheduled.insert(to))
                    affected.push_back(to);
            }
        }

        //step 2: evaluate each marked gate once
        for (int j = 0; j < affected.size(); j++) {
            int id = affected[j];
            if (g.gate(id).type == GATE_DFF)
                continue; //sequential logic comes in the final milestone

            char newVal = state.evaluate(id);

            //only push if its different from what the gate is already going to become
            if (newVal != projected[id]) {
                if (!q.push(t + g.gate(id).delay, id, newVal))
                    return false; //queue full, error already printed
                projected[id] = newVal;
            }
        }
        if (q.size() > peakSize)
            peakSize = q.size();

        //ready for the next time step
        affected.clear();
        scheduled.clear();
    }
    return true;
}

char EventSim::valueAt(int gate, int time) const {
    char v = 'X'; //every net starts unknown
    for (int i = 0; i < log.size() && log[i].getTime() <= time; i++)
        if (log[i].getGate() == gate)
            v = log[i].getVal();
    return v;
}

void EventSim::print() const {
    printf("Event-driven simulation (%d events, last change at %d):\n", processed, lastTime);
    printf("  time\toutput\tvalue\n");
    for (int i = 0; i < log.size(); i++) {
        int gate = log[i].getGate();
        if (g.gate(gate).type != GATE_OUTPUT)
            continue;
        printf("  %d\t", log[i].getTime());
        printNetName(g.gate(gate).name);
        printf("\t%c\n", log[i].getVal());
    }
}

//design decisions:
//all events of a time step are applied before evaluating, so a gate with two changing inputs sees both and is evaluated once
//the hash set checks "already marked?" in O(1) but cant be looped over, so marked gates also go in the affected list
//projected is compared instead of the current value because older events for the gate may still be waiting
//transport delay: every scheduled change happens, so glitches show in the waveform
//zero delay gates push events at the same t, they are handled in another round with the same t
//the queue has a fixed capacity worked out before the run, so it never grows
//changes go in a log instead of straight to the VCD, so the sim doesnt depend on the output format
//maxEvents stops a bad circuit from running forever

//complexity:
//each event is one push and one pop, O(log k), so O(k log k) for k events
//hash set insert O(1) average, valueAt O(changes)
//space O(V + capacity + changes)
