/*
   Copyright (C) 1998,199 T. Scott Dattalo

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
#include "cmd_break.h"
#include "cmd_bus.h"
#include "cmd_clear.h"
#include "cmd_disasm.h"
#include "cmd_dump.h"
#include "cmd_echo.h"
#include "cmd_frequency.h"
#include "cmd_help.h"
#include "cmd_list.h"
#include "cmd_load.h"
#include "cmd_log.h"
#include "cmd_node.h"
#include "cmd_module.h"
#include "cmd_processor.h"
#include "cmd_quit.h"
#include "cmd_reset.h"
#include "cmd_run.h"
#include "cmd_set.h"
#include "cmd_step.h"
#include "cmd_stimulus.h"
#include "cmd_symbol.h"
#include "cmd_trace.h"
#include "cmd_version.h"
#include "cmd_x.h"
#include "cmd_icd.h"

#include "../src/pic-processor.h"
#include "../src/trace.h"

int quit_gpsim = 0;

command *command_list[] =
{
  &attach,
  &c_break,
  &c_bus,
  &clear,
  &disassemble,
  &dump,
  //  &echo,
  &frequency,
  &help,
  &c_icd,
  &c_list,
  &c_load,
  &c_log,
  &c_node,
  &c_module,
  &c_processor,
  &quit,
  &reset,
  &c_run,
  &c_set,
  &step,
  &c_stimulus,
  &c_symbol,
  &c_trace,
  &version,
  &c_x
};


pic_processor *command::cpu = NULL;

command::command(void)
    {
      op = NULL;
      name = NULL;
      token_value = 0;
      cpu = NULL;
    }

command::command(struct cmd_options *options,int tv) 
    { 
      op = options; 
      token_value = tv;
      cpu = NULL;
    };


int number_of_commands = sizeof(command_list) / sizeof(command  *);


command *search_commands(const string &s)
{

   int i=0;

   while(i<number_of_commands) {
     
     if(strcmp(command_list[i]->name, s.c_str()) == 0) {
       return command_list[i];
     }

     i++;

   }

   return NULL;
}

void execute_line(char *cmd)
{
  if(verbose)
    cout << "Executing a line:\n  " << cmd;
}


void command::new_processor(pic_processor *p)
{

  cpu = p;
  trace.switch_cpus(cpu);

}


bool command::have_cpu(bool display_warning)
{

  if(NULL == cpu)
    {
      if(display_warning)
	cout << "No cpu has been selected\n";
      return(0);
    }

  return 1;

}
