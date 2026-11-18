#include <iostream>
#include <iomanip>
#include "Graph.hpp"


//takes the gatetype and returns its name as a charactar array
const char* gateTypeName(GateType t) {
    switch (t) {
    case GATE_INPUT:  return "INPUT";
    case GATE_OUTPUT: return "OUTPUT";
    case GATE_NOT:    return "NOT";
    case GATE_AND:    return "AND";
    case GATE_OR:     return "OR";
    case GATE_XOR:    return "XOR";
    case GATE_NAND:   return "NAND";
    case GATE_DFF:    return "DFF";
    }
    return "?";
}

//true for state-holding elements.
bool isSequential(GateType t) { return t == GATE_DFF; }

//number of input pins each gate type has
int numInputPins(GateType t) {
    switch (t) {
    case GATE_INPUT:  return 0;
    case GATE_OUTPUT: return 1;
    case GATE_NOT:    return 1;
    case GATE_AND:    return 2;
    case GATE_OR:     return 2;
    case GATE_XOR:    return 2;
    case GATE_NAND:   return 2;
    case GATE_DFF:    return 2;   // D and CLK
    }
    return INVALID;   //only reached for a value not in the enum
}


//graph construction
int Graph::addGate(const char* name, GateType type, int delay) {
    if (name == nullptr || delay < 0) return INVALID;

    Gate g;
    g.id = gates_.size();                //next free id = current count

    //manual copy of name
    int i = 0;
    while (name[i] != '\0' && i < MAX_NAME_LEN - 1) {
        g.name[i] = name[i]; 
        i++;
    }
    g.name[i] = '\0';

    g.type  = type;
    g.delay = delay;
    gates_.push_back(g); //fanin/fanout start empty
    return g.id;
}

int Graph::addEdge(int from, int to, int pin) {
   
    //validation: reject anything that is not a legal wire.
    if (!validGate(from) || !validGate(to)) return INVALID; //id is out of range
    if (gates_[from].type == GATE_OUTPUT) return INVALID; //the driver is a primary OUTPUT
    if (pin < 0 || pin >= numInputPins(gates_[to].type)) return INVALID;  //the pin number does not exist on the gate type
    for (int k = 0; k < gates_[to].fanin.size(); k++)
        if (edges_[gates_[to].fanin[k]].pin == pin) return INVALID; //pin is already taken

    //store the wire once, register its id at both endpoints
    Edge e;
    e.from = from; e.to = to; e.pin = pin;
    int eid = edges_.size();
    edges_.push_back(e);
    gates_[from].fanout.push_back(eid);
    gates_[to].fanin.push_back(eid);
    return eid;
}

//quaries
int  Graph::numGates() const { return gates_.size(); }
int  Graph::numEdges() const { return edges_.size(); }
bool Graph::validGate(int id) const { return id >= 0 && id < gates_.size(); }

//return the gate or wire at that position. They do not check the id, so the caller MUST pass a valid one.
const Gate& Graph::gate(int id) const { return gates_[id]; }
const Edge& Graph::edge(int eid) const { return edges_[eid]; }

//num of wires entering/leaving a gate
int Graph::outDegree(int id) const { return gates_[id].fanout.size(); }
int Graph::inDegree(int id)  const { return gates_[id].fanin.size(); }

//edges_[eid] is the wire, .to is the gate it ends at, gates_[..] is that gate itself, .type is the type of it and we call isSequential(..).
bool Graph::isCombinationalEdge(int eid) const {
    return !isSequential(gates_[edges_[eid].to].type);
}

//It loops over the wires coming into the gate and counts only the combinational ones.
int Graph::combInDegree(int id) const {
    int count = 0;
    for (int k = 0; k < gates_[id].fanin.size(); k++)
        if (isCombinationalEdge(gates_[id].fanin[k])) count++;
    return count;
}

//debugging
void Graph::print() const {
    std::cout << "Graph: " << numGates() << " gates, " << numEdges() << " wires\n";

    for (int i = 0; i < gates_.size(); i++) {
        const Gate& g = gates_[i];

        std::cout << "  [" << g.id << "] "
                  << std::left
                  << std::setw(8) << g.name << " "
                  << std::setw(6) << gateTypeName(g.type) << " "
                  << std::right
                  << "delay=" << g.delay << "  drives:";

        if (g.fanout.empty()) std::cout << " (nothing)";

        for (int k = 0; k < g.fanout.size(); k++) {
            const Edge& e = edges_[g.fanout[k]];
            std::cout << " " << gates_[e.to].name << ".pin" << e.pin;
        }
        
        std::cout << "\n";
    }
}