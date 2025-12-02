#include "ShaderManager.h"
#include <QDebug>
#include <QFile>
#include <QOpenGLContext>

ShaderManager::ShaderManager() : m_program(nullptr) {}

ShaderManager::~ShaderManager() {
  // QOpenGLShaderProgram is automatically cleaned up by unique_ptr
  // but we need to ensure context is active if we were doing manual cleanup
}

bool ShaderManager::loadFractalShader() {
  // OpenGL ES (Android/iOS) doesn't support native double precision
  // Only desktop OpenGL has glUniform1d and GL_ARB_gpu_shader_fp64
#ifndef __ANDROID__
  // Try native double precision first (requires GL_ARB_gpu_shader_fp64)
  qInfo() << "Attempting to load native double precision shader...";

  m_program = std::make_unique<QOpenGLShaderProgram>();

  // Load vertex shader (same for both)
  if (!m_program->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                          ":/shaders/shaders/fractal.vert")) {
    qCritical() << "Failed to compile vertex shader:" << m_program->log();
    return false;
  }

  // Try native double precision fragment shader first
  bool doubleSuccess = m_program->addShaderFromSourceFile(
      QOpenGLShader::Fragment, ":/shaders/shaders/fractal_double.frag");

  if (doubleSuccess && m_program->link()) {
    m_usingNativeDoubles = true;
    qInfo() << "✓ Successfully loaded NATIVE DOUBLE PRECISION shader";
    qInfo() << "  Precision: ~15-17 decimal digits (true 64-bit doubles)";
    return true;
  }

  // Fall back to float-float emulation
  qWarning() << "Native double precision not supported, falling back to "
                "float-float emulation";
  qWarning() << "Double shader error:" << m_program->log();
#else
  // Android: Skip native doubles, use float-float emulation directly
  qInfo() << "Android detected - using float-float emulation (OpenGL ES limitation)";
#endif

  // Recreate program for fallback
  m_program = std::make_unique<QOpenGLShaderProgram>();

  if (!m_program->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                          ":/shaders/shaders/fractal.vert")) {
    qCritical() << "Failed to compile vertex shader:" << m_program->log();
    return false;
  }

  if (!m_program->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                          ":/shaders/shaders/fractal.frag")) {
    qCritical() << "Failed to compile fragment shader:" << m_program->log();
    return false;
  }

  if (!m_program->link()) {
    qCritical() << "Failed to link shader program:" << m_program->log();
    return false;
  }

  m_usingNativeDoubles = false;
  qInfo() << "✓ Loaded float-float emulation shader";
  qInfo() << "  Precision: ~14-15 decimal digits (emulated)";
  return true;
}

QOpenGLShaderProgram *ShaderManager::getProgram() const {
  return m_program.get();
}
