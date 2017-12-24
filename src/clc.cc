/*
   Copyright (C) 2017 Roy R Rankin

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

// CONFIGURABLE LOGIC CELL (CLC)

//#define DEBUG
#if defined(DEBUG)
#include "../config.h"
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

#include "pic-processor.h"
#include "clc.h"

  // Report state changes on incoming INx pins
  class INxSignalSink : public SignalSink
  {
  public:
    INxSignalSink(CLC *_clc, int _index)
      : m_clc(_clc), index(_index)
    {
    }
  
    virtual void setSinkState(char new3State) 
	{m_clc->setState(new3State, index); }
    virtual void release() {delete this; }
  private:
    CLC *m_clc;
    int index;
  };

    class CLCSigSource : public SignalControl
    {
    public:
      CLCSigSource(CLC *_clc, PinModule *_pin)
        : m_clc(_clc),
        m_state('?'), m_pin(_pin)
      {
        assert(m_clc);
      }
      virtual ~CLCSigSource() { }
    
      void setState(char _state) { m_state = _state; }
      virtual char getState() { return m_state; }
      virtual void release() { m_clc->releasePinSource(m_pin); }
    
    private:
      CLC *m_clc;
      char m_state;
      PinModule *m_pin;
    };


    void CLCxCON::put(unsigned int new_value)
    {
        new_value &= write_mask;
        unsigned int diff = new_value ^ value.get();
        trace.raw(write_trace.get() | value.get());
        value.put(new_value);
        if (!diff) return;
        m_clc->update_clccon(diff);
    }

    void CLCxSEL0::put(unsigned int new_value)
    {
        new_value &= write_mask;
        trace.raw(write_trace.get() | value.get());
        unsigned int diff = new_value ^ value.get();
        value.put(new_value);
        if (!diff) return;
	if (diff & 0xf)
	    m_clc->D1S(new_value & 0xf);
	if (diff & 0xf0)
	    m_clc->D2S((new_value & 0xf0) >> 4);
    }
    void CLCxSEL1::put(unsigned int new_value)
    {
        new_value &= write_mask;
        trace.raw(write_trace.get() | value.get());
        unsigned int diff = new_value ^ value.get();
        value.put(new_value);
        if (!diff) return;
	if (diff & 0xf)
	    m_clc->D3S(new_value & 0xf);
	if (diff & 0xf0)
	    m_clc->D4S((new_value & 0xf0) >> 4);
    }

    CLC::CLC(Processor *cpu, unsigned int _index) :
      clcxcon(this, cpu, "clcxcon", "Configurable Logic Cell Control Register"),
      clcxpol(this, cpu, "clcxpol", "Configurable Logic Cell Signal Polarity"),
      clcxsel0(this, cpu, "clcxsel0", "Multiplexer Data 1 and 2 Select Register"),
      clcxsel1(this, cpu, "clcxsel1", "Multiplexer Data 3 and 4 Select Register"),
      clcxgls0(this, cpu, "clcxgls0", "Gate 1 Logic Select Register"),
      clcxgls1(this, cpu, "clcxgls1", "Gate 2 Logic Select Register"),
      clcxgls2(this, cpu, "clcxgls2", "Gate 3 Logic Select Register"),
      clcxgls3(this, cpu, "clcxgls3", "Gate 4 Logic Select Register"),
      index(_index)
    {
	for(int i=0; i<2; i++)
	{
	    INxsink[i] = 0;
	    INxactive[i] = 0;
	}
	for(int i=0; i<4; i++)
	{
	    CMxOUT_level[i] = false;
	    pwmx_level[i] = false;
	}
    }

    CLC::~CLC()
    {
        if (CLCxsrc)
        {
           if(srcCLCxactive)
                oeCLCx(false);
           delete CLCxsrc;
           CLCxsrc = 0;
        }

    }
	
    void CLC::setCLCxPin(PinModule *alt_pin)
    {
    }

    void CLC::D1S(int select)
    {
	D1Sselect = select;
    }
    void CLC::D2S(int select)
    {
	D2Sselect = select;
    }
    void CLC::D3S(int select)
    {
	D3Sselect = select;
    }
    void CLC::D4S(int select)
    {
	D4Sselect = select;
    }
    void CLC::t0_overflow()
    {
	Dprintf(("CLC%d t0_overflow() enable=%d\n", index+1, (bool)(clcxcon.value.get() & LCxEN)));
    }
    void CLC::t1_overflow()
    {
	Dprintf(("CLC%d t1_overflow() enable=%d\n", index+1, (bool)(clcxcon.value.get() & LCxEN)));
    }
    void CLC::t2_match()
    {
	Dprintf(("CLC%d t2_match() enable=%d\n", index+1, (bool)(clcxcon.value.get() & LCxEN)));
    }
    void CLC::NCO_out(bool level)
    {
	Dprintf(("CLC%d NCO_out() level=%d enable=%d\n", index+1, level, (bool)(clcxcon.value.get() & LCxEN)));
    }
    void CLC::CxOUT_sync(bool level, int cm)
    {
        if (CMxOUT_level[cm] != level)
	{
	Dprintf(("CLC%d CxOUT_sync() level=%d cm=%d enable=%d\n", index+1, level, cm, (bool)(clcxcon.value.get() & LCxEN)));
	   CMxOUT_level[cm] = level;
	}
    }
    void CLC::out_pwm(bool level, int id)
    {
	if (pwmx_level[id] != level)
	{
	Dprintf(("CLC%d out_pwm() id=%d level=%d enable=%d\n", index+1, id, level, (bool)(clcxcon.value.get() & LCxEN)));
	    pwmx_level[id] = level;
	}
    }

    // notification on input pin change
    void CLC::setState(char new3State, int index)
    {
    }
    void CLC::enableINxpin(int i, bool on)
    {
        if (on)
        {
	  if (!INxactive[i])
	  {
	    char name[7] = "LCyINx";
	    if (!INxgui[i].length())
                INxgui[i] = pinCLCxIN[i]->getPin().GUIname();
	    name[2] = '0' + index;
	    name[5] = '0' + i;
            pinCLCxIN[i]->getPin().newGUIname(name);
            if (!INxsink[i])
                INxsink[i] = new INxSignalSink(this, i);
            pinCLCxIN[i]->addSink(INxsink[i]);
            INxstate[i] = pinCLCxIN[i]->getPin().getState();
	  }
	  INxactive[i]++;
        }
	else if (! --INxactive[i])
	{
            if (INxgui[i].length())
                pinCLCxIN[i]->getPin().newGUIname(INxgui[i].c_str());
            else
                pinCLCxIN[i]->getPin().newGUIname(pinCLCxIN[i]->getPin().name().c_str());
            if (INxsink[i])
                pinCLCxIN[i]->removeSink(INxsink[i]);
        }
    }
    // Enable/disable output pin
    void CLC::oeCLCx(bool on)
    {
        if (on)
        {
           if (!srcCLCxactive)
           {
		char name[] = "CLCx";
		name[3] = '1' + index;
                CLCxgui = pinCLCx->getPin().GUIname();
                pinCLCx->getPin().newGUIname(name);
                if (!CLCxsrc)CLCxsrc = new CLCSigSource(this, pinCLCx);
                pinCLCx->setSource(CLCxsrc);
                srcCLCxactive = true;
                CLCxsrc->setState((clcxcon.value.get() & LCxOE) ? '1' : '0');
                pinCLCx->updatePinModule();
           }
        }
        else if (srcCLCxactive)
        {
            if (CLCxgui.length())
                pinCLCx->getPin().newGUIname(CLCxgui.c_str());
            else
                pinCLCx->getPin().newGUIname(pinCLCx->getPin().name().c_str());
            pinCLCx->setSource(0);
            srcCLCxactive = false;
            pinCLCx->updatePinModule();
        }
    }
void CLC::releasePinSource(PinModule *pin)
{
    if (pin)
    {

        if (pin == pinCLCx) srcCLCxactive = false;
    }
}

void CLC::update_clccon(unsigned int diff)
{
    unsigned int val = clcxcon.value.get();
    if (diff & LCxEN)	// clc off or on
    {
	if (val & LCxEN)	// CLC on
	{
	    fprintf(stderr, "warning CLC does not work, in development\n");
            scan_gates(true);
	}
	else			// CLC off
	{
	    scan_gates(false);
	    oeCLCx(false);
	}
    }
    if (diff & LCxOE)
    {
        if ((val & (LCxOE|LCxEN)) == (LCxOE|LCxEN))
	    oeCLCx(true);
        if ((val & (LCxOE|LCxEN)) == (LCxEN))
	    oeCLCx(false);
    }

}
void CLC::scan_gates(bool on)
{
   unsigned int active = clcxgls0.value.get() | clcxgls1.value.get() |
   			 clcxgls2.value.get() | clcxgls3.value.get();
   Dprintf(("scan_gates CLC%d on=%d active=0x%x\n", index, on, active));
   if (active & 0x3)
   {
        unsigned int DS0 = (clcxsel0.value.get() & 0xf0);
	Dprintf(("\tdata 1 0x%x\n", DS0));
        if (DS0 == 0)	// CLCxIN0
	    enableINxpin(0, on);
	else if (DS0 == 1) // CLCxIN1
	    enableINxpin(1, on);
   }
   if (active & 0xc)
	Dprintf(("\tdata 2 0x%x\n", (clcxsel0.value.get() & 0xf0)>>4));
   if (active & 0x30)
	Dprintf(("\tdata 3 0x%x\n", clcxsel1.value.get() & 0x0f));
   if (active & 0xc0)
   {
        unsigned int DS4 = (clcxsel1.value.get() & 0xf0)>>4;
	Dprintf(("\tdata 4 0x%x\n", DS4));
        if (DS4 == 4)	// CLCxIN0
	    enableINxpin(0, on);
	else if (DS4 == 5) // CLCxIN1
	    enableINxpin(1, on);
	
   }
}



