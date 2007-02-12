/*
   Copyright (C) 1998 T. Scott Dattalo

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
