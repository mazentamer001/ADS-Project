// =============================================================================
// VCDWriter.cpp -- see VCDWriter.hpp
// =============================================================================
#include "VCDWriter.hpp"
#include "Logic.hpp"

// Identifier = base-94 number written with the printable characters '!'..'~'.
void VCDWriter::idCode(int n, char* out) {
    int i = 0;
    do { out[i++] = (char)('!' + n % 94); n /= 94; } while (n > 0);
    out[i] = '\0';
}

static char vcdChar(char v) {          // VCD wants lowercase x / z
    return v == 'X' ? 'x' : v == 'Z' ? 'z' : v;
}

bool VCDWriter::open(const char* filename, const Graph& g, const char* topName) {
    close(-1);
    f_ = fopen(filename, "w");
    if (!f_) { fprintf(stderr, "cannot write '%s'\n", filename); return false; }

    int n = g.numGates();
    last_.clear();   last_.resize(n, 'X');
    traced_.clear(); traced_.resize(n, 0);
    curTime_ = 0;

    fprintf(f_, "$version ADS-Project logic simulator $end\n");
    fprintf(f_, "$timescale 1ns $end\n");
    fprintf(f_, "$scope module %s $end\n", topName);
    char id[8];
    for (int i = 0; i < n; i++) {
        if (g.gate(i).type == GATE_OUTPUT) continue;      // probe repeats the net it watches
        traced_[i] = 1;
        idCode(i, id);
        fprintf(f_, "$var wire 1 %s %s $end\n", id, g.gate(i).name);
    }
    fprintf(f_, "$upscope $end\n$enddefinitions $end\n");

    fprintf(f_, "#0\n$dumpvars\n");
    for (int i = 0; i < n; i++) {
        if (!traced_[i]) continue;
        idCode(i, id);
        fprintf(f_, "x%s\n", id);
    }
    fprintf(f_, "$end\n");
    return true;
}

void VCDWriter::change(int time, int gate, char val) {
    if (!f_ || gate < 0 || gate >= last_.size() || !traced_[gate]) return;
    char v = normalizeValue(val);
    if (v == 0 || v == last_[gate]) return;               // not a logic value, or nothing changed
    if (time < curTime_) time = curTime_;                 // times must never go backwards
    if (time > curTime_) { fprintf(f_, "#%d\n", time); curTime_ = time; }
    char id[8];
    idCode(gate, id);
    fprintf(f_, "%c%s\n", vcdChar(v), id);
    last_[gate] = v;
}

void VCDWriter::close(int endTime) {
    if (!f_) return;
    if (endTime > curTime_) fprintf(f_, "#%d\n", endTime);
    fclose(f_);
    f_ = nullptr;
}
