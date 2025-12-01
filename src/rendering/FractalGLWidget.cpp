#include "FractalGLWidget.h"
#include <QDebug>
#include <QMouseEvent>
#include <QWheelEvent>
#include <cmath>

FractalGLWidget::FractalGLWidget(QWidget *parent)
    : QOpenGLWidget(parent), m_vao(0), m_vbo(0), m_isDragging(false),
      m_velocity(0, 0) {

  // Enable mouse tracking for smooth interaction
  setMouseTracking(true);
  setFocusPolicy(Qt::StrongFocus);

  // Animation timer for 60 FPS
  m_animationTimer = new QTimer(this);
  connect(m_animationTimer, &QTimer::timeout, this, &FractalGLWidget::animate);
  m_animationTimer->start(16); // ~60 FPS

  m_frameTimer.start();
}

FractalGLWidget::~FractalGLWidget() {
  makeCurrent();
  if (m_vao)
    glDeleteVertexArrays(1, &m_vao);
  if (m_vbo)
    glDeleteBuffers(1, &m_vbo);
  doneCurrent();
}

void FractalGLWidget::initializeGL() {
  initializeOpenGLFunctions();

  // Load shaders
  if (!m_shaderManager.loadFractalShader()) {
    qCritical() << "Failed to load fractal shaders!";
    return;
  }

  // Create full screen quad
  GLfloat vertices[] = {
      -1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f,
  };

  glGenVertexArrays(1, &m_vao);
  glBindVertexArray(m_vao);

  glGenBuffers(1, &m_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // Position attribute (location 0 in shader)
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  // Create palette texture
  createPaletteTexture();
}

void FractalGLWidget::resizeGL(int w, int h) { glViewport(0, 0, w, h); }

void FractalGLWidget::paintGL() {
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  QOpenGLShaderProgram *program = m_shaderManager.getProgram();
  if (!program || !program->bind())
    return;

  updateUniforms();

  // Bind palette texture
  if (m_paletteTexture) {
    glActiveTexture(GL_TEXTURE0);
    m_paletteTexture->bind();
    program->setUniformValue("u_paletteTexture", 0);
  }

  glBindVertexArray(m_vao);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

  program->release();
}

void FractalGLWidget::updateUniforms() {
  QOpenGLShaderProgram *program = m_shaderManager.getProgram();

  // Resolution
  program->setUniformValue("u_resolution", QVector2D(width(), height()));

  // Zoom Center (Double Precision Emulation)
  DoubleSplit centerX = splitDouble(m_state.zoomCenterX);
  DoubleSplit centerY = splitDouble(m_state.zoomCenterY);
  DoubleSplit zoomSize = splitDouble(m_state.zoomSize);

  program->setUniformValue("u_zoomCenter_x", QVector2D(centerX.hi, centerX.lo));
  program->setUniformValue("u_zoomCenter_y", QVector2D(centerY.hi, centerY.lo));
  program->setUniformValue("u_zoomSize", QVector2D(zoomSize.hi, zoomSize.lo));

  // Other uniforms
  program->setUniformValue("u_maxIterations", m_state.maxIterations);
  program->setUniformValue("u_paletteId", m_state.paletteId);
  program->setUniformValue("u_fractalType", m_state.fractalType);
  program->setUniformValue("u_juliaC",
                           QVector2D(m_state.juliaCx, m_state.juliaCy));

  // High precision flag
  bool highPrecision = m_state.zoomSize < 0.001 && m_state.fractalType < 2;
  program->setUniformValue("u_highPrecision", highPrecision);
}

FractalGLWidget::DoubleSplit FractalGLWidget::splitDouble(double value) {
  // Emulate splitDouble from JS/GLSL
  float hi = static_cast<float>(value);
  float lo = static_cast<float>(value - static_cast<double>(hi));
  return {hi, lo};
}

void FractalGLWidget::createPaletteTexture() {
  // Ported from generateFractalExtremePalette in script.js
  // Generates the "Extreme" palette texture (ID 4)
  // For other palettes, the shader uses cosine gradients, but ID 4 needs this
  // texture

  struct ColorStop {
    float pos;
    int r, g, b;
  };
  std::vector<ColorStop> stops = {
      {0.00f, 0, 0, 0},       {0.05f, 25, 7, 26},     {0.10f, 9, 1, 47},
      {0.15f, 4, 4, 73},      {0.20f, 0, 7, 100},     {0.25f, 12, 44, 138},
      {0.30f, 24, 82, 177},   {0.35f, 57, 125, 209},  {0.40f, 134, 181, 229},
      {0.45f, 211, 236, 248}, {0.50f, 241, 233, 191}, {0.55f, 248, 201, 95},
      {0.60f, 255, 170, 0},   {0.65f, 240, 126, 13},  {0.70f, 204, 71, 10},
      {0.75f, 158, 1, 66},    {0.80f, 110, 0, 95},    {0.85f, 106, 0, 168},
      {0.90f, 77, 16, 140},   {0.95f, 45, 20, 80},    {1.00f, 0, 0, 0}};

  const int textureSize = 2048;
  QImage image(textureSize, 1, QImage::Format_RGBA8888);

  for (int i = 0; i < textureSize; ++i) {
    float t = static_cast<float>(i) / (textureSize - 1);

    // Find stops
    ColorStop lower = stops.front();
    ColorStop upper = stops.back();

    for (size_t j = 0; j < stops.size() - 1; ++j) {
      if (t >= stops[j].pos && t <= stops[j + 1].pos) {
        lower = stops[j];
        upper = stops[j + 1];
        break;
      }
    }

    float localT = (t - lower.pos) / (upper.pos - lower.pos);
    float smoothT = localT * localT * (3 - 2 * localT); // Smoothstep

    int r = std::round(lower.r + (upper.r - lower.r) * smoothT);
    int g = std::round(lower.g + (upper.g - lower.g) * smoothT);
    int b = std::round(lower.b + (upper.b - lower.b) * smoothT);

    image.setPixelColor(i, 0, QColor(r, g, b, 255));
  }

  m_paletteTexture = std::make_unique<QOpenGLTexture>(image);
  m_paletteTexture->setMinificationFilter(QOpenGLTexture::Linear);
  m_paletteTexture->setMagnificationFilter(QOpenGLTexture::Linear);
  m_paletteTexture->setWrapMode(QOpenGLTexture::Repeat);
}

// Mouse event handlers
void FractalGLWidget::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    m_isDragging = true;
    m_lastMousePos = event->pos();
    m_velocity = QPointF(0, 0);
  }
}

