/*
   Copyright (C) 1998 T. Scott Dattalo

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

#ifndef __XREF_H__
#define __XREF_H__

//-------------------------------------------------------------------

#include <list>
using namespace std;

class gpsimObject;

class XrefObject {
 private:
  gpsimObject *data;
  list<void*> xrefs;
 public:
  XrefObject();
  XrefObject(gpsimObject *value);
  virtual ~XrefObject();
    
  virtual void _add(void *xref);
  virtual void clear(void *xref);
  virtual void _update();
  virtual int get_val(void);
  virtual void assign_data(gpsimObject *);
};

#endif
