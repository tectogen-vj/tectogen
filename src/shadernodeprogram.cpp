#include "shadernodeprogram.h"

void ShaderNodeProgram::invoke(NodeProgramHandle instance, NodeProgramState *state) {
  ShaderNodeProgram::Userdata* userdata=(ShaderNodeProgram::Userdata*)state->userdata;
  const NodeProgramDescriptor* selfDesc=state->descriptor;

  for(int i=0; i<selfDesc->portCount; i++) {
    if(selfDesc->portDescriptors[i].role==NodeProgramPortRoleInput) {
      if(selfDesc->portDescriptors[i].type==NodeProgramPortTypeScalar) {
        double** payload=(double**)(state->portState[i].payload);
        if(payload && *payload) {
          GLfloat v=(float) **payload;
          if(userdata) {
            for(auto* display:userdata->targetDisplays) {
              // FIXME PERFORMANCE move this to when shader is linked
              const char* name=selfDesc->portDescriptors[i].name;
              GLint loc=glGetUniformLocation(display->inv.program, name);
              glUniform1f(loc,v);
            }
          }
        }
      }
    }
  }
}
