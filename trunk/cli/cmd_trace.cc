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
#include "cmd_trace.h"
#include "../src/trace_orb.h"

cmd_trace c_trace;

static cmd_options cmd_trace_options[] =
{
  "r",1,    OPT_TT_NUMERIC,
  NULL,0,0
};


cmd_trace::cmd_trace(void)
{ 
  name = "trace";

    brief_doc = string("Dump the trace history");

    long_doc = string ("\ntrace [dump_amount]\n\
\ttrace will print out the most recent \"dump_amount\" traces.\n\
\tIf no dump_amount is specified, then the entire trace buffer\n\
\twill be displayed.
");

  op = cmd_trace_options; 
}

void cmd_trace::trace(void)
{
  trace_dump_all();
}

void cmd_trace::trace(int numberof)
{

  trace_dump_n(numberof);
}

void cmd_trace::trace(cmd_options *opt)
{

  switch(opt->value) {
  case 1:
    cout << "test\n";
    break;
  default:
    cout << " Invalid set option\n";
  }

}

void cmd_trace::trace(cmd_options_num *con)
{

  switch(con->co->value) {
  case 1:
    trace_dump_raw(con->n);
    break;
  default:
    cout << " Invalid set option\n";
  }

}
