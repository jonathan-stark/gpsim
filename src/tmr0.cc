/*
   Copyright (C) 1998 Scott Dattalo

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

#include <string>
#include "stimuli.h"

#include "xref.h"

//--------------------------------------------------
// member functions for the TMR0 base class
//--------------------------------------------------
TMR0::TMR0(void)
{
  break_point = 0;
  value=0;
  synchronized_cycle=0;
  prescale=1;
  new_name("tmr0");
}

void TMR0::start(int restart_value, int sync=0)
{

  value = restart_value;
  if(verbose)
    cout << "TMRO::start\n";

  synchronized_cycle = cpu->cycles.value + sync;

  prescale = 1 << (cpu->option_reg.get_psa() ? 0 : (1+cpu->option_reg.get_prescale()));

  prescale_counter = prescale;

  last_cycle = value * prescale;
  last_cycle = synchronized_cycle - last_cycle;

  unsigned int fc = last_cycle + 256 * prescale;

  if(future_cycle)
    cpu->cycles.reassign_break(future_cycle, fc, this);
  else
    cpu->cycles.set_break(fc, this);

  future_cycle = fc;

  if(verbose)
    cout << "TMR0::start   last_cycle = " << hex << last_cycle << " future_cycle = " << future_cycle << '\n';


}


// %%%FIX ME%%% 
void TMR0::increment(void)
{
  //  cout << "TMR0 increment because of external clock ";

  if(--prescale_counter == 0)
    {
      prescale_counter = prescale;
      if(++value == 256)
	{
	  value = 0;
	  if(cpu->base_isa() == _14BIT_PROCESSOR_)
	    {
	      cpu14->intcon->set_t0if();
	    }
	}
      trace.register_write(address,value);
    }
  //  cout << value << '\n';
}

void TMR0::put(unsigned int new_value)
{
  if(cpu->option_reg.get_t0cs())
    {
      cout << "TMR0::put external clock...\n";
    }

  start(new_value,2);

  trace.register_write(address,value);

#if 0
  // Note, anytime something is written to TMR0, the prescaler, if it's
  // assigned to tmr0, is also cleared. This is implicitly handled by
  // saving the value of cpu's cycle counter and associating that value
  // with the tmr rollover.

  value = new_value & 0xff;

  prescale = 1 << (cpu->option_reg.get_psa() ? 0 : (1+cpu->option_reg.get_prescale()));
  prescale_counter = prescale;

  synchronized_cycle = cpu->cycles.value + 2;

  last_cycle = value * prescale;
  last_cycle = synchronized_cycle - last_cycle;

  unsigned int fc = last_cycle + 256 * prescale;

  if(future_cycle)
    cpu->cycles.reassign_break(future_cycle, fc, this);
  else
    cpu->cycles.set_break(fc, this);

  future_cycle = fc;

  trace.register_write(address,value);

  /*
  cout << "TMR0::put\n";
  cout << " value " << value << '\n';
  cout << " future_cycle " << future_cycle << '\n';
  cout << " last_cycle " << last_cycle << '\n';
  */
#endif
}

unsigned int TMR0::get_value(void)
{
  // If the TMR0 is being read immediately after being written, then
  // it hasn't had enough time to synchronize with the PIC's clock.
  if(cpu->cycles.value <= synchronized_cycle)
    return value;

  if(cpu->option_reg.get_t0cs())
    return(value);

  int new_value = (cpu->cycles.value - last_cycle)/ prescale;

  //  if(new_value == 256) {
    // tmr0 is about to roll over. However, the user code
    // has requested the current value before the callback function
    // has been invoked. S0 just return 0.

  //    new_value = 0;

  //  }

  if (new_value > 255)
    {
      cout << "TMR0: bug TMR0 is larger than 255...\n";
      cout << "cpu->cycles.value = " << cpu->cycles.value <<
	"  last_cycle = " << last_cycle <<
	"  prescale = "  << prescale << 
	"  calculated value = " << new_value << '\n';

      // cop out. tmr0 has a bug. So rather than annoy
      // the user with an infinite number of messages,
      // let's just go ahead and reset the logic.
      new_value &= 0xff;
      last_cycle = new_value*prescale;
      last_cycle = cpu->cycles.value - last_cycle;
      synchronized_cycle = last_cycle;
    }

  value = new_value;
  return(value);
  
}

unsigned int TMR0::get(void)
{
  value = get_value();
  trace.register_read(address, value);
  return value;
}
void TMR0::new_prescale(void)
{
  //cout << "tmr0 new_prescale\n";

  int new_value = (cpu->cycles.value - last_cycle)/prescale;

  if(new_value>255)
    cout << "TMR0 bug value>255 - new_prescale\n";

  prescale = 1 << (cpu->option_reg.get_psa() ? 0 : (1+cpu->option_reg.get_prescale()));
  prescale_counter = prescale;

  if(cpu->option_reg.get_t0cs())
    {
      //cout << "external clock...\n";

      value = new_value;
    }
  else
    {
      // Get the current value of TMR0

      //cout << "cycles " << cpu->cycles.value  << " old prescale " << prescale;


      //cout << " new prescale " << prescale;

      // Now compute the 'last_cycle' as though if TMR0 had been running on the 
      // new prescale all along. Recall, 'last_cycle' records the value of the cpu's
      // cycle counter when tmr0 last rolled over.

      last_cycle = value * prescale;
      last_cycle = synchronized_cycle - last_cycle;

      //cout << " effective last_cycle " << last_cycle << '\n';

      if(cpu->cycles.value <= synchronized_cycle)
	last_cycle += (synchronized_cycle - cpu->cycles.value);

      unsigned int fc = last_cycle + 256 * prescale;
      //cout << "moving break from " << future_cycle << " to " << fc << '\n';

      cpu->cycles.reassign_break(future_cycle, fc, this);

      future_cycle = fc;
    }
}

void TMR0::new_clock_source(void)
{

  //  cout << "TMR0:new_clock_source changed to the ";
  if(cpu->option_reg.get_t0cs())
    {
      //cout << "external\n";
      //      cpu->cycles.
    }
  else
    {
      //cout << "internal\n";
      put(value);    // let TMR0::put() set a cycle counter break point
    }
}

// TMR0 callback is called when the cycle counter hits the break point that
// was set in TMR0::put. The cycle counter will clear the break point, so
// we don't need to worry about it. At this point, TMR0 is rolling over.

void TMR0::callback(void)
{

  //cout<<"TMR0 rollover: " << hex << cpu->cycles.value << '\n';

  // If tmr0 is being clocked by the external clock, then at some point
  // the simulate code must have switched from the internal clock to
  // external clock. The cycle break point was still set, so just ignore it.
  if(cpu->option_reg.get_t0cs())
    {
      future_cycle = 0;  // indicates that tmr0 no longer has a break point
      return;
    }

  value = 0;
  synchronized_cycle = cpu->cycles.value;
  last_cycle = synchronized_cycle;
  future_cycle = last_cycle + 256*prescale;
  cpu->cycles.set_break(future_cycle, this);
  if(cpu->base_isa() == _14BIT_PROCESSOR_)
    {
      cpu14->intcon->set_t0if();
    }


}
