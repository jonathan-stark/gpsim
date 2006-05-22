/*
   Copyright (C) 1998 T. Scott Dattalo
   Copyright (C) 2006 Roy R Rankin

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
//#define DEBUG

#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

//--------------------------------------------------
// CCPRL
//--------------------------------------------------

CCPRL::CCPRL()
{

  ccprh = 0;
  tmrl = 0;

}

void CCPRL::put(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());

  value.put(new_value);

  if(tmrl && tmrl->compare_mode)
    start_compare_mode();   // Actually, re-start with new capture value.

}

void CCPRL::capture_tmr()
{

  tmrl->get_low_and_high();

  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());
  value.put(tmrl->value.get());

  trace.raw(ccprh->write_trace.get() | ccprh->value.get());
  //trace.register_write(ccprh->address,ccprh->value.get());
  ccprh->value.put(tmrl->tmrh->value.get());

  int c = value.get() + 256*ccprh->value.get();
  if(verbose & 4)
    cout << "CCPRL captured: " << c << '\n';
}

void CCPRL::start_compare_mode()
{

  tmrl->compare_mode = 1;

  int capture_value = value.get() + 256*ccprh->value.get();
  //cout << "start compare mode with capture value = " << capture_value << '\n';
  tmrl->compare_value = capture_value;
  tmrl->update();
}

void CCPRL::stop_compare_mode()
{
  // If tmr1 is in the compare mode, then change to non-compare and update
  // the tmr breakpoint.

  if(tmrl && tmrl->compare_mode)
    {
      tmrl->compare_mode = 0;
      tmrl->update();
    }
}

void CCPRL::start_pwm_mode()
{
  //cout << "CCPRL: starting pwm mode\n";

  ccprh->pwm_mode = 1;
}
void CCPRL::stop_pwm_mode()
{
  //cout << "CCPRL: stopping pwm mode\n"; 

  ccprh->pwm_mode = 0;
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

CCPRH::CCPRH()
{

  ccprl = 0;
  pwm_mode = 0;
  pwm_value = 0;
}

// put_value allows PWM code to put data
void CCPRH::put_value(unsigned int new_value)
{
      trace.raw(write_trace.get() | value.get());
      //trace.register_write(address,value.get());

      value.put(new_value);
}
void CCPRH::put(unsigned int new_value)
{

  //cout << "CCPRH put \n";

  if(pwm_mode == 0)   // In pwm_mode, CCPRH is a read-only register.
    {
      put_value(new_value);

      if(ccprl && ccprl->tmrl && ccprl->tmrl->compare_mode)
	ccprl->start_compare_mode();   // Actually, re-start with new capture value.

    }
}

unsigned int CCPRH::get()
{
  //cout << "CCPRH get\n";

  trace.raw(read_trace.get() | value.get());
  //trace.register_read(address, read_value);
  return value.get();
}

//--------------------------------------------------
// 
//--------------------------------------------------
class CCPSignalSource : public SignalControl
{
public:
  CCPSignalSource(CCPCON *_ccp)
    : m_ccp(_ccp)
  {
    assert(m_ccp);
  }
  char getState()
  {
    return m_ccp->getState();
  }
private:
  CCPCON *m_ccp;
};

//--------------------------------------------------
//
//--------------------------------------------------

class CCPSignalSink : public SignalSink
{
public:
  CCPSignalSink(CCPCON *_ccp)
    : m_ccp(_ccp)
  {
    assert(_ccp);
  }

  void setSinkState(char new3State)
  {
    m_ccp->new_edge( new3State=='1' || new3State=='W');
  }
private:
  CCPCON *m_ccp;
};

//--------------------------------------------------
// CCPCON
//--------------------------------------------------
CCPCON::CCPCON()
  : m_PinModule(0),
    m_source(0),
    m_bInputEnabled(false),
    m_bOutputEnabled(false),
    m_cOutputState('?'),
    edges(0),
    ccprl(0), pir_set(0), tmr2(0), adcon0(0)

{
}
void CCPCON::setIOpin(PinModule *new_PinModule)
{
  Dprintf(("CCPCON::setIOpin\n"));

  m_PinModule = new_PinModule;

  m_sink = new CCPSignalSink(this);
  m_PinModule->addSink(m_sink);

  m_source = new CCPSignalSource(this);

}
void CCPCON::setCrosslinks(CCPRL *_ccprl, PIR_SET *_pir_set, TMR2 *_tmr2)
{
  ccprl = _ccprl;
  pir_set = _pir_set;
  tmr2 = _tmr2;
}
void CCPCON::setADCON(ADCON0 *_adcon0)
{
  adcon0 = _adcon0;
}
char CCPCON::getState()
{
  return m_bOutputEnabled ?  m_cOutputState : '?';
}

void CCPCON::new_edge(unsigned int level)
{
  Dprintf(("CCPCON::new_edge() level=%d\n",level));

  switch(value.get() & (CCPM3 | CCPM2 | CCPM1 | CCPM0))
    {
    case ALL_OFF0:
    case ALL_OFF1:
    case ALL_OFF2:
    case ALL_OFF3:
      Dprintf(("--CCPCON not enabled\n"));
      return;

    case CAP_FALLING_EDGE:
      if (level == 0 && ccprl) {
	ccprl->capture_tmr();
	pir_set->set_ccpif();
	Dprintf(("--CCPCON caught falling edge\n"));
      }
      break;

    case CAP_RISING_EDGE:
      if (level && ccprl) {
	ccprl->capture_tmr();
	pir_set->set_ccpif();
	Dprintf(("--CCPCON caught rising edge\n"));
      }
      break;

    case CAP_RISING_EDGE4:
      if (level && --edges <= 0) {
	ccprl->capture_tmr();
	pir_set->set_ccpif();
	edges = 4;
	Dprintf(("--CCPCON caught 4th rising edge\n"));
      }
	//else cout << "Saw rising edge, but skipped\n";
      break;


    case CAP_RISING_EDGE16:
      if (level && --edges <= 0) {
	ccprl->capture_tmr();
	pir_set->set_ccpif();
	edges = 16;
	Dprintf(("--CCPCON caught 4th rising edge\n"));
      }
      //else cout << "Saw rising edge, but skipped\n";
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

void CCPCON::compare_match()
{

  Dprintf(("CCPCON::compare_match()\n"));

  switch(value.get() & (CCPM3 | CCPM2 | CCPM1 | CCPM0))
    {
    case ALL_OFF0:
    case ALL_OFF1:
    case ALL_OFF2:
    case ALL_OFF3:
      Dprintf(("-- CCPCON not enabled\n"));
      return;

    case CAP_FALLING_EDGE:
    case CAP_RISING_EDGE:
    case CAP_RISING_EDGE4:
    case CAP_RISING_EDGE16:
      Dprintf(("-- CCPCON is programmed for capture. bug?\n"));
      break;

    case COM_SET_OUT:
      m_cOutputState = '1';
      m_PinModule->updatePinModule();
      if (pir_set)
	pir_set->set_ccpif();
      Dprintf(("-- CCPCON setting compare output to 1\n"));
      break;

    case COM_CLEAR_OUT:
      m_cOutputState = '0';
      m_PinModule->updatePinModule();
      if (pir_set)
	pir_set->set_ccpif();
      Dprintf(("-- CCPCON setting compare output to 0\n"));
      break;

    case COM_INTERRUPT:
      if (pir_set)
	pir_set->set_ccpif();
      Dprintf(("-- CCPCON setting interrupt\n"));
      break;

    case COM_TRIGGER:
      if (ccprl)
	ccprl->tmrl->clear_timer();
      if (pir_set)
	pir_set->set_ccpif();
      if(adcon0)
	adcon0->start_conversion();

      Dprintf(("-- CCPCON triggering an A/D conversion\n"));
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
  Dprintf(("CCPCON::pwm_match()\n"));

  if( (value.get() & PWM0) == PWM0) {

    m_cOutputState = level ? '1' : '0';

    // if the level is 'high', then tmr2 == pr2 and the pwm cycle
    // is starting over. In which case, we need to update the duty
    // cycle by reading ccprl and the ccp X & Y and caching them
    // in ccprh's pwm slave register.

    if(level) {
      ccprl->ccprh->pwm_value = ((value.get()>>4) & 3) | 4*ccprl->value.get();
      tmr2->pwm_dc(ccprl->ccprh->pwm_value, address);
      ccprl->ccprh->put_value(ccprl->value.get());
      if (!ccprl->ccprh->pwm_value)  // if duty cycle == 0 output stays low
	m_cOutputState = '0';
    }
    m_PinModule->updatePinModule();

    //cout << "iopin should change\n";
  }  else
    cout << "not pwm mode. bug?\n";
}

void CCPCON::put(unsigned int new_value)
{

  Dprintf(("CCPCON::put() new_value=0x%x\n",new_value));
  trace.raw(write_trace.get() | value.get());

  value.put(new_value);
  if (!ccprl || !tmr2)
    return;

  bool oldbInEn  = m_bInputEnabled;
  bool oldbOutEn = m_bOutputEnabled;

  switch(value.get() & (CCPM3 | CCPM2 | CCPM1 | CCPM0))
    {
    case ALL_OFF0:
    case ALL_OFF1:
    case ALL_OFF2:
    case ALL_OFF3:
      if (ccprl)
      {
	ccprl->stop_compare_mode();
	ccprl->stop_pwm_mode();
      }
      if (tmr2)
	tmr2->stop_pwm(address);
      m_bInputEnabled = false;
      m_bOutputEnabled = false;
      break;
    case CAP_FALLING_EDGE:
    case CAP_RISING_EDGE:
      edges = 0;
      ccprl->stop_compare_mode();
      ccprl->stop_pwm_mode();
      tmr2->stop_pwm(address);
      m_bInputEnabled = true;
      m_bOutputEnabled = false;
      break;

    case CAP_RISING_EDGE4:
      edges &= 3;
      ccprl->stop_compare_mode();
      ccprl->stop_pwm_mode();
      tmr2->stop_pwm(address);
      m_bInputEnabled = true;
      m_bOutputEnabled = false;
      break;

    case CAP_RISING_EDGE16:
      ccprl->stop_compare_mode();
      ccprl->stop_pwm_mode();
      tmr2->stop_pwm(address);
      m_bInputEnabled = true;
      m_bOutputEnabled = false;
      break;

    case COM_SET_OUT:
    case COM_CLEAR_OUT:
    case COM_INTERRUPT:
    case COM_TRIGGER:
      ccprl->tmrl->ccpcon = this;
      ccprl->start_compare_mode();
      ccprl->stop_pwm_mode();
      tmr2->stop_pwm(address);

      if(adcon0)
	adcon0->start_conversion();

      m_bInputEnabled = true;
      m_bOutputEnabled = false;
      //if(adcon0) cout << "CCP triggering an A/D\n";

      break;
    case PWM0:
    case PWM1:
    case PWM2:
    case PWM3:
      ccprl->stop_compare_mode();
/* do this when TMR2 == PR2
      ccprl->start_pwm_mode();
      tmr2->pwm_dc( ccprl->ccprh->pwm_value, address);
*/
      m_bInputEnabled = false;
      m_bOutputEnabled = true;
      m_cOutputState = '0';
      break;

    }

  if (oldbOutEn != m_bOutputEnabled && m_PinModule) 
    m_PinModule->setSource(m_bOutputEnabled ? m_source : 0);

  if ((oldbInEn  != m_bInputEnabled  || 
       oldbOutEn != m_bOutputEnabled)
      && m_PinModule)
    m_PinModule->updatePinModule();

}

