/* Parser for gpsim
   Copyright (C) 2004 Scott Dattalo

This file is part of the libgpsim library of gpsim

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see 
<http://www.gnu.org/licenses/lgpl-2.1.html>.
*/

#include <string>

#if !defined(__OPERATOR_H__)
#define __OPERATOR_H__

#include "expr.h"

using namespace std;


class Operator : public Expression {
  
 public:  
  Operator(string newOpString)
    {
      opString = newOpString;
    }
  virtual ~Operator(void)
    {
    }
  string showOp()
    {
      return(opString);
    }

 private:
  string opString;
};


class BinaryOperator : public Operator {
  
 public:  
  BinaryOperator(string opString, Expression* leftExpr, Expression* rightExpr);
  virtual ~BinaryOperator();
  virtual Value* shortCircuit(Value* leftValue);
  virtual Value* applyOp(Value* leftValue, Value* rightValue)=0;
  virtual Value* evaluate();
  virtual Expression *getLeft();
  virtual Expression *getRight();

  string show();
  string showType();
  string toString();

 protected:
  Expression* leftExpr;
  Expression* rightExpr;
  Value* value;

};

class UnaryOperator : public Operator {
  
 public:  
  UnaryOperator(string opString, Expression* expr);
  virtual ~UnaryOperator();
  virtual Value* applyOp(Value* value)=0;

  virtual Value* evaluate();

  string show();
  string showType();
  string toString();

 protected:
  Expression*  expr;
  Value* value;

};

class ComparisonOperator : public BinaryOperator {
public:
  enum ComparisonTypes {
    eOpEq,
    eOpGe,
    eOpGt,
    eOpLe,
    eOpLt,
    eOpNe
  };
  ComparisonOperator(string opString, Expression*, Expression*);
  virtual ~ComparisonOperator();
  virtual Value* applyOp(Value* leftValue, Value* rightValue);

  bool less() { return bLess;}
  bool equal() { return bEqual;}
  bool greater() { return bGreater;}

  virtual ComparisonTypes isa()=0;
  virtual int set_break(ObjectBreakTypes bt=eBreakAny, ObjectActionTypes at=eActionHalt, Expression *expr=0);

protected:
  bool bLess;
  bool bEqual;
  bool bGreater;
};

//-----------------------------------------------------------------
class OpAbstractRange : public BinaryOperator {

public:
  OpAbstractRange(Expression *leftExpr, Expression *rightExpr);
  virtual ~OpAbstractRange();
  Value* applyOp(Value* leftValue, Value* rightValue);
};

//-----------------------------------------------------------------
class OpAdd : public BinaryOperator {

public:
  OpAdd(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpAdd();
  virtual Value* applyOp(Value* leftValue, Value* rightValue);

};

//-----------------------------------------------------------------
class OpAnd : public BinaryOperator {

public:
  OpAnd(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpAnd();
  Value* applyOp(Value* leftValue, Value* rightValue);
};

//-----------------------------------------------------------------
class OpDiv : public BinaryOperator {

public:
  OpDiv(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpDiv();
  Value* applyOp(Value* leftValue, Value* rightValue);
};

//-----------------------------------------------------------------
class OpEq : public ComparisonOperator {

public:
  OpEq(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpEq();
  ComparisonOperator::ComparisonTypes isa() {return ComparisonOperator::eOpEq;}
};

//-----------------------------------------------------------------
class OpGe : public ComparisonOperator {

public:
  OpGe(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpGe();
  ComparisonOperator::ComparisonTypes isa() {return ComparisonOperator::eOpGe;}
};

//-----------------------------------------------------------------
class OpGt : public ComparisonOperator {

public:
  OpGt(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpGt();
  ComparisonOperator::ComparisonTypes isa() {return ComparisonOperator::eOpGt;}
};

//-----------------------------------------------------------------
class OpLe : public ComparisonOperator {

public:
  OpLe(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpLe();
  ComparisonOperator::ComparisonTypes isa() {return ComparisonOperator::eOpLe;}
};

//-----------------------------------------------------------------
class OpLogicalAnd : public BinaryOperator {

public:
  OpLogicalAnd(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpLogicalAnd();
  Value* applyOp(Value* leftValue, Value* rightValue);
};

//-----------------------------------------------------------------
class OpLogicalOr : public BinaryOperator {

public:
  OpLogicalOr(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpLogicalOr();
  Value* applyOp(Value* leftValue, Value* rightValue);
};

//-----------------------------------------------------------------
class OpLt : public ComparisonOperator {

public:
  OpLt(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpLt();
  ComparisonOperator::ComparisonTypes isa() {return ComparisonOperator::eOpLt;}
};

//-----------------------------------------------------------------
class OpMpy : public BinaryOperator {

public:
  OpMpy(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpMpy();
  Value* applyOp(Value* leftValue, Value* rightValue);
};

//-----------------------------------------------------------------
class OpNe : public ComparisonOperator {

public:
  OpNe(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpNe();
  ComparisonOperator::ComparisonTypes isa() {return ComparisonOperator::eOpNe;}
};

//-----------------------------------------------------------------
class OpOr : public BinaryOperator {

public:
  OpOr(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpOr();
  Value* applyOp(Value* leftValue, Value* rightValue);
};

//-----------------------------------------------------------------
class OpShl : public BinaryOperator {

public:
  OpShl(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpShl();
  Value* applyOp(Value* leftValue, Value* rightValue);
};

//-----------------------------------------------------------------
class OpShr : public BinaryOperator {

public:
  OpShr(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpShr();
  Value* applyOp(Value* leftValue, Value* rightValue);
};

//-----------------------------------------------------------------
class OpSub : public BinaryOperator {

public:
  OpSub(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpSub();
  Value* applyOp(Value* leftValue, Value* rightValue);
};

//-----------------------------------------------------------------
class OpXor : public BinaryOperator {

public:
  OpXor(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpXor();
  Value* applyOp(Value* leftValue, Value* rightValue);
};

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//            Unary objects
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

// -----------------------------------------------------------------
class OpLogicalNot : public UnaryOperator {

public:
  OpLogicalNot(Expression* expr);
  virtual ~OpLogicalNot();
  Value* applyOp(Value* value);
};

// -----------------------------------------------------------------
class OpNegate : public UnaryOperator {

public:
  OpNegate(Expression* expr);
  virtual ~OpNegate();
  Value* applyOp(Value* value);
};

// -----------------------------------------------------------------
class OpOnescomp : public UnaryOperator {

public:
  OpOnescomp(Expression* expr);
  virtual ~OpOnescomp();
  Value* applyOp(Value* value);
};

// -----------------------------------------------------------------
class OpPlus : public UnaryOperator {

public:
  OpPlus(Expression* expr);
  virtual ~OpPlus();
  Value* applyOp(Value* value);
};

// -----------------------------------------------------------------
class OpIndirect : public UnaryOperator {

public:
  OpIndirect(Expression* expr);
  virtual ~OpIndirect();
  Value* applyOp(Value* value);
};

// -----------------------------------------------------------------
class OpAddressOf : public UnaryOperator {

public:
  OpAddressOf(Expression* expr);
  virtual ~OpAddressOf();
  Value* evaluate();
  Value* applyOp(Value* value);
};

#endif // __OPERATOR_H__
