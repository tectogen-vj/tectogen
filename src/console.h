#pragma once

#include "imgui.h"

class Console {
public:
  ImGuiCol errCol, warnCol, infoCol, debugCol, defaultCol;
  int show();
  void colorsDark();
  void colorsLight();
};
