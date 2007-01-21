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
#include "cmd_trace.h"

//#include "../src/processor.h"
#include "../src/trace.h"
#include "../src/trace_orb.h"

cmd_trace c_trace;

#define TRACE_RAW_CMD		1
#define TRACE_MASK_CMD		2
#define TRACE_SAVE_CMD          3
#define TRACE_LOGON_CMD		4
#define TRACE_LOGOFF_CMD	5
#define TRACE_INFO_CMD  	6

static cmd_options cmd_trace_options[] =
{
  {"r",		 TRACE_RAW_CMD,	        OPT_TT_NUMERIC},
  {"raw",        TRACE_RAW_CMD,	        OPT_TT_NUMERIC},
  {"mask",	 TRACE_MASK_CMD,	OPT_TT_NUMERIC},
  {"log",        TRACE_LOGON_CMD,	OPT_TT_STRING},
  {"save",       TRACE_SAVE_CMD,	OPT_TT_STRING},
  {"info",       TRACE_INFO_CMD,	OPT_TT_BITFLAG},
  {"disable_log",TRACE_LOGOFF_CMD,	OPT_TT_BITFLAG},
 {0,0,0}
};

cmd_trace::cmd_trace()
  : command("trace", "tr")
{ 

  brief_doc = string("Dump the trace history");

  long_doc = string ("\ntrace [dump_amount | raw | log fname | disable_log]\n"
                     "\ttrace will print out the most recent \"dump_amount\" traces.\n"
                     "\tIf no dump_amount is specified, then only the lat few trace\n"
                     "\tevents will be displayed.\n\n"
                     "\ttrace raw expr -- display the trace contents in a minimally decoded manner\n"
                     "\ttrace log fname -- log all raw trace events to a file\n"
                     "\ttrace save fname -- save the decode trace buffer to a file\n"
                     "\ttrace disable_log -- stop all file logging\n"
                     );

  op = cmd_trace_options; 
}

void cmd_trace::trace(void)
{
  get_trace().dump(0,stdout);
}

void cmd_trace::trace(Expression *expr)
{
  int n = (int)evaluate(expr);
  get_trace().dump(n,stdout);
}

void cmd_trace::trace(cmd_options *opt)
{

  switch(opt->value) {

  case TRACE_LOGOFF_CMD:
    get_trace().disableLogging();
    cout << "Logging to file disabled" << endl;
    break;
  case TRACE_INFO_CMD:
    get_trace().showInfo();
    break;
  default:
    cout << " Invalid set option\n";
  }

}

void cmd_trace::trace(cmd_options_expr *coe)
{
  double dvalue = 0.0;

  if(coe->expr)
    dvalue = evaluate(coe->expr);

  int value = (int) dvalue;

  switch(coe->co->value)
    {

    case TRACE_RAW_CMD:
      get_trace().dump_raw(value);
      break;

    default:
      cout << " Invalid option\n";
    }

}

void cmd_trace::trace(cmd_options_num *con)
{
  switch(con->co->value) {
  case TRACE_RAW_CMD:
    get_trace().dump_raw(con->n);
    break;
  case TRACE_MASK_CMD:
    // trace_watch_register(con->n);
    cout << "THIS IS BROKEN.... logging register " << con->n << '\n';
    break;
  default:
    cout << " Invalid trace option\n";
  }

}

void cmd_trace::trace(cmd_options_str *cos)
{
  switch(cos->co->value) {
  case TRACE_LOGON_CMD:
    get_trace().enableLogging(cos->str);
    break;
  case TRACE_SAVE_CMD:
    {
      FILE *fp = fopen(cos->str,"w");
      if(fp) {
	get_trace().dump(-1,fp);
	fclose(fp);
      }
    }
    break;
  default:
    cout << " Invalid set option\n";
  }

}
