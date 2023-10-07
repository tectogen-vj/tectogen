#include "video.h"

#include "shader.h"

int Video::init() {
  context.createBackground();
  context.init();
  // set up previewFBO for Off-Screen rendering
  glGenFramebuffers(1, &previewFBO);


  // Set up screen plane
  glGenVertexArrays(1, &vertex_array);
  glBindVertexArray(vertex_array);
  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(Shader::screenPlaneVertices), Shader::screenPlaneVertices, GL_STATIC_DRAW);
  glFlush();
  return 0;
}

void Video::bindPreview(GLuint tex) {
  glBindFramebuffer(GL_FRAMEBUFFER, previewFBO);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
  // Use glCheckFramebufferStatus to verify that the framebuffer is complete
  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if(status != GL_FRAMEBUFFER_COMPLETE) {
    throw;
  }
}

void Video::unbindPreview() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
