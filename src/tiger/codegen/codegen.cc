#include "tiger/codegen/codegen.h"

#include <cassert>
#include <sstream>

extern frame::RegManager *reg_manager;

namespace {

constexpr int maxlen = 1024;


} // namespace

namespace cg {

void CodeGen::Codegen() {
  /* TODO: Put your lab5 code here */
  assem::InstrList *instr_list = new assem::InstrList();
  std::string frame_name = frame_->label_->Name();
  fs_ = frame_name + "_framesize";

  temp::TempList *regs = reg_manager->CalleeSaves();
  std::list<temp::Temp*> regList = regs->GetList();
  std::string assem = "movq `s0, `d0";
  for(auto reg : regList) {
    temp::Temp *new_temp = temp::TempFactory::NewTemp();
    instr_list->Append(new assem::OperInstr(assem, new temp::TempList({new_temp}), new temp::TempList({reg}), nullptr));
    tmp_regs.push_back(new_temp);
  }
  tree::StmList *stms = traces_->GetStmList();
  std::list<tree::Stm*> stm_list = stms->GetList();

  for(auto stm : stm_list) {
    stm->Munch(*instr_list, fs_);
  }
  temp::TempList *regsCallee = reg_manager->CalleeSaves();
  std::list<temp::Temp*> regListcallee = regsCallee->GetList();
  std::string assem2 = "movq `s0, `d0";

  int pos = tmp_regs.size() - 1;
  for(auto it = regListcallee.rbegin(); it != regListcallee.rend(); ++it) {
    instr_list->Append(new assem::OperInstr(assem2, new temp::TempList({*it}), new temp::TempList({tmp_regs[pos]}), nullptr));
    --pos;
  }
  tmp_regs.clear();

  assem_instr_ = std::make_unique<AssemInstr>(frame::ProcEntryExit2(instr_list));
}



void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}
} // namespace cg

namespace tree {
/* TODO: Put your lab5 code here */

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  left_->Munch(instr_list, fs);
  right_->Munch(instr_list, fs);
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  std::string assem = label_->Name();
  instr_list.Append(new assem::LabelInstr(assem, label_));
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
    instr_list.Append(new assem::OperInstr("jmp `j0", nullptr, nullptr,
                                         new assem::Targets(jumps_)));
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  std::string op = "";
  switch (op_) {
    case EQ_OP:
      op = "je";
      break;
    case NE_OP:
      op = "jne";
      break;
    case LT_OP:
      op = "jl";
      break;
    case GT_OP:
      op = "jg";
      break;
    case LE_OP: 
      op = "jle";
      break;
    case GE_OP: 
      op = "jge";
      break;
    case ULT_OP:
      op = "jb";
      break;
    case ULE_OP:
      op = "jbe";
      break;
    case UGT_OP:
      op = "ja";
      break;
    case UGE_OP:
      op = "jae";
      break;
    default: {
      break;
    }
  }
  temp::Temp *left = left_->Munch(instr_list, fs);
  temp::Temp *right = right_->Munch(instr_list, fs);

  temp::TempList *srcList = new temp::TempList();
  srcList->Append(left);
  srcList->Append(right);
  std::string assemble = "cmpq `s1, `s0";

  instr_list.Append(new assem::OperInstr(assemble, nullptr, srcList, nullptr));
  std::string jumpLabel = "`j0";
  if(true_label_) {
      jumpLabel = true_label_->Name();
  }
  assemble = op + " " + jumpLabel;
  auto labelList = new std::vector<temp::Label *>();
  labelList->push_back(true_label_);
  instr_list.Append(new assem::OperInstr(assemble, nullptr, nullptr, new assem::Targets(labelList)));
}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  if (typeid(*dst_) == typeid(MemExp)) {
    auto dstMem = static_cast<MemExp *>(dst_);
    if (typeid(*dstMem->exp_) == typeid(BinopExp)) {
      auto memBinop = static_cast<BinopExp *>(dstMem->exp_);

      if (memBinop->op_ == PLUS_OP) {
        if (typeid(*memBinop->left_) == typeid(ConstExp)) {
          auto right_temp = memBinop->right_->Munch(instr_list, fs);
          auto leftExp = static_cast<ConstExp *>(memBinop->left_);
          std::ostringstream assem;
          if (typeid(*src_) == typeid(ConstExp)) {
            auto srcExp = static_cast<ConstExp *>(src_);
            if (right_temp == reg_manager->FramePointer()) {
              assem << "movq $" << srcExp->consti_ << ",(" << fs << "_framesize" << leftExp->consti_ << ")(`s0)";
              right_temp = reg_manager->StackPointer();
            } 
            else {
              assem << "movq $" << srcExp->consti_ << "," << leftExp->consti_ << "(`s0)";
            }

            instr_list.Append(new assem::OperInstr(assem.str(), nullptr, new temp::TempList(right_temp),nullptr));
          } 
          else {
            if (right_temp == reg_manager->FramePointer()) {
              assem << "movq `s0,(" << fs << "_framesize" << leftExp->consti_ << ")(`s1)";
              right_temp = reg_manager->StackPointer();
            } 
            else {
              assem << "movq `s0," << leftExp->consti_ << "(`s1)";
            }
            instr_list.Append(
              new assem::OperInstr(
                assem.str(),
                new temp::TempList(),
                new temp::TempList{src_->Munch(instr_list, fs), right_temp},
                nullptr
              )
            );
          }
        } 
        else if (typeid(*memBinop->right_) == typeid(ConstExp)) {
          auto left_temp = memBinop->left_->Munch(instr_list, fs);
          auto rightExp = static_cast<ConstExp *>(memBinop->right_);
          std::ostringstream assem;
          if (typeid(*src_) == typeid(ConstExp)) {
            auto srcExp = static_cast<ConstExp *>(src_);
            if (left_temp == reg_manager->FramePointer()) {
              assem << "movq $" << srcExp->consti_ << ",(" << fs << "_framesize" << rightExp->consti_ << ")(`s0)";
              left_temp = reg_manager->StackPointer();
            } else {
              assem << "movq $" << srcExp->consti_ << "," << rightExp->consti_ << "(`s0)";
            }
            instr_list.Append(
              new assem::OperInstr(
                assem.str(),
                nullptr,
                new temp::TempList(left_temp),
                nullptr
              )
            );
          } 
          else {
            if (left_temp == reg_manager->FramePointer()) {
              assem << "movq `s0,(" << fs << "_framesize" << rightExp->consti_ << ")(`s1)";
              left_temp = reg_manager->StackPointer();
            } else {
              assem << "movq `s0," << rightExp->consti_ << "(`s1)";
            }
            instr_list.Append(
              new assem::OperInstr(
                assem.str(),
                nullptr,
                new temp::TempList{src_->Munch(instr_list, fs), left_temp},
                nullptr
              )
            );
          }
        } 
        else {
          std::ostringstream assem;
          auto srcMunch = new temp::TempList{src_->Munch(instr_list, fs), dstMem->exp_->Munch(instr_list, fs)};
          assem << "movq `s0,(`s1)";
          instr_list.Append(
            new assem::OperInstr(
              assem.str(),
              nullptr,
              srcMunch,
              nullptr
            )
          );
        }
      }
    } else if (typeid(*dstMem->exp_) == typeid(ConstExp)) {
      auto dst2cst = static_cast<ConstExp *>(dstMem->exp_);
      if (typeid(*src_) != typeid(ConstExp)) {
        std::stringstream assem;
        assem << "movq `s0," << dst2cst->consti_;
        instr_list.Append(
          new assem::OperInstr(
            assem.str(),
            new temp::TempList(),
            new temp::TempList(src_->Munch(instr_list, fs)),
            nullptr
          )
        );
      } else {
        auto srcExp = static_cast<ConstExp *>(src_);
        std::string assem;
        assem = "movq $" + std::to_string(srcExp->consti_) + "," + std::to_string(dst2cst->consti_);
        instr_list.Append(
          new assem::OperInstr(
            assem,
            new temp::TempList(),
            new temp::TempList(),
            nullptr
          )
        );
      }
    } else {
      if (typeid(*src_) != typeid(ConstExp)) {
        auto srcMunch = new temp::TempList{src_->Munch(instr_list, fs), dstMem->exp_->Munch(instr_list, fs)};
        instr_list.Append(new assem::OperInstr("movq `s0,(`s1)",nullptr,srcMunch,nullptr));
      } 
      else {
        std::stringstream assem;
        auto srcExp = static_cast<ConstExp *>(src_);
        assem << "movq $" << srcExp->consti_ << "(`s0)";
        auto dstmemMunch = new temp::TempList(dstMem->Munch(instr_list, fs));
        instr_list.Append(new assem::OperInstr(assem.str(), nullptr, dstmemMunch, nullptr));
      }
    }
  } else if(typeid(*src_) == typeid(MemExp)) {
    auto srcMem = static_cast<MemExp *>(src_);
    if (typeid(*srcMem->exp_) == typeid(BinopExp)) {
      auto memBinop = static_cast<BinopExp *>(srcMem->exp_);
      if (memBinop->op_ == PLUS_OP) {
        if (typeid(*memBinop->left_) == typeid(ConstExp)) {
          auto right_temp = memBinop->right_->Munch(instr_list, fs);
          auto leftExp = static_cast<ConstExp *>(memBinop->left_);
          std::ostringstream assem;
          if (right_temp == reg_manager->FramePointer()) {
            if (leftExp->consti_ < 0) {
              assem << "movq (" << fs << "_framesize" << leftExp->consti_ << ")(`s0)," << "`d0";
            } else {
              assem << "movq (" << fs << "_framesize+" << leftExp->consti_ << ")(`s0)," << "`d0";
            }
            right_temp == reg_manager->StackPointer();
          } 
          else {
            assem << "movq " << leftExp->consti_ << "(`s0)," << "`d0";
          }
          instr_list.Append(
            new assem::OperInstr(
              assem.str(),
              new temp::TempList(dst_->Munch(instr_list, fs)),
              new temp::TempList(right_temp),
              nullptr
            )
          );
        } 
        else if (typeid(*memBinop->right_) == typeid(ConstExp)) {
          auto left_temp = memBinop->left_->Munch(instr_list, fs);
          auto rightExp = static_cast<ConstExp *>(memBinop->right_);
          std::ostringstream assem;

          if (left_temp == reg_manager->FramePointer()) {
            if (rightExp->consti_ < 0) {
              assem << "movq (" << fs << "_framesize" << rightExp->consti_ << ")(`s0)," << "`d0";
            } 
            else {
              assem << "movq (" << fs << "_framesize+" << rightExp->consti_ << ")(`s0)," << "`d0";
            }
            left_temp = reg_manager->StackPointer();
          } 
          else {
            assem << "movq " << rightExp->consti_ << "(`s0)," << "`d0";
          }
          instr_list.Append(
            new assem::OperInstr(
              assem.str(),
              new temp::TempList(dst_->Munch(instr_list, fs)),
              new temp::TempList(left_temp),
              nullptr
            )
          );
        } else {
          instr_list.Append(
            new assem::OperInstr(
              "movq (`s0),`d0",
              new temp::TempList(dst_->Munch(instr_list, fs)),
              new temp::TempList(srcMem->exp_->Munch(instr_list, fs)),
              nullptr
            )
          );
        }
      }
    } else if (typeid(*srcMem->exp_) == typeid(ConstExp)) {
      auto srcExp = static_cast<ConstExp *>(srcMem->exp_);
      std::ostringstream assem;
      assem << "movq " << srcExp->consti_ << ",`d0";
      instr_list.Append(
        new assem::OperInstr(
          assem.str(),
          new temp::TempList(dst_->Munch(instr_list, fs)),
          new temp::TempList(),
          nullptr
        )
      );
    } else {
      auto dstMunch = new temp::TempList(dst_->Munch(instr_list, fs));
      auto srcmemMunch = new temp::TempList(srcMem->exp_->Munch(instr_list, fs)); 
      instr_list.Append(new assem::OperInstr("movq (`s0),`d0", dstMunch, srcmemMunch, nullptr));
    }
  } else if (typeid(*src_) == typeid(ConstExp)) {
    auto srcExp = static_cast<ConstExp *>(src_);
    std::ostringstream assem;
    assem << "movq $" << srcExp->consti_ << ",`d0";
    instr_list.Append(
      new assem::OperInstr(
        assem.str(),
        new temp::TempList(dst_->Munch(instr_list, fs)),
        new temp::TempList(),
        nullptr
      )
    );
  } else {
    instr_list.Append(
      new assem::MoveInstr(
        "movq `s0,`d0",
        new temp::TempList(dst_->Munch(instr_list, fs)),
        new temp::TempList(src_->Munch(instr_list, fs))
      )
    );
  }
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  exp_->Munch(instr_list, fs);
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto left_temp = left_->Munch(instr_list, fs);
  auto right_temp = right_->Munch(instr_list, fs);
  auto result_temp = temp::TempFactory::NewTemp();
  std::string assem;
  assem = "movq `s0, `d0";
  instr_list.Append(new assem::MoveInstr(assem, new temp::TempList({result_temp}), new temp::TempList({left_temp})));
  switch (op_){
    case PLUS_OP:
    {
      assem = "addq `s0, `d0";
      instr_list.Append(new assem::OperInstr(assem, new temp::TempList(result_temp), new temp::TempList({right_temp, result_temp}), nullptr));
      break;
    }
    case MINUS_OP:
    {
      assem = "subq `s0, `d0";
      instr_list.Append(new assem::OperInstr(assem, new temp::TempList(result_temp), new temp::TempList({right_temp, result_temp}), nullptr));
      break;
    }
    case MUL_OP:
    {
      assem = "movq `s0, `d0";
      instr_list.Append(new assem::MoveInstr(assem, new temp::TempList({reg_manager->GetRegister(0)}), new temp::TempList({result_temp})));
      assem = "imulq `s0";
      instr_list.Append(new assem::OperInstr(assem, new temp::TempList({reg_manager->GetRegister(0), reg_manager->GetRegister(3)}), new temp::TempList({right_temp}), nullptr));
      assem = "movq `s0, `d0";
      instr_list.Append(new assem::MoveInstr(assem, new temp::TempList({result_temp}), new temp::TempList({reg_manager->GetRegister(0)})));
      break;
    }
    case DIV_OP:
    {
      assem = "movq `s0, `d0";
      instr_list.Append(new assem::MoveInstr(assem, new temp::TempList({reg_manager->GetRegister(0)}), new temp::TempList({result_temp})));
      assem = "cqto";
      instr_list.Append(new assem::OperInstr(assem, new temp::TempList({reg_manager->GetRegister(0), reg_manager->GetRegister(3)}), new temp::TempList({reg_manager->GetRegister(0)}), nullptr));
      assem = "idivq `s0";
      instr_list.Append(new assem::OperInstr(assem, new temp::TempList({reg_manager->GetRegister(0), reg_manager->GetRegister(3)}), new temp::TempList({right_temp}), nullptr));
      assem = "movq `s0, `d0";
      instr_list.Append(new assem::MoveInstr(assem, new temp::TempList({result_temp}), new temp::TempList({reg_manager->GetRegister(0)})));
      break;
    }
    case AND_OP:
    {
      temp::Label *trueL = temp::LabelFactory::NewLabel(); 
      temp::Label *falseL = temp::LabelFactory::NewLabel(); 
      auto trueLabel = new tree::LabelStm(trueL);
      auto falseLabel = new tree::LabelStm(falseL);
      std::vector<temp::Label *> *trueList = new std::vector<temp::Label *>{trueL};
      std::vector<temp::Label *> *falseList = new std::vector<temp::Label *>{falseL};
      assem::Targets *trueTarget = new assem::Targets(trueList);
      assem::Targets *falseTarget = new assem::Targets(falseList);
      assem = "cmpq $0, `s0";
      instr_list.Append(new assem::OperInstr(assem, nullptr, new temp::TempList({left_temp}), nullptr));
      assem = "je `j0";
      instr_list.Append(new assem::OperInstr(assem, nullptr, nullptr, falseTarget));
      assem = "cmpq $0, `s0";
      instr_list.Append(new assem::OperInstr(assem, nullptr, new temp::TempList({right_temp}), nullptr));
      assem = "je `j0";
      instr_list.Append(new assem::OperInstr(assem, nullptr, nullptr, falseTarget));
      assem = "movq $1, `s0";
      instr_list.Append(new assem::OperInstr(assem, nullptr, new temp::TempList({result_temp}), nullptr));
      assem = "jmp `j0";
      instr_list.Append(new assem::OperInstr(assem, nullptr, nullptr, trueTarget));
      falseLabel->Munch(instr_list, fs);
      assem = "movq $0, `s0";
      instr_list.Append(new assem::OperInstr(assem, nullptr, new temp::TempList({result_temp}), nullptr));
      trueLabel->Munch(instr_list, fs);
      break;
    }
    case OR_OP:
    {
      temp::Label *trueL = temp::LabelFactory::NewLabel(); 
      temp::Label *falseL = temp::LabelFactory::NewLabel(); 
      auto trueLabel = new tree::LabelStm(trueL);
      auto falseLabel = new tree::LabelStm(falseL);
      std::vector<temp::Label *> *trueList = new std::vector<temp::Label *>{trueL};
      std::vector<temp::Label *> *falseList = new std::vector<temp::Label *>{falseL};
      assem::Targets *trueTarget = new assem::Targets(trueList);
      assem::Targets *falseTarget = new assem::Targets(falseList);
      assem = "cmpq $0, `s0";
      instr_list.Append(new assem::OperInstr(assem, nullptr, new temp::TempList({left_temp}), nullptr));
      assem = "jne `j0";
      instr_list.Append(new assem::OperInstr(assem, nullptr, nullptr, trueTarget));
      assem = "cmpq $0, `s0";
      instr_list.Append(new assem::OperInstr(assem, nullptr, new temp::TempList({right_temp}), nullptr));
      assem = "jne `j0";
      instr_list.Append(new assem::OperInstr(assem, nullptr, nullptr, trueTarget));
      assem = "movq $0, `s0";
      instr_list.Append(new assem::OperInstr(assem, nullptr, new temp::TempList({result_temp}), nullptr));
      assem = "jmp `j0";
      instr_list.Append(new assem::OperInstr(assem, nullptr, nullptr, falseTarget));
      trueLabel->Munch(instr_list, fs);
      assem = "movq $1, `s0";
      instr_list.Append(new assem::OperInstr(assem, nullptr, new temp::TempList({result_temp}), nullptr));
      falseLabel->Munch(instr_list, fs);
      break;
    }
    default:
      break;
  }
  return result_temp;
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *resReg = temp::TempFactory::NewTemp();
  auto res = exp_->Munch(instr_list, fs);
  instr_list.Append(new assem::OperInstr("movq (`s0), `d0", new temp::TempList(resReg), new temp::TempList(res), nullptr));
  return resReg;
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  if(temp_ != reg_manager->FramePointer()) {
    return temp_;
  }
  temp::Temp* reg = temp::TempFactory::NewTemp();
  std::stringstream stream;
  stream << "leaq " << fs << "(`s0), `d0";
  std::string assem = stream.str();
  instr_list.Append(new assem::OperInstr(assem, new temp::TempList(reg), new temp::TempList(reg_manager->StackPointer()), nullptr));
  return reg;
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  stm_->Munch(instr_list, fs);
  return exp_->Munch(instr_list, fs);
}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  auto string_address = temp::TempFactory::NewTemp();
  std::ostringstream assem;
  assem << "leaq " << name_->Name() << "(%rip),`d0";
  instr_list.Append(new assem::OperInstr(assem.str(),new temp::TempList(string_address),new temp::TempList(),
                                        nullptr)
                    );
  return string_address;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *ret = temp::TempFactory::NewTemp();
  std::string assem = "movq $" + std::to_string(consti_) + ", `d0";
  instr_list.Append(
      new assem::MoveInstr(assem, new temp::TempList({ret}), nullptr));
  return ret;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *result = temp::TempFactory::NewTemp();
  std::list<Exp *> argsList = args_->GetNonConstList();
  tree::Exp *static_link = argsList.front();
  args_->PopStaticLink();
  temp::Temp *staticlink_reg = static_link->Munch(instr_list, fs);
  auto src_arglist = args_->MunchArgs(instr_list, fs);
  std::ostringstream assem1;
  assem1 << "subq $" << std::to_string(8) << ", %rsp";
  instr_list.Append(new assem::OperInstr(assem1.str(), nullptr, nullptr, nullptr));
  std::ostringstream assem2;
  assem2 << "movq `s0, (%rsp)";
  instr_list.Append(new assem::OperInstr(assem2.str(), nullptr, new temp::TempList({staticlink_reg}), nullptr));
  std::ostringstream assem3;
  assem3 << "callq " << static_cast<tree::NameExp*>(fun_)->name_->Name();
  instr_list.Append(new assem::OperInstr(assem3.str(), reg_manager->ArgRegs(), src_arglist, nullptr));
  std::ostringstream assem4;
  assem4 << "movq `s0, `d0";
  instr_list.Append(new assem::MoveInstr(assem4.str(), new temp::TempList({result}), new temp::TempList({reg_manager->ReturnValue()})));
  int offset = 8;
  if(args_->GetList().size() > 6) {
    offset += (args_->GetList().size() - 6) * 8;
  }
  std::ostringstream assem5;
  assem5 << "addq $" << std::to_string(offset) << ", %rsp";
  instr_list.Append(new assem::OperInstr(assem5.str(), nullptr, nullptr, nullptr));
  args_->Insert(static_link);
  
  return result;
}

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list,
                                   std::string_view fs) {
  temp::TempList *arg_regs = reg_manager->ArgRegs();
  temp::TempList *list = new temp::TempList();
  temp::Temp *rsp = reg_manager->StackPointer();
  temp::Temp *ret = temp::TempFactory::NewTemp();
  int position = 0;
  std::list<Exp *>::iterator it = exp_list_.begin();
  for (; it != exp_list_.end(); it++) {
    if (position >= 6) break;
    temp::Temp *arg = (*it)->Munch(instr_list, fs);
    list->Append(arg);
    instr_list.Append(new assem::MoveInstr(
        "movq `s0, `d0", new temp::TempList({arg_regs->NthTemp(position)}),
        new temp::TempList({arg})));
    position++;
  }
  std::vector<Exp *> tmp_list;
  for (; it != exp_list_.end(); it++)
    tmp_list.push_back(*it);
  int size = tmp_list.size();
  for (int i = size - 1; i >= 0; i--) {
    int word_size = reg_manager->WordSize();
    temp::Temp *arg = tmp_list[i]->Munch(instr_list, fs);
    std::string assem = "subq $" + std::to_string(word_size) + ", `d0";
    instr_list.Append(new assem::OperInstr(assem, new temp::TempList({rsp}),
                                           nullptr, nullptr));
    instr_list.Append(new assem::MoveInstr("movq `s0, (`d0)",
                                           new temp::TempList({rsp}),
                                           new temp::TempList({arg})));
  }
  return list;
}

void ExpList::PopStaticLink() {
  exp_list_.pop_front();
  
}

} // namespace tree
