/*
   Copyright (C) 1998,1999 T. Scott Dattalo

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

#include "../config.h"
#include "trace.h"
#include "gpsim_interface.h"
#include "sim_context.h"

// in attribute.cc:
extern void init_attributes();

//==============================================================
// simulation_cleanup()
//
//  Called just before gpsim exits.
//

void simulation_cleanup(void)
{

  // Flush the log file (if there is one).

  trace_log.close_logfile();

}


//========================================================================
// initialize_gpsim_core()
//
int initialize_gpsim_core()
{

  init_attributes();
  CSimulationContext::GetContext()->Initialize();
  return 0;
}
