// Milestone 1 data-structure regression tests.
#include "DynArray.hpp"
#include "EventQueue.h"
#include "hashmap_final.hpp"
#include "Hashset.h"
#include "Graph.hpp"
#include <cstdio>

static int failures = 0;
#define CHECK(expr) do { if (!(expr)) { std::printf("FAIL line %d: %s\n", __LINE__, #expr); ++failures; } } while (0)

static void testEventQueueFifoTieBreak() {
    EventQueue q(16);
    q.push(10, 1, '0');
    q.push(5, 2, '1');
    q.push(10, 3, 'X');
    q.push(10, 4, 'Z');
    CHECK(q.size() == 4);
    CHECK(q.pop().getNetId() == 2); // earliest timestamp first
    CHECK(q.pop().getNetId() == 1); // FIFO for equal timestamps
    CHECK(q.pop().getNetId() == 3);
    CHECK(q.pop().getNetId() == 4);
    CHECK(q.isEmpty());
}

static void testHashMapRehash() {
    HashMap<int> map(8);
    const int count = 100;
    char key[32];
    for (int i = 0; i < count; ++i) {
        std::snprintf(key, sizeof key, "net_%d", i);
        CHECK(map.put(key, i));
    }
    CHECK(map.bucketCount() > 8);
    CHECK(map.size() == count);
    for (int i = 0; i < count; ++i) {
        std::snprintf(key, sizeof key, "net_%d", i);
        int* value = map.get(key);
        CHECK(value != nullptr);
        if (value) CHECK(*value == i);
    }
    CHECK(!map.put("net_5", 500)); // existing key is updated, not inserted
    CHECK(*map.get("net_5") == 500);
}

static void testHashSetInsertRemoveClear() {
    GateHashSet set(8);
    CHECK(set.insert(10));
    CHECK(!set.insert(10));
    CHECK(set.insert(20));
    CHECK(set.contains(10));
    CHECK(set.contains(20));
    CHECK(set.size() == 2);
    CHECK(set.remove(10));
    CHECK(!set.contains(10));
    CHECK(!set.remove(10));
    CHECK(set.contains(20));
    set.clear();
    CHECK(set.empty());
    CHECK(set.size() == 0);
    CHECK(!set.contains(20));
    CHECK(set.insert(30)); // usable after clear
    CHECK(set.contains(30));
}

static void testGraphRejectsAlreadyDrivenPin() {
    Graph g;
    int a = g.addGate("a", GATE_INPUT, 0);
    int b = g.addGate("b", GATE_INPUT, 0);
    int n = g.addGate("n", GATE_AND, 1);
    CHECK(g.addEdge(a, n, 0) != INVALID);
    CHECK(g.addEdge(b, n, 0) == INVALID); // pin 0 already has a driver
    CHECK(g.addEdge(b, n, 1) != INVALID);
    CHECK(g.inDegree(n) == 2);
}

int main() {
    testEventQueueFifoTieBreak();
    testHashMapRehash();
    testHashSetInsertRemoveClear();
    testGraphRejectsAlreadyDrivenPin();
    if (failures) std::printf("%d data-structure test(s) FAILED\n", failures);
    else std::printf("all data-structure tests passed\n");
    return failures ? 1 : 0;
}
