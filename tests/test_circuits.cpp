// Circuit-file regression tests: supported combinational circuits, sequential
// structure parsing, and deliberate combinational-loop rejection.
#include "Parser.hpp"
#include "TimingAnalyzer.hpp"
#include <cstdio>
#include <cstring>

static int failures = 0;
#define CHECK(expr) do { if (!(expr)) { std::printf("FAIL line %d: %s\n", __LINE__, #expr); ++failures; } } while (0)

static void testAcyclic(const char* filename) {
    Parser p;
    CHECK(p.load(filename));
    TimingAnalyzer ta;
    CHECK(ta.analyze(p.g));
    CHECK(ta.topoOrder().size() == p.g.numGates());
    CHECK(ta.criticalDelay() >= 0);
}

static void testLoopRejected() {
    Parser p;
    CHECK(p.load("tests/loop_error.net"));
    TimingAnalyzer ta;
    CHECK(!ta.analyze(p.g));
    CHECK(!ta.loopGates().empty());
}

int main() {
    testAcyclic("tests/full_adder.net");
    testAcyclic("tests/ripple8.net");
    testAcyclic("tests/cla8.net");
    testAcyclic("tests/counter.net");
    testLoopRejected();
    if (failures) std::printf("%d circuit test(s) FAILED\n", failures);
    else std::printf("all circuit tests passed\n");
    return failures ? 1 : 0;
}
