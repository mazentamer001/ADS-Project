#include "SchematicView.hpp"

#include "Graph.hpp"   // GateType

#include <QContextMenuEvent>
#include <QFontDatabase>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QLineF>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QRegularExpression>
#include <QWheelEvent>
#include <QtGlobal>

#include <algorithm>

namespace {
const double kW = 56, kH = 40;      // gate symbol size (scene units)
const double kColW = 150;           // distance between columns
const double kRowH = 72;            // distance between rows
const double kMargin = 30;
const double kPinHit = 9;           // pin hit radius (scene units)

QColor valColor(char v) {
    switch (v) {
        case '1': return QColor(90, 230, 110);
        case '0': return QColor(40, 150, 70);
        case 'Z': return QColor(90, 160, 255);
        default:  return QColor(235, 70, 70);   // X
    }
}

bool isPort(int type) { return type == GATE_INPUT || type == GATE_OUTPUT; }

int pinCount(int type) {
    if (type == GATE_INPUT) return 0;
    if (type == GATE_OUTPUT || type == GATE_NOT) return 1;
    return 2;
}

const char* typeKeyword(int type) {
    switch (type) {
        case GATE_NOT:  return "NOT";
        case GATE_AND:  return "AND";
        case GATE_OR:   return "OR";
        case GATE_XOR:  return "XOR";
        case GATE_NAND: return "NAND";
        case GATE_DFF:  return "DFF";
        default:        return "?";
    }
}

double distToSegment(const QPointF& p, const QPointF& a, const QPointF& b) {
    const double dx = b.x() - a.x(), dy = b.y() - a.y();
    const double len2 = dx * dx + dy * dy;
    double t = len2 > 0 ? ((p.x() - a.x()) * dx + (p.y() - a.y()) * dy) / len2 : 0;
    t = qBound(0.0, t, 1.0);
    return QLineF(p, QPointF(a.x() + t * dx, a.y() + t * dy)).length();
}

// Draws the gate symbol in local coordinates (0,0)-(kW,kH). Pins: inputs on the
// left edge, output at (kW, kH/2).
void drawSymbol(QPainter& p, int type) {
    const double w = kW, h = kH;
    switch (type) {
    case GATE_AND:
    case GATE_NAND: {
        const bool inv = (type == GATE_NAND);
        const double bw = inv ? w - 8 : w;
        QPainterPath path;
        path.moveTo(0, 0);
        path.lineTo(bw - h / 2, 0);
        path.arcTo(QRectF(bw - h, 0, h, h), 90, -180);
        path.lineTo(0, h);
        path.closeSubpath();
        p.drawPath(path);
        if (inv) p.drawEllipse(QPointF(bw + 4, h / 2), 4, 4);
        break;
    }
    case GATE_OR:
    case GATE_XOR: {
        const bool x = (type == GATE_XOR);
        const double off = x ? 7 : 0, bw = w - off;
        QPainterPath path;
        path.moveTo(off, 0);
        path.quadTo(off + bw * 0.55, 0, w, h / 2);
        path.quadTo(off + bw * 0.55, h, off, h);
        path.quadTo(off + bw * 0.25, h / 2, off, 0);
        path.closeSubpath();
        p.drawPath(path);
        if (x) {
            QPainterPath arc;
            arc.moveTo(0, 0);
            arc.quadTo(bw * 0.25, h / 2, 0, h);
            p.setBrush(Qt::NoBrush);
            p.drawPath(arc);
        }
        break;
    }
    case GATE_NOT: {
        const double bw = w - 8;
        QPolygonF tri;
        tri << QPointF(0, 0) << QPointF(bw, h / 2) << QPointF(0, h);
        p.drawPolygon(tri);
        p.drawEllipse(QPointF(bw + 4, h / 2), 4, 4);
        break;
    }
    case GATE_INPUT: {
        QPolygonF poly;
        poly << QPointF(0, h * 0.2) << QPointF(w * 0.7, h * 0.2) << QPointF(w, h / 2)
             << QPointF(w * 0.7, h * 0.8) << QPointF(0, h * 0.8);
        p.drawPolygon(poly);
        break;
    }
    case GATE_OUTPUT: {
        QPolygonF poly;
        poly << QPointF(0, h / 2) << QPointF(w * 0.3, h * 0.2) << QPointF(w, h * 0.2)
             << QPointF(w, h * 0.8) << QPointF(w * 0.3, h * 0.8);
        p.drawPolygon(poly);
        break;
    }
    default:
        p.drawRect(QRectF(0, 0, w, h));
        p.drawText(QRectF(0, 0, w, h), Qt::AlignCenter, QString::fromLatin1(typeKeyword(type)));
        break;
    }
}
}  // namespace

