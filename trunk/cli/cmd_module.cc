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
#include "../src/modules.h"

void load_module(char *module_name);

//void display_available_modules(void);
//void load_module_library(char *library_name);

cmd_module c_module;

#define CMD_MOD_LIST    1
#define CMD_MOD_LOAD    2
#define CMD_MOD_DUMP    3
#define CMD_MOD_LIB     4


static cmd_options cmd_module_options[] =
{
  "list",      CMD_MOD_LIST,    OPT_TT_BITFLAG,
  "load",      CMD_MOD_LOAD,    OPT_TT_STRING,
  "dump",      CMD_MOD_DUMP,    OPT_TT_BITFLAG,
  "library",   CMD_MOD_LIB ,    OPT_TT_STRING,
  "lib",       CMD_MOD_LIB ,    OPT_TT_STRING,
  NULL,0,0
};


cmd_module::cmd_module(void)
{ 
  name = "module";

  brief_doc = string("Select & Display modules");

  long_doc = string (
"module [ [load module_type [module_name]] | [lib lib_name] | [list] | \n\
[[dump | pins] module_name] ]\
\n\tIf no options are specified, then the currently defined module(s)\
\n\twill be displayed. This is the same as the `module list' command.\
\n\tThe `module load lib_name' tells gpsim to search for the module\
\n\tlibrary called `lib_name' and to load it. (Note that the format of\
\n\tmodule libraries is exactly the same as a Linux shared library. This\
\n\tmeans that the module library should reside in a path available to\
\n\tdlopen(). Please see the README.MODULES in the gpsim distribution.\
\n\tTo instantiate a new module, then type\
\n\t  module module_type module_name\
\n\twhere module_type refers to a specific module in a module library\
\n\tand module_name is the user name assigned to it.\
\n\tInformation about a module can be displayed by the commad\
\n\t  module module_name [dump | pins]
\n\twhere module_name is the name that you assigned when the module\
\n\twas instantiated. The optional dump and pins identifiers specify\
\n\tthe information you wish to display (dump is the default).\
\n\n\texamples:
\n\n\tmodule                      // Display the modules you've already defined.\
\n\tmodule lib my_mods.so       // Load the module library called my_mods.\
\n\tmodule list                 // Display the list of modules supported.\
\n\tmodule load lcd my_lcd      // Create an instance of an 'lcd'\
\n\tmodule pins my_lcd          // Display the pin states of an instantiated module\
\n\tmodule load lcd lcd2x20     // Create a new module.\
\n\tmodule load led start_light // and another.\
\n");

  op = cmd_module_options; 
}


void cmd_module::module(void)
{

  if(verbose)
    cout << "cmd_module: display modules\n";
  dump_module_list();

}

void cmd_module::module(int bit_flag)
{

  switch(bit_flag)
    {

    case CMD_MOD_LIST:
      display_available_modules();
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
	load_module_library(cos->str);
      break;
    case CMD_MOD_LOAD:
      //if(verbose)
	cout << "module command got the module " << cos->str << '\n';
	load_module(cos->str);

    case CMD_MOD_DUMP:
      cout <<  " is not supported yet\n";
      break;

    default:
      cout << "cmd_module error\n";
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
