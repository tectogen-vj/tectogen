#include "multibuffer.h"

MultiBuffer::MultiBuffer(size_t bufsize, int lookbackFrames):
  bufsize(bufsize),
  buf(bufsize*ringCount,0),
  ptr(&buf[0]),
  ringCount(lookbackFrames),
  ring_ptr(ringCount)
{
  printf("I have allocated %i blocks of %zu bytes (%zu) at %p, pointed to by %p\n", ringCount, bufsize, buf.size(), ptr, &ptr);
  void* bptr(&buf[0]);
  for(int i=0; i<ringCount; i++) {
    ring_ptr[i].raw=&buf[i*bufsize];
    //ring_ptr[i+ringCount].raw=&buf[i*bufsize];
  }
}

void MultiBuffer::increment() {
  idx=(idx+1)%ringCount;
  ptr=&buf[idx*bufsize];
}
