#ifndef FRACTALGLWIDGET_H
#define FRACTALGLWIDGET_H

#include "Constants.h"
#include "ShaderManager.h"
#include <QElapsedTimer>
#include <QOpenGLExtraFunctions>
#include <QOpenGLTexture>
#include <QOpenGLWidget>
#include <QTimer>

/**
 * @brief Main OpenGL widget for rendering fractals
 *
 * Handles the rendering loop, shader uniforms, and palette texture generation.
 * Implements the core rendering logic ported from WebGL.
 */
class FractalGLWidget : public QOpenGLWidget, protected QOpenGLExtraFunctions {
  Q_OBJECT

public:
  explicit FractalGLWidget(QWidget *parent = nullptr);
  ~FractalGLWidget() override;

protected:
  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;

  // Mouse interaction
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;

private slots:
  void animate();

private:
  void createPaletteTexture();
  void updateUniforms();
  void updatePhysics(double deltaTime);

  // Helper to split double for emulated precision (Dekker's algorithm)
  struct DoubleSplit {
    float hi;
    float lo;
  };
  DoubleSplit splitDouble(double value);

  // Rendering resources
  ShaderManager m_shaderManager;
  std::unique_ptr<QOpenGLTexture> m_paletteTexture;
  QTimer *m_animationTimer;
  QElapsedTimer m_frameTimer;

  // Full screen quad buffers
  GLuint m_vao;
  GLuint m_vbo;

  // Interaction state
  bool m_isDragging;
  QPointF m_lastMousePos;
  QPointF m_velocity;

  // State
  struct State {
    double zoomCenterX = -0.5;
    double zoomCenterY = 0.0;
    double zoomSize = 3.0;
    int maxIterations = 500;
    int paletteId = 0;
    int fractalType = 0; // 0: Mandelbrot
    double juliaCx = -0.7269;
    double juliaCy = 0.1889;
  };

  State m_state;       // Current state (rendered)
  State m_targetState; // Target state (for smooth interpolation)
};

#endif // FRACTALGLWIDGET_H