void FractalGLWidget::mouseMoveEvent(QMouseEvent *event) {
  if (m_isDragging) {
    QPointF delta = event->pos() - m_lastMousePos;

    // Convert pixel delta to fractal coordinates (use height for aspect ratio)
    double scale = m_state.targetZoomSize / height();
    m_state.targetZoomCenterX -= delta.x() * scale;
    m_state.targetZoomCenterY += delta.y() * scale; // Y is inverted

    // Update velocity for momentum (scaled to fractal space)
    m_velocity.setX(delta.x() * scale * 0.5);
    m_velocity.setY(delta.y() * scale * 0.5);

    m_lastMousePos = event->pos();
    // No need to call update() - animation timer handles continuous rendering
  }
}

void FractalGLWidget::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    m_isDragging = false;
  }
}

void FractalGLWidget::wheelEvent(QWheelEvent *event) {
  // Get scroll delta (normalized to +1 or -1)
  double delta = event->angleDelta().y() > 0 ? -1.0 : 1.0;

  // Base zoom factor (from web version)
  double baseFactor = 0.15;

  // Progressive slowdown as we zoom in (exponential)
  // This prevents zoom from "tapping out" too early
  double zoomFactor = 1.0 + baseFactor * std::pow(m_state.targetZoomSize, 0.3);

  // Get mouse position in normalized coordinates
  // CRITICAL: Use height for both X and Y to maintain aspect ratio
  QPointF mousePos = event->position();
  double uvx = (mousePos.x() - width() / 2.0) / height();
  double uvy = (height() - mousePos.y() - height() / 2.0) / height();

  // Calculate world position at mouse cursor
  double wx = m_state.targetZoomCenterX + uvx * m_state.targetZoomSize;
  double wy = m_state.targetZoomCenterY + uvy * m_state.targetZoomSize;

  // Apply zoom
  if (delta > 0) {
    m_state.targetZoomSize *= zoomFactor;
  } else {
    m_state.targetZoomSize /= zoomFactor;
  }

  // Apply smooth zoom limit at 0.5x (zoomSize = 6.0)
  const double maxZoomSize = 6.0;
  if (m_state.targetZoomSize > maxZoomSize) {
    // Smooth resistance with subtle bounce
    double excess = m_state.targetZoomSize - maxZoomSize;
    double resistance = 1.0 / (1.0 + excess * 0.5);
    m_state.targetZoomSize = maxZoomSize + excess * resistance;

    // Snap center to starting position when at limit
    m_state.targetZoomCenterX = -0.5;
    m_state.targetZoomCenterY = 0.0;
  } else {
    // Adjust center to keep mouse position fixed in world space
    m_state.targetZoomCenterX = wx - uvx * m_state.targetZoomSize;
    m_state.targetZoomCenterY = wy - uvy * m_state.targetZoomSize;
  }

  // No need to call update() - animation timer handles continuous rendering
}

