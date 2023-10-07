#pragma once

#include "asynctex.h"
#include "shaderinvocation.h"

class DisplayPayload {
public:
  DisplayPayload():renderTex(160,120){}
  AsyncTex renderTex;
  ShaderInvocation inv;
  inline int getWidth() {return 160;}
  inline int getHeight() {return 120;}
};