SchematicView::SchematicView(QWidget* parent)
    : QWidget(parent), endTime_(0), time_(0), hasValues_(false),
      zoom_(1), panX_(0), panY_(0), autoFit_(true),
      mode_(None), selected_(-1), selectedEdge_(-1), wireFrom_(-1), moved_(false) {
    setMinimumHeight(160);
    setFocusPolicy(Qt::StrongFocus);
    setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
}

// ---------------------------------------------------------------- data in/out

void SchematicView::setData(const QVector<SchemNode>& nodes, const QVector<SchemEdge>& edges,
                            int endTime) {
    nodes_ = nodes;
    edges_ = edges;
    endTime_ = endTime;
    time_ = endTime;
    hasValues_ = false;
    for (int i = 0; i < nodes_.size(); i++)
        if (!nodes_[i].times.isEmpty()) { hasValues_ = true; break; }
    selected_ = -1;
    selectedEdge_ = -1;
    mode_ = None;

    layout();
    // keep positions the user chose (matched by name) across re-runs
    for (int i = 0; i < nodes_.size(); i++) {
        const QString key = nodes_[i].name + (nodes_[i].type == GATE_OUTPUT ? "$out" : "");
        if (manualPos_.contains(key)) pos_[i] = manualPos_.value(key);
    }
    if (autoFit_) fit(); else update();
}

void SchematicView::clearData() {
    nodes_.clear();
    edges_.clear();
    pos_.clear();
    driver_.clear();
    manualPos_.clear();
    hasValues_ = false;
    selected_ = selectedEdge_ = -1;
    mode_ = None;
    autoFit_ = true;
    update();
}

void SchematicView::setTime(int t) {
    if (t == time_) return;
    time_ = t;
    update();
}

void SchematicView::autoLayout() {
    manualPos_.clear();
    layout();
    fit();
}

// Columns = logic level. Inside a column, gates are ordered by the average
// vertical position of their drivers (keeps wires short), then centred.
void SchematicView::layout() {
    const int n = nodes_.size();
    pos_ = QVector<QPointF>(n);
    driver_ = QVector<int>(n, -1);

    QVector<QVector<int> > drivers(n);
    for (int e = 0; e < edges_.size(); e++) {
        drivers[edges_[e].to].append(edges_[e].from);
        if (driver_[edges_[e].to] < 0) driver_[edges_[e].to] = edges_[e].from;
    }

    int maxLevel = 0;
    for (int i = 0; i < n; i++) maxLevel = qMax(maxLevel, nodes_[i].level);
    QVector<QVector<int> > cols(maxLevel + 1);
    for (int i = 0; i < n; i++) cols[nodes_[i].level].append(i);
    int maxRows = 0;
    for (int L = 0; L <= maxLevel; L++) maxRows = qMax(maxRows, int(cols[L].size()));

    QVector<char> placed(n, 0);
    QVector<double> key(n, 0.0);
    for (int L = 0; L <= maxLevel; L++) {
        QVector<int>& col = cols[L];
        for (int k = 0; k < col.size(); k++) {
            const int id = col[k];
            double sum = 0;
            int cnt = 0;
            for (int d = 0; d < drivers[id].size(); d++) {
                const int src = drivers[id][d];
                if (placed[src]) { sum += pos_[src].y(); cnt++; }
            }
            key[id] = cnt ? sum / cnt : 1e9 + id;
        }
        std::sort(col.begin(), col.end(), [&](int a, int b) {
            if (key[a] != key[b]) return key[a] < key[b];
            return a < b;
        });
        const double off = (maxRows - col.size()) / 2.0;
        for (int k = 0; k < col.size(); k++) {
            pos_[col[k]] = QPointF(L * kColW, (k + off) * kRowH);
            placed[col[k]] = 1;
        }
    }
}

