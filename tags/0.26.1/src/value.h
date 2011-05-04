/*
   Copyright (C) 1998-2004 Scott Dattalo

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

#ifndef __VALUE_H__
#define __VALUE_H__

#include "gpsim_object.h"
#include <glib.h>
#include <cstring>

class Processor;
class Module;
#include "xref.h"

class Expression;
class ComparisonOperator;
class Packet;

//------------------------------------------------------------------------
//
/// Value - the base class that supports types
///
/// Everything that can hold a value is derived from the Value class.
/// The primary purpose of this is to provide external objects (like
/// the gui) an abstract way of getting the value of diverse things
/// like registers, program counters, cycle counters, etc.
///
/// In addition, expressions of Values can be created and operated
/// on.


class Value : public gpsimObject
{
public:
  Value();
  Value(const char *name, const char *desc, Module *pM=0);
  virtual ~Value();

  virtual unsigned int get_leftVal() {return 0;}
  virtual unsigned int get_rightVal() {return 0;}

  /// Value 'set' methods provide a mechanism for casting values to the
  /// the type of this value. If the type cast is not supported in a
  /// derived class, an Error will be thrown.

  virtual void set(const char *cP,int len=0);
  virtual void set(double);
  virtual void set(gint64);
  virtual void set(int);
  virtual void set(bool);
  virtual void set(Value *);
  virtual void set(Expression *);
  virtual void set(Packet &);

  /// Value 'get' methods provide a mechanism of casting Value objects
  /// to other value types. If the type cast is not supported in a
  /// derived class, an Error will be thrown.

  virtual void get(bool &b);
  virtual void get(int &);
  virtual void get(guint64 &);
  virtual void get(gint64 &);
  virtual void get(double &);
  virtual void get(char *, int len);
  virtual void get(Packet &);

  inline operator gint64() {
    gint64 i;
    get(i);
    return i;
  }

  inline operator int() {
    gint64 i;
    get(i);
    return (int)i;
  }

  inline operator unsigned int() {
    gint64 i;
    get(i);
    return (unsigned int)i;
  }

  inline Value & operator =(int i) {
    set(i);
    return *this;
  }

  inline Value & operator =(unsigned int i) {
    set((int)i);
    return *this;
  }

  /// compare - this method will compare another object to this
  /// object. It takes a pointer to a ComparisonOperator as its
  /// input. Of the object's are mismatched for the particular
  /// operator, a 'Type Mismatch' Error will be thown.

  virtual bool compare(ComparisonOperator *compOp, Value *rvalue);

  /// copy - return an object that is identical to this one.

  virtual Value *copy();

  /// xrefs - a cross reference allows a Value to notify another
  /// Value when it is changed.

  //**  virtual void set_xref(Value *);
  //**  virtual Value *get_xref();

  // Some Value types that are used for symbol classes
  // contain a gpsimValue type that have update listeners.
  virtual void update(); // {}
  virtual Value* evaluate() { return copy(); }

  virtual void add_xref(void *xref);
  virtual void remove_xref(void *xref);
  XrefObject xref() { return _xref; }


  virtual void set_module(Module *new_cpu);
  Module *get_module();
  virtual void set_cpu(Processor *new_cpu);
  Processor *get_cpu() const;
  //virtual string toString();

  void addName(string &r_sAliasedName);

private:
  XrefObject _xref;

protected:
  // A pointer to the module that owns this value.
  Module *cpu;
  // Aliased names for this Value
  list<string> *m_aka;
};


/*****************************************************************
 ValueWrapper
 */
class ValueWrapper : public Value
{
public:
  ValueWrapper(Value *pCopy);
  virtual ~ValueWrapper();

  virtual unsigned int get_leftVal();
  virtual unsigned int get_rightVal();
  virtual void set(const char *cP,int len=0);
  virtual void set(double);
  virtual void set(gint64);
  virtual void set(int);
  virtual void set(bool);
  virtual void set(Value *);
  virtual void set(Expression *);
  virtual void set(Packet &);

  virtual void get(bool &b);
  virtual void get(int &);
  virtual void get(guint64 &);
  virtual void get(gint64 &);
  virtual void get(double &);
  virtual void get(char *, int len);
  virtual void get(Packet &);
  virtual Value *copy();
  virtual void update();
  virtual Value* evaluate();
  virtual bool compare(ComparisonOperator *compOp, Value *rvalue);
private:
  Value *m_pVal;
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
  Boolean(const char *_name, bool newValue, const char *desc=0);
  static bool Parse(const char *pValue, bool &bValue);
  static Boolean * NewObject(const char *_name, const char *pValue, const char *desc);
  virtual ~Boolean();

