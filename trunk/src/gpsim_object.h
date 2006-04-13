
/*
   Copyright (C) 2004 T. Scott Dattalo

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


#ifndef __GPSIM_OBJECT_H__
#define __GPSIM_OBJECT_H__

#include <string>
using namespace std;

class BreakType;

/// gpsimObject - base class for most of gpsim's objects
/// 

class Expression;

class gpsimObject {
 public:

  gpsimObject();
  gpsimObject(const char *_name, const char *desc=0);
  virtual ~gpsimObject();

  /// Get the name of the object
  virtual string &name(void) const;

  /// copy the name to a user char array
  virtual char *name(char *, int len);

  /// copy the object value to a user char array
  virtual char *toString(char *, int len);
  virtual char *toBitStr(char *, int len);

  /// Assign a new name to the object
  virtual void new_name(const char *);
  virtual void new_name(string &);

  /// description - get a description of this object. If the object has 
  /// a name, then 'help value_name' at the command line will display
  /// the description.

  virtual string description();
  void set_description(const char *);

  /// Access object-specific information
  string show();
  string showType();
  virtual string toString();


  // Breakpoint types supported by Value
  enum ObjectBreakTypes {
    eBreakAny,
    eBreakWrite,
    eBreakRead,
    eBreakExecute
  };

  /// breakpoints
  /// set a break point on a gpsim object. The BreakType specifies the
  /// the condition for which the break will trigger when this value
  /// is accessed. In addition, the optional expr is a boolean expression
  /// that is evaluated when the Object is accessed. The expression must
  /// evaluate to true for the break to trigger. If the break is successfully
  /// set then a non-negative number (the break point number) will be returned.
  /// If the break fails, then -1 is returned.

  virtual int set_break(ObjectBreakTypes bt=eBreakAny, Expression *expr=0);
  virtual int clear_break();

protected:

  string  name_str;               // A unique name to describe the object
  const char *cpDescription;      // A desciption of the object

};

//------------------------------------------------------------------------
// BreakTypes
//
class BreakType
{
public:
  BreakType(int _type) 
    : m_type(_type)
  {
  }
  virtual int type()
  {
    return m_type;
  }
protected:
  int m_type;
};
#endif //  __GPSIM_OBJECT_H__
