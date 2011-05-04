/*
   Copyright (C) 1998-2003 Scott Dattalo

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


#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <cstdio>

#include "processor.h"

#include "value.h"
#include "errors.h"
#include "operator.h"

#include "protocol.h"
#include "../config.h"
#include "cmd_gpsim.h"

#include <typeinfo>

char * TrimWhiteSpaceFromString(char * pBuffer) {
  size_t iPos = 0;
  char * pChar = pBuffer;
  while(*pChar != 0 && ::isspace(*pChar)) {
    pChar++;
  }
  if(pBuffer != pChar) {
    memmove(pBuffer, pChar, strlen(pBuffer) - iPos);
  }
  iPos = strlen(pBuffer);
  if(iPos > 0) {
    pChar = pBuffer + iPos - 1;
    while(pBuffer != pChar && ::isspace(*pChar)) {
      *pChar = 0;
      pChar--;
    }
  }
  return pBuffer;
}

char * UnquoteString(char * pBuffer) {
  char cQuote;
  if(*pBuffer == '\'') {
    cQuote = '\'';
  }
  else if(*pBuffer == '"') {
    cQuote = '"';
  }
  else {
    return pBuffer;
  }
  int iLen = strlen(pBuffer);
  if(iLen > 1) {
    if(pBuffer[iLen - 1] == cQuote) {
      memmove(&pBuffer[0], &pBuffer[1], iLen - 2);
      pBuffer[iLen - 2] = 0;
    }
  }
  return pBuffer;
}

string &toupper(string & sStr) {
  string::iterator it;
  string::iterator itEnd = sStr.end();
  for(it = sStr.begin(); it != itEnd; it++) {
    if(isalpha(*it)) {
      *it = toupper((int)*it);
    }
  }
  return sStr;
}

//------------------------------------------------------------------------
Value::Value()
  : cpu(0), m_aka(0)
{
}

Value::Value(const char *_name, const char *desc, Module *pMod)
  : gpsimObject(_name,desc), cpu(pMod),m_aka(0)
{
}

Value::~Value()
{
  // Remove references of this Value from the symbol table:
  if (cpu) {

    //cout << "Deleting value named:" << name_str <<  " addr "<< this << endl;
    cpu->removeSymbol(name_str);

    if (m_aka) {
      //cout << "m_aka ==" << m_aka << endl;

      list <string>::iterator it;
      it = m_aka->begin();
      while(it != m_aka->end()) {
        string s(*it);
        //cout << " Value label " << s << endl;
        cpu->removeSymbol(s);
        ++it;
      }
      m_aka->clear();
      delete m_aka;
    }

  }
}
void Value::update()
{
  _xref._update();
}

void Value::add_xref(void *an_xref)
{
  _xref._add(an_xref);
}

void Value::remove_xref(void *an_xref)
{
  _xref.clear(an_xref);
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
  try {
    Value *v=0;

    if(!expr)
      throw new Error(" null expression ");
    if (verbose)
      cout << toString() << " is being assigned expression " << expr->toString() << endl;
    v = expr->evaluate();
    if(!v)
      throw new Error(" cannot evaluate expression ");

    set(v);
    delete v;
  }


  catch (Error *err) {
    if(err)
      cout << "ERROR:" << err->toString() << endl;
    delete err;
  }

}

void Value::set(Packet &pb)
{
  cout << "Value,"<<name()<<" is ignoring packet buffer for set()\n";
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
  // FIXME - casting a signed int to an unsigned int -- probably should issue a warning
  gint64 i64;
  get(i64);
  i = (gint64) i64;
}

void Value::get(bool &b)
{
  throw new Error(showType() +
                  " cannot be converted to a boolean");
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

void Value::get(Packet &pb)
{
  cout << "Value,"<<name()<<" is ignoring packet buffer for get()\n";
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

/*
void Value::set_xref(Value *v)
{
  delete xref;
  xref = v;
}
Value *Value::get_xref()
{
  return xref;
}
*/

Processor *Value::get_cpu() const
{
  return static_cast<Processor *>(cpu);
}

void Value::set_cpu(Processor *new_cpu)
{
  cpu = new_cpu;
}
void Value::set_module(Module *new_cpu)
{
  cpu = new_cpu;
}
Module *Value::get_module()
{
  return cpu;
}

