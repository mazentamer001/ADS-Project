#include "MainWindow.hpp"
#include <QApplication>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    MainWindow w;
    w.resize(1300, 800);
    w.show();
    return app.exec();
}