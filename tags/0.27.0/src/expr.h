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

#include <list>
#include <string>
#include <glib.h>
#include "value.h"

#if !defined(__EXPR_H__)
#define __EXPR_H__
using namespace std;

class Expression;
class symbol;
typedef list<Expression*> ExprList_t;
typedef list<Expression*>::iterator ExprList_itor;



class Expression : public gpsimObject
{

 public:

  Expression();

  virtual ~Expression();

  virtual Value* evaluate()=0;

};
//************************************************************************//
//
// Literal Expressions
//
// A Literal Expression is a wrapper around a Value object.

//----------------------------------------------------------------

class IndexedSymbol : public Expression {

public:

  IndexedSymbol(gpsimObject *, ExprList_t*);
  virtual ~IndexedSymbol();
  virtual Value* evaluate();
  string toString();

 private:
  Value *       m_pSymbol;
  ExprList_t *  m_pExprList;
};

//-----------------------------------------------------------------
class LiteralSymbol : public Expression {

public:

  LiteralSymbol(gpsimObject *);
  //LiteralSymbol(gpsimObject *, ExprList_t*);
  virtual ~LiteralSymbol();
  virtual Value* evaluate();
  virtual int set_break(ObjectBreakTypes bt=eBreakAny, ObjectActionTypes at=eActionHalt, Expression *expr=0);
  virtual int clear_break();
  string toString();
  Value *GetSymbol();

 private:
  Value *sym;
};

//-----------------------------------------------------------------
class LiteralArray : public Expression {

public:

  LiteralArray(ExprList_t*);
  virtual ~LiteralArray();
  virtual Value* evaluate();
  string toString();
private:
  ExprList_t *  m_pExprList;

};

//-----------------------------------------------------------------
class LiteralBoolean : public Expression {

public:
  LiteralBoolean(Boolean* value);
  virtual ~LiteralBoolean();
  virtual Value* evaluate();
  string toString();

private:
  Boolean* value;
};


//-----------------------------------------------------------------
class LiteralInteger : public Expression {

public:
  LiteralInteger(Integer* value);
  virtual ~LiteralInteger();
  virtual Value* evaluate();
  virtual int set_break(ObjectBreakTypes bt=eBreakAny, ObjectActionTypes at=eActionHalt, Expression *expr=0);
  string toString();

private:
  Integer* value;
};

//-----------------------------------------------------------------
class LiteralFloat : public Expression {

public:
  LiteralFloat(Float* value);
  virtual ~LiteralFloat();
  virtual Value* evaluate();
  string toString();

private:
  Float* value;
};


//-----------------------------------------------------------------
class LiteralString : public Expression {

public:
  LiteralString(String* newValue);
  virtual ~LiteralString();
  virtual Value* evaluate();
  string toString();

private:
  String* value;
};

class RegisterExpression : public Expression {

public:

  RegisterExpression(unsigned int uAddress);
  virtual ~RegisterExpression();
  virtual Value* evaluate();
  string toString();

 private:
  unsigned int  m_uAddress;
};

#endif // __EXPR_H__
