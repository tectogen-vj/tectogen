#include "instreammanager.h"

#include "app.h"
#include "logs.h"
#include "nodeprograminstancewrapper.h"
#include "nodeprogrammanager.h"
#include "profiler.h"
#include "ringbuffer.h"

#include "fftw3.h"
#include "soundio/soundio.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <complex>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

struct SoundIoRingBuffer;

InStreamManager::InStreamManager():
  analysisThread(&InStreamManager::analysisThreadFunc, this),
  fftElem(blocksize/2+1),
  fftAlign(((fftElem*sizeof(fftwf_complex)-1)/blocksize+1)*blocksize),

  inStreamBuffer(analysisBlocks,totalBufferBlocks-analysisBlocks,blocksize)
{
  /* The buffers holding the results will only be allocated once nodes with fft
   * output are instantiated. Measuring on a temporary buffer created using
   * fftw_malloc works here because the nodes' buffers are create using the
   * ffw_malloc allocator. */
  void* measureBuffer=fftw_malloc(sizeof(fftwf_complex)*fftElem);
  fftPlan=fftwf_plan_dft_r2c_1d(blocksize, inStreamBuffer.writep(), (fftwf_complex*)measureBuffer, FFTW_MEASURE);
  fftw_free(measureBuffer);
}

InStreamManager::~InStreamManager()
{
  running=false;
  terminate=true;
  inputBlockCV.notify_all();
  analysisResultsCV.notify_all();
  soundio_instream_destroy(soundIoInStream);
  fftwf_destroy_plan(fftPlan);
  analysisThread.join();
}

/**
 * @brief start receiving audio
 * @param nSoundIoInStream a not yet start soundio object
 */

void InStreamManager::start(SoundIoInStream *nSoundIoInStream)
{
  int err=0;
  if(nSoundIoInStream->format!=SoundIoFormatFloat32NE) {
    Logs::get().log(logSeverity_Err, "SoundIO", "Only native endian float supported");
    return;
  }
  if((err=soundio_instream_open(nSoundIoInStream))) {
    Logs::get().logf(logSeverity_Err, "SoundIO", "instream open error: %s", soundio_strerror(err));
    return;
  }
  if(soundIoInStream) {
    soundio_instream_destroy(soundIoInStream);
  }
  soundIoInStream=nSoundIoInStream;

  soundIoInStream->read_callback=InStreamManager::soundio_read_callback;
  soundIoInStream->userdata=this;


  // TODO: error handling
  err=soundio_instream_start(soundIoInStream);
  sample_rate=soundIoInStream->sample_rate;
  assert(!err);
}

/**
 * @brief
 * @return whether any audio samples have yet been received
 */
bool InStreamManager::isRunning()
{
  return running;
}

void InStreamManager::waitAnalysis()
{
  std::unique_lock lock(analysisResultsMutex);
  // HACK
  while(!terminate && !hasAnalysisResults) {
    analysisResultsCV.wait(lock);
  }
  hasAnalysisResults=false;
  // HACK??
  inputBlockCV.notify_one();
  inStreamBuffer.sync();
}

const SoundIoInStream * const InStreamManager::instreamInfo()
{
  return soundIoInStream;
}

/**
 * @brief waits for audio samples, then analyzes, then notifies renderer
 * once notified by the audio input thread of a new block of data being filled,
 * this will grab the latest invocation description and excecute the audio part
 * of it. Once finished, it will notify the video thread that it may start
 * to render those results.
 */
