#include "shadermanager.h"

#include "logs.h"
#include "app.h"

#define DMON_IMPL
#include "dmon.h"

#include <fstream>
#include <iterator>
#include <utility>

ShaderManager::ShaderManager(): vertex_shader(GL_VERTEX_SHADER, Shader::vertex_shader_text) {
}

int ShaderManager::init()
{
  context.createBackground();
  dmon_init();
  this->shaderReloadThread=std::thread([this](){
    context.makeCurrent();
    // TODO: fully move vertex shader into ShaderManager?
    vertex_shader.compile_shader();
    std::unique_lock lock(shaderChangeMutex);
    while(!terminate) {
      link();
      shaderChangeCV.wait(lock);
    }
  });
  return 0;
}

void ShaderManager::watch(std::string path) {
  if(!this->path.empty()) {
      Logs::get().log(logSeverity_Warn, "Shader", "Tried to initialize shadermanager twice, ignoring");
      return;
    }
  this->path=path;
  Logs::get().logf(logSeverity_Info, "Shader", "Enumerating shaders in %s", path.c_str());
  try {
    for (const auto& file : std::filesystem::directory_iterator(path)) {
      reloadFile(file);
    }
  } catch(std::filesystem::filesystem_error err) {
    Logs::get().logf(logSeverity_Err, "Shader", "Could not open directory '%s'", path.c_str());
    return;
  }
  dmon_watch(path.c_str(), [](dmon_watch_id watch_id, dmon_action action, const char* rootdir,
             const char* filepath, const char* oldfilepath, void* user){
      ShaderManager* t=(ShaderManager*)user;
      Logs::get().logf(logSeverity_Info, "Shader", "file changed %s | %s",filepath, oldfilepath);
      if(filepath) {
          // HACK
          t->reloadFile(t->path / filepath);
        }
      if(oldfilepath) {
          // HACK
          t->reloadFile(t->path / oldfilepath);
        }
    }
  , DMON_WATCHFLAGS_RECURSIVE, this);
  shaderChangeCV.notify_one();
}

void ShaderManager::shutdown() {
  if(_dmon_init) {
    dmon_deinit();
  }
  shaderfiles.clear();
  terminate=true;
  shaderChangeCV.notify_one();
  this->shaderReloadThread.join();
}

void ShaderManager::link() {
  bool changed=false;
  for (auto& pair : shaderfiles) {
    Shader& shader=pair.second.shader;
    if(shader==0) {
      shader.compile_shader();
      pair.second.parse();
      App::get().nodemanager.library.addFragmentShader(&pair.second); // FIXME: THIS HAS POTENTIAL FOR RACE CONDITIONS

      Logs::get().logf(logSeverity_Debug, "Shader", "Shader compiled %s", pair.first.c_str());
      // TODO: CHeck for error
      changed=true;
    }
  }
}

void ShaderManager::reloadFile(const std::string file) {
  try {
    reloadFile(std::filesystem::directory_entry(file));
  } catch (...) {
    Logs::get().logf(logSeverity_Err,"Shader", "Failed to reload %s", file.c_str());
  }
}

void ShaderManager::reloadFile(const std::filesystem::directory_entry file) {
  const std::string filename=file.path().filename().string();
  if (file.is_regular_file()) {
    // TODO: error handling
    shaderfiles.erase(filename);
    Logs::get().logf(logSeverity_Info, "Shader", "Shader found %s", filename.c_str());
    shaderfiles.emplace(filename,FragmentShader(file));
    shaderChangeCV.notify_one();
  } else {
    if(shaderfiles.erase(filename))
    {
      Logs::get().logf(logSeverity_Info, "Shader", "Shader REMOVED %s", filename.c_str());
      shaderChangeCV.notify_one();
    } else {
      Logs::get().logf(logSeverity_Info, "Shader", "Shader NOT compiled %s", filename.c_str());
    }
  }
}

