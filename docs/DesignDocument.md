# Design Document

## 1. Project purpose and scope

The project is an event-oriented gate-level circuit tool. It reads a structural netlist, stores the circuit as a directed graph, accepts time-stamped primary-input changes, checks for combinational feedback, calculates the longest propagation-delay path, and can write a Value Change Dump (VCD) waveform.

The implementation is intentionally based on custom data structures rather than the C++ Standard Template Library containers. The current codebase includes the parser, graph, data structures, timing analyzer, and VCD writer. The command-line program has placeholders for the full levelized and event-driven simulation engines; DFF state updates are not yet implemented.

## 2. Architecture

### 2.1 Input and parsing layer

- `Parser` reads a `.net` file in two passes.
- Pass 1 validates declarations, creates gate nodes, and records `net name -> gate id` in `HashMap<int>`.
- Pass 2 resolves input names and creates graph edges. This permits a gate to refer to a net declared later in the file.
- `SimParser` reads `.stim` changes, validates timestamps and logic values, and resolves each input name through the parser.
- `ParseUtil.hpp` contains tokenization, case-insensitive keyword comparison, integer parsing, and formatted parse errors.

### 2.2 Circuit representation

`Graph` stores one `Gate` per node and one `Edge` per wire. Every gate owns lists of incoming and outgoing edge IDs. Each edge stores its source, destination, and destination pin number. Gate IDs are array indices and therefore remain stable as logical identifiers even if the backing dynamic array reallocates.

`Graph::addEdge` rejects invalid gate IDs, output nodes used as drivers, nonexistent pin numbers, and input pins that already have a driver.

### 2.3 Analysis and output

`TimingAnalyzer` uses Kahn's topological algorithm to order combinational dependencies. If not every gate can be ordered, a combinational loop exists. Once ordered, dynamic programming computes each gate's arrival time:

`arrival[g] = gate_delay[g] + max(arrival[driver])`

The analyzer stores the predecessor that produced the latest arrival, allowing it to reconstruct the critical path. Primary output probes are preferred as endpoints.

`VCDWriter` emits a VCD header and records value changes supplied by the caller. It omits output-probe duplicates and ignores repeated values.

### 2.4 Sequential boundary behavior

A DFF has D on pin 0 and CLK on pin 1. Edges entering a DFF are treated as non-combinational by `Graph::isCombinationalEdge`. Consequently, a feedback path passing through a DFF does not count as a combinational loop. The parser accepts DFF structural netlists for analysis, but the current simulator does not compute next-state values.

## 3. Data-structure and algorithm table

| Structure / algorithm | Implementation and role | Main operations | Complexity |
|---|---|---|---|
| `DynArray<T>` | Contiguous dynamically resized array; backing storage for gates, edges, and adjacency lists | indexed access, `push_back`, `resize`, `clear` | access O(1); append O(1) amortized, O(n) resize |
| `Graph` | Directed adjacency-list graph; gates are vertices and wires are edges | `addGate`, `addEdge`, degree and edge queries | add gate O(1) amortized; add edge O(1) amortized plus small pin scan |
| `Queue` | Circular dynamic array used by graph/topological workflows | enqueue, dequeue, front | enqueue O(1) amortized; dequeue/front O(1) |
| `Stack` | Dynamic array used by iterative depth-first traversal workflows | push, pop, top | push O(1) amortized; pop/top O(1) |
| `HashMap<T>` | Separate chaining; net name to gate ID | put, get, contains, removekey, clearnodes | average O(1); rehash O(n) |
| `GateHashSet` | Hash buckets of gate IDs; avoids duplicate scheduled gate IDs | insert, contains, remove, clear | average O(1); rehash O(n) |
| `EventQueue` | Binary min-heap ordered by event time, then insertion sequence | push, pop, peek, nextTime | push/pop O(log k); peek O(1) |
| Kahn topological order | Queue-based indegree reduction over combinational edges | build order and detect incomplete order | O(V + E) |
| Critical-path DP | One pass over topological order; records arrival and predecessor | arrival-time update and path reconstruction | O(V + E) |

Here, `V` is the number of gates, `E` is the number of wires, and `k` is the number of queued events.

## 4. Design trade-offs

### 4.1 Dynamic array instead of linked list

Gate and edge IDs are direct array indices, and the application frequently performs indexed access. A dynamic array provides O(1) indexing and good locality. A linked list would make ID lookup slower and add pointer overhead. The trade-off is occasional O(n) copying during capacity growth.

### 4.2 Adjacency list instead of adjacency matrix

Digital circuits are sparse: each gate has only a small number of input pins. An adjacency matrix would require O(V²) storage and would not naturally store the destination pin number. The adjacency-list graph stores each wire once and records its edge ID at both endpoints, using O(V + E) space.

### 4.3 Separate chaining for hash structures

The HashMap uses linked nodes in buckets, while GateHashSet uses dynamic-array buckets. Both handle collisions without requiring a fixed maximum number of keys. Rehashing grows the bucket array when the load factor passes the configured threshold. The trade-off is an occasional O(n) rehash in exchange for average constant-time lookup and insertion.

### 4.4 Binary heap and stable event ordering

A sorted event list would make insertion O(k). The binary min-heap provides O(log k) insertion and removal. A heap does not inherently preserve insertion order for equal priorities, so `EventQueue` assigns a monotonically increasing sequence number and compares `(time, seq)`. This gives FIFO behavior among events at the same time.

### 4.5 Kahn's algorithm for loop detection and timing order

A topological order is required before longest-path dynamic programming can be evaluated safely. Kahn's algorithm processes zero-indegree gates and decrements the indegree of their combinational consumers. If fewer than V gates are processed, at least one combinational cycle remains. It provides O(V + E) time and reuses the queue abstraction.

### 4.6 DFF edges as timing boundaries

A DFF stores state, so its input is not a same-cycle combinational dependency. Ignoring edges that terminate at DFFs allows timing analysis to treat sequential feedback as a boundary rather than incorrectly reporting it as a combinational loop. This does not by itself implement sequential simulation.

## 5. Verification plan

`make test` runs three test executables:

1. `test_ds`: checks equal-time FIFO ordering in EventQueue, HashMap rehash, HashSet insert/remove/clear, and Graph rejection of a second driver on the same pin.
2. `test_analysis`: checks full-adder timing, a simple two-gate chain, combinational-loop detection, and VCD output.
3. `test_circuits`: parses and analyzes the full adder, ripple8, cla8, and structural counter, and confirms that `loop_error.net` is rejected by timing analysis.

The stimulus files provide input changes for the combinational examples and a clock sequence for the structural counter. The counter's expected behavior is documented as a reference only; DFF state simulation is outside the current implementation.

## 6. Known limitations and next work

- Integrate complete levelized and event-driven simulation modules into `main.cpp`.
- Implement DFF clock-edge and next-state behavior before claiming a simulated counter.
- Add end-to-end output comparison that writes actual simulated values and compares them automatically with `tests/expected/`.
- Consolidate the older duplicate graph-algorithm source files if those algorithms are brought into the active build.
