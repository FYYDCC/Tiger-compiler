#include "tiger/liveness/flowgraph.h"
#include <map>

namespace fg {

void FlowGraphFactory::AssemFlowGraph() {
  /* TODO: Put your lab6 code here */
  int edges = 0;

  std::list<assem::Instr *> instr_list = instr_list_->GetList();
  graph::Node<assem::Instr> *now = nullptr;
  graph::Node<assem::Instr> *prev = nullptr;
  std::vector<graph::Node<assem::Instr> *> jump_nodes;

  // jump单独处理 其余加边
  for (auto instr : instr_list) {
    bool is_jmp=false;
    now = flowgraph_->NewNode(instr);

    if (typeid(*instr) == typeid(assem::LabelInstr)) {
      label_table->Enter(((assem::LabelInstr *)instr)->label_, now);
    }

    if (typeid(*instr) == typeid(assem::OperInstr)) {
      auto oper_instr = ((assem::OperInstr *)instr);
      if (oper_instr->jumps_) {
        jump_nodes.push_back(now);
        is_jmp = oper_instr->assem_.find("jmp") != std::string::npos;
      }
    }

    if (prev) {
      edges++;
      flowgraph_->AddEdge(prev, now);
    }

    if (is_jmp) {
      prev = nullptr;
    } else {
      prev = now;
    }
  }

  for (auto node : jump_nodes) {
    auto instr = (assem::OperInstr *)(node->NodeInfo());
    auto label = (*instr->jumps_->labels_)[0];
    flowgraph_->AddEdge(node, label_table->Look(label));
    edges++;
  }
}

} // namespace fg

namespace assem {

temp::TempList *LabelInstr::Def() const {
  /* TODO: Put your lab6 code here */
  return new temp::TempList();
}

temp::TempList *MoveInstr::Def() const {
  /* TODO: Put your lab6 code here */
  if (dst_) {
    return dst_;
  } else {
    return new temp::TempList();
  }
}

temp::TempList *OperInstr::Def() const {
  /* TODO: Put your lab6 code here */
  if (dst_) {
    return dst_;
  } else {
    return new temp::TempList();
  }
}

temp::TempList *LabelInstr::Use() const {
  /* TODO: Put your lab6 code here */
  return new temp::TempList();
}

temp::TempList *MoveInstr::Use() const {
  /* TODO: Put your lab6 code here */
  if (src_) {
    return src_;
  } else {
    return new temp::TempList();
  }
}

temp::TempList *OperInstr::Use() const {
  /* TODO: Put your lab6 code here */
  if (src_) {
    return src_;
  } else {
    return new temp::TempList();
  }
}
} // namespace assem
