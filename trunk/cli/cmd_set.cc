/*
   Copyright (C) 1999-2000 T. Scott Dattalo

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
#include <stdio.h>

#include "command.h"
#include "cmd_set.h"
#include "expr.h"
#include "errors.h"
#include "input.h"

#include "../src/pic-processor.h"
#include "../src/interface.h"


static int radix = 0;   // FIXME

cmd_set c_set;

enum {
  SET_VERBOSE,
  SET_RADIX,
  SET_GUI_UPDATE
};

static cmd_options cmd_set_options[] =
{
  {"r",          SET_RADIX,      OPT_TT_NUMERIC},
  {"radix",      SET_RADIX,      OPT_TT_NUMERIC},
  {"v",          SET_VERBOSE,    OPT_TT_BITFLAG},
  {"verbose",    SET_VERBOSE,    OPT_TT_BITFLAG},
  {"gui_update", SET_GUI_UPDATE, OPT_TT_BITFLAG},
  {0,0,0}
};


cmd_set::cmd_set(void)
{ 
  name = "set";

  brief_doc = string("display and control gpsim behavior flags");

  long_doc = string ("set\n"
    "\twith no options, set will display the state of all of gpsim's\n"
    "\tbehavior flags. Use this to determine the flags that may be\n"
    "\tmodified.\n"
    "\n");

  op = cmd_set_options; 
}



void cmd_set::set(void)
{

  cout << "r | radix = " << radix << " (not fully functional)\n";
  cout << "v | verbose =  " << verbose << '\n';
  cout << "gui_update = " << gi.gui_update_rate << '\n';
}

void cmd_set::set(int bit_flag, Expression *expr)
{
  int number=1;

  if(expr) {
    try
      {
	Value *v = expr->evaluate();
	if(v) {
	  number = v->getAsInt();
	  delete v;
	}
	delete expr;
      }
    catch (Error *err)
      {
	if(err)
	  cout << "ERROR:" << err->toString() << endl;
	delete err;

	return;
      }
  }

  switch(bit_flag) {
  case SET_VERBOSE:
    verbose = number;
    break;
  case SET_GUI_UPDATE:
    gi.set_update_rate(number);
    break;

  default:
    cout << " Invalid set option\n";
  }
}

