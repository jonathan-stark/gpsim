/*
   Copyright (C) 2001 T. Scott Dattalo

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

#ifndef __ATTRIBUTE_H__
#define __ATTRIBUTE_H__

//-------------------------------------------------------------------

#include <list>
#include <iostream>

#include "xref.h"

enum  ATTRIBUTE_VALUE_TYPES {
  ATTRIBUTE_INT,
  ATTRIBUTE_FLOAT,
  ATTRIBUTE_STRING
};

//-------------------------------------------------------------------
//  AttributeType - a class that allows overloading of various data types
//
class Attribute {
public:

  char *name;    // 

  ATTRIBUTE_VALUE_TYPES type;


  ATTRIBUTE_VALUE_TYPES isa(void) { return type; }
  Attribute(void);
  Attribute(char *init_name);
  virtual void new_name(char *);
  virtual char *get_name(void) { return(name);};

  virtual void   set(int i)    = 0;
  virtual void   set(double f) = 0;
  virtual void   set(char *s)  = 0;
  virtual void   set(char *s,       int i)        = 0;
  virtual char  *sGet(char *buffer, int buf_size) = 0;
  virtual double fGet(void) { return 0.0; }
  virtual int    nGet(void) { return 0; }


};

class IntAttribute : public Attribute
{
 public:

  int  value;

  virtual void set(int i);
  virtual void set(double f);
  virtual void set(char *s);
  virtual void set(char *s, int i);
  virtual double fGet(void) { return (double)value; }
  virtual int    nGet(void) { return value; }
  virtual char *sGet(char *buffer, int buf_size);
};

class FloatAttribute : public Attribute
{
 public:

  double  value;

  virtual void set(int i);
  virtual void set(double f);
  virtual void set(char *s);
  virtual void set(char *s, int i);
  virtual double fGet(void) { return value; }
  virtual int    nGet(void) { return (int) value; }
  virtual char *sGet(char *buffer, int buf_size);
  FloatAttribute(char *init_name, double _v) : Attribute(init_name), value(_v) { }
  FloatAttribute(void) { }
};

class StringAttribute : public Attribute
{
 public:

  char *value;
  int  nvalue;

  virtual void set(int i);
  virtual void set(double f);
  virtual void set(char *s);
  virtual void set(char *s, int i);
  virtual char *sGet(char *buffer, int buf_size);
};

#endif // __ATTRIBUTE_H__
