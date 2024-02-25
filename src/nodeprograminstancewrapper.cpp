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
  state({.portState=portStateArr.data(),.userdata=nullptr,.descriptor=node.getProgramDescriptor(),.instanceId=node.id}),
  buffers(node.getProgramDescriptor()->portCount)
{
  NodeGraph& ng=App::get().nodemanager.nodegraph;
  auto lookbackFrames=App::get().lookbackFrames;
  for(int i=0; i<node.getProgramDescriptor()->portCount; i++) {
    const tn_PortDescriptor& portdesc=node.getProgramDescriptor()->portDescriptors[i];
    if(portdesc.role==tn_PortRoleOutput) {
      if(portdesc.type==tn_PortTypeSampleBlock) {
        size_t bufsize=App::get().instreammanager.blocksize*sizeof(float);
        buffers[i].emplace(MultiBuffer{bufsize, lookbackFrames});
      } else if(portdesc.type==tn_PortTypeSpectrum) {
        size_t bufsize=App::get().instreammanager.fftAlign;
        buffers[i].emplace(MultiBuffer{bufsize, lookbackFrames});
      } else if(portdesc.type==tn_PortTypeScalar) {
        size_t bufsize=sizeof(double);
        buffers[i].emplace(MultiBuffer{bufsize, lookbackFrames});
      }
      NodeOutput& output=ng.nodeOutputs.at(node.id+i+1);
    } else if (portdesc.role==tn_PortRoleInput) {
      placeholders.resize(i);
      state.portState[i].portData.count=lookbackFrames;
      if(portdesc.type==tn_PortTypeShader) {
        placeholders.emplace_back(std::make_unique<uint8_t[]>(4*sizeof(float)));
      } else if(portdesc.type==tn_PortTypeScalar) {
        placeholders.emplace_back(std::make_unique<uint8_t[]>(sizeof(double)));
      }
    }
    if(buffers[i].has_value()) {
      state.portState[i].portData.count=lookbackFrames;
      state.portState[i].portData.ring_buffer=buffers[i].value().ring_ptr.data();
    }
  }
  if(auto instantiateFunc=node.getProgramDescriptor()->instantiate) { // FIXME: Needs deinstatiate or resource leak
    state.userdata=instantiateFunc(node.getProgramDescriptor(), state.config);
  }
}

NodeProgramInstanceWrapper::M::M(M &&a) noexcept :
  node(a.node),
  descriptor(a.descriptor),
  portStateArr(a.portStateArr),
  state({.portState=portStateArr.data(),.userdata=nullptr,.descriptor=node.getProgramDescriptor(),.instanceId=node.id})
{
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
        auto sourceNode=source->source;
        // FIXME: value none coredump when connecting shader with shader
        // FIXME: STYLE: split this up, add intermediates and methods to the Node* classes
        MultiBuffer& sourceNodeOutputBuffer=sourceNode->getInstance().m->buffers[source->id-source->source->id-1].value();
        state.portState[i].portData.ring_buffer=sourceNodeOutputBuffer.ring_ptr.data();
      } else {
        state.portState[i].portData.ring_buffer=nullptr;
      }
    }
  }
}

NodeProgramInstanceWrapper::NodeProgramInstanceWrapper(const Node &node):
  m(std::make_shared<M>(node))
{}

NodeProgramInstanceWrapper::M::~M()
{}

void NodeProgramInstanceWrapper::invoke(unsigned long idx)
{
  m->descriptor->invoke(&m->state, idx);
}
