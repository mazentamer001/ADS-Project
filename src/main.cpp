// =============================================================================
// main.cpp -- Person 4: command line interface
//
//   sim <circuit.net> [stimulus.stim] [-o waves.vcd]
//
// Flow: load netlist -> loop check -> (levelized sim) -> (event sim) -> timing
//       analysis -> comparison counts.
//
// The two simulators are written by Person 3. Until their headers are merged this
// file builds without them: the loop check, timing analysis and VCD output all
// work, and the stimulus is replayed into the VCD so the waveform path can be
// tested. Build with -DHAVE_SIMS once LevelizedSim/EventSim exist and fill in the
// marked TODO blocks.
// =============================================================================
#include "Parser.hpp"
#include "SimParser.hpp"
#include "TimingAnalyzer.hpp"
#include "VCDWriter.hpp"
#include <cstdio>
#include <cstring>

#ifdef HAVE_GRAPHALGOS
#include "GraphAlgos.hpp"     // Person 2: levelize(), findLoop()
#endif
#ifdef HAVE_SIMS
#include "LevelizedSim.hpp"   // Person 3
#include "EventSim.hpp"
#endif

static void usage(const char* prog) {
    fprintf(stderr, "usage: %s <circuit.net> [stimulus.stim] [-o waves.vcd]\n", prog);
}

// Names the gates that are stuck on a combinational loop.
static void reportLoop(const Graph& g, const TimingAnalyzer& ta) {
#ifdef HAVE_GRAPHALGOS

    DynArray<int> cycle;
    if (findLoop(g, cycle) && !cycle.empty()) {
        fprintf(stderr, "error: combinational loop detected:");
        for (int i = 0; i < cycle.size(); i++) {
            fprintf(stderr, " %s ->", g.gate(cycle[i]).name);
        }
        fprintf(stderr, " %s\n", g.gate(cycle[0]).name);
        return;
    }
#endif
    fprintf(stderr, "error: combinational loop detected. Gates involved or downstream of it:");
    for (int i = 0; i < ta.loopGates().size(); i++)
        fprintf(stderr, " %s", g.gate(ta.loopGates()[i]).name);
    fprintf(stderr, "\n");
}

int main(int argc, char** argv) {
    const char* netFile  = nullptr;
    const char* stimFile = nullptr;
    const char* vcdFile  = nullptr;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) { usage(argv[0]); return 1; }
            vcdFile = argv[++i];
        } else if (!netFile)  netFile  = argv[i];
        else if (!stimFile)   stimFile = argv[i];
        else { usage(argv[0]); return 1; }
    }
    if (!netFile) { usage(argv[0]); return 1; }

    // 1. load the netlist
    Parser p;
    if (!p.load(netFile)) return 1;
    printf("Loaded %s: %d gates, %d wires, %d inputs, %d outputs\n", netFile,
           p.g.numGates(), p.g.numEdges(), p.inputs.size(), p.outputs.size());

    // 2. loop check (TimingAnalyzer's Kahn pass doubles as the detector)
    TimingAnalyzer ta;
    if (!ta.analyze(p.g)) { reportLoop(p.g, ta); return 2; }
    printf("Loop check: no combinational loops\n");

    // 3. stimulus
    SimParser sp;
    if (stimFile) {
        if (!sp.load(stimFile, p)) return 1;
        printf("Loaded %s: %d input changes at %d time steps, last time %d\n",
               stimFile, sp.numEvents(), sp.numTimeSteps(), sp.lastTime());
    }

    // 4. simulation + waveform
    VCDWriter vcd;
    if (vcdFile && !vcd.open(vcdFile, p.g)) return 1;

#ifdef HAVE_SIMS
    // Person 3: both simulators run on the same stimulus.
    int endTime = sp.lastTime();
    if (stimFile) {
        // levelized: one pass over the topological order per input vector
        LevelizedSim lev(p.g);
        lev.run(sp.events, ta.topoOrder());
        lev.print();

        // event-driven: only affected gates, with delays
        EventSim ev(p.g);
        ev.run(sp.events);
        ev.print();

        // waveform: replay every recorded value change
        const DynArray<ValueChange>& ch = ev.changes();
        for (int i = 0; i < ch.size(); i++)
            vcd.change(ch[i].getTime(), ch[i].getGate(), ch[i].getVal());
        if (ev.endTime() > endTime) endTime = ev.endTime();

        // both sims must agree once the circuit settles: sample the event-driven
        // outputs just before the next input vector (or at the very end)
        int mismatches = 0;
        for (int v = 0; v < lev.numVectors(); v++) {
            int t = (v + 1 < lev.numVectors()) ? lev.vectorTime(v + 1) - 1 : ev.endTime();
            for (int k = 0; k < lev.numOutputs(); k++)
                if (ev.valueAt(lev.outputGate(k), t) != lev.outputValue(v, k)) mismatches++;
        }
        if (mismatches == 0) printf("Results match: both simulators give the same settled outputs\n");
        else printf("Results differ in %d place(s) (inputs may change before the circuit settles)\n", mismatches);

        printf("Levelized evaluations: %d     Event-driven evaluations: %d\n",
               lev.evaluations(), ev.evaluations());
    }
#else
    if (stimFile) {
        printf("Simulators not built in (compile with -DHAVE_SIMS): replaying inputs only\n");
        for (int i = 0; i < sp.events.size(); i++)
            vcd.change(sp.events[i].time, sp.events[i].gate, sp.events[i].val);
    }
#endif
#ifndef HAVE_SIMS
    int endTime = sp.lastTime();
#endif
    if (vcd.isOpen()) { vcd.close(endTime); printf("Waveform written to %s\n", vcdFile); }

    // 5. timing analysis
    ta.print(p.g);
    return 0;
}
