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
#include <iostream.h>
#include <iomanip.h>
#include <string>

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

  ccprh = NULL;

}

void CCPRL::put(unsigned int new_value)
{

  //cout << "CCPRL put \n";

  value = new_value;

  if(tmr1l->compare_mode)
    start_compare_mode();   // Actually, re-start with new capture value.

  trace.register_write(address,value);

}

void CCPRL::capture_tmr(void)
{

  tmr1l->get_low_and_high();

  value = tmr1l->value;
  trace.register_write(address,value);

  ccprh->value = tmr1l->tmr1h->value;
  trace.register_write(ccprh->address,ccprh->value);

  tmr1l->pir1->set_ccpif();

  int c = value + 256*ccprh->value;
  //cout << "CCPRL captured: " << c << '\n';
}

void CCPRL::start_compare_mode(void)
{

  tmr1l->compare_mode = 1;

  int capture_value = value + 256*ccprh->value;
  //cout << "start compare mode with capture value = " << capture_value << '\n';
  tmr1l->compare_value = capture_value;
  tmr1l->update();
}

void CCPRL::stop_compare_mode(void)
{
  // If tmr1 is in the compare mode, then change to non-compare and update
  // the tmr breakpoint.

  if(tmr1l->compare_mode)
    {
      tmr1l->compare_mode = 0;
      tmr1l->update();
    }
}

void CCPRL::start_pwm_mode(void)
{
  //cout << "CCPRL: starting pwm mode\n";

  ccprh->pwm_mode = 1;


}

//--------------------------------------------------
// CCPRH
//--------------------------------------------------

CCPRH::CCPRH(void)
{

  ccprl = NULL;
  pwm_mode = 0;
  pwm_value = 0;
}

void CCPRH::put(unsigned int new_value)
{

  //cout << "CCPRH put \n";

  if(pwm_mode == 0)   // In pwm_mode, CCPRH is a read-only register.
    {
      value = new_value;

      if(ccprl->tmr1l->compare_mode)
	ccprl->start_compare_mode();   // Actually, re-start with new capture value.

      trace.register_write(address,value);
    }
}

unsigned int CCPRH::get(void)
{
  //cout << "CCPRH get\n";

  unsigned int read_value =  (pwm_mode) ? (pwm_value >>2) : value;

  trace.register_read(address, read_value);
}

//--------------------------------------------------
// CCPCON
//--------------------------------------------------
CCPCON::CCPCON(void)
{

  edges = 0;
  adcon0 = NULL;

}

