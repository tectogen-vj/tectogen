/* The purpose of this graph is to represent a logical arrangement of node
 * programs for display and modification within the ui. It is there for
 * quick and convenient use in the ui.
 * It references node Programs using shared pointers. Different data structures
 * for excecute of the node programs can be built from this and keep shared
 * pointers as well to prevent use after free.
 */

#pragma once

#include "nodeprogramapi.h"
#include "nodeprograminstancewrapper.h"
#include "nodeprogrammetadata.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

typedef int nodeid;

struct NodeInput;
struct NodeOutput;

class Node {
private:
  //const NodeProgramDescriptor* program;
  const NodeProgramType program;
  std::optional<NodeProgramInstanceWrapper> instance;
public:
  const nodeid id;
  Node(nodeid id, const NodeProgramType program):id(id), program(program) {};
  std::vector<NodeInput*> in;
  std::vector<NodeOutput*> out;
  inline const NodeProgramDescriptor* getProgramDescriptor() const {return program.desc;}
  inline const NodeProgramType getProgramType() const {return program;}
  //NodeProgramState* state;
  NodeProgramInstanceWrapper getInstance() const;
  void instantiate();
};

struct NodeInput {
  const nodeid id;
  NodeInput(nodeid id, Node* target):id(id), target(target), source(nullptr) {};
  NodeOutput* source;
  Node* target;
  inline const NodeProgramPortDescriptor* getPortDescriptor() {
    return &target->getProgramDescriptor()->portDescriptors[id-target->id-1];
  }
};

struct NodeOutput {
  const nodeid id;
  NodeOutput(nodeid id, Node* source):id(id), source(source) {};
  std::unordered_set<NodeInput*> targets;
  Node* source;
  inline const NodeProgramPortDescriptor* getPortDescriptor() {
    return &source->getProgramDescriptor()->portDescriptors[id-source->id-1];
  }
};

class NodeGraph {
private:
public:
  std::unordered_map<nodeid, Node> nodes;
  std::unordered_map<nodeid, NodeInput> nodeInputs;
  std::unordered_map<nodeid, NodeOutput> nodeOutputs;
  int objectId=0; // the ids are unique across node IDs, node input IDs and node output IDs,
  Node* addNode(const NodeProgramType program);
  void removeNode(Node& nd);
  inline void removeNode(nodeid id) {
    removeNode(nodes.at(id));
  }
  void addLink(nodeid from, nodeid to);
  void removeLink(nodeid to);
};