void SchematicView::fit() {
    autoFit_ = true;
    if (pos_.isEmpty()) { update(); return; }
    double minX = pos_[0].x(), minY = pos_[0].y(), maxX = minX, maxY = minY;
    for (int i = 1; i < pos_.size(); i++) {
        minX = qMin(minX, pos_[i].x());
        minY = qMin(minY, pos_[i].y());
        maxX = qMax(maxX, pos_[i].x());
        maxY = qMax(maxY, pos_[i].y());
    }
    const double sceneW = maxX - minX + kW + 2 * kMargin;
    const double sceneH = maxY - minY + kH + 2 * kMargin + 20;
    zoom_ = qMin(qMin(width() / sceneW, height() / sceneH), 1.6);
    panX_ = (width() - sceneW * zoom_) / 2 + (kMargin - minX) * zoom_;
    panY_ = (height() - sceneH * zoom_) / 2 + (kMargin + 10 - minY) * zoom_;
    update();
}

void SchematicView::resizeEvent(QResizeEvent*) {
    if (autoFit_) fit();
}

// ---------------------------------------------------------------- geometry

QPointF SchematicView::toScene(const QPointF& s) const {
    return QPointF((s.x() - panX_) / zoom_, (s.y() - panY_) / zoom_);
}

char SchematicView::valueAt(int nodeId, int t) const {
    int id = nodeId;
    // an output port shows the value of the net that feeds it
    if (nodes_[id].type == GATE_OUTPUT && id < driver_.size() && driver_[id] >= 0) id = driver_[id];
    const SchemNode& s = nodes_[id];
    char v = 'X';
    for (int k = 0; k < s.times.size(); k++) {
        if (s.times[k] <= t) v = s.values[k];
        else break;
    }
    return v;
}

QPointF SchematicView::outPin(int id) const {
    return pos_[id] + QPointF(kW, kH / 2);
}

QPointF SchematicView::inPin(int id, int pin) const {
    const int np = pinCount(nodes_[id].type);
    const double y = (np <= 1) ? kH / 2 : kH * (0.3 + 0.4 * pin);
    return pos_[id] + QPointF(0, y);
}

QVector<QPointF> SchematicView::wirePoints(int e) const {
    const SchemEdge& ed = edges_[e];
    const QPointF a = outPin(ed.from);
    const QPointF b = inPin(ed.to, ed.pin);
    const double xm = (b.x() > a.x() + 24) ? b.x() - 16 : a.x() + 14;
    QVector<QPointF> pts;
    pts << a << QPointF(xm, a.y()) << QPointF(xm, b.y()) << b;
    return pts;
}

int SchematicView::gateAt(const QPointF& s) const {
    for (int i = pos_.size() - 1; i >= 0; i--)
        if (QRectF(pos_[i], QSizeF(kW, kH)).contains(s)) return i;
    return -1;
}

int SchematicView::outPinAt(const QPointF& s) const {
    for (int i = 0; i < nodes_.size(); i++)
        if (nodes_[i].type != GATE_OUTPUT && QLineF(outPin(i), s).length() <= kPinHit) return i;
    return -1;
}

bool SchematicView::inPinAt(const QPointF& s, int& node, int& pin) const {
    for (int i = 0; i < nodes_.size(); i++) {
        const int np = pinCount(nodes_[i].type);
        for (int k = 0; k < np; k++) {
            if (QLineF(inPin(i, k), s).length() <= kPinHit) {
                node = i;
                pin = k;
                return true;
            }
        }
    }
    return false;
}