//--------------------------------------------------
// T1CON
//--------------------------------------------------
T1CON::T1CON()
  : tmrl(0)
{

  new_name("T1CON");

}

void T1CON::put(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());

  unsigned int diff = value.get() ^ new_value;
  value.put(new_value);
  if (!tmrl)
    return;
  // First, check the tmr1 clock source bit to see if we are  changing from
  // internal to external (or vice versa) clocks.
  if( diff & TMR1CS)
    tmrl->new_clock_source();

  if( diff & TMR1ON)
    tmrl->on_or_off(value.get() & TMR1ON);
  else  if( diff & (T1CKPS0 | T1CKPS1))
    tmrl->update();

}

unsigned int T1CON::get()
{
  trace.raw(read_trace.get() | value.get());
  //trace.register_read(address,value.get());
  return(value.get());
}

unsigned int T1CON::get_prescale()
{
  return( ((value.get() &(T1CKPS0 | T1CKPS1)) >> 4) + cpu_pic->pll_factor);
}



//--------------------------------------------------
// member functions for the TMRH base class
//--------------------------------------------------
TMRH::TMRH()
  : tmrl(0)
{

  value.put(0);
  new_name("TMRH");

}

void TMRH::put(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());
  value.put(new_value & 0xff);
  if(!tmrl)
    return;
  tmrl->synchronized_cycle = get_cycles().value;
  tmrl->last_cycle = tmrl->synchronized_cycle - (tmrl->value.get() + (value.get()<<8))*tmrl->prescale;

  if(tmrl->t1con->get_tmr1on())
    tmrl->update();

}

