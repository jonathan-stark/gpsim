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


#include "../config.h"
#include "14bit-processors.h"
#include "interface.h"

//#include <string>
//#include "stimuli.h"

#include "xref.h"

//--------------------------------------------------
void WDT::update(void)
{
  if(wdte){

    value = cpu->time_to_cycles(timeout);
    prescale = cpu->option_reg.get_psa() ? (cpu->option_reg.get_prescale()) : 0;

    if(future_cycle) {

      guint64 fc = cpu->cycles.value + value * (1<<prescale);

      //cout << "WDT::update:  moving break from " << future_cycle << " to " << fc << '\n';

      cpu->cycles.reassign_break(future_cycle, fc, this);
      future_cycle = fc;

    } else {
    
      future_cycle = cpu->cycles.value + value * (1<<prescale);

      cpu->cycles.set_break(future_cycle, this);
    }
  }

}

//--------------------------------------------------
// WDT::put - shouldn't be called?
//

void WDT::put(unsigned int new_value)
{
  value = new_value;

  update();

}

void WDT::initialize(bool enable, double _timeout)
{
  break_point = 0;
  wdte = enable;
  timeout = _timeout;
  warned = 0;

  if(verbose)
    cout << " WDT init called "<< ( (enable) ? "enabling\n" :", but disabling WDT\n");

  if(wdte)
    {
      cout << "Enabling WDT " << " timeout = " << timeout << " seconds\n";
      value = cpu->time_to_cycles(timeout);
      prescale = cpu->option_reg.get_psa() ? (cpu->option_reg.get_prescale()) : 0;

      future_cycle = cpu->cycles.value + value * (1<<prescale);

      cpu->cycles.set_break(future_cycle, this);

    }

}

void WDT::callback(void)
{


  if(wdte) {
    cout<<"WDT timeout: " << hex << cpu->cycles.value << '\n';

    future_cycle = 0;
    update();

    // The TO bit gets cleared when the WDT times out.
    cpu->status.put_TO(0);

    if(break_point)
      bp.halt();
    else if(bp.have_sleep()) {
      bp.clear_sleep();
    }else
      cpu->reset(WDT_RESET);
  }

}

void WDT::clear(void)
{
  if(wdte)
    update();
  else
    {
      if(!warned)
	{
	  warned = 1;
	  cout << "The WDT is not enabled - clrwdt has no effect!\n";
	}
    }

}

void WDT::start_sleep(void)
{

  if(wdte) {
    prescale = 0;

    guint64 fc = cpu->cycles.value + value * (1<<prescale);

    //cout << "WDT::start_sleep:  moving break from " << future_cycle << " to " << fc << '\n';

    cpu->cycles.reassign_break(future_cycle, fc, this);

    future_cycle = fc;
  }
}

void WDT::new_prescale(void)
{

  update();

}

void WDT::callback_print(void)
{

  cout << "WDT\n";
}
