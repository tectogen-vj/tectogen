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
 public:
  UIComponent();
  int init();
  int tick();
  int shutdown();
  Context context;
  bool initialRun = false;
  bool shouldClose = false;
  Console console;
  AudioConfigWindow audioConfigWindow;
  MonitoringWindow monitoringWindow;
  NodeWindow nodeWindow;
  GLuint vertex_buffer;
  bool show_app_stack_tool=false;
#ifdef USE_JET_LIVE
  JetTools jetTools;
#endif
};