int SchematicView::edgeAt(const QPointF& s) const {
    for (int e = edges_.size() - 1; e >= 0; e--) {
        const QVector<QPointF> pts = wirePoints(e);
        for (int k = 0; k + 1 < pts.size(); k++)
            if (distToSegment(s, pts[k], pts[k + 1]) <= 5) return e;
    }
    return -1;
}

// ---------------------------------------------------------------- mouse / keys

void SchematicView::wheelEvent(QWheelEvent* e) {
    if (e->angleDelta().y() == 0) return;
    autoFit_ = false;
    const double f = e->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    const QPointF a = e->pos();
    const QPointF scene = toScene(a);
    zoom_ = qBound(0.1, zoom_ * f, 6.0);
    panX_ = a.x() - scene.x() * zoom_;
    panY_ = a.y() - scene.y() * zoom_;
    update();
    e->accept();
}

void SchematicView::mousePressEvent(QMouseEvent* e) {
    setFocus();
    if (e->button() != Qt::LeftButton) return;
    const QPointF s = toScene(e->pos());
    selectedEdge_ = -1;

    int g = outPinAt(s);
    if (g >= 0) {                       // start a wire from this output pin
        mode_ = Wiring;
        wireFrom_ = g;
        wireEnd_ = s;
        update();
        return;
    }
    g = gateAt(s);
    if (g >= 0) {                       // move a gate
        mode_ = Moving;
        selected_ = g;
        grab_ = s - pos_[g];
        moved_ = false;
        update();
        return;
    }
    selected_ = -1;
    const int ed = edgeAt(s);
    if (ed >= 0) {                      // select a wire
        selectedEdge_ = ed;
        update();
        return;
    }
    mode_ = Panning;
    dragStart_ = e->pos();
    dragPan_ = QPointF(panX_, panY_);
    update();
}

void SchematicView::mouseMoveEvent(QMouseEvent* e) {
    const QPointF s = toScene(e->pos());
    if (mode_ == Panning) {
        autoFit_ = false;
        panX_ = dragPan_.x() + (e->pos().x() - dragStart_.x());
        panY_ = dragPan_.y() + (e->pos().y() - dragStart_.y());
        update();
    } else if (mode_ == Moving && selected_ >= 0) {
        pos_[selected_] = s - grab_;
        moved_ = true;
        update();
    } else if (mode_ == Wiring) {
        wireEnd_ = s;
        update();
    }
}

void SchematicView::mouseReleaseEvent(QMouseEvent*) {
    const Mode m = mode_;
    mode_ = None;
    if (m == Wiring) {
        int t = -1, pin = -1;
        const int from = wireFrom_;
        wireFrom_ = -1;
        if (inPinAt(wireEnd_, t, pin)) {
            connectPins(from, t, pin);   // may re-run the simulation: nothing after this
            return;
        }
    } else if (m == Moving && moved_) {
        freezePositions();
    }
    update();
}

void SchematicView::mouseDoubleClickEvent(QMouseEvent*) { fit(); }

void SchematicView::keyPressEvent(QKeyEvent* e) {
    if (e->key() == Qt::Key_Delete || e->key() == Qt::Key_Backspace) {
        if (selectedEdge_ >= 0) { deleteEdge(selectedEdge_); return; }
        if (selected_ >= 0)     { deleteNode(selected_);     return; }
    }
    QWidget::keyPressEvent(e);
}

