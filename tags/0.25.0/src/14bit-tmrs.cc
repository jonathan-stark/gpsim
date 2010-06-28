/*
   Copyright (C) 1998 T. Scott Dattalo
   Copyright (C) 2006,2009,2010 Roy R Rankin

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
#include <string>

#include "../config.h"
#include "14bit-tmrs.h"
#include "stimuli.h"
#include "a2dconverter.h"

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

CCPRL::CCPRL(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc),
    ccprh(0), ccpcon(0), tmrl(0)
{
}

bool CCPRL::test_compare_mode()
{
    return tmrl && ccpcon && ccpcon->test_compare_mode();
}

void CCPRL::put(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  value.put(new_value);

  if(test_compare_mode())
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

void CCPRL::start_compare_mode(CCPCON *ref)
{
  int capture_value = value.get() + 256*ccprh->value.get();
  if(verbose & 4)
    cout << "start compare mode with capture value = " << capture_value << '\n';

  if ( ref )
  {
    ccpcon = ref;
  }
  if ( ccpcon )
    tmrl->set_compare_event ( capture_value, ccpcon );
  else
    cout << "CPRL: Attempting to set a compare callback with no CCPCON\n";
}

void CCPRL::stop_compare_mode()
{
  // If this CCP is in the compare mode, then change to non-compare and cancel
  // the tmr breakpoint.

  if(test_compare_mode())
  {
    tmrl->clear_compare_event ( ccpcon );
  }
  ccpcon = 0;
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

CCPRH::CCPRH(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc),
    ccprl(0),pwm_mode(0),pwm_value(0)
{
}

// put_value allows PWM code to put data
void CCPRH::put_value(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  value.put(new_value);
}

void CCPRH::put(unsigned int new_value)
{

  //cout << "CCPRH put \n";

  if(pwm_mode == 0)   // In pwm_mode, CCPRH is a read-only register.
    {
      put_value(new_value);

      if(ccprl && ccprl->test_compare_mode())
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
    : m_ccp(_ccp),
    state('?')
  {
    assert(m_ccp);
  }
  ~CCPSignalSource()
  {
  }

  void setState(char m_state) { state = m_state; }
  virtual char getState() { return state; }
  virtual void release() 
  {
    delete this;
  }
private:
  CCPCON *m_ccp;
  char state;
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

  void release() 
  {
    delete this;
  }
  void setSinkState(char new3State)
  {
    m_ccp->new_edge( new3State=='1' || new3State=='W');
  }
private:
  CCPCON *m_ccp;
};

class Tristate : public SignalControl
{
public:
  Tristate() { }
  ~Tristate() { }
  char getState() { return '1'; }	// set port to high impedance by setting it to input
  void release() { delete this; }
};
//--------------------------------------------------
// CCPCON
//--------------------------------------------------
CCPCON::CCPCON(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc),
    pstrcon(0),
    pwm1con(0),
    eccpas(0),
    bit_mask(0x3f),
    m_sink(0),
    m_tristate(0),
    m_bInputEnabled(false),
    m_bOutputEnabled(false),
    m_cOutputState('?'),
    edges(0), bridge_shutdown(false),
    ccprl(0), pir(0), tmr2(0), adcon0(0)
{
	m_PinModule[0] = 0;
}

CCPCON::~CCPCON() 
{ 
}

// EPWM has four outputs PWM 1
void CCPCON::setIOpin(PinModule *p1, PinModule *p2, PinModule *p3, PinModule *p4)
{
  Dprintf(("%s::setIOpin %s\n", name().c_str(), (p1 && &(p1->getPin())) ? p1->getPin().name().c_str():"unknown"));
  if (!&(p1->getPin()))
  {
	Dprintf(("FIXME %s::setIOpin called where p1 has unassigned pin\n", name().c_str()));
  }

  if (p1 && &(p1->getPin()))
  {
    if (m_PinModule[0])
    {
	if (m_PinModule[0] != p1)
	fprintf(stderr, "FIXME %s::setIOpin called for port %s then %s %p %p\n", 
		name().c_str(), 
		m_PinModule[0]->getPin().name().c_str(), 
		p1->getPin().name().c_str() , m_PinModule[0], p1
	);
    }
    else
    {
        m_PinModule[0] = p1;
        m_sink = new CCPSignalSink(this);
	m_tristate = new Tristate();
        m_source[0] = new CCPSignalSource(this);
        p1->addSink(m_sink);
    }
  }
  m_PinModule[1] = m_PinModule[2] = m_PinModule[3] = 0;
  m_source[1] = m_source[2] = m_source[3] = 0;
  if (p2)
  {
    m_PinModule[1] = p2;
    m_source[1] = new CCPSignalSource(this);
  }
  if (p3)
  {
    m_PinModule[2] = p3;
    m_source[2] = new CCPSignalSource(this);
  }
  if (p4)
  {
    m_PinModule[3] = p4;
    m_source[3] = new CCPSignalSource(this);
  }

}

void CCPCON::setCrosslinks(CCPRL *_ccprl, PIR *_pir, TMR2 *_tmr2, ECCPAS *_eccpas)
{
  ccprl = _ccprl;
  pir = _pir;
  tmr2 = _tmr2;
  eccpas = _eccpas;
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
  Dprintf(("%s::new_edge() level=%d\n",name().c_str(), level));

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
	pir->set_ccpif();
	Dprintf(("--CCPCON caught falling edge\n"));
      }
      break;

    case CAP_RISING_EDGE:
      if (level && ccprl) {
	ccprl->capture_tmr();
	pir->set_ccpif();
	Dprintf(("--CCPCON caught rising edge\n"));
      }
      break;

    case CAP_RISING_EDGE4:
      if (level && --edges <= 0) {
	ccprl->capture_tmr();
	pir->set_ccpif();
	edges = 4;
	Dprintf(("--CCPCON caught 4th rising edge\n"));
      }
	//else cout << "Saw rising edge, but skipped\n";
      break;


    case CAP_RISING_EDGE16:
      if (level && --edges <= 0) {
	ccprl->capture_tmr();
	pir->set_ccpif();
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

  Dprintf(("%s::compare_match()\n", name().c_str()));

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
      m_source[0]->setState('1');
      m_PinModule[0]->updatePinModule();
      if (pir)
	pir->set_ccpif();
      Dprintf(("-- CCPCON setting compare output to 1\n"));
      break;

    case COM_CLEAR_OUT:
      m_cOutputState = '0';
      m_source[0]->setState('0');
      m_PinModule[0]->updatePinModule();
      if (pir)
	pir->set_ccpif();
      Dprintf(("-- CCPCON setting compare output to 0\n"));
      break;

    case COM_INTERRUPT:
      if (pir)
	pir->set_ccpif();
      Dprintf(("-- CCPCON setting interrupt\n"));
      break;

    case COM_TRIGGER:
      if (ccprl)
	ccprl->tmrl->clear_timer();
      if (pir)
	pir->set_ccpif();
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

// handle dead-band delay in half-bridge mode
void CCPCON::callback()
{

    if(delay_source0)
    {
       	m_source[0]->setState('1');
    	m_PinModule[0]->updatePinModule();
        delay_source0 = false;
    }
    if(delay_source1)
    {
       	m_source[1]->setState('1');
    	m_PinModule[1]->updatePinModule();
        delay_source1 = false;
    }
}
void CCPCON::pwm_match(int level)
{
  unsigned int new_value = value.get();
  Dprintf(("%s::pwm_match() level=%d\n", name().c_str(), level));


  // if the level is 1, then tmr2 == pr2 and the pwm cycle
  // is starting over. In which case, we need to update the duty
  // cycle by reading ccprl and the ccp X & Y and caching them
  // in ccprh's pwm slave register.
  if(level == 1) {
      // Auto shutdown comes off at start of PWM if ECCPASE clear
      if (bridge_shutdown && (!eccpas || !(eccpas->get_value() & ECCPAS::ECCPASE)))
      {
          Dprintf(("bridge_shutdown=%d eccpas=%p ECCPAS=%x\n", bridge_shutdown, eccpas, 
		eccpas ? eccpas->get_value() & ECCPAS::ECCPASE: 0));
	  for(int i = 0; i < 4; i++)
	  {
            if(m_PinModule[i])
	    {
		 m_PinModule[i]->setControl(0); //restore defailt pin direction
        	 m_PinModule[i]->updatePinModule();
	    }
	  }
	  bridge_shutdown = false;
      }

      ccprl->ccprh->pwm_value = ((value.get()>>4) & 3) | 4*ccprl->value.get();
      tmr2->pwm_dc(ccprl->ccprh->pwm_value, address);
      ccprl->ccprh->put_value(ccprl->value.get());
  }
  if( !pwm1con) { // simple PWM

    if (bridge_shutdown == false) // some processors use shutdown and simple PWM
    {
        m_cOutputState = level ? '1' : '0';
        m_source[0]->setState(level ? '1' : '0');
        m_PinModule[0]->setSource(m_source[0]);
    
    
        if(level && !ccprl->ccprh->pwm_value)  // if duty cycle == 0 output stays low
              m_source[0]->setState('0');

        m_PinModule[0]->updatePinModule();
    
        //cout << "iopin should change\n";
    }
  }  
  else	// EPWM
  {


      if (!bridge_shutdown)
      	drive_bridge(level, new_value);
  }
}
//
//  Drive PWM bridge
//
void CCPCON::drive_bridge(int level, int new_value)
{
      unsigned int pstrcon_value;
      // pstrcon allows port steering for "single output"
      // if it is not defined, just use the first port
      if (pstrcon)
	  pstrcon_value = pstrcon->value.get();
      else
	  pstrcon_value = 1;
      int pwm_width;
      int delay = pwm1con->value.get() & ~PWM1CON::PRSEN;

      bool active_high[4];
      switch (new_value & (CCPM3|CCPM2|CCPM1|CCPM0))
      {
        case PWM0:	//P1A, P1C, P1B, P1D active high
	active_high[0] = true;
	active_high[1] = true;
	active_high[2] = true;
	active_high[3] = true;
    	break;
    
        case PWM1:	// P1A P1C active high P1B P1D active low
	active_high[0] = true;
	active_high[1] = false;
	active_high[2] = true;
	active_high[3] = false;
    	break;
    
        case PWM2: 	// P1A P1C active low P1B P1D active high 
	active_high[0] = false;
	active_high[1] = true;
	active_high[2] = false;
	active_high[3] = true;
    	break;
    
        case PWM3:	// //P1A, P1C, P1B, P1D active low
	active_high[0] = false;
	active_high[1] = false;
	active_high[2] = false;
	active_high[3] = false;
    	break;
    
        default:
            cout << "not pwm mode. bug?\n";
    	return;
    	break;
      }
      switch((new_value & (P1M1|P1M0))>>6) // ECCP bridge mode
      {
	    case 0:	// Single
		Dprintf(("Single bridge pstrcon=0x%x\n", pstrcon_value));
		for (int i = 0; i <4; i++)
		{
		    if (pstrcon_value & (1<<i))
		    {
			m_PinModule[i]->setSource(m_source[i]);
			// follow level except where duty cycle = 0
			if (level && ccprl->ccprh->pwm_value)
          		    m_source[i]->setState(active_high[i]?'1':'0');
			else
          		    m_source[i]->setState(active_high[i]?'0':'1');
    			m_PinModule[i]->updatePinModule();
		    }
		    else if (m_PinModule[i])
			m_PinModule[i]->setSource(0);
		}
		break;
	
	    case 2:	// Half-Bridge
		Dprintf(("half-bridge\n"));
		m_PinModule[0]->setSource(m_source[0]);
		m_PinModule[1]->setSource(m_source[1]);
		m_PinModule[2]->setSource(0);
		m_PinModule[3]->setSource(0);
		delay_source0 = false;
		delay_source1 = false;
		// FIXME need to add deadband
		// follow level except where duty cycle = 0
		pwm_width = level ? 
			ccprl->ccprh->pwm_value : 
			((tmr2->pr2->value.get()+1)*4)-ccprl->ccprh->pwm_value;
		if (!(level^active_high[0]) && ccprl->ccprh->pwm_value)
		{
		    // No delay, change state
		    if (delay == 0)
       		    	m_source[0]->setState('1');
		    else if (delay < pwm_width) // there is a delay
		    {
			future_cycle = get_cycles().get() + delay;
          		get_cycles().set_break(future_cycle, this);
			delay_source0 = true;
		    }
		}
		else
		{
       		    m_source[0]->setState('0');
		}
		if (!(level^active_high[1]) && ccprl->ccprh->pwm_value)
		{
       		    m_source[1]->setState('0');
		}
		else
		{
		    // No delay, change state
		    if (delay == 0)
       		    	m_source[1]->setState('1');
		    else if (delay < pwm_width) // there is a delay
		    {
			future_cycle = get_cycles().get() + delay;
          		get_cycles().set_break(future_cycle, this);
			delay_source1 = true;
		    }
		}
    		m_PinModule[0]->updatePinModule();
    		m_PinModule[1]->updatePinModule();
		break;
	
	    case 1:	// Full bidge Forward
		Dprintf(("full-bridge, forward\n"));
		m_PinModule[0]->setSource(m_source[0]);
		m_PinModule[1]->setSource(m_source[1]);
		m_PinModule[2]->setSource(m_source[2]);
		m_PinModule[3]->setSource(m_source[3]);
		// P1D toggles
		if (level && ccprl->ccprh->pwm_value)
          	    m_source[3]->setState(active_high[3]?'1':'0');
		else
          	    m_source[3]->setState(active_high[3]?'0':'1');
		// P1A High (if active high)
          	m_source[0]->setState(active_high[0]?'1':'0');
		// P1B, P1C low (if active high)
          	m_source[1]->setState(active_high[1]?'0':'1');
          	m_source[2]->setState(active_high[2]?'0':'1');
    		m_PinModule[0]->updatePinModule();
    		m_PinModule[1]->updatePinModule();
    		m_PinModule[2]->updatePinModule();
    		m_PinModule[3]->updatePinModule();
		break;
	
	    case 3:	// Full bridge reverse
		Dprintf(("full-bridge reverse\n"));
		m_PinModule[0]->setSource(m_source[0]);
		m_PinModule[1]->setSource(m_source[1]);
		m_PinModule[2]->setSource(m_source[2]);
		m_PinModule[3]->setSource(m_source[3]);
		// P1B toggles
		if (level && ccprl->ccprh->pwm_value)
          	    m_source[1]->setState(active_high[1]?'1':'0');
		else
          	    m_source[1]->setState(active_high[1]?'0':'1');
		// P1C High (if active high)
          	m_source[2]->setState(active_high[2]?'1':'0');
		// P1A, P1D low (if active high)
          	m_source[0]->setState(active_high[0]?'0':'1');
          	m_source[3]->setState(active_high[3]?'0':'1');
    		m_PinModule[0]->updatePinModule();
    		m_PinModule[1]->updatePinModule();
    		m_PinModule[2]->updatePinModule();
    		m_PinModule[3]->updatePinModule();
		break;

	    default:
		printf("%s::pwm_match impossible ECCP bridge mode\n", name().c_str());
		break;
       }
	
}
//
// Set PWM bridge into shutdown mode
//
void CCPCON::shutdown_bridge(int eccpas)
{
    bridge_shutdown = true;

    Dprintf(("eccpas=0x%x\n", eccpas));

    switch(eccpas & (ECCPAS::PSSBD0 | ECCPAS::PSSBD1))
    {
    case 0:	// B D output 0
	if (m_source[1]) m_source[1]->setState('0');
	if (m_source[3]) m_source[3]->setState('0');
	break;

    case 1:	// B, D output 1
	if (m_source[1]) m_source[1]->setState('1');
	if (m_source[3]) m_source[3]->setState('1');
	break;

    default:	// Tristate B & D
        if(m_PinModule[1])  m_PinModule[1]->setControl(m_tristate);
        if(m_PinModule[3])  m_PinModule[3]->setControl(m_tristate);
	break;
    }
    switch(eccpas & ((ECCPAS::PSSAC0 | ECCPAS::PSSAC1) >> 2))
    {
    case 0:	// A, C output 0
	m_source[0]->setState('0');
	if (m_source[2]) m_source[2]->setState('0');
	break;

    case 1:	// A, C output 1
	m_source[0]->setState('1');
	if (m_source[2]) m_source[2]->setState('1');
	break;

    default:	// Tristate A & C
        m_PinModule[0]->setControl(m_tristate);
        if(m_PinModule[2])  m_PinModule[2]->setControl(m_tristate);
	break;
    }
    m_PinModule[0]->updatePinModule();
    if (m_PinModule[1]) m_PinModule[1]->updatePinModule();
    if (m_PinModule[2]) m_PinModule[2]->updatePinModule();
    if (m_PinModule[3]) m_PinModule[3]->updatePinModule();
}

void CCPCON::put(unsigned int new_value)
{

  unsigned int old_value =  value.get();
  new_value &= bit_mask;
  
  Dprintf(("%s::put() new_value=0x%x\n",name().c_str(), new_value));
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

      // RP - According to 16F87x data sheet section 8.2.1 clearing CCP1CON also clears the latch
      m_cOutputState = '0';
      m_source[0]->setState('0');
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
      m_bOutputEnabled = true;
    case COM_INTERRUPT:
    case COM_TRIGGER:
      ccprl->start_compare_mode(this);
      ccprl->stop_pwm_mode();
      tmr2->stop_pwm(address);

      // RP - just writing CCP2CON doesn't trigger the ADC; that only happens on a match
      //if(adcon0)
      //  adcon0->start_conversion();

      m_bInputEnabled = false;
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
      m_bOutputEnabled = false;	// this is done in pwm_match
      m_cOutputState = '0';
      if ((old_value & P1M0) && (new_value & P1M0)) // old and new full-bridge
      {		// need to adjust timer if P1M1 also changed
	Dprintf(("full bridge repeat old=0x%x new=%x\n", old_value, new_value));
      }
      else
          pwm_match(2);
      return;
      break;

    }

    if (oldbOutEn != m_bOutputEnabled && m_PinModule)
    {
	if (m_bOutputEnabled)
            m_PinModule[0]->setSource(m_source[0]);
	else
	{
            m_PinModule[0]->setSource(0);
	    m_source[0]->setState('?');
	}
    }

    if ((oldbInEn  != m_bInputEnabled  ||
       oldbOutEn != m_bOutputEnabled)
      && m_PinModule[0])
        m_PinModule[0]->updatePinModule();

}

bool CCPCON::test_compare_mode()
{
  switch(value.get() & (CCPM3 | CCPM2 | CCPM1 | CCPM0))
  {
    case ALL_OFF0:
    case ALL_OFF1:
    case ALL_OFF2:
    case ALL_OFF3:
    case CAP_FALLING_EDGE:
    case CAP_RISING_EDGE:
    case CAP_RISING_EDGE4:
    case CAP_RISING_EDGE16:
    case PWM0:
    case PWM1:
    case PWM2:
    case PWM3:
      return false;  
      break;

    case COM_SET_OUT:
    case COM_CLEAR_OUT:
    case COM_INTERRUPT:
    case COM_TRIGGER:
      return true;  
      break;
  }
  return false;
}



// Attribute for frequency of external Timer1 oscillator
class TMR1_Freq_Attribute : public Float
{
public:
  TMR1_Freq_Attribute(Processor * _cpu, double freq); 

  virtual void set(double d);
  double get_freq();

private:
  Processor * cpu;
};

TMR1_Freq_Attribute::TMR1_Freq_Attribute(Processor * _cpu, double freq)
  : Float("tmr1_freq",freq, " Tmr1 oscillator frequency."),
    cpu(_cpu)
{
}

double TMR1_Freq_Attribute::get_freq()
{
	double d;
	Float::get(d);
	return(d);
}
void TMR1_Freq_Attribute::set(double d)
{
    Float::set ( d );
}

//--------------------------------------------------
// T1CON
//--------------------------------------------------
T1CON::T1CON(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc),
    tmrl(0)
{

  pCpu->addSymbol(freq_attribute = new TMR1_Freq_Attribute(pCpu, 32768.));
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
  if( diff & (TMR1CS | T1OSCEN))
    tmrl->new_clock_source();

  if( diff & TMR1ON)
    tmrl->on_or_off(value.get() & TMR1ON);
  else  if( diff & (T1CKPS0 | T1CKPS1 | TMR1GE | T1GINV))
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
  return( ((value.get() &(T1CKPS0 | T1CKPS1)) >> 4) );
}



//--------------------------------------------------
// member functions for the TMRH base class
//--------------------------------------------------
TMRH::TMRH(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc),
    tmrl(0)
{

  value.put(0);
  new_name("TMRH");

}

void TMRH::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());
  if(!tmrl)
  {
    value.put(new_value & 0xff);
    return;
  }

  tmrl->set_ext_scale();
  value.put(new_value & 0xff);
  tmrl->synchronized_cycle = get_cycles().get();
  tmrl->last_cycle = tmrl->synchronized_cycle 
	- (gint64)((tmrl->value.get() + (value.get()<<8) 
	* tmrl->prescale * tmrl->ext_scale) +0.5);


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

  if(get_cycles().get() <= tmrl->synchronized_cycle)
    return value.get();

  // If the TMR is not running then return.
  if(!tmrl->t1con->get_tmr1on())
    return value.get();

  tmrl->current_value();

  return(value.get());
  
}


//--------------------------------------------------
//
//--------------------------------------------------

class TMRl_GateSignalSink : public SignalSink
{
public:
  TMRl_GateSignalSink(TMRL *_tmr1l)
    : m_tmr1l(_tmr1l)
  {
    assert(_tmr1l);
  }

  void release() 
  {
    delete this;
  }
  void setSinkState(char new3State)
  {
    m_tmr1l->IO_gate( new3State=='1' || new3State=='W');
  }
private:
  TMRL *m_tmr1l;
};

//--------------------------------------------------
// trivial class to represent a compare event reference
//--------------------------------------------------

class TMR1CapComRef
{
public:
    TMR1CapComRef * next;

    CCPCON * ccpcon;
    unsigned int value;

    TMR1CapComRef ( CCPCON * c, unsigned int v ) : ccpcon(c), value(v) {};
};

//--------------------------------------------------
// member functions for the TMRL base class
//--------------------------------------------------
TMRL::TMRL(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc),
    m_cState('?'), m_GateState(false), m_bExtClkEnabled(false), 
    m_sleeping(false), m_t1gss(true), m_Interrupt(0)
{

  value.put(0);
  synchronized_cycle=0;
  prescale_counter=prescale=1;
  break_value = 0x10000;
  last_cycle = 0;
  future_cycle = 0;

  ext_scale = 1.;
  tmrh    = 0;
  t1con   = 0;
  compare_queue = 0;
}

TMRL::~TMRL()
{
  if (m_Interrupt)
    m_Interrupt->release();
}
/*
 * If we are similating an external RTC crystal for timer1,
 * compute scale factor between crsytal speed and processor
 * instruction cycle rate
 */ 
