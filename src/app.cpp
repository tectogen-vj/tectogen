#include "app.h"
#include "logs.h"

#include <cstdlib>
#include <memory>
#include <filesystem>

int App::init() {

  atexit([](){App::get().shutdown();});


  ui.init();
  video.init();
  shadermanager.init();
  std::filesystem::path bindir = std::filesystem::weakly_canonical(std::filesystem::path(argv[0])).parent_path();
  std::filesystem::path watchdir=bindir.parent_path()/"share"/"tectogen"/"shaders";
  shadermanager.watch(watchdir.string());

  ui.context.makeCurrent();

  return 0;
}

int App::tick() {
  return ui.tick();
}

int App::shutdown() {
  Logs::get().log(logSeverity_Info, "UI", "Shutting Down");
  ui.shutdown();
  shadermanager.shutdown();
  return 0;
}

App& App::get() {
  static App instance;
  return instance;
}

App::App() {
  startTime = std::chrono::high_resolution_clock::now();
}

int App::run(int argc, char **argv) {
  this->argc=argc;
  this->argv=argv;
  init();
  Logs::get().log(logSeverity_Debug, "UI", "Initialization finished");
  tick();
  Logs::get().log(logSeverity_Info, "UI", "Starting main loop");
  while (!ui.shouldClose) {
    tick();
  }

  // shutdown called by atexit!

  return 0;
}
