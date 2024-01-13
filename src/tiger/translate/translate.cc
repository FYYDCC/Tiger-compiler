#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>

#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/x64frame.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/frame.h"

extern frame::Frags *frags;
extern frame::RegManager *reg_manager;

namespace tr {

tree::Exp *ExternalCall(std::string s, tree::ExpList *args) {
  return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(s)),
                           args);
}

Access *Access::AllocLocal(Level *level, bool escape) {
  /* TODO: Put your lab5 code here */
  frame::Frame *frame = level->frame_;
  frame::Access *access = frame->AllocLocal(escape);
  auto result = new Access(level, access);
  return result;
}

class Cx {
public:
  PatchList trues_;
  PatchList falses_;
  tree::Stm *stm_;

  Cx(PatchList trues, PatchList falses, tree::Stm *stm)
      : trues_(trues), falses_(falses), stm_(stm) {}
};

class Exp {
public:
  [[nodiscard]] virtual tree::Exp *UnEx() = 0;
  [[nodiscard]] virtual tree::Stm *UnNx() = 0;
  [[nodiscard]] virtual Cx UnCx(err::ErrorMsg *errormsg) = 0;
};

class ExpAndTy {
public:
  tr::Exp *exp_;
  type::Ty *ty_;

  ExpAndTy(tr::Exp *exp, type::Ty *ty) : exp_(exp), ty_(ty) {}
};

class ExExp : public Exp {
public:
  tree::Exp *exp_;

  explicit ExExp(tree::Exp *exp) : exp_(exp) {}

  [[nodiscard]] tree::Exp *UnEx() override { 
    /* TODO: Put your lab5 code here */
    return exp_;
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(exp_);
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* TODO: Put your lab5 code here */
     tree::CjumpStm *stm = new tree::CjumpStm(
        tree::RelOp::EQ_OP, exp_, new tree::ConstExp(1), nullptr, nullptr);
    tr::PatchList trues(std::list<temp::Label **>{&stm->true_label_});
    tr::PatchList falses(std::list<temp::Label **>{&stm->false_label_});
    return Cx(trues, falses, stm);
  }
};

class NxExp : public Exp {
public:
  tree::Stm *stm_;

  explicit NxExp(tree::Stm *stm) : stm_(stm) {}

  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */
    return new tree::EseqExp(stm_, new tree::ConstExp(0));
  }
  [[nodiscard]] tree::Stm *UnNx() override { 
    /* TODO: Put your lab5 code here */
    return stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* TODO: Put your lab5 code here */
    PatchList trues;
    PatchList falses;
    errormsg->Error(0, "UnCx called in Nxexp");
    return Cx(trues, falses, nullptr);
  }
};

class CxExp : public Exp {
public:
  Cx cx_;

  CxExp(PatchList trues, PatchList falses, tree::Stm *stm)
      : cx_(trues, falses, stm) {}
  
  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */
    temp::Temp *tmp = temp::TempFactory::NewTemp();
    temp::Label *t = temp::LabelFactory::NewLabel();
    temp::Label *f = temp::LabelFactory::NewLabel();

    cx_.trues_.DoPatch(t);
    cx_.falses_.DoPatch(f);

    return new tree::EseqExp(
      new tree::MoveStm(new tree::TempExp(tmp), new tree::ConstExp(1)),
      new tree::EseqExp(cx_.stm_,
      new tree::EseqExp(new tree::LabelStm(f),
      new tree::EseqExp(new tree::MoveStm(new tree::TempExp(tmp), new tree::ConstExp(0)),
      new tree::EseqExp(new tree::LabelStm(t), new tree::TempExp(tmp)))))
    );
  }
  
  [[nodiscard]] tree::Stm *UnNx() override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(UnEx());
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override { 
    /* TODO: Put your lab5 code here */
    return cx_;
  }
};

void ProgTr::Translate() {
  /* TODO: Put your lab5 code here */
  FillBaseTEnv();
  FillBaseVEnv();

  tr::ExpAndTy *result = absyn_tree_->Translate(venv_.get(), tenv_.get(), main_level_.get(),
                         temp::LabelFactory::NamedLabel("tigermain"),
                         errormsg_.get());
  frags->PushBack(new frame::ProcFrag(result->exp_->UnNx(), main_level_->frame_));
}

