// =============================================================================
// SimParser.cpp -- see SimParser.hpp for the file format
// =============================================================================
#include "SimParser.hpp"
#include "ParseUtil.hpp"
#include "Logic.hpp"

bool SimParser::load(const char* filename, const Parser& p) {
    events.clear();
    steps = 0;

    FILE* f = fopen(filename, "r");
    if (!f) { fprintf(stderr, "cannot open '%s'\n", filename); return false; }

    char  line[MAX_LINE];
    char* tok[MAX_TOK];
    int   lineNo = 0;
    int   prevTime = -1;
    bool  ok = true;

    while (ok && fgets(line, sizeof line, f)) {
        lineNo++;
        int n = tokenize(line, tok, MAX_TOK);
        if (n < 0) { parseError(filename, lineNo, "too many words on one line"); ok = false; break; }
        if (n == 0) continue;

        if (eqNoCase(tok[0], "CLOCK")) {
            parseError(filename, lineNo, "CLOCK is not supported yet (sequential circuits come later)");
            ok = false; break;
        }
        if (n != 3) { parseError(filename, lineNo, "expected: <time> <input net> <value>"); ok = false; break; }

        StimEvent e;
        if (!parseNonNegInt(tok[0], e.time)) {
            parseError(filename, lineNo, "time '%s' is not a non-negative integer", tok[0]);
            ok = false; break;
        }
        if (e.time < prevTime) {
            parseError(filename, lineNo, "time %d goes backwards (previous line had %d)", e.time, prevTime);
            ok = false; break;
        }
        e.gate = p.findNet(tok[1]);
        if (e.gate == INVALID) { parseError(filename, lineNo, "unknown net '%s'", tok[1]); ok = false; break; }
        if (p.g.gate(e.gate).type != GATE_INPUT) {
            parseError(filename, lineNo, "'%s' is not a primary input, only INPUT nets can be driven", tok[1]);
            ok = false; break;
        }
        e.val = (tok[2][1] == '\0') ? normalizeValue(tok[2][0]) : 0;    // must be exactly one character
        if (e.val == 0) {
            parseError(filename, lineNo, "value '%s' must be 0, 1, X or Z", tok[2]);
            ok = false; break;
        }

        if (e.time != prevTime) steps++;
        prevTime = e.time;
        events.push_back(e);
    }
    fclose(f);
    if (!ok) { events.clear(); steps = 0; }
    return ok;
}