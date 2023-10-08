#pragma once

#include "context.h"
#include "shader.h"
#include "shaderprogram.h"

#include <GL/glew.h>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

class ShaderManager
{
private:
  std::shared_ptr<ShaderProgram> currentProgram;
  GLuint nextProgram;
  Context context;
  std::thread shaderReloadThread;
  std::mutex shaderChangeMutex;
  std::condition_variable shaderChangeCV;
  bool terminate=false;
public:
  std::map<std::string, FragmentShader> shaderfiles;
  ShaderManager();
  void start(std::string path);
  void shutdown();
  void link();
  std::filesystem::path path;
  void reloadFile(std::string file);
  void reloadFile(std::filesystem::directory_entry file);
  std::shared_ptr<ShaderProgram> getProgram();
  Shader vertex_shader;
};
