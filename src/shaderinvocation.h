#pragma once

#include <GL/glew.h> //GLUint

class ShaderInvocation {
public:
  GLuint shader=0;
  GLuint program=0;
  GLint resolution_location=0;
  GLint vpos_location=0;
};
