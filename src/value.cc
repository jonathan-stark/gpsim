/*
   Copyright (C) 1998-2003 Scott Dattalo

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


#include <stdio.h>
#include <iostream>
#include <iomanip>

#include "processor.h"

#include "value.h"
#include "errors.h"
#include "operator.h"

//------------------------------------------------------------------------
Value::Value()
{
  constant = false;
}

Value::~Value()
{
}

Value::Value(bool isConstant)
{
  constant = isConstant;
}

int Value::getAsInt()
{
  throw new Error(showType() +
		  " cannot be converted to an integer ");
}
void Value::get(int &i)
{
  i = getAsInt();
}

double Value::getAsDouble()
{
  throw new Error(showType() +
		  " cannot be converted to a double ");
}
char *Value::getAsStr(char *buffer, int buf_size)
{
  throw new Error(showType() +
		  "::getAsStr() should not be called ");
  /*
  if(!buffer || buf_size==0)
    return 0;

  if(value)
    strncpy(buffer,value,buf_size);
  else
    *buffer = 0;

  return buffer;
  */
}

bool Value::isConstant()
{
  return constant;
}

bool Value::compare(ComparisonOperator *compOp, Value *rvalue)
{
  throw new Error(compOp->showOp() + 
		  " comparison is not defined for " + showType());
}

void Value::set(char *cP)
{
  throw new Error(" cannot assign string to a " + showType());
}
void Value::set(char *cP,int i)
{
  throw new Error(" cannot assign string to a " + showType());
}
void Value::set(double d)
{
  throw new Error(" cannot assign a double to a " + showType());
}

/*
bool Value::operator<(Value *rv)
{
  throw Error("OpLT");
}
bool Value::operator>(Value *rv)
{
  throw Error("OpGT");
}
bool Value::operator<=(Value *rv)
{
  throw Error("OpLE");
}
bool Value::operator>=(Value *rv)
{
  throw Error("OpGE");
}
bool Value::operator==(Value *rv)
{
  throw Error("OpEQ");
}
bool Value::operator!=(Value *rv)
{
  throw Error("OpNE");
}
bool Value::operator&&(Value *rv)
{
  throw Error("Op&&");
}
bool Value::operator||(Value *rv)
{
  throw Error("Op||");
}
*/

//------------------------------------------------------------------------
// gpsimValue

gpsimValue::gpsimValue(void)
  : cpu(0)
{
}

gpsimValue::gpsimValue(Module *_cpu)
  : cpu(_cpu)
{
}

gpsimValue::~gpsimValue(void)
{
}

void gpsimValue::update(void)
{
  _xref._update();
}

void gpsimValue::add_xref(void *an_xref)
{
  _xref._add(an_xref);
}

void gpsimValue::remove_xref(void *an_xref)
{
  _xref.clear(an_xref);
}

string gpsimValue::toString()
{
  char buff[64];
  snprintf(buff,sizeof(buff), " = 0x%x",get_value());
  string s = name() + string(buff);
  return s;
}

Processor *gpsimValue::get_cpu()
{
  return static_cast<Processor *>(cpu);
}

void gpsimValue::set_cpu(Processor *new_cpu)
{
  cpu = new_cpu;
}


/*****************************************************************
 * The AbstractRange class.
 */
AbstractRange::AbstractRange(unsigned int newLeft, unsigned int newRight)
{
  left = newLeft;
  right = newRight;
}


AbstractRange::AbstractRange(unsigned int newLeft, unsigned int newRight, bool isConstant)
  : Value(isConstant)
{
  left = newLeft;
  right = newRight;
}


AbstractRange::~AbstractRange()
{
}


string AbstractRange::toString()
{
  string str = "";
  
  str = Integer::toString("%02d", left) + ":" + Integer::toString("%02d", right);
  return (str);
}

string AbstractRange::toString(char* format)
{
  char cvtBuf[1024];

  sprintf(cvtBuf, format, left, right);
  return (string(&cvtBuf[0]));
}

unsigned int AbstractRange::get_leftVal()
{
  return(left);
}

unsigned int AbstractRange::get_rightVal()
{
  return(right);
}

AbstractRange* AbstractRange::typeCheck(Value* val, string valDesc)
{
  if (typeid(*val) != typeid(AbstractRange)) {
    throw new TypeMismatch(valDesc, "AbstractRange", val->showType());
  }
  // This static cast is totally safe in light of our typecheck, above.
  return((AbstractRange*)(val));
}
bool AbstractRange::compare(ComparisonOperator *compOp, Value *rvalue)
{
  throw new Error(compOp->showOp() + 
		  " comparison is not defined for " + showType());
}

