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

#ifndef __INTERFACE_H__
#define __INTERFACE_H__

/*
 * interface.h
 */

#include <stdio.h>
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

#include "cmd_gpsim.h"
extern unsigned int gpsim_is_initialized;
LIBGPSIM_EXPORT void initialization_is_complete(void);

#define INVALID_VALUE 0xffffffff



void gpsim_set_bulk_mode(int flag);
extern const char *get_dir_delim(const char *path);
  
LIBGPSIM_EXPORT int initialize_gpsim_core();


#endif /* __INTERFACE_H__ */