unsigned int TMRH::get()
{

  trace.raw(read_trace.get() | value.get());
  return get_value();
}

// For the gui and CLI
unsigned int TMRH::get_value()
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
TMRL::TMRL()
  : m_cState('?'), m_bExtClkEnabled(false), m_Interrupt(0)
{

  value.put(0);
  synchronized_cycle=0;
  prescale_counter=prescale=1;
  break_value = 0x10000;
  compare_value = 0;
  compare_mode = 0;
  last_cycle = 0;

  tmrh    = 0;
  t1con   = 0;
  ccpcon  = 0;
  new_name("TMRL");

}

void TMRL::setIOpin(PinModule *extClkSource)
{
  Dprintf(("TMRL::setIOpin\n"));

  if (extClkSource)
    extClkSource->addSink(this);
}

void TMRL::setSinkState(char new3State)
{
  if (new3State != m_cState) {
    m_cState = new3State;

    if (m_bExtClkEnabled && (m_cState == '1' || m_cState == 'W'))
      increment();
  }
}

//------------------------------------------------------------
// setInterruptSource()
//
// This Timer can be an interrupt source. When the interrupt
// needs to be generated, then the InterruptSource object will
// direct the interrupt to where it needs to go (usually this
// is the Peripheral Interrupt Register).