/*
bool AbstractRange::operator<(Value *rv)
{
  AbstractRange *_rv = typeCheck(rv,"OpLT");
  return right < _rv->left;
}
*/

/*****************************************************************
 * The Boolean class.
 */
Boolean::Boolean(bool newValue)
{
  value = newValue;
}

Boolean::Boolean(bool newValue, bool isConstant)
  : Value(isConstant)
{
  value = newValue;
}

Boolean::~Boolean()
{
}

string Boolean::toString()
{
  return (string(value ? "true" : "false"));
}

string Boolean::toString(bool value)
{
  return (string(value ? "true" : "false"));
}

string Boolean::toString(char* format)
{
  char cvtBuf[1024];

  sprintf(cvtBuf, format, value);
  return (string(&cvtBuf[0]));
}

bool Boolean::getVal()
{
  return value;
}

Boolean* Boolean::typeCheck(Value* val, string valDesc)
{
  if (typeid(*val) != typeid(Boolean)) {
    throw new TypeMismatch(valDesc, "Boolean", val->showType());
  }

  // This static cast is totally safe in light of our typecheck, above.
  return((Boolean*)(val));
}

bool Boolean::compare(ComparisonOperator *compOp, Value *rvalue)
{
  
  Boolean *rv = typeCheck(rvalue,"");

  switch(compOp->isa()) {
  case ComparisonOperator::eOpEq:
    return value == rv->value;
  case ComparisonOperator::eOpNe:
    return value != rv->value;
  default:
    Value::compare(compOp, rvalue);  // error
  }

  return false; // keep the compiler happy.
}

/*
bool Boolean::operator&&(Value *rv)
{
  Boolean *_rv = typeCheck(rv,"Op&&");
  return value && _rv->value;
}

bool Boolean::operator||(Value *rv)
{
  Boolean *_rv = typeCheck(rv,"Op||");
  return value || _rv->value;
}

bool Boolean::operator==(Value *rv)
{
  Boolean *_rv = typeCheck(rv,"OpEq");
  return value == _rv->value;
}

bool Boolean::operator!=(Value *rv)
{
  Boolean *_rv = typeCheck(rv,"OpNe");
  return value != _rv->value;
}
*/

/*****************************************************************
 * The Integer class.
 */

Integer::Integer(gint64 newValue, bool isConstant)
  : Value(isConstant)
{
  value = newValue;
}

string Integer::toString()
{
  return toString("%d");
}


string Integer::toString(char* format)
{
  char cvtBuf[1024];

  snprintf(cvtBuf,sizeof(cvtBuf), format, value);
  return (string(&cvtBuf[0]));
}


string Integer::toString(char* format, gint64 value)
{
  char cvtBuf[1024];

  snprintf(cvtBuf,sizeof(cvtBuf), format, value);
  return (string(&cvtBuf[0]));
}

string Integer::toString(gint64 value)
{
  char cvtBuf[1024];

  snprintf(cvtBuf,sizeof(cvtBuf), "%Ld", value);
  return (string(&cvtBuf[0]));  
}

gint64 Integer::getVal()
{
  return value;
}

Integer* Integer::typeCheck(Value* val, string valDesc)
{
  if (typeid(*val) != typeid(Integer)) {
    throw new TypeMismatch(valDesc, "Integer", val->showType());
  }

  // This static cast is totally safe in light of our typecheck, above.
  return((Integer*)(val));
}

Integer* Integer::assertValid(Value* val, string valDesc, gint64 valMin)
{
  Integer* iVal;
  gint64 i;

  iVal = Integer::typeCheck(val, valDesc);
  i = iVal->getVal();
  
  if (i < valMin) {
    throw new Error(valDesc +
                    " must be greater than " + Integer::toString(valMin) + 
                    ", saw " + Integer::toString(i)
                    );
  }
  
  return(iVal);
}

Integer* Integer::assertValid(Value* val, string valDesc, gint64 valMin, gint64 valMax)
{
  Integer* iVal;
  gint64 i;
  
  iVal = (Integer::typeCheck(val, valDesc));

  i = iVal->getVal();
  
  if ((i < valMin) || (i>valMax)) {
    throw new Error(valDesc +
                    " must be be in the range [" + Integer::toString(valMin) + ".." + 
                    Integer::toString(valMax) + "], saw " + Integer::toString(i)
                    );
  }
  
  return(iVal);
}

