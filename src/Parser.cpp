// =============================================================================
// Parser.cpp -- see Parser.hpp for the file format and conventions
// =============================================================================
#include "Parser.hpp"
#include "ParseUtil.hpp"
#include <cstring>

// Set to true when flip-flops are implemented in the simulators.
static const bool ALLOW_SEQUENTIAL = false;

// text -> gate type for the gates that have a name on a gate line
static bool parseType(const char* s, GateType& t) {
    if (eqNoCase(s, "NOT"))  { t = GATE_NOT;  return true; }
    if (eqNoCase(s, "AND"))  { t = GATE_AND;  return true; }
    if (eqNoCase(s, "OR"))   { t = GATE_OR;   return true; }
    if (eqNoCase(s, "XOR"))  { t = GATE_XOR;  return true; }
    if (eqNoCase(s, "NAND")) { t = GATE_NAND; return true; }
    if (eqNoCase(s, "DFF"))  { t = GATE_DFF;  return true; }
    return false;
}

int Parser::findNet(const char* name) const {
    int* p = names.get(name);              // HashMap::get returns a pointer, or nullptr
    return p ? *p : INVALID;
}

// A name must be non-empty, contain no '$' (reserved), and still fit in
// Gate::name (MAX_NAME_LEN incl. the '\0') after 'extraChars' are appended.
bool Parser::validName(const char* file, int line, const char* name, int extraChars) {
    if (strchr(name, '$')) {
        parseError(file, line, "'%s': the character '$' is reserved", name);
        return false;
    }
    if ((int)strlen(name) + extraChars >= MAX_NAME_LEN) {
        parseError(file, line, "name '%s' is too long (max %d characters)", name, MAX_NAME_LEN - 1 - extraChars);
        return false;
    }
    return true;
}

// creates the gate and registers its name; rejects duplicates
bool Parser::registerGate(const char* file, int line, const char* name, GateType t, int delay) {
    if (names.contains(name)) {
        parseError(file, line, "duplicate net '%s' (already defined)", name);
        return false;
    }
    int id = g.addGate(name, t, delay);
    names.put(name, id);
    return true;
}

bool Parser::pass(FILE* f, const char* file, int passNo) {
    char  line[MAX_LINE];
    char* tok[MAX_TOK];
    int   lineNo = 0;

    while (fgets(line, sizeof line, f)) {
        lineNo++;
        if (!strchr(line, '\n') && !feof(f)) {          // buffer filled without reaching end of line
            parseError(file, lineNo, "line too long (max %d characters)", MAX_LINE - 1);
            return false;
        }
        int n = tokenize(line, tok, MAX_TOK);
        if (n < 0) { parseError(file, lineNo, "too many words on one line (max %d)", MAX_TOK); return false; }
        if (n == 0) continue;                           // blank or comment-only line

        // ---------------- INPUT a b c ----------------
        if (eqNoCase(tok[0], "INPUT")) {
            if (passNo == 1) {
                if (n < 2) { parseError(file, lineNo, "INPUT needs at least one name"); return false; }
                for (int i = 1; i < n; i++) {
                    if (!validName(file, lineNo, tok[i], 0)) return false;
                    if (!registerGate(file, lineNo, tok[i], GATE_INPUT, 0)) return false;
                    inputs.push_back(findNet(tok[i]));
                }
            }
        }
        // ---------------- OUTPUT s co ----------------
        else if (eqNoCase(tok[0], "OUTPUT")) {
            if (passNo == 1 && n < 2) { parseError(file, lineNo, "OUTPUT needs at least one name"); return false; }
            for (int i = 1; i < n; i++) {
                char oname[MAX_NAME_LEN];
                if (passNo == 1) {
                    if (!validName(file, lineNo, tok[i], 4)) return false;       // room for "$out"
                    snprintf(oname, sizeof oname, "%s$out", tok[i]);
                    if (!registerGate(file, lineNo, oname, GATE_OUTPUT, 0)) return false;
                    outputs.push_back(findNet(oname));
                } else {
                    snprintf(oname, sizeof oname, "%s$out", tok[i]);
                    int from = findNet(tok[i]);
                    if (from == INVALID) { parseError(file, lineNo, "unknown net '%s'", tok[i]); return false; }
                    if (g.addEdge(from, findNet(oname), 0) == INVALID) {
                        parseError(file, lineNo, "cannot connect net '%s' to its output", tok[i]);
                        return false;
                    }
                }
            }
        }
        // ---------------- TYPE name delay in1 [in2] ----------------
        else {
            GateType t;
            if (!parseType(tok[0], t)) { parseError(file, lineNo, "unknown keyword or gate type '%s'", tok[0]); return false; }
            if (isSequential(t) && !ALLOW_SEQUENTIAL) {
                parseError(file, lineNo, "sequential gates (%s) are not supported yet", tok[0]);
                return false;
            }
            int pins = numInputPins(t);
            if (passNo == 1) {
                if (n < 3) { parseError(file, lineNo, "expected: %s name delay input...", tok[0]); return false; }
                if (n - 3 != pins) {
                    parseError(file, lineNo, "%s needs exactly %d input%s, found %d", tok[0], pins, pins == 1 ? "" : "s", n - 3);
                    return false;
                }
                int delay;
                if (!parseNonNegInt(tok[2], delay)) {
                    parseError(file, lineNo, "delay '%s' is not a non-negative integer", tok[2]);
                    return false;
                }
                if (!validName(file, lineNo, tok[1], 0)) return false;
                if (!registerGate(file, lineNo, tok[1], t, delay)) return false;
            } else {
                int to = findNet(tok[1]);
                for (int p = 0; p < pins; p++) {
                    int from = findNet(tok[3 + p]);
                    if (from == INVALID) { parseError(file, lineNo, "unknown net '%s'", tok[3 + p]); return false; }
                    if (g.gate(from).type == GATE_OUTPUT) {
                        parseError(file, lineNo, "'%s' is an output node and cannot drive a gate", tok[3 + p]);
                        return false;
                    }
                    if (g.addEdge(from, to, p) == INVALID) {
                        parseError(file, lineNo, "cannot connect '%s' to pin %d of '%s'", tok[3 + p], p, tok[1]);
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

bool Parser::load(const char* filename) {
    if (loaded) { fprintf(stderr, "Parser: a Parser can only load one file\n"); return false; }
    loaded = true;
    FILE* f = fopen(filename, "r");
    if (!f) { fprintf(stderr, "cannot open '%s'\n", filename); return false; }
    bool ok = pass(f, filename, 1);                       // pass 1: gates + hash map
    if (ok) { rewind(f); ok = pass(f, filename, 2); }     // pass 2: wires
    fclose(f);
    return ok;
}