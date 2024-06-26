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


void invoke(tn_State* state, unsigned long idx);

}
