#include "console.h"

#include "logs.h"

#include "imconfig.h"
#include "imgui.h"

#include <cstddef>
#include <string>

int Console::show() {
  //ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
  //ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
  ImGui::Begin("Logs", NULL, ImGuiWindowFlags_MenuBar);
  ImGui::BeginMenuBar();
  static logSeverity minLevel = logSeverity_Debug | (logSeverity_Debug - 1);
  if (ImGui::BeginMenu("LogLevel")) {
    if (ImGui::MenuItem("Error", NULL, minLevel & logSeverity_Err)) {
      minLevel = logSeverity_Err;
      minLevel |= (logSeverity_Err - 1);
    }
    if (ImGui::MenuItem("Warning", NULL, minLevel & logSeverity_Warn)) {
      minLevel = logSeverity_Warn;
      minLevel |= (logSeverity_Warn - 1);
    }
    if (ImGui::MenuItem("Info", NULL, minLevel & logSeverity_Info)) {
      minLevel = logSeverity_Info;
      minLevel |= (logSeverity_Info - 1);
    }
    if (ImGui::MenuItem("Debug", NULL, minLevel & logSeverity_Debug)) {
      minLevel = logSeverity_Debug;
      minLevel |= (logSeverity_Debug - 1);
    }
    Logs::get().setFilter(minLevel);
    ImGui::EndMenu();
  }
  static bool timestamps=true;
  if (ImGui::BeginMenu("Options")) {
    if (ImGui::MenuItem("Show Timestamp", NULL, timestamps)) {
      timestamps = !timestamps;
    }
    ImGui::EndMenu();
  }
  ImGui::EndMenuBar();
  ImGui::BeginChild("Log");
  bool scrollToEnd = false;
  if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
    scrollToEnd = true;
  }
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
  Logs& logs = Logs::get();
  ImGuiListClipper clipper;
  const int clipperItemCount=logs.length();
  clipper.Begin(clipperItemCount);
  while (clipper.Step()) {
    for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
      const LogEntry entry = logs.getline(i);
      ImVec4 color;
      switch (entry.severity) {
        case logSeverity_Err:
          color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
          break;
        case logSeverity_Warn:
          color = ImVec4(0.9f, 0.9f, 0.4f, 1.0f);
          break;
        case logSeverity_Info:
          color = ImVec4(0.6f, 0.9f, 0.6f, 1.0f);
          break;
        case logSeverity_Debug:
          color = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
          break;
        default:
          color = ImVec4(1.0f, 0.0f, 0.4f, 1.0f);
      }

      ImGui::PushStyleColor(ImGuiCol_Text, color);
      if(timestamps) {
        ImGui::Text("%7zu %s: %s ", entry.timestamp, entry.emitter, entry.message.c_str());
      } else {
        ImGui::Text("%s: %s ", entry.emitter, entry.message.c_str());
      }
      ImGui::PopStyleColor();
    }
  }
  clipper.End();
  ImGui::PopStyleVar();
  if (scrollToEnd) {
    ImGui::SetScrollHereY(1.0f);
  }
  ImGui::EndChild();
  ImGui::End();
  //ImGui::PopStyleColor(2);
  return 0;
}
