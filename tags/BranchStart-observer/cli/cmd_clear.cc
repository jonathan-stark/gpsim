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
#include "cmd_clear.h"

#include "../src/pic-processor.h"

cmd_clear clear;

static cmd_options cmd_clear_options[] =
{
  NULL,0,0
};


cmd_clear::cmd_clear(void)
{ 
  name = "clear";

    brief_doc = string("Remove a break point");

    long_doc = string ("clear bp_number\n\
where bp_number is the number assigned to the break point\n\
when it was created. (type \"break\" without any arguments to\n\
display the currently set break points.\n\
");

  op = cmd_clear_options; 
}


void cmd_clear::clear(int bp_number)
{

  bp.clear(bp_number);

}

