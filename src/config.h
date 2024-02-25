#pragma once

#ifdef CONFIG_IMPL
#include <visit_struct/visit_struct.hpp>
#endif

#include "imconfig.h"
#include "imgui.h"

#include <string>

struct ImGuiContext;
struct ImGuiSettingsHandler;
struct ImGuiTextBuffer;

namespace Shaderaudio::Config {

void WriteAll(ImGuiContext* ctx, ImGuiSettingsHandler* handler, ImGuiTextBuffer* buf);
void* ReadOpen(ImGuiContext*, ImGuiSettingsHandler*, const char* name);
void ReadLine(ImGuiContext* ctx, ImGuiSettingsHandler*, void*, const char* line);
void ApplyAll(ImGuiContext* ctx, ImGuiSettingsHandler*);

void setHandler(ImGuiContext& context);

struct TectogenConf {
  std::string backendName{};
  std::string deviceName{};
  int samplerate{};
  std::string layoutName{};
};
extern TectogenConf tectogenConf;
}
#ifdef CONFIG_IMPL
VISITABLE_STRUCT(Shaderaudio::Config::TectogenConf,
  backendName,
  deviceName,
  samplerate,
  layoutName
);
#endif

