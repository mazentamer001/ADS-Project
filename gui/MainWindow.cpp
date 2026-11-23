#include "MainWindow.hpp"
#include "WaveformView.hpp"
#include "SchematicView.hpp"
#include "SimRunner.hpp"

#include <QFile>
#include <QFileDialog>
#include <QFontDatabase>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QTimer>
#include <QToolBar>

namespace {
const char* kExampleNet =
    "# Full adder. Format: TYPE name delay input1 [input2]\n"
    "# A gate's name is also the name of the net it drives.\n"
    "INPUT  a b cin\n"
    "OUTPUT sum cout\n"
    "\n"
    "XOR  x1   2  a b\n"
    "XOR  sum  2  x1 cin\n"
    "AND  a1   1  a b\n"
    "AND  a2   1  x1 cin\n"
    "OR   cout 1  a1 a2\n";

const char* kExampleStim =
    "# <time> <input net> <value>\n"
    "0   a 0\n"
    "0   b 0\n"
    "0   cin 0\n"
    "20  cin 1\n"
    "40  b 1\n"
    "40  cin 0\n"
    "60  cin 1\n"
    "80  a 1\n"
    "80  b 0\n"
    "80  cin 0\n"
    "100 cin 1\n"
    "120 b 1\n"
    "120 cin 0\n"
    "140 cin 1\n";
}  // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Logic Simulator");

    const QFont mono = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    netEdit_  = new QPlainTextEdit;
    stimEdit_ = new QPlainTextEdit;
    log_      = new QPlainTextEdit;
    netEdit_->setFont(mono);
    stimEdit_->setFont(mono);
    log_->setFont(mono);
    log_->setReadOnly(true);
    netEdit_->setLineWrapMode(QPlainTextEdit::NoWrap);
    stimEdit_->setLineWrapMode(QPlainTextEdit::NoWrap);

    wave_ = new WaveformView;
    schem_ = new SchematicView;
    connect(wave_, &WaveformView::cursorTimeChanged, schem_, &SchematicView::setTime);
    connect(schem_, &SchematicView::netlistEdited, this, &MainWindow::onSchematicEdited);
    connect(schem_, &SchematicView::nodeRenamed, this, &MainWindow::onNodeRenamed);

    auto* tabs = new QTabWidget;
    tabs->addTab(netEdit_, "Netlist (.net)");
    tabs->addTab(stimEdit_, "Stimulus (.stim)");

    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setWidget(wave_);

    auto* right = new QSplitter(Qt::Vertical);
    right->addWidget(schem_);
    right->addWidget(scroll);
    right->addWidget(log_);
    right->setSizes(QList<int>() << 330 << 260 << 150);

    auto* main = new QSplitter(Qt::Horizontal);
    main->addWidget(tabs);
    main->addWidget(right);
    main->setSizes(QList<int>() << 420 << 880);
    setCentralWidget(main);

    QToolBar* tb = addToolBar("Main");
    tb->setMovable(false);
    tb->addAction("Open .net", this, &MainWindow::openNet);
    tb->addAction("Save .net", this, &MainWindow::saveNet);
    tb->addAction("Open .stim", this, &MainWindow::openStim);
    tb->addAction("Save .stim", this, &MainWindow::saveStim);
    tb->addAction("Example", this, &MainWindow::loadExample);
    tb->addSeparator();
    QAction* run = tb->addAction("Run", this, &MainWindow::runSim);
    run->setShortcut(QKeySequence("F5"));
    run->setToolTip("Simulate (F5)");
    tb->addSeparator();
    tb->addAction("Zoom +", wave_, &WaveformView::zoomIn);
    tb->addAction("Zoom -", wave_, &WaveformView::zoomOut);
    tb->addAction("Fit wave", wave_, &WaveformView::zoomFit);
    tb->addAction("Fit schematic", schem_, &SchematicView::fit);
    tb->addAction("Auto layout", schem_, &SchematicView::autoLayout);

    loadExample();
    QTimer::singleShot(0, this, &MainWindow::runSim);
}

bool MainWindow::readInto(QPlainTextEdit* edit, const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Open", "Cannot open " + path);
        return false;
    }
    edit->setPlainText(QString::fromUtf8(f.readAll()));
    return true;
}

bool MainWindow::writeFrom(const QPlainTextEdit* edit, const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Save", "Cannot write " + path);
        return false;
    }
    f.write(edit->toPlainText().toUtf8());
    f.write("\n");
    statusBar()->showMessage("Saved " + path, 3000);
    return true;
}

void MainWindow::openNet() {
    QString path = QFileDialog::getOpenFileName(this, "Open netlist", QString(),
                                                "Netlist (*.net);;All files (*)");
    if (path.isEmpty()) return;
    if (readInto(netEdit_, path)) {
        netPath_ = path;
        schem_->clearData();
        runSim();
    }
}

void MainWindow::openStim() {
    QString path = QFileDialog::getOpenFileName(this, "Open stimulus", QString(),
                                                "Stimulus (*.stim);;All files (*)");
    if (path.isEmpty()) return;
    if (readInto(stimEdit_, path)) {
        stimPath_ = path;
        runSim();
    }
}

void MainWindow::saveNet() {
    if (netPath_.isEmpty())
        netPath_ = QFileDialog::getSaveFileName(this, "Save netlist", "circuit.net",
                                                "Netlist (*.net);;All files (*)");
    if (!netPath_.isEmpty()) writeFrom(netEdit_, netPath_);
}

void MainWindow::saveStim() {
    if (stimPath_.isEmpty())
        stimPath_ = QFileDialog::getSaveFileName(this, "Save stimulus", "input.stim",
                                                 "Stimulus (*.stim);;All files (*)");
    if (!stimPath_.isEmpty()) writeFrom(stimEdit_, stimPath_);
}

void MainWindow::loadExample() {
    netEdit_->setPlainText(kExampleNet);
    stimEdit_->setPlainText(kExampleStim);
    netPath_.clear();
    stimPath_.clear();
    schem_->clearData();   // forget manual positions of the previous circuit
}

void MainWindow::runSim() {
    RunResult r = runSimulation(netEdit_->toPlainText(), stimEdit_->toPlainText());

    QString text;
    if (!r.errors.isEmpty()) text += r.errors + "\n\n";
    text += r.report;
    log_->setPlainText(text.trimmed());

    // the schematic is redrawn whenever the netlist parsed (even with a loop error).
    // If it did not parse, the last drawing stays so a half-finished edit is not lost.
    if (!r.nodes.isEmpty()) schem_->setData(r.nodes, r.edges, r.endTime);

    if (r.ok) {
        wave_->setData(r.waves, r.endTime);
        statusBar()->showMessage("Simulation finished", 3000);
    } else {
        wave_->clearData();
        statusBar()->showMessage("Error: see the message panel", 5000);
    }
}

// A structural edit in the schematic: put the regenerated netlist in the editor and re-run.
void MainWindow::onSchematicEdited(const QString& netlistText) {
    netEdit_->setPlainText(netlistText);
    runSim();
}

// A gate was renamed in the schematic: keep the stimulus lines pointing at it.
void MainWindow::onNodeRenamed(const QString& oldName, const QString& newName) {
    const QRegularExpression re("^(\\s*\\d+\\s+)" + QRegularExpression::escape(oldName) + "(?=\\s|$)");
    QStringList lines = stimEdit_->toPlainText().split("\n");
    for (int i = 0; i < lines.size(); i++) lines[i].replace(re, "\\1" + newName);
    stimEdit_->setPlainText(lines.join("\n"));
}