void TMRL::set_ext_scale() 
{ 
    current_value();
    if (t1con->get_t1oscen() && t1con->get_tmr1cs())
    {
    	ext_scale = get_cycles().instruction_cps()/
	            t1con->freq_attribute->get_freq();
    }
    else
	ext_scale = 1.;

    if (future_cycle)
    {
      last_cycle = get_cycles().get()
			- (gint64)(value_16bit *( prescale * ext_scale) + 0.5);
    }
}

void TMRL::release()
{

}

void TMRL::setIOpin(PinModule *extClkSource)
{
  Dprintf(("%s::setIOpin %s\n", name().c_str(), extClkSource?extClkSource->getPin().name().c_str():""));
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

void TMRL::set_compare_event ( unsigned int value, CCPCON *host )
{
  TMR1CapComRef * event = compare_queue;

  if ( host )
  {
    while ( event )
    {
      if ( event->ccpcon == host )
      {
        event->value = value;
        update();
        return;
      }
      event = event->next;
    }
    event = new TMR1CapComRef ( host, value );
    event->next = compare_queue;
    compare_queue = event;
    update();
  }
  else
      cout << "TMRL::set_compare_event called with no CAPCOM\n";
}

void TMRL::clear_compare_event ( CCPCON *host )
{
  TMR1CapComRef * event = compare_queue;
  TMR1CapComRef * * eptr = &compare_queue;

  while ( event )
  {
    if ( event->ccpcon == host )
    {
      *eptr = event->next;
      delete event;
      update();
      return;
    }
    eptr = &event->next;
    event = event->next;
  }
}

void TMRL::setGatepin(PinModule *extGateSource)
{
  Dprintf(("TMRL::setGatepin\n"));

  if (extGateSource)
    extGateSource->addSink(new TMRl_GateSignalSink(this));
}

void TMRL::set_T1GSS(bool arg)
{

    m_t1gss = arg;
    if (m_t1gss)
	IO_gate(m_io_GateState);
    else
	compare_gate(m_compare_GateState);
}
void TMRL::compare_gate(bool state)
{
  m_compare_GateState = state;
  if (!m_t1gss && m_GateState != state)
  {
    m_GateState = state;
    
    Dprintf(("TMRL::compare_gate state %d \n", state));

    if (t1con->get_tmr1GE())
    {
	update();
    }
  }
}
void TMRL::IO_gate(bool state)
{
  m_io_GateState = state;
  if (m_t1gss && m_GateState != state)
  {
    m_GateState = state;
    
    Dprintf(("TMRL::IO_gate state %d \n", state));

    if (t1con->get_tmr1GE())
    {
	update();
    }
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

  // In synchronous counter mode prescaler works but rest of tmr1 does not
  if (t1con->get_t1sync() == 0 && m_sleeping)
    return;

    // If TMRH/TMRL have been manually changed, we'll want to 
    // get the up-to-date values;

    trace.raw(write_trace.get() | value.get());
    //trace.register_write(address,value.get());
    current_value();

    value_16bit = 0xffff & ( value_16bit + 1);

    tmrh->value.put((value_16bit >> 8) & 0xff);
    value.put(value_16bit & 0xff);
    if(value_16bit == 0 && m_Interrupt)
    {
      if (verbose & 4)
          cout << "TMRL:increment interrupt now=" << dec << get_cycles().get() << " value_16bit "  << value_16bit << endl;
      m_Interrupt->Trigger();
    }
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

    last_cycle = (gint64)(get_cycles().get() -
	(value.get() + (tmrh->value.get()<<8)) * prescale * ext_scale + 0.5);
    update();
  }
  else {

    Dprintf(("TMR1 is being turned off\n"));

    // turn off the timer and save the current value
    current_value();
    if (future_cycle)
    {
        get_cycles().clear_break(this);
	future_cycle = 0;
    }
  }

}
//
// If anything has changed to affect when the next TMR1 break point
// will occur, this routine will make sure the break point is moved
// correctly.
//

void TMRL::update()
{

  Dprintf(("TMR1 update 0x%"PRINTF_GINT64_MODIFIER"x\n",get_cycles().get()));

  // The second part of the if will always be true unless TMR1 Gate enable
  // pin has been defined by a call to TMRL::setGatepin()
  //
  bool gate = t1con->get_t1GINV() ? m_GateState : !m_GateState;
  if(t1con->get_tmr1on() && (t1con->get_tmr1GE() ? gate : true)) 
  {
    if(t1con->get_tmr1cs() && t1con->get_t1oscen()) 
    {
	/*
	 external timer1 clock runs off a crystal which is typically
	 32768 Hz and is independant on the instruction clock, but
	 gpsim runs on the instruction clock. Ext_scale is the ratio 
	 of these two clocks so the breakpoint can be adjusted to be
	 triggered at the correct time. 
	*/
      if(verbose & 0x4)
	cout << "Tmr1 External clock\n";
      
    }
    else if(t1con->get_tmr1cs() && !  t1con->get_t1oscen()) 
    {
      prescale = 1 << t1con->get_prescale();
      prescale_counter = prescale;
      set_ext_scale();
      return;
    }
    else
    {
      if(verbose & 0x4)
	cout << "Tmr1 Internal clock\n";
    }

    set_ext_scale();


      // Note, unlike TMR0, anytime something is written to TMRL, the 
      // prescaler is unaffected on the P18 processors. However, it is
      // reset on the p16f88 processor, which is how the current code
      // works. This only effects the external drive mode.

      prescale = 1 << t1con->get_prescale();
      prescale_counter = prescale;

      if(verbose & 0x4)
        cout << "TMRL: Current prescale " << prescale << ", ext scale " << ext_scale << '\n';
      //  synchronized_cycle = cycles.get() + 2;
      synchronized_cycle = get_cycles().get();


      last_cycle = synchronized_cycle  
			- (gint64)(value_16bit *( prescale * ext_scale) + 0.5);


      break_value = 0x10000;  // Assume that a rollover will happen first.

      for ( TMR1CapComRef * event = compare_queue; event; event = event->next )
      {
        if(verbose & 0x4)
	    cout << "compare mode on " << event->ccpcon << ", value = " << event->value << '\n';
	if ( event->value > value_16bit && event->value < break_value )
	{
	    // A compare interrupt is going to happen before the timer
	    // will rollover.
	    break_value = event->value;
	}
      }
      if(verbose & 0x4)
        cout << "TMR1 now at " << value_16bit << ", next event at " << break_value << '\n';

      guint64 fc = get_cycles().get() 
		+ (guint64)((break_value - value_16bit) * prescale * ext_scale);

      if(future_cycle)
	get_cycles().reassign_break(future_cycle, fc, this);
      else
	get_cycles().set_break(fc, this);

      future_cycle = fc;
    }
  else
    {
      // turn off the timer and save the current value
      if (future_cycle)
      {
        current_value();
        get_cycles().clear_break(this);
	future_cycle = 0;
      } 
      //cout << "TMR1: not running\n";
    }
}

void TMRL::put(unsigned int new_value)
{

  set_ext_scale();
  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());
  value.put(new_value & 0xff);

  if (!tmrh || !t1con)
    return;

  synchronized_cycle = get_cycles().get();
  last_cycle = synchronized_cycle - (gint64)(( value.get() 
	+ (tmrh->value.get()<<8)) * prescale * ext_scale + 0.5);

  current_value();

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
  if(get_cycles().get() <= synchronized_cycle)
    return value.get();

  // If TMRL is not on, then return the current value
  if(!t1con->get_tmr1on())
    return value.get();

  current_value();

  return(value.get());
}