void SchematicView::contextMenuEvent(QContextMenuEvent* e) {
    const QPointF s = toScene(e->pos());
    QMenu menu;

    const int g = gateAt(s);
    if (g >= 0) {
        selected_ = g;
        selectedEdge_ = -1;
        update();
        const int type = nodes_[g].type;

        QAction* aRename = nullptr;
        QAction* aDelay = nullptr;
        QAction* aOut = nullptr;
        QAction* aDelete = nullptr;
        QHash<QAction*, int> typeActions;

        if (type != GATE_OUTPUT) aRename = menu.addAction("Rename...");
        if (!isPort(type)) aDelay = menu.addAction("Set delay...");
        if (pinCount(type) == 2 && type != GATE_DFF) {
            QMenu* tm = menu.addMenu("Change type");
            const int types[4] = {GATE_AND, GATE_OR, GATE_XOR, GATE_NAND};
            for (int i = 0; i < 4; i++) {
                QAction* a = tm->addAction(typeKeyword(types[i]));
                a->setEnabled(types[i] != type);
                typeActions.insert(a, types[i]);
            }
        }
        if (type != GATE_OUTPUT) {
            bool hasOut = false;
            for (int k = 0; k < edges_.size(); k++)
                if (edges_[k].from == g && nodes_[edges_[k].to].type == GATE_OUTPUT) hasOut = true;
            aOut = menu.addAction(hasOut ? "Remove output probe" : "Mark as output");
        }
        menu.addSeparator();
        aDelete = menu.addAction(type == GATE_OUTPUT ? "Delete output probe" : "Delete gate");

        QAction* c = menu.exec(e->globalPos());
        if (!c) return;
        if (c == aRename) renameNode(g);
        else if (c == aDelay) setDelay(g);
        else if (c == aOut) toggleOutput(g);
        else if (c == aDelete) deleteNode(g);
        else if (typeActions.contains(c)) changeType(g, typeActions.value(c));
        return;
    }

    const int ed = edgeAt(s);
    if (ed >= 0) {
        selectedEdge_ = ed;
        selected_ = -1;
        update();
        QAction* aDel = menu.addAction("Delete wire");
        if (menu.exec(e->globalPos()) == aDel) deleteEdge(ed);
        return;
    }

    QHash<QAction*, int> addActions;
    QMenu* am = menu.addMenu("Add here");
    const int types[6] = {GATE_INPUT, GATE_AND, GATE_OR, GATE_XOR, GATE_NAND, GATE_NOT};
    const char* labels[6] = {"Input", "AND", "OR", "XOR", "NAND", "NOT"};
    for (int i = 0; i < 6; i++) addActions.insert(am->addAction(labels[i]), types[i]);
    menu.addSeparator();
    QAction* aAuto = menu.addAction("Auto layout");
    QAction* aFit = menu.addAction("Fit to window");

    QAction* c = menu.exec(e->globalPos());
    if (!c) return;
    if (c == aAuto) autoLayout();
    else if (c == aFit) fit();
    else if (addActions.contains(c)) addNode(addActions.value(c), s);
}

// ---------------------------------------------------------------- editing

void SchematicView::rebuildDrivers() {
    driver_ = QVector<int>(nodes_.size(), -1);
    for (int e = 0; e < edges_.size(); e++)
        if (driver_[edges_[e].to] < 0) driver_[edges_[e].to] = edges_[e].from;
}

// An output probe is named after the net that feeds it.
void SchematicView::refreshOutputNames() {
    rebuildDrivers();
    for (int i = 0; i < nodes_.size(); i++)
        if (nodes_[i].type == GATE_OUTPUT && driver_[i] >= 0)
            nodes_[i].name = nodes_[driver_[i]].name;
}

void SchematicView::clearSimData() {
    for (int i = 0; i < nodes_.size(); i++) {
        nodes_[i].times.clear();
        nodes_[i].values.clear();
        nodes_[i].arrival = -1;
        nodes_[i].onCritical = false;
        nodes_[i].onLoop = false;
    }
    for (int e = 0; e < edges_.size(); e++) {
        edges_[e].critical = false;
        edges_[e].loop = false;
    }
    hasValues_ = false;
}

void SchematicView::freezePositions() {
    for (int i = 0; i < nodes_.size(); i++) {
        const QString key = nodes_[i].name + (nodes_[i].type == GATE_OUTPUT ? "$out" : "");
        manualPos_.insert(key, pos_[i]);
    }
}

