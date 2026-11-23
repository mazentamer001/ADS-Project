#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QString>

class QPlainTextEdit;
class WaveformView;
class SchematicView;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void openNet();
    void openStim();
    void saveNet();
    void saveStim();
    void loadExample();
    void runSim();
    void onSchematicEdited(const QString& netlistText);
    void onNodeRenamed(const QString& oldName, const QString& newName);

private:
    bool readInto(QPlainTextEdit* edit, const QString& path);
    bool writeFrom(const QPlainTextEdit* edit, const QString& path);

    QPlainTextEdit* netEdit_;
    QPlainTextEdit* stimEdit_;
    QPlainTextEdit* log_;
    WaveformView*   wave_;
    SchematicView*  schem_;
    QString netPath_;
    QString stimPath_;
};

#endif