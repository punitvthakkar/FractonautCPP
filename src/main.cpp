#include "rendering/FractalGLWidget.h"
#include <QApplication>
#include <QMainWindow>
#include <QSurfaceFormat>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  // Set OpenGL format to Core Profile 4.1 (macOS maximum)
  QSurfaceFormat format;
  format.setVersion(4, 1);
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
  format.setSwapInterval(1); // V-sync for smooth 60 FPS
  QSurfaceFormat::setDefaultFormat(format);

  QMainWindow window;
  window.setWindowTitle("Fractonaut");
  window.resize(1024, 768);

  // Use our new FractalGLWidget
  FractalGLWidget *glWidget = new FractalGLWidget();
  window.setCentralWidget(glWidget);

  window.show();

  return app.exec();
}
