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


#include "trace.h"
#include "symbol.h"
#include "gpsim_time.h"
#include "protocol.h"

//========================================================================
//
// Cycle_Counter attribute
//
// The Cycle_counter attribute exposes the cycle counter through the 
// gpsim attribute interface. This allows for it to be queried from
// the command line or sockets.

class CycleCounterAttribute : public Integer
{
protected:
public:
  CycleCounterAttribute() :
    Integer(0) 
  {
    new_name("cycles");
    set_description(" Simulation time in terms of cycles.");
  }
  void set(gint64 i)
  {
    static bool warned = false;
    if(!warned)
      cout << "cycle counter is read only\n";
    warned = true;
  }
  void get(gint64 &i)
  {
    i = cycles.get();
  }
  void get(Packet &p)
  {
    p.EncodeUInt64(cycles.get());
  }
};



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
//
Integer *verbosity;

//========================================================================
// initialize_gpsim_core()
//
int initialize_gpsim_core()
{

  verbosity = new Integer("sim.verbosity",1,"gpsim's verboseness 0=nothing printed 0xff=very verbose");
  get_symbol_table().add(verbosity);
  get_symbol_table().add(new CycleCounterAttribute());

  return 0;
}