// Call after every structural change. Emits last: the receiver re-runs the
// simulation and may replace our data through setData().
void SchematicView::commitEdit() {
    refreshOutputNames();
    clearSimData();
    freezePositions();
    selectedEdge_ = -1;
    update();
    emit netlistEdited(generateNetlist());
}

QString SchematicView::generateNetlist() const {
    QStringList inputs, outputs, gates;
    for (int i = 0; i < nodes_.size(); i++) {
        const SchemNode& n = nodes_[i];
        if (n.type == GATE_INPUT) {
            inputs << n.name;
        } else if (n.type == GATE_OUTPUT) {
            if (driver_[i] >= 0) outputs << nodes_[driver_[i]].name;
        } else {
            QString line = QString("%1 %2 %3").arg(typeKeyword(n.type)).arg(n.name).arg(n.delay);
            const int np = pinCount(n.type);
            for (int p = 0; p < np; p++) {
                int src = -1;
                for (int e = 0; e < edges_.size(); e++)
                    if (edges_[e].to == i && edges_[e].pin == p) { src = edges_[e].from; break; }
                if (src < 0) break;   // unconnected pin: the parser will report it
                line += " " + nodes_[src].name;
            }
            gates << line;
        }
    }
    QString text = "# Generated from the schematic editor\n";
    if (!inputs.isEmpty())  text += "INPUT  " + inputs.join(" ") + "\n";
    if (!outputs.isEmpty()) text += "OUTPUT " + outputs.join(" ") + "\n";
    text += "\n" + gates.join("\n") + "\n";
    return text;
}

QString SchematicView::uniqueName(const QString& prefix) const {
    for (int k = 1;; k++) {
        const QString name = prefix + QString::number(k);
        bool used = false;
        for (int i = 0; i < nodes_.size(); i++)
            if (nodes_[i].type != GATE_OUTPUT && nodes_[i].name == name) { used = true; break; }
        if (!used) return name;
    }
}

void SchematicView::addNode(int type, const QPointF& scenePos) {
    SchemNode s;
    s.id = nodes_.size();
    s.type = type;
    s.delay = (type == GATE_INPUT) ? 0 : 1;
    s.name = uniqueName(type == GATE_INPUT ? "in" : "g");
    nodes_.append(s);
    pos_.append(scenePos - QPointF(kW / 2, kH / 2));
    selected_ = s.id;
    commitEdit();
}

void SchematicView::deleteNode(int id) {
    if (id < 0 || id >= nodes_.size()) return;
    const int n = nodes_.size();
    QVector<char> kill(n, 0);
    kill[id] = 1;
    // output probes fed by a deleted gate go with it
    for (int e = 0; e < edges_.size(); e++)
        if (edges_[e].from == id && nodes_[edges_[e].to].type == GATE_OUTPUT) kill[edges_[e].to] = 1;

    QVector<int> remap(n, -1);
    int cnt = 0;
    for (int i = 0; i < n; i++)
        if (!kill[i]) remap[i] = cnt++;

    QVector<SchemNode> nn;
    QVector<QPointF> np;
    for (int i = 0; i < n; i++) {
        if (remap[i] < 0) continue;
        SchemNode s = nodes_[i];
        s.id = remap[i];
        nn.append(s);
        np.append(pos_[i]);
    }
    QVector<SchemEdge> ne;
    for (int e = 0; e < edges_.size(); e++) {
        SchemEdge ed = edges_[e];
        if (remap[ed.from] < 0 || remap[ed.to] < 0) continue;
        ed.from = remap[ed.from];
        ed.to = remap[ed.to];
        ne.append(ed);
    }
    nodes_ = nn;
    pos_ = np;
    edges_ = ne;
    selected_ = -1;
    commitEdit();
}

void SchematicView::deleteEdge(int e) {
    if (e < 0 || e >= edges_.size()) return;
    edges_.remove(e);
    commitEdit();
}

