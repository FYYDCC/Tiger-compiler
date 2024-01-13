#include "tiger/frame/x64frame.h"
#include <sstream>

extern frame::RegManager *reg_manager;

namespace frame {
/* TODO: Put your lab5 code here */
temp::TempList *X64RegManager::Registers(){
  temp::TempList* templist = new temp::TempList();
  templist->Append(regs_[0]);
  templist->Append(regs_[1]);
  templist->Append(regs_[2]);
  templist->Append(regs_[3]);
  templist->Append(regs_[4]);
  templist->Append(regs_[5]);
  templist->Append(regs_[6]);
  templist->Append(regs_[8]);
  templist->Append(regs_[9]);
  templist->Append(regs_[10]);
  templist->Append(regs_[11]);
  templist->Append(regs_[12]);
  templist->Append(regs_[13]);
  templist->Append(regs_[14]);
  templist->Append(regs_[15]);
}

temp::TempList *X64RegManager::ArgRegs() {
  temp::TempList *result = new temp::TempList();
  result->Append(regs_[5]);
  result->Append(regs_[4]);
  result->Append(regs_[3]);
  result->Append(regs_[2]);
  result->Append(regs_[8]);
  result->Append(regs_[9]);
  return result;
}

temp::TempList *X64RegManager::CallerSaves() {
  temp::TempList *result = new temp::TempList();
  result->Append(regs_[0]);
  result->Append(regs_[2]);
  result->Append(regs_[3]);
  result->Append(regs_[4]);
  result->Append(regs_[5]);
  result->Append(regs_[8]);
  result->Append(regs_[9]);
  result->Append(regs_[10]);
  result->Append(regs_[10]);
  result->Append(regs_[11]);
  return result;
}


temp::TempList *X64RegManager::CalleeSaves() {
  temp::TempList *result = new temp::TempList();
  result->Append(regs_[1]);
  result->Append(regs_[6]);
  result->Append(regs_[12]);
  result->Append(regs_[13]);
  result->Append(regs_[14]);
  result->Append(regs_[15]);
  return result;
}


temp::TempList *X64RegManager::ReturnSink() {
  temp::TempList *result = new temp::TempList();
  result->Append(regs_[1]);
  result->Append(regs_[6]);
  result->Append(regs_[12]);
  result->Append(regs_[13]);
  result->Append(regs_[14]);
  result->Append(regs_[15]);
  result->Append(regs_[7]);
  result->Append(regs_[0]);
  return result;
}

int X64RegManager::WordSize() {
  return 8;
}

temp::Temp *X64RegManager::FramePointer() {
  return regs_[6];
}

temp::Temp *X64RegManager::StackPointer() {
  return regs_[7];
} 

temp::Temp *X64RegManager::ReturnValue() {
  return regs_[0];
}



/* TODO: Put your lab5 code here */
tree::Stm* ProcEntryExit1(Frame *frame, tree::Stm *stm) {
  if(frame->view_shift_ != nullptr) {
    return new tree::SeqStm(frame->view_shift_, stm);
  } 
  return stm;
}


assem::InstrList* ProcEntryExit2(assem::InstrList* body) {
  static temp::TempList *sink_list = nullptr;
  if(!sink_list) {
    sink_list = reg_manager->ReturnSink();
  }
  body->Append(new assem::OperInstr("", nullptr, sink_list, nullptr));
  return body;
}


assem::Proc* ProcEntryExit3(frame::Frame* frame, assem::InstrList* body) {
  std::ostringstream prologue;
  std::ostringstream epilogue;

  prologue << ".set " << frame->label_->Name() << "_framesize, " << std::to_string(-frame->s_offset) << "\n";
  prologue << frame->label_->Name() << ":\n";
  prologue << "subq $" << std::to_string(-frame->s_offset) << ", %rsp\n";

  epilogue << "addq $" << std::to_string(-frame->s_offset) << ", %rsp\n";
  epilogue << "retq\n";

  return new assem::Proc(prologue.str(), body, epilogue.str());
}

X64Frame::X64Frame(temp::Label *name, std::list<bool> formals){
    view_shift_ = nullptr;
    label_ = name;
    s_offset = 0;
    for(auto formal : formals) {
      formals_.emplace_back(AllocLocal(formal));
    }
    newFrame(formals);
}

void X64Frame::newFrame(std::list<bool> formals) {
    std::list<temp::Temp *> arg_reg_list = reg_manager->ArgRegs()->GetList();
    int max_inreg = arg_reg_list.size();
    auto it = arg_reg_list.begin();
    int nth = 0;

    tree::Exp *dst, *src;
    tree::Stm *stm;

    for(auto formal : formals_) {
      dst = formal->ToExp(new tree::TempExp(reg_manager->FramePointer()));
      if(nth < max_inreg) {
        src = new tree::TempExp(*it);
        ++it;
      } else {
        src = new tree::MemExp(new tree::BinopExp(tree::BinOp::PLUS_OP, new tree::TempExp(reg_manager->FramePointer()), new tree::ConstExp((nth - max_inreg + 2) * 8)));
      }
      ++nth;
      stm = new tree::MoveStm(dst, src);

      if(view_shift_ != nullptr) {
        view_shift_ = new tree::SeqStm(view_shift_, stm);
      } else {
        view_shift_ = stm;
      }
    }
}

Access* X64Frame::AllocLocal(bool escape) {
    Access *access = nullptr;
    if(escape) {
      s_offset -= 8;
      access = new InFrameAccess(s_offset);
    } else {
      access = new InRegAccess(temp::TempFactory::NewTemp());
    }
    return access;
  }

std::string X64Frame::GetLabel() {
    return label_->Name();
}
} // namespace frame
