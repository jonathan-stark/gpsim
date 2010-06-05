/*
   Copyright (C) 1998 Scott Dattalo

This file is part of the libgpsim library of gpsim

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see 
<http://www.gnu.org/licenses/lgpl-2.1.html>.
*/


#include <stdio.h>
#include <iostream>
#include <iomanip>


#include "../config.h"
#include "14bit-processors.h"
#include "interface.h"

#include <string>
#include "stimuli.h"

#include "xref.h"

//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d-%s() ",__FILE__,__LINE__,__FUNCTION__); printf arg; }
#else
#define Dprintf(arg) {}
#endif


//--------------------------------------------------
// member functions for the TMR0 base class
//--------------------------------------------------
TMR0::TMR0(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu,pName,pDesc),
    m_pOptionReg(0)
{
  value.put(0);
  synchronized_cycle=0;
  future_cycle=0;
  last_cycle=0;
  state=0;      // start disabled (will change to enabled by other init code)
  prescale=1;
  m_bLastClockedState=false;
  new_name("tmr0");
}

//------------------------------------------------------------------------
void TMR0::release()
{
}
//------------------------------------------------------------------------
// setSinkState
//
// Called when the I/O pin driving TMR0 changes states.

void TMR0::setSinkState(char new3State)
{
  bool bNewState = new3State == '1';

  if (m_bLastClockedState != bNewState) {
    m_bLastClockedState = bNewState;
    if ( verbose & 2 )
      printf("TMR0::setSinkState:%d cs:%d se:%d\n",bNewState,get_t0cs(),get_t0se());
    if (get_t0cs() && bNewState != get_t0se())
      increment();
  }
}

void TMR0::set_cpu(Processor *new_cpu, PortRegister *reg, unsigned int pin, OPTION_REG *pOption)
{
  cpu = new_cpu;
  m_pOptionReg = pOption;
  reg->addSink(this,pin);
}

// RCP - add an alternate way to connect to a CPU
void TMR0::set_cpu(Processor *new_cpu, PinModule *pin,OPTION_REG *pOption)
{
  cpu = new_cpu;
  m_pOptionReg = pOption;
  pin->addSink(this);
}

//------------------------------------------------------------
// Stop the tmr.
//
void TMR0::stop()
{

  Dprintf(("\n"));

  // If tmr0 is running, then stop it:
  if (state & 1) {

    // refresh the current value.
    get_value();

    state &= (~1);      // the timer is disabled.
    clear_trigger();

  }
}
  
void TMR0::start(int restart_value, int sync)
{

  Dprintf(("restart_value=%d sync=%d\n",restart_value, sync));

  state |= 1;          // the timer is on

  value.put(restart_value&0xff);

  //old_option = cpu_pic->option_reg.value.get();
  old_option = m_pOptionReg->get_value();

  prescale = 1 << get_prescale();
  prescale_counter = prescale;

  if(get_t0cs()) {
    Dprintf(("External clock\n"));

  } else {

    synchronized_cycle = get_cycles().get() + sync;
  
    last_cycle = (restart_value % max_counts()) * prescale;
    last_cycle = synchronized_cycle - last_cycle;

    guint64 fc = last_cycle + max_counts() * prescale;


    if(future_cycle)
      get_cycles().reassign_break(future_cycle, fc, this);
    else
      get_cycles().set_break(fc, this);

    future_cycle = fc;

    Dprintf(("last_cycle:0x%"PRINTF_GINT64_MODIFIER"x future_cycle:0x%"PRINTF_GINT64_MODIFIER"x\n",last_cycle,future_cycle));

  }



}

void TMR0::clear_trigger()
{
  Dprintf(("\n"));

  if (future_cycle) {
    future_cycle = 0;
    get_cycles().clear_break(this);
  }
  last_cycle = 0;

}

unsigned int TMR0::get_prescale()
{
  Dprintf(("OPTION::PSA=%d\n", m_pOptionReg->get_psa()));

  //return (cpu_pic->option_reg.get_psa() ? 0 : (1+cpu_pic->option_reg.get_prescale()));
  return (m_pOptionReg->get_psa()  ? 0 : (1+m_pOptionReg->get_prescale()));
}

// This is used to drive timer ias counter of IO port if T0CS is true
//
void TMR0::increment()
{
  Dprintf(("\n"));

  if((state & 1) == 0)
    return;

  if(--prescale_counter == 0)
    {
      trace.raw(write_trace.get() | value.get());
      prescale_counter = prescale;
      if(value.get() == 255)
	{
	  //cout << "TMR0 rollover because of external clock ";
	  value.put(0);
	  set_t0if();

	}
      else
	value.put(value.get() + 1);
    }
  //  cout << "TMR0 value ="<<value.get() << '\n';
}


void TMR0::put_value(unsigned int new_value)
{
  Dprintf(("\n"));

  value.put(new_value & 0xff);

  // If tmr0 is enabled, then start it up.
  if(state & 1)
    start(new_value,2);
}

void TMR0::put(unsigned int new_value)
{
  Dprintf(("\n"));

  trace.raw(write_trace.get() | value.get());
  put_value(new_value);
}

