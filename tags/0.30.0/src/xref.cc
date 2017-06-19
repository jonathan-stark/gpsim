/*
   Copyright (C) 2004 T. Scott Dattalo

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

#include "../config.h"
#include "xref.h"
#include "gpsim_interface.h"
#include "value.h"

//-------------------------------------------------------------------
XrefObject::XrefObject()
{
    data=0;
}
XrefObject::XrefObject(gpsimObject *value)
{
    data=value;
}

XrefObject::~XrefObject()
{
    list<void*>::iterator ioi;

    ioi=xrefs.begin();

    while((ioi=xrefs.begin()) != xrefs.end())
    {
      XrefObject *xref = (XrefObject *) *ioi;
      clear(xref);
      delete xref;
    }
    return;

}

void *XrefObject::first_xref()
{
    list<void*>::iterator ioi;

    if (!xrefs.empty())
    {
      ioi=xrefs.begin();
      return(*ioi);
    }
    else
      return 0;
}
void XrefObject::_add(void *xref)
{
  xrefs.push_back(xref);
}

void XrefObject::clear(void *xref)
{
    xrefs.remove(xref);
}

void XrefObject::_update()
{
  list<void*>::iterator ioi;

  ioi=xrefs.begin();
  for(;ioi!=xrefs.end();++ioi)
  {
    gpointer *xref = (gpointer *) *ioi;

    gi.update_object(xref,get_val());
  }
}


int XrefObject::get_val(void)
{

  if(data)
    return data->get_value();

  return 0;
}

void XrefObject::assign_data(gpsimObject *new_data)
{

  data = new_data;
  
}
