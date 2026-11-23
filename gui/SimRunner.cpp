#include "SimRunner.hpp"

#include "Parser.hpp"
#include "SimParser.hpp"
#include "TimingAnalyzer.hpp"
#include "EventSim.hpp"
#include "LevelizedSim.hpp"
#ifdef HAVE_GRAPHALGOS
#include "GraphAlgos.hpp"
#endif

#include <QFile>
#include <QTemporaryDir>
#include <QStringList>

#include <cstdio>
#include <memory>
#include <unistd.h>

// The Parser and SimParser report errors with fprintf(stderr, ...). To show them
// in the GUI we point stderr at a temp file while they run, then read it back.
class StderrCapture {
public:
    explicit StderrCapture(const QString& path) : path_(path), saved_(-1) {
        fflush(stderr);
        saved_ = dup(fileno(stderr));
        FILE* fp = freopen(path.toLocal8Bit().constData(), "w", stderr);
        (void)fp;
    }
    ~StderrCapture() { finish(); }

    QString finish() {
        if (saved_ >= 0) {
            fflush(stderr);
            dup2(saved_, fileno(stderr));
            close(saved_);
            saved_ = -1;
        }
        QFile f(path_);
        if (!f.open(QIODevice::ReadOnly)) return QString();
        return QString::fromLocal8Bit(f.readAll());
    }

private:
    QString path_;
    int saved_;
};

static void writeFile(const QString& path, const QString& text) {
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(text.toUtf8());
        f.write("\n");
    }
}

static QString gateName(const Graph& g, int id) {
    return QString::fromLatin1(g.gate(id).name);
}

// Copies the graph into plain Qt data for the schematic: gates with their logic
// level (from levelize() when available), wires, critical path and loop marks.
// ta/path may be null (loop case). loopIds lists the gates on a loop, in order.
static void fillGraphData(RunResult& r, const Graph& g, const TimingAnalyzer* ta,
                          const DynArray<int>* path, const QVector<int>& loopIds) {
    const int n = g.numGates();

    // logic level of every gate = column in the schematic
    QVector<int> level(n, 0);
    bool done = false;
#ifdef HAVE_GRAPHALGOS
    {
        DynArray<int> order, lv;
        if (levelize(g, order, lv)) {
            for (int i = 0; i < n; i++) level[i] = lv[i];
            done = true;
        }
    }
#endif
    if (!done) {   // no GraphAlgos, or a loop: longest-path relaxation, bounded by n rounds
        for (int it = 0; it < n; it++) {
            bool changed = false;
            for (int e = 0; e < g.numEdges(); e++) {
                if (!g.isCombinationalEdge(e)) continue;
                const Edge& ed = g.edge(e);
                if (level[ed.to] < level[ed.from] + 1 && level[ed.from] + 1 <= n) {
                    level[ed.to] = level[ed.from] + 1;
                    changed = true;
                }
            }
            if (!changed) break;
        }
    }

    QVector<int> posInPath(n, -1), posInLoop(n, -1);
    if (path)
        for (int i = 0; i < path->size(); i++) posInPath[(*path)[i]] = i;
    for (int i = 0; i < loopIds.size(); i++) posInLoop[loopIds[i]] = i;

    r.nodes.resize(n);
    for (int id = 0; id < n; id++) {
        const Gate& gt = g.gate(id);
        SchemNode& s = r.nodes[id];
        s.id = id;
        s.name = gateName(g, id);
        if (gt.type == GATE_OUTPUT && s.name.endsWith("$out")) s.name.chop(4);
        s.type = int(gt.type);
        s.delay = gt.delay;
        s.level = level[id];
        s.arrival = ta ? ta->arrival(id) : -1;
        s.onCritical = posInPath[id] >= 0;
        s.onLoop = posInLoop[id] >= 0;
    }

    r.edges.resize(g.numEdges());
    for (int e = 0; e < g.numEdges(); e++) {
        const Edge& ed = g.edge(e);
        SchemEdge& se = r.edges[e];
        se.from = ed.from;
        se.to = ed.to;
        se.pin = ed.pin;
        se.critical = posInPath[ed.from] >= 0 && posInPath[ed.to] == posInPath[ed.from] + 1;
        se.loop = !loopIds.isEmpty() && posInLoop[ed.from] >= 0 && posInLoop[ed.to] >= 0 &&
                  posInLoop[ed.to] == (posInLoop[ed.from] + 1) % loopIds.size();
    }
}

