#ifndef JETTOOLS_H
#define JETTOOLS_H

#include <jet/live/Live.hpp>
#include <jet/live/Utility.hpp>

#include "imconfig.h"
#include "imgui.h"
#include "logs.h"

#include <iostream>
#include <memory>

class EventListener : public jet::ILiveListener {
 public:
  void onLog(jet::LogSeverity severity, const std::string& message) override {
    logSeverity s;
    switch (severity) {
      case jet::LogSeverity::kDebug:
        s = logSeverity_Debug;
        break;
      case jet::LogSeverity::kInfo:
        s = logSeverity_Info;
        break;
      case jet::LogSeverity::kWarning:
        s = logSeverity_Warn;
        break;
      default:
        s = logSeverity_Err;
        break;
    }
    Logs::get().log(s, "JetLive", message);
  }
  void onCodePreLoad() override {}
  void onCodePostLoad() override {}
};

class JetTools {
 private:
  jet::Live liveInstance;

 public:
  JetTools() : liveInstance(jet::make_unique<EventListener>()) {}
  void update() {
    liveInstance.update();
  };
  void reload() {
    liveInstance.tryReload();
  };
};

#endif  // JETTOOLS_H
