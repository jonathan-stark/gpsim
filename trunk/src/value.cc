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

#include "protocol.h"

//------------------------------------------------------------------------
Value::Value()
  : cpDescription(0)
{
}

Value::Value(const char *_name, const char *desc)
  : cpDescription(desc)
{
  new_name(_name);
}

Value::~Value()
{
}

void Value::set(const char *cP,int i)
{
  throw new Error(" cannot assign string to a " + showType());
}
void Value::set(double d)
{
  throw new Error(" cannot assign a double to a " + showType());
}
void Value::set(gint64 i)
{
  throw new Error(" cannot assign an integer to a " + showType());
}
void Value::set(bool v)
{
  throw new Error(" cannot assign a boolean to a " + showType());
}

void Value::set(int i)
{
  gint64 i64 = i;
  set(i64);
}

void Value::set(Value *v)
{
  throw new Error(" cannot assign a Value to a " + showType());
}

void Value::set(Expression *expr)
{
  Value *v=0;

  try {

    if(!expr)
      throw new Error(" null expression ");

    v = expr->evaluate();
    if(!v)
      throw new Error(" cannot evaluate expression ");

    set(v);

  }


  catch (Error *err) {
    if(err)
      cout << "ERROR:" << err->toString() << endl;
    delete err;
  }


  delete v;
  delete expr;

}

void Value::set(Packet &pb)
{
  cout << "Value is ignoring packet buffer\n";
}

void Value::get(gint64 &i)
{
  throw new Error(showType() +
		  " cannot be converted to an integer ");
}

void Value::get(int &i)
{
  gint64 i64;
  get(i64);
  i = (int) i64;
}

void Value::get(guint64 &i)
{
  // FIXME - casting an signed int to an unsigned int -- probably should issue a warning
  gint64 i64;
  get(i64);
  i = (gint64) i64;
}

void Value::get(double &d)
{
  throw new Error(showType() +
		  " cannot be converted to a double ");
}

// get as a string - no error is thrown if the derived class
// does not provide a method for converting to a string - 
// instead we'll return a bogus value.

void Value::get(char *buffer, int buf_size)
{
  if(buffer)
    strncpy(buffer,"INVALID",buf_size);
}

bool Value::compare(ComparisonOperator *compOp, Value *rvalue)
{
  throw new Error(compOp->showOp() + 
		  " comparison is not defined for " + showType());
}

Value *Value::copy()
{
  throw new Error(" cannot copy " + showType());

}

string Value::description()
{
  if(cpDescription)
    return string(cpDescription);
  else
    return string("no description");
}

