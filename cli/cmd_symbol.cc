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


#include <iostream>
#include <iomanip>
#include <string>

#include "command.h"
#include "cmd_symbol.h"
#include "../src/symbol_orb.h"

cmd_symbol c_symbol;

static cmd_options cmd_symbol_options[] =
{
  {0,0,0}
};


cmd_symbol::cmd_symbol(void)
{ 
  name = "symbol";

  brief_doc = string("Add or display symbols");

  long_doc = string ("symbol [symbol_name [symbol_type value]]\n"
    "\n"
    "\tIf no options are supplied, the entire symbol table will be\n"
    "\tdisplayed. If only the symbol_name is provided, then only\n"
    "\tthat symbol will be displayed. If both the symbol_name and\n"
    "\tthe symbol_type are supplied, then a new symbol will be\n"
    "\tadded to the symbol table.\n"
    "\n"
    "\tValid symbol types:\n"
    "\t  ioport | iop, constant\n"
    "\n"
    "Examples:\n"
    "\tsymbol            // display the symbol table\n"
    "\tsymbol george constant 42  // create a new constant symbol\n"
    "\t                           // named george and equal to 42\n");

  op = cmd_symbol_options; 
}


void cmd_symbol::dump_all(void)
{
  symbol_dump_all();
}

void cmd_symbol::dump_one(char *sym_name)
{
  symbol_dump_one(sym_name);
}

void cmd_symbol::add_one(char *sym_name, char *sym_type, int value)
{
  symbol_add_one(cpu, sym_name,sym_type,value);
}

