#include "ShaderManager.h"
#include <QDebug>
#include <QFile>

ShaderManager::ShaderManager() : m_program(nullptr) {}

ShaderManager::~ShaderManager() {
  // QOpenGLShaderProgram is automatically cleaned up by unique_ptr
  // but we need to ensure context is active if we were doing manual cleanup
}

bool ShaderManager::loadFractalShader() {
  m_program = std::make_unique<QOpenGLShaderProgram>();

  // Load and compile vertex shader
  if (!m_program->addShaderFromSourceFile(QOpenGLShader::Vertex,
                                          ":/shaders/shaders/fractal.vert")) {
    qCritical() << "Failed to compile vertex shader:" << m_program->log();
    return false;
  }

  // Load and compile fragment shader
  if (!m_program->addShaderFromSourceFile(QOpenGLShader::Fragment,
                                          ":/shaders/shaders/fractal.frag")) {
    qCritical() << "Failed to compile fragment shader:" << m_program->log();
    return false;
  }

  // Link shader program
  if (!m_program->link()) {
    qCritical() << "Failed to link shader program:" << m_program->log();
    return false;
  }

  return true;
}

QOpenGLShaderProgram *ShaderManager::getProgram() const {
  return m_program.get();
}