void  Value::set_description(const char *new_description)
{
  cpDescription = new_description;
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
void AbstractRange::set(Value *v)
{
  AbstractRange *ar=typeCheck(v, string(""));
  left = ar->get_leftVal();
  right = ar->get_rightVal();
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

Boolean::Boolean(const char *_name, bool newValue,const char *_desc)
  : Value(_name,_desc)
{
  value = newValue;

}

Boolean::~Boolean()
{

}

string Boolean::toString()
{
  bool b;
  get(b);
  return (string(b ? "true" : "false"));
}

string Boolean::toString(bool value)
{
  return (string(value ? "true" : "false"));
}

string Boolean::toString(char* format)
{
  char cvtBuf[1024];
  bool b;
  get(b);

  sprintf(cvtBuf, format, b);
  return (string(&cvtBuf[0]));
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


// get(bool&) - primary method for accessing the value.
void Boolean::get(bool &b)
{
  b = value;
}

// get(int&) - type cast an integer into a boolean. Note
// that we call get(bool &) instead of directly accessing
// the member value. The reason for this is so that derived
// classes can capture the access.
void Boolean::get(int &i)
{ 
  bool b;
  get(b);
  i = b ? 1 : 0; 
}
/*
void Boolean::get(double &d) 
{
  bool b;
  get(b);
  d = b ? 1.0 : 0.0;
}
*/
void Boolean::get(char *buffer, int buf_size)
{
  if(buffer) {

    bool b;
    get(b);
    if(b)
      strncpy(buffer,"true",buf_size);
    else 
      strncpy(buffer,"false",buf_size);
  }

}

void Boolean::set(Value *v)
{
  Boolean *bv = typeCheck(v,string("set "));
  bool b = bv->getVal();
  set(b);
}

void Boolean::set(bool v)
{
  value = v;
}

void Boolean::set(const char *buffer, int buf_size)
{
  if(buffer) {

    if(strncmp("true", buffer, sizeof("true")-1) == 0)
      set(true);
    else 
      if(strncmp("false", buffer, sizeof("false")-1) == 0)
	set(false);
  }
}

void Boolean::set(Packet &p)
{
  bool b;
  if(p.DecodeBool(b))
    set(b);
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
Integer::Integer(gint64 newValue)
{
  value = newValue;
}

Integer::Integer(const char *_name, gint64 newValue,const char *_desc)
  : Value(_name,_desc)
{
   value = newValue;
}

Integer::~Integer()
{
}
void Integer::set(double d)
{
  gint64 i = (gint64)d;
  set(i);
}

void Integer::set(gint64 i)
{
  value = i;
}
void Integer::set(int i)
{
  gint64 ii = i;
  set(ii);
}
void Integer::set(Value *v)
{
  Integer *iv = typeCheck(v,string("set "));
  set(iv->getVal());
}

void Integer::set(Packet &p)
{
  unsigned int i;
  if(p.DecodeUInt32(i)) {

    set((int)i);
    return;
  }

  guint64 i64;
  if(p.DecodeUInt64(i64)) {

    set((gint64)i64);
    return;
  }
}

void Integer::set(const char *buffer, int buf_size)
{
  if(buffer) {

    gint64 i;

    int converted = sscanf(buffer, "%Ld",  &i);

    if(converted)
      set(i);
  }
}


void Integer::get(gint64 &i)
{ 
  i = value;
}
void Integer::get(double &d)
{ 
  gint64 i;
  get(i);
  d = i;;
}

void Integer::get(char *buffer, int buf_size)
{
  if(buffer) {

    gint64 i;
    get(i);

    snprintf(buffer,buf_size,"%Ld",i);
  }

}

string Integer::toString()
{
  char buf[256];
  gint64 i;
  get(i);
  snprintf(buf,sizeof(buf)," = %Ld = 0x%08LX",i,i);
  return (name() +string(buf));
}


string Integer::toString(char* format)
{
  char cvtBuf[1024];

  gint64 i;
  get(i);

  snprintf(cvtBuf,sizeof(cvtBuf), format, i);
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
  iVal->get(i);
  
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

  iVal->get(i);
  
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

  gint64 i,r;

  get(i);
  rv->get(r);

  if(i < r)
    return compOp->less();

  if(i > r)
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

Float::Float(const char *_name, double newValue,const char *_desc)
  : Value(_name,_desc)
{
  value = newValue;
}

Float::~Float()
{
}

void Float::set(double d)
{
  value = d;
}

void Float::set(gint64 i)
{
  double d = i;
  set(d);
}

void Float::set(Value *v)
{
  Float *fv = typeCheck(v,string("set "));
  double d = fv->getVal();
  set(d);
}

void Float::set(const char *buffer, int buf_size)
{
  if(buffer) {

    double d;

    int converted = sscanf(buffer, "%lf",  &d);

    if(converted)
      set(d);
  }
}

void Float::set(Packet &p)
{
  double d;
  if(p.DecodeFloat(d)) {

    set(d);
  }

}

void Float::get(gint64 &i)
{ 
  double d;
  get(d);
  i = (gint64)d;
}
void Float::get(double &d)
{ 
  d = value;
}

void Float::get(char *buffer, int buf_size)
{
  if(buffer) {

    double d;;
    get(d);

    snprintf(buffer,buf_size,"%g",d);
  }

}
string Float::toString()
{
  return toString("%#-16.16g");
}


string Float::toString(char* format)
{
  char cvtBuf[1024];

  double d;
  get(d);

  sprintf(cvtBuf, format, d);
  return (string(&cvtBuf[0]));
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

  double d,r;
  get(d);
  rv->get(r);

  if(d < r)
    return compOp->less();

  if(d > r)
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
String::String(const char *newValue)
{
  if(newValue)
    value = strdup(newValue);
  else
    value = 0;
}
String::String(const char *_name, const char *newValue,const char *_desc)
  : Value(_name,_desc)
{
  if(newValue)
    value = strdup(newValue);
  else
    value = 0;
}


String::~String()
{
  if(value)
    free(value);
}


string String::toString()
{
  if(value)
    return string(value);
  else 
    return string("");
}

void String::set(Value *v)
{
  char buf[1024];

  if(v) {
    v->get(buf, sizeof(buf));
    set(buf);
  }
}

void String::set(Packet &p)
{
  cout << " fixme String::set(Packet &) is not implemented\n";
}

void String::set(const char *s,int len)
{
  if(value)
    free(value);
  if(s)
    value = strdup(s);
  else
    value = 0;
}
void String::get(char *buf, int len)
{
  if(buf && value) {
    strncpy(buf,value,len);
  }
}

/*
string String::toString(char* format)
{
  char cvtBuf[1024];

  sprintf(cvtBuf, format, value.c_str());
  return (string(&cvtBuf[0]));
}
*/
/**
String* String::typeCheck(Value* val, string valDesc)
{
  if (typeid(*val) != typeid(String)) {
    throw new TypeMismatch(valDesc, "String", val->showType());
  }

  // This static cast is totally safe in light of our typecheck, above.
  return((String*)(val));
}
*/

/*
bool String::compare(ComparisonOperator *compOp, Value *rvalue)
{
  
  String *rv = typeCheck(rvalue,"");

  if(value < rv->value)
    return compOp->less();

  if(value > rv->value)
    return compOp->greater();

  return compOp->equal();
}
*/
const char *String::getVal()
{
  return value;
}

Value *String::copy()
{
  return new String(value);
}