// Animation loop (runs at 60 FPS via timer)
void FractalGLWidget::animate() {
  qint64 elapsed = m_frameTimer.elapsed();
  double deltaTime = elapsed / 1000.0;
  m_frameTimer.restart();

  // Cap deltaTime to prevent huge jumps when window is moved/resized
  if (deltaTime > 0.1) deltaTime = 0.016; // ~60 FPS fallback

  updatePhysics(deltaTime);
  update(); // Trigger paintGL
}

void FractalGLWidget::updatePhysics(double deltaTime) {
  // Velocity physics - only when not dragging
  if (!m_isDragging) {
    // Normalize deltaTime to 60 FPS (as per web version)
    double dt60 = deltaTime * 60.0;

    // Apply velocity to target position
    m_state.targetZoomCenterX -= m_velocity.x() * dt60;
    m_state.targetZoomCenterY -= m_velocity.y() * dt60;

    // Apply friction with exponential decay (matches web version)
    const double friction = 0.9;
    double frictionPow = std::pow(friction, dt60);
    m_velocity.setX(m_velocity.x() * frictionPow);
    m_velocity.setY(m_velocity.y() * frictionPow);

    // Early zero-out for better performance (threshold from web version)
    if (std::abs(m_velocity.x()) < 1e-9 && std::abs(m_velocity.y()) < 1e-9) {
      m_velocity = QPointF(0, 0);
    }
  }

  // CRITICAL: Smooth interpolation (lerp) from current to target state
  // This is what makes everything feel smooth!
  const double lerpFactor = 1.0 - std::pow(0.1, deltaTime * 10.0);

  // Apply smooth zoom limit at 0.5x (zoomSize = 6.0)
  const double maxZoomSize = 6.0;
  if (m_state.targetZoomSize > maxZoomSize) {
    double excess = m_state.targetZoomSize - maxZoomSize;
    double resistance = 1.0 / (1.0 + excess * 0.5);
    m_state.targetZoomSize = maxZoomSize + excess * resistance;
    m_state.targetZoomCenterX = -0.5;
    m_state.targetZoomCenterY = 0.0;
  }

  // Smooth lerp interpolation (THE KEY TO SMOOTHNESS!)
  double diffSize = m_state.targetZoomSize - m_state.zoomSize;
  double diffX = m_state.targetZoomCenterX - m_state.zoomCenterX;
  double diffY = m_state.targetZoomCenterY - m_state.zoomCenterY;

  m_state.zoomSize += diffSize * lerpFactor;
  m_state.zoomCenterX += diffX * lerpFactor;
  m_state.zoomCenterY += diffY * lerpFactor;
}
