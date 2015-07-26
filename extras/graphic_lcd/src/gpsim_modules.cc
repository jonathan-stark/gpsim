/*
   Copyright (C) 2005 T. Scott Dattalo

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

#include <config.h>

/*
 */

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include <src/modules.h>

#ifdef HAVE_GUI
#include "glcd_100X32_sed1520.h"
#include "osram.h"
#endif


static Module_Types available_modules[] =
{
#ifdef HAVE_GUI
  { {"LCD100X32", "LCD100X32"},   gLCD_100X32_SED1520::construct },
  { {"OSRAM128X64", "OSRAM128X64"},   OSRAM::PK27_Series::construct },
#endif

  // No more modules
  { {0,0},0}
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/********************************************************************************
 * get_mod_list - Display all of the modules in this library.
 *
 * This is a required function for gpsim compliant libraries.
 */

Module_Types * get_mod_list()
{
  return available_modules;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

