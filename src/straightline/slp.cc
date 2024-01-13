#include "straightline/slp.h"

#include <iostream>

namespace A {
int A::CompoundStm::MaxArgs() const {
  int left = stm1->MaxArgs();
  int right = stm2->MaxArgs();
  return left > right ? left : right;
  // TODO: put your code here (lab1).
}

Table *A::CompoundStm::Interp(Table *t) const {
  t = stm1->Interp(t);
  return stm2->Interp(t);
  // TODO: put your code here (lab1).
}

int A::AssignStm::MaxArgs() const {
  return exp->MaxArgs();
  // TODO: put your code here (lab1).
}

Table *A::AssignStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  IntAndTable *newIntAndTable = exp->Interp(t);
  Table *newTable = newIntAndTable->t;
  return newTable->Update(id, newIntAndTable->i);
}

int A::PrintStm::MaxArgs() const {
  return exps->MaxArgs();
  // TODO: put your code here (lab1).
}

Table *A::PrintStm::Interp(Table *t) const {
  IntAndTable *newIntAndTable = exps->Interp(t);
  return newIntAndTable->t;
  // TODO: put your code here (lab1).
}


int Table::Lookup(const std::string &key) const {
  if (id == key) {
    return value;
  } else if (tail != nullptr) {
    return tail->Lookup(key);
  } else {
    assert(false);
  }
}

Table *Table::Update(const std::string &key, int val) const {
  return new Table(key, val, this);
}
}  // namespace A

int A::IdExp::MaxArgs() const{
  return 0;
}

int A::NumExp::MaxArgs() const{
  return 0;
}

int A::OpExp::MaxArgs() const{
  return left->MaxArgs() > right->MaxArgs() ? left->MaxArgs() : right->MaxArgs();
}

int A::EseqExp::MaxArgs() const{
  return stm->MaxArgs() > exp->MaxArgs() ? stm->MaxArgs() : exp->MaxArgs();
}

int A::PairExpList::MaxArgs() const{
  int maxArgs = exp->MaxArgs() > tail->MaxArgs() ? exp->MaxArgs() : tail->MaxArgs();
  int maxExps = this->NumExps();
  return maxArgs > maxExps ? maxArgs : maxExps;
}

int A::PairExpList::NumExps() const{
  return tail->NumExps() + 1;
}

int A::LastExpList::MaxArgs() const{
  return 0;
}

int A::LastExpList::NumExps() const{
  return 1;
}

A::IntAndTable *A::IdExp::Interp(Table *t) const{
  return new IntAndTable(t->Lookup(id), t);
}

A::IntAndTable *A::NumExp::Interp(Table *t) const{
  return new IntAndTable(num, t);
}

A::IntAndTable *A::OpExp::Interp(Table *t) const{
  IntAndTable *leftT = left->Interp(t);
  IntAndTable *rightT = right->Interp(leftT->t);
  switch(oper){
    case PLUS:
      return new IntAndTable(leftT->i + rightT->i, rightT->t);
    case MINUS:
      return new IntAndTable(leftT->i - rightT->i, rightT->t);
    case TIMES:
      return new IntAndTable(leftT->i * rightT->i, rightT->t);
    case DIV:
      return new IntAndTable(leftT->i / rightT->i, rightT->t);
  }
}

A::IntAndTable *A::EseqExp::Interp(Table *t) const{
  Table *t1 = stm->Interp(t);
  return exp->Interp(t1);
}

A::IntAndTable *A::PairExpList::Interp(Table *t) const{
  IntAndTable *intandTable1 = exp->Interp(t);
  std::cout << intandTable1->i;
  IntAndTable *intandTable2 = tail->Interp(intandTable1->t);
  return intandTable2;
}

A::IntAndTable *A::LastExpList::Interp(Table *t) const{
  std::cout << exp->Interp(t)->i << std::endl;
  return exp->Interp(t);
}