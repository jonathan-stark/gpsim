/*
   Copyright (C) 1998-2000 Scott Dattalo

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


#include <stdio.h>
#include <iostream.h>
#include <iomanip.h>


#include "14bit-processors.h"
#include "interface.h"

//#include <string>
//#include "stimuli.h"

#include "xref.h"

//--------------------------------------------------
void WDT::put(unsigned int new_value)
{
  value = new_value;
}

void WDT::initialize(bool enable, double _timeout)
{
  break_point = 0;
  wdte = enable;
  timeout = _timeout;
  warned = 0;

  if(verbose)
    cout << " WDT init called. en = " << enable << " to = " << timeout << '\n';
  if(wdte)
    {
      cout << "Enabling WDT\n";

      value = cpu->time_to_cycles(timeout);
      prescale = cpu->option_reg.get_psa() ? (cpu->option_reg.get_prescale()) : 0;

      future_cycle = cpu->cycles.value + value * (1<<prescale);

      cpu->cycles.set_break(future_cycle, this);

    }

}

void WDT::callback(void)
{


  cout<<"WDT timeout: " << hex << cpu->cycles.value << '\n';

  prescale = cpu->option_reg.get_psa() ? (cpu->option_reg.get_prescale()) : 0;

  future_cycle = cpu->cycles.value + value * (1<<prescale);

  cpu->cycles.set_break(future_cycle, this);

  if(break_point)
    bp.halt();
  else
    cpu->reset(WDT_RESET);

}

void WDT::clear(void)
{
  if(wdte)
    {
      prescale = cpu->option_reg.get_psa() ? (cpu->option_reg.get_prescale()) : 0;

      guint64 fc = cpu->cycles.value + value * (1<<prescale);

      // cout << "moving break from " << future_cycle << " to " << fc << '\n';

      cpu->cycles.reassign_break(future_cycle, fc, this);

      future_cycle = fc;
    }
  else
    {
      if(!warned)
	{
	  warned = 1;
	  cout << "The WDT is not enabled - clrwdt has no effect!\n";
	}
    }

}
