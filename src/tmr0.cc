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
#include <iostream>
#include <iomanip>


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
  value.put(0);
  synchronized_cycle=0;
  future_cycle=0;
  last_cycle=0;
  state=0;      // start disabled (will change to enabled by other init code)
  prescale=1;
  new_name("tmr0");
}

void TMR0::stop(void)
{
  state &= (~1);      // the timer is disabled.
}

void TMR0::start(int restart_value, int sync)
{

  state |= 1;          // the timer is on

  value.put(restart_value);
  if(verbose)
    cout << "TMRO::start\n";

  old_option = cpu_pic->option_reg.value.get();

  prescale = 1 << get_prescale();
  prescale_counter = prescale;

  if(get_t0cs()) {
    if(verbose)
      cout << "tmr0 starting with external clock \n";

  } else {

    synchronized_cycle = get_cycles().value + sync;
  
    last_cycle = value.get() * prescale;
    last_cycle = synchronized_cycle - last_cycle;

    guint64 fc = last_cycle + max_counts() * prescale;


    if(future_cycle)
      get_cycles().reassign_break(future_cycle, fc, this);
    else
      get_cycles().set_break(fc, this);

    future_cycle = fc;

    //if(verbose)
      cout << "TMR0::start   last_cycle = " << hex << last_cycle << " future_cycle = " << future_cycle << '\n';


  }



}

void TMR0::clear_break(void)
{
  if(verbose)
    cout << "TMR0 break was cleared\n";

  future_cycle = 0;
  last_cycle = 0;

}

unsigned int TMR0::get_prescale(void)
{
  return (cpu_pic->option_reg.get_psa() ? 0 : (1+cpu_pic->option_reg.get_prescale()));
}

void TMR0::increment(void)
{
  //  cout << "TMR0 increment because of external clock ";

  if((state & 1) == 0)
    return;

  if(--prescale_counter == 0)
    {
      trace.raw(write_trace.get() | value.get());
      prescale_counter = prescale;
      if(value.get() == 255)
	{
	  value.put(0);
	  set_t0if();

	}
      else
	value.put(value.get() + 1);
    }
  //  cout << "TMR0 value ="<<value.get() << '\n';
}

void TMR0::put(unsigned int new_value)
{
  if(get_t0cs())
    {
      cout << "TMR0::put external clock...\n";
    }

  trace.raw(write_trace.get() | value.get());

  // If tmr0 is enabled, then start it up.

  if(state & 1)
    start(new_value,2);

}

unsigned int TMR0::get_value(void)
{
  // If the TMR0 is being read immediately after being written, then
  // it hasn't had enough time to synchronize with the PIC's clock.
  if(get_cycles().value <= synchronized_cycle)
    return value.get();

  // If we're running off the external clock or the tmr is disabled
  // then just return the register value.
  if(get_t0cs() || ((state & 1)==0))
    return(value.get());

  int new_value = (int )((get_cycles().value - last_cycle)/ prescale);

  //  if(new_value == 256) {
    // tmr0 is about to roll over. However, the user code
    // has requested the current value before the callback function
    // has been invoked. S0 just return 0.

  //    new_value = 0;

  //  }

  if (new_value > 255)
    {
      cout << "TMR0: bug TMR0 is larger than 255...\n";
      cout << "cycles.value = " << get_cycles().value <<
	"  last_cycle = " << last_cycle <<
	"  prescale = "  << prescale << 
	"  calculated value = " << new_value << '\n';

      // cop out. tmr0 has a bug. So rather than annoy
      // the user with an infinite number of messages,
      // let's just go ahead and reset the logic.
      new_value &= 0xff;
      last_cycle = new_value*prescale;
      last_cycle = get_cycles().value - last_cycle;
      synchronized_cycle = last_cycle;
    }

  value.put(new_value);
  return(value.get());
  
}

