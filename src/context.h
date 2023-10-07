#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cstddef>

class Context {
public:
  int ogl_major, ogl_minor;
  const char* glsl_version = NULL;
  Context();
  int create(const char* title, bool background=false);
  int createBackground();
  int init();
  GLFWwindow* window=nullptr;
  void makeCurrent();
};
