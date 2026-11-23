// tests/test_analysis.cpp -- Person 4: tests for TimingAnalyzer and VCDWriter
#include "Parser.hpp"
#include "TimingAnalyzer.hpp"
#include "VCDWriter.hpp"
#include <cstdio>
#include <cstring>

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

static void testFullAdder(const char* dir) {
    char file[256];
    snprintf(file, sizeof file, "%s/full_adder.net", dir);
    Parser p;
    CHECK(p.load(file));
    TimingAnalyzer ta;
    CHECK(ta.analyze(p.g));
    // a/b -> x1 (2) -> sum (2) -> sum$out (0)   = 4 ; x1 -> a2 (1) -> cout (1) = 4 as well
    CHECK(ta.criticalDelay() == 4);
    DynArray<int> path;
    ta.criticalPath(path);
    CHECK(path.size() >= 3);
    CHECK(p.g.gate(path[0]).type == GATE_INPUT);
    CHECK(p.g.gate(path.back()).type == GATE_OUTPUT);
    CHECK(ta.topoOrder().size() == p.g.numGates());
}

static void testChain() {          // in -> NOT(3) -> NOT(5) -> out : delay 8
    Graph g;
    int in = g.addGate("in", GATE_INPUT, 0);
    int n1 = g.addGate("n1", GATE_NOT, 3);
    int n2 = g.addGate("n2", GATE_NOT, 5);
    int o  = g.addGate("o$out", GATE_OUTPUT, 0);
    g.addEdge(in, n1, 0); g.addEdge(n1, n2, 0); g.addEdge(n2, o, 0);
    TimingAnalyzer ta;
    CHECK(ta.analyze(g));
    CHECK(ta.criticalDelay() == 8);
    CHECK(ta.criticalEndpoint() == o);
}

static void testLoop() {           // two NANDs cross-coupled -> loop
    Graph g;
    int a = g.addGate("a", GATE_INPUT, 0);
    int b = g.addGate("b", GATE_INPUT, 0);
    int n1 = g.addGate("n1", GATE_NAND, 1);
    int n2 = g.addGate("n2", GATE_NAND, 1);
    g.addEdge(a, n1, 0); g.addEdge(n2, n1, 1);
    g.addEdge(b, n2, 0); g.addEdge(n1, n2, 1);
    TimingAnalyzer ta;
    CHECK(!ta.analyze(g));
    CHECK(ta.loopGates().size() == 2);
}

static void testVcd() {
    Graph g;
    int a = g.addGate("a", GATE_INPUT, 0);
    int o = g.addGate("a$out", GATE_OUTPUT, 0);
    g.addEdge(a, o, 0);
    VCDWriter w;
    CHECK(w.open("test_analysis.vcd", g));
    w.change(0, a, '0');
    w.change(5, a, '0');    // no change -> ignored
    w.change(10, a, '1');
    w.change(10, o, '1');   // probe is not traced -> ignored
    w.close(20);
    FILE* f = fopen("test_analysis.vcd", "r");
    CHECK(f != nullptr);
    char buf[4096] = {0};
    size_t n = f ? fread(buf, 1, sizeof buf - 1, f) : 0;
    buf[n] = 0;
    if (f) fclose(f);
    CHECK(strstr(buf, "$var wire 1 ! a $end"));
    CHECK(!strstr(buf, "a$out"));
    CHECK(strstr(buf, "#10\n1!"));
    CHECK(!strstr(buf, "#5"));
    CHECK(strstr(buf, "#20"));
}

int main(int argc, char** argv) {
    testFullAdder(argc > 1 ? argv[1] : "tests");
    testChain();
    testLoop();
    testVcd();
    printf(failures ? "%d test(s) FAILED\n" : "all analysis tests passed\n", failures);
    return failures ? 1 : 0;
}
