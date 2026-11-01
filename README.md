# ADS-Project

# Event-Driven Gate-Level Logic Simulator and Timing Analyzer B4

CSCE 2200 – Applied Data Structures Project - (Team 10)

A simulator for digital circuits described as a netlist (AND, OR, NOT, XOR, NAND gates and D flip-flops, each with a delay). It propagates value changes as timed events, produces waveforms, and runs static timing analysis to find the critical path. All data structures are implemented from scratch, with no STL.

## Team

| Member | ID | Responsible for |
|--------|----|-----------------|
| _TBD_ | | Vector (`DynArray`) |
| _name_ | | Directed Graph |
| _TBD_ | | Stack |
| _TBD_ | | Queue |
| _TBD_ | | Hash Set |
| _TBD_ | | Hash Map |
| _TBD_ | | Priority Queue |

## Data Structures

| Structure | Used for | Status |
|-----------|----------|--------|
| Vector (`DynArray<T>`) | Base storage for everything else | Done |
| Directed Graph (adjacency list) | The circuit: gates are nodes, wires are edges | Done |
| Stack | DFS for combinational loop detection | _TBD_ |
| Queue | Kahn's algorithm (levelization) | _TBD_ |
| Hash Set | Gates already scheduled in the current time step | _TBD_ |
| Hash Map | Net name to gate id | _TBD_ |
| Priority Queue | Event queue ordered by time, FIFO among equal times | _TBD_ |

### 1. Vector (`DynArray<T>`)

A resizable array stored in one contiguous block of memory that grows when it becomes full. It is the storage layer everything else sits on: the graph's gate and edge arrays and each gate's fanin and fanout lists are all `DynArray`s, and the parser uses it while reading a circuit file because the number of gates and wires is not known in advance.

- **Operations:** `push_back`, `operator[]`, `size`, `empty`
- **Complexity:** access by index O(1); `push_back` O(1) amortized; insert or delete in the middle O(n); space O(n)
- **Why an array:** the project only appends and constantly accesses by index, and contiguous memory is cache-friendly. A linked list would make access by index O(n).
- **Caveat:** elements such as `Gate` own arrays of their own, so the copy constructor and assignment operator must make deep copies, otherwise two arrays share memory and free it twice.

### 2. Stack

Implementation: Dynamic array , with a top index tracking the top element.



Operations: push, pop, top, isEmpty, size.



Complexity: push O(1) amortized; pop O(1); top O(1); space O(n).



Why an array: Provides contiguous memory, efficient access, and amortized constant-time insertion.


### 3. Queue

Implementation: Circular dynamic array, with front, rear, and count tracking the elements.



Operations: enqueue, dequeue, front, isEmpty, size.



Complexity: enqueue O(1) amortized; dequeue O(1); front O(1); space O(n).



Why a circular array: Reuses available positions and avoids shifting elements after every dequeue.



### 4. Hash Set


### 5. Hash Map


### 6. Priority Queue (`EventQueue`)

The event queue for event-driven simulation. Each event says "at time t, net n changes to value v", and the simulator always takes the earliest event next.

- **Implementation:** binary min-heap stored in a dynamic array. `heap[0]` is always the earliest event; the parent of `i` is `(i-1)/2` and its children are `2i+1` and `2i+2`. The array starts at 16 and doubles when full.
- **Operations:** `push`, `pop`, `peek`, `nextTime`, `isEmpty`, `size`, `clear`
- **Complexity:** `push` O(log k); `pop` O(log k); `peek`, `nextTime`, `isEmpty`, `size`, `clear` O(1); space O(k). A full simulation with k events is O(k log k).
- **FIFO among equal times:** a heap is not stable on its own, so every event gets a `seq` number in the order it was pushed. Events are compared by time first, and by `seq` when times are equal.
- **Why a heap:** a sorted list would make every insert O(n). The heap gives O(log k) for both insert and removal.
- **Values:** `val` is a `char` (`'0'`, `'1'`, `'X'`, `'Z'`) so the queue already supports four-valued logic.
- **Empty queue:** `pop` and `peek` print an error and return an event with `time = -1` instead of crashing.

**Use in the project:** the simulator pops the earliest event, updates that net's value, walks the driving gate's fanout in the graph, and pushes a new event at `time + delay` for each receiving gate whose output changes. `netId` is the gate id the hash map returns for a net name. The hash set stops the same gate from being scheduled twice in one time step. Clock edges for flip-flops are also pushed as events.


### 7. Directed Graph (adjacency list)

The in-memory form of the circuit. Gates are nodes, and wires are directed edges from a driver gate into a specific input pin of a receiving gate.

- **Implementation:** two `DynArray`s, `gates_` and `edges_`, where the array index is the id. Each gate stores `fanin` and `fanout` lists of edge ids. Each edge stores `from`, `to`, and `pin`.
- **Operations:** `addGate`, `addEdge` (validates ids, pin numbers, duplicate drivers, and outputs used as drivers), `gate`, `edge`, `inDegree`, `outDegree`, `isCombinationalEdge`, `combInDegree`, `print`
- **Complexity:** `addGate` O(1) amortized; `addEdge` O(1) amortized plus an O(indegree) pin check (at most 2 entries); queries O(1); `combInDegree` O(indegree); `print` O(V + E); space O(V + E)
- **Why an adjacency list:** circuits are sparse (at most 2 inputs per gate), so an adjacency matrix would waste O(V²) space and could not store which pin a wire connects to. Making each wire its own object lets it carry its pin number.
- **Flip-flop handling:** `isCombinationalEdge` and `combInDegree` treat flip-flops as sources, so Kahn's algorithm cuts the circuit open at every DFF and feedback through a flip-flop (as in a counter) is not mistaken for an illegal loop.

**Use in the project:** the parser creates one gate per component and one edge per wire. DFS over the graph detects illegal combinational loops, and Kahn's algorithm assigns levels for levelized simulation. In event-driven simulation, when a gate's output changes, the simulator walks its fanout, uses each edge's `to` and `pin` to find which input changed, and schedules a new event after that gate's delay. Static timing analysis walks the gates in topological order using each gate's `delay` to find the critical path. Gate names are used for the VCD waveform output.

## Project Layout

```
DynArray.hpp     Vector
Graph.hpp        Directed graph: declarations and usage comments
Graph.cpp        Directed graph: implementation
Hashset.h        Hash Set
EventQueue.h     Priority queue (event queue)
```

## Milestones

- [x] Milestone 1: data structures and initial plan (Tuesday, 3 November 2026)
- [ ] Milestone 2: application and design documentation (Wednesday, 24 November 2026)
- [ ] Final submission and presentation (Sunday, 20 December 2026)
