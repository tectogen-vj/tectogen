#include "uicomponent.h"

#include "app.h"
#include "config.h"
#include "instreammanager.h"
#include "logs.h"
#include "ringbuffer.h"
#include "shader.h"
#include "shadermanager.h"

#include "imconfig.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include "imgui_impl_opengl3.h"
#include "imgui_internal.h" // for DockBuilderDockWindow etc.

#include <sago/platform_folders.h>

#include <GLFW/glfw3.h>
#include <cstdlib>
#include <memory>

using namespace Shaderaudio;

UIComponent::UIComponent() {

}

int UIComponent::init() {

  context.create(App::get().title);
  context.init();
  context.makeCurrent();


  glfwMaximizeWindow(context.window);
  glfwSwapInterval(1);  // Enable vsync

  IMGUI_CHECKVERSION();
  ImGuiContext& imContext = *ImGui::CreateContext();


  Config::setHandler(imContext);

  ImGuiIO& io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
  //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
  //io.ConfigViewportsNoAutoMerge = true;
  //io.ConfigViewportsNoTaskBarIcon = true;

  std::string configDir=sago::getConfigHome()+"/tectogen";

  std::filesystem::path configPath(configDir);

  if (!std::filesystem::exists(configPath)) {
      try {
        std::filesystem::create_directories(configPath);
        Logs::get().logf(logSeverity_Debug, "UI", "Created config dir %s", configPath.c_str());
      } catch (const std::filesystem::filesystem_error& e) {
        Logs::get().logf(logSeverity_Err, "UI", "Failed to create config dir %s", configPath.c_str());
      }
  }

  imGuiINIPath=configDir+"/imgui.ini";
  io.IniFilename=imGuiINIPath.c_str();

  if (imContext.IO.IniFilename)
    ImGui::LoadIniSettingsFromDisk(imContext.IO.IniFilename);

  initialRun = !imContext.SettingsLoaded;

  ImGui::StyleColorsDark();


  ImGui_ImplGlfw_InitForOpenGL(context.window, true);

  if(context.ogl_major==0 || context.ogl_major==2) {
    context.ogl_major=2;
    ImGui_ImplOpenGL2_Init();
  } else {
    ImGui_ImplOpenGL3_Init(context.glsl_version);
  }


  //TODO: Remove from this component
  GLuint vertex_array, vertex_buffer;
  glGenVertexArrays(1, &vertex_array);
  glBindVertexArray(vertex_array);
  glGenBuffers(1, &vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(Shader::screenPlaneVertices), Shader::screenPlaneVertices, GL_STATIC_DRAW);


  audioConfigWindow.init();
  nodeWindow.init();
  monitoringWindow.init();

  return 0;
}

int UIComponent::tick() {
  if(glfwWindowShouldClose(context.window)) {
    this->shouldClose=true;
  }
  glfwPollEvents();
#ifdef USE_JET_LIVE
  jetTools.update();
#endif

  context.ogl_major==2 ? ImGui_ImplOpenGL2_NewFrame() : ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  ImGui::PushStyleColor(ImGuiCol_Separator,ImGui::GetStyleColorVec4(ImGuiCol_TitleBg));
  static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

  ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
  ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->Pos);
  ImGui::SetNextWindowSize(viewport->Size);
  ImGui::SetNextWindowViewport(viewport->ID);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  window_flags |=
      ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
  window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

  if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
    window_flags |= ImGuiWindowFlags_NoBackground;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("DockSpace", nullptr, window_flags);
  ImGui::PopStyleVar();
  ImGui::PopStyleVar(2);

  ImGuiIO& io = ImGui::GetIO();
  if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0,0,0,0));
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    ImGui::PopStyleColor();
    ImGuiContext& context = *ImGui::GetCurrentContext();
    /* INITIAL DOCKSPACE */
    if (initialRun) {
      Logs::get().log(logSeverity_Info, "UI", "Dockspace unconfigured, setting up initial configuration");
      initialRun = false;

      ImGui::DockBuilderRemoveNode(dockspace_id);
      ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
      ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
      ImGuiID dock_id_prop = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.2f, NULL, &dockspace_id);
      ImGuiID dock_id_prop2 = ImGui::DockBuilderSplitNode(dock_id_prop, ImGuiDir_Up, 0.3f, NULL, &dock_id_prop);

      ImGui::DockBuilderDockWindow("node editor", dockspace_id);
      ImGui::DockBuilderDockWindow("Logs", dock_id_prop);
      ImGui::DockBuilderDockWindow("Monitoring", dock_id_prop);
      ImGui::DockBuilderDockWindow("Audio Configuration", dock_id_prop2);
      ImGui::DockBuilderFinish(dockspace_id);
    }

  }
  ImGui::End();

  if(ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Quit")) {
        exit(0);
      }
      if (ImGui::MenuItem("ImGui Stack Tool", NULL, &show_app_stack_tool)) {

      }
#ifdef USE_JET_LIVE
      if (ImGui::MenuItem("hotload")) {
        jetTools.reload();
      }
#endif
      ImGui::EndMenu();
    }
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImVec4(0.6f, 0.9f, 0.6f, 1.0f)));
    ImGui::Text("ui-fps:%3i\n",(int)ImGui::GetIO().Framerate);

    ImGui::PopStyleColor();
    ImGui::EndMainMenuBar();
  }
  if(show_app_stack_tool) {
    ImGui::ShowStackToolWindow(&show_app_stack_tool);
  }
  console.show();
  audioConfigWindow.show();
  nodeWindow.show();
  monitoringWindow.show();
  ImGui::PopStyleColor();
  ImGui::Render();
  int display_w, display_h;
  glfwGetFramebufferSize(context.window, &display_w, &display_h);
  glViewport(0, 0, display_w, display_h);

  glClearColor(0.2f, 0.2f, 0.2f, 1);

  glClear(GL_COLOR_BUFFER_BIT);

  context.ogl_major==2 ? ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData()) : ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
  }
  glfwSwapBuffers(context.window);
  return 0;
}
int UIComponent::shutdown() {
  ImGui::Shutdown();
  return 0;
}