void SchematicView::connectPins(int from, int to, int pin) {
    if (from < 0 || to < 0 || from >= nodes_.size() || to >= nodes_.size()) return;
    if (nodes_[from].type == GATE_OUTPUT) return;           // outputs drive nothing
    if (pin >= pinCount(nodes_[to].type)) return;           // pin does not exist
    for (int e = edges_.size() - 1; e >= 0; e--)            // a pin has exactly one driver
        if (edges_[e].to == to && edges_[e].pin == pin) edges_.remove(e);
    SchemEdge ed;
    ed.from = from;
    ed.to = to;
    ed.pin = pin;
    edges_.append(ed);
    commitEdit();
}

void SchematicView::renameNode(int id) {
    const QString old = nodes_[id].name;
    bool ok = false;
    const QString nn = QInputDialog::getText(this, "Rename",
        "New name (letters, digits and _, at most 27 characters):",
        QLineEdit::Normal, old, &ok).trimmed();
    if (!ok || nn == old) return;
    static const QRegularExpression re("^[A-Za-z_][A-Za-z0-9_]{0,26}$");
    if (!re.match(nn).hasMatch()) {
        QMessageBox::warning(this, "Rename", "Invalid name: " + nn);
        return;
    }
    for (int i = 0; i < nodes_.size(); i++) {
        if (i != id && nodes_[i].type != GATE_OUTPUT && nodes_[i].name == nn) {
            QMessageBox::warning(this, "Rename", "A gate called " + nn + " already exists.");
            return;
        }
    }
    nodes_[id].name = nn;
    emit nodeRenamed(old, nn);   // lets the stimulus text follow the new name
    commitEdit();
}

void SchematicView::setDelay(int id) {
    bool ok = false;
    const int d = QInputDialog::getInt(this, "Delay",
        "Propagation delay of " + nodes_[id].name + ":", nodes_[id].delay, 0, 1000, 1, &ok);
    if (!ok || d == nodes_[id].delay) return;
    nodes_[id].delay = d;
    commitEdit();
}

void SchematicView::changeType(int id, int newType) {
    if (nodes_[id].type == newType) return;
    nodes_[id].type = newType;
    commitEdit();
}

void SchematicView::toggleOutput(int id) {
    for (int e = 0; e < edges_.size(); e++) {
        if (edges_[e].from == id && nodes_[edges_[e].to].type == GATE_OUTPUT) {
            deleteNode(edges_[e].to);   // remove the probe
            return;
        }
    }
    SchemNode s;
    s.id = nodes_.size();
    s.type = GATE_OUTPUT;
    s.name = nodes_[id].name;
    s.delay = 0;
    nodes_.append(s);
    pos_.append(pos_[id] + QPointF(kColW * 0.8, 0));
    SchemEdge ed;
    ed.from = id;
    ed.to = s.id;
    ed.pin = 0;
    edges_.append(ed);
    commitEdit();
}

// ---------------------------------------------------------------- painting

