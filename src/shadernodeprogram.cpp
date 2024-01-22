#include "shadernodeprogram.h"

void ShaderNodeProgram::invoke(tn_State *state, unsigned long idx) {
  ShaderNodeProgram::Userdata* userdata=(ShaderNodeProgram::Userdata*)state->userdata;
  const tn_Descriptor* selfDesc=state->descriptor;

  for(int i=0; i<selfDesc->portCount; i++) {
    if(selfDesc->portDescriptors[i].role==tn_PortRoleInput) {
      if(selfDesc->portDescriptors[i].type==tn_PortTypeScalar) {
        auto port=state->portState[i];
        if(port.portData.ring_buffer) {
          double* payload=tn_getPM(port, idx).scalar.v;
          GLfloat v=(float) *payload;
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
