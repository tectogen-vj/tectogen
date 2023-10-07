#include "multibuffer.h"

MultiBuffer::MultiBuffer(size_t bufsize):
  bufsize(bufsize),
  buf(bufsize*ringCount,0),
  ptr(&buf[0])
{
  printf("I have allocated %i blocks of %zu bytes (%zu) at %p, pointed to by %p\n", ringCount, bufsize, buf.size(), ptr, &ptr);
}

void MultiBuffer::increment() {
  idx=(idx+1)%ringCount;
  ptr=&buf[idx*bufsize];
}
