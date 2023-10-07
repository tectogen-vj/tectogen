#pragma once

#include "ringbuffer.h"

#include "soundio/soundio.h"

#include <atomic>
#include <complex>
#include <condition_variable>
#include <fftw3.h>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

struct SoundIo;
struct SoundIoInStream;

/* How the ring buffer is organized:
 *
 * a total of totalBufferBlocks is composed of:
 *
 *   a section for analysis the beginning of which is marked by the readptr.
 *
 *   a section of fully read samples called delay which means that the analysis (or consumption of analyzed data) is lagging behind.
 *   To prevent the ringbuffer from overflowing,
 *   InStreamManager will not analyze blocks which lag behind writepointer more than maxDelayBlocks full blocks and instead skip blocks
 *   In an emergency situation where the read callback would advance beyond read ptr, samples will be discarded
 *
 *   one input block within which the writeptr progresses.
 *   When the read callback advances writeptr over a block boundary, it will signal InStreamManager to process more data.
 *   If the callback receives so many samples that after copying them, write ptr would advance beyond read ptr, samples will be discarded.
 *
 * To prevent ring buffer *overflows*, analysis results can be extracted either via a nonblocking method that returns a null value if no new analysis data is available
 * or via a blocking method that will wait for data before running the analysis
 *
 */

/* The different statuses of the InStreamManager worker thread:
 *
 *   Waiting for samples, no current results:
 *     the newest block of samples has already been analyzed, waiting for the read_callback to fill a block. getAnalysisResults will block
 *     Next status: Analyzing samples
 *
 *   Analyzing samples:
 *     the read_callback has signalled the thread to analyze blocksize*analysisBlocks blocks starting at read ptr, it propagates the fft buffer with the fft results
 *     of the blocksize newest samples from inStreamBuffer. getAnalysisResults will block
 *
 *   Waiting for samples, current results:
 *     result is ready to be consumed
 *
 */

class InStreamManager
{
public: // public interface to be called from the main thread, must be safe towards analysis and soundio thread
  InStreamManager();
  ~InStreamManager();

  /* for fftw plan reuse, blocksize (or both the real and complex arrays of such size)
  *  has to align with SIMD-alignment!
  *  This should be safe to assume with blocksize=1024
  *  Otherwise: Change initialization and analysis func to use one array and copy!
  *  https://www.fftw.org/fftw3_doc/New_002darray-Execute-Functions.html
  */
  const int blocksize=1024;
  const int totalBufferBlocks=512;
  //const int maxDelayBlocks=3;
  const int analysisBlocks=500;
  const int fftElem; // The Number of complex elements of the fft result
  const int fftAlign; // The size in bytes of an fft buffer including padding, multiple of blocksize

  int sample_rate;

  void start(SoundIoInStream *soundIoInStream);
  bool isRunning();
  void stop();
  void waitAnalysis();
  const SoundIoInStream* const instreamInfo();

  RingBuffer<float> inStreamBuffer;

  fftwf_plan fftPlan;
private:

  std::mutex inputBlockMutex;
  std::condition_variable inputBlockCV;

  std::mutex analysisResultsMutex;
  std::condition_variable analysisResultsCV;
  std::atomic_bool hasAnalysisResults=false;

  std::thread analysisThread;
  void analysisThreadFunc();

  std::atomic_bool running=false;
  bool terminate=false;

  SoundIo* dummySoundIO;

  SoundIoInStream *soundIoInStream=nullptr;

  static void soundio_read_callback(struct SoundIoInStream *, int frame_count_min, int frame_count_max);
  static void soundio_overflow_callback(struct SoundIoInStream *);
  static void soundio_error_callback(struct SoundIoInStream *, int err);
  const void* soundio_userdata;
};