//%%%FIXME%%% inline this
// if break inactive (future_cycle == 0), just read the TMR1H and TMR1L 
// registers otherwise compute what the register should be and then
// update TMR1H and TMR1L.
// RP: Using future_cycle here is not strictly right. What we really want is
// the condition "TMR1 is running on a GPSIM-generated clock" (as opposed
// to being off, or externally clocked by a stimulus). The presence of a
// breakpoint is _usually_ a good indication of this, but not while we're
// actually processing that breakpoint. For the time being, we work around
// this by calling current_value "redundantly" in callback()
//
void TMRL::current_value()
{
  if(future_cycle == 0)
    value_16bit = tmrh->value.get() * 256 + value.get();
  else
  {
    value_16bit = (guint64)((get_cycles().get() - last_cycle)/ 
		(prescale* ext_scale)) & 0xffff;
    value.put(value_16bit & 0xff);
    tmrh->value.put((value_16bit>>8) & 0xff);
  }
}

unsigned int TMRL::get_low_and_high()
{
  // If the TMRL is being read immediately after being written, then
  // it hasn't had enough time to synchronize with the PIC's clock.
  if(get_cycles().get() <= synchronized_cycle)
    return value.get();

  current_value();

  trace.raw(read_trace.get() | value.get());

  trace.raw(tmrh->read_trace.get() | tmrh->value.get());

  return(value_16bit);
  
}

