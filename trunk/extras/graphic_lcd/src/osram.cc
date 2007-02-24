/*
   Copyright (C) 2007 T. Scott Dattalo

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


#include "osram.h"
#include "glcd.h"
#include "ssd0323.h"

#include <gpsim/stimuli.h>
#include <gpsim/ioports.h>
#include <gpsim/packages.h>
#include <gpsim/symbol.h>
#include <gpsim/trace.h>
#include <gpsim/gpsim_interface.h>


#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif



//------------------------------------------------------------------------
OSRAM_128064PK27_Series::OSRAM_128064PK27_Series(const char *_new_name)
  : gLCD_Module(_new_name,"OSRAM 128X64 Graphics OLED module",128,64)
{

}


OSRAM_128064PK27_Series::~OSRAM_128064PK27_Series()
{
}
