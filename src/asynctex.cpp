#include "asynctex.h"


void AsyncTex::finalizeWrite() {
  glDeleteSync(sync);
  sync=nullptr;
  isWriting=false;
  isReady=true;
  std::swap(writing,ready);
  everWritten=true;
}

GLuint AsyncTex::mktex(unsigned int w, unsigned int h) {
  GLuint tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  return tex;
}

AsyncTex::AsyncTex(unsigned int w, unsigned int h) :
  writing(mktex(w,h)),
  showing(mktex(w,h)),
  ready(mktex(w,h))
{
}

AsyncTex::~AsyncTex() {
  glDeleteTextures(1, &writing);
  glDeleteTextures(1, &showing);
  glDeleteTextures(1, &ready);
}

GLuint AsyncTex::beginWrite() {
  std::lock_guard<std::mutex> lock(mtx);
  if(sync) {
    glWaitSync(sync, 0, GL_TIMEOUT_IGNORED);
    finalizeWrite();
  }
  if(isWriting) {
    throw;
  }
  isWriting=true;
  return writing;
}

void AsyncTex::endWrite() {
  std::lock_guard<std::mutex> lock(mtx);
  sync=glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
  if(!isWriting) {
    throw;
  }
}

GLuint AsyncTex::beginShow() {
  std::lock_guard<std::mutex> lock(mtx);
  if(isShowing) {
    throw;
  }
  if(sync) {
    GLint status;
    glGetSynciv(sync, GL_SYNC_STATUS, sizeof(status), nullptr, &status);
    if(status==GL_SIGNALED) {
      finalizeWrite();
    }
  }
  isShowing=true;
  if(isReady) {
    std::swap(ready,showing);
    isReady=false;
  }
  return showing;
}

void AsyncTex::endShow() {
  std::lock_guard<std::mutex> lock(mtx);
  isShowing=false;
}
