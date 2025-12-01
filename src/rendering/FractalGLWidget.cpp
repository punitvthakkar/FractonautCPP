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

    // Convert pixel delta to fractal coordinates
    double pixelToFractal = m_state.zoomSize / height();
    m_state.zoomCenterX -= delta.x() * pixelToFractal;
    m_state.zoomCenterY += delta.y() * pixelToFractal; // Y is inverted

    // Update velocity for momentum
    m_velocity = delta * 0.5;

    m_lastMousePos = event->pos();
    update();
  }
}

void FractalGLWidget::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    m_isDragging = false;
  }
}

void FractalGLWidget::wheelEvent(QWheelEvent *event) {
  // Logarithmic zoom with focal point
  double zoomFactor = event->angleDelta().y() > 0 ? 0.9 : 1.1;

  // Get mouse position in fractal coordinates
  QPointF mousePos = event->position();
  double pixelToFractal = m_state.zoomSize / height();

  double mouseFractalX =
      m_state.zoomCenterX + (mousePos.x() - width() / 2.0) * pixelToFractal;
  double mouseFractalY =
      m_state.zoomCenterY - (mousePos.y() - height() / 2.0) * pixelToFractal;

  // Zoom
  double oldZoomSize = m_state.zoomSize;
  m_state.zoomSize *= zoomFactor;

  // Adjust center to keep mouse position fixed
  double newPixelToFractal = m_state.zoomSize / height();
  m_state.zoomCenterX =
      mouseFractalX - (mousePos.x() - width() / 2.0) * newPixelToFractal;
  m_state.zoomCenterY =
      mouseFractalY + (mousePos.y() - height() / 2.0) * newPixelToFractal;

  update();
}

// Animation loop
void FractalGLWidget::animate() {
  double deltaTime = m_frameTimer.elapsed() / 1000.0;
  m_frameTimer.restart();

  updatePhysics(deltaTime);
  update();
}

void FractalGLWidget::updatePhysics(double deltaTime) {
  if (!m_isDragging && m_velocity.manhattanLength() > 0.01) {
    // Apply momentum
    double pixelToFractal = m_state.zoomSize / height();
    m_state.zoomCenterX -= m_velocity.x() * pixelToFractal;
    m_state.zoomCenterY += m_velocity.y() * pixelToFractal;

    // Apply friction (from appconstitution.md: 0.9)
    m_velocity *= 0.9;

    // Stop if velocity is very small
    if (std::abs(m_velocity.x()) < 0.01 && std::abs(m_velocity.y()) < 0.01) {
      m_velocity = QPointF(0, 0);
    }
  }
}