// set m_bExtClkEnable is tmr1 is being clocked by an external stimulus
void TMRL::new_clock_source()
{

  m_bExtClkEnabled = false;

  current_value();

  if(t1con->get_tmr1cs() && ! t1con->get_t1oscen()) // external stimuli
  {
      if(verbose & 0x4)
	cout << "Tmr1 External Stimuli\n";
      if (future_cycle)
      {
        // Compute value_16bit with old prescale and ext_scale
      	current_value();
        get_cycles().clear_break(this);
        future_cycle = 0;
      }
      prescale = 1 << t1con->get_prescale();
      prescale_counter = prescale;
      set_ext_scale();
      m_bExtClkEnabled = true;
  }
  else if(! t1con->get_tmr1cs() && ! t1con->get_t1oscen()) // Fosc/4
  {
      if(verbose & 0x4)
	cout << "Tmr1 Fosc/4 \n";
      put(value.get());
  }
  else {
      if(verbose & 0x4)
	cout << "Tmr1 External Crystal\n";
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

  last_cycle = get_cycles().get();
  if(verbose & 0x4)
    cout << "TMR1 has been cleared\n";
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
  if(t1con->get_tmr1cs() && ! t1con->get_t1oscen())
    {
      if(verbose & 4)
        cout << "TMRL:callback No oscillator\n";
      value.put(0);
      tmrh->value.put(0);
      future_cycle = 0;  // indicates that TMRL no longer has a break point
      return;
    }

  current_value();      // Because this relies on future_cycle, we must call it before clearing that

  future_cycle = 0;     // indicate that there's no break currently set

  if(break_value < 0x10000)
  {
      // The break was due to a "compare"

      if ( value_16bit != break_value )
          cout << "TMR1 compare break: value=" << value_16bit << " but break_value=" << break_value << '\n';

      if(verbose & 4)
        cout << "TMR1 break due to compare "  << hex << get_cycles().get() << '\n';

      for ( TMR1CapComRef * event = compare_queue; event; event = event->next )
      {
	if ( event->value == break_value )
	{
	    // This CCP channel has a compare at this time
          event->ccpcon->compare_match();
	}
      }
    }
  else
    {

      // The break was due to a roll-over

      //cout<<"TMRL rollover: " << hex << cycles.get() << '\n';
      if (m_Interrupt)
	m_Interrupt->Trigger();

      // Reset the timer to 0.

      synchronized_cycle = get_cycles().get();
      last_cycle = synchronized_cycle;
      value.put(0);
      tmrh->value.put(0);
    }

  update();

}

//---------------------------

void TMRL::callback_print()
{
  cout << "TMRL " << name() << " CallBack ID " << CallBackID << '\n';

}


//---------------------------

void TMRL::sleep()
{
    m_sleeping = true;
    Dprintf(("TMRL::sleep t1sysc %d\n", t1con->get_t1sync()));
    // If tmr1 is running off Fosc/4 this assumes Fosc stops during sleep
    if (  t1con->get_tmr1on() && t1con->get_tmr1cs() == 0)
    {
      if (future_cycle)
      {
        current_value();
        get_cycles().clear_break(this);
	future_cycle = 0;
      } 
    }
}

//---------------------------

void TMRL::wake()
{
    m_sleeping = false;
    Dprintf(("TMRL::wake\n"));
    if (  t1con->get_tmr1on() && t1con->get_tmr1cs() == 0)
    {
	update();
    }
}

//--------------------------------------------------
// member functions for the PR2 base class
//--------------------------------------------------

PR2::PR2(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc),
    tmr2(0)
{
}

void PR2::put(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());
  Dprintf(("PR2:: put %x\n", new_value));

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

T2CON::T2CON(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc),
    tmr2(0)
{
}

void T2CON::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());

  unsigned int diff = value.get() ^ new_value;
  value.put(new_value);

  if (tmr2) {
    
    tmr2->new_pre_post_scale();

    if( diff & TMR2ON)
      tmr2->on_or_off(value.get() & TMR2ON);

  }
}



