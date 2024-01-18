#pragma once

#include "nodeprogramapi.h"

#include "multibuffer.h"

#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class Node;

class NodeProgramInstanceWrapper {
private:
  struct M {
    const Node& node;
    const tn_Descriptor* descriptor;
    std::vector<tn_PortState> portStateArr;
    tn_State state;
    M(const Node& node);
    ~M();
    *M(const M&) = delete;
    M(M&& a) noexcept;
    M& operator=(const M&) = delete;
		std::vector<std::optional<MultiBuffer>> buffers; // TODO: obsolete
    std::vector<std::optional<std::unique_ptr<uint8_t[]>>> placeholders; // HACK: instead introduce a playload type (polymorphic/union)
    void linkBuffers();
  };
  std::shared_ptr<M> m;
protected:
  NodeProgramInstanceWrapper(const Node& node);
  void linkBuffers() {m->linkBuffers();};
  friend class Node;
  friend class NodeProgramManager;
public:
  //~NodeProgramInstanceWrapper();
  /*NodeProgramInstanceWrapper(const NodeProgramInstanceWrapper&) = delete;
    NodeProgramInstanceWrapper(NodeProgramInstanceWrapper&& a) noexcept;
    NodeProgramInstanceWrapper& operator=(const NodeProgramInstanceWrapper&) = delete;*/
  void invoke(unsigned long idx);
  inline std::vector<std::optional<MultiBuffer>>* getBuffers() {return &m->buffers;}
  inline std::vector<std::optional<std::unique_ptr<uint8_t[]>>>* getPlaceholders() {return &m->placeholders;}
  inline tn_State* getProgramState() {return &m->state;}
};