RunResult runSimulation(const QString& netText, const QString& stimText) {
    RunResult r;

    QTemporaryDir dir;
    if (!dir.isValid()) {
        r.errors = "error: cannot create a temporary directory";
        return r;
    }
    const QString netPath  = dir.filePath("circuit.net");
    const QString stimPath = dir.filePath("input.stim");
    const QString errPath  = dir.filePath("stderr.txt");
    writeFile(netPath, netText);
    writeFile(stimPath, stimText);

    auto prettyErrors = [&](QString s) {
        s.replace(netPath, "netlist");
        s.replace(stimPath, "stimulus");
        return s.trimmed();
    };

    // 1. netlist
    std::unique_ptr<Parser> p(new Parser());
    bool loaded = false;
    {
        StderrCapture cap(errPath);
        loaded = p->load(netPath.toLocal8Bit().constData());
        r.errors = prettyErrors(cap.finish());
    }
    if (!loaded) return r;
    const Graph& g = p->g;

    QStringList rep;
    rep << QString("Netlist: %1 gates, %2 wires, %3 inputs, %4 outputs")
               .arg(g.numGates()).arg(g.numEdges())
               .arg(p->inputs.size()).arg(p->outputs.size());

    // 2. loop check + static timing analysis
    TimingAnalyzer ta;
    if (!ta.analyze(g)) {
        QStringList names;
        QVector<int> loopIds;
        bool named = false;
#ifdef HAVE_GRAPHALGOS
        DynArray<int> cyc;
        if (findLoop(g, cyc) && !cyc.empty()) {
            for (int i = 0; i < cyc.size(); i++) {
                names << gateName(g, cyc[i]);
                loopIds.append(cyc[i]);
            }
            names << gateName(g, cyc[0]);
            r.errors = "error: combinational loop detected: " + names.join(" -> ");
            named = true;
        }
#endif
        if (!named) {
            for (int i = 0; i < ta.loopGates().size(); i++) {
                names << gateName(g, ta.loopGates()[i]);
                loopIds.append(ta.loopGates()[i]);
            }
            r.errors = "error: combinational loop detected. Gates involved or downstream: "
                       + names.join(" ");
        }
        fillGraphData(r, g, nullptr, nullptr, loopIds);   // schematic shows the loop in red
        return r;
    }
    rep << "Loop check: no combinational loops";

    DynArray<int> path;
    ta.criticalPath(path);
    QStringList steps;
    for (int i = 0; i < path.size(); i++)
        steps << QString("%1(%2)").arg(gateName(g, path[i])).arg(ta.arrival(path[i]));
    rep << QString("Critical path (delay %1): %2")
               .arg(ta.criticalDelay()).arg(steps.join(" -> "));

    fillGraphData(r, g, &ta, &path, QVector<int>());
    r.ok = true;

    // 3. stimulus + simulations (skipped when the stimulus box is empty)
    if (stimText.trimmed().isEmpty()) {
        rep << "No stimulus: add lines like \"20 a 1\" to simulate.";
        r.report = rep.join("\n");
        return r;
    }

    SimParser sp;
    bool stimOk = false;
    {
        StderrCapture cap(errPath);
        stimOk = sp.load(stimPath.toLocal8Bit().constData(), *p);
        QString e = prettyErrors(cap.finish());
        if (!e.isEmpty()) r.errors = e;
    }
    if (!stimOk) {
        r.ok = false;
        r.report = rep.join("\n");
        return r;
    }
    rep << QString("Stimulus: %1 input changes at %2 time steps, last time %3")
               .arg(sp.numEvents()).arg(sp.numTimeSteps()).arg(sp.lastTime());

    LevelizedSim lev(g);
    lev.run(sp.events, ta.topoOrder());

    EventSim ev(g);
    const bool finished = ev.run(sp.events);

    rep << QString("Levelized evaluations: %1").arg(lev.evaluations());
    rep << QString("Event-driven evaluations: %1  (%2 events, last change at %3)")
               .arg(ev.evaluations()).arg(ev.eventsProcessed()).arg(ev.endTime());
    if (!finished)
        rep << "warning: event-driven run stopped early (event limit or full queue)";

    // 4. waveforms: every net except the "$out" probe nodes
    const int n = g.numGates();
    QVector<int> kindOf(n, 2);
    for (int id = 0; id < n; id++)
        if (g.gate(id).type == GATE_INPUT) kindOf[id] = 0;
    for (int id = 0; id < n; id++) {
        const Gate& gt = g.gate(id);
        if (gt.type == GATE_OUTPUT && gt.fanin.size() > 0) {
            int driver = g.edge(gt.fanin[0]).from;
            if (kindOf[driver] == 2) kindOf[driver] = 1;
        }
    }

    QVector<int> waveIndex(n, -1);
    for (int kind = 0; kind < 3; kind++) {
        for (int id = 0; id < n; id++) {
            if (g.gate(id).type == GATE_OUTPUT || kindOf[id] != kind) continue;
            WaveSignal w;
            w.name = gateName(g, id);
            w.kind = kind;
            waveIndex[id] = r.waves.size();
            r.waves.append(w);
        }
    }

    const DynArray<ValueChange>& ch = ev.changes();
    for (int i = 0; i < ch.size(); i++) {
        int gid = ch[i].getGate();
        if (gid < 0 || gid >= n) continue;
        // same change list also feeds the schematic's wire colours
        r.nodes[gid].times.append(ch[i].getTime());
        r.nodes[gid].values.append(ch[i].getVal());
        int wi = waveIndex[gid];
        if (wi < 0) continue;
        r.waves[wi].times.append(ch[i].getTime());
        r.waves[wi].values.append(ch[i].getVal());
    }

    r.endTime = sp.lastTime();
    if (ev.endTime() > r.endTime) r.endTime = ev.endTime();

    r.report = rep.join("\n");
    return r;
}