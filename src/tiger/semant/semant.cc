#include "tiger/absyn/absyn.h"
#include "tiger/semant/semant.h"
#include <unordered_set>

namespace absyn {

void AbsynTree::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  root_->SemAnalyze(venv, tenv, 0, errormsg);
}

type::Ty *SimpleVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  env::EnvEntry *entry = venv->Look(sym_);
  if (entry && typeid(*entry) == typeid(env::VarEntry)) {
    return (static_cast<env::VarEntry *>(entry))->ty_->ActualTy();
  } else {
    errormsg->Error(pos_, "undefined variable %s", sym_->Name().data());
  }
  return type::IntTy::Instance();
}

type::Ty *FieldVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  type::Ty *varTy = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (varTy && typeid(*varTy) == typeid(type::RecordTy)) {
    auto fieldList = (static_cast<type::RecordTy *>(varTy))->fields_->GetList();
    for (auto field : fieldList) 
    {
      if (field->name_ == sym_) {
        return field->ty_->ActualTy();
      }
    }
    errormsg->Error(pos_, "field %s doesn't exist", sym_->Name().data());
    return type::IntTy::Instance();
  }
  else{
    errormsg->Error(pos_, "not a record type");
    return type::IntTy::Instance();
  }
  /* TODO: Put your lab4 code here */
}

type::Ty *SubscriptVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *varTy = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (typeid(*varTy) == typeid(type::ArrayTy)){
    type::Ty *subTy = subscript_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if (!subTy->IsSameType(type::IntTy::Instance())) {
      errormsg->Error(pos_, "int type required");
      return type::IntTy::Instance();
    }
    return (static_cast<type::ArrayTy *>(varTy))->ty_->ActualTy();
  }
  else {
    errormsg->Error(pos_, "array type required");
    return type::IntTy::Instance();
  }
}

type::Ty *VarExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return var_->SemAnalyze(venv, tenv, labelcount, errormsg);
}

type::Ty *NilExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::NilTy::Instance();
}

type::Ty *IntExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::IntTy::Instance();
}

type::Ty *StringExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::StringTy::Instance();
}

type::Ty *CallExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  env::EnvEntry *entry = venv->Look(func_);
  if (entry && typeid(*entry) == typeid(env::FunEntry)) {
    env::FunEntry *funEntry = static_cast<env::FunEntry *>(entry);
    auto formalList = (static_cast<env::FunEntry *>(entry))->formals_->GetList();
    auto argList = args_->GetList();
    auto formalItr = formalList.begin(); 
    auto argItr = argList.begin();
    while(argItr != argList.end())
    {
      if(formalItr == formalList.end())
      {
        errormsg->Error(pos_, "too many params in function %s",
                        func_->Name().data());
        break;
      }
      type::Ty *argTy = (*argItr)->SemAnalyze(venv, tenv, labelcount, errormsg);
      if(!argTy->IsSameType(*formalItr)) {
        errormsg->Error((*argItr)->pos_, "para type mismatch");
        return type::IntTy::Instance();
      }
      formalItr++;
      argItr++;
    }
    return funEntry->result_;
  }
  else {
    errormsg->Error(pos_, "undefined function %s", func_->Name().data());
  }
  return type::IntTy::Instance();
}

type::Ty *OpExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *leftTy = left_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *rightTy = right_->SemAnalyze(venv, tenv, labelcount, errormsg);
  switch (oper_) {
    case PLUS_OP:
    case MINUS_OP:
    case TIMES_OP:
    case DIVIDE_OP:
    case absyn::AND_OP:
    case absyn::OR_OP:
      if (!rightTy->IsSameType(type::IntTy::Instance()) ||
          !leftTy->IsSameType(type::IntTy::Instance())) {
        errormsg->Error(pos_, "integer required");
      }
      break;
    case LT_OP:
    case LE_OP:
    case GT_OP:
    case GE_OP:
    case EQ_OP:
    case NEQ_OP:
      if (!leftTy->IsSameType(rightTy)) {
        errormsg->Error(pos_, "same type required");
      }
      break;
    default:
      break;
  }
  return type::IntTy::Instance();
}

type::Ty *RecordExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *ty = tenv->Look(typ_);
  type::Ty *ret = ty;
  if (ty && typeid(*(ty->ActualTy())) == typeid(type::RecordTy))
  {
    ty = ty->ActualTy();
    auto recFieldlist = (static_cast<type::RecordTy *>(ty))->fields_->GetList();
    auto Fieldlist = this->fields_->GetList();
    auto recItr = recFieldlist.begin();
    auto FieItr = Fieldlist.begin();
    while(recItr != recFieldlist.end())
    {
      if(FieItr == Fieldlist.end())
      {
        errormsg->Error(pos_, "field list too short");
        break;
        return type::IntTy::Instance();
      }
      type::Ty* fieldty = (*FieItr)->exp_->SemAnalyze(venv, tenv, labelcount, errormsg);
      if((*recItr)->name_ != (*FieItr)->name_)
      {
        errormsg->Error(pos_, "record exp arg name not match");
        return type::IntTy::Instance();
      }
      if (!(*recItr)->ty_->IsSameType(fieldty)) 
      {
        errormsg->Error(pos_, "record exp arg type not match");
        return type::IntTy::Instance();
      }
      ++recItr;
      ++FieItr;
    }
    if(recItr != recFieldlist.end())
    {
      errormsg->Error(pos_, "field list too long");
      return type::IntTy::Instance();
    }
  }
  else {
    errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
    return type::IntTy::Instance();
  }
  return ret;
}

