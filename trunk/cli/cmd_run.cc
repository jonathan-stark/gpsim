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
#include "cmd_run.h"

#include "../src/pic-processor.h"

cmd_run c_run;

static cmd_options cmd_run_options[] =
{
  0,0,0
};


cmd_run::cmd_run(void)
{ 
  name = "run";

  brief_doc = string("Execute the pic program");

  long_doc = string ("run\n\
\tStart simulating and don't stop until a break is encountered.\n\
\n");

  op = cmd_run_options; 
}

void cmd_run::run(void)
{

  if(!cpu)
    {
      cout << "no cpu has been selected\n";
      return;
    }

  cout << "running...\n";

  cpu->run();


}
