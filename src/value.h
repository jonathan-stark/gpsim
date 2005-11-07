/*
   Copyright (C) 1998-2004 Scott Dattalo

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

#ifndef __VALUE_H__
#define __VALUE_H__

#include "gpsim_object.h"
#include <glib.h>

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
  Value(const char *name, const char *desc);

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

  /// description - get a description of this value. If the value has 
  /// a name, then 'help value_name' will display the description.

  virtual string description();
  void set_description(const char *);

  /// xrefs - a cross reference allows a Value to notify another
  /// Value when it is changed.

  virtual void set_xref(Value *);
  virtual Value *get_xref();

  /// breakpoints
  virtual void set_break();
  virtual void clear_break();

  virtual void setClearableSymbol(bool bClear);
  virtual bool isClearable();
  // Some Value types that are used for symbol classes
  // contain a gpsimValue type that have update listeners.
  virtual void update(void) {}

 private:
  const char *cpDescription;
  Value *xref;
protected:
  bool m_bClearableSymbol;
};

//========================================================================
//
/// gpsimValue is a Value object that is unique to the simulator.
/// All value objects of the behavioral models are derived from gpsimValue.
/// A gpsimValue object 'belongs to' a Module.

class gpsimValue : public gpsimObject 
{
public:

  gpsimValue(void);
  gpsimValue(Module *);
  virtual ~gpsimValue();


  /// Access functions
  /// \deprecated Use SimulatedGet()
  virtual unsigned int get(void)
  {
    return get_value();
  }

  /// \deprecated Use SimulatedSet()
  virtual void put(unsigned int new_value)
  {
    put_value(new_value);
  }

  /// put_value is the same as put(), but some extra stuff like
  /// interfacing to the gui is done. (It's more efficient than
  /// burdening the run time performance with (unnecessary) gui
  /// calls.)
  ///
  /// \deprecated Use ShuntedSet() where appropriate. If ShuntedSet()
  /// does not meet the procise needs consider defining a new
  /// pair of access functions.

  virtual void put_value(unsigned int new_value) = 0;

  /// A variation of get(). (for register values, get_value returns
  /// just the value whereas get() returns the value and does other
  /// register specific things like tracing.

  virtual unsigned int get_value(void) = 0;

  /// SimulatedGet() invokes the full simulation of getting a
  /// a value from a simulated object.
  virtual unsigned int  SimulatedGet(void) {
    return ShuntedGet();
  }

  /// SimulatedSet() invokes the full simulation of setting a
  /// a value of a simulated object.
  virtual void          SimulatedSet(unsigned int new_value) {
    ShuntedSet(new_value);
  }

  /// ShuntedGet() bypasses all simulated effects of a SimulatedSet().
  /// \return Returns an identical value as a SimulatedGet()
  virtual unsigned int  ShuntedGet(void) {
    // for now use old API
    return get_value();
  }

  /// ShuntedSet() bypasses all simulated effects of a SimulatedSet().
  virtual void          ShuntedSet(unsigned int new_value){
    // for now use old API
    put_value(new_value);
  }

  virtual void set_module(Module *new_cpu)
  {
    cpu = new_cpu;
  }

  Module *get_module(void)
  {
    return cpu;
  }

  virtual void set_cpu(Processor *new_cpu);

  Processor *get_cpu(void) const;

  // When the value changes, then update() is called 
  // to update all things that are watching this value.
  virtual void update(void);
  virtual void add_xref(void *xref);
  virtual void remove_xref(void *xref);

  XrefObject xref(void) { return _xref; }

  virtual string toString();

protected:
  // If we are linking with a gui, then here are a
  // few declarations that are used to send data to it.
  // This is essentially a singly-linked list of pointers
  // to structures. The structures provide information
  // such as where the data is located, the type of window
  // it's in, and also the way in which the data is presented
  
  XrefObject _xref;

  // A pointer to the module that owns this value.
  Module *cpu;

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
  static Boolean * New(const char *_name, const char *pValue, const char *desc);
  virtual ~Boolean();

  string toString();
  string toString(char* format);
  static string toString(bool value);
  static string toString(char* format, bool value);

  virtual void get(bool &b);
  virtual void get(int &i);
  virtual void get(char *, int len);
  virtual void get(Packet &);

  virtual void set(bool);
  virtual void set(Value *);
  virtual void set(const char *cP,int len=0);
  virtual void set(Packet &);

  bool getVal() { return value; }

  static Boolean* Boolean::typeCheck(Value* val, string valDesc);
  virtual bool compare(ComparisonOperator *compOp, Value *rvalue);

  virtual Value *copy() { return new Boolean(value); }

  /// copy the object value to a user char array
  virtual char *toString(char *, int len);
  virtual char *toBitStr(char *, int len);

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
  static Integer *  New(const char *_name, const char *pValue, const char *desc);

  virtual ~Integer();

  virtual string toString();
  string toString(char* format);
  static string toString(gint64 value);
  static string toString(char* format, gint64 value);

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

  virtual Value *copy() { return new Integer(value); }
  /// copy the object value to a user char array
  virtual char *toString(char *, int len);
  virtual char *toBitStr(char *, int len);

  static Integer* Integer::typeCheck(Value* val, string valDesc);
  static Integer* Integer::assertValid(Value* val, string valDesc, gint64 valMin);
  static Integer* Integer::assertValid(Value* val, string valDesc, gint64 valMin, gint64 valMax);
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
  static Float * New(const char *_name, const char *pValue, const char *desc);
  virtual ~Float();

  virtual string toString();
  string toString(char* format);
  static string toString(double value);
  static string toString(char* format, double value);

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

  virtual Value *copy() { return new Float(value); }
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
  string toString(char* format);

  virtual unsigned int get_leftVal();
  virtual unsigned int get_rightVal();

  virtual void set(Value *);

  virtual Value *copy() { return new AbstractRange(left,right); }
  /// copy the object value to a user char array
  virtual char *toString(char *, int len);

  static AbstractRange* AbstractRange::typeCheck(Value* val, string valDesc);
  virtual bool compare(ComparisonOperator *compOp, Value *rvalue);

private:
  unsigned int left;
  unsigned int right;
};


char * TrimWhiteSpaceFromString(char * pBuffer);
char * UnquoteString(char * pBuffer);
string &toupper(string & sStr);

#endif // __VALUE_H__
