#include "FractalGLWidget.h"
#include <QApplication>
#include <QClipboard>
#include <QDebug>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <cmath>

FractalGLWidget::FractalGLWidget(QWidget *parent)
    : QOpenGLWidget(parent), m_vao(0), m_vbo(0), m_isDragging(false),
      m_velocity(0, 0) {

  // Initialize state
  m_state = State();
  m_targetState = m_state;

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

void FractalGLWidget::resizeGL(int w, int h) {
  // w and h are already multiplied by devicePixelRatio in QOpenGLWidget if set
  // up correctly But we should rely on devicePixelRatio() for uniforms to be
  // safe
  glViewport(0, 0, w, h);
}

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

  // Handle High-DPI: Use physical pixels for resolution
  float dpr = devicePixelRatio();
  program->setUniformValue("u_resolution",
                           QVector2D(width() * dpr, height() * dpr));

  // Zoom Center (Double Precision Emulation)
  DoubleSplit centerX = splitDouble(m_state.zoomCenterX);
  DoubleSplit centerY = splitDouble(m_state.zoomCenterY);
  DoubleSplit zoomSize = splitDouble(m_state.zoomSize);

  program->setUniformValue("u_zoomCenter_x_hi", centerX.hi);
  program->setUniformValue("u_zoomCenter_x_lo", centerX.lo);
  program->setUniformValue("u_zoomCenter_y_hi", centerY.hi);
  program->setUniformValue("u_zoomCenter_y_lo", centerY.lo);
  program->setUniformValue("u_zoomSize_hi", zoomSize.hi);
  program->setUniformValue("u_zoomSize_lo", zoomSize.lo);

  // Other uniforms
  program->setUniformValue("u_maxIterations", m_state.maxIterations);
  program->setUniformValue("u_paletteId", m_state.paletteId);
  program->setUniformValue("u_fractalType", m_state.fractalType);
  program->setUniformValue("u_juliaC",
                           QVector2D(m_state.juliaCx, m_state.juliaCy));

  // High precision flag - Enable much earlier to avoid pixelation
  // Also ensure it's on for deep zooms
  // DEBUG: Force true to verify if logic is the issue
  bool highPrecision =
      true; // m_state.zoomSize < 0.1 && m_state.fractalType < 2;
  program->setUniformValue("u_highPrecision", highPrecision);

  // DEBUG: Print split values for the coordinates causing issues
  static int debugCounter = 0;
  if (debugCounter++ % 300 == 0) { // Print every ~5 seconds
    // Test coordinate from user report
    double testVal = -0.9197790506151321;
    DoubleSplit split = splitDouble(testVal);
    qDebug() << "--- Split Logic Verification ---";
    qDebug() << "Input:" << QString::number(testVal, 'g', 17);
    qDebug() << "Hi:" << QString::number(split.hi, 'g', 9);
    qDebug() << "Lo:" << QString::number(split.lo, 'g', 9);
    qDebug() << "Reconstructed:"
             << QString::number((double)split.hi + (double)split.lo, 'g', 17);
    qDebug() << "Error:"
             << QString::number(testVal - ((double)split.hi + (double)split.lo),
                                'g', 17);
  }

  // DEBUG: Print split values for the coordinates causing issues
  // static int debugCounter = 0; // This was moved above
  if (debugCounter++ % 300 == 0) { // Print every ~5 seconds
    qDebug() << "Center X:" << m_state.zoomCenterX;
    qDebug() << "Split X:" << centerX.hi << centerX.lo;
    qDebug() << "High Precision:" << highPrecision;
  }
}

FractalGLWidget::DoubleSplit FractalGLWidget::splitDouble(double value) {
  // Emulate splitDouble from JS/GLSL
  // Simple cast method is most robust for passing double as two floats
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
    m_lastMousePos = event->position(); // Use floating point position
    m_velocity = QPointF(0, 0);

    // Sync target to current when starting drag to avoid jumps
    m_targetState.zoomCenterX = m_state.zoomCenterX;
    m_targetState.zoomCenterY = m_state.zoomCenterY;
  }
}

void FractalGLWidget::mouseMoveEvent(QMouseEvent *event) {
  if (m_isDragging) {
    QPointF currentPos = event->position();
    QPointF delta = currentPos - m_lastMousePos;

    // Convert pixel delta to fractal coordinates
    // Use logical height since event coordinates are logical
    double pixelToFractal = m_state.zoomSize / height();

    // Direct manipulation for immediate response during drag
    m_state.zoomCenterX -= delta.x() * pixelToFractal;
    m_state.zoomCenterY +=
        delta.y() * pixelToFractal; // Y is inverted in fractal space

    // Update target too so it doesn't drift back
    m_targetState.zoomCenterX = m_state.zoomCenterX;
    m_targetState.zoomCenterY = m_state.zoomCenterY;

    // Update velocity for momentum - smoother calculation
    // Use a simple low-pass filter or just the raw delta
    m_velocity = delta;

    m_lastMousePos = currentPos;
    update();
  }
}

