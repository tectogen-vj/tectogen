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
  NodeProgramDescriptor* const desc;
  NodeProgramMetadata* const meta;
  NodeProgramType(NodeProgramDescriptor* const desc, NodeProgramMetadata* const meta) :
    desc(desc), meta(meta) {}
};