void CCPCON::new_edge(unsigned int level)
{

  //cout << "CCPCON processing new edge\n";

  switch(value & (CCPM3 | CCPM2 | CCPM1 | CCPM0))
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

  switch(value & (CCPM3 | CCPM2 | CCPM1 | CCPM0))
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
      iopin->put_state(1);
      pir->set_ccpif();
      break;

    case COM_CLEAR_OUT:
      iopin->put_state(0);
      pir->set_ccpif();
      break;

    case COM_INTERRUPT:
      pir->set_ccpif();
      break;

    case COM_TRIGGER:
      ccprl->tmr1l->clear_timer();
      pir->set_ccpif();
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

  if( (value & PWM0) == PWM0)
    {
      iopin->put_state(level);

      // if the level is 'high', then tmr2 == pr2 and the pwm cycle
      // is starting over. In which case, we need to update the duty
      // cycle by reading ccprl and the ccp X & Y and caching them
      // in ccprh's pwm slave register.

      if(level)
	{
	  ccprl->ccprh->pwm_value =  ((value>>4) & 3) | 4*ccprl->value;
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

  //cout << name() << " new value " << new_value << '\n';
  value = new_value;
  switch(value & (CCPM3 | CCPM2 | CCPM1 | CCPM0))
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
      ccprl->tmr1l->ccpcon = this;
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

  trace.register_write(address,value);

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

  unsigned int diff = value ^ new_value;
  value = new_value;
  
  // First, check the tmr1 clock source bit to see if we are  changing from
  // internal to external (or vice versa) clocks.
  if( diff & TMR1CS)
    tmr1l->new_clock_source();

  if( diff & TMR1ON)
    tmr1l->on_or_off(value & TMR1ON);
  else  if( diff & (T1CKPS0 | T1CKPS1))
    tmr1l->update();

  trace.register_write(address,value);

}
//--------------------------------------------------
// PIR1
//--------------------------------------------------
PIR1::PIR1(void)
{

  valid_bits = TMR1IF | TMR2IF | CCP1IF | SSPIF | TXIF | RCIF | PSPIF;

  break_point = 0;
  new_name("pir1");

}


//--------------------------------------------------
// PIR2
//--------------------------------------------------
PIR2::PIR2(void)
{

  break_point = 0;
  new_name("pir2");

}

//--------------------------------------------------
// PIE
//--------------------------------------------------
void PIE::put(unsigned int new_value)
{
  value = new_value;
  trace.register_write(address,value);

  if( pir->interrupt_status())
    {
      pir->intcon->peripheral_interrupt();
    }

}


//--------------------------------------------------
// member functions for the TMR1H base class
//--------------------------------------------------
TMR1H::TMR1H(void)
{

  break_point = 0;
  value=0;
  new_name("TMR1H");

}

void TMR1H::put(unsigned int new_value)
{

  value = new_value & 0xff;
  if(tmr1l->t1con->get_tmr1on())
    tmr1l->update();
  trace.register_write(address,value);

}


//--------------------------------------------------
// member functions for the TMR1L base class
//--------------------------------------------------
TMR1L::TMR1L(void)
{

  break_point = 0;
  value=0;
  synchronized_cycle=0;
  prescale=1;
  break_value = 0x10000;
  compare_value = 0;
  compare_mode = 0;
  last_cycle = 0;

  new_name("TMR1L");

}

// %%%FIX ME%%% 
void TMR1L::increment(void)
{
  //  cout << "TMR1L increment because of external clock ";

  if(--prescale_counter == 0)
    {
      prescale_counter = prescale;
      if(++value == 256)
	{
	  value = 0;
	  cpu14->intcon->set_t0if();
	}
      trace.register_write(address,value);
    }
  //  cout << value << '\n';
}

//
void TMR1L::on_or_off(int new_state)
{

  if(new_state)
    {
      //cout << "TMR1 is being turned on\n";
      // turn on the timer

      // Effective last cycle
      last_cycle = cpu->cycles.value - value_16bit*prescale;
      update();
    }
  else
    {
      //cout << "TMR1 is being turned off\n";
      // turn off the timer and save the current value
      current_value();
      value = value_16bit & 0xff;
      tmr1h->value = (value_16bit>>8) & 0xff;
    }

}
//
// If anything has changed to affect when the next TMR1 break point
// will occur, this routine will make sure the break point is moved
// correctly.
//

void TMR1L::update(void)
{

  //cout << "TMR1 update "  << hex << cpu->cycles.value.lo << '\n';
  if(t1con->get_tmr1on())
    {
      if(t1con->get_tmr1cs())
	{
	  cout << "TMR1::put external clock (not implemented)...\n";
	}
      else
	{
	  //cout << "Internal clock\n";

	  //value_16bit = ((tmr1h->value & 0xff) << 8) | value;
	  current_value();

	  //cout << "Current value " << value_16bit << '\n';

	  // Note, unlike TMR0, anytime something is written to TMR1L, the 
	  // prescaler is unaffected.

	  prescale = 1 << t1con->get_prescale();
	  prescale_counter = prescale;

	  //  synchronized_cycle = cpu->cycles.value + 2;
	  synchronized_cycle = cpu->cycles.value;

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

	  guint64 fc = cpu->cycles.value + (break_value - value_16bit) * prescale;

	  if(future_cycle)
	    cpu->cycles.reassign_break(future_cycle, fc, this);
	  else
	    cpu->cycles.set_break(fc, this);

	  //cout << "TMR1: update; new break cycle = " << fc << '\n';
	  future_cycle = fc;
	}
    }
  else
    {
      //cout << "TMR1: not running\n";
    }
}

void TMR1L::put(unsigned int new_value)
{

  value = new_value & 0xff;
  if(t1con->get_tmr1on())
    update();
  trace.register_write(address,value);

}

unsigned int TMR1L::get(void)
{

  // If the TMR1L is being read immediately after being written, then
  // it hasn't had enough time to synchronize with the PIC's clock.
  if(cpu->cycles.value <= synchronized_cycle)
    return value;

  //  int new_value = (cpu->cycles.value - last_cycle)/ prescale;
  current_value();

  value = (value_16bit & 0xff);
  trace.register_read(address, value);
  return(value);
  
}


//%%%FIXME%%% inline this
void TMR1L::current_value(void)
{
  value_16bit = ((cpu->cycles.value - last_cycle)/ prescale) & 0xffff;
}

unsigned int TMR1L::get_low_and_high(void)
{

  // If the TMR1L is being read immediately after being written, then
  // it hasn't had enough time to synchronize with the PIC's clock.
  if(cpu->cycles.value <= synchronized_cycle)
    return value;

  //  value_16bit = (cpu->cycles.value.lo - last_cycle)/ prescale;

  current_value();

  value = (value_16bit & 0xff);
  trace.register_read(address, value);

  tmr1h->value = (value_16bit>>8) & 0xff;
  trace.register_read(tmr1h->address, tmr1h->value);

  return(value_16bit);
  
}

void TMR1L::new_clock_source(void)
{

  //cout << "TMR1L:new_clock_source changed to the ";
  if(cpu14->option_reg.get_t0cs())
    {
      //cout << "external\n";
      //      cpu->cycles.
    }
  else
    {
      //cout << "internal\n";
      put(value);    // let TMR1L::put() set a cycle counter break point
    }
}

//
// clear_timer - This is called by either the CCP or PWM modules to 
// reset the timer to zero. This is rather easy since the current TMR
// value is always referenced to the cpu cycle counter. 
//

void TMR1L::clear_timer(void)
{

  last_cycle = cpu->cycles.value;
  //cout << "TMR1 has been cleared\n";
}

// TMR1L callback is called when the cycle counter hits the break point that
// was set in TMR1L::put. The cycle counter will clear the break point, so
// we don't need to worry about it. At this point, TMR1L is rolling over.

void TMR1L::callback(void)
{

  // If TMR1L is being clocked by the external clock, then at some point
  // the simulate code must have switched from the internal clock to
  // external clock. The cycle break point was still set, so just ignore it.
  if(t1con->get_tmr1cs())
    {
      future_cycle = 0;  // indicates that TMR1L no longer has a break point
      return;
    }

  future_cycle = 0;     // indicate that there's no break currently set
  //cout << "in tmr1l callback break_value = " << break_value << '\n';

  if(break_value < 0x10000)
    {

      // The break was due to a "compare"

      //cout << "TMR1 break due to compare "  << hex << cpu->cycles.value.lo << '\n';
      ccpcon->compare_match();

    }
  else
    {

      // The break was due to a roll-over

      //cout<<"TMR1L rollover: " << hex << cpu->cycles.value.lo << '\n';
      pir1->set_tmr1if();

      // Reset the timer to 0.

      synchronized_cycle = cpu->cycles.value;
      last_cycle = synchronized_cycle;

    }

  update();

}








//---------------------------


//--------------------------------------------------
// member functions for the PR2 base class
//--------------------------------------------------

PR2::PR2(void)
{

  new_name("PR2");

}

void PR2::put(unsigned int new_value)
{

  if(value != new_value)
    {
      value = new_value;
      tmr2->new_pr2();
    }
  else
    value = new_value;

  trace.register_write(address,value);

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
  value = new_value;
  tmr2->new_pre_post_scale();
  trace.register_write(address,value);

}



//--------------------------------------------------
// member functions for the TMR2 base class
//--------------------------------------------------
TMR2::TMR2(void)
{
  update_state = TMR2_PWM1_UPDATE | TMR2_PWM2_UPDATE | TMR2_PR2_UPDATE;
  pwm_mode = 0;
  break_point = 0;
  value=0;
  synchronized_cycle=0;
  future_cycle = 0;
  prescale=1;
  new_name("TMR2");
}

void TMR2::start(void)
{

  value = 0;
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

void TMR2::update(int ut = TMR2_DONTCARE_UPDATE)
{

  //cout << "TMR2 update. cpu cycle " << cpu->cycles.value <<'\n';

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

	  break_value = (1 + pr2->value) << 2;
	  unsigned int pwm_break_value = break_value;

	  last_update = TMR2_PR2_UPDATE;

	  if(pwm_mode & ut & TMR2_PWM1_UPDATE)
	    {
	      // We are in pwm mode... So let's see what happens first: a pr2 compare
	      // or a duty cycle compare. (recall, the duty cycle is really 10-bits)

	      if( (duty_cycle1 > (value*4*prescale) ) && (duty_cycle1 < break_value))
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

	      if( (duty_cycle2 > (value*4*prescale) ) && (duty_cycle2 < break_value))
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
	      last_cycle = cpu->cycles.value;
	    }
	  else
	    break_value = pwm_break_value;

	  guint64 fc = last_cycle + (((break_value>>2) - value) & 0xff) * prescale;

	  if(fc <= future_cycle)
	    {
	      cout << "TMR2: update BUG! future_cycle is screwed\n";
	    }

	  //cout << "TMR2: update new break at cycle "<<hex<<fc<<'\n';
	  cpu->cycles.reassign_break(future_cycle, fc, this);

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


  value = new_value & 0xff;

  if(future_cycle)
    {
      // If TMR2 is enabled (i.e. counting) then 'future_cycle' is non-zero,
      // which means there's a cycle break point set on TMR2 that needs to
      // be moved to a new cycle.

      last_cycle = cpu->cycles.value;
      guint64 fc = last_cycle + ((pr2->value - value) & 0xff) * prescale;

      cpu->cycles.reassign_break(future_cycle, fc, this);

      future_cycle = fc;

      // 'clear' the post scale counter. (I've actually implemented the post scale counter
      // as a count-down counter, so 'clearing it' means resetting it to the starting point.

      post_scale = t2con->get_post_scale();
    }

  trace.register_write(address,value);

}

unsigned int TMR2::get(void)
{

  if(t2con->get_tmr2on())
    {
      ///int new_value = (cpu->cycles.value - last_cycle)/ prescale;

      ///value = new_value;

      current_value();
    }

  trace.register_read(address, value);
  return(value);
  
}

void TMR2::new_pre_post_scale(void)
{

  //cout << "T2CON was written to, so update TMR2\n";

  if(future_cycle)
    {
      // If TMR2 is enabled (i.e. counting) then 'future_cycle' is non-zero,
      // which means there's a cycle break point set on TMR2 that needs to
      // be moved to a new cycle.

      // Get the current value of TMR2
      ///value = (cpu->cycles.value - last_cycle)/prescale;
      current_value();

      //cout << "cycles " << cpu->cycles.value.lo  << " old prescale " << prescale;

      prescale = t2con->get_pre_scale();

      //cout << " prescale " << prescale;

      // Now compute the 'last_cycle' as though if TMR2 had been running on the 
      // new prescale all along. Recall, 'last_cycle' records the value of the cpu's
      // cycle counter when TMR2 last rolled over.

      last_cycle = cpu->cycles.value - value * prescale;
      //cout << " effective last_cycle " << last_cycle << '\n';

      //cout << "tmr2's current value " << value << '\n';

      guint64 fc = cpu->cycles.value;

      if(pr2->value == value)
	fc += 0x100 * prescale;
      else
	fc +=  ((pr2->value - value) & 0xff) * prescale;

      //cout << "moving break from " << future_cycle << " to " << fc << '\n';

      cpu->cycles.reassign_break(future_cycle, fc, this);

      future_cycle = fc;
    }
  else
    {
      //cout << "TMR2 was off, but now it's on.\n";

      prescale = t2con->get_pre_scale();
      if(pr2->value == value)
	future_cycle = 0x100 * prescale;
      else
	future_cycle =  ((pr2->value - value) & 0xff) * prescale;

      last_cycle = cpu->cycles.value;
      future_cycle += cpu->cycles.value;
      cpu->cycles.set_break(future_cycle, this);
    }

  post_scale = t2con->get_post_scale();

}

void TMR2::new_pr2(void)
{

  if(t2con->get_tmr2on())
    {
      //update the tmr2 break point...
      ///      value = (cpu->cycles.value - last_cycle)/ prescale;
      current_value();

      // Get the current value of the prescale counter (because
      // writing to pr2 doesn't affect the pre/post scale counters).

      int curr_prescale = value * prescale - (cpu->cycles.value - last_cycle);

      guint64 fc = cpu->cycles.value + curr_prescale;

      if(pr2->value == value)
	{  // May wanta ignore the == case and instead allow the cycle break handle it...
	  fc += 0x100 * prescale;
	  last_cycle += 0x100 * prescale;
	}
      else
	fc +=  ((pr2->value - value) & 0xff) * prescale;

      cpu->cycles.reassign_break(future_cycle, fc, this);

      future_cycle = fc;
    }


}

void TMR2::current_value(void)
{
  value = ((cpu->cycles.value - last_cycle)/ prescale ); // & 0xff;

  if(value>0xff)
    cout << "TMR2 BUG!! value = " << value << " which is greater than 0xff\n";
}

// TMR2 callback is called when the cycle counter hits the break point that
// was set in TMR2::put. The cycle counter will clear the break point, so
// we don't need to worry about it. At this point, TMR2 is equal to PR2.

void TMR2::callback(void)
{

  //cout<<"TMR2 callback cycle: " << hex << cpu->cycles.value << '\n';

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
	  last_cycle = cpu->cycles.value;

	  if(pwm_mode & TMR2_PWM1_UPDATE)
	    ccp1con->pwm_match(1);

	  if(pwm_mode & TMR2_PWM2_UPDATE)
	    ccp2con->pwm_match(1);

	  if(--post_scale < 0)
	    {
	      //cout << "setting IF\n";
	      pir1->set_tmr2if();
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

  t2con = NULL;
  pr2   = NULL;
  tmr2  = NULL;

}

void TMR2_MODULE::initialize(T2CON *t2con_, PR2 *pr2_, TMR2  *tmr2_)
{

  t2con = t2con_;
  pr2   = pr2_;
  tmr2  = tmr2_;

}