  string toString();
  string toString(const char* format);
  static string toString(bool value);
  static string toString(const char* format, bool value);

  virtual void get(bool &b);
  virtual void get(int &i);
  virtual void get(char *, int len);
  virtual void get(Packet &);

  virtual void set(bool);
  virtual void set(Value *);
  virtual void set(const char *cP,int len=0);
  virtual void set(Packet &);

  bool getVal() { return value; }

  static Boolean* typeCheck(Value* val, string valDesc);
  virtual bool compare(ComparisonOperator *compOp, Value *rvalue);

  virtual Value *copy();

  /// copy the object value to a user char array
  virtual char *toString(char *return_str, int len);
  virtual char *toBitStr(char *return_str, int len);

  inline operator bool() {
    bool bValue;
    get(bValue);
    return bValue;
  }

  inline Boolean &operator = (bool bValue) {
    set(bValue);
    return *this;
  }

private:
  bool value;
};

inline bool operator!=(Boolean &LValue, Boolean &RValue) {
  return (bool)LValue != (bool)RValue;
}


//------------------------------------------------------------------------
/// Integer - built in gpsim type for a 64-bit integer.

class Integer : public Value {

public:

  Integer(const Integer &new_value);
  Integer(gint64 new_value);
  Integer(const char *_name, gint64 new_value, const char *desc=0);
  static bool       Parse(const char *pValue, gint64 &iValue);
  static Integer *  NewObject(const char *_name, const char *pValue, const char *desc);

  virtual ~Integer();

  virtual string toString();
  string toString(const char* format);
  static string toString(gint64 value);
  static string toString(const char* format, gint64 value);

  virtual void get(gint64 &i);
  virtual void get(double &d);
  virtual void get(char *, int len);
  virtual void get(Packet &);

  virtual void set(gint64 v);
  virtual void set(int);
  virtual void set(double d);
  virtual void set(Value *);
  virtual void set(const char *cP,int len=0);
  virtual void set(Packet &);

  static void setDefaultBitmask(gint64 bitmask);

  inline void setBitmask(gint64 bitmask) {
    this->bitmask = bitmask;
  }

  inline gint64 getBitmask() {
    return bitmask;
  }

  gint64 getVal() { return value; }

  virtual int set_break(ObjectBreakTypes bt=eBreakAny, ObjectActionTypes at=eActionHalt, Expression *expr=0);

  virtual Value *copy();
  /// copy the object value to a user char array
  virtual char *toString(char *, int len);
  virtual char *toBitStr(char *, int len);

  static Integer* typeCheck(Value* val, string valDesc);
  static Integer* assertValid(Value* val, string valDesc, gint64 valMin);
  static Integer* assertValid(Value* val, string valDesc, gint64 valMin, gint64 valMax);
  virtual bool compare(ComparisonOperator *compOp, Value *rvalue);

  inline operator gint64() {
    gint64 i;
    get(i);
    return i;
  }

  inline operator guint64() {
    gint64 i;
    get(i);
    return (guint64)i;
  }

  inline operator bool() {
    gint64 i;
    get(i);
    return i != 0;
  }

  inline operator int() {
    gint64 i;
    get(i);
    return (int)i;
  }

  inline operator unsigned int() {
    gint64 i;
    get(i);
    return (unsigned int)i;
  }

  inline Integer & operator =(const Integer &i) {
    Integer & ii = (Integer &)i;
    gint64 iNew = (gint64)ii;
    set(iNew);
    bitmask = i.bitmask;
    return *this;
  }

  inline Integer & operator =(int i) {
    set(i);
    return *this;
  }

  inline Integer & operator =(unsigned int i) {
    set((int)i);
    return *this;
  }

  inline Integer & operator &=(int iValue) {
    gint64 i;
    get(i);
    set((int)i & iValue);
    return *this;
  }

  inline Integer & operator |=(int iValue) {
    gint64 i;
    get(i);
    set((int)i | iValue);
    return *this;
  }

  inline Integer & operator +=(int iValue) {
    gint64 i;
    get(i);
    set((int)i + iValue);
    return *this;
  }

