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

#ifndef __INTERFACE_H__
#define __INTERFACE_H__

/*
 * interface.h
 *
 * Here are (hopefully) all of the definitions needed for an
 * external program (such as a gui) to interface with gpsim
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <glib.h>
#include "modules.h"
#include "symbol.h"

typedef enum _REGISTER_TYPE
{
    REGISTER_RAM,
    REGISTER_EEPROM
} REGISTER_TYPE;

struct file_context {
  char *name;           /* file name */
  FILE *file_ptr;
  int *line_seek;       /* an array of offsets into the file that point to
			 *  the start of the source lines. */
  int max_line;
};

extern int verbose;
extern unsigned int gpsim_is_initialized;
void  initialization_is_complete(void);

#define INVALID_VALUE 0xffffffff

  /*********************************************************************
   * 
   * What follows are a whole bunch of functions that provide access to
   * gpsim's innards. This is what the gui uses to interface to gpsim.
   * All of the functions here have "C" signatures, which is to say
   * they may be called from "C" code as well as from C++
   */


#define SYMBOL_NAME_LEN 32
typedef struct _sym
{
    enum SYMBOL_TYPE type;
    char *name;
    int value;
} sym;




  int gpsim_open(unsigned int processor_id, const char *file);
  void gpsim_set_bulk_mode(int flag);
  



#endif /* __INTERFACE_H__ */
