#pragma once

#include "nodeprogramapi.h"
#include "nodeprogrammetadata.h"
#include "shader.h"

#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class NodeProgramLibrary
{
private:
  struct Entry {
    std::unique_ptr<tn_Descriptor> programDescriptor;
    std::unique_ptr<NodeProgramMetadata> metadata;
    NodeProgramType programType;
    std::vector<bool> hasInputType;
    std::unique_ptr<tn_PortDescriptor[]> portDescriptors;
    Entry(std::vector<bool>::size_type maxKnownInputType):
      programDescriptor(std::make_unique<tn_Descriptor>()),
      metadata(std::make_unique<NodeProgramMetadata>()),
      hasInputType(std::vector<bool>(maxKnownInputType+1, false)),
      programType(programDescriptor.get(), metadata.get())
    {

    }
  };
  std::vector<Entry> lib;
  std::vector<bool>::size_type maxKnownInputType;
  void reg(Entry& newEntry);
public:
  NodeProgramLibrary();
  NodeProgramType addProgramType(const char* identifier,
    std::initializer_list<tn_PortDescriptor> portDescriptors,
                                 tn_invoke_function invokeFunction,
                                 tn_instantiate_function instantiateFunction = nullptr);
  void addFragmentShader(FragmentShader* frag);
  std::vector<NodeProgramType> getProgramsWithInputType(tn_PortType type);
  std::vector<NodeProgramType> getPrograms();
  std::optional<NodeProgramType> getProgramByName(const char* name);

};
