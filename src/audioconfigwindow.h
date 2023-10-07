#pragma once

#include "soundio/soundio.h"

#include <vector>

class AudioConfigWindow {
private:
  std::vector<SoundIo*> apis;
  SoundIoBackend iSelect=SoundIoBackendNone;
  SoundIoDevice* dSelect=nullptr;
  int sampleRate=44100;
  void openSelected();
  int layoutSelect=0;
  SoundIoFormat formatSelect=SoundIoFormatFloat32NE;

public:
  struct ApiInfo {
    const char* name;
    const char* description;
  };
  AudioConfigWindow();
  int init();
  int show();
  static const int preferredSampleRates[];
};
