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
#include "cmd_module.h"

#include "../src/pic-processor.h"

void display_available_modules(void);

cmd_module c_module;

#define CMD_MOD_LIST    1
#define CMD_MOD_LOAD    2
#define CMD_MOD_DUMP    3
#define CMD_MOD_LIB     4


static cmd_options cmd_module_options[] =
{
  "list",      CMD_MOD_LIST,    OPT_TT_BITFLAG,
  "load",      CMD_MOD_LOAD,    OPT_TT_BITFLAG,
  "dump",      CMD_MOD_DUMP,    OPT_TT_BITFLAG,
  "library",   CMD_MOD_LIB ,    OPT_TT_STRING,
  "lib",       CMD_MOD_LIB ,    OPT_TT_STRING,
  NULL,0,0
};


cmd_module::cmd_module(void)
{ 
  name = "module";

  brief_doc = string("Select & Display modules");

  long_doc = string ("module [new_module_type [new_module_name]] | [list] | [dump]\
\n\tIf no new module is specified, then the currently defined module(s)\
\n\twill be displayed. To see a list of the modules supported by gpsim,\
\n\ttype 'module list'.  To define a new module, specify the module\
\n\ttype and name. To display the state of the I/O module, type 'module\
\n\tdump' (For now, this will display the pin numbers and their current state.\
\n\n\texamples:
\n\n\tmodule                   // Display the modules you've already defined.\
\n\tmodule list              // Display the list of modules supported.\
\n\tmodule lcd pins          // Display the module `lcd' and the pin states\
\n\tmodule lcd lcd2x20       // Create a new module.\
\n\tmodule led start_light   // and another.\
\n");

  op = cmd_module_options; 
}


void cmd_module::module(void)
{

  if(verbose)
    cout << "cmd_module: display modules\n";
  dump_processor_list();

}

void cmd_module::module(int bit_flag)
{

  switch(bit_flag)
    {

    case CMD_MOD_LIST:
      display_available_modules();
      break;

    case CMD_MOD_LOAD:
    case CMD_MOD_DUMP:
      cout <<  " is not supported yet\n";
      break;
    default:
      cout << "cmd_module error\n";
    }

}


void cmd_module::module(cmd_options_str *cos)
{


  switch(cos->co->value)
    {
    case CMD_MOD_LIB:
      //if(verbose)
	cout << "module command got the library " << cos->str << '\n';

      break;
    }

}

/*
void cmd_module::module(char * module_type, char * module_new_name)
{

  //new_module(add_module( module_type,  module_new_name));

  if(!cpu)
    cout << "Unable to add module\n";
  if(have_cpu(1) && verbose)
     cout <<"seems like cmd_module worked\n";
}
*/
