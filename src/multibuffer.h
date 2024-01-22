#ifndef MULTIBUFFER_H
#define MULTIBUFFER_H

#include "fftw3.h"
#include "nodeprogramapi.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

template<typename Tdata>
class fftw_allocator : public std::allocator<Tdata>
{
public:
    template <typename U>
    struct rebind { typedef fftw_allocator<U> other; };
    Tdata* allocate(size_t n) { return (Tdata*) fftw_malloc(sizeof(Tdata) * n); }
    void deallocate(Tdata* data, std::size_t size) { fftw_free(data); }
};

template <typename T>
using fftw_vector = std::vector<T, fftw_allocator<T>>;

class MultiBuffer
{
public:
  const int ringCount;
  // A pointer to each element within the ring buffer to ease access from within a node program (receives this buffer as the ring_buffer member of tn_PortBuffer)
  std::vector<tn_PortMessage> ring_ptr;
protected:
  const size_t bufsize;
  size_t idx=0;
  fftw_vector<uint8_t> buf;
  void* ptr;

public:
  class Accessor {
  private:
    const size_t idx;
    MultiBuffer& m;
  protected:
    Accessor(MultiBuffer& m):
      idx(m.idx),
      m(m)
    {

    }
    friend class MultiBuffer;
  public:
    inline size_t getBufsize() const {
      return m.bufsize;
    }
    inline void* operator[](int idx) {
      if(idx>0) {
        return &m.buf[((m.idx+1+idx)%m.ringCount)*m.bufsize];
      } else {
        return &m.buf[((m.idx+(m.ringCount+idx))%m.ringCount)*m.bufsize];
      }
    }
  };
  MultiBuffer(size_t bufsize, int lookbackFrames);
  inline void** getWrite() {
    return &ptr;
  }
  void increment();
  inline Accessor getAccessor() {
    return Accessor(*this);
  }
  friend class Accessor;
};

#endif // MULTIBUFFER_H
