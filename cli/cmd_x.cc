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
#include "cmd_x.h"
#include "cmd_dump.h"

#include "../src/pic-processor.h"
#include "../src/symbol_orb.h"


cmd_x c_x;

static cmd_options cmd_x_options[] =
{
  0,0,0
};


cmd_x::cmd_x(void)
{ 
  name = "x";

  brief_doc = string("examine and/or modify memory");

  long_doc = string ("\nx [file_register] [new_value]\n\
\toptions:\n\
\t\tfile_register - ram location to be examined or modified.\n\
\t\tnew_value - the new value written to the file_register.\n\
\t\tif no options are specified, then the entire contents\n\
\t\tof the file registers will be displayed (dump).\n\
");

  op = cmd_x_options; 
}


void cmd_x::x(void)
{

  dump.dump(cmd_dump::DUMP_RAM);
  dump.dump(cmd_dump::DUMP_SFRS);
  if(cpu)
    cpu->dump_registers();

}

void cmd_x::x(int reg)
{


  if(!cpu)
    return;

  if(reg<0 || (reg >= cpu->register_memory_size()) )
    {
      cout << "bad file register\n";
      return;
    }

  int ov = cpu->registers[reg]->get_value();

  cout << cpu->registers[reg]->name() << '(' << hex << reg << ')';

  cout << " is " << ov << '\n';



}

void cmd_x::x(int reg, int val)
{


  if(!cpu)
    return;

  if(reg<0 || (reg >= cpu->register_memory_size()) )
    {
      cout << "bad file register\n";
      return;
    }

  int ov = cpu->registers[reg]->get_value();

  cout << cpu->registers[reg]->name() << '(' << hex << reg << ')';

  if(ov == val  ||  val < 0 || val > 255)
    {
      cout << " is " << ov << '\n';
      return;
    }

  // write the new value to the file register:
  cpu->registers[reg]->put_value(val);

  // read it back (some of the file registers have bit's that can't be changed)
  cout << " was " << ov << " now is " 
       << cpu->registers[reg]->get_value() << '\n';


}

void cmd_x::x(char *reg_name)
{

  print_symbol(reg_name);

}

void cmd_x::x(char *reg_name, int val)
{
  update_symbol_value(reg_name,val);
}
