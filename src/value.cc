/*
   Copyright (C) 1998-2003 Scott Dattalo

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


#include <stdio.h>
#include <iostream>
#include <iomanip>


#include "value.h"

gpsimValue::gpsimValue(void)
{
  cpu = 0;
}

gpsimValue::gpsimValue(Processor *_cpu)
  : cpu(_cpu)
{
}

gpsimValue::~gpsimValue(void)
{
}

void gpsimValue::update(void)
{
  _xref._update();
}

void gpsimValue::add_xref(void *an_xref)
{
  _xref._add(an_xref);
}

void gpsimValue::remove_xref(void *an_xref)
{
  _xref.clear(an_xref);
}

string gpsimValue::toString()
{
  char buff[64];
  snprintf(buff,sizeof(buff), " = 0x%x",get_value());
  string s = name() + string(buff);
  return s;
}
