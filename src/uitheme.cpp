#include "uitheme.h"
#include "imgui.h"
#include "imgui_internal.h" //for ImLerp
#include "imnodes.h"

void UITheme::set()
{
  // Tectogen colors
  ClearColor=ImVec4(0.13f, 0.08f, 0.12f, 1.00f); // #20141e

  // ImGui colors
  ImGuiStyle& imGuiStyle = ImGui::GetStyle();
  ImVec4* imGuiColors=imGuiStyle.Colors;
  imGuiColors[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
  imGuiColors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
  imGuiColors[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
  imGuiColors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  imGuiColors[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
  imGuiColors[ImGuiCol_Border]                 = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
  imGuiColors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  imGuiColors[ImGuiCol_FrameBg]                = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
  imGuiColors[ImGuiCol_FrameBgHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
  imGuiColors[ImGuiCol_FrameBgActive]          = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
  imGuiColors[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
  imGuiColors[ImGuiCol_TitleBgActive]          = ImColor::HSV(0.71f, 0.32f, 0.32f);
  imGuiColors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
  imGuiColors[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
  imGuiColors[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
  imGuiColors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
  imGuiColors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
  imGuiColors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
  imGuiColors[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  imGuiColors[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
  imGuiColors[ImGuiCol_SliderGrabActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  imGuiColors[ImGuiCol_Button]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
  imGuiColors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  imGuiColors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
  imGuiColors[ImGuiCol_Header]                 = ImColor::HSV(0.71f, 0.44f, 0.44f);
  imGuiColors[ImGuiCol_HeaderHovered]          = ImColor::HSV(0.82f, 0.28f, 0.66f);
  imGuiColors[ImGuiCol_HeaderActive]           = ImColor::HSV(0.71f, 0.28f, 0.66f);
  imGuiColors[ImGuiCol_Separator]              = imGuiColors[ImGuiCol_Border];
  imGuiColors[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
  imGuiColors[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
  imGuiColors[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
  imGuiColors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
  imGuiColors[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
  imGuiColors[ImGuiCol_Tab]                    = ImLerp(imGuiColors[ImGuiCol_Header],       imGuiColors[ImGuiCol_TitleBgActive], 0.80f);
  imGuiColors[ImGuiCol_TabHovered]             = imGuiColors[ImGuiCol_HeaderHovered];
  imGuiColors[ImGuiCol_TabActive]              = ImLerp(imGuiColors[ImGuiCol_HeaderActive], imGuiColors[ImGuiCol_TitleBgActive], 0.60f);
  imGuiColors[ImGuiCol_TabUnfocused]           = ImLerp(imGuiColors[ImGuiCol_Tab],          imGuiColors[ImGuiCol_TitleBg], 0.80f);
  imGuiColors[ImGuiCol_TabUnfocusedActive]     = ImLerp(imGuiColors[ImGuiCol_TabActive],    imGuiColors[ImGuiCol_TitleBg], 0.40f);
  imGuiColors[ImGuiCol_DockingPreview]         = imGuiColors[ImGuiCol_HeaderActive] * ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
  imGuiColors[ImGuiCol_DockingEmptyBg]         = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
  imGuiColors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
  imGuiColors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
  imGuiColors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
  imGuiColors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
  imGuiColors[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
  imGuiColors[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);   // Prefer using Alpha=1.0 here
  imGuiColors[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);   // Prefer using Alpha=1.0 here
  imGuiColors[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
  imGuiColors[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
  imGuiColors[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
  imGuiColors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
  imGuiColors[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
  imGuiColors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
  imGuiColors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
  imGuiColors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);

  // ImNodes colors
  ImNodesStyle& imNodesStyle = ImNodes::GetStyle();
  unsigned int* imNodesColors=imNodesStyle.Colors;
  imNodesColors[ImNodesCol_NodeBackground] = IM_COL32(50, 50, 50, 255);
  imNodesColors[ImNodesCol_NodeBackgroundHovered] = IM_COL32(75, 75, 75, 255);
  imNodesColors[ImNodesCol_NodeBackgroundSelected] = IM_COL32(75, 75, 75, 255);
  imNodesColors[ImNodesCol_NodeOutline] = IM_COL32(100, 100, 100, 255);
  // title bar colors match ImGui's titlebg colors
  imNodesColors[ImNodesCol_TitleBar] = ImColor(imGuiColors[ImGuiCol_Header]);
  imNodesColors[ImNodesCol_TitleBarHovered] = ImColor(imGuiColors[ImGuiCol_HeaderHovered]);
  imNodesColors[ImNodesCol_TitleBarSelected] = ImColor(imGuiColors[ImGuiCol_HeaderActive]);
  // link colors match ImGui's slider grab colors
  imNodesColors[ImNodesCol_Link] = IM_COL32(61, 133, 224, 200);
  imNodesColors[ImNodesCol_LinkHovered] = IM_COL32(66, 150, 250, 255);
  imNodesColors[ImNodesCol_LinkSelected] = IM_COL32(66, 150, 250, 255);
  // pin colors match ImGui's button colors
  imNodesColors[ImNodesCol_Pin] = IM_COL32(53, 150, 250, 180);
  imNodesColors[ImNodesCol_PinHovered] = IM_COL32(53, 150, 250, 255);

  imNodesColors[ImNodesCol_BoxSelector] = IM_COL32(61, 133, 224, 30);
  imNodesColors[ImNodesCol_BoxSelectorOutline] = IM_COL32(61, 133, 224, 150);

  imNodesColors[ImNodesCol_GridBackground] = IM_COL32(40, 40, 50, 200);
  imNodesColors[ImNodesCol_GridLine] = IM_COL32(200, 200, 200, 40);
  imNodesColors[ImNodesCol_GridLinePrimary] = IM_COL32(240, 240, 240, 60);

  // minimap colors
  imNodesColors[ImNodesCol_MiniMapBackground] = IM_COL32(25, 25, 25, 150);
  imNodesColors[ImNodesCol_MiniMapBackgroundHovered] = IM_COL32(25, 25, 25, 200);
  imNodesColors[ImNodesCol_MiniMapOutline] = IM_COL32(150, 150, 150, 100);
  imNodesColors[ImNodesCol_MiniMapOutlineHovered] = IM_COL32(150, 150, 150, 200);
  imNodesColors[ImNodesCol_MiniMapNodeBackground] = IM_COL32(200, 200, 200, 100);
  imNodesColors[ImNodesCol_MiniMapNodeBackgroundHovered] = IM_COL32(200, 200, 200, 255);
  imNodesColors[ImNodesCol_MiniMapNodeBackgroundSelected] =
      imNodesColors[ImNodesCol_MiniMapNodeBackgroundHovered];
  imNodesColors[ImNodesCol_MiniMapNodeOutline] = IM_COL32(200, 200, 200, 100);
  imNodesColors[ImNodesCol_MiniMapLink] = imNodesColors[ImNodesCol_Link];
  imNodesColors[ImNodesCol_MiniMapLinkSelected] = imNodesColors[ImNodesCol_LinkSelected];
  imNodesColors[ImNodesCol_MiniMapCanvas] = IM_COL32(200, 200, 200, 25);
  imNodesColors[ImNodesCol_MiniMapCanvasOutline] = IM_COL32(200, 200, 200, 200);
}
