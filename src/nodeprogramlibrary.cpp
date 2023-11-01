#include "nodeprogramlibrary.h"

#include "logs.h"
#include "shadernodeprogram.h"

#include <algorithm>
#include <cstring>
#include <optional>
#include <utility>

void NodeProgramLibrary::reg(Entry& newEntry) {
  auto portCount=newEntry.programDescriptor->portCount;
  const tn_PortDescriptor* portDescriptors=newEntry.programDescriptor->portDescriptors;
  for (int i=0; i<portCount; i++) {
    auto& portDesc=portDescriptors[i];
    if(portDesc.role == tn_PortRoleInput) {
      if (portDesc.type>maxKnownInputType) {
        maxKnownInputType=portDesc.type;
        newEntry.hasInputType.resize(maxKnownInputType+1, false);
      }
      newEntry.hasInputType[portDesc.type]=true;
    }
  }

  lib.push_back(std::move(newEntry));
}

NodeProgramLibrary::NodeProgramLibrary() : maxKnownInputType(0) {
}

NodeProgramType NodeProgramLibrary::addProgramType(const char* identifier,
    std::initializer_list<tn_PortDescriptor> portDescriptors,
    void (*invokeFunction)(tn_Handle handle, tn_State* state),
    tn_Handle (*instantiateFunction)(const tn_Descriptor* descriptor))
{
  Entry newEntry(maxKnownInputType);

  newEntry.portDescriptors = std::make_unique<tn_PortDescriptor[]>(portDescriptors.size());
  std::copy(portDescriptors.begin(), portDescriptors.end(), newEntry.portDescriptors.get());
  newEntry.programDescriptor->portDescriptors = newEntry.portDescriptors.get();
  newEntry.programDescriptor->portCount = static_cast<unsigned int>(portDescriptors.size());

  newEntry.programDescriptor->identifier = identifier;

  if(invokeFunction) {
    newEntry.metadata->type=NodeProgramMetadata::Type::Runnable;
  } else if (newEntry.programDescriptor->portCount==1
             && newEntry.programDescriptor->portDescriptors[0].type==tn_PortTypeShader
             && newEntry.programDescriptor->portDescriptors[0].role==tn_PortRoleInput) {
    newEntry.metadata->type=NodeProgramMetadata::Type::Display;
  }

  assert(newEntry.metadata->type!=NodeProgramMetadata::Type::None);

  newEntry.programDescriptor->invoke = invokeFunction;
  newEntry.programDescriptor->instantiate = instantiateFunction;

  reg(newEntry);
  return lib.back().programType;

}

void NodeProgramLibrary::addFragmentShader(FragmentShader *frag) {
  NodeProgramMetadata::Type type=NodeProgramMetadata::Type::None;
  if(frag->isCoordShader()) {
    type=NodeProgramMetadata::Type::CoordShader;
  } else if(frag->isPixelShader()) {
    type=NodeProgramMetadata::Type::PixelShader;
  } else {
    Logs::get().logf(logSeverity_Warn, "ProgramLibrary", "Shaderfile %s not registered: No matching signature detected", frag->filename.c_str());
    return;
  }
  Entry newEntry(maxKnownInputType);
  newEntry.metadata->type=type;

  size_t portCount;

  if(type==NodeProgramMetadata::Type::CoordShader) {
    // "Shader" in, "Shader" out plus the uniforms, that is the number of ports on CoordShader
    portCount=2+frag->uniforms.size();
    newEntry.portDescriptors = std::make_unique<tn_PortDescriptor[]>(portCount);
    tn_PortDescriptor* descriptors=newEntry.portDescriptors.get();
    newEntry.programDescriptor->portDescriptors = descriptors;
    for(int i=0; i<portCount; i++) {
      tn_PortDescriptor& portDesc=descriptors[i];
      // Parameters first, uniforms second
      if(i<2) {
        portDesc.name=frag->parameters[i].v.c_str();
        if(frag->parameters[i].s==FragmentShader::Param::Storage::in) {
          portDesc.role=tn_PortRoleInput;
        } else {
          portDesc.role=tn_PortRoleOutput;
        }
        portDesc.type=tn_PortTypeShader;
      } else {
        portDesc.name=frag->uniforms[i-2].c_str();
        portDesc.role=tn_PortRoleInput;
        portDesc.type=tn_PortTypeScalar;

      }
    }
  } else if(type==NodeProgramMetadata::Type::PixelShader) {
    // all params except the optional vec2 input plus the uniforms,
    // that is the number of ports on CoordShader
    int vec2InCount=std::count_if(frag->parameters.begin(), frag->parameters.end(), [](FragmentShader::Param& p) {
      return p.s==FragmentShader::Param::Storage::in && p.t==FragmentShader::Param::Type::vec2;
    });

    int paramCount=frag->parameters.size()-vec2InCount;
    portCount=paramCount+frag->uniforms.size();
    newEntry.portDescriptors = std::make_unique<tn_PortDescriptor[]>(portCount);
    tn_PortDescriptor* descriptors=newEntry.portDescriptors.get();
    newEntry.programDescriptor->portDescriptors = descriptors;
    int i=0;
    for(auto it=frag->parameters.begin(); it!=frag->parameters.end(); it++) {
      // don't list vec2 as ports!
      if(it->t==FragmentShader::Param::Type::vec2) {
        continue;
      }
      tn_PortDescriptor& portDesc=descriptors[i];
      portDesc.name=it->v.c_str();
      if(it->s==FragmentShader::Param::Storage::in) {
        portDesc.role=tn_PortRoleInput;
      } else {
        portDesc.role=tn_PortRoleOutput;
      }
      portDesc.type=tn_PortTypeShader;
      i++;
    }
    for(auto it=frag->uniforms.begin(); it!=frag->uniforms.end(); it++) {
      tn_PortDescriptor& portDesc=descriptors[i];
      portDesc.name=it->c_str();
      portDesc.role=tn_PortRoleInput;
      portDesc.type=tn_PortTypeScalar;

      i++;
    }
    assert(i==portCount);
  }

  newEntry.programDescriptor->identifier=frag->filename.c_str();
  newEntry.programDescriptor->portCount = static_cast<unsigned int>(portCount);

  newEntry.programDescriptor->invoke = ShaderNodeProgram::invoke;
  newEntry.programDescriptor->instantiate = nullptr;

  reg(newEntry);
}

std::vector<NodeProgramType> NodeProgramLibrary::getProgramsWithInputType(tn_PortType type) {
  std::vector<NodeProgramType> programs;

  for (auto& entry : lib) {
    size_t idx=static_cast<size_t>(type);
    if (idx < entry.hasInputType.size() && entry.hasInputType[idx]) {
      programs.push_back(entry.programType);
    }
  }

  return programs;
}

std::vector<NodeProgramType> NodeProgramLibrary::getPrograms() {
  std::vector<NodeProgramType> programs;
  programs.reserve(lib.size());
  for (auto& entry : lib) {
    programs.push_back(entry.programType);
  }

  return programs;
}

std::optional<NodeProgramType> NodeProgramLibrary::getProgramByName(const char *name) {
  for(auto& entry : lib) {
    if(strcmp(name, entry.programDescriptor->identifier) == 0) {
      return entry.programType;
    }
  }
  return std::nullopt;
}