void SchematicView::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.fillRect(rect(), QColor(18, 18, 22));

    if (nodes_.isEmpty()) {
        p.setPen(QColor(130, 130, 140));
        p.drawText(rect(), Qt::AlignCenter,
                   "Press Run (F5) to see the schematic, or right-click to start drawing one");
        return;
    }

    p.save();
    p.translate(panX_, panY_);
    p.scale(zoom_, zoom_);

    QFont f = font();
    f.setPixelSize(11);
    p.setFont(f);

    // ---- wires ----
    for (int e = 0; e < edges_.size(); e++) {
        const SchemEdge& ed = edges_[e];
        const QVector<QPointF> pts = wirePoints(e);
        QPainterPath wire;
        wire.moveTo(pts[0]);
        for (int k = 1; k < pts.size(); k++) wire.lineTo(pts[k]);

        p.setBrush(Qt::NoBrush);
        if (e == selectedEdge_) {
            p.setPen(QPen(QColor(90, 170, 255, 130), 8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.drawPath(wire);
        }
        if (ed.critical) {
            p.setPen(QPen(QColor(255, 70, 70, 110), 7, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            p.drawPath(wire);
        }
        if (ed.loop) {
            p.setPen(QPen(QColor(255, 160, 40), 2.5, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin));
        } else {
            const QColor c = hasValues_ ? valColor(valueAt(ed.from, time_)) : QColor(120, 120, 130);
            p.setPen(QPen(c, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        }
        p.drawPath(wire);
    }

    // ---- gates ----
    for (int id = 0; id < nodes_.size(); id++) {
        const SchemNode& s = nodes_[id];
        p.save();
        p.translate(pos_[id]);

        QColor outline(200, 200, 210);
        double lw = 1.5;
        if (s.onLoop)          { outline = QColor(255, 160, 40); lw = 2.5; }
        else if (s.onCritical) { outline = QColor(255, 80, 80);  lw = 2.5; }
        p.setPen(QPen(outline, lw));
        p.setBrush(QColor(34, 34, 42));
        drawSymbol(p, s.type);

        // port value inside the flag
        if (isPort(s.type) && hasValues_) {
            const char v = valueAt(id, time_);
            p.setPen(valColor(v));
            p.drawText(QRectF(0, 0, kW, kH), Qt::AlignCenter, QString(QChar(v)));
        }

        // name above
        p.setPen(s.onCritical ? QColor(255, 130, 130) : QColor(215, 215, 225));
        p.drawText(QRectF(-20, -17, kW + 40, 14), Qt::AlignCenter, s.name);

        // delay and arrival time below (gates only)
        if (!isPort(s.type) || s.onCritical) {
            QString info;
            if (!isPort(s.type)) info = QString("d%1").arg(s.delay);
            if (s.arrival >= 0) info += (info.isEmpty() ? "" : " ") + QString("@%1").arg(s.arrival);
            p.setPen(s.onCritical ? QColor(255, 110, 110) : QColor(140, 140, 150));
            p.drawText(QRectF(-20, kH + 2, kW + 40, 14), Qt::AlignCenter, info);
        }
        p.restore();
    }

    // ---- pins (the dots you drag from and to) ----
    p.setPen(QPen(QColor(160, 160, 170), 1));
    for (int id = 0; id < nodes_.size(); id++) {
        p.setBrush(QColor(34, 34, 42));
        const int np = pinCount(nodes_[id].type);
        for (int k = 0; k < np; k++) p.drawEllipse(inPin(id, k), 3, 3);
        if (nodes_[id].type != GATE_OUTPUT) {
            p.setBrush(QColor(160, 160, 170));
            p.drawEllipse(outPin(id), 3, 3);
        }
    }

    // ---- selection ----
    if (selected_ >= 0 && selected_ < pos_.size()) {
        p.setPen(QPen(QColor(90, 170, 255), 1.5, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawRect(QRectF(pos_[selected_] - QPointF(5, 5), QSizeF(kW + 10, kH + 10)));
    }

    // ---- wire being drawn ----
    if (mode_ == Wiring && wireFrom_ >= 0) {
        p.setPen(QPen(QColor(255, 210, 60), 2, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawLine(outPin(wireFrom_), wireEnd_);
        int t = -1, pin = -1;
        if (inPinAt(wireEnd_, t, pin)) p.drawEllipse(inPin(t, pin), 7, 7);
    }
    p.restore();

    // ---- legend ----
    p.setFont(font());
    p.setPen(QColor(170, 170, 180));
    const QString legend = hasValues_
        ? QString("t = %1   (hover over the waveform to scrub)").arg(time_)
        : QString("no simulation data");
    p.drawText(QPointF(10, 16), legend + "   |   red = critical path, d = delay, @ = arrival");
    p.setPen(QColor(120, 120, 130));
    p.drawText(QPointF(10, height() - 8),
               "drag gate = move   drag from right pin to left pin = wire   right-click = menu   Del = delete");
}