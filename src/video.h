#pragma once

#include "context.h"
#include <GL/glew.h> //GLUint


class Video {
public:
  GLuint previewFBO;
  GLuint vertex_array, vertex_buffer;
  int init();
  // Bind previewFBO to the texture identified by tex
  void bindPreview(GLuint tex);
  // Unbind previewFBO
  void unbindPreview();
  Context context;
};
