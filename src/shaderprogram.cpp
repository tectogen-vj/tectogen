#include "logs.h"
#include "shaderprogram.h"

ShaderProgram::ShaderProgram(GLuint program): program(program) {

}

ShaderProgram::~ShaderProgram() {
  if(program) {
    glDeleteProgram(program);

  }
  Logs::get().log(logSeverity_Debug, "Shader", "Shader program deleted");
}
