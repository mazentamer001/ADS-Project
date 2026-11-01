#ifndef GRAPH_HPP
#define GRAPH_HPP

#include "DynArray.hpp"

//Every kind of node the graph can contain
enum GateType {
    GATE_INPUT,   //primary input
    GATE_OUTPUT,  //primary output
    GATE_NOT,
    GATE_AND,
    GATE_OR,
    GATE_XOR,
    GATE_NAND,
    GATE_DFF      //D flip-flop: pin 0 = D, pin 1 = CLK
};

const int MAX_NAME_LEN = 32;  //instead of using string we set a limit to charactar array
const int INVALID = -1;   //returned when an operation fails

//free helper functions about gate types
const char* gateTypeName(GateType t);   //printable name
bool isSequential(GateType t);          //true for state-holding gates (DFF)
int  numInputPins(GateType t);          //how many input pins the type has

//one wire, example: Wire A -> AND  :  Edge { from = 0, to = 2, pin = 0 }
struct Edge {
    int from;  //who sends the signal?
    int to;    //who recieves it?
    int pin;   //which input slot on the reciever?
};

//one node of the graph.
struct Gate {
    int  id;                    //index of this gate in the graph (to be more exact its the index in the dynamic array inside the graph)
    char name[MAX_NAME_LEN];    //name of the gate
    GateType type;
    int delay;                  //propagation delay
    DynArray<int> fanin;        //ids of edges that END at this gate (index in dynamic array)
    DynArray<int> fanout;       //ids of edges that START at this gate (index in dynamic array)
};

class Graph {
private:
    DynArray<Gate> gates_;   //all nodes, index == gate id
    DynArray<Edge> edges_;   //all wires/edges, index == edge id

public:


    //adds a new gate to the end of gates_ and returns its id (ids are assigned 0,1,2, in order). O(1) amortized.
    int addGate(const char* name, GateType type, int delay);

    //adds a new wire from gate (from)into input slot (pin) of gate (to). It returns the new wire's id or INVALID if the request is illegal:
    //   -an id is out of range
    //   -the pin number does not exist on the gate type
    //   -that pin is already driven (an input pin has exactly ONE driver)
    //   -the driver is a primary OUTPUT (outputs drive nothing)
    // Cost: O(1) amortized + a small check for the duplicate-pin check (<= 2).
    int addEdge(int from, int to, int pin);

    //return number of V and E
    int  numGates() const;
    int  numEdges() const;

    //returns true if id is a real gate (from 0 to numGates() - 1)
    bool validGate(int id) const;

    //get the gate or the wire at that id
    const Gate& gate(int id) const;
    const Edge& edge(int eid) const;

    //outDegree is how many wires leave the gate (size of its fanout). inDegree is how many wires enter it (size of its fanin).
    int outDegree(int id) const;
    int inDegree(int id) const;

    //"Combinational" means the signal passes through immediately (after the gate delay). This returns true for normal wires and false if the wire ends at a flip-flop.
    bool isCombinationalEdge(int eid) const;

    //Like inDegree, but it counts only the combinational incoming wires, which we will need for Kahn's algorithm.
    int combInDegree(int id) const;

    //print every gate with the pins it drives (for tests and debugging).
    void print() const;
};

#endif

//note regarding implementation:
/*
    We use dynamic arrays because gates and wires are only ever appended, never removed or inserted in the middle, 
    and the simulator constantly accesses them by id. Arrays give O(1) access by index and contiguous memory, 
    which is cache-friendly. Linked lists would make id lookup O(n) and add pointer overhead with no benefit. 
    An adjacency matrix would use O(V^2) space for a sparse circuit and could not store the pin number.
*/