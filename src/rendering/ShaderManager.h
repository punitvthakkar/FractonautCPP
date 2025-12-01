#ifndef SHADERMANAGER_H
#define SHADERMANAGER_H

#include <QOpenGLShaderProgram>
#include <QString>
#include <memory>

/**
 * @brief Manages OpenGL shader programs for fractal rendering
 *
 * Handles loading, compiling, and linking of vertex and fragment shaders.
 * Provides access to the compiled QOpenGLShaderProgram.
 */
class ShaderManager {
public:
  ShaderManager();
  ~ShaderManager();

  /**
   * @brief Loads and compiles the fractal shader program
   * @return true if successful, false otherwise
   */
  bool loadFractalShader();

  /**
   * @brief Returns the active shader program
   * @return Pointer to QOpenGLShaderProgram, or nullptr if not loaded
   */
  QOpenGLShaderProgram *getProgram() const;

  /**
   * @brief Check if using native double precision
   * @return true if GPU supports and is using native doubles
   */
  bool isUsingNativeDoubles() const { return m_usingNativeDoubles; }

private:
  std::unique_ptr<QOpenGLShaderProgram> m_program;
  bool m_usingNativeDoubles = false;
};

#endif // SHADERMANAGER_H