//--------------------------------------------------
// member functions for the TMR2 base class
//--------------------------------------------------
TMR2::TMR2(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc),
    pwm_mode(0),
    update_state(TMR2_PWM1_UPDATE | TMR2_PWM2_UPDATE | TMR2_PR2_UPDATE),
    last_update(0),
    prescale(1),
    prescale_counter(0),
    last_cycle(0),
    pr2(0), pir_set(0), t2con(0), ccp1con(0), ccp2con(0), ssp_module(0)
{
  value.put(0);
  future_cycle = 0;
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


void TMR2::on_or_off(int new_state)
{

  if(new_state) {

    Dprintf(("TMR2 is being turned on\n"));

    // turn on the timer

    // Effective last cycle
    // Compute the "effective last cycle", i.e. the cycle
    // at which TMR2 was last 0 had it always been counting.

    last_cycle = get_cycles().get() - value.get()*prescale;
    update();
  }
  else {

    Dprintf(("TMR2 is being turned off\n"));

    // turn off the timer and save the current value
    current_value();
  }

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

  //cout << "TMR2 update. cpu cycle " << hex << cycles.get() <<'\n';

  if(t2con->get_tmr2on()) {

    if(future_cycle) {

      // If TMR2 is enabled (i.e. counting) then 'future_cycle' is non-zero,
      // which means there's a cycle break point set on TMR2 that needs to
      // be moved to a new cycle.

      current_value();

      // Assume that we are not in pwm mode (and hence the next break will
      // be due to tmr2 matching pr2)

      break_value = 1 + pr2->value.get();
      guint64 fc = get_cycles().get() + (break_value - value.get()) * prescale;

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

  unsigned int old_value = get_value();


  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());

  value.put(new_value & 0xff);

  if(future_cycle)
    {
      // If TMR2 is enabled (i.e. counting) then 'future_cycle' is non-zero,
      // which means there's a cycle break point set on TMR2 that needs to
      // be moved to a new cycle.

      guint64 current_cycle = get_cycles().get();
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
	guint64 togo = (future_cycle - get_cycles().get()) * prescale / old_prescale;

	if (!togo)	// I am not sure this can happen RRR
	    callback();
	else
	{
	  guint64 fc = togo + get_cycles().get();

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
        future_cycle = get_cycles().get();
        get_cycles().set_break(future_cycle, this);
	callback();
      }
      else if (value.get() > pr2->value.get()) // TMR2 > PR2
      {
	cout << "Warning TMR2 turned on with TMR2 greater than PR2\n";
	// this will cause TMR2 to wrap
        future_cycle  = get_cycles().get() + 
		(1 + pr2->value.get() + (0x100 -  value.get())) * prescale;
        get_cycles().set_break(future_cycle, this);
      }
      else
      {
	future_cycle = get_cycles().get() + 1;
          get_cycles().set_break(future_cycle, this);
	  last_cycle = get_cycles().get() - value.get();
          update(update_state);
      }
  }

}

void TMR2::new_pr2(unsigned int new_value)
{
  Dprintf(("TMR2::new_pr2 on=%d\n", t2con->get_tmr2on()));

  if(t2con->get_tmr2on())
    {

      unsigned int cur_break = (future_cycle - last_cycle)/prescale;
      unsigned int new_break = (pwm_mode)? (1 + new_value) << 2 : 1 + new_value;
      unsigned int now_cycle = (get_cycles().get() - last_cycle) / prescale;

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
  unsigned int tmr2_val = (get_cycles().get() - last_cycle)/ prescale;

  if (pwm_mode)
       tmr2_val >>= 2;

  value.put(tmr2_val & 0xff);

  if(tmr2_val > 0x100)	// Can get to 0x100 during transition
  {
   cout << "TMR2 BUG!! value = " << tmr2_val << " which is greater than 0xff\n";
  }
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
	last_cycle = get_cycles().get();
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
	  last_cycle = get_cycles().get();

         if (ssp_module)
               ssp_module->tmr2_clock();
          if ((ccp1con->value.get() & CCPCON::PWM0) == CCPCON::PWM0)
	  {
               ccp1con->pwm_match(1);
	  }

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

//--------------------------------------------------
//
//--------------------------------------------------

class INT_SignalSink : public SignalSink
{
public:
  INT_SignalSink(ECCPAS *_eccpas, int _index)
    : m_eccpas(_eccpas), m_index(_index)
  {
    assert(_eccpas);
  }

  void release() 
  {
    delete this;
  }
  void setSinkState(char new3State)
  {
    m_eccpas->set_trig_state( m_index, new3State=='0' || new3State=='w');
  }
private:
  ECCPAS *m_eccpas;
  int     m_index;
};

//--------------------------------------------------
// ECCPAS
//--------------------------------------------------
ECCPAS::ECCPAS(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc),
    pwm1con(0), ccp1con(0), bit_mask(0xff),
    m_PinModule(0)
{
    trig_state[0] = trig_state[1] = trig_state[2] = false;
}

ECCPAS::~ECCPAS()
{
}
void ECCPAS::link_registers(PWM1CON *_pwm1con, CCPCON *_ccp1con)
{
	pwm1con = _pwm1con;
	ccp1con = _ccp1con;
}
void ECCPAS::put(unsigned int new_value)
{
  Dprintf(("ECCPAS::put() new_value=0x%x\n",new_value));
  trace.raw(write_trace.get() | value.get());
  put_value(new_value);
}
void ECCPAS::put_value(unsigned int new_value)
{

  int old_value = value.get();
  new_value &= bit_mask;


  // Auto-shutdown trigger active
  //   make sure ECCPASE is set
  //   if change in shutdown status, drive bridge outputs as per current flags
  if (shutdown_trigger(new_value))
  {
	new_value |= ECCPASE;
	if ((new_value ^ old_value) &  (ECCPASE|PSSAC1|PSSAC0|PSSBD1|PSSBD0))
	    ccp1con->shutdown_bridge(new_value);
  }
  else // no Auto-shutdown triggers active
  {
      if (pwm1con->value.get() & PWM1CON::PRSEN) // clear ECCPASE bit
	new_value &= ~ ECCPASE;
  }
  value.put(new_value);
}
// Return true is shutdown trigger is active
bool ECCPAS::shutdown_trigger(int key)
{

  if ((key & ECCPAS0) && trig_state[0])
	return true;

  if ((key & ECCPAS1) && trig_state[1])
	return true;

  if ((key & ECCPAS2) && trig_state[2])
	return true;

  return false;
}
// connect IO pins to shutdown trigger source
void ECCPAS::setIOpin(PinModule *p0, PinModule *p1, PinModule *p2)
{
    if (p0)
    {
        m_PinModule = p0;
        m_sink = new INT_SignalSink(this, 0);
        p0->addSink(m_sink);
    }
    if (p1)
    {
        m_PinModule = p1;
        m_sink = new INT_SignalSink(this, 1);
        p1->addSink(m_sink);
    }
    if (p2)
    {
        m_PinModule = p2;
        m_sink = new INT_SignalSink(this, 2);
        p2->addSink(m_sink);
    }
}

// set shutdown trigger states
void ECCPAS::set_trig_state(int index, bool state)
{
    if (trig_state[index] != state)
    {
	Dprintf(("index=%d state=%d old=%d\n", index, state, trig_state[index]));
        trig_state[index] = state;
        put_value(value.get());
    }
}
// Trigger state from comparator 1
void ECCPAS::c1_output(int state)
{
    set_trig_state(0, state);
}
// Trigger state from comparator 2
void ECCPAS::c2_output(int state)
{
    set_trig_state(1, state);
}
//--------------------------------------------------
// PWM1CON
//--------------------------------------------------
PWM1CON::PWM1CON(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc),
  bit_mask(0xff)
{
}

PWM1CON::~PWM1CON()
{
}
void PWM1CON::put(unsigned int new_value)
{

  new_value &= bit_mask;
  Dprintf(("PWM1CON::put() new_value=0x%x\n",new_value));
  trace.raw(write_trace.get() | value.get());

  value.put(new_value);
}
//--------------------------------------------------
// PSTRCON
//--------------------------------------------------
PSTRCON::PSTRCON(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc)
{
}

PSTRCON::~PSTRCON()
{
}
void PSTRCON::put(unsigned int new_value)
{

  Dprintf(("PSTRCON::put() new_value=0x%x\n",new_value));
  new_value &= STRSYNC|STRD|STRC|STRB|STRA;
  trace.raw(write_trace.get() | value.get());

  value.put(new_value);
}
