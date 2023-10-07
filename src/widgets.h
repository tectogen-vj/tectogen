#pragma once

#include "imconfig.h" // NOLINT
#include "imgui.h"

#include <cfloat>
#include <cstddef>

namespace ImGui {
//const char* overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size
    template<typename T> void PlotSpectrum(const char* label, T* data, int nbins, int samplerate, const char* overlay_text = NULL, float scale_min=0, float scale_max = FLT_MAX, ImVec2 frame_size = ImVec2(0, 0));
}