void TMRL::setInterruptSource(InterruptSource *_int)
{
  m_Interrupt = _int;
}

void TMRL::increment()
{
  Dprintf(("TMRL increment because of external clock\n"));

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
    if(value_16bit == 0 && m_Interrupt)
      m_Interrupt->Trigger();
  }

}

void TMRL::on_or_off(int new_state)
{

  if(new_state) {

    Dprintf(("TMR1 is being turned on\n"));

    // turn on the timer

    // Effective last cycle
    // Compute the "effective last cycle", i.e. the cycle
    // at which TMR1 was last 0 had it always been counting.

    last_cycle = get_cycles().value - value_16bit*prescale;
    update();
  }
  else {

    Dprintf(("TMR1 is being turned off\n"));

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

void TMRL::update()
{

  Dprintf(("TMR1 update 0x%llx\n",get_cycles().value));

  if(t1con->get_tmr1on()) {

    if(t1con->get_tmr1cs()) {
      cout << "TMRl::update - external clock is not fully implemented\n";
    } else {
	
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

  if (!tmrh || !t1con)
    return;

  synchronized_cycle = get_cycles().value;
  last_cycle = synchronized_cycle - ( value.get() + (tmrh->value.get()<<8)) * prescale;

  if(t1con->get_tmr1on())
    update();

}

unsigned int TMRL::get()
{
  trace.raw(read_trace.get() | value.get());
  return get_value();
}

// For the gui and CLI
unsigned int TMRL::get_value()
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
void TMRL::current_value()
{
  if(t1con->get_tmr1cs())
    value_16bit = tmrh->value.get() * 256 + value.get();
  else
    value_16bit = (unsigned int)((get_cycles().value - last_cycle)/ prescale) & 0xffff;
}

unsigned int TMRL::get_low_and_high()
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

void TMRL::new_clock_source()
{

  if(t1con->get_tmr1cs())
    m_bExtClkEnabled = true;
  else {
    m_bExtClkEnabled = false;
    put(value.get());    // let TMRL::put() set a cycle counter break point
  }
}

//
// clear_timer - This is called by either the CCP or PWM modules to 
// reset the timer to zero. This is rather easy since the current TMR
// value is always referenced to the cpu cycle counter. 
//

void TMRL::clear_timer()
{

  last_cycle = get_cycles().value;
  //cout << "TMR1 has been cleared\n";
}

// TMRL callback is called when the cycle counter hits the break point that
// was set in TMRL::put. The cycle counter will clear the break point, so
// we don't need to worry about it. At this point, TMRL is rolling over.

void TMRL::callback()
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
      if (m_Interrupt)
	m_Interrupt->Trigger();

      // Reset the timer to 0.

      synchronized_cycle = get_cycles().value;
      last_cycle = synchronized_cycle;

    }

  update();

}

//---------------------------

void TMRL::callback_print()
{
  cout << "TMRL " << name() << " CallBack ID " << CallBackID << '\n';

}


//--------------------------------------------------
// member functions for the PR2 base class
//--------------------------------------------------

PR2::PR2()
  :  tmr2(0)
{

  new_name("PR2");

}

void PR2::put(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());

  if(value.get() != new_value)
    {
      if (tmr2)
	tmr2->new_pr2(new_value);
      value.put(new_value);
    }
  else
    value.put(new_value);

}

//--------------------------------------------------
// member functions for the T2CON base class
//--------------------------------------------------

T2CON::T2CON()
  : tmr2(0)
{

  new_name("T2CON");

}

void T2CON::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());
  value.put(new_value);
  if (tmr2)
    tmr2->new_pre_post_scale();

}



