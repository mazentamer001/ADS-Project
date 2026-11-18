//Pure functions: "given gate type T and input values A, B, what is the output?"
//no data structures, no circuit knowledge. Combinational gates only: INPUT is set
//from outside, and DFF (sequential) is not simulated yet.
// Values are the characters '0' '1' 'X' (unknown) 'Z' (high impedance i.e. disconnected).
// A 'Z' arriving at a gate input is treated exactly like 'X'.

#ifndef LOGIC_HPP
#define LOGIC_HPP

#include "Graph.hpp"   // only for the GateType enum

// Turns user text ('0','1','x','X','z','Z') into one of '0' '1' 'X' 'Z'.
// Returns 0 (the null char) if c is not a logic value.
inline char normalizeValue(char c) {
    switch (c) {
        case '0': return '0';
        case '1': return '1';
        case 'x': case 'X': return 'X';
        case 'z': case 'Z': return 'Z';
        default:  return 0;
    }
}

// true only for a definite 0 or 1
inline bool isKnown(char v) { return v == '0' || v == '1'; }

// NOT: 0->1, 1->0, anything unknown stays X
inline char logicNot(char a) {
    if (a == '0') return '1';
    if (a == '1') return '0';
    return 'X';
}

// AND: a single 0 forces the output to 0 even if the other input is unknown
inline char logicAnd(char a, char b) {
    if (a == '0' || b == '0') return '0';
    if (a == '1' && b == '1') return '1';
    return 'X';
}

// OR: a single 1 forces the output to 1 even if the other input is unknown
inline char logicOr(char a, char b) {
    if (a == '1' || b == '1') return '1';
    if (a == '0' && b == '0') return '0';
    return 'X';
}

// XOR: no input can force the result, so any unknown input gives X
inline char logicXor(char a, char b) {
    if (!isKnown(a) || !isKnown(b)) return 'X';
    return (a == b) ? '0' : '1';
}

// Output of one combinational gate of type t. b is ignored by 1-input gates.
//   GATE_OUTPUT is just a probe on a net, so it passes its input through untouched.
//   GATE_INPUT / GATE_DFF are not computed here: returns 'X' as a safe default.
inline char evalGate(GateType t, char a, char b) {
    switch (t) {
        case GATE_NOT:    return logicNot(a);
        case GATE_AND:    return logicAnd(a, b);
        case GATE_OR:     return logicOr(a, b);
        case GATE_XOR:    return logicXor(a, b);
        case GATE_NAND:   return logicNot(logicAnd(a, b));
        case GATE_OUTPUT: return a;
        default:          return 'X';
    }
}

#endif