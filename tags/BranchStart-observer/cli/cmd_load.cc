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
#include "cmd_load.h"

#include "../src/pic-processor.h"

extern int parser_warnings;

// instead of including the whole symbol.h file, just get what we need:
int load_symbol_file(pic_processor **, char *);
void display_symbol_file_error(int);

cmd_load c_load;

static cmd_options cmd_load_options[] =
{
  "h",1,    OPT_TT_BITFLAG,
  "c",2,    OPT_TT_BITFLAG,
  "s",3,    OPT_TT_BITFLAG,
  NULL,0,0
};


cmd_load::cmd_load(void)
{ 
  name = "load";

  brief_doc = string("Load either a hex,command, or .cod file");

  long_doc = string ("load  h | c | s  file_name\
\n\n\tload either a hex file, command file, or a symbol file.\
\n\t(Byte Craft's .cod files are the only symbol files that\
\n\tare recognized.)\
\n\n\tExample:\
\n\t  load s perfect_program.cod\
\n\t    will load the symbol file perfect_program.cod\
\n\t    note that the .cod file contains the hex stuff\
\n");

  op = cmd_load_options; 
}

//--------------------
//
//
#define MAX_LINE_LENGTH 256  
int parse_string(char *cmd_string);
extern void process_command_file(char * file_name);

void cmd_load::load(int bit_flag,char *filename)
{
  int verbose_save;
  switch(bit_flag)
    {
    case 1:
      if(have_cpu(1))
	{
	  if(verbose)
	    cout << "cmd_load::load hex file " << filename << '\n';
	  cpu->load_hex(filename);
	}
      else
	cout << " No cpu has been selected\n";
      break;

    case 2:
      /* Don't display parser warnings will processing the command file */
      parser_warnings = 0;
      process_command_file(filename);
      parser_warnings = 1;
      break;
    case 3:
      if(verbose)
	cout << " cmd_load::load cod file "  << filename << '\n';

      int i=load_symbol_file(&cpu, filename);

      if(i)
	{
	  cout << "found a fatal error in the symbol file " << filename <<'\n';
	  display_symbol_file_error(i);
	}
      else
	new_processor(cpu);

      break;
    }
}