//--------------------------------------------------
// member functions for the TMR2 base class
//--------------------------------------------------
TMR2::TMR2()
  : pr2(0), pir_set(0), t2con(0), ccp1con(0), ccp2con(0)
{
  update_state = TMR2_PWM1_UPDATE | TMR2_PWM2_UPDATE | TMR2_PR2_UPDATE;
  pwm_mode = 0;
  value.put(0);
  future_cycle = 0;
  prescale=1;
  new_name("TMR2");
}

void TMR2::callback_print() 
{
  cout << "TMR2 " << name() << " CallBack ID " << CallBackID << '\n';
}

void TMR2::start()
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

      // Update the cycle break if this is the first time to go into pwm mode
      if( (pwm_mode & TMR2_PWM1_UPDATE) == 0)
	{
	  pwm_mode |= TMR2_PWM1_UPDATE;
//wait for next TMR2 update	  update();	
	}
    }
  else if(ccp_address == ccp2con->address)
    {
      //cout << "TMR2: starting pwm mode with ccp2. duty cycle = " << hex << dc << '\n';

      duty_cycle2 = dc;

      // Update the cycle break if this is the first time to go into pwm mode
      if( (pwm_mode & TMR2_PWM2_UPDATE) == 0)
	{
	  pwm_mode |= TMR2_PWM2_UPDATE;

//wait for next TMR2 update	  update();	
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
      if(last_update & TMR2_PWM1_UPDATE)
         update_state &= (~TMR2_PWM1_UPDATE);

    }
  else if(ccp_address == ccp2con->address)
    {
      // cout << "TMR2:  stopping pwm mode with ccp2.\n";
      pwm_mode &= ~TMR2_PWM2_UPDATE;
      if(last_update & TMR2_PWM2_UPDATE)
         update_state &= (~TMR2_PWM2_UPDATE);
    }

  if((pwm_mode ^ old_pwm) && future_cycle && t2con->get_tmr2on())
    update(update_state);

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

  //cout << "TMR2 update. cpu cycle " << hex << cycles.value <<'\n';

  if(t2con->get_tmr2on()) {

    if(future_cycle) {

      // If TMR2 is enabled (i.e. counting) then 'future_cycle' is non-zero,
      // which means there's a cycle break point set on TMR2 that needs to
      // be moved to a new cycle.

      current_value();

      // Assume that we are not in pwm mode (and hence the next break will
      // be due to tmr2 matching pr2)

      break_value = 1 + pr2->value.get();
      guint64 fc = get_cycles().value + (break_value - value.get()) * prescale;

      last_update = TMR2_PR2_UPDATE;

      if (pwm_mode)
      {
          break_value <<= 2;    // now pwm value
          fc = last_cycle + break_value * prescale;
      }

      if(pwm_mode & ut & TMR2_PWM1_UPDATE) {

	// We are in pwm mode... So let's see what happens first: a pr2 compare
	// or a duty cycle compare. (recall, the duty cycle is really 10-bits)

	if( (duty_cycle1 > (value.get()*4) ) && (duty_cycle1 < break_value))
	  {
	    //cout << "TMR2:PWM1 update\n";
	    last_update = TMR2_PWM1_UPDATE;
            fc = last_cycle + duty_cycle1 * prescale;
	  }
      }

      if(pwm_mode & ut & TMR2_PWM2_UPDATE)
	{
	  // We are in pwm mode... So let's see what happens first: a pr2 compare
	  // or a duty cycle compare. (recall, the duty cycle is really 10-bits)

	  if( (duty_cycle2 > (value.get()*4) ) && (duty_cycle2 < break_value))
	  {
	      //cout << "TMR2:PWM2 update\n";
                                                                                
            if (last_update == TMR2_PWM1_UPDATE)
            {
                // set break for first duty cycle change
                if (duty_cycle2 < duty_cycle1)
                {
                    fc = last_cycle + duty_cycle2 * prescale;
                    last_update = TMR2_PWM2_UPDATE;
                }
                else if (duty_cycle2 == duty_cycle1)
                    last_update |= TMR2_PWM2_UPDATE;
            }
            else
            {
                last_update = TMR2_PWM2_UPDATE;
                fc = last_cycle + duty_cycle2 * prescale;
            }
                                                                                
	}
      }

      if(fc < future_cycle)
        cout << "TMR2: update note: new breakpoint=" << hex << fc <<
           " before old breakpoint " << future_cycle << endl;

      if (fc != future_cycle)
      {
          // cout << "TMR2: update new break at cycle "<<hex<<fc<<'\n';
          get_cycles().reassign_break(future_cycle, fc, this);
          future_cycle = fc;
      }

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
  current_value();

  unsigned int old_value = value.get();


  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());

  value.put(new_value & 0xff);

  if(future_cycle)
    {
      // If TMR2 is enabled (i.e. counting) then 'future_cycle' is non-zero,
      // which means there's a cycle break point set on TMR2 that needs to
      // be moved to a new cycle.

      guint64 current_cycle = get_cycles().value;
      unsigned int delta = (future_cycle - last_cycle);
      int shift = (new_value - old_value) * prescale;

      if (pwm_mode)
	shift <<= 2;
  
      // set cycle when tmr2 would have been zero

      last_cycle = current_cycle - shift;
      unsigned int now = (current_cycle - last_cycle);
      

      guint64 fc;

      /*
	Three possible cases
	   1> TMR2 is still before the next break point.
		Adjust the breakpoint to ocurr at correct TMR2 value
	   2> TMR2 is now greater the PR2
		Assume TMR2 must count up to 0xff, roll-over and then
		we are back in business. High CCP outputs stay high.
	   3> TMR2 is now less than PR2 but greater than a CCP duty cycle point.
		The CCP output stays as the duty cycle comparator does not 
		match on this cycle.
      */

      if (now < delta) // easy case, just shift break.
      {
          fc = last_cycle + delta;
          get_cycles().reassign_break(future_cycle, fc, this);
          future_cycle = fc;
      }
      else if (now >= break_value * prescale)  // TMR2 now greater than PR2
      {
        // set break to when TMR2 will overflow
	last_update |= TMR2_WRAP;
	if (pwm_mode)
	    fc = last_cycle + (0x100 * prescale << 2);
	else
	    fc = last_cycle + 0x100 * prescale;
        get_cycles().reassign_break(future_cycle, fc, this);
        future_cycle = fc;
      }
      else	// new break < PR2 but > duty cycle break
      {
          update(update_state);
      }



     /* 
	'clear' the post scale counter. (I've actually implemented the 
	post scale counter as a count-down counter, so 'clearing it' 
	means resetting it to the starting point.
     */
      if (t2con)
	post_scale = t2con->get_post_scale();
    }
}

unsigned int TMR2::get()
{

  if(t2con->get_tmr2on())
    {
      current_value();
    }

  trace.raw(read_trace.get() | value.get());
  // trace.register_read(address, value.get());
  return(value.get());
  
}

unsigned int TMR2::get_value()
{

  if(t2con->get_tmr2on())
    {
      current_value();
    }

  return(value.get());
  
}
void TMR2::new_pre_post_scale()
{

  //cout << "T2CON was written to, so update TMR2 " << t2con->get_tmr2on() << "\n";

  if(!t2con->get_tmr2on()) {
    // TMR2 is not on. If has just been turned off, clear the callback breakpoint.

    if(future_cycle) {
      get_cycles().clear_break(this);
      future_cycle = 0;
    }
    return;
  }

  unsigned int old_prescale = prescale;
  prescale = t2con->get_pre_scale();
  post_scale = t2con->get_post_scale();

  if(future_cycle)
    {
      // If TMR2 is enabled (i.e. counting) then 'future_cycle' is non-zero,
      // which means there's a cycle break point set on TMR2 that needs to
      // be moved to a new cycle.

      // Get the current value of TMR2
      ///value = (cycles.value - last_cycle)/prescale;

      current_value();

      //cout << "cycles " << cycles.value.lo  << " old prescale " << prescale;


      //cout << " prescale " << prescale;


      if (prescale != old_prescale)	// prescaler value change
      {
	// togo is number of cycles to next callback based on new prescaler.
	guint64 togo = (future_cycle - get_cycles().value) * prescale / old_prescale;

	if (!togo)	// I am not sure this can happen RRR
	    callback();
	else
	{
	   guint64 fc = togo + get_cycles().value;

          get_cycles().reassign_break(future_cycle, fc, this);
          future_cycle = fc;
	}
      }
    }
  else
    {
      //cout << "TMR2 was off, but now it's on.\n";

      if (value.get() == pr2->value.get()) // TMR2 == PR2
      {
        future_cycle = get_cycles().value;
        get_cycles().set_break(future_cycle, this);
	callback();
      }
      else if (value.get() > pr2->value.get()) // TMR2 > PR2
      {
	cout << "Warning TMR2 turned on with TMR2 greater than PR2\n";
	// this will cause TMR2 to wrap
        future_cycle  = get_cycles().value + 
		(1 + pr2->value.get() + (0x100 -  value.get())) * prescale;
        get_cycles().set_break(future_cycle, this);
      }
      else
      {
          future_cycle = get_cycles().value + 1;
          get_cycles().set_break(future_cycle, this);
          update(update_state);
      }
  }

}

void TMR2::new_pr2(unsigned int new_value)
{

  if(t2con->get_tmr2on())
    {

      unsigned int cur_break = (future_cycle - last_cycle)/prescale;
      unsigned int new_break = (pwm_mode)? (1 + new_value) << 2 : 1 + new_value;
      unsigned int now_cycle = (get_cycles().value - last_cycle) / prescale;

      guint64 fc = last_cycle;

      /*
	PR2 change casses

	1> TMR2 greater than new PR2
		TMR2 wraps through 0xff
	2> New PR2 breakpoint less than current breakpoint or
	   current break point due to PR2
		Change breakpoint to new value based on PR2
	3> Other breakpoint < PR2 breakpoint
		No need to do anything.
     */


      if (now_cycle > new_break)	// TMR2 > PR2 do wrap
      {
        // set break to when TMR2 will overflow
	last_update |= TMR2_WRAP;
	if (pwm_mode)
	    fc += (0x100 * prescale << 2);
	else
	    fc += 0x100 * prescale;
        get_cycles().reassign_break(future_cycle, fc, this);
        future_cycle = fc;
      }
      else if (cur_break == break_value ||	// breakpoint due to pr2
	       new_break < cur_break)		// new break less than current
      {
	fc += new_break * prescale;
        get_cycles().reassign_break(future_cycle, fc, this);
        future_cycle = fc;
      }
    }
}

void TMR2::current_value()
{
  unsigned int tmr2_val = (get_cycles().value - last_cycle)/ prescale;

  if (pwm_mode)
       tmr2_val >>= 2;

  value.put(tmr2_val & 0xff);

  if(tmr2_val > 0x100)	// Can get to 0x100 during transition
   cout << "TMR2 BUG!! value = " << value.get() << " which is greater than 0xff\n";
}

// TMR2 callback is called when the cycle counter hits the break point that
// was set in TMR2::put. The cycle counter will clear the break point, so
// we don't need to worry about it. At this point, TMR2 is equal to PR2.

void TMR2::callback()
{

  //cout<<"TMR2 callback cycle: " << hex << cycles.value << '\n';

  // If tmr2 is still enabled, then set up for the next break. 
  // If tmr2 was disabled then ignore this break point.
  if(t2con->get_tmr2on())
    {

      // What caused the callback: PR2 match or duty cyle match ?

      if (last_update & TMR2_WRAP) // TMR2 > PR2 
      {
	last_update &= ~TMR2_WRAP;
	// This (implicitly) resets the timer to zero:
	last_cycle = get_cycles().value;
      }
      else if( last_update & (TMR2_PWM1_UPDATE | TMR2_PWM2_UPDATE))
      {

        if(last_update & TMR2_PWM1_UPDATE)
	{
	  // duty cycle match
	  //cout << "TMR2: duty cycle match for pwm1 \n";
	  update_state &= (~TMR2_PWM1_UPDATE);
	  ccp1con->pwm_match(0);
	}
        if(last_update & TMR2_PWM2_UPDATE)
	{
	  // duty cycle match
	  //cout << "TMR2: duty cycle match for pwm2 \n";
	  update_state &= (~TMR2_PWM2_UPDATE);
	  ccp2con->pwm_match(0);
	}
      }
      else
	{
	  // matches PR2

	  //cout << "TMR2: PR2 match. pwm_mode is " << pwm_mode <<'\n';

	  // This (implicitly) resets the timer to zero:
	  last_cycle = get_cycles().value;

          if ((ccp1con->value.get() & CCPCON::PWM0) == CCPCON::PWM0)
               ccp1con->pwm_match(1);

          if ((ccp2con->value.get() & CCPCON::PWM0) == CCPCON::PWM0)
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

TMR2_MODULE::TMR2_MODULE()
{

  t2con = 0;
  pr2   = 0;
  tmr2  = 0;
  cpu   = 0;
  name_str = 0;

}

void TMR2_MODULE::initialize(T2CON *t2con_, PR2 *pr2_, TMR2  *tmr2_)
{

  t2con = t2con_;
  pr2   = pr2_;
  tmr2  = tmr2_;

}
