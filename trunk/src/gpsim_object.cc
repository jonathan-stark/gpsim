
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
{
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
  #pragma message("--->You might want to clean up the result of typeid() here...")
#else
  #warning --->You might want to clean up the result of typeid() here...
#endif

  return string(name);
}


string gpsimObject::show()
{
  return showType() + ":" + toString();
}

void gpsimObject::new_name(char *s)
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

string &gpsimObject::name(void)
{
  return name_str;
}

string gpsimObject::toString()
{
  return showType();
}
