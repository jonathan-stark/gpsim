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
#include "viewable.h"


#if !defined(__EXPR_H__)
#define __EXPR_H__
using namespace std;

class Value : public Viewable
{
 public:
  Value();
  virtual ~Value();
  virtual int get()
    {
      return 0;
    }
  virtual void put(int v)
    {
    }
};


class Expression : public Viewable
{

 public:

  Expression();

  virtual ~Expression();

  virtual Value* evaluate()=0;

};

class Integer : public Value {

public:
	
  Integer(int new_value) : Value() { value = new_value;};
  virtual ~Integer() {}

  string toString();
  string toString(char* format);
  static string toString(int value);
  static string toString(char* format, int value);

  virtual int get() { return value; }
  virtual void put(int v) {value = v; }
private:
  int value;
};

//-----------------------------------------------------------------
class LiteralInteger : public Expression {

public:
  LiteralInteger(Integer* value);
  virtual ~LiteralInteger();
  virtual Value* evaluate() {
    return value;
  }
  string toString()
  {
    return value->toString();
  }

private:
  Integer* value;
};


#endif // __EXPR_H__
