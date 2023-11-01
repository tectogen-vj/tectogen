#pragma once

#include "displaypayload.h"
#include "nodeprogramapi.h"

struct NodeProgramMetadata {
  enum class Type {
    None,
    Runnable,
    PixelShader,
    CoordShader,
    Display,
    Source
  };
  Type type=Type::None;
  union extra {
    DisplayPayload displayPayload;
  };
};

struct NodeProgramType {
  tn_Descriptor* const desc;
  NodeProgramMetadata* const meta;
  NodeProgramType(tn_Descriptor* const desc, NodeProgramMetadata* const meta) :
    desc(desc), meta(meta) {}
};
