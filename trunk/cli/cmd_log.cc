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
#include "cmd_log.h"
#include "../src/trace_orb.h"
#include "../src/pic-processor.h"
#include "../src/symbol_orb.h"
#include "../src/trace.h"

cmd_log c_log;

#define LOG_ON		1
#define LOG_OFF		2
#define WRITE       3
#define READ        4
#define WRITE_VALUE 5
#define READ_VALUE  6


static cmd_options cmd_trace_options[] =
{
  "on",	 LOG_ON,      OPT_TT_BITFLAG,
  "off", LOG_OFF,     OPT_TT_BITFLAG,
  "w",   WRITE,       OPT_TT_BITFLAG,
  "r",   READ,        OPT_TT_BITFLAG,
  "wv",  WRITE_VALUE, OPT_TT_BITFLAG,
  "rv",  READ_VALUE,  OPT_TT_BITFLAG,
  0,0,0
};

cmd_log::cmd_log(void)
{ 
  name = "log";

    brief_doc = string("Log/record events to a file");

    long_doc = string ("\nlog [[on [file_name]]|[off]] | [w|r reg] [wv|rv reg val]\n\
\tLog will record simulation history in a file. It's similar to the\n\
\tbreak command\n\
\tExamples:\n\
\t\tlog               - Display log status\n\
\t\tlog on            - Begin logging in file gpsim.log\n\
\t\tlog on file.log   - Begin logging in file.log\n\
\t\tlog off           - Stop logging\n\
\t\tlog w temp_hi     - Log all writes to reg temp_hi\n\
");

  op = cmd_trace_options; 
}

void cmd_log::log(void)
{
  trace_log.status();
}



void cmd_log::log(cmd_options *opt)
{

  if(!cpu)
    cout << "warning, no cpu\n";

  switch(opt->value) {
  case LOG_ON:
    trace_log.enable_logging(0);
    break;
  case LOG_OFF:
    trace_log.disable_logging();
    break;
  default:
    cout << " Invalid set option\n";
  }

}

void cmd_log::log(cmd_options *opt, char *str, int val, int mask)
{

  int sym_value;

  if(!cpu)
    cout << "warning, no cpu\n";

  switch(opt->value) {
  case LOG_ON:
    trace_log.enable_logging(str);
    break;
  case LOG_OFF:
    trace_log.disable_logging();
    break;
  case WRITE:
  case READ:
  case WRITE_VALUE:
  case READ_VALUE:

      if(!get_symbol_value(str,&sym_value)) {
	cout << '`' << str << '\'' << " was not found in the symbol table\n";
	return;
      }

      log(opt, sym_value, val, mask);

    break;

  default:
    cout << "Error, Unknown option\n";
  }


}

void cmd_log::log(cmd_options *opt, int reg, int value, int mask)
{
  int b=MAX_BREAKPOINTS;
  char *str=0;

  if(!cpu)
    cout << "warning, no cpu\n";

  switch(opt->value) {
  case LOG_ON:
    cout << "logging on file int,int,int (ignoring)"  << endl;
    break;
  case LOG_OFF:
    trace_log.disable_logging();
    break;
  case WRITE:
    if((value >= 0) || (mask >= 0)) {
      cout << "too many args\n";
      return;
    }
    
    b = bp.set_notify_write(cpu, reg);
    if(b < MAX_BREAKPOINTS)
      cout << "log register " << reg << " when it is written. bp#: " << b << '\n';

    break;
  case READ:
    if((value >= 0) || (mask >= 0)) {
      cout << "too many args\n";
      return;
    }

    b = bp.set_notify_read(cpu, reg);
    if(b < MAX_BREAKPOINTS)
      cout << "log register " << reg << " when it is read.\n" << 
	"bp#: " << b << '\n';


    break;
  case WRITE_VALUE:
  case READ_VALUE:

    if(opt->value == READ_VALUE) {
      b = bp.set_notify_read_value(cpu, reg,value,mask);
      str = "read from";
    } else {

      b = bp.set_notify_write_value(cpu, reg,value,mask);
      str = "written to";
    }

    if(b<MAX_BREAKPOINTS) {

      cout << "log when ";
      if(mask == 0 || mask == 0xff)
	cout << "0x" << hex << (value&0xff);
      else {
	cout << "bit pattern ";
	for(unsigned int ui=0x80; ui; ui>>=1) 
	  if(ui & mask) {
	    if(ui & value)
	      cout << '1';
	    else
	      cout << '0';
	  }
	  else
	    cout << 'X';
      }
      
      cout << " is " << str <<" register " << reg << '\n' << 
	"bp#: " << b << '\n';
    }
    break;

  default:
    cout << "error log opt, int,int,int\n";
  }


}

void cmd_log::log(int number)
{

  if(!cpu)
    return;

  cout << "log number " << number << endl;

}


