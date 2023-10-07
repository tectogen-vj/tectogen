#pragma once

/*
 * beginWrite and endWrite may only be used in the texture write-thread
 * beginDisplay and endDisplay may only be used in the texture display-thread
 *
 * the write-thread may decide to want to render to a texture at any time
 * the display-thread may decide to want to display the texture to which A has most recently finished writing at any time
 * they shouldn't need to wait for each others render/display
 *
 * the threads will never work on the same texture
 */

#include <GL/glew.h>
#include <mutex>

class AsyncTex {
private:
  GLsync sync=nullptr;

  void finalizeWrite();

  std::mutex mtx;
  bool isWriting=false;
  GLuint writing;
  bool isShowing=false;
  GLuint showing;
  bool isReady=false;
  GLuint ready;

  GLuint mktex(unsigned int w, unsigned int h);
public:
  AsyncTex(unsigned int w, unsigned int h);
  ~AsyncTex();
  GLuint beginWrite();
  void endWrite();
  GLuint beginShow();
  void endShow();

  bool everWritten=false;
};
