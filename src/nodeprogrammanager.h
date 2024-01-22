#pragma once

#include "nodegraph.h"
#include "nodeprogramapi.h"
#include "nodeprograminstancewrapper.h"
#include "nodeprogramlibrary.h"

#include <atomic>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

class InvocationList {
public:
  std::vector<NodeProgramInstanceWrapper> list;
  std::vector<NodeProgramInstanceWrapper> displays;
  std::vector<NodeProgramInstanceWrapper> parametrizedFragments; // not part of "list" because these nodes need to wait for the video thread to finish as they set the uniform values
};

class NodeProgramManager {
public:
  NodeProgramLibrary library;
  NodeGraph nodegraph;
  std::atomic<InvocationList*> newInvocationList;
  std::atomic<InvocationList*> invocationList;
  //std::vector<std::unique_ptr<tn_Descriptor>> nodeTypes;
  std::optional<NodeProgramType> audioInType;
  std::optional<NodeProgramType> displayType;
  void loadTypes();
  bool buildInvocationList();
  const char* plotIdentifier="Plot"; // HACK: identifying non-runnable nodes by identifier? Why not!
};
