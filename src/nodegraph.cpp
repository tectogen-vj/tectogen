#include "nodegraph.h"

#include <utility>


Node *NodeGraph::addNode(const NodeProgramType program) {
  Node* newNode=&nodes.insert({objectId, Node(objectId, program)}).first->second;
  objectId++;
  for(int i=0; i<program.desc->portCount; i++) {
    const tn_PortDescriptor& port=program.desc->portDescriptors[i];
    switch (port.role) {
      case tn_PortRoleInput:
      {
        const auto& nin=nodeInputs.insert({objectId, NodeInput(objectId, newNode)});
        newNode->in.push_back(&nin.first->second);
        break;
      }
      case tn_PortRoleOutput:
      {
        const auto& non=nodeOutputs.insert({objectId, NodeOutput(objectId, newNode)});
        newNode->out.push_back(&non.first->second);
        break;
      }
      default:
      break;
    }
    objectId++;
  }
  // allocates the node objects buffers
  newNode->instantiate();
  return newNode;
}

void NodeGraph::removeNode(Node &nd) {
  std::vector<nodeid> removelinkids;
  for(NodeInput* in:nd.in) {
    if(in->source) {
      removelinkids.push_back(in->id);
    }
  }
  for(NodeOutput* out:nd.out) {
    for(NodeInput* in:out->targets) {
      removelinkids.push_back(in->id);
    }
  }
  for(nodeid linkid:removelinkids) {
    removeLink(linkid);
  }
  nodes.erase(nd.id);
}

void NodeGraph::addLink(nodeid from, nodeid to) {
  NodeOutput& out=nodeOutputs.at(from);
  NodeInput& in=nodeInputs.at(to);
  out.targets.insert(&in);
  in.source=&out;
}

void NodeGraph::removeLink(nodeid to) {
  NodeInput& in=nodeInputs.at(to);
  in.source->targets.erase(&in);
  in.source=nullptr;
}