void FractalGLWidget::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    m_isDragging = false;
  }
}

#include <QApplication>
#include <QClipboard>

void FractalGLWidget::wheelEvent(QWheelEvent *event) {
  // Smoother, slower zoom factor for "satisfying" feel
  // Was 0.8/1.25, making it closer to 1.0 slows it down
  double zoomFactor = event->angleDelta().y() > 0 ? 0.92 : 1.08;

  // Get mouse position
  QPointF mousePos = event->position();

  // Calculate mouse position in fractal space relative to CURRENT center
  double relX = mousePos.x() - width() / 2.0;
  double relY = mousePos.y() - height() / 2.0;
  double pixelToFractal = m_targetState.zoomSize / height();

  double mouseFractalX = m_targetState.zoomCenterX + relX * pixelToFractal;
  double mouseFractalY = m_targetState.zoomCenterY - relY * pixelToFractal;

  // Update TARGET zoom size
  m_targetState.zoomSize *= zoomFactor;

  // Calculate new target center to keep mouse position fixed
  double newPixelToFractal = m_targetState.zoomSize / height();
  m_targetState.zoomCenterX = mouseFractalX - relX * newPixelToFractal;
  m_targetState.zoomCenterY = mouseFractalY + relY * newPixelToFractal;

  // Don't call update() here, let animate() handle the interpolation
}

void FractalGLWidget::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_P) {
    qDebug() << "--- Debug Coordinates ---";
    qDebug() << "X:" << QString::number(m_state.zoomCenterX, 'g', 16);
    qDebug() << "Y:" << QString::number(m_state.zoomCenterY, 'g', 16);
    qDebug() << "Zoom:" << QString::number(m_state.zoomSize, 'g', 16);
    qDebug() << "High Precision:" << (m_state.zoomSize < 0.1);
    qDebug() << "-------------------------";
  }
  if (event->key() == Qt::Key_C) {
    QString coords = QString("X: %1\nY: %2\nZoom: %3")
                         .arg(QString::number(m_state.zoomCenterX, 'g', 16))
                         .arg(QString::number(m_state.zoomCenterY, 'g', 16))
                         .arg(QString::number(m_state.zoomSize, 'g', 16));

    QApplication::clipboard()->setText(coords);
    qDebug() << "Coordinates copied to clipboard!";
  }
  QOpenGLWidget::keyPressEvent(event);
}

// Animation loop
void FractalGLWidget::animate() {
  // Use fixed time step for physics to avoid instability
  updatePhysics(0.016); // ~60 FPS

  // Always update for smooth zoom interpolation
  update();
}

void FractalGLWidget::updatePhysics(double deltaTime) {
  // 1. Smooth Zoom Interpolation
  // Interpolate zoom size (logarithmic interpolation would be better, but
  // linear on value is okay for small steps) Using exponential smoothing:
  // current += (target - current) * factor
  double smoothFactor = 0.08; // Slightly smoother than 0.1

  m_state.zoomSize +=
      (m_targetState.zoomSize - m_state.zoomSize) * smoothFactor;
  m_state.zoomCenterX +=
      (m_targetState.zoomCenterX - m_state.zoomCenterX) * smoothFactor;
  m_state.zoomCenterY +=
      (m_targetState.zoomCenterY - m_state.zoomCenterY) * smoothFactor;

  // 2. Momentum Panning
  if (!m_isDragging && m_velocity.manhattanLength() > 0.1) {
    double pixelToFractal = m_state.zoomSize / height();

    // Apply velocity to BOTH current and target to maintain momentum
    double dx = m_velocity.x() * pixelToFractal;
    double dy = m_velocity.y() * pixelToFractal;

    m_state.zoomCenterX -= dx;
    m_state.zoomCenterY += dy;
    m_targetState.zoomCenterX -= dx;
    m_targetState.zoomCenterY += dy;

    // Apply friction - tune this for "sacred smoothness"
    // 0.95 is smoother/longer slide than 0.9
    m_velocity *= 0.92;

    // Stop if velocity is very small
    if (m_velocity.manhattanLength() < 0.1) {
      m_velocity = QPointF(0, 0);
    }
  }
}
