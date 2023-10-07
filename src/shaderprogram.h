#pragma once

#include <GL/glew.h> //GLUint

class ShaderProgram
{
  friend class ShaderManager;
private:
  GLuint program;
protected:
  ShaderProgram(GLuint);
public:
  ~ShaderProgram();
  inline operator GLuint() const { return program; }
};
