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
#include "../src/stimuli.h"
#include "cmd_attach.h"

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

  long_doc = string ("attach node1 stimulus_1 [stimulus_2 stimulu_N]\n"
    "\t  attach is used to define the connections between stimuli and nodes.\n"
    "\tAt least one node and one stimulus must be specified. If more stimuli\n"
    "\tare specified then they will all be attached to the node.\n\n"
    "\tstimulus_n                 May be one of four forms:\n"
    "\t  pin(<number>) or pin(<symbol>)\n"
    "\t             The single argument form refers to a pin of the currently\n"
    "\t             active cpu. The <number> argument defined the pin number\n"
    "\t             of active cpu. The <symbol> argument refers to the\n"
    "\t             name of the pin. If the <symbol> is scoped to a specific\n"
    "\t             attribute (i.e. MyProc.PORTA0) the pin of the specified\n"
    "\t             module will be attached.\n"
    "\t  pin(<module>, <number>) or pin(<module>, <symbol>)\n"
    "\t             The dual argument form refers to the pin of the specified\n"
    "\t             module.\n"
    "\t  <module>   Name of the module or string variable that contains the\n"
    "\t             module name.\n"
    "\t  <symbol>   A symbolic integer constant representing the pin number.\n"
    "\t  <number>   A literal integer value of the pin number.\n"
    "\n"
    "\texamples:\n"
    "\n"
    "\t  processor p16f627 P16\n"
    "\t  node pin2pin_test            // Define a new node.\n"
    "\t  attach pin2pin_test pin(porta4) pin(P16, portb0) // Different ways to \n"
    "\t  attach pin2pin_test pin(4) pin(0)                // connect two I/O\n"
    "\t  attach pin2pin_test pin(P16,portb0)              // pins to the node.\n"
    "\t  attach pin2pin_test pin(P16,0)\n"
    "\t  node                         // Display the new \"net list\".\n\n"
    "\tdeprecated:\n"
    "\t  attach pin2pin_test porta4 portb0\n"
    );

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

void cmd_attach::attach(Value *pNode, PinList_t *pPinList)
{
  stimuli_attach(pNode, pPinList);
}