bool Integer::compare(ComparisonOperator *compOp, Value *rvalue)
{
  
  Integer *rv = typeCheck(rvalue,"");

  if(value < rv->value)
    return compOp->less();

  if(value > rv->value)
    return compOp->greater();

  return compOp->equal();
}

/*
bool Integer::operator<(Value *rv)
{
  Integer *_rv = typeCheck(rv,"OpLT");
  return value < _rv->value;
}

bool Integer::operator>(Value *rv)
{
  Integer *_rv = typeCheck(rv,"OpGT");
  return value > _rv->value;
}

bool Integer::operator<=(Value *rv)
{
  Integer *_rv = typeCheck(rv,"OpLE");
  return value <= _rv->value;
}

bool Integer::operator>(Value *rv)
{
  Integer *_rv = typeCheck(rv,"OpGT");
  return value > _rv->value;
}
*/

/*****************************************************************
 * The Float class.
 */
Float::Float(double newValue)
{
  value = newValue;
}

Float::Float(double newValue, bool isConstant)
  : Value(isConstant)
{
  value = newValue;
}

Float::~Float()
{
}

string Float::toString()
{
  return toString("%#-16.16g");
}


string Float::toString(char* format)
{
  char cvtBuf[1024];

  sprintf(cvtBuf, format, value);
  return (string(&cvtBuf[0]));
}

double Float::getVal()
{
  return(value);
}

void Float::set(double v)
{
  value = v;
}

Float* Float::typeCheck(Value* val, string valDesc)
{
  if (typeid(*val) != typeid(Float)) {
    throw new TypeMismatch(valDesc, "Float", val->showType());
  }

  // This static cast is totally safe in light of our typecheck, above.
  return((Float*)(val));
}

bool Float::compare(ComparisonOperator *compOp, Value *rvalue)
{
  
  Float *rv = typeCheck(rvalue,"");

  if(value < rv->value)
    return compOp->less();

  if(value > rv->value)
    return compOp->greater();

  return compOp->equal();
}

/*
bool Float::operator<(Value *rv)
{
  Float *_rv = typeCheck(rv,"OpLT");
  return value < _rv->value;
}
*/

/*****************************************************************
 * The String class.
 */
String::String(string newValue)
{
  value = newValue;
}

String::String(string newValue, bool isConstant)
  : Value(isConstant)
{
  value = newValue;
}


String::~String()
{
}


// -----------------------------------------------------------------
// The string toString is a bit more complicated than you might
// expect.  The complication comes from the fact that we want to
// display the string in a fashion that will allow it to be
// reparsed.  This really means that we need to reformat all
// non-printing chars that we encounter in the string.
string String::toString()
{
  unsigned int i;
  string msg;
  unsigned char c;

  msg = "\"";
  for (i=0; i<value.size(); ++i) {
    c = value.at(i);

    if (isprint((int)c)) {
      /* Look for the printable chars that need to be escaped
         so that the string could be easily reparsed by other
         tools. */
      if (c=='\\') msg += "\\";
      if (c=='"') msg += "\\";
      
      msg += c;
    }
    else {
      /* Display non-printing chars in a C-like fashion */
      switch (c) {
      case '\a': msg += "\\a"; break;
      case '\b': msg += "\\b"; break;
      case '\f': msg += "\\f"; break;
      case '\n': msg += "\\n"; break;
      case '\r': msg += "\\r"; break;
      case '\t': msg += "\\t"; break;
      case '\v': msg += "\\v"; break;
      default:   msg += "\\" + Integer::toString("%02x", c);
      }
    }
  }
  msg += '"';

  return(msg);
}

string String::toString(char* format)
{
  char cvtBuf[1024];

  sprintf(cvtBuf, format, value.c_str());
  return (string(&cvtBuf[0]));
}


string String::getVal()
{
  return(value);
}


String* String::typeCheck(Value* val, string valDesc)
{
  if (typeid(*val) != typeid(String)) {
    throw new TypeMismatch(valDesc, "String", val->showType());
  }

  // This static cast is totally safe in light of our typecheck, above.
  return((String*)(val));
}

bool String::compare(ComparisonOperator *compOp, Value *rvalue)
{
  
  String *rv = typeCheck(rvalue,"");

  if(value < rv->value)
    return compOp->less();

  if(value > rv->value)
    return compOp->greater();

  return compOp->equal();
}

/*
bool String::operator<(Value *rv)
{
  String *_rv = typeCheck(rv,"OpLT");
  return value < _rv->value;
}
*/
