
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


#include <typeinfo>
#include <stdio.h>
#include <ctype.h>
#include "gpsim_object.h"

//========================================================================

gpsimObject::gpsimObject()
: cpDescription(0)
{
}

gpsimObject::gpsimObject(const char *_name, const char *desc)
  : cpDescription(desc)
{
  if (_name)
    name_str = _name;
}

gpsimObject::~gpsimObject()
{
}

// The 'type' of any Viewable object is equivalent to the class name.
string gpsimObject::showType()
{
  const char* name;
  
  name = typeid(*this).name();
  
  /* Unfortunately, the class name string returned by typeid() is
   * implementation-specific.  If a particular compiler produces
   * ugly output, this is your chance to clean it up. */
#if defined __GNUC__
  /* GNU C++ puts the length of the class name in front of the
     actual class name.  We will skip over it for clarity. */
  while (isdigit(*name)) {
    name++;
  }
#elif defined _MSC_VER
  /*
  From Visual C++ on line documentation
  The type_info::name member function returns a const char* to
  a null-terminated string representing the human-readable name
  of the type. The memory pointed to is cached and should never
  be directly deallocated.
  */
  // Skip over the word 'class '.
  name += 6;
#else
  #warning --->You might want to clean up the result of typeid() here...
#endif

  return string(name);
}


string gpsimObject::show()
{
  return showType() + ":" + toString();
}

void gpsimObject::new_name(const char *s)
{
  if(s)
    name_str = string(s);
}

void gpsimObject::new_name(string &new_name)
{
  name_str = new_name;
}

char *gpsimObject::name(char *return_str, int len)
{
  if(return_str)
    snprintf(return_str,len,"%s",name_str.c_str());

  return return_str;
}

char *gpsimObject::toString(char *return_str, int len)
{
  if(return_str)
    *return_str = 0;

  return return_str;
}
char *gpsimObject::toBitStr(char *return_str, int len)
{
  if(return_str)
    *return_str = 0;

  return return_str;
}

string &gpsimObject::name(void) const
{
  return (string &)name_str;
}

string gpsimObject::toString()
{
  return showType();
}

// If derived classes don't redefine set_break and clear_break then
// the default behavior is to not support these capabilities.
int gpsimObject::set_break(ObjectBreakTypes bt, Expression *expr)
{
  //cout << showType() << " objects do not support break points\n";
  return -1;
}
int gpsimObject::clear_break()
{
  //cout << showType() << " objects do not support break points\n";
  return -1;
}

string gpsimObject::description()
{
  if(cpDescription)
    return string(cpDescription);
  else
    return string("no description");
}

void  gpsimObject::set_description(const char *new_description)
{
  cpDescription = new_description;
}
