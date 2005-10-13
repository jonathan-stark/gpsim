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
#include "cmd_load.h"
#include "input.h"

#include "../src/sim_context.h"
#include "../src/processor.h"

extern int parser_warnings;
extern void redisplay_prompt(void);  // in input.cc

// instead of including the whole symbol.h file, just get what we need:
void display_symbol_file_error(int);

cmd_load c_load;

#define CMD_LOAD_HEXFILE 1
#define CMD_LOAD_CMDFILE 2
#define CMD_LOAD_CODFILE 3 // s for Symbol file


static cmd_options cmd_load_options[] =
{
  {"h",CMD_LOAD_HEXFILE,    OPT_TT_BITFLAG},
  {"c",CMD_LOAD_CMDFILE,    OPT_TT_BITFLAG},
  {"s",CMD_LOAD_CODFILE,    OPT_TT_BITFLAG},
 { 0,0,0}
};


cmd_load::cmd_load(void)
{ 
  name = "load";
  abbreviation = "ld";

  brief_doc = string("Load either a program or command file");

  long_doc = string ("load [processortype] programfile \
\nload cmdfile.stc\
\n\n\tLoad either a program or command file. Program files may be in\
\n\thex or cod (symbol) file format.\
\n\t(Byte Craft's .cod files are the only program files with symbols\
\n\tthat are recognized.)\
\n\
\n\t  processortype - (optional) Name of the processor type simulation\
\n\t                  to load the program file.\
\n\t                  Ignored if the processor command has been previous\
\n\t                  used.\
\n\t  codfile       - a hex or cod formatted file. Cod is often called\
\n\t                  a symbol file.\
\n\t  cmdfile.stc   - a gpsim command file. Must have an .stc extension.\
\n\
\n\tdeprecated:\
\n\t  load  h | c | s  file_name\
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
extern void process_command_file(const char * file_name);

/**
  * cmd_load.load()
  * returns boolean
  */
int cmd_load::load(int bit_flag,const char *filename)
{
  int iReturn = (int)TRUE;
  switch(bit_flag)
    {
    case CMD_LOAD_HEXFILE:
    case CMD_LOAD_CODFILE:
      if(verbose) {
        switch(bit_flag) {
          case CMD_LOAD_HEXFILE:
  	        cout << "cmd_load::load hex file " << filename << '\n';
            break;
          case CMD_LOAD_CODFILE:
            cout << " cmd_load::load cod file "  << filename << '\n';
            break;
        }
      }
      iReturn = CSimulationContext::GetContext()->LoadProgram(
        filename);
      break;

    case CMD_LOAD_CMDFILE:
      /* Don't display parser warnings will processing the command file */
      parser_warnings = 0;
      process_command_file(filename);
      parser_warnings = 1;
      break;
    default:
      cout << "Unknown option flag" << endl;
      iReturn = (int)FALSE; // as a boolean
    }

  // Most of the time diagnostic info will get printed while a processor
  // is being loaded.

  redisplay_prompt();

  return iReturn;
}

int cmd_load::load(Value *file, Value *pProcessorType)
{
  cout << endl;
  string sFile;
  string sProcType;
  const char * psProcessorType = 0;
  sFile = file->toString();
  if (pProcessorType) {
    sProcType = pProcessorType->toString();
    psProcessorType = sProcType.c_str();
  }

  return gpsim_open(get_active_cpu(), sFile.c_str(), psProcessorType);
}

int cmd_load::load(const char *file, const char *pProcessorType)
{
  return gpsim_open(get_active_cpu(), file, pProcessorType);
}