void Value::addName(string &r_sAliasedName)
{
  if (!m_aka)
    m_aka = new list<string>();

  //cout << "Adding name " << r_sAliasedName << " to "<< name() << endl;
  m_aka->push_back(r_sAliasedName);
}

//------------------------------------------------------------------------
ValueWrapper::ValueWrapper(Value *pCopy)
  : m_pVal(pCopy)
{
}
ValueWrapper::~ValueWrapper()
{
}
unsigned int ValueWrapper::get_leftVal()
{
  return m_pVal->get_leftVal();
}
unsigned int ValueWrapper::get_rightVal()
{
  return m_pVal->get_rightVal();
}
void ValueWrapper::set(const char *cP,int len)
{
  m_pVal->set(cP,len);
}
void ValueWrapper::set(double d)
{
  m_pVal->set(d);
}
void ValueWrapper::set(gint64 i)
{
  m_pVal->set(i);
}
void ValueWrapper::set(int i)
{
  m_pVal->set(i);
}
void ValueWrapper::set(bool b)
{
  m_pVal->set(b);
}
void ValueWrapper::set(Value *v)
{
  m_pVal->set(v);
}
void ValueWrapper::set(Expression *e)
{
  m_pVal->set(e);
}
void ValueWrapper::set(Packet &p)
{
  m_pVal->set(p);
}
void ValueWrapper::get(bool &b)
{
  m_pVal->get(b);
}
void ValueWrapper::get(int &i)
{
  m_pVal->get(i);
}
void ValueWrapper::get(guint64 &i)
{
  m_pVal->get(i);
}
void ValueWrapper::get(gint64 &i)
{
  m_pVal->get(i);
}
void ValueWrapper::get(double &d)
{
  m_pVal->get(d);
}
void ValueWrapper::get(char *pC, int len)
{
  m_pVal->get(pC,len);
}
void ValueWrapper::get(Packet &p)
{
  m_pVal->get(p);
}
Value *ValueWrapper::copy()
{
  return m_pVal->copy();
}
void ValueWrapper::update()
{
  m_pVal->update();
}
Value *ValueWrapper::evaluate()
{
  return m_pVal->evaluate();
}

bool ValueWrapper::compare(ComparisonOperator *compOp, Value *rvalue)
{
  if(!compOp || !rvalue)
    return false;

  gint64 i,r;

  m_pVal->get(i);
  rvalue->get(r);

  if(i < r)
    return compOp->less();

  if(i > r)
    return compOp->greater();

  return compOp->equal();
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
  char buff[256];

  string str = "";

  snprintf(buff,sizeof(buff),"%d:%d",left,right);
  return (string(buff));
}

string AbstractRange::toString(const char* format)
{
  char cvtBuf[1024];

  sprintf(cvtBuf, format, left, right);
  return (string(&cvtBuf[0]));
}

