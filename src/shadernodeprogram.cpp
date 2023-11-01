#include "shadernodeprogram.h"

void ShaderNodeProgram::invoke(tn_Userdata instance, tn_State *state) {
  ShaderNodeProgram::Userdata* userdata=(ShaderNodeProgram::Userdata*)state->userdata;
  const tn_Descriptor* selfDesc=state->descriptor;

  for(int i=0; i<selfDesc->portCount; i++) {
    if(selfDesc->portDescriptors[i].role==tn_PortRoleInput) {
      if(selfDesc->portDescriptors[i].type==tn_PortTypeScalar) {
        double** payload=(double**)(state->portState[i].payload_symbol_to_be_obsoleted);
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
