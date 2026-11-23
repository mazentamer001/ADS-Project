// =============================================================================
// SchematicView.hpp -- editable circuit schematic.
//   View : column = logic level (levelize), wires coloured by value at the cursor
//          time, critical path red, combinational loop dashed orange.
//   Edit : drag a gate to move it, drag from an output pin (right side) to an
//          input pin (left side) to add a wire, right-click for the menu
//          (add / rename / delay / type / mark output / delete), Del removes the
//          selected gate or wire.
//   Every structural edit regenerates the netlist text (netlistEdited).
//   Wheel = zoom, drag on empty space = pan, double-click = fit.
// =============================================================================
#ifndef SCHEMATICVIEW_HPP
#define SCHEMATICVIEW_HPP

#include "SimRunner.hpp"
#include <QHash>
#include <QPointF>
#include <QString>
#include <QWidget>

class SchematicView : public QWidget {
    Q_OBJECT
public:
    explicit SchematicView(QWidget* parent = nullptr);

    void setData(const QVector<SchemNode>& nodes, const QVector<SchemEdge>& edges, int endTime);
    void clearData();              // forget the circuit and any manual layout

signals:
    void netlistEdited(const QString& netlistText);               // new text for the .net editor
    void nodeRenamed(const QString& oldName, const QString& newName);

public slots:
    void setTime(int t);           // wire colours show the values at time t
    void fit();
    void autoLayout();             // drop manual positions, lay out by logic level

protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void wheelEvent(QWheelEvent* e) override;
    void mousePressEvent(QMouseEvent* e) override;
    void mouseMoveEvent(QMouseEvent* e) override;
    void mouseReleaseEvent(QMouseEvent* e) override;
    void mouseDoubleClickEvent(QMouseEvent* e) override;
    void keyPressEvent(QKeyEvent* e) override;
    void contextMenuEvent(QContextMenuEvent* e) override;

private:
    enum Mode { None, Panning, Moving, Wiring };

    // view helpers
    void layout();
    char valueAt(int nodeId, int t) const;
    QPointF outPin(int id) const;
    QPointF inPin(int id, int pin) const;
    QVector<QPointF> wirePoints(int e) const;
    QPointF toScene(const QPointF& screen) const;

    // hit testing (scene coordinates)
    int  gateAt(const QPointF& s) const;
    int  outPinAt(const QPointF& s) const;
    bool inPinAt(const QPointF& s, int& node, int& pin) const;
    int  edgeAt(const QPointF& s) const;

    // editing
    void rebuildDrivers();
    void refreshOutputNames();
    void clearSimData();
    void freezePositions();
    void commitEdit();             // refresh, drop sim data, keep layout, emit new text
    QString generateNetlist() const;
    QString uniqueName(const QString& prefix) const;
    void addNode(int type, const QPointF& scenePos);
    void deleteNode(int id);
    void deleteEdge(int e);
    void connectPins(int from, int to, int pin);
    void renameNode(int id);
    void setDelay(int id);
    void changeType(int id, int newType);
    void toggleOutput(int id);

    QVector<SchemNode> nodes_;
    QVector<SchemEdge> edges_;
    QVector<QPointF>   pos_;      // top-left of each gate, scene units
    QVector<int>       driver_;   // first driver of each gate
    QHash<QString, QPointF> manualPos_;   // positions kept across re-runs, by name

    int    endTime_;
    int    time_;
    bool   hasValues_;
    double zoom_, panX_, panY_;
    bool   autoFit_;

    Mode    mode_;
    int     selected_;            // selected gate, -1 = none
    int     selectedEdge_;        // selected wire, -1 = none
    int     wireFrom_;            // wiring: source gate
    QPointF wireEnd_;             // wiring: current end (scene)
    QPointF grab_;                // moving: cursor offset inside the gate
    bool    moved_;
    QPointF dragStart_, dragPan_;
};

#endif