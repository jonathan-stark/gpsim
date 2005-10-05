
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


/// gpsimObject - base class for most of gpsim's objects
/// 


class gpsimObject {
 public:

  gpsimObject();
  virtual ~gpsimObject();

  virtual string &name(void) const;

  /// copy the name to a user char array
  virtual char *name(char *, int len);

  /// copy the object value to a user char array
  virtual char *toString(char *, int len);
  virtual char *toBitStr(char *, int len);

  virtual void new_name(const char *);
  virtual void new_name(string &);

  string show();
  string showType();
  virtual string toString();


protected:

  string  name_str;               // A unique name to describe the Value


};

#endif //  __GPSIM_OBJECT_H__
