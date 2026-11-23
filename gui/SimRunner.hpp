// =============================================================================
// SimRunner.hpp -- GUI glue: runs the existing C++ simulator on text from the
// editors and returns plain data the GUI can draw. No Qt widgets in here.
// =============================================================================
#ifndef SIMRUNNER_HPP
#define SIMRUNNER_HPP

#include <QString>
#include <QVector>

// One net's waveform. (Not named "signals": that is a Qt keyword macro.)
struct WaveSignal {
    QString name;
    int kind = 2;               // 0 = primary input, 1 = output net, 2 = internal
    QVector<int>  times;        // time of each value change
    QVector<char> values;       // '0' '1' 'X' 'Z'
};

// One gate of the circuit graph, for the schematic view.
struct SchemNode {
    int id = 0;
    QString name;               // "x$out" probe nodes are shown as "x"
    int type = 0;               // GateType from Graph.hpp, stored as int
    int delay = 0;
    int level = 0;              // column: logic level from levelize()
    int arrival = -1;           // static-timing arrival time, -1 = unknown
    bool onCritical = false;
    bool onLoop = false;
    QVector<int>  times;        // value changes of the net this gate drives
    QVector<char> values;
};

// One wire: net driven by 'from' goes into input pin 'pin' of 'to'.
struct SchemEdge {
    int from = 0, to = 0, pin = 0;
    bool critical = false;
    bool loop = false;
};

struct RunResult {
    bool ok = false;            // false = parse error or loop (see errors)
    QString errors;             // parser / loop messages
    QString report;             // statistics and critical path
    QVector<WaveSignal> waves;
    QVector<SchemNode> nodes;   // filled whenever the netlist parsed (even with a loop)
    QVector<SchemEdge> edges;
    int endTime = 0;
};

// Writes both texts to temp files, parses them, runs the loop check, timing
// analysis, levelized sim and event-driven sim. stimText may be empty.
RunResult runSimulation(const QString& netText, const QString& stimText);

#endif