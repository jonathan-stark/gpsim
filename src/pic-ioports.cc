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
#include <string>


#include "../config.h"
#include "pic-processor.h"
#include "14bit-processors.h"  // %%% FIXME %%% remove the dependencies on this
#include "pic-ioports.h"
#include "interface.h"
#include "p16x6x.h"
#include "p16f62x.h"

#include "xref.h"
//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

//-------------------------------------------------------------------
//
//                 ioports.cc
//
// The ioport infrastructure for gpsim is provided here. The class
// taxonomy for the IOPORT class is:
//
//  file_register 
//     |-> sfr_register
//            |-> IOPORT
//                  |-> PORTA
//                  |-> PORTB
//                  |-> PORTC
//                  |-> PORTD
//                  |-> PORTE
//                  |-> PORTF
// 
// Each I/O port has an associated array of I/O pins which provide an
// interface to the virtual external world of the stimuli.
//
//-------------------------------------------------------------------

class PicSignalSource : public SignalControl
{
public:
  PicSignalSource(PortRegister *_reg, unsigned int bitPosition)
    : m_register(_reg), m_bitMask(1<<bitPosition)
  {
  }
  char getState()
  {
    // return m_register ? 
    //  (((m_register->getDriving()&m_bitMask)!=0)?'1':'0') : 'Z';
    char r = m_register ? (((m_register->getDriving()&m_bitMask)!=0)?'1':'0') : 'Z';
    /**/
    Dprintf(("PicSignalSource::getState() %s  bitmask:0x%x state:%c\n",
	     (m_register?m_register->name().c_str():"NULL"),
	     m_bitMask,r));
    /**/
    return r;
  }
private:
  PortRegister *m_register;
  unsigned int  m_bitMask;
};



//------------------------------------------------------------------------

PicPortRegister::PicPortRegister(const char *port_name,
				 unsigned int numIopins, 
				 unsigned int enableMask)
  : PortRegister(numIopins, false), m_tris(0)
{
  new_name(port_name);
  PortRegister::setEnableMask(enableMask);
}

class PicSignalControl : public SignalControl
{
public:
  PicSignalControl(Register *_reg, unsigned int bitPosition)
    : m_register(_reg), m_bitMask(1<<bitPosition)
  {
  }
  char getState()
  {
    return m_register ? m_register->get3StateBit(m_bitMask) : '?';
  }
private:
  Register *m_register;
  unsigned int m_bitMask;
};

void PicPortRegister::setTris(PicTrisRegister *new_tris)
{
  if (!m_tris)
    m_tris = new_tris;

    unsigned int mask = getEnableMask();
    for (unsigned int i=0, m = 1; i<mNumIopins; i++, m <<= 1) {
      if (mask & m)
          operator[](i).setDefaultControl(new PicSignalControl(m_tris, i));
    }
}
//------------------------------------------------------------------------

PicTrisRegister::PicTrisRegister(const char *tris_name, PicPortRegister *_port)
  : sfr_register(),m_port(_port)
{
  new_name(tris_name);
  if (m_port)
    m_port->setTris(this);
}
void PicTrisRegister::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.data);
  value.data = new_value;
  if (m_port)
    m_port->updatePort();
}

unsigned int PicTrisRegister::get(void)
{
  return value.data;
}




PicPortBRegister::PicPortBRegister(const char *port_name, unsigned int numIopins, unsigned int enableMask)
  : PicPortRegister(port_name, numIopins, enableMask),
    m_bRBPU(false),
    m_bIntEdge(false)
{
}

void PicPortBRegister::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.data);

  unsigned int diff = mEnableMask & (new_value ^ value.data);
  if(diff) {
    drivingValue = new_value & mEnableMask;
    value.data = drivingValue;
    // If no stimuli are connected to the Port pins, then the driving
    // value and the driven value are the same. If there are external
    // stimuli (or perhaps internal peripherals) overdriving or overriding
    // this port, then the call to updatePort() will update 'drivenValue'
    // to its proper value.
    updatePort();
  }

  cpu14->intcon->set_rbif(false);
}

unsigned int PicPortBRegister::get()
{
  cpu14->intcon->set_rbif(false);

  return rvDrivenValue.data;
}

//------------------------------------------------------------------------
// setbit
// FIXME - a sink should be created for the intf and rbif functions.

void PicPortBRegister::setbit(unsigned int bit_number, char new3State)
{
  Dprintf(("PicPortBRegister::setbit() bit=%d,val=%c\n",bit_number,new3State));

  bool bNewValue = new3State=='1' || new3State=='W';
  if (bit_number == 0 && (((rvDrivenValue.data&1)==1)!=m_bIntEdge) 
      && (bNewValue == m_bIntEdge))
    cpu14->intcon->set_intf(true);


  PortRegister::setbit(bit_number, new3State);

  unsigned int bitMask = (1<<bit_number) & 0xF0;

  if ( (drivingValue ^ rvDrivenValue.data) & m_tris->get() & bitMask )
    cpu14->intcon->set_rbif(true);
}

void PicPortBRegister::setRBPU(bool bNewRBPU)
{
  m_bRBPU = !bNewRBPU;

  Dprintf(("PicPortBRegister::setRBPU() =%d\n",(m_bRBPU?1:0)));

  unsigned int mask = getEnableMask();
  for (unsigned int i=0, m=1; mask; i++, m<<= 1)
    if (mask & m) {
      mask ^= m;
      operator[](i).getPin().update_pullup(m_bRBPU ? '1' : '0',true);
    }

}

void PicPortBRegister::setIntEdge(bool bNewIntEdge)
{
  m_bIntEdge = bNewIntEdge;
}

