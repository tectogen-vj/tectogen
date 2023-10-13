#pragma once

#include "instreammanager.h"
#include "nodeprogrammanager.h"
#include "shadermanager.h"
#include "uicomponent.h"
#include "video.h"

#include <chrono>

class App {
  /*
   * SINGLETON
   */
  
public:
  static App& get();
  
private:
  App();
  
public:
  App(App const&) = delete;
  void operator=(App const&) = delete;
  
  /*
   * NOTELGNIS
   */
public:
  const char* title="tectogen";
  int run(int argc, char** argv);
  std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
  InStreamManager instreammanager; // REFACTOR: Audio Component
  ShaderManager shadermanager;
  NodeProgramManager nodemanager;
  Video video;

  UIComponent ui;

private:
  int init();
  int tick();
  int shutdown();

  int argc;
  char** argv;
};
