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
#include "cmd_disasm.h"

#include "../src/pic-processor.h"

cmd_disassemble disassemble;

static cmd_options cmd_disassemble_options[] =
{
  NULL,0,0
};


cmd_disassemble::cmd_disassemble(void)
{ 
  name = "disassemble";

    brief_doc = string("Disassemble the current cpu");

    long_doc = string ("\ndisassemble [[start | length] [end]]\n\n\
\t no arguments:  disassembles 10 instructions before and 5 after the pc.\n\
\t one argument:  disassemble [length] instructions after the pc.\n\
\ttwo arguments:  disassemble from [start] to [end].\n\
\n\
");

  op = cmd_disassemble_options; 
}

/*
void cmd_disassemble::disassemble(void)
{

  if(cpu)
    cpu->disassemble(cpu->pc.value - 10, cpu->pc.value + 5);

}

void cmd_disassemble::disassemble(int length)
{

  if(cpu)
    cpu->disassemble(cpu->pc.value, cpu->pc.value + length);

}
*/

void cmd_disassemble::disassemble(int start, int end)
{

  if(cpu)
    cpu->disassemble(cpu->pc->value + start, cpu->pc->value + end);

}
