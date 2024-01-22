#include "nodewindow.h"

#include "app.h"
#include "imgui.h"
#include "imnodes.h"
#include "nodegraph.h"
#include "nodeprogramapi.h"
#include "nodeprogramlibrary.h"
#include "nodeprogrammanager.h"
#include "widgets.h"
#include "logs.h"

#include <unordered_map>
#include <utility>
#include <vector>

NodeWindow::NodeWindow() {}

int NodeWindow::init() {
  ImNodes::CreateContext();
  ImNodes::SetNodeGridSpacePos(-1, ImVec2(200.0f, 200.0f));
  ImNodes::SetNodeGridSpacePos(-2, ImVec2(400.0f, 400.0f));

  NodeProgramManager& npm=App::get().nodemanager;
  npm.loadTypes();

  npm.nodegraph.addNode(npm.audioInType.value());
  npm.buildInvocationList();
  return 0;
}

int NodeWindow::show() {
  NodeProgramManager& npm=App::get().nodemanager;
  auto& nodeGraph=App::get().nodemanager.nodegraph;
  bool p_open;
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("node editor", &p_open, ImGuiWindowFlags_NoBackground);
  {
    ImGui::PopStyleVar();

    if(ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_NoPopupHierarchy | ImGuiHoveredFlags_DockHierarchy | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem ) && ImGui::IsMouseClicked(1)) {
      ImGui::OpenPopup("add node");
    }

    if (ImGui::BeginPopup("add node"))
    {
      const ImVec2 click_pos = ImGui::GetMousePosOnOpeningCurrentPopup();

      auto match=npm.library.getPrograms();
      for(auto& prog: match) {
        if(ImGui::MenuItem(prog.desc->identifier)) {
          int newId=nodeGraph.addNode(prog)->id;
          ImNodes::SetNodeScreenSpacePos(newId, click_pos);
        }
      }
      ImGui::EndPopup();
    }
    int link_id;
    if (ImNodes::IsLinkDestroyed(&link_id)) {
      nodeGraph.removeLink(link_id);
      npm.buildInvocationList();
    }
    int start_attr, end_attr;
    if (ImNodes::IsLinkCreated(&start_attr, &end_attr)) {
      NodeOutput& start_output=nodeGraph.nodeOutputs.at(start_attr);
      Node& startnode=*start_output.source;
      int start_node_idx=start_attr-startnode.id-1;
      const tn_PortDescriptor& start_descriptor=startnode.getProgramDescriptor()->portDescriptors[start_node_idx];

      NodeInput& end_input=nodeGraph.nodeInputs.at(end_attr);
      Node& endnode=*end_input.target;
      int end_node_idx=end_attr-endnode.id-1;
      const tn_PortDescriptor& end_descriptor=endnode.getProgramDescriptor()->portDescriptors[end_node_idx];

      if(start_descriptor.type==end_descriptor.type) {
        nodeGraph.addLink(start_attr, end_attr);
      }
      if(!npm.buildInvocationList()) {
        nodeGraph.removeLink(end_attr);
      }
    }

    static int dropped_link_id=-1;
    const bool open_popup = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)
        && ImNodes::IsLinkDropped(&dropped_link_id, false)
        && nodeGraph.nodeOutputs.find(dropped_link_id)!=nodeGraph.nodeOutputs.end();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
    if (!ImGui::IsAnyItemHovered() && open_popup)
    {
      ImGui::OpenPopup("add matching node");
    }

    if (ImGui::BeginPopup("add matching node"))
    {
      const ImVec2 click_pos = ImGui::GetMousePosOnOpeningCurrentPopup();
      int sourceNodeId=nodeGraph.nodeOutputs.at(dropped_link_id).source->id;
      int sourceOutputId=dropped_link_id-sourceNodeId-1;
      tn_PortType sourceOutputType=nodeGraph.nodeOutputs.at(dropped_link_id).source->getProgramDescriptor()->portDescriptors[sourceOutputId].type;

      auto match=npm.library.getProgramsWithInputType(sourceOutputType);
      for(auto& prog: match) {
        if(ImGui::MenuItem(prog.desc->identifier)) {
          int newId=nodeGraph.addNode(prog)->id;
          ImNodes::SetNodeScreenSpacePos(newId, click_pos);
          for(int i=0; i<prog.desc->portCount; i++) {
            if(prog.desc->portDescriptors[i].role==tn_PortRoleInput
               && prog.desc->portDescriptors[i].type==sourceOutputType) {
              nodeGraph.addLink(dropped_link_id, newId+1+i);
              if(!npm.buildInvocationList()) {
                nodeGraph.removeLink(newId+1+i);
              }
              break;
            }
          }
        }
      }
      /*
      for(const auto& nodeType: ng.nodeClassList) {
        const char* nodeName=nodeType.first.c_str();
        if (ImGui::MenuItem(nodeName)) {
          auto newNode=ng.createNode(nodeName);
          ImNodes::SetNodeScreenSpacePos(newNode->id, click_pos);

          if(newNode->inputs.size()==1) {
            //ImNodes::Link(newNode->inputs.front().id,dropped_link_id,newNode->inputs.front().id);
            ng.createLink(dropped_link_id, newNode->inputs.front().id);
          }
        }
      }*/

      ImGui::EndPopup();
    }
    ImGui::PopStyleVar();

  }

  ImNodes::PushColorStyle(ImNodesCol_GridBackground, IM_COL32(0,0,0,0));
  ImNodes::PushColorStyle(ImNodesCol_GridLine, IM_COL32(0,0,0,0));

  std::vector<Node*> delNodes;
  ImNodes::BeginNodeEditor();
  {
    for (auto& node: nodeGraph.nodes) {
      Node& thenode=node.second;
      ImNodes::BeginNode(node.second.id);
      ImNodes::BeginNodeTitleBar();
      if(ImGui::Button("x")) {

        delNodes.push_back(&node.second);
      }
      ImGui::SameLine();
      const tn_Descriptor* program=node.second.getProgramDescriptor();
      // ImGui::Text("%s [%i]", program->identifier, node.second.id);
      ImGui::Text("%s", program->identifier);
      ImNodes::EndNodeTitleBar();
      if(node.second.getProgramType().meta->type==NodeProgramMetadata::Type::Display) {
        DisplayPayload* dp=(DisplayPayload*)node.second.getInstance().getProgramState()->userdata;
        if(dp && dp->renderTex.everWritten) {
          dp->renderTex.endShow();
          ImGui::Image((void*)(intptr_t)dp->renderTex.beginShow(), ImVec2(160, 120));
        } else {
          ImGui::Button("Preview Box", ImVec2(160, 120));
        //
        }
      }

      if(node.second.getProgramType().desc->identifier==npm.plotIdentifier) {
        ImNodes::PushColorStyle(ImNodesCol_Pin, ImColor::HSV(0.58,0.79,1,0.7));
        ImNodes::PushColorStyle(ImNodesCol_PinHovered, ImColor::HSV(0.58,0.79,1,1));
        ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);
        ImNodes::BeginInputAttribute(node.second.id+1);
        if(node.second.in[0]->source) {
          Node* n=node.second.in[0]->source->source;
          int oid=node.second.in[0]->source->id-n->id-1;
          InStreamManager& ism=App::get().instreammanager;

          auto buffers=*n->getInstance().getBuffers();
          auto ring=buffers[oid].value().ring_ptr;
          std::tuple<std::vector<tn_PortMessage>, size_t> accDesc={ring, ism.ringIdx};
          ImGui::PlotLines("",[](void* data, int idx) -> float {
              auto desc=(std::tuple<std::vector<tn_PortMessage>, size_t>*)data;
              int startIdx=std::get<1>(*desc);
              auto vec=std::get<0>(*desc);
              int ringCount=vec.size();
              return *vec[(startIdx+idx)%ringCount].scalar.v;
            }, (void*)&accDesc
                           ,ring.size(),0,nullptr,FLT_MAX,FLT_MAX,ImVec2(300,100));

        }
        ImNodes::EndInputAttribute();
        ImNodes::PopAttributeFlag();
        ImNodes::PopColorStyle();
      } else {
        for(int i=0; i<program->portCount; i++) {
          const tn_PortDescriptor& portdesc=program->portDescriptors[i];
          switch(portdesc.type) {
          case tn_PortTypeScalar:
            ImNodes::PushColorStyle(ImNodesCol_Pin, ImColor::HSV(0.58,0.79,1,0.7));
            ImNodes::PushColorStyle(ImNodesCol_PinHovered, ImColor::HSV(0.58,0.79,1,1));
          break;
          case tn_PortTypeSampleBlock:
            ImNodes::PushColorStyle(ImNodesCol_Pin, ImColor::HSV(0.83,0.79,1,0.7));
            ImNodes::PushColorStyle(ImNodesCol_PinHovered, ImColor::HSV(0.83,0.79,1,1));
          break;
          case tn_PortTypeSpectrum:
            ImNodes::PushColorStyle(ImNodesCol_Pin, ImColor::HSV(0.33,0.79,1,0.7));
            ImNodes::PushColorStyle(ImNodesCol_PinHovered, ImColor::HSV(0.33,0.79,1,1));
          break;
          case tn_PortTypeShader:
            ImNodes::PushColorStyle(ImNodesCol_Pin, ImColor::HSV(0.08,0.79,1,0.7));
            ImNodes::PushColorStyle(ImNodesCol_PinHovered, ImColor::HSV(0.08,0.79,1,1));
          break;
          default:
            ImNodes::PushColorStyle(ImNodesCol_Pin, IM_COL32(0,255,0,255));
          break;
          }

          switch (portdesc.role) {
            case tn_PortRoleInput: {
              auto* source=nodeGraph.nodeInputs.at(node.second.id+1+i).source;
              ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);
              ImNodes::BeginInputAttribute(node.second.id+1+i);
              const auto& placeholderlist=node.second.getInstance().getPlaceholders();
              if(!source && i<placeholderlist->size() && placeholderlist->at(i).has_value()) {
                auto* placeholder=node.second.getInstance().getPlaceholders()->at(i).value().get();
                if(portdesc.type==tn_PortTypeShader) {
                  ImGui::ColorEdit4("Placeholder Color", (float*)placeholder, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel );
                  ImGui::SameLine();
                } else if(portdesc.type==tn_PortTypeScalar) {
                  ImGui::SetNextItemWidth(20.0f);
                  ImGui::DragScalar("", ImGuiDataType_Double, (float*)placeholder, 0.1f);
                  ImGui::SameLine();
                }
              }
              ImGui::TextUnformatted(portdesc.name);
              ImNodes::EndInputAttribute();
              ImNodes::PopAttributeFlag();
              break;
            }
            case tn_PortRoleOutput:
            ImNodes::BeginOutputAttribute(node.second.id+1+i);
            ImGui::TextUnformatted(portdesc.name);
            ImNodes::EndInputAttribute();
            default:
            break;
          }
          ImNodes::PopColorStyle();
        }
      }
      ImNodes::EndNode();
    }

    // TODO: Check implications of referencing a shared_ptr
    /*for(const auto& node: ng.nodeList) {
      ImNodes::BeginNode(node.second->id);
      ImNodes::BeginNodeTitleBar();
      if(ImGui::Button("x")) {
        // TODO
        Logs::get().log(logSeverity_Debug, "UI", "Stub!");
      }
      ImGui::SameLine();
      if(App::get().nodeWindowShowNodeId) {
        ImGui::Text("%s [%i]", node.second->name.c_str(), node.second->id);
      } else {
        ImGui::TextUnformatted(node.second->name.c_str());
      }
      ImNodes::EndNodeTitleBar();
      ImNodes::PushAttributeFlag(ImNodesAttributeFlags_EnableLinkDetachWithDragClick);
      for(const auto& input: node.second->inputs) {
        ImNodes::BeginInputAttribute(input.id);
        if(input.drawUi) {
          input.drawUi(input);
        }
        ImNodes::EndInputAttribute();
      }
      ImNodes::PopAttributeFlag();
      for(const auto& output: node.second->outputs) {
        ImNodes::BeginOutputAttribute(output.id);
        if(output.drawUi) {
          output.drawUi(output);
        }
        ImNodes::EndOutputAttribute();
      }
      ImNodes::EndNode();

    }*/
    for(auto& input: nodeGraph.nodeInputs) {
      if(input.second.source!=nullptr) {
        ImNodes::Link(input.second.id, input.second.source->id, input.second.id);
      }
    }
  }
  /*for(auto& edge: ng.edgeList) {
      ImNodes::Link(edge.first,edge.second.first->id, edge.second.second->id);
    }*/

  ImNodes::MiniMap(0.05, ImNodesMiniMapLocation_BottomRight);
  ImNodes::EndNodeEditor();

  int pinHovered=0;
  if(ImNodes::IsPinHovered(&pinHovered)) {
    auto out=nodeGraph.nodeOutputs.find(pinHovered);
    if(out!=nodeGraph.nodeOutputs.end()) {
      Node* n=out->second.source;
      int oid=pinHovered-n->id-1;
      InStreamManager& ism=App::get().instreammanager;
      const tn_PortType type=n->getProgramDescriptor()->portDescriptors[oid].type;
      if(type==tn_PortTypeSampleBlock) {
        ImGui::BeginTooltip();
        auto buffers=*n->getInstance().getBuffers();
        ImGui::PlotLines("",buffers[oid].value().ring_ptr[ism.ringIdx].time_window.buffer,1024,0,nullptr,-1,1,ImVec2(300,100));
        ImGui::EndTooltip();
      } else if(type==tn_PortTypeSpectrum) {
        ImGui::BeginTooltip();
        if(ism.instreamInfo()) {
          auto buffers=*n->getInstance().getBuffers();
          std::complex<float>* arr=(std::complex<float>*)buffers[oid].value().ring_ptr[ism.ringIdx].frequency_window.buffer;
          ImGui::PlotSpectrum("",arr, ism.fftElem, ism.instreamInfo()->sample_rate, NULL,0, 30, ImVec2(300,100));
        } else {
          ImGui::Text("no input active/samplerate unknown");
        }
        ImGui::EndTooltip();
      } else if(type==tn_PortTypeScalar) {
        ImGui::BeginTooltip();
        auto buffers=*n->getInstance().getBuffers();
        auto ring=buffers[oid].value().ring_ptr;
        std::tuple<std::vector<tn_PortMessage>, size_t> accDesc={ring, ism.ringIdx};
        ImGui::PlotLines("",[](void* data, int idx) -> float {
            auto desc=(std::tuple<std::vector<tn_PortMessage>, size_t>*)data;
            int startIdx=std::get<1>(*desc);
            auto vec=std::get<0>(*desc);
            int ringCount=vec.size();
            return *vec[(startIdx+idx)%ringCount].scalar.v;
          }, (void*)&accDesc
                         ,ring.size(),0,nullptr,FLT_MAX,FLT_MAX,ImVec2(300,100));
        ImGui::EndTooltip();
      }
    }
  }

  if(ImGui::IsKeyReleased(ImGuiKey_Delete) &&
     (ImNodes::NumSelectedNodes() | ImNodes::NumSelectedLinks() ) ) {
    std::vector<int> nodesSelected(ImNodes::NumSelectedNodes());
    ImNodes::GetSelectedNodes(nodesSelected.data());
    for(nodeid nid:nodesSelected) {
      delNodes.push_back(&nodeGraph.nodes.at(nid));
    }
  }
  for(Node* delNode:delNodes) {
    nodeGraph.removeNode(*delNode);
  }
  if(!delNodes.empty()) {
    if(!npm.buildInvocationList()) {
      Logs::get().log(logSeverity_Err, "nodewindow", "cannot recover from a graph made invalid by node deletion");
    }
  }

  ImNodes::PopColorStyle();
  ImNodes::PopColorStyle();
  ImGui::End();
  return 0;
}
