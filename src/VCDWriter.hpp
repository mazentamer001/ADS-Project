// =============================================================================
// VCDWriter.hpp -- Person 4: writes a Value Change Dump file for GTKWave
//
// Usage (the event simulator only needs these three calls):
//     VCDWriter vcd;
//     vcd.open("out.vcd", graph);          // header: one signal per net
//     vcd.change(time, gateId, '1');       // call for every value change, times non-decreasing
//     vcd.close(endTime);                  // also called by the destructor
//
// One signal per gate, named after its net. GATE_OUTPUT probes ("x$out") are
// skipped because they just repeat the net x. Everything starts as X at time 0.
// Changes that do not alter the value are ignored, so the simulator may call
// change() freely. Values: '0' '1' 'X' 'Z' (any case).
// =============================================================================
#ifndef VCDWRITER_HPP
#define VCDWRITER_HPP

#include "Graph.hpp"
#include "DynArray.hpp"
#include <cstdio>

class VCDWriter {
public:
    VCDWriter() : f_(nullptr), curTime_(0) {}
    ~VCDWriter() { close(-1); }

    // non-copyable (owns a FILE*)
    VCDWriter(const VCDWriter&) = delete;
    VCDWriter& operator=(const VCDWriter&) = delete;

    bool open(const char* filename, const Graph& g, const char* topName = "top");
    void change(int time, int gate, char val);
    void close(int endTime);      // endTime < 0: no final timestamp
    bool isOpen() const { return f_ != nullptr; }

private:
    FILE*         f_;
    int           curTime_;
    DynArray<char> last_;      // last written value per gate
    DynArray<char> traced_;    // 1 if the gate has a signal in the file

    static void idCode(int n, char* out);   // VCD short identifier for signal number n
};

#endif
