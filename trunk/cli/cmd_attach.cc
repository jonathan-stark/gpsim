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
#include <vector>

#include "command.h"
#include "cmd_attach.h"
#include "../src/stimuli.h"

//#include "../src/stimulus_orb.h"

cmd_attach attach;

static cmd_options cmd_attach_options[] =
{
  {0,0,0}
};


cmd_attach::cmd_attach(void)
{ 
  name = "attach";

  brief_doc = string("Attach stimuli to nodes");

  long_doc = string ("attach node1 stimulus1 [stimulus2 stimulu_N]\n"
    "\t  attach is used to define the connections between stimuli and nodes.\n"
    "\tAt least one node and one stimulus must be specified. If more stimuli\n"
    "\tare specified then they will all be attached to the node.\n"
    "\n"
    "\texamples:\n"
    "\n"
    "\tnode pin2pin_test                  // Define a new node.\n"
    "\tattach pin2pin_test porta4 portb0  // Connect two I/O pins to the node.\n"
    "\tnode                               // Display the new \"net list\".\n");

  op = cmd_attach_options; 
}


void cmd_attach::attach(list <string> * strings)
{
  stimuli_attach(strings);
}

void cmd_attach::attach(SymbolList_t *symbols)
{
  stimuli_attach(symbols);
}
