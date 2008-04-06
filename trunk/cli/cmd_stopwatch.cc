/*
   Copyright (C) 2003 T. Scott Dattalo

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
#include "cmd_stopwatch.h"
#include "../src/pic-processor.h"

enum STOPWATCH_OPTIONS {
  eStart,
  eStop,
  eClear
};

static cmd_options cmd_stopwatch_options[] =
{
  {"start",eStart,    OPT_TT_BITFLAG},
  {"stop", eStop,    OPT_TT_BITFLAG},
  {"clear",eClear,    OPT_TT_BITFLAG},
  {0,0,0}
};

cmd_stopwatch::cmd_stopwatch()
  : command("stopwatch",0)
{ 
  brief_doc = string("Measure time between events");

  long_doc = string ("\nstopwatch [clear]\n"
    "\tThis command measures CPU cycles and time between events\n"
    "\tWithout any options, the current value of the stopwatch is\n"
    "\tdisplayed. The stopwatch is started at reset and restarted\n"
		     "\tevery time the 'clear' option is specified.\n");

  op = cmd_stopwatch_options;
}



void cmd_stopwatch::set()
{
  cout << "start stop watch \n";
}

void cmd_stopwatch::set(int bit_flag)
{

  switch(bit_flag) {

  case eStop:
    // Stop
    cout << "stop stop watch \n";
    break;
  case eStart:
    // Start
    cout << "start stop watch \n";
    break;
  case eClear:
    // Clear
    cout << "clear stop watch \n";
    break;
  }

}



cmd_stopwatch stopwatch;
