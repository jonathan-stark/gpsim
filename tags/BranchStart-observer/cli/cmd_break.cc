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


#include <iostream.h>
#include <iomanip.h>
#include <string>

#include "command.h"
#include "cmd_break.h"

#include "../src/pic-processor.h"
#include "../src/symbol_orb.h"

cmd_break c_break;

#define CYCLE       1
#define EXECUTION   2
#define WRITE       3
#define READ        4
#define WRITE_VALUE 5
#define READ_VALUE  6
#define WDT         7

static cmd_options cmd_break_options[] =
{
  "c",   CYCLE,       OPT_TT_BITFLAG,
  "e",   EXECUTION,   OPT_TT_BITFLAG,
  "w",   WRITE,       OPT_TT_BITFLAG,
  "r",   READ,        OPT_TT_BITFLAG,
  "wv",  WRITE_VALUE, OPT_TT_BITFLAG,
  "rv",  READ_VALUE,  OPT_TT_BITFLAG,
  "wdt", WDT,         OPT_TT_BITFLAG,
  NULL,0,0
};


cmd_break::cmd_break(void)
{ 
  name = "break";

    brief_doc = string("Set a break point");

    long_doc = string ("break [c e | w | r | wv | rv | wdt [location] [value] ]\n\n\
\toptions:\n\
\t\tc   - cycle\n\
\t\te   - execution\n\
\t\tw   - write\n\
\t\tr   - read\n\
\t\twv  - write value\n\
\t\trv  - read value\n\
\t\twdt - wdt timeout\n\
\t\t    - no argument, display the break points that are set.
\texamples:\n\
\t\tbreak e 0x20     // set an execution break point at address 0x20\n\
\t\tbreak wv 0x30 0  // break if a zero is written to register 0x30\n\
\t\tbreak c 1000000  // break on the one million'th cycle\n\
\t\tbreak            // display all of the break points\n\
\n");

  op = cmd_break_options; 
}


void cmd_break::list(void)
{
  if(cpu)
    bp.dump();
}

const char *TOO_FEW_ARGS="missing register or location\n";
const char *TOO_MANY_ARGS="too many arguments\n";

void cmd_break::set_break(int bit_flag)
{

  if(cpu==NULL)
    return;

  if(bit_flag != WDT)
    {
      cout << TOO_FEW_ARGS;
      return;
    }

  int b = bp.set_wdt_break(cpu);
  if(b < MAX_BREAKPOINTS)
    cout << "break when wdt times out.  " <<
      "bp#: " << b << '\n';

}


void cmd_break::set_break(int bit_flag, guint64 value)
{

  if(cpu==NULL)
    return;

  int b;

  switch(bit_flag) {

  case CYCLE:
    b = bp.set_cycle_break(cpu,value);

    if(b < MAX_BREAKPOINTS)
      cout << "break at cycle: " << value << " break #: " <<  b << '\n';
    else
      cout << "failed to set cycle break\n";

    break;

  case EXECUTION:
    b = bp.set_execution_break(cpu, value);
    if(b < MAX_BREAKPOINTS)
      {
	cout << "break at address: " << value << " break #: " << b << '\n';
      }
    else
      cout << "failed to set execution break (check the address)\n";

    break;

  case WRITE:

    b = bp.set_write_break(cpu, value);
    if(b < MAX_BREAKPOINTS)
      cout << "break when register " << value << " is written. bp#: " << b << '\n';

    break;

  case READ:
    b = bp.set_read_break(cpu, value);
    if(b < MAX_BREAKPOINTS)
      cout << "break when register " << value << " is read.\n" << 
	"bp#: " << b << '\n';

    break;

  case WRITE_VALUE:
  case READ_VALUE:
    cout << TOO_FEW_ARGS;
    break;
  case WDT:
    cout << TOO_MANY_ARGS;
  }
}

void cmd_break::set_break(int bit_flag, int reg, int value)
{

  if(cpu==NULL)
    return;

  int b;

  switch(bit_flag) {

  case CYCLE:
  case EXECUTION:
  case WRITE:
  case READ:
  case WDT:
    cout << TOO_MANY_ARGS;
    break;

  case READ_VALUE:

    b = bp.set_read_value_break(cpu, reg,value);

    if(b < MAX_BREAKPOINTS)
      cout << "break when " << value << " is read from register " << reg << '\n' << 
	"bp#: " << b << '\n';

	
    break;

  case WRITE_VALUE:

    b = bp.set_write_value_break(cpu, reg,value);

    if(b < MAX_BREAKPOINTS)
      cout << "break when " << value << " is written to register " << reg << '\n' << 
	"bp#: " << b << '\n';

  }
}

void cmd_break::set_break(int bit_flag, char *sym, int value)
{
  int sym_value;

  if(get_symbol_value(sym,&sym_value))
    {
      cout << '`' << sym << '\'' << " was not found in the symbol table\n";
      return;
    }

  set_break(bit_flag,sym_value,value);

}

void cmd_break::set_break(int bit_flag, char *sym)
{
  int sym_value;

  if(get_symbol_value(sym,&sym_value))
    {
      cout << '`' << sym << '\'' << " was not found in the symbol table\n";
      return;
    }

  set_break(bit_flag,sym_value);

}
