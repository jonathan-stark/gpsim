/*
   Copyright (C) 1998 T. Scott Dattalo

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
along with gpasm; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <string>

#include "../config.h"
#include "14bit-tmrs.h"
#include "stimuli.h"
#include "p16x7x.h"

//
// 14bit-tmrs.cc
//
// Timer 1&2  modules for the 14bit core pic processors.
//


//--------------------------------------------------
// CCPRL
//--------------------------------------------------

CCPRL::CCPRL(void)
{

  ccprh = 0;

}

void CCPRL::put(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());

  value.put(new_value);

  if(tmrl->compare_mode)
    start_compare_mode();   // Actually, re-start with new capture value.

}

void CCPRL::capture_tmr(void)
{

  tmrl->get_low_and_high();

  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());
  value.put(tmrl->value.get());

  trace.raw(ccprh->write_trace.get() | ccprh->value.get());
  //trace.register_write(ccprh->address,ccprh->value.get());
  ccprh->value.put(tmrl->tmrh->value.get());

  tmrl->pir_set->set_ccpif();

  int c = value.get() + 256*ccprh->value.get();
  if(verbose & 4)
    cout << "CCPRL captured: " << c << '\n';
}

void CCPRL::start_compare_mode(void)
{

  tmrl->compare_mode = 1;

  int capture_value = value.get() + 256*ccprh->value.get();
  //cout << "start compare mode with capture value = " << capture_value << '\n';
  tmrl->compare_value = capture_value;
  tmrl->update();
}

void CCPRL::stop_compare_mode(void)
{
  // If tmr1 is in the compare mode, then change to non-compare and update
  // the tmr breakpoint.

  if(tmrl->compare_mode)
    {
      tmrl->compare_mode = 0;
      tmrl->update();
    }
}

void CCPRL::start_pwm_mode(void)
{
  //cout << "CCPRL: starting pwm mode\n";

  ccprh->pwm_mode = 1;


}

//--------------------------------------------------
// assign_tmr - assign a new timer to the capture compare module
//
// This was created for the 18f family where it's possible to dynamically
// choose which clock is captured during an event.
//
void CCPRL::assign_tmr(TMRL *ptmr)
{
  if(ptmr) {
    cout << "Reassigning CCPRL clock source\n";

    tmrl = ptmr;
  }

}

//--------------------------------------------------
// CCPRH
//--------------------------------------------------

CCPRH::CCPRH(void)
{

  ccprl = 0;
  pwm_mode = 0;
  pwm_value = 0;
}

void CCPRH::put(unsigned int new_value)
{

  //cout << "CCPRH put \n";

  if(pwm_mode == 0)   // In pwm_mode, CCPRH is a read-only register.
    {
      trace.raw(write_trace.get() | value.get());
      //trace.register_write(address,value.get());

      value.put(new_value);

      if(ccprl->tmrl->compare_mode)
	ccprl->start_compare_mode();   // Actually, re-start with new capture value.

    }
}

unsigned int CCPRH::get(void)
{
  //cout << "CCPRH get\n";

  unsigned int read_value =  (pwm_mode) ? (pwm_value >>2) : value.get();

  trace.raw(read_trace.get() | value.get());
  //trace.register_read(address, read_value);
  return read_value;
}

//--------------------------------------------------
// CCPCON
//--------------------------------------------------
CCPCON::CCPCON(void)
{

  edges = 0;
  adcon0 = 0;

}

void CCPCON::new_edge(unsigned int level)
{
  if(verbose &4)
    cout << "CCPCON processing new edge\n";

  switch(value.get() & (CCPM3 | CCPM2 | CCPM1 | CCPM0))
    {
    case ALL_OFF0:
    case ALL_OFF1:
    case ALL_OFF2:
    case ALL_OFF3:
      //cout << "CCPCON not enabled\n";
      return;

    case CAP_FALLING_EDGE:
      if (level == 0)
	ccprl->capture_tmr();

      //if(level==0) cout << "CCPCON caught falling edge\n";
      break;

    case CAP_RISING_EDGE:
      if (level)
	ccprl->capture_tmr();
      //if(level)cout << "CCPCON caught rising edge\n";
      break;

    case CAP_RISING_EDGE4:
      //cout << "4th rising  level = " << level << '\n';
      if (level)
	{
	  if(--edges <= 0)
	    {
	      ccprl->capture_tmr();
	      edges = 4;
	      //cout << "CCPCON caught 4th rising edge\n";
	    }
	  //else cout << "Saw rising edge, but skipped\n";
	}
      break;


    case CAP_RISING_EDGE16:
      if (level)
	{
	  if(--edges <= 0)
	    {
	      ccprl->capture_tmr();
	      edges = 16;
	      //cout << "CCPCON caught 16th rising edge\n";
	    }
	  //else cout << "Saw rising edge, but skipped\n";
	}
      break;

    case COM_SET_OUT:
    case COM_CLEAR_OUT:
    case COM_INTERRUPT:
    case COM_TRIGGER:
    case PWM0:
    case PWM1:
    case PWM2:
    case PWM3:
      //cout << "CCPCON is set up as an output\n";
      return;
    }
}

void CCPCON::compare_match(void)
{

  //cout << name() << " compare match\n";

  switch(value.get() & (CCPM3 | CCPM2 | CCPM1 | CCPM0))
    {
    case ALL_OFF0:
    case ALL_OFF1:
    case ALL_OFF2:
    case ALL_OFF3:
      //cout << "CCPCON not enabled\n";
      return;

    case CAP_FALLING_EDGE:
    case CAP_RISING_EDGE:
    case CAP_RISING_EDGE4:
    case CAP_RISING_EDGE16:
      //cout << "CCPCON is programmed for capture??\n";
      break;

    case COM_SET_OUT:
      iopin->putDrivingState(true);
      pir_set->set_ccpif();
      break;

    case COM_CLEAR_OUT:
      iopin->putDrivingState(false);
      pir_set->set_ccpif();
      break;

    case COM_INTERRUPT:
      pir_set->set_ccpif();
      break;

    case COM_TRIGGER:
      ccprl->tmrl->clear_timer();
      pir_set->set_ccpif();
      if(adcon0)
	adcon0->start_conversion();

      //if(adcon0) cout << "CCP triggering an A/D\n";

      break;

    case PWM0:
    case PWM1:
    case PWM2:
    case PWM3:
      //cout << "CCPCON is set up as an output\n";
      return;
    }
}

void CCPCON::pwm_match(int level)
{
  //cout << name() << " CCPCON PWM match\n";

  if( (value.get() & PWM0) == PWM0)
    {
      iopin->putDrivingState(level ? true : false);

      // if the level is 'high', then tmr2 == pr2 and the pwm cycle
      // is starting over. In which case, we need to update the duty
      // cycle by reading ccprl and the ccp X & Y and caching them
      // in ccprh's pwm slave register.

      if(level)
	{
	  ccprl->ccprh->pwm_value = ((value.get()>>4) & 3) | 4*ccprl->value.get();
	  tmr2->pwm_dc(ccprl->ccprh->pwm_value, address);
	}

      //cout << "iopin should change\n";
    }
  else
    {
      cout << "not pwm mode. bug?\n";
    }
}

void CCPCON::put(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());

  value.put(new_value);
  switch(value.get() & (CCPM3 | CCPM2 | CCPM1 | CCPM0))
    {
    case ALL_OFF0:
    case ALL_OFF1:
    case ALL_OFF2:
    case ALL_OFF3:
      ccprl->stop_compare_mode();
      tmr2->stop_pwm(address);
      break;
    case CAP_FALLING_EDGE:
    case CAP_RISING_EDGE:
      edges = 0;
      ccprl->stop_compare_mode();
      tmr2->stop_pwm(address);
      break;

    case CAP_RISING_EDGE4:
      edges &= 3;
      ccprl->stop_compare_mode();
      tmr2->stop_pwm(address);
      break;

    case CAP_RISING_EDGE16:
      ccprl->stop_compare_mode();
      tmr2->stop_pwm(address);
      break;

    case COM_SET_OUT:
    case COM_CLEAR_OUT:
    case COM_INTERRUPT:
    case COM_TRIGGER:
      ccprl->tmrl->ccpcon = this;
      ccprl->start_compare_mode();
      tmr2->stop_pwm(address);

      if(adcon0)
	adcon0->start_conversion();

      //if(adcon0) cout << "CCP triggering an A/D\n";

      break;
    case PWM0:
    case PWM1:
    case PWM2:
    case PWM3:
      ccprl->stop_compare_mode();
      ccprl->start_pwm_mode();
      tmr2->pwm_dc( ccprl->ccprh->pwm_value, address);
      break;

    }
}

//--------------------------------------------------
// T1CON
//--------------------------------------------------
T1CON::T1CON(void)
{

  new_name("T1CON");

}

void T1CON::put(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());

  unsigned int diff = value.get() ^ new_value;
  value.put(new_value);
  
  // First, check the tmr1 clock source bit to see if we are  changing from
  // internal to external (or vice versa) clocks.
  if( diff & TMR1CS)
    tmrl->new_clock_source();

  if( diff & TMR1ON)
    tmrl->on_or_off(value.get() & TMR1ON);
  else  if( diff & (T1CKPS0 | T1CKPS1))
    tmrl->update();

}

unsigned int T1CON::get(void)
{
  trace.raw(read_trace.get() | value.get());
  //trace.register_read(address,value.get());
  return(value.get());
}

unsigned int T1CON::get_prescale(void)
{
  return( ((value.get() &(T1CKPS0 | T1CKPS1)) >> 4) + cpu_pic->pll_factor);
}



//--------------------------------------------------
// member functions for the TMRH base class
//--------------------------------------------------
TMRH::TMRH(void)
{

  value.put(0);
  new_name("TMRH");

}

void TMRH::put(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());
  value.put(new_value & 0xff);

  tmrl->synchronized_cycle = get_cycles().value;
  tmrl->last_cycle = tmrl->synchronized_cycle - (tmrl->value.get() + (value.get()<<8))*tmrl->prescale;

  if(tmrl->t1con->get_tmr1on())
    tmrl->update();

}

unsigned int TMRH::get(void)
{

  trace.raw(read_trace.get() | value.get());
  return get_value();
}

// For the gui and CLI
unsigned int TMRH::get_value(void)
{
  // If the TMR1 is being read immediately after being written, then
  // it hasn't had enough time to synchronize with the PIC's clock.

  if(get_cycles().value <= tmrl->synchronized_cycle)
    return value.get();

  // If he TMR is not running then return.
  if(!tmrl->t1con->get_tmr1on())
    return value.get();

  tmrl->current_value();

  value.put(((tmrl->value_16bit)>>8) & 0xff);

  return(value.get());
  
}


//--------------------------------------------------
// member functions for the TMRL base class
//--------------------------------------------------
TMRL::TMRL(void)
{

  value.put(0);
  synchronized_cycle=0;
  prescale_counter=prescale=1;
  break_value = 0x10000;
  compare_value = 0;
  compare_mode = 0;
  last_cycle = 0;

  new_name("TMRL");

}

// %%%FIX ME%%% 
void TMRL::increment(void)
{
  if(verbose & 4)
    cout << "TMRL increment because of external clock ";

  if(--prescale_counter == 0) {
    prescale_counter = prescale;

    // If TMRH/TMRL have been manually changed, we'll want to 
    // get the up-to-date values;

    trace.raw(write_trace.get() | value.get());
    //trace.register_write(address,value.get());
    current_value();

    value_16bit = 0xffff & ( value_16bit + 1);

    tmrh->value.put((value_16bit >> 8) & 0xff);
    value.put(value_16bit & 0xff);
    if(value_16bit == 0)
      pir_set->set_tmr1if();
  }

}

//
void TMRL::on_or_off(int new_state)
{

  if(new_state)
    {
      if(verbose & 0x4)
	cout << "TMR1 is being turned on\n";
      // turn on the timer

      // Effective last cycle
      // Compute the "effective last cycle", i.e. the cycle
      // at which TMR1 was last 0 had it always been counting.

      last_cycle = get_cycles().value - value_16bit*prescale;
      update();
    }
  else
    {
      if(verbose & 0x4)
	cout << "TMR1 is being turned off\n";

      // turn off the timer and save the current value
      current_value();
      value.put(value_16bit & 0xff);
      tmrh->value.put((value_16bit>>8) & 0xff);
    }

}
//
// If anything has changed to affect when the next TMR1 break point
// will occur, this routine will make sure the break point is moved
// correctly.
//

void TMRL::update(void)
{

  if(verbose & 0x4)
    cout << "TMR1 update "  << hex << get_cycles().value << '\n';

  if(t1con->get_tmr1on())
    {
      if(t1con->get_tmr1cs())
	{
	  cout << "TMR1::put external clock (not implemented)...\n";
	}
      else
	{
	  if(verbose & 0x4)
	    cout << "Internal clock\n";

	  current_value();


	  // Note, unlike TMR0, anytime something is written to TMRL, the 
	  // prescaler is unaffected.

	  prescale = 1 << t1con->get_prescale();
	  prescale_counter = prescale;

	  //cout << "TMRL: Current prescale " << prescale << '\n';
	  //  synchronized_cycle = cycles.value + 2;
	  synchronized_cycle = get_cycles().value;

	  last_cycle = synchronized_cycle - value_16bit * prescale;

	  break_value = 0x10000;  // Assume that a rollover will happen first.

	  if(compare_mode)
	    {
	      //cout << "compare mode. compare_value = " << compare_value << '\n';
	      if(compare_value > value_16bit)
		{
		  // A compare interrupt is going to happen before the timer
		  // will rollover.
		  break_value = compare_value - value_16bit;
		}
	    }

	  guint64 fc = get_cycles().value + (break_value - value_16bit) * prescale;

	  if(future_cycle)
	    get_cycles().reassign_break(future_cycle, fc, this);
	  else
	    get_cycles().set_break(fc, this);

	  //cout << "TMR1: update; new break cycle = " << fc << '\n';
	  future_cycle = fc;
	}
    }
  else
    {
      //cout << "TMR1: not running\n";
    }
}

void TMRL::put(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());
  value.put(new_value & 0xff);

  synchronized_cycle = get_cycles().value;
  last_cycle = synchronized_cycle - ( value.get() + (tmrh->value.get()<<8)) * prescale;

  if(t1con->get_tmr1on())
    update();

}

unsigned int TMRL::get(void)
{
  trace.raw(read_trace.get() | value.get());
  return get_value();
}

// For the gui and CLI
unsigned int TMRL::get_value(void)
{
  // If the TMRL is being read immediately after being written, then
  // it hasn't had enough time to synchronize with the PIC's clock.
  if(get_cycles().value <= synchronized_cycle)
    return value.get();

  // If TMRL is not on, then return the current value
  if(!t1con->get_tmr1on())
    return value.get();

  current_value();

  value.put(value_16bit & 0xff);

  return(value.get());
}

//%%%FIXME%%% inline this
void TMRL::current_value(void)
{
  if(t1con->get_tmr1cs())
    value_16bit = tmrh->value.get() * 256 + value.get();
  else
    value_16bit = (unsigned int)((get_cycles().value - last_cycle)/ prescale) & 0xffff;
}

unsigned int TMRL::get_low_and_high(void)
{

  // If the TMRL is being read immediately after being written, then
  // it hasn't had enough time to synchronize with the PIC's clock.
  if(get_cycles().value <= synchronized_cycle)
    return value.get();

  //  value_16bit = (cycles.value.lo - last_cycle)/ prescale;

  current_value();

  value.put(value_16bit & 0xff);
  trace.raw(read_trace.get() | value.get());
  //trace.register_read(address, value.get());

  tmrh->value.put((value_16bit>>8) & 0xff);
  trace.raw(tmrh->read_trace.get() | tmrh->value.get());
  //trace.register_read(tmrh->address, tmrh->value.get());

  return(value_16bit);
  
}

void TMRL::new_clock_source(void)
{

  //cout << "TMRL:new_clock_source changed to the ";
  if(t1con->get_tmr1cs())
    {
      cout << "TMR1::new_clock_source external clock (not implemented)...\n";
    }
  else
    {
      //cout << "internal\n";
      put(value.get());    // let TMRL::put() set a cycle counter break point
    }
}

//
// clear_timer - This is called by either the CCP or PWM modules to 
// reset the timer to zero. This is rather easy since the current TMR
// value is always referenced to the cpu cycle counter. 
//

void TMRL::clear_timer(void)
{

  last_cycle = get_cycles().value;
  //cout << "TMR1 has been cleared\n";
}

// TMRL callback is called when the cycle counter hits the break point that
// was set in TMRL::put. The cycle counter will clear the break point, so
// we don't need to worry about it. At this point, TMRL is rolling over.

void TMRL::callback(void)
{

  if(verbose & 4)
    cout << "TMRL::callback\n";

  // If TMRL is being clocked by the external clock, then at some point
  // the simulate code must have switched from the internal clock to
  // external clock. The cycle break point was still set, so just ignore it.
  if(t1con->get_tmr1cs())
    {
      future_cycle = 0;  // indicates that TMRL no longer has a break point
      return;
    }

  future_cycle = 0;     // indicate that there's no break currently set
  //cout << "in tmrl callback break_value = " << break_value << '\n';

  if(break_value < 0x10000)
    {

      // The break was due to a "compare"

      //cout << "TMR1 break due to compare "  << hex << cycles.value.lo << '\n';
      ccpcon->compare_match();

    }
  else
    {

      // The break was due to a roll-over

      //cout<<"TMRL rollover: " << hex << cycles.value.lo << '\n';
      pir_set->set_tmr1if();

      // Reset the timer to 0.

      synchronized_cycle = get_cycles().value;
      last_cycle = synchronized_cycle;

    }

  update();

}

//---------------------------

void TMRL::callback_print(void)
{
  cout << "TMRL " << name() << " CallBack ID " << CallBackID << '\n';

}


//--------------------------------------------------
// member functions for the PR2 base class
//--------------------------------------------------

PR2::PR2(void)
{

  new_name("PR2");

}

void PR2::put(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());

  if(value.get() != new_value)
    {
      value.put(new_value);
      tmr2->new_pr2();
    }
  else
    value.put(new_value);

}

//--------------------------------------------------
// member functions for the T2CON base class
//--------------------------------------------------

T2CON::T2CON(void)
{

  new_name("T2CON");

}

void T2CON::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());
  value.put(new_value);
  tmr2->new_pre_post_scale();

}



//--------------------------------------------------
// member functions for the TMR2 base class
//--------------------------------------------------
TMR2::TMR2(void)
{
  update_state = TMR2_PWM1_UPDATE | TMR2_PWM2_UPDATE | TMR2_PR2_UPDATE;
  pwm_mode = 0;
  value.put(0);
  synchronized_cycle=0;
  future_cycle = 0;
  prescale=1;
  new_name("TMR2");
}

void TMR2::callback_print(void) 
{
  cout << "TMR2 " << name() << " CallBack ID " << CallBackID << '\n';
}

void TMR2::start(void)
{

  value.put(0);
  prescale = 0;
  last_cycle = 0;
  future_cycle = 0;

}

//
// pwm_dc - 
//
//  

void TMR2::pwm_dc(unsigned int dc, unsigned int ccp_address)
{

  //cout << "TMR2_PWM constants pwm1 " << TMR2_PWM1_UPDATE << " pwm2 " << TMR2_PWM2_UPDATE << '\n';
  if(ccp_address == ccp1con->address)
    {
      //cout << "TMR2:  pwm mode with ccp1. duty cycle = " << hex << dc << '\n';

      duty_cycle1 = dc;

      // Update the cycle break iff this is the first time to go into pwm mode
      if( (pwm_mode & TMR2_PWM1_UPDATE) == 0)
	{
	  pwm_mode |= TMR2_PWM1_UPDATE;
	  update();
	}
    }
  else if(ccp_address == ccp2con->address)
    {
      //cout << "TMR2: starting pwm mode with ccp2. duty cycle = " << hex << dc << '\n';

      duty_cycle2 = dc;

      // Update the cycle break iff this is the first time to go into pwm mode
      if( (pwm_mode & TMR2_PWM2_UPDATE) == 0)
	{
	  pwm_mode |= TMR2_PWM2_UPDATE;

	  update();
	}
    }
  else
    {
      cout << "TMR2: error bad ccpxcon address while in pwm_dc()\n";
      cout << "ccp_address = " << ccp_address << " expected 1con " << 
	ccp1con->address << " or 2con " << ccp2con->address << '\n';
    }

}

//
// stop_pwm
//

void TMR2::stop_pwm(unsigned int ccp_address)
{

  int old_pwm = pwm_mode;

  if(ccp_address == ccp1con->address)
    {
      // cout << "TMR2:  stopping pwm mode with ccp1.\n";
      pwm_mode &= ~TMR2_PWM1_UPDATE;
    }
  else if(ccp_address == ccp2con->address)
    {
      // cout << "TMR2:  stopping pwm mode with ccp2.\n";
      pwm_mode &= ~TMR2_PWM2_UPDATE;
    }

  if(pwm_mode ^ old_pwm)
    update();

}

//
// update 
//  This member function will determine if/when there is a TMR2 break point
// that needs to be set and will set/move it if so.
//  There are three different break sources: 1) TMR2 matching PR2 2) TMR2 matching
// the ccp1 registers in pwm mode and 3) TMR2 matching the ccp2 registers in pwm
// mode.
//

void TMR2::update(int ut)
{

  //cout << "TMR2 update. cpu cycle " << cycles.value <<'\n';

  if(t2con->get_tmr2on())
    {
      if(future_cycle)
	{
	  // If TMR2 is enabled (i.e. counting) then 'future_cycle' is non-zero,
	  // which means there's a cycle break point set on TMR2 that needs to
	  // be moved to a new cycle.

	  current_value();

	  // Assume that we are not in pwm mode (and hence the next break will
	  // be due to tmr2 matching pr2)

	  break_value = (1 + pr2->value.get()) << 2;
	  unsigned int pwm_break_value = break_value;

	  last_update = TMR2_PR2_UPDATE;

	  if(pwm_mode & ut & TMR2_PWM1_UPDATE)
	    {
	      // We are in pwm mode... So let's see what happens first: a pr2 compare
	      // or a duty cycle compare. (recall, the duty cycle is really 10-bits)

	      if( (duty_cycle1 > (value.get()*4*prescale) ) && (duty_cycle1 < break_value))
		{
		  pwm_break_value = duty_cycle1;
		  last_update = TMR2_PWM1_UPDATE;
		  //cout << "TMR2:PWM1 update\n";
		}
	    }

	  if(pwm_mode & ut & TMR2_PWM2_UPDATE)
	    {
	      // We are in pwm mode... So let's see what happens first: a pr2 compare
	      // or a duty cycle compare. (recall, the duty cycle is really 10-bits)

	      if( (duty_cycle2 > (value.get()*4*prescale) ) && (duty_cycle2 < break_value))
		{
		  pwm_break_value = duty_cycle2;
		  last_update = TMR2_PWM2_UPDATE;
		  //cout << "TMR2:PWM2 update\n";
		}
	    }


	  // If TMR2 is configured for pwm'ing,
	  // make sure that the new break cycle is beyond the current one (this
	  // is necessary because the 'duty_cycle' may be larger than the 'period'.)
	  //cout << "TMR2: break_value " <<hex<<break_value << " pwm_break_value " << pwm_break_value <<'\n';
	  if(pwm_break_value >= break_value)
	    {
	      last_update = TMR2_PR2_UPDATE;
	      update_state = TMR2_PWM1_UPDATE | TMR2_PWM2_UPDATE | TMR2_PR2_UPDATE;
	      last_cycle = get_cycles().value;
	    }
	  else
	    break_value = pwm_break_value;

	  guint64 fc = last_cycle + ((break_value>>2) - value.get())  * prescale;

	  if(fc <= future_cycle)
	    {
	      cout << "TMR2: update BUG! future_cycle is screwed\n";
	    }

	  //cout << "TMR2: update new break at cycle "<<hex<<fc<<'\n';
	  get_cycles().reassign_break(future_cycle, fc, this);

	  future_cycle = fc;


	}
      else
	{
	  cout << "TMR2 BUG!! tmr2 is on but has no cycle_break set on it\n";
	}

    }
  else
    {
      //cout << "TMR2 is not running (no update occurred)\n";
    }

}
void TMR2::put(unsigned int new_value)
{


  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());

  value.put(new_value & 0xff);

  if(future_cycle)
    {
      // If TMR2 is enabled (i.e. counting) then 'future_cycle' is non-zero,
      // which means there's a cycle break point set on TMR2 that needs to
      // be moved to a new cycle.

      last_cycle = get_cycles().value;
      guint64 fc = last_cycle + ((pr2->value.get() - value.get()) & 0xff) * prescale;

      get_cycles().reassign_break(future_cycle, fc, this);

      future_cycle = fc;

      // 'clear' the post scale counter. (I've actually implemented the post scale counter
      // as a count-down counter, so 'clearing it' means resetting it to the starting point.

      post_scale = t2con->get_post_scale();
    }
}

unsigned int TMR2::get(void)
{

  if(t2con->get_tmr2on())
    {
      ///int new_value = (cycles.value - last_cycle)/ prescale;

      ///value = new_value;

      current_value();
    }

  trace.raw(read_trace.get() | value.get());
  // trace.register_read(address, value.get());
  return(value.get());
  
}

unsigned int TMR2::get_value(void)
{

  if(t2con->get_tmr2on())
    {
      ///int new_value = (cycles.value - last_cycle)/ prescale;

      ///value = new_value;

      current_value();
    }

  return(value.get());
  
}
void TMR2::new_pre_post_scale(void)
{

  //cout << "T2CON was written to, so update TMR2\n";

  if(!t2con->get_tmr2on()) {
    // TMR2 is not on. If has just been turned off, clear the callback breakpoint.

    if(future_cycle) {
      get_cycles().clear_break(this);
      future_cycle = 0;
      return;
    }
  }

  if(future_cycle)
    {
      // If TMR2 is enabled (i.e. counting) then 'future_cycle' is non-zero,
      // which means there's a cycle break point set on TMR2 that needs to
      // be moved to a new cycle.

      // Get the current value of TMR2
      ///value = (cycles.value - last_cycle)/prescale;
      current_value();

      //cout << "cycles " << cycles.value.lo  << " old prescale " << prescale;

      prescale = t2con->get_pre_scale();

      //cout << " prescale " << prescale;

      // Now compute the 'last_cycle' as though if TMR2 had been running on the 
      // new prescale all along. Recall, 'last_cycle' records the value of the cpu's
      // cycle counter when TMR2 last rolled over.

      last_cycle = get_cycles().value - value.get() * prescale;
      //cout << " effective last_cycle " << last_cycle << '\n';

      //cout << "tmr2's current value " << value << '\n';

      guint64 fc = get_cycles().value;

      if(pr2->value.get() == value.get())
	fc += 0x100 * prescale;
      else
	fc +=  ((pr2->value.get() - value.get()) & 0xff) * prescale;

      //cout << "moving break from " << future_cycle << " to " << fc << '\n';

      get_cycles().reassign_break(future_cycle, fc, this);

      future_cycle = fc;
    }
  else
    {
      //cout << "TMR2 was off, but now it's on.\n";

      prescale = t2con->get_pre_scale();
      if(pr2->value.get() == value.get())
	future_cycle = 0x100 * prescale;
      else
	future_cycle =  ((pr2->value.get() - value.get()) & 0xff) * prescale;

      last_cycle = get_cycles().value;
      future_cycle += get_cycles().value;
      get_cycles().set_break(future_cycle, this);
    }

  post_scale = t2con->get_post_scale();

}

void TMR2::new_pr2(void)
{

  if(t2con->get_tmr2on())
    {
      //update the tmr2 break point...
      ///      value = (cycles.value - last_cycle)/ prescale;
      current_value();

      // Get the current value of the prescale counter (because
      // writing to pr2 doesn't affect the pre/post scale counters).

      guint64 curr_prescale = value.get() * prescale - (get_cycles().value - last_cycle);

      guint64 fc = get_cycles().value + curr_prescale;

      if(pr2->value.get() == value.get())
	{  // May wanta ignore the == case and instead allow the cycle break handle it...
	  fc += 0x100 * prescale;
	  last_cycle += 0x100 * prescale;
	}
      else
	fc +=  ((pr2->value.get() - value.get()) & 0xff) * prescale;

      get_cycles().reassign_break(future_cycle, fc, this);

      future_cycle = fc;
    }


}

void TMR2::current_value(void)
{
//  value.put((unsigned int)((get_cycles().value - last_cycle)/ prescale));
  value.put((unsigned int)(pr2->value.get() - (future_cycle - get_cycles().value)/ prescale ));
// MGF

  if(value.get()>0xff)
    cout << "TMR2 BUG!! value = " << value.get() << " which is greater than 0xff\n";
}

// TMR2 callback is called when the cycle counter hits the break point that
// was set in TMR2::put. The cycle counter will clear the break point, so
// we don't need to worry about it. At this point, TMR2 is equal to PR2.

void TMR2::callback(void)
{

  //cout<<"TMR2 callback cycle: " << hex << cycles.value << '\n';

  // If tmr2 is still enabled, then set up for the next break. 
  // If tmr2 was disabled then ignore this break point.
  if(t2con->get_tmr2on())
    {

      // What caused the callback: PR2 match or duty cyle match ?

      //if((pwm_mode) && (break_value == duty_cycle1))
      if(last_update == TMR2_PWM1_UPDATE)
	{
	  // duty cycle match
	  //cout << "TMR2: duty cycle match for pwm1 \n";
	  update_state &= (~TMR2_PWM1_UPDATE);
	  ccp1con->pwm_match(0);
	}
      else if(last_update == TMR2_PWM2_UPDATE)
	{
	  // duty cycle match
	  //cout << "TMR2: duty cycle match for pwm2 \n";
	  update_state &= (~TMR2_PWM2_UPDATE);
	  ccp2con->pwm_match(0);
	}
      else
	{
	  // matches PR2

	  //cout << "TMR2: PR2 match. pwm_mode is " << pwm_mode <<'\n';

	  // This (implicitly) resets the timer to zero:
	  last_cycle = get_cycles().value;

	  if(pwm_mode & TMR2_PWM1_UPDATE)
	    ccp1con->pwm_match(1);

	  if(pwm_mode & TMR2_PWM2_UPDATE)
	    ccp2con->pwm_match(1);

	  if(--post_scale < 0)
	    {
	      //cout << "setting IF\n";
	      pir_set->set_tmr2if();
	      post_scale = t2con->get_post_scale();
	    }

	  update_state = TMR2_PWM1_UPDATE | TMR2_PWM2_UPDATE | TMR2_PR2_UPDATE;

	}
      update(update_state);

    }
  else
    future_cycle = 0;
}


//------------------------------------------------------------------------
// TMR2_MODULE
//
// 

TMR2_MODULE::TMR2_MODULE(void)
{

  t2con = 0;
  pr2   = 0;
  tmr2  = 0;

}

void TMR2_MODULE::initialize(T2CON *t2con_, PR2 *pr2_, TMR2  *tmr2_)
{

  t2con = t2con_;
  pr2   = pr2_;
  tmr2  = tmr2_;

}