unsigned int TMR0::get(void)
{
  value.put(get_value());
  trace.raw(read_trace.get() | value.get());
  return value.get();
}
void TMR0::new_prescale(void)
{
  //cout << "tmr0 new_prescale\n";

  unsigned int new_value;

  int option_diff = old_option ^ cpu_pic->option_reg.value.get();

  old_option ^= option_diff;   // save old option value. ( (a^b) ^b = a)

  if(option_diff & OPTION_REG::T0CS) {
    // TMR0's clock source has changed.
    if(verbose)
      cout << "T0CS has changed to ";

    if(cpu_pic->option_reg.get_t0cs()) {
      // External clock
      if(verbose)
	cout << "external clock\n";
      get_cycles().clear_break(future_cycle);
      future_cycle = 0;
    } else {
      // Internal Clock
      if(verbose)
	cout << "internal clock\n";
    }
    start(value.get());

  } else {

    if(get_t0cs() || ((state & 1)==0)) {
      prescale = 1 << get_prescale();
      prescale_counter = prescale;

    } else {

    if(last_cycle < (gint64)get_cycles().value)
	  new_value = (unsigned int)((get_cycles().value - last_cycle)/prescale);
    else
	  new_value = 0;

      if(new_value>=max_counts()) {
	cout << "TMR0 bug (new_prescale): exceeded max count"<< max_counts() <<'\n';
	cout << "   last_cycle = 0x" << hex << last_cycle << endl;
	cout << "   cpu cycle = 0x" << hex << (get_cycles().value) << endl;

	cout << "   prescale = 0x" << hex << prescale << endl;

      }


      // Get the current value of TMR0

      // cout << "cycles " << cycles.value  << " old prescale " << prescale;

      prescale = 1 << get_prescale();
      prescale_counter = prescale;

      // cout << " new prescale " << prescale;

      // Now compute the 'last_cycle' as though if TMR0 had been running on the 
      // new prescale all along. Recall, 'last_cycle' records the value of the cpu's
      // cycle counter when tmr0 last rolled over.

      last_cycle = value.get() * prescale;
      last_cycle = synchronized_cycle - last_cycle;

      // cout << " effective last_cycle " << last_cycle << '\n';

      if(get_cycles().value <= synchronized_cycle)
	last_cycle += (synchronized_cycle - get_cycles().value);

      guint64 fc = last_cycle + max_counts() * prescale;

      // cout << "moving break from " << future_cycle << " to " << fc << '\n';

      get_cycles().reassign_break(future_cycle, fc, this);

      future_cycle = fc;
    }
  }
}

unsigned int TMR0::get_t0cs(void)
{
  return cpu_pic->option_reg.get_t0cs();
}
void TMR0::set_t0if(void)
{
  if(cpu_pic->base_isa() == _14BIT_PROCESSOR_)
    {
      cpu14->intcon->set_t0if();
    }
}

void TMR0::new_clock_source(void)
{
  // This is handled in the new_prescale() function now
#if 0
  //  cout << "TMR0:new_clock_source changed to the ";
  if(get_t0cs())
    {
      //cout << "external\n";
      //      cycles.
    }
  else
    {
      //cout << "internal\n";
      put(value.get());    // let TMR0::put() set a cycle counter break point
    }
#endif
}

// TMR0 callback is called when the cycle counter hits the break point that
// was set in TMR0::put. The cycle counter will clear the break point, so
// we don't need to worry about it. At this point, TMR0 is rolling over.

void TMR0::callback(void)
{

  //cout<<"TMR0 rollover: " << hex << cycles.value << '\n';

  if((state & 1) == 0) {

    cout << "TMR0 callback ignored because timer is disabled\n";
  }

  // If tmr0 is being clocked by the external clock, then at some point
  // the simulate code must have switched from the internal clock to
  // external clock. The cycle break point was still set, so just ignore it.
  if(get_t0cs())
    {
      future_cycle = 0;  // indicates that tmr0 no longer has a break point
      return;
    }

  value.put(0);
  synchronized_cycle = get_cycles().value;
  last_cycle = synchronized_cycle;
  future_cycle = last_cycle + max_counts()*prescale;
  get_cycles().set_break(future_cycle, this);
  set_t0if();
}

void  TMR0::reset(RESET_TYPE r)
{

  switch(r) {
  case POR_RESET:
    value = por_value;
    break;
  default:
    break;
  }


}

void TMR0::callback_print(void)
{

  cout << "TMR0\n";
}
