// =============================================================================
// Parser.hpp -- Milestone 2: reads the circuit description (.net file)
//
// This is the "module file" of the simulator: it describes the gates and how they
// are wired, like a structural Verilog module. Parsing builds the Graph and a
// HashMap (net name -> gate id).
//
// FILE FORMAT (one statement per line, '#' starts a comment, keywords not case-sensitive):
//     INPUT  a b cin               primary inputs
//     OUTPUT sum cout              primary outputs (each watches the net of that name)
//     XOR  x1  2  a b              TYPE  name  delay  input1 [input2]
// The gate's name IS the name of the net it drives. Types: NOT AND OR XOR NAND.
// (DFF is recognised but rejected: sequential circuits are not supported yet.)
//
// CONVENTIONS used by the rest of the project
//   * net == the id of the gate that drives it (every gate drives exactly one net)
//   * "OUTPUT x" creates a GATE_OUTPUT node named "x$out", fed by net x.
//     '$' is therefore reserved and cannot appear in user names.
//   * Input pin order follows the order written in the line (pin 0 = first input).
//
// The file is read TWICE so a gate may use a net that is defined further down:
//   pass 1: check syntax, create every gate, fill the hash map
//   pass 2: look the input nets up and create the wires
// =============================================================================
#ifndef PARSER_HPP
#define PARSER_HPP

#include "Graph.hpp"
#include "DynArray.hpp"
#include "hashmap_final.hpp"
#include <cstdio>

class Parser {
public:
    Graph          g;        // the circuit
    DynArray<int>  inputs;   // ids of all GATE_INPUT nodes, in file order
    DynArray<int>  outputs;  // ids of all GATE_OUTPUT nodes, in file order

    Parser() : loaded(false) {}

    // Reads and builds everything. Returns false after printing
    // "file:line: error: ..." for the first problem found. One Parser loads one file.
    bool load(const char* filename);

    // gate id of the net called 'name' (for an output node use "name$out"), or INVALID
    int findNet(const char* name) const;

private:
    HashMap<int> names;      // net name -> gate id  (HashMap is non-copyable, so Parser is too)
    bool loaded;

    bool pass(FILE* f, const char* file, int passNo);
    bool validName(const char* file, int line, const char* name, int extraChars);
    bool registerGate(const char* file, int line, const char* name, GateType t, int delay);
};

#endif