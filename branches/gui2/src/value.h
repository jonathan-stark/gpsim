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
#include "xref.h"

class ComparisonOperator;

//------------------------------------------------------------------------
//
// Values
//
// Everything that can hold a value is derived from the Value class.
// The primary purpose of this is to provide external objects (like
// the gui) an abstract way of getting the value of diverse things
// like registers, program counters, cycle counters, etc.
//
// In addition, expressions of Values can be created and operated
// on.
//

class Value : public gpsimObject
{
 public:
  Value();
  Value(bool isConstant);

  virtual ~Value();
  virtual int getAsInt();
  virtual double getAsDouble();
  virtual unsigned int get_leftVal() {return getAsInt();}
  virtual unsigned int get_rightVal() {return getAsInt();}
  bool isConstant();

  virtual bool compare(ComparisonOperator *compOp, Value *rvalue);
  /*
  virtual bool operator&&(Value *);
  virtual bool operator||(Value *);
  */
 private:
  bool constant;
};

//========================================================================
//
// gpsimValue is a Value object that is unique to the simulator.

class gpsimValue : public Value {
 public:

  gpsimValue(void);
  gpsimValue(Processor *);
  virtual ~gpsimValue();

  // Access functions
  virtual unsigned int get(void)
    {
      return get_value();
    }

  virtual void put(unsigned int new_value)
    {
      put_value(new_value);
    }

  
  // put_value is the same as put(), but some extra stuff like
  // interfacing to the gui is done. (It's more efficient than
  // burdening the run time performance with (unnecessary) gui
  // calls.)
  //

  virtual void put_value(unsigned int new_value) = 0;

  // A variation of get(). (for register values, get_value returns
  // just the value whereas get() returns the value and does other
  // register specific things like tracing.

  virtual unsigned int get_value(void) = 0;

  virtual void set_cpu(Processor *new_cpu)
    {
      cpu = new_cpu;
    }

  Processor *get_cpu(void)
    {
      return cpu;
    }

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

  // A pointer to the processor that owns this value.
  // FIXME - should this be a Module pointer instead?
  Processor *cpu;

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
  virtual bool compare(ComparisonOperator *compOp, Value *rvalue);

  /*
  bool operator&&(Value *);
  bool operator||(Value *);
  bool operator==(Value *);
  bool operator!=(Value *);
  */
private:
  bool value;
};


/*****************************************************************/
class Integer : public Value {

public:
	
  Integer(gint64 new_value) : Value() { value = new_value;};
  Integer(gint64 newValue, bool isConstant);
  virtual ~Integer() {}

  string toString();
  string toString(char* format);
  static string toString(gint64 value);
  static string toString(char* format, gint64 value);
  gint64 getVal();

  virtual int getAsInt() { return (int)value; }
  virtual double getAsDouble() { return (double)value;}
  virtual void put(gint64 v) {value = v; }

  static Integer* Integer::typeCheck(Value* val, string valDesc);
  static Integer* Integer::assertValid(Value* val, string valDesc, gint64 valMin);
  static Integer* Integer::assertValid(Value* val, string valDesc, gint64 valMin, gint64 valMax);
  virtual bool compare(ComparisonOperator *compOp, Value *rvalue);

  /*
  virtual bool operator<(Value *v);
  virtual bool operator>(Value *);
  virtual bool operator<=(Value *);
  virtual bool operator>=(Value *);
  virtual bool operator==(Value *);
  virtual bool operator!=(Value *);
  virtual bool operator&&(Value *);
  virtual bool operator||(Value *);
  */
private:
  gint64 value;
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
  virtual bool compare(ComparisonOperator *compOp, Value *rvalue);

  /*
  virtual bool operator&&(Value *);
  virtual bool operator||(Value *);
  */
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
  virtual bool compare(ComparisonOperator *compOp, Value *rvalue);

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

  virtual unsigned int get_leftVal();
  virtual unsigned int get_rightVal();

  static AbstractRange* AbstractRange::typeCheck(Value* val, string valDesc);
  virtual bool compare(ComparisonOperator *compOp, Value *rvalue);

private:
  unsigned int left;
  unsigned int right;
};

/*****************************************************************/
/*
class gpsimSymbol : public Value {

public:
	
  gpsimSymbol(symbol *);
  gpsimSymbol(symbol *, bool isConstant);

  virtual ~gpsimSymbol();


  string toString();

  virtual int getAsInt();
  virtual double getAsDouble();

  virtual symbol *get_sym() { return sym; }
private:
  symbol *sym;
};
*/

#endif // __VALUE_H__
