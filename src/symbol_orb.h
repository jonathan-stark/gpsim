/*
   Copyright (C) 1999 T. Scott Dattalo

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


#ifndef __SYMBOL_ORB_H__
#define __SYMBOL_ORB_H__

//------------------------------------
// *** KNOWN CHANGE ***
//
//  THIS FILE IS (OR SHOULD BE) TEMPORARY
//
// Here are a set of support functions called by the CLI. Eventually
// these will be replaced with an indirect interface such as CORBA.


void symbol_dump_all(void);
void symbol_dump_one(char *sym_name);
void symbol_add_one(pic_processor *cpu, char *sym_name, char *sym_type, int value);
int  get_symbol_value(char *sym, int *sym_value);
void print_symbol(char *sym);
void update_symbol_value(char *sym, int sym_value);

#endif