type::Ty *SeqExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto expList = seq_->GetList();
  type::Ty *ty;
  for (Exp* exp : expList)
    ty = exp->SemAnalyze(venv, tenv, labelcount, errormsg);
  return ty;
}

type::Ty *AssignExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *varTy = var_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *expTy = exp_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (typeid(*varTy) == typeid(SimpleVar))
  {
    SimpleVar *simVar = static_cast<SimpleVar *>(var_);
    env::EnvEntry *entry = venv->Look(simVar->sym_);
    if (entry && entry->readonly_) {
      errormsg->Error(pos_, "loop variable can't be assigned");
      return type::IntTy::Instance();
    }
  }
  if(!varTy->IsSameType(expTy)) {
    errormsg->Error(exp_->pos_, "assign exp left and right unmatch");
    return type::IntTy::Instance();
  }
  return type::VoidTy::Instance();
}

type::Ty *IfExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* ty = test_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (typeid(*ty) != typeid(type::IntTy)) {
    errormsg->Error(test_->pos_, "integer required");
    return type::IntTy::Instance();
  }

  type::Ty* thenTy = then_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if (!elsee_) {
    if(typeid(*thenTy) != typeid(type::VoidTy))
    {
      errormsg->Error(then_->pos_, "if-then exp's body must produce no value");
      return type::IntTy::Instance();
    }
    return type::VoidTy::Instance();
  }
  else {
    type::Ty *elseTy = elsee_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if (!thenTy->IsSameType(elseTy)) {
      errormsg->Error(elsee_->pos_, "then exp and else exp type mismatch");
      return type::VoidTy::Instance();
    }
    else {
      return thenTy;
    }
  }
}

type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  venv->BeginScope();
  tenv->BeginScope();
  type::Ty* ty = test_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  if (typeid(*ty) != typeid(type::IntTy)) {
    errormsg->Error(test_->pos_, "condition must int");
  }
  type::Ty *bodyTy = body_->SemAnalyze(venv, tenv, labelcount + 1, errormsg);
  if(typeid(*bodyTy) != typeid(type::VoidTy)) {
    errormsg->Error(pos_, "while body must produce no value");
  }
  tenv->EndScope();
  venv->EndScope();
  return type::VoidTy::Instance();
}

type::Ty *ForExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  venv->BeginScope();
  tenv->BeginScope();
  type::Ty *loTy = lo_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *hiTy = hi_->SemAnalyze(venv, tenv, labelcount, errormsg);

  if (typeid(*loTy) != typeid(type::IntTy) || typeid(*hiTy) != typeid(type::IntTy)) {
    errormsg->Error(this->pos_, "for exp's range type is not integer");
  }
  
  env::EnvEntry *entry = venv->Look(var_);
  venv->Enter(this->var_, new env::VarEntry(type::IntTy::Instance(), true));
  type::Ty *bodyTy = body_->SemAnalyze(venv, tenv, labelcount + 1, errormsg);
  if (typeid(*bodyTy) != typeid(type::VoidTy)) {
    errormsg->Error(this->pos_, "for body must produce no value");
  }
  tenv->EndScope();
  venv->EndScope();
  return type::VoidTy::Instance();
}

type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if (labelcount == 0) {
    errormsg->Error(this->pos_, "break is not inside any loop");
  }
  return type::VoidTy::Instance();
}

type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  venv->BeginScope();
  tenv->BeginScope();
  for (Dec* dec : decs_->GetList())
  {
    dec->SemAnalyze(venv, tenv, labelcount, errormsg);
  }
  auto ret = body_->SemAnalyze(venv, tenv, labelcount, errormsg);
  tenv->EndScope();
  venv->EndScope();
  return ret;
}

type::Ty *ArrayExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *arrayType = tenv->Look(this->typ_)->ActualTy();
  if (arrayType == nullptr || typeid(*arrayType) != typeid(type::ArrayTy)) {
    errormsg->Error(this->pos_, "undefined type");
  }
  type::Ty *sizeTy = size_->SemAnalyze(venv, tenv, labelcount, errormsg);
  if(typeid(*sizeTy) != typeid(type::IntTy)) {
    errormsg->Error(pos_, "the array size type must be int");
  }
  type::Ty *elemTy = ((type::ArrayTy *) arrayType)->ty_->ActualTy();
  type::Ty *initTy = init_->SemAnalyze(venv, tenv, labelcount, errormsg)->ActualTy();
  if(!initTy->IsSameType(elemTy)) {
    errormsg->Error(init_->pos_, "type mismatch");
  }
  return arrayType;
}

