/* Parser for gpsim
   Copyright (C) 2004 Scott Dattalo

This file is part of gpsim.

gpsim is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

gpsim is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with gpsim; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

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


  string show();
  string showType();
  string toString();

 protected:
  Expression* leftExpr;
  Expression* rightExpr;
  Value* value;

};

//-----------------------------------------------------------------
class OpAdd : public BinaryOperator {

public:
  OpAdd(Expression* leftExpr, Expression* rightExpr);
  virtual ~OpAdd();
  virtual Value* applyOp(Value* leftValue, Value* rightValue);

};


#endif // __OPERATOR_H__