unsigned int TMR0::get_value()
{
  // If the TMR0 is being read immediately after being written, then
  // it hasn't had enough time to synchronize with the PIC's clock.
  if(get_cycles().get() <= synchronized_cycle)
    return value.get();

  // If we're running off the external clock or the tmr is disabled
  // then just return the register value.
  if(get_t0cs() || ((state & 1)==0))
    return(value.get());

  int new_value = (int )((get_cycles().get() - last_cycle)/ prescale);

  //  if(new_value == 256) {
    // tmr0 is about to roll over. However, the user code
    // has requested the current value before the callback function
    // has been invoked. S0 just return 0.

  //    new_value = 0;

  //  }

  if (new_value > 255)
    {
      cout << "TMR0: bug TMR0 is larger than 255...\n";
      cout << "cycles.value = " << get_cycles().get() <<
	"  last_cycle = " << last_cycle <<
	"  prescale = "  << prescale << 
	"  calculated value = " << new_value << '\n';

      // cop out. tmr0 has a bug. So rather than annoy
      // the user with an infinite number of messages,
      // let's just go ahead and reset the logic.
      new_value &= 0xff;
      last_cycle = new_value*prescale;
      last_cycle = get_cycles().get() - last_cycle;
      synchronized_cycle = last_cycle;
    }

  value.put(new_value);
  return(value.get());
  
}

unsigned int TMR0::get()
{
  value.put(get_value());
  trace.raw(read_trace.get() | value.get());
  return value.get();
}
void TMR0::new_prescale()
{
  Dprintf(("\n"));

  unsigned int new_value;

  int option_diff = old_option ^ m_pOptionReg->get_value();

  old_option ^= option_diff;   // save old option value. ( (a^b) ^b = a)


  if(option_diff & OPTION_REG::T0CS) {
    // TMR0's clock source has changed.
    if(verbose)
      cout << "T0CS has changed to ";

    if(m_pOptionReg->get_t0cs()) {
      // External clock
      if(verbose)
	cout << "external clock\n";
      if (future_cycle)
      {
        future_cycle = 0;
        get_cycles().clear_break(this);
      }
    } else {
      // Internal Clock
      if(verbose)
	cout << "internal clock\n";
    }

    start(value.get());

  } else {

    // Refresh the current tmr0 value. The current tmr0 value is used
    // below to recompute the value for 'last_cycle'
    get_value();

    if(get_t0cs() || ((state & 1)==0)) {
      prescale = 1 << get_prescale();
      prescale_counter = prescale;

    } else {

      if(last_cycle < (gint64)get_cycles().get())
        new_value = (unsigned int)((get_cycles().get() - last_cycle)/prescale);
      else
        new_value = 0;

      if(new_value>=max_counts()) {
        cout << "TMR0 bug (new_prescale): exceeded max count"<< max_counts() <<'\n';
        cout << "   last_cycle = 0x" << hex << last_cycle << endl;
        cout << "   cpu cycle = 0x" << hex << (get_cycles().get()) << endl;
      
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

      /*
      last_cycle = value.get() * prescale;
      last_cycle = synchronized_cycle - last_cycle;

      if(get_cycles().get() <= synchronized_cycle)
	last_cycle += (synchronized_cycle - get_cycles().get());
      */

      last_cycle = value.get() * prescale;
      last_cycle = get_cycles().get() - last_cycle;
      synchronized_cycle = last_cycle;

      guint64 fc = last_cycle + max_counts() * prescale;

      // cout << "moving break from " << future_cycle << " to " << fc << '\n';

      get_cycles().reassign_break(future_cycle, fc, this);

      future_cycle = fc;
    }
  }
}

bool TMR0::get_t0cs()
{

  //return cpu_pic->option_reg.get_t0cs() != 0;
  return m_pOptionReg->get_t0cs() != 0;
}
bool TMR0::get_t0se()
{
  //return cpu_pic->option_reg.get_t0se() != 0;
  return m_pOptionReg->get_t0se() != 0;
}

void TMR0::set_t0if()
{
  if(cpu_pic->base_isa() == _14BIT_PROCESSOR_)
    {
      cpu14->intcon->set_t0if();
    }
}

void TMR0::new_clock_source()
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

void TMR0::callback()
{

  Dprintf(("now=0x%"PRINTF_GINT64_MODIFIER"x\n",get_cycles().get()));

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
  synchronized_cycle = get_cycles().get();
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

void TMR0::callback_print()
{

  cout << "TMR0\n";
}

// Suspend TMR0 for sleep
void TMR0::sleep()
{

    if(verbose)
	printf("TMR0::sleep state=%d\n", state);

    if((state & RUNNING))
    {
	stop();
	state = SLEEPING;
    }
}
// wake up TMR0 when sleep command terminates
void TMR0::wake()
{
    if(verbose)
	printf("TMR0::wake state=%d\n", state);

    if ((state & SLEEPING))
    {
	if (! (state & RUNNING))
        {
	    state = STOPPED;
	    start(value.get(), 0);
	}
	else
	    state &= ~SLEEPING;
	
    }
}