tree::Exp *FramePtr(tr::Level *current, tr::Level *target) {
  tree::Exp *frame = new tree::TempExp(reg_manager->FramePointer());

  while(current != target) {
    frame = new tree::MemExp(new tree::BinopExp(tree::PLUS_OP, frame, new tree::ConstExp(reg_manager->WordSize())));
    current = current->parent_;
  }

  return frame;
}


} // namespace tr

namespace absyn {
tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return root_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  env::EnvEntry *entry = venv->Look(sym_);
  env::VarEntry *var_entry = dynamic_cast<env::VarEntry*>(entry); 
  tr::Access *access = var_entry->access_;
  tree::Exp *frame = FramePtr(level, access->level_);
  tr::Exp *exp = new tr::ExExp(access->access_->ToExp(frame));
  return new tr::ExpAndTy(exp, var_entry->ty_->ActualTy());
}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *var = var_->Translate(venv, tenv, level, label, errormsg);
  type::RecordTy *ty = dynamic_cast<type::RecordTy *>(var->ty_->ActualTy());
  if (ty && typeid(*ty) == typeid(type::RecordTy)){
    std::list<type::Field *> field_list =((type::RecordTy *)ty)->fields_->GetList();
    int offset = 0;
    for (auto field : field_list) {
      if (field->name_ == sym_) {
        tree::BinopExp *addr = new tree::BinopExp(tree::PLUS_OP, var->exp_->UnEx(),
                               new tree::ConstExp(offset * 8));
        tr::ExExp *exp = new tr::ExExp(new tree::MemExp(addr));
        return new tr::ExpAndTy(exp, field->ty_);
      }
      offset++;
    }
  }
  errormsg->Error(pos_, "FieldVar translate error");
  return new tr::ExpAndTy(nullptr, type::NilTy::Instance());
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *var = var_->Translate(venv, tenv, level, label, errormsg);
  type::Ty *varty = var->ty_->ActualTy();
  if(typeid(*varty) != typeid(type::ArrayTy)) {
    errormsg->Error(pos_, "subscript var should be array type\n");
    return new tr::ExpAndTy(nullptr, type::NilTy::Instance());
  }
  type::ArrayTy *ty = dynamic_cast<type::ArrayTy *>(var->ty_->ActualTy());
  tr::ExpAndTy *subscript_exp = subscript_->Translate(venv, tenv, level, label, errormsg);
  tree::Exp *offset = new tree::BinopExp(tree::MUL_OP, subscript_exp->exp_->UnEx(),
                                         new tree::ConstExp(reg_manager->WordSize()));
  tr::Exp *exp = new tr::ExExp(
      new tree::MemExp(new tree::BinopExp(tree::PLUS_OP, var->exp_->UnEx(), offset)));
  return new tr::ExpAndTy(exp, ty->ty_);
}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return var_->Translate(venv, tenv, level, label, errormsg);
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),
                          type::NilTy::Instance());
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(val_)),
                          type::IntTy::Instance());
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  temp::Label *label_ = temp::LabelFactory::NewLabel();
  frags->PushBack(new frame::StringFrag(label_, str_));
  return new tr::ExpAndTy(new tr::ExExp(new tree::NameExp(label_)),type::StringTy::Instance());
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  env::FunEntry *entry = static_cast<env::FunEntry*>(venv->Look(func_));
  tree::ExpList *expList = new tree::ExpList();
  std::list<absyn::Exp *> argList = args_->GetList();
  tree::Exp *staticLink = FramePtr(level, entry->level_->parent_);
  if(staticLink){
    expList->Append(staticLink);
  }
  for (auto arg : argList) {
    tr::ExpAndTy *expandty = arg->Translate(venv, tenv, level, label, errormsg);
    expList->Append(expandty->exp_->UnEx());
  }
  if(entry->level_->parent_){
    tr::Exp* exp = new tr::ExExp(new tree::CallExp(new tree::NameExp(func_), expList));
    if(entry->result_){
      return new tr::ExpAndTy(exp, entry->result_->ActualTy());
    }
    else{
      return new tr::ExpAndTy(exp, type::VoidTy::Instance());
    }
  }
  else{
    tree::Exp* exp = tr::ExternalCall(func_->Name(), expList);
    if(entry->result_){
      return new tr::ExpAndTy(new tr::ExExp(exp), entry->result_->ActualTy());
    }
    else{
      return new tr::ExpAndTy(new tr::ExExp(exp), type::VoidTy::Instance());
    }
  }
}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::Exp *exp = nullptr;
  tr::ExpAndTy *left = left_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *right = right_->Translate(venv, tenv, level, label, errormsg);
  switch(oper_){
    case absyn::PLUS_OP:
      if(typeid(*left->ty_->ActualTy()) != typeid(type::IntTy)) {
        errormsg->Error(pos_, "the left value in binop exp should be integer\n");
      }
      if(typeid(*right->ty_->ActualTy()) != typeid(type::IntTy)) {
        errormsg->Error(pos_, "the right value in binop exp should be integer\n");
      }
      exp = new tr::ExExp(new tree::BinopExp(tree::PLUS_OP, left->exp_->UnEx(),right->exp_->UnEx()));
      break;
    case absyn::MINUS_OP:
      if(typeid(*left->ty_->ActualTy()) != typeid(type::IntTy)) {
        errormsg->Error(pos_, "the left value should be integer in binop\n");
      }
      if(typeid(*right->ty_->ActualTy()) != typeid(type::IntTy)) {
        errormsg->Error(pos_, "the right value should be integer in binop\n");
      }
      exp = new tr::ExExp(new tree::BinopExp(tree::MINUS_OP, left->exp_->UnEx(),
                                           right->exp_->UnEx()));
      break;
    case absyn::TIMES_OP:
      if(typeid(*left->ty_->ActualTy()) != typeid(type::IntTy)) {
        errormsg->Error(pos_, "the left value should be integer in binop\n");
      }
      if(typeid(*right->ty_->ActualTy()) != typeid(type::IntTy)) {
        errormsg->Error(pos_, "the right value should be integer in binop\n");
      }
      exp = new tr::ExExp(new tree::BinopExp(tree::MUL_OP, left->exp_->UnEx(),
                                           right->exp_->UnEx()));
      break;
    case absyn::DIVIDE_OP:
      if(typeid(*left->ty_->ActualTy()) != typeid(type::IntTy)) {
        errormsg->Error(pos_, "the left value should be integer in binop\n");
      }
      if(typeid(*right->ty_->ActualTy()) != typeid(type::IntTy)) {
        errormsg->Error(pos_, "the right value should be integer in binop\n");
      }
      exp = new tr::ExExp(new tree::BinopExp(tree::DIV_OP, left->exp_->UnEx(),
                                           right->exp_->UnEx()));
      break;
    case absyn::AND_OP:
      if(typeid(*left->ty_->ActualTy()) != typeid(type::IntTy)) {
        errormsg->Error(pos_, "the left value should be integer in binop\n");
      }
      if(typeid(*right->ty_->ActualTy()) != typeid(type::IntTy)) {
        errormsg->Error(pos_, "the right value should be integer in binop\n");
      }
      exp = new tr::ExExp(new tree::BinopExp(tree::AND_OP, left->exp_->UnEx(), 
                                            right->exp_->UnEx()));
      break;
    case absyn::OR_OP:
      if(typeid(*left->ty_->ActualTy()) != typeid(type::IntTy)) {
        errormsg->Error(pos_, "the left value should be integer in binop\n");
      }
      if(typeid(*right->ty_->ActualTy()) != typeid(type::IntTy)) {
        errormsg->Error(pos_, "the right value should be integer in binop\n");
      }
      exp = new tr::ExExp(new tree::BinopExp(tree::OR_OP, left->exp_->UnEx(), 
                                            right->exp_->UnEx()));
      break;
    case absyn::EQ_OP:
    case absyn::NEQ_OP:
    {
      tr::PatchList *trues = new tr::PatchList(), *falses = new tr::PatchList();
      type::Ty *left_ty = left->ty_->ActualTy();
      type::Ty *right_ty = right->ty_->ActualTy();
      tree::CjumpStm *stm;

      if(typeid(*left_ty) != typeid(*right_ty) && typeid(*right_ty) != typeid(type::NilTy)) {
        errormsg->Error(pos_, "the left value and right value not match in cjump\n");
      }
      if (typeid(*left_ty) == typeid(type::StringTy) 
          && typeid(*right_ty) == typeid(type::StringTy))
      {
        auto expList = new tree::ExpList();
        auto staticLink = FramePtr(level, level);
        expList->Append(staticLink);
        expList->Append(left->exp_->UnEx());
        expList->Append(right->exp_->UnEx());
        auto name = tr::ExternalCall("string_equal", expList);
        tree::RelOp op;
        if (oper_ == absyn::EQ_OP)
          op = tree::EQ_OP;
        else
          op = tree::NE_OP;
        stm = new tree::CjumpStm(op, name, new tree::ConstExp(1), nullptr, nullptr);
      }
      else{
        tree::RelOp op;
        if (oper_ == absyn::EQ_OP)
          op = tree::EQ_OP;
        else
          op = tree::NE_OP;
        stm = new tree::CjumpStm(op, left->exp_->UnEx(), right->exp_->UnEx(), nullptr, nullptr);
      }   
      trues->AddPatch(&stm->true_label_);
      falses->AddPatch(&stm->false_label_);
      exp = new tr::CxExp(*trues, *falses, stm); 
      break;
    }
    case absyn::LT_OP:
    case absyn::LE_OP:
    case absyn::GT_OP:
    case absyn::GE_OP:
    {
      type::Ty *left_ty = left->ty_->ActualTy();
      type::Ty *right_ty = right->ty_->ActualTy();
      if(typeid(*left_ty) != typeid(*right_ty) && typeid(*right_ty) != typeid(type::NilTy)) {
        errormsg->Error(pos_, "the left value and right value not match in cjump\n");
      }
      tr::PatchList *trues = new tr::PatchList(), *falses = new tr::PatchList();
      tree::RelOp op;
      tree::CjumpStm *stm;
      if (oper_ == absyn::LT_OP)
        op = tree::LT_OP;
      else if (oper_ == absyn::LE_OP)
        op = tree::LE_OP;
      else if (oper_ == absyn::GT_OP)
        op = tree::GT_OP;
      else
        op = tree::GE_OP;
      stm = new tree::CjumpStm(op, left->exp_->UnEx(), right->exp_->UnEx(),
                             nullptr, nullptr);
      trues->AddPatch(&stm->true_label_);
      falses->AddPatch(&stm->false_label_);
      exp = new tr::CxExp(*trues, *falses, stm);
      break;
    }
    default:
      errormsg->Error(pos_, "unknown error in optranslate\n");
  }
  return new tr::ExpAndTy(exp, type::IntTy::Instance());
}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,      
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<absyn::EField *> fieldList = fields_->GetList();
  type::Ty *ty = tenv->Look(typ_);
  ty = ty->ActualTy();
  if(typeid(*ty) != typeid(type::RecordTy)) {
    errormsg->Error(pos_, "not a record type");
    ty = type::IntTy::Instance();
  }
  auto expList = new tree::ExpList();
  for(auto field : fieldList){
    expList->Append(field->exp_->Translate(venv, tenv, level, label, errormsg)->exp_->UnEx());
  }
  temp::Temp *tmp = temp::TempFactory::NewTemp();
  auto list = new tree::ExpList();
  auto staticLink = FramePtr(level, level);
  list->Append(staticLink);
  list->Append(new tree::ConstExp(reg_manager->WordSize() * fieldList.size()));
  tree::Stm *stm = new tree::MoveStm(new tree::TempExp(tmp), 
                                    tr::ExternalCall("alloc_record", list));
  
  int i = 0;
  std::list<tree::Exp*> heapList = expList->GetList();
  for(auto exp : heapList) {
    tree::Exp *move = new tree::MemExp(new tree::BinopExp(tree::PLUS_OP, new tree::TempExp(tmp), 
                                                        new tree::ConstExp(i * 8)));
    tree::Stm *moveStm = new tree::MoveStm(move, exp);
    stm = new tree::SeqStm(stm, moveStm);
    ++i;
  }  
  return new tr::ExpAndTy(new tr::ExExp
                          (new tree::EseqExp(stm, new tree::TempExp(tmp))),
                         ty);
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<absyn::Exp *> expList = seq_->GetList();
  tr::Exp *seqExp = new tr::ExExp(new tree::ConstExp(0));
  type::Ty *ty = type::IntTy::Instance();
  for (auto exp : expList) {
    tr::ExpAndTy *expandty = exp->Translate(venv, tenv, level, label, errormsg);
    ty = expandty->ty_;
    if (!expandty->exp_) {
      seqExp = new tr::ExExp(
          new tree::EseqExp(seqExp->UnNx(), new tree::ConstExp(0)));
    } else {
      seqExp =
          new tr::ExExp(new tree::EseqExp(seqExp->UnNx(), expandty->exp_->UnEx()));
    }
  }
  return new tr::ExpAndTy(seqExp, ty);
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,                       
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *var = var_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *exp = exp_->Translate(venv, tenv, level, label, errormsg);
  tree::MoveStm *stm = new tree::MoveStm(var->exp_->UnEx(), exp->exp_->UnEx());
  return new tr::ExpAndTy(new tr::NxExp(stm), type::VoidTy::Instance());
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *test = test_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *then = then_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *theElse = nullptr;
  if (elsee_) {
    theElse = elsee_->Translate(venv, tenv, level, label, errormsg);
  }
  temp::Temp *r = temp::TempFactory::NewTemp(); 
  temp::Label *t = temp::LabelFactory::NewLabel(); 
  temp::Label *f = temp::LabelFactory::NewLabel(); 
  temp::Label *done = temp::LabelFactory::NewLabel();

  tr::Cx cx = test->exp_->UnCx(errormsg);
  cx.trues_.DoPatch(t);
  cx.falses_.DoPatch(f);

  auto labelList = new std::vector<temp::Label*>();
  labelList->emplace_back(done);
  tr::Exp *exp = nullptr;
  if(theElse) {
    exp = new tr::ExExp(
            new tree::EseqExp(cx.stm_,
            new tree::EseqExp(new tree::LabelStm(t),
            new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r), then->exp_->UnEx()), 
            new tree::EseqExp(new tree::JumpStm(new tree::NameExp(done), labelList),
            new tree::EseqExp(new tree::LabelStm(f), 
            new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r), theElse->exp_->UnEx()),
            new tree::EseqExp(new tree::JumpStm(new tree::NameExp(done), labelList),
            new tree::EseqExp(new tree::LabelStm(done),
            new tree::TempExp(r))))))))) 
          );
    return new tr::ExpAndTy(exp, then->ty_);
  } else {
    exp = new tr::NxExp(
            new tree::SeqStm(cx.stm_,
              new tree::SeqStm(new tree::LabelStm(t),
                new tree::SeqStm(then->exp_->UnNx(),
                  new tree::LabelStm(f))))
          );
    return new tr::ExpAndTy(exp, type::VoidTy::Instance());
  }
}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,            
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  temp::Label *bodyLabel = temp::LabelFactory::NewLabel();
  temp::Label *testLabel = temp::LabelFactory::NewLabel();
  temp::Label *doneLabel = temp::LabelFactory::NewLabel();
  tr::ExpAndTy *test = test_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *body = body_->Translate(venv, tenv, level, doneLabel, errormsg);
  if(typeid(*test->ty_) == typeid(type::IntTy) 
    && typeid(*body->ty_) == typeid(type::VoidTy)){
    tr::Cx testCx = test->exp_->UnCx(errormsg);
    testCx.trues_.DoPatch(bodyLabel);
    testCx.falses_.DoPatch(doneLabel);
    auto labelList = new std::vector<temp::Label*>();
    labelList->emplace_back(testLabel);
    tr::Exp *exp = new tr::NxExp(
          new tree::SeqStm(new tree::LabelStm(testLabel),
            new tree::SeqStm(testCx.stm_,
              new tree::SeqStm(new tree::LabelStm(bodyLabel),
                new tree::SeqStm(body->exp_->UnNx(),
                  new tree::SeqStm(new tree::JumpStm(new tree::NameExp(testLabel), labelList),
                    new tree::LabelStm(doneLabel))))))
        );
    return new tr::ExpAndTy(exp, type::VoidTy::Instance());
  }
  errormsg->Error(pos_, "int in test and while need void");
  return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *lo = lo_->Translate(venv, tenv, level, label, errormsg);
  tr::ExpAndTy *hi = hi_->Translate(venv, tenv, level, label, errormsg);
  venv->BeginScope();
  tr::Access *access = tr::Access::AllocLocal(level, this->escape_);
  env::EnvEntry *new_var_entry = new env::VarEntry(access, lo->ty_, true);
  venv->Enter(var_, new_var_entry);
  tr::Exp* limit = new tr::ExExp(new tree::TempExp(temp::TempFactory::NewTemp()));
  auto regaccess = (static_cast<frame::InRegAccess*>(access->access_));
  tr::Exp *loop = new tr::ExExp(new tree::TempExp
  (regaccess->reg));

  temp::Label *doneLabel = temp::LabelFactory::NewLabel();
  temp::Label *bodyLabel = temp::LabelFactory::NewLabel();
  temp::Label *incrLabel = temp::LabelFactory::NewLabel();
  tr::ExpAndTy *body = body_->Translate(venv, tenv, level, doneLabel, errormsg);
  tree::Stm *loopStm = new tree::MoveStm(loop->UnEx(), lo->exp_->UnEx());
  tree::Stm *limitStm = new tree::MoveStm(limit->UnEx(), hi->exp_->UnEx());
  tree::Stm *LEstm = new tree::CjumpStm(tree::LE_OP, loop->UnEx(), limit->UnEx(), bodyLabel, doneLabel);
  tree::Stm *increaseStm = new tree::MoveStm(loop->UnEx(), 
    new tree::BinopExp(tree::PLUS_OP, loop->UnEx(), new tree::ConstExp(1)));
  auto labelList = new std::vector<temp::Label*>();
  labelList->push_back(bodyLabel);
  tree::Stm *testStm = new tree::SeqStm(new tree::CjumpStm(tree::LT_OP, loop->UnEx(), limit->UnEx(), incrLabel, doneLabel),
                          new tree::SeqStm(new tree::LabelStm(incrLabel),
                            new tree::SeqStm(increaseStm,
                              new tree::JumpStm(new tree::NameExp(bodyLabel), labelList)))
                        );

  tree::Stm *stm = new tree::SeqStm(loopStm,
                    new tree::SeqStm(limitStm,
                      new tree::SeqStm(LEstm,
                        new tree::SeqStm(new tree::LabelStm(bodyLabel),
                          new tree::SeqStm(body->exp_->UnNx(),
                            new tree::SeqStm(testStm,
                              new tree::LabelStm(doneLabel))))))
                  );
  venv->EndScope();
  return new tr::ExpAndTy(new tr::NxExp(stm), type::VoidTy::Instance());
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::vector<temp::Label *> *labelList = new std::vector<temp::Label*>();
  labelList->push_back(label);
  tr::Exp *exp = new tr::NxExp(new tree::JumpStm(new tree::NameExp(label), labelList));
  return new tr::ExpAndTy(exp, type::VoidTy::Instance());
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  static bool is_main = true;
  bool enter = false;
  if (is_main) {
    is_main = false;
    enter = true;
  }
  venv->BeginScope();
  tenv->BeginScope();
  std::list<absyn::Dec *> decList = decs_->GetList();
  int i = 1;
  tree::Stm *stm = nullptr;
  tree::Exp *exp = nullptr;
  for(auto dec : decList) {
    if(i) {
      stm = dec->Translate(venv, tenv, level, label, errormsg)->UnNx();
      i--;
    } else {
      stm = new tree::SeqStm(stm, dec->Translate(venv, tenv, level, label, errormsg)->UnNx());
    }
  }
  tr::ExpAndTy *body = body_->Translate(venv, tenv, level, label, errormsg);

  venv->EndScope();
  tenv->EndScope();
  if(!stm) {
    exp = body->exp_->UnEx();
  } else {
    exp = new tree::EseqExp(stm, body->exp_->UnEx());
  }
  stm = new tree::ExpStm(exp);
  return new tr::ExpAndTy(new tr::ExExp(exp), body->ty_->ActualTy());

  frags->PushBack(new frame::ProcFrag(stm, level->frame_));
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,                    
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty *ty = tenv->Look(typ_)->ActualTy();
  if(typeid(*ty) != typeid(type::ArrayTy)){
    errormsg->Error(pos_, "array exp need type array %s\n", typ_->Name());
  } 
  tree::Exp *size =
      size_->Translate(venv, tenv, level, label, errormsg)->exp_->UnEx();
  tree::Exp *init =
      init_->Translate(venv, tenv, level, label, errormsg)->exp_->UnEx();
  auto args = new tree::ExpList();
  auto staticLink = FramePtr(level, level);
  args->Append(staticLink);
  args->Append(size);
  args->Append(init);
  tree::Exp *exp = tr::ExternalCall("init_array", args);
  return new tr::ExpAndTy(new tr::ExExp(exp), ty);
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(nullptr, type::VoidTy::Instance());
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<FunDec *> fundecList = functions_->GetList();
  for(auto fundec : fundecList){
    std::list<bool> escapeList;
    std::list<Field *> fieldList = fundec->params_->GetList();
    for (auto field : fieldList) {
      escapeList.push_back(field->escape_);
    }
    temp::Label *nameLabel = temp::LabelFactory::NamedLabel(fundec->name_->Name());
    tr::Level *funLevel = tr::Level::newLevel(nameLabel, escapeList, level);
    type::Ty *result = type::VoidTy::Instance();
    if (fundec->result_) {
      result = tenv->Look(fundec->result_);
    }
    type::TyList *formalList = fundec->params_->MakeFormalTyList(tenv, errormsg);
    venv->Enter(fundec->name_, new env::FunEntry(funLevel, nameLabel, formalList, result));
  }

  for(auto fundec : fundecList){
    venv->BeginScope();
    tenv->BeginScope();
    env::FunEntry *funcEntry = static_cast<env::FunEntry*>(venv->Look(fundec->name_));
    std::list<type::Ty *> formalList= fundec->params_->MakeFormalTyList(tenv, errormsg)->GetList();
    std::list<Field *> fieldList = fundec->params_->GetList();
    std::list<frame::Access *> formals = funcEntry->level_->frame_->formals_;

    auto formallistItr = formalList.begin();
    auto formalsItr = formals.begin();
    if(formalList.size() < formals.size()) {
      errormsg->Error(pos_, "has static link\n");
      ++formalsItr;
    }
    for(auto param : fieldList) {
      venv->Enter(param->name_
                ,new env::VarEntry(new tr::Access(funcEntry->level_, *formalsItr), *formallistItr));
      ++formalsItr;
      ++formallistItr;
    }
    tr::ExpAndTy *body = fundec->body_->Translate(venv, tenv, funcEntry->level_, funcEntry->label_, errormsg);
    venv->EndScope();
    tenv->EndScope();

    frags->PushBack(
      new frame::ProcFrag(
        frame::ProcEntryExit1(funcEntry->level_->frame_,
              new tree::MoveStm(new tree::TempExp(reg_manager->ReturnValue()),
              body->exp_->UnEx())),
        funcEntry->level_->frame_));
  }
  return new tr::ExExp(new tree::ConstExp(0));
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *init = init_->Translate(venv, tenv, level, label, errormsg);
  tree::Exp *exp = init->exp_->UnEx();
  type::Ty *ty = init->ty_->ActualTy();
  tr::Access *access = tr::Access::AllocLocal(level, this->escape_);
  venv->Enter(this->var_, new env::VarEntry(access, ty));
  tree::Exp *frameExp = new tree::TempExp(reg_manager->FramePointer());
  return new tr::NxExp
    (new tree::MoveStm
      ((new tr::ExExp(access->access_->ToExp(frameExp)))->UnEx(), 
        exp));
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<absyn::NameAndTy *> typeList = types_->GetList();
  for (auto type : typeList) {
    tenv->Enter(type->name_, new type::NameTy(type->name_, nullptr));
  }
  for (auto &type : typeList) {
    type::NameTy *nameTy = static_cast<type::NameTy*>(tenv->Look(type->name_));
    nameTy->ty_ = type->ty_->Translate(tenv, errormsg);
  }
  return new tr::ExExp(new tree::ConstExp(0));
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty *ty = tenv->Look(name_);
  return new type::NameTy(name_, ty);
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::FieldList *fieldList = record_->MakeFieldList(tenv, errormsg);
  return new type::RecordTy(fieldList);
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new type::ArrayTy(tenv->Look(array_));
}

} // namespace absyn