type::Ty *VoidExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<FunDec *> fundec_list = this->functions_->GetList();
  for(FunDec *fundec : fundec_list)
  {
    type::Ty *resultTy = type::VoidTy::Instance();
    if (fundec->result_ != nullptr){
      resultTy = tenv->Look(fundec->result_);
    }
    if (venv->Look(fundec->name_) != nullptr) {
      errormsg->Error(this->pos_, "two functions have the same name");
      continue;
    }
    type::TyList *formalTyList = fundec->params_->MakeFormalTyList(tenv, errormsg);
    venv->Enter(fundec->name_, new env::FunEntry(formalTyList, resultTy));
  }
  for (FunDec *fundec : fundec_list)
  {
    venv->BeginScope();
    auto formalList = fundec->params_->MakeFieldList(tenv, errormsg)->GetList();
    for (type::Field* paramItr : formalList) {
      venv->Enter(paramItr->name_, new env::VarEntry(paramItr->ty_));
    }
    type::Ty *bodyTy = fundec->body_->SemAnalyze(venv, tenv, labelcount, errormsg);
    if (fundec->result_ == nullptr && !bodyTy->IsSameType(type::VoidTy::Instance())) {
      errormsg->Error(fundec->body_->pos_, "procedure returns value");
    }
    else if(fundec->result_ != nullptr && !tenv->Look(fundec->result_)->IsSameType(bodyTy)) {
      errormsg->Error(this->pos_, "the return value dismatches with result type!");
    }
    venv->EndScope();
  }
}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *initTy = this->init_->SemAnalyze(venv, tenv, labelcount, errormsg);
  type::Ty *ty;
  if (typ_) {
    ty = tenv->Look(typ_)->ActualTy();
    if (!ty) {
      errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
      return;
    }
    if(!ty->IsSameType(initTy->ActualTy())) {
      errormsg->Error(init_->pos_, "type mismatch");
    }
    else{
      venv->Enter(var_, new env::VarEntry(initTy));
    }
  }
  else {
    if (typeid(*initTy) == typeid(type::NilTy)) {
      errormsg->Error(pos_, "init should not be nil without type specified");
      return;
    }
    venv->Enter(var_, new env::VarEntry(initTy));
    return;
  }  
}

void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                         err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  auto typeList = types_->GetList();
  for (NameAndTy *nameAndty : typeList) {
    if (tenv->Look(nameAndty->name_) != nullptr) {
      errormsg->Error(this->pos_, "two types have the same name");
      continue;
    }
    tenv->Enter(nameAndty->name_, new type::NameTy(nameAndty->name_, nullptr));
  }
  for (NameAndTy *nameAndty : typeList) {
    type::Ty *ty = tenv->Look(nameAndty->name_);
    type::NameTy *nameTy = static_cast<type::NameTy*>(ty);
    nameTy->ty_ = nameAndty->ty_->SemAnalyze(tenv, errormsg);
  }
  for (NameAndTy *nameAndty : typeList) {
    type::Ty *cur1 = tenv->Look(nameAndty->name_);
    type::Ty *cur2 = cur1;
    while(typeid(*cur2) == typeid(type::NameTy)) {
      cur2 = static_cast<type::NameTy*>(cur2)->ty_;
      if(static_cast<type::NameTy*>(cur2)->sym_ == static_cast<type::NameTy*>(cur1)->sym_) {
        errormsg->Error(pos_, "illegal type cycle");
        return;
      }
    }
  }
}

type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty* type = tenv->Look(name_);
  if (type == nullptr) {
    errormsg->Error(this->pos_, "undefined type %s", this->name_);
    return type::VoidTy::Instance();
  }
  return type;
}

type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if(record_ == nullptr) {
    errormsg->Error(pos_, "the type field in record is empty");
    return type::VoidTy::Instance();
  }
  return new type::RecordTy(this->record_->MakeFieldList(tenv, errormsg));
}

type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *arrayTy = tenv->Look(this->array_);
  if (arrayTy == NULL) {
    errormsg->Error(this->pos_, "undefined type %s", this->array_);
    return type::VoidTy::Instance();
  }
  return new type::ArrayTy(arrayTy);
}

} // namespace absyn

namespace sem {

void ProgSem::SemAnalyze() {
  FillBaseVEnv();
  FillBaseTEnv();
  absyn_tree_->SemAnalyze(venv_.get(), tenv_.get(), errormsg_.get());
}

} // namespace tr