char *AbstractRange::toString(char *return_str, int len)
{
  if(return_str) {

    snprintf(return_str,len,"%d:%d",left,right);
  }

  return return_str;
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

Value *AbstractRange::copy()
{
  return new AbstractRange(get_leftVal(),get_rightVal());
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

bool Boolean::Parse(const char *pValue, bool &bValue) {
  if(strncmp("true", pValue, sizeof("true")-1) == 0) {
    bValue = true;
    return true;
  }
  else if(strncmp("false", pValue, sizeof("false")-1) == 0) {
        bValue = false;
    return true;
  }
  return false;
}

Boolean * Boolean::NewObject(const char *_name, const char *pValue, const char *desc) {
  bool bValue;
  if(Parse(pValue, bValue)) {
    return new Boolean(_name, bValue);
  }
  return NULL;
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

char *Boolean::toString(char *return_str, int len)
{
  if(return_str) {
    bool b;
    get(b);
    snprintf(return_str,len,"%s",(b ? "true" : "false"));
  }

  return return_str;
}
char *Boolean::toBitStr(char *return_str, int len)
{
  if(return_str) {
    bool b;
    get(b);
    snprintf(return_str,len,"%d",(b ? 1 : 0));
  }

  return return_str;
}

string Boolean::toString(const char* format)
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


Value *Boolean::copy()
{
  bool b;
  get(b);
  return new Boolean(b);
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
void Boolean::get(Packet &pb)
{
  bool b;
  get(b);
  pb.EncodeBool(b);
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
  //if(get_xref())
  //  get_xref()->set(v);
}

void Boolean::set(const char *buffer, int buf_size)
{
  if(buffer) {
    bool bValue;
    if(Parse(buffer, bValue)) {
      set(bValue);
    }
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
Integer::Integer(const Integer &new_value) {
  Integer & nv = (Integer&)new_value;
  nv.get(value);
  bitmask = new_value.bitmask;
}

Integer::Integer(gint64 newValue)
{
  value = newValue;
  bitmask = def_bitmask;
}

Integer::Integer(const char *_name, gint64 newValue,const char *_desc)
  : Value(_name,_desc)
{
  value = newValue;
  bitmask = def_bitmask;
}

gint64 Integer::def_bitmask = 0xffffffff;

Integer::~Integer()
{
}

void Integer::setDefaultBitmask(gint64 bitmask) {
  def_bitmask = bitmask;
}

Value *Integer::copy()
{
  gint64 i;
  get(i);
  return new Integer(i);
}

void Integer::set(double d)
{
  gint64 i = (gint64)d;
  set(i);
}

void Integer::set(gint64 i)
{
  value = i;
  //if(get_xref())
  //  get_xref()->set(i);
}
void Integer::set(int i)
{
  gint64 ii = i;
  set(ii);
}
void Integer::set(Value *v)
{
  gint64 iv = 0;
  if (v)
    v->get(iv);

  set(iv);
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
    if(Parse(buffer, i)) {
      set(i);
    }
  }
}

bool Integer::Parse(const char *pValue, gint64 &iValue) {
    if(::isdigit(*pValue)) {
      if(strchr(pValue, '.')) {
        return false;
      }
      else {
        // decimal or 0x integer
        return sscanf(pValue, "%" PRINTF_GINT64_MODIFIER "i", &iValue) == 1;
      }
    }
    else if(*pValue == '$' && ::isxdigit(*(pValue+1))) {
      // hexidecimal integer
      char szHex[10] = "0x";
      strcat(&szHex[0], pValue + 1);
      return sscanf(szHex, "%"  PRINTF_GINT64_MODIFIER "i" , &iValue) == 1;
    }
    return false;
}

Integer * Integer::NewObject(const char *_name, const char *pValue, const char *desc) {
  gint64 iValue;
  if(Parse(pValue, iValue)) {
    return new Integer(_name, iValue, desc);
  }
  return NULL;
}


void Integer::get(gint64 &i)
{
  i = value;
}

void Integer::get(double &d)
{
  gint64 i;
  get(i);
  d = (double)i;
}

void Integer::get(char *buffer, int buf_size)
{
  if(buffer) {

    gint64 i;
    get(i);
    long long int j = i;
    snprintf(buffer,buf_size,"%" PRINTF_INT64_MODIFIER "d",j);
  }

}
void Integer::get(Packet &pb)
{
  gint64 i;
  get(i);

  unsigned int j = (unsigned int) (i &0xffffffff);
  pb.EncodeUInt32(j);
}

int Integer::set_break(ObjectBreakTypes bt, ObjectActionTypes at, Expression *expr)
{
  Processor *pCpu = get_active_cpu();
  if (pCpu) {

    // Legacy code compatibility!

    if ( bt == eBreakWrite || bt == eBreakRead ) {

      // Cast the integer into a register and set a register break point
      unsigned int iRegAddress = (unsigned int) value;
      Register *pReg = &pCpu->rma[iRegAddress];
      return get_bp().set_break(bt, at, pReg, expr);
    } else if ( bt == eBreakExecute) {

      unsigned int iProgAddress = (unsigned int) value;
      return get_bp().set_execution_break(pCpu, iProgAddress, expr);
    }
  }

  return -1;
}

string Integer::toString()
{
  gint64 i;
  get(i);
  IUserInterface & TheUI = GetUserInterface();
  return string(TheUI.FormatValue(i, (unsigned int)bitmask));
}


string Integer::toString(const char* format)
{
  char cvtBuf[1024];

  gint64 i;
  get(i);

  snprintf(cvtBuf,sizeof(cvtBuf), format, i);
  return (string(&cvtBuf[0]));
}


string Integer::toString(const char* format, gint64 value)
{
  char cvtBuf[1024];

  snprintf(cvtBuf,sizeof(cvtBuf), format, value);
  return (string(&cvtBuf[0]));
}

string Integer::toString(gint64 value)
{
  char cvtBuf[1024];
  long long int v=value;
  snprintf(cvtBuf,sizeof(cvtBuf), "%" PRINTF_INT64_MODIFIER "d", v);
  return (string(&cvtBuf[0]));
}

char *Integer::toString(char *return_str, int len)
{
  if(return_str) {
    gint64 i;
    get(i);
    IUserInterface & TheUI = GetUserInterface();
    strncpy(return_str, TheUI.FormatValue(i), len);
//    snprintf(return_str,len,"%" PRINTF_INT64_MODIFIER "d",i);
  }

  return return_str;
}
char *Integer::toBitStr(char *return_str, int len)
{
  if(return_str) {
    gint64 i;
    get(i);
    int j=0;
    int mask=1<<31;
    for( ; mask ; mask>>=1, j++)
      if(j<len)
        return_str[j] = ( (i & mask) ? 1 : 0);

    if(j<len)
      return_str[j]=0;
  }

  return return_str;
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

bool Float::Parse(const char *pValue, double &fValue)
{
  return pValue ? sscanf(pValue,"%lg",&fValue) == 1 : false;
}

Float * Float::NewObject(const char *_name, const char *pValue, const char *desc) {
  double fValue;
  if(Parse(pValue, fValue)) {
    return new Float(_name, fValue);
  }
  return NULL;
}

Float::~Float()
{
}

void Float::set(double d)
{
  value = d;
  //if(get_xref())
  //  get_xref()->set(d);
}

void Float::set(gint64 i)
{
  double d = (double)i;
  set(d);
}

void Float::set(Value *v)
{
  /* typeCheck means cannot set integers - RRR
  Float *fv = typeCheck(v,string("set "));
  double d = fv->getVal();
  set(d);
  */
   double d;

  if (typeid(*v) != typeid(Float) &&
      typeid(*v) != typeid(Integer))
  {
    throw new TypeMismatch(string("set "), "Float", v->showType());
  }
   v->get(d);
   set(d);
}

void Float::set(const char *buffer, int buf_size)
{
  if(buffer) {

    double d;
    if(Parse(buffer, d)) {
      set(d);
    }
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
void Float::get(Packet &pb)
{
  double d;
  get(d);

  pb.EncodeFloat(d);
}

Value *Float::copy() {
  double d;
  get(d);
  return new Float(d);
}

string Float::toString()
{
  return toString("%#-16.16g");
}


string Float::toString(const char* format)
{
  char cvtBuf[1024];

  double d;
  get(d);

  sprintf(cvtBuf, format, d);
  return (string(&cvtBuf[0]));
}

char *Float::toString(char *return_str, int len)
{
  if(return_str) {

    double d;
    get(d);
    snprintf(return_str,len,"%g",d);
  }

  return return_str;
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

String::String(const char *newValue, size_t len)
{
  if (newValue) {
    value = (char *)malloc(len + 1);
    strncpy(value, newValue, len);
    value[len] = '\0';
  }
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

char *String::toString(char *return_str, int len)
{
  if(return_str) {

    if(value)
      snprintf(return_str,len,"%s",value);
    else
      *return_str = 0;
  }

  return return_str;
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
  else if(buf) {
    buf[0] = 0;
  }
}

void String::get(Packet &p)
{
  p.EncodeString(value);
}

const char *String::getVal()
{
  return value;
}

Value *String::copy()
{
  return new String(value);
}

//------------------------------------------------------------------------
namespace gpsim {
  Function::Function(const char *_name, const char *desc)
    : gpsimObject(_name,desc)
  {
  }

  Function::~Function()
  {
    cout << "Function destructor\n";
  }

  string Function::description()
  {
    if(cpDescription)
      return string(cpDescription);
    else
      return string("no description");
  }

  string Function::toString()
  {
    return name();
  }

  void Function::call(ExprList_t *vargs)
  {
    cout << "calling " << name() << endl;
  }

}
