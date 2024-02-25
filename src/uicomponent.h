#pragma once

#include "context.h"
#include "nodewindow.h"

#ifdef USE_JET_LIVE
#include "jettools.h"
#endif
#include "audioconfigwindow.h"
#include "console.h"
#include "monitoringwindow.h"

#include <GL/glew.h>

class UIComponent
{
private:
  ImColor bgClearColor={0.2f, 0.2f, 0.2f,1.0f};
 public:
  UIComponent();
  int init();
  int tick();
  int shutdown();
  void applyTheme();
  Context context;
  bool initialRun = false;
  bool shouldClose = false;
  Console console;
  AudioConfigWindow audioConfigWindow;
  MonitoringWindow monitoringWindow;
  NodeWindow nodeWindow;
  GLuint vertex_buffer;
  bool show_app_stack_tool=false;
  std::string imGuiINIPath;
#ifdef USE_JET_LIVE
  JetTools jetTools;
#endif
};
