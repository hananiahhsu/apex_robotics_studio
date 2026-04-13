#include "MainWindow.h"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    apex::qt::MainWindow window;
    window.show();
    return app.exec();
}