void InStreamManager::analysisThreadFunc() {
  std::unique_lock lock(inputBlockMutex);
  lock.unlock();
  while(!terminate) {
    inputBlockCV.wait(lock);
    inStreamBuffer.sync();
    if(running && inStreamBuffer.hasData() && !terminate) {
      inStreamBuffer.advanceReadPtrBlock();


      auto& nodemgr=App::get().nodemanager;
      if(nodemgr.newInvocationList) {
        delete nodemgr.invocationList;
        nodemgr.invocationList.store(nodemgr.newInvocationList);
        nodemgr.newInvocationList=nullptr;
      }
      InvocationList* invocationListPtr=nodemgr.invocationList.load();
      if(invocationListPtr) {
        std::vector<NodeProgramInstanceWrapper>& ivl=invocationListPtr->list;
        for(NodeProgramInstanceWrapper& inv:ivl) {
          inv.invoke();
        }

        // Todo: Wait for renderer to finish here!

        for(NodeProgramInstanceWrapper& inv:invocationListPtr->parametrizedFragments) {
          inv.invoke();
        }

        std::vector<MultiBuffer*>& buffers=invocationListPtr->buffers;
        for(MultiBuffer* buffer:buffers) {
          buffer->increment();
        }

        std::vector<NodeProgramInstanceWrapper>& displays=invocationListPtr->displays;
        auto& video=App::get().video;
        for(NodeProgramInstanceWrapper& disp:displays) {
          App::get().video.context.makeCurrent();

          DisplayPayload* dp=(DisplayPayload*)disp.getProgramState()->userdata;
          video.bindPreview(dp->renderTex.beginWrite());

          glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
          glUseProgram(dp->inv.program);

          glBindVertexArray(video.vertex_array);

          // TODO: Move out of mainloop
          GLint vpos_location = glGetAttribLocation(dp->inv.program, "vPos");

          glEnableVertexAttribArray(dp->inv.vpos_location);
          glUniform2f(dp->inv.resolution_location, dp->getWidth(), dp->getHeight());
          glVertexAttribPointer(dp->inv.vpos_location, 2, GL_FLOAT, GL_FALSE,
                                sizeof(float) * 2, (void*) 0);

          glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
          //glFlush();

          //video.unbindPreview();
          dp->renderTex.endWrite();
        }

      }

      //HACK
      hasAnalysisResults=true;
      analysisResultsCV.notify_one();
    }
  }
}

/**
 * @brief Callback thread for audio smples as documented for soundio
 */
void InStreamManager::soundio_read_callback(SoundIoInStream* instream, int frame_count_min, int frame_count_max) {
  InStreamManager* t=(InStreamManager*)instream->userdata;
  t->running=true;

  SoundIoRingBuffer* rc = t->inStreamBuffer.buffer;
  struct SoundIoChannelArea *areas;
  int err;

  char *write_ptr = soundio_ring_buffer_write_ptr(rc);
  //const int free_bytes = soundio_ring_buffer_free_count(rc);
  const int free_bytes = (t->blocksize*t->totalBufferBlocks*sizeof(float))-soundio_ring_buffer_fill_count(rc);
  const int free_count = free_bytes / instream->bytes_per_frame;

  int store_count=std::min(free_count, frame_count_max);
  {
    const int tbb=t->totalBufferBlocks;
    const int ab=t->analysisBlocks;
    assert(free_bytes <= (tbb-ab)*t->blocksize*sizeof(float));
    assert(store_count<=free_bytes/sizeof(float));
  }
  int frames_left=frame_count_max;
  while(frames_left) {
    int frame_count = frames_left;
    if ((err = soundio_instream_begin_read(instream, &areas, &frame_count))) {
      Logs::get().logf(logSeverity_Err, "SoundIO", "begin read error: %s", soundio_strerror(err));
      exit(1);
    }
    // 1. Fill the remaining free buffer
    int store_count_now=std::min(store_count, frame_count);
    if (!areas) {
      // Due to an overflow there is a hole. Fill the ring buffer with
      // silence for the size of the hole.
      memset(write_ptr, 0, store_count_now * sizeof(float));
    } else {
      for(int frame=0; frame<store_count_now; frame++) {
        memcpy(write_ptr, areas[0].ptr, sizeof(float));
        areas[0].ptr += areas[0].step;
        write_ptr += sizeof(float);
      }
    }
    // 2. Discard any remaining samples
    if ((err = soundio_instream_end_read(instream))) {
      Logs::get().logf(logSeverity_Err, "SoundIO", "end read error: %s", soundio_strerror(err));
      exit(1);
    }
    frames_left -= frame_count;
  }
  soundio_ring_buffer_advance_write_ptr(rc, store_count*sizeof(float));
  // HACK: Should only notify if a block is completed e.g.
  // if(freecount%blocksize<=frame_count)
  t->inputBlockCV.notify_one();
}
