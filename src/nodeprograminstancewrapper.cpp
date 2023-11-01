#include "nodeprograminstancewrapper.h"

#include "app.h"
#include "nodegraph.h"

NodeProgramInstanceWrapper Node::getInstance() const {
  return instance.value();
}

void Node::instantiate() {
  if(!instance.has_value()) {
    instance.emplace(NodeProgramInstanceWrapper(*this));
  }
}

NodeProgramInstanceWrapper::M::M(const Node &node) :
  node(node),
  descriptor(node.getProgramDescriptor()),
  portStateArr(node.getProgramDescriptor()->portCount),
  state({.portState=portStateArr.data(),.userdata=nullptr,.descriptor=node.getProgramDescriptor(),.instanceId=node.id})
{
  NodeGraph& ng=App::get().nodemanager.nodegraph;
  for(int i=0; i<node.getProgramDescriptor()->portCount; i++) {
    const tn_PortDescriptor& portdesc=node.getProgramDescriptor()->portDescriptors[i];
    if(portdesc.role==tn_PortRoleOutput) {
      // make sure to emplace at index i, insert nullopts before
      buffers.resize(i);
      if(portdesc.type==tn_PortTypeSampleBlock) {
        size_t bufsize=App::get().instreammanager.blocksize*sizeof(float);
        buffers.emplace_back(bufsize);
        state.portState[i].payload_symbol_to_be_obsoleted=buffers.back()->getWrite();
      } else if(portdesc.type==tn_PortTypeSpectrum) {
        size_t bufsize=App::get().instreammanager.fftAlign;
        buffers.emplace_back(bufsize);
        state.portState[i].payload_symbol_to_be_obsoleted=buffers.back()->getWrite();
      } else if(portdesc.type==tn_PortTypeScalar) {
        size_t bufsize=sizeof(double);
        buffers.emplace_back(bufsize);
        state.portState[i].payload_symbol_to_be_obsoleted=buffers.back()->getWrite();
      }
      NodeOutput& output=ng.nodeOutputs.at(node.id+i+1);
    } else if (portdesc.role==tn_PortRoleInput) {
      placeholders.resize(i);
      if(portdesc.type==tn_PortTypeShader) {
        placeholders.emplace_back(std::make_unique<uint8_t[]>(4*sizeof(float)));
      } else if(portdesc.type==tn_PortTypeScalar) {
        placeholders.emplace_back(std::make_unique<uint8_t[]>(sizeof(double)));
      }
    }
  }
}

NodeProgramInstanceWrapper::M::M(M &&a) noexcept :
  node(a.node),
  descriptor(a.descriptor),
  portStateArr(a.portStateArr),
  state({.portState=portStateArr.data(),.userdata=nullptr,.descriptor=node.getProgramDescriptor(),.instanceId=node.id})
{
  for(int i=0; i<descriptor->portCount; i++) {
    state.portState[i].payload_symbol_to_be_obsoleted=a.state.portState[i].payload_symbol_to_be_obsoleted;
    a.state.portState[i].payload_symbol_to_be_obsoleted=nullptr;
  }
}

void NodeProgramInstanceWrapper::M::linkBuffers()
{
  NodeGraph& ng=App::get().nodemanager.nodegraph;
  for(int i=0; i<node.getProgramDescriptor()->portCount; i++) {
    const tn_PortDescriptor& portdesc=node.getProgramDescriptor()->portDescriptors[i];
    if (portdesc.role==tn_PortRoleInput && portdesc.type!=tn_PortTypeShader) {
      // STYLE
      NodeInput& input=ng.nodeInputs.at(node.id+i+1);
      NodeOutput* source=input.source;
      if(source) {
        // FIXME: value none coredump when connecting shader with shader
        // FIXME: STYLE: split this up, add intermediates and methods to the Node* classes
        state.portState[i].payload_symbol_to_be_obsoleted=source->source->getInstance().m->buffers[source->id-source->source->id-1].value().getWrite();
      } else {
        state.portState[i].payload_symbol_to_be_obsoleted=nullptr;
      }
    }
  }
}

NodeProgramInstanceWrapper::NodeProgramInstanceWrapper(const Node &node):
  m(std::make_shared<M>(node))
{}

NodeProgramInstanceWrapper::M::~M()
{}

void NodeProgramInstanceWrapper::invoke()
{
  m->descriptor->invoke(nullptr,&m->state);
}
