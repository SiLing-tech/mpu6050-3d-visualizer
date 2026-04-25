#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("MPU6050 Visualizer");

    MainWindow window;
    window.setWindowTitle("MPU6050 3D Visualizer");
    window.resize(1280, 720);
    window.show();

    return app.exec();
}
