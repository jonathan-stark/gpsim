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

#include <iostream>
#include <stdio.h>

#include <src/modules.h>

#ifdef HAVE_GUI
#include "glcd_100X32_sed1520.h"
#include "osram.h"
#endif


Module_Types available_modules[] =
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
 * mod_list - Display all of the modules in this library.
 *
 * This is a required function for gpsim compliant libraries.
 */

  Module_Types * get_mod_list(void)
  {

    return available_modules;

  }
/********************************************************************************
 * mod_list - Display all of the modules in this library.
 *
 * This is a required function for gpsim compliant libraries.
 */

void mod_list(void)
{

  unsigned int number_of = sizeof(available_modules) / sizeof(Module_Types);
  unsigned int i,j,l;
  unsigned int k,longest;

  for(i=0,longest=0; i<number_of; i++)
    {
      k = (unsigned int)strlen(available_modules[i].names[1]);
      if(k>longest)
	longest = k;
    }

  k=0;
  do
    {

      for(i=0; (i<4) && (k<number_of); i++)
	{
	  cout << available_modules[k].names[1];
	  if(i<3)
	    {
	      l = longest + 2 - (unsigned int)strlen(available_modules[k].names[1]);
	      for(j=0; j<l; j++)
		cout << ' ';
	    }
	  k++;
	}
      cout << '\n';
    } while (k < number_of);

}


/************************************************************
 *
 * _init() - this is called when the library is opened.
 */

void init(void)
{

  //cout << "gpsim modules has been opened\n";
  printf("%s\n",__FUNCTION__);
}

/************************************************************
 *
 * _fini() - this is called when the library is closed.
 */

void fini(void)
{

  //cout << "gpsim modules has been closed\n";
  printf("%s\n",__FUNCTION__);
}


#ifdef __cplusplus
}
#endif /* __cplusplus */

