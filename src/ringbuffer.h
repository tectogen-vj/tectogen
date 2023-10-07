#pragma once

#include "soundio/soundio.h"

#include "monitoring.h"

#include <atomic>
#include <cassert>
#include <mutex>

template<class T>
class RingBuffer {
public:
  T* operator[] (const int idx) const {
    if(idx==0) {
      return (T*)(soundio_ring_buffer_read_ptr(buffer));
    }

    if(idx==-1) {
      return (T*)(soundio_ring_buffer_read_ptr(buffer)+(readblocks-1)*effectiveBockSize);
    }
    if(idx>=0) {
      if(idx>=readblocks) {
        return nullptr;
      } else {
        return (T*)(soundio_ring_buffer_read_ptr(buffer)+(idx)*effectiveBockSize);
      }
    } else {
      if((-idx)>=readblocks) {
        return nullptr;
      } else {
        return (T*)(soundio_ring_buffer_read_ptr(buffer)+(readblocks+idx)*effectiveBockSize);
      }
    }
  }
  [[nodiscard]] int length() const { return readblocks; }
  friend class InStreamManager;
private:
  const int blockelems;
  const int readblocks;
  const int align;
  const int effectiveBockSize;
  SoundIoRingBuffer* buffer;
  std::atomic<int> lookahead;
  std::atomic<char*> lookaheadptr;
  std::mutex lookaheadLock;
 public:
  RingBuffer(
    const int readblocks,
    const int writeblocks,
    const int blockelems=1,
    const int align=0) :
    readblocks(readblocks),
    blockelems(blockelems),
    align(align),
    effectiveBockSize(align?((blockelems*sizeof(T)-1)/align+1)*align:blockelems*sizeof(T))
  {
    std::lock_guard<std::mutex> lock(lookaheadLock);
    if(!sioapi) {
      sioapi=soundio_create();
    }
    buffer=soundio_ring_buffer_create(sioapi, effectiveBockSize * (readblocks+writeblocks));

    /* initial layout
     *
     * |'  .RE.AD.  |' W.RI.TE| B.UF.F |
     *
     */
    soundio_ring_buffer_advance_write_ptr(buffer, effectiveBockSize*readblocks);
    lookahead=0;
    lookaheadptr=soundio_ring_buffer_read_ptr(buffer);
  }

  ~RingBuffer() {
    soundio_ring_buffer_destroy(buffer);
  }

  void advanceWritePtr(int elem) {
    soundio_ring_buffer_advance_write_ptr(buffer, elem*sizeof(T));
  }

  void advanceWritePtrBlock(int blocks=1) {
    soundio_ring_buffer_advance_write_ptr(buffer, blocks*effectiveBockSize);
  }

  void advanceReadPtrBlock() {
    // TODO: Bounds checking? (soundio_ring_buffer will assert)
    //soundio_ring_buffer_advance_read_ptr(buffer, blocks*effectiveBockSize);
    assert(hasData());
    std::lock_guard<std::mutex> lock(lookaheadLock);
    lookahead+=1;
    lookaheadptr+=effectiveBockSize;
  }

  void sync() {
    std::lock_guard<std::mutex> lock(lookaheadLock);
    soundio_ring_buffer_advance_read_ptr(buffer, lookahead*effectiveBockSize);
    lookahead=0;
    lookaheadptr=soundio_ring_buffer_read_ptr(buffer);
  }

  bool hasData() {
    std::lock_guard<std::mutex> lock(lookaheadLock);
    int samples_ready=soundio_ring_buffer_fill_count(buffer);
    Monitoring::monitor(samples_ready, "samples ready");
    bool hasData=samples_ready >= (readblocks+lookahead+1)*effectiveBockSize;
    return hasData;
  }

  T* writep() {
    return (T*)(soundio_ring_buffer_write_ptr(buffer));
  }

  T* newest(int idx=0) {
    if(idx>=readblocks) {
      return nullptr;
    } else {
      return (T*)(lookaheadptr+(readblocks-idx-1)*effectiveBockSize);
    }
  }
  T* oldest(int idx=0) {
    if(idx>=readblocks) {
      return nullptr;
    } else {
      return (T*)(lookaheadptr+(idx)*effectiveBockSize);
    }
  }

  /* Thoughts on the pointer arithmetic at play here:
   *
   * readptr<writeptr is the not confusing situation. Then (writeptr-readptr)/sizeof(T) is the delay
   * writeptr<readptr... Maybe we don't have to get into this?
   *
   * There's fill_count and free_count.
   *   fill_count=effectiveBockSize*readblocks+delay
   *   fill_count=effectiveBockSize*writeblocksblocks+buffer
   *
   */
private:
  static SoundIo* sioapi;
};

template<class T> SoundIo* RingBuffer<T>::sioapi=nullptr;