  inline Integer & operator ++(int) {
    gint64 i;
    get(i);
    set((int)i + 1);
    return *this;
  }

  inline Integer & operator --(int) {
    gint64 i;
    get(i);
    set((int)i - 1);
    return *this;
  }

  inline Integer & operator <<(int iShift) {
    gint64 i;
    get(i);
    set(i << iShift);
    return *this;
  }

  inline bool operator !() {
    gint64 i;
    get(i);
    return i == 0;
  }

private:
  gint64 value;
  // Used for display purposes
  gint64 bitmask;
  static gint64 def_bitmask;
};

inline bool operator!=(Integer &iLValue, Integer &iRValue) {
  return (gint64)iLValue != (gint64)iRValue;
}

//------------------------------------------------------------------------
/// Float - built in gpsim type for a 'double'

class Float : public Value {

public:

  Float(double newValue = 0.0);
  Float(const char *_name, double newValue, const char *desc=0);
  static bool Parse(const char *pValue, double &fValue);
  static Float * NewObject(const char *_name, const char *pValue, const char *desc);
  virtual ~Float();

  virtual string toString();
  string toString(const char* format);
  static string toString(double value);
  static string toString(const char* format, double value);

  virtual void get(gint64 &i);
  virtual void get(double &d);
  virtual void get(char *, int len);
  virtual void get(Packet &);

  virtual void set(gint64 v);
  virtual void set(double d);
  virtual void set(Value *);
  virtual void set(const char *cP,int len=0);
  virtual void set(Packet &);

  double getVal() { return value; }

  virtual Value *copy();
  /// copy the object value to a user char array
  virtual char *toString(char *, int len);

  static Float* typeCheck(Value* val, string valDesc);
  virtual bool compare(ComparisonOperator *compOp, Value *rvalue);

  inline operator double() {
    double d;
    get(d);
    return d;
  }

  inline Float & operator = (double d) {
    set((double)d);
    return *this;
  }

  inline Float & operator = (int d) {
    set((double)d);
    return *this;
  }

  inline Float & operator += (Float &d) {
    set((double)*this + (double)d );
    return *this;
  }

  inline Float & operator *= (Float &d) {
    set((double)*this * (double)d );
    return *this;
  }

  inline Float & operator *= (double d) {
    set((double)*this * d );
    return *this;
  }

private:
  double value;
};

inline bool operator!=(Float &iLValue, Float &iRValue) {
  return (double)iLValue != (double)iRValue;
}


/*****************************************************************/
class String : public Value {

public:

  String(const char *newValue);
  String(const char *newValue, size_t len);
  String(const char *_name, const char *newValue, const char *desc=0);
  virtual ~String();

  virtual string toString();

  const char *getVal();

  virtual void set(Value *);
  virtual void set(const char *cP,int len=0);
  virtual void set(Packet &);

  virtual void get(char *, int len);
  virtual void get(Packet &);

  virtual Value *copy();
  /// copy the object value to a user char array
  virtual char *toString(char *, int len);

  inline operator const char *() {
    return getVal();
  }

private:
  char *value;
};

inline bool operator!=(String &LValue, String &RValue) {
  return strcmp((const char *)LValue, (const char *)RValue) != 0;
}


/*****************************************************************/

class AbstractRange : public Value {

public:

  AbstractRange(unsigned int leftVal, unsigned int rightVal);
  virtual ~AbstractRange();

  virtual string toString();
  string toString(const char* format);

  virtual unsigned int get_leftVal();
  virtual unsigned int get_rightVal();

  virtual void set(Value *);

  virtual Value *copy();
  /// copy the object value to a user char array
  virtual char *toString(char *return_str, int len);

  static AbstractRange* typeCheck(Value* val, string valDesc);
  virtual bool compare(ComparisonOperator *compOp, Value *rvalue);

private:
  unsigned int left;
  unsigned int right;
};

//------------------------------------------------------------------------
// Function -- maybe should go into its own header file.
//

typedef list<Expression*> ExprList_t;
namespace gpsim {
  class Function : public gpsimObject {

  public:

    Function(const char *_name, const char *desc=0);
    virtual ~Function();
    virtual string description();
    virtual string toString();

    void call(ExprList_t *vargs);
  };
}

char * TrimWhiteSpaceFromString(char * pBuffer);
char * UnquoteString(char * pBuffer);
string &toupper(string & sStr);

#endif // __VALUE_H__
