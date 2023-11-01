#pragma once

#include "displaypayload.h"
#include "nodeprogramapi.h"

#include <string>
#include <vector>

namespace ShaderNodeProgram {

class Userdata {
public:
  std::vector<DisplayPayload*> targetDisplays;

  // FIXME TODO unused
  std::vector<GLint> uniformLocations;
};


void invoke(tn_Userdata instance, tn_State* state);

}
