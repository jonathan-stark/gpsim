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

#include <list>
#include <string>
#include "viewable.h"


#if !defined(__EXPR_H__)
#define __EXPR_H__
using namespace std;

class Expression;
typedef list<Expression*> ExprList_t;
typedef list<Expression*>::iterator ExprList_itor;

class Value : public Viewable
{
 public:
  Value();
  Value(bool isConstant);

  virtual ~Value();
  virtual int getAsInt();
  virtual double getAsDouble();

 private:
  bool constant;
};


class Expression : public Viewable
{

 public:

  Expression();

  virtual ~Expression();

  virtual Value* evaluate()=0;

};

/*****************************************************************
 * Now we introduce classes for the basic built-in data types.
 * These classes are created by extending the Value class.  For
 * convenience, they all must instantiate a getVal() method that
 * returns valueof the object in question as a simple value of
 * the base data type.  For example, invoking getVal() on a
 * Boolean oject must return a simple 'bool' value.
 */
/*****************************************************************/
class Boolean : public Value {

public:
	
  Boolean(bool newValue);
  Boolean(bool newValue, bool isConstant);
  virtual ~Boolean();

  string toString();
  string toString(char* format);
  static string toString(bool value);
  static string toString(char* format, bool value);
  bool getVal();
  virtual int getAsInt() { return (value) ? 1 : 0; }
  virtual double getAsDouble() { return (value) ? 1.0 : 0.0;}

  static Boolean* Boolean::typeCheck(Value* val, string valDesc);
  //Value* eval();


private:
  bool value;
};


/*****************************************************************/
class Integer : public Value {

public:
	
  Integer(int new_value) : Value() { value = new_value;};
  Integer(int newValue, bool isConstant);
  virtual ~Integer() {}

  string toString();
  string toString(char* format);
  static string toString(int value);
  static string toString(char* format, int value);
  int getVal();

  virtual int getAsInt() { return value; }
  virtual double getAsDouble() { return (double)value;}
  virtual void put(int v) {value = v; }

  static Integer* Integer::typeCheck(Value* val, string valDesc);
  static Integer* Integer::assertValid(Value* val, string valDesc, int valMin);
  static Integer* Integer::assertValid(Value* val, string valDesc, int valMin, int valMax);

private:
  int value;
};

/*****************************************************************/
class Float : public Value {

public:
	
  Float(double newValue);
  Float(double newValue, bool isConstant);
  virtual ~Float();

  string toString();
  string toString(char* format);
  static string toString(double value);
  static string toString(char* format, double value);
  double getVal();
  virtual int getAsInt() { return (int)value; }
  virtual double getAsDouble() { return value;}

  static Float* typeCheck(Value* val, string valDesc);
  //Value* eval();

private:
  double value;
};


/*****************************************************************/
class String : public Value {

public:
	
  String(string newValue);
  String(string newValue, bool isConstant);
  virtual ~String();

  string toString();
  string toString(char* format);
  static string toString(string value);
  static string toString(char* format, string value);
  string getVal();

  static String* typeCheck(Value* val, string valDesc);

private:
  string value;
};

/*****************************************************************/

class AbstractRange : public Value {

public:
	
  AbstractRange(unsigned int leftVal, unsigned int rightVal);
  AbstractRange(unsigned int leftVal, unsigned int rightVal, bool isConstant);
  virtual ~AbstractRange();

  string toString();
  string toString(char* format);

  unsigned int get_leftVal();
  unsigned int get_rightVal();

  static AbstractRange* AbstractRange::typeCheck(Value* val, string valDesc);
  

private:
  unsigned int left;
  unsigned int right;
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

#endif // __EXPR_H__
