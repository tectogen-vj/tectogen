#define CONFIG_IMPL
#include "config.h"
#undef CONFIG_IMPL

#include "logs.h"

#include "imgui.h"
#include "imgui_internal.h" // for ImGuiSettingsHandler, ImHashStr, ImGui,Context
#include "visit_struct/visit_struct.hpp"

#include <cstdlib>
#include <cstring> // for strcmp
#include <map>
#include <sstream>
#include <string>
#include <utility>

namespace Shaderaudio::Config {

std::map<std::string, std::string> kv;
Audio audio;

inline std::string cstr(std::string s) {return s;};
inline std::string cstr(int i) {return std::to_string(i);};
inline void set(std::string& t, const std::string s) {t=s;};
inline void set(int& t, const std::string s){t=std::stoi(s);};

void WriteAll(ImGuiContext *ctx, ImGuiSettingsHandler *handler, ImGuiTextBuffer *buf) {
  buf->appendf("[%s][Audio]\n", handler->TypeName);
  visit_struct::for_each(audio, [&buf](const char * name, const auto & value) {
    buf->appendf("%s=%s\n", name, cstr(value).c_str());
  });
  buf->appendf("\n");
}

void *ReadOpen(ImGuiContext *, ImGuiSettingsHandler *, const char *name)
{
  {
    if (strcmp(name, "Audio") != 0) {
      return NULL;
    }
    return (void*)1; // HACK
  }
}

void ReadLine(ImGuiContext *ctx, ImGuiSettingsHandler *, void *, const char *line)
{
  Logs::get().log(logSeverity_Warn, "UI", "ini handler ReadLineFn Stub");
  std::string k,v;
  std::istringstream iss(line);
  if(line[0]!='\0') {
    Logs::get().logf(logSeverity_Warn, "UI", "line %s", line);
    std::getline(iss, k, '=');
    std::getline(iss, v);
    if(k.length()>0) {
      kv[k]=v;
    }
  }
}

void ApplyAll(ImGuiContext* ctx, ImGuiSettingsHandler*)
{
  Logs::get().log(logSeverity_Warn, "UI", "ini handler ApplyAllFn Stub");
  visit_struct::for_each(audio, [](const char * name, auto & value) {
    const auto& v=kv.find(name);
    if(v!=kv.end()) {
      set(value, v->second);
    }
  });
}

void setHandler(ImGuiContext &context)
{
  ImGuiSettingsHandler ini_handler;
  ini_handler.TypeName = "Config";
  ini_handler.TypeHash = ImHashStr("Config");
  ini_handler.ClearAllFn = [](ImGuiContext* ctx, ImGuiSettingsHandler*){
    //Logs::get().log(logSeverity_Warn, "UI", "ini handler ClearAllFn Stub");
  };
  ini_handler.ReadInitFn = [](ImGuiContext* ctx, ImGuiSettingsHandler*){
    //Logs::get().log(logSeverity_Warn, "UI", "ini handler ReadInitFn Stub");
  };
  ini_handler.ReadOpenFn = Shaderaudio::Config::ReadOpen;
  //Logs::get().log(logSeverity_Warn, "UI", "ini handler ReadOpenFn Stub");
  ini_handler.ReadLineFn = Shaderaudio::Config::ReadLine;
  ini_handler.ApplyAllFn = Shaderaudio::Config::ApplyAll;
  ini_handler.WriteAllFn = Shaderaudio::Config::WriteAll;
  context.SettingsHandlers.push_back(ini_handler);
}

}
