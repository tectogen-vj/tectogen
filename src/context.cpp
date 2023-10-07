#include "context.h"

#include "logs.h"

#include <GL/glew.h>
#include <memory>

void glfw_error_callback(int error, const char* description) {
  Logs::get().logf(logSeverity_Err, "GLFW", "Error %i: %s", error, description);
}

void APIENTRY glDebugOutput(GLenum source,
                            GLenum type,
                            unsigned int id,
                            GLenum severity,
                            GLsizei length,
                            const char *message,
                            const void *userParam)
{
  // ignore non-significant error/warning codes
  if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

  const char* errorsource;
  const char* errortype;
  const char* errorseverity;

  switch (source)
  {
    case GL_DEBUG_SOURCE_API:             errorsource = "API"; break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   errorsource = "Window System"; break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: errorsource = "Shader Compiler"; break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:     errorsource = "Third Party"; break;
    case GL_DEBUG_SOURCE_APPLICATION:     errorsource = "Application"; break;
    case GL_DEBUG_SOURCE_OTHER:           errorsource = "Other"; break;
    default:                              errorsource = "Unknown"; break;
  }

  switch (type)
  {
    case GL_DEBUG_TYPE_ERROR:               errortype = "Error"; break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: errortype = "Deprecated Behaviour"; break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  errortype = "Undefined Behaviour"; break;
    case GL_DEBUG_TYPE_PORTABILITY:         errortype = "Portability"; break;
    case GL_DEBUG_TYPE_PERFORMANCE:         errortype = "Performance"; break;
    case GL_DEBUG_TYPE_MARKER:              errortype = "Marker"; break;
    case GL_DEBUG_TYPE_PUSH_GROUP:          errortype = "Push Group"; break;
    case GL_DEBUG_TYPE_POP_GROUP:           errortype = "Pop Group"; break;
    case GL_DEBUG_TYPE_OTHER:               errortype = "Other"; break;
    default:                                errortype = "Unknown"; break;
  }

  switch (severity)
  {
    case GL_DEBUG_SEVERITY_HIGH:         errorseverity = "high"; break;
    case GL_DEBUG_SEVERITY_MEDIUM:       errorseverity = "medium"; break;
    case GL_DEBUG_SEVERITY_LOW:          errorseverity = "low"; break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: errorseverity = "notification"; break;
    default:                             errorseverity = "Unknown"; break;
  }

  Logs::get().logf(logSeverity_Err, "GL", "%s, %s: %s", errortype, errorsource, message);

  //raise(SIGTRAP);
}

Context::Context() {}

int Context::createBackground() {
  return create("", true);
}
int Context::create(const char* title, bool background) {
  // HACK
  static GLFWwindow* primarywindow=nullptr;
  if(window) {
    return 1;
  }
  glfwSetErrorCallback(glfw_error_callback);
  if (!glfwInit()) {
    return 1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);  // TODO, HACK
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  if(background) {
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  } else {
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
  }
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

          // Create window with graphics context
  window = glfwCreateWindow(1280, 720, title, NULL, primarywindow);
  if (window == NULL) {
    Logs::get().log(logSeverity_Info, "GLFW", "Trying fallback OpenGL");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 1);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    window = glfwCreateWindow(1280, 720, title, NULL, primarywindow);
  }
  if (window == NULL)
    return 1;
  if(!primarywindow) {
    primarywindow=window;
  }
  return 0;
}

int Context::init() {
  makeCurrent();

  if (glewInit() != GLEW_OK) {
    Logs::get().log(logSeverity_Err, "GL", "Failed to initialize GLEW OpenGL loader!");
    return 1;
  } else {
    Logs::get().logf(logSeverity_Info, "GL", "Successfully Initialized OpenGL: %s", glGetString(GL_VERSION));
  }

  int flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
  if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
  {
    Logs::get().log(logSeverity_Info, "GL", "OpenGL Debug Context enabled");
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(glDebugOutput, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
  }

#if not defined(IMGUI_IMPL_OPENGL_ES2) and not defined(IMGUI_IMPL_OPENGL_ES3)
  {
    glGetIntegerv(GL_MAJOR_VERSION, &ogl_major);
    glGetIntegerv(GL_MINOR_VERSION, &ogl_minor);
    if (ogl_major == 2) {
      glsl_version = "#version 120";
    } else if (ogl_major == 3 || (ogl_major == 4 && ogl_minor == 0)) {
      glsl_version = "#version 130";
    } else if (ogl_major == 4) {
      glsl_version = "#version 410 core";
    }
  }
#endif
  return 0;
}

void Context::makeCurrent() {

  glfwMakeContextCurrent(window);
}
