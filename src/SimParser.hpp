// =============================================================================
// SimParser.hpp -- Milestone 2: reads the simulation input (.stim file)
//
// This is the "testbench" of the simulator: it says WHEN each primary input
// changes to WHAT value. It needs the loaded Parser to turn net names into gate ids.
//
// FILE FORMAT (one change per line, '#' starts a comment):
//     <time> <input net> <value>          e.g.   20  a  1
// value is 0, 1, X or Z (either case). Times are integers >= 0 and must not go
// backwards. Several lines may share a time: they happen at the same instant and
// keep their file order. (CLOCK lines are not supported yet.)
// =============================================================================
#ifndef SIMPARSER_HPP
#define SIMPARSER_HPP

#include "Parser.hpp"
#include "DynArray.hpp"

// one input change: at 'time', primary input 'gate' (a gate id) becomes 'val'
struct StimEvent {
    int  time;
    int  gate;
    char val;     // '0' '1' 'X' 'Z'
};

class SimParser {
public:
    DynArray<StimEvent> events;   // sorted by time, file order kept for equal times

    SimParser() : steps(0) {}

    // Returns false after printing "file:line: error: ..." for the first problem.
    bool load(const char* filename, const Parser& p);

    int numEvents() const { return events.size(); }
    int numTimeSteps() const { return steps; }                              // distinct time values
    int lastTime() const { return events.empty() ? 0 : events.back().time; }

private:
    int steps;
};

#endif