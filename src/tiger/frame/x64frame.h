//
// Created by wzl on 2021/10/12.
//

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include "tiger/frame/frame.h"

namespace frame {
class InFrameAccess : public Access {
public:
  int offset;

  explicit InFrameAccess(int offset) : offset(offset) {}
  /* TODO: Put your lab5 code here */
    tree::Exp *ToExp(tree::Exp *framePtr) const override {
      tree::BinopExp *exp =
          new tree::BinopExp(tree::PLUS_OP, framePtr, new tree::ConstExp(offset));
      return new tree::MemExp(exp);
    }
};


class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}
  /* TODO: Put your lab5 code here */
  tree::Exp *ToExp(tree::Exp *framePtr) const override {
    return new tree::TempExp(reg);
  }
};

class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */
public:
  X64Frame(temp::Label *name, std::list<bool> formals);

  void newFrame(std::list<bool> formals) override;

  Access* AllocLocal(bool escape) override;

  std::string GetLabel() override;
};

class X64RegManager : public RegManager {
public:
   X64RegManager(){
    temp::Temp *rax = temp::TempFactory::NewTemp();
    temp::Temp *rbx = temp::TempFactory::NewTemp();
    temp::Temp *rcx = temp::TempFactory::NewTemp();
    temp::Temp *rdx = temp::TempFactory::NewTemp();
    temp::Temp *rsi = temp::TempFactory::NewTemp();
    temp::Temp *rdi = temp::TempFactory::NewTemp();
    temp::Temp *rbp = temp::TempFactory::NewTemp();
    temp::Temp *rsp = temp::TempFactory::NewTemp();
    temp::Temp *r8 = temp::TempFactory::NewTemp();
    temp::Temp *r9 = temp::TempFactory::NewTemp();
    temp::Temp *r10 = temp::TempFactory::NewTemp();
    temp::Temp *r11 = temp::TempFactory::NewTemp();
    temp::Temp *r12 = temp::TempFactory::NewTemp();
    temp::Temp *r13 = temp::TempFactory::NewTemp();
    temp::Temp *r14 = temp::TempFactory::NewTemp();
    temp::Temp *r15 = temp::TempFactory::NewTemp();

    temp::Map::Name()->Enter(rax, new std::string("%rax"));
    temp::Map::Name()->Enter(rbx, new std::string("%rbx"));
    temp::Map::Name()->Enter(rcx, new std::string("%rcx"));
    temp::Map::Name()->Enter(rdx, new std::string("%rdx"));
    temp::Map::Name()->Enter(rbp, new std::string("%rbp"));
    temp::Map::Name()->Enter(rsp, new std::string("%rsp"));
    temp::Map::Name()->Enter(rdi, new std::string("%rdi"));
    temp::Map::Name()->Enter(rsi, new std::string("%rsi"));
    temp::Map::Name()->Enter(r8, new std::string("%r8"));
    temp::Map::Name()->Enter(r9, new std::string("%r9"));
    temp::Map::Name()->Enter(r10, new std::string("%r10"));
    temp::Map::Name()->Enter(r11, new std::string("%r11"));
    temp::Map::Name()->Enter(r12, new std::string("%r12"));
    temp::Map::Name()->Enter(r13, new std::string("%r13"));
    temp::Map::Name()->Enter(r14, new std::string("%r14"));
    temp::Map::Name()->Enter(r15, new std::string("%r15"));

    regs_ = {rax, rbx, rcx, rdx, rsi, rdi, rbp, rsp,
             r8,  r9,  r10, r11, r12, r13, r14, r15};
   }
  /* TODO: Put your lab5 code here */
  temp::TempList *Registers() override;
  temp::TempList *ArgRegs() override;
  temp::TempList *CallerSaves() override;
  temp::TempList *CalleeSaves() override;
  temp::TempList *ReturnSink() override;
  int WordSize() override;
  temp::Temp *FramePointer() override;
  temp::Temp *StackPointer() override;
  temp::Temp *ReturnValue() override;
};

} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
