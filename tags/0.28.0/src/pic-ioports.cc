/*
   Copyright (C) 1998 Scott Dattalo
   Copyright (C) 2009 Roy R. Rankin

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
#include "pic-processor.h"
#include "14bit-processors.h"  // %%% FIXME %%% remove the dependencies on this
#include "pic-ioports.h"
#include "interface.h"
#include "psp.h"

#include "xref.h"
//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
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

    char r = m_register ? (((m_register->getDriving()&m_bitMask)!=0)?'1':'0') : 'Z';
    /**/
    Dprintf(("PicSignalSource::getState() %s  bitmask:0x%x state:%c\n",
	     (m_register?m_register->name().c_str():"NULL"),
	     m_bitMask,r));
    /**/
    return r;
  }
  void release()
  {
    delete this;
  }
private:
  PortRegister *m_register;
  unsigned int  m_bitMask;
};



//------------------------------------------------------------------------

PicPortRegister::PicPortRegister(Processor *pCpu, const char *pName, const char *pDesc,
                                 /*const char *port_name,*/
				 unsigned int numIopins, 
				 unsigned int enableMask)
  : PortRegister(pCpu, pName, pDesc,numIopins, false), m_tris(0)
{
  setEnableMask(enableMask);
}

class PicSignalControl : public SignalControl
{
public:
  PicSignalControl(PicTrisRegister *_reg, unsigned int bitPosition)
    : m_register(_reg), m_bitMask(1<<bitPosition)
  {
  }
  ~PicSignalControl()
  {
  }

  virtual char getState()
  {
    return m_register ? m_register->get3StateBit(m_bitMask) : '?';
  }
  virtual void release()
  {
    delete this;
  }
private:
  PicTrisRegister *m_register;
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

PicTrisRegister::PicTrisRegister(Processor *pCpu, const char *pName, const char *pDesc,
                                 /*const char *tris_name,*/
				 PicPortRegister *_port,
                                 bool bIgnoreWDTResets,
				 unsigned int enableMask)
  : sfr_register(pCpu, pName, pDesc),
    m_port(_port),
    m_EnableMask(enableMask),
    m_bIgnoreWDTResets(bIgnoreWDTResets)
{
  if (m_port)
    m_port->setTris(this);
}
void PicTrisRegister::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.data);
  value.put((value.get() & ~m_EnableMask) | (new_value & m_EnableMask));

  if (m_port)
    m_port->updatePort();
}

void PicTrisRegister::put_value(unsigned int new_value)
{
  value.put(new_value & m_EnableMask);

  if (m_port)
    m_port->updatePort();
}

unsigned int PicTrisRegister::get()
{
  trace.raw(read_trace.get() | value.data);
  return value.data;
}

void PicTrisRegister::setEnableMask(unsigned int enableMask)
{
  m_EnableMask = enableMask;
}

char PicTrisRegister::get3StateBit(unsigned int bitMask)
{
  RegisterValue rv = getRV_notrace();
  unsigned int enabled = bitMask & m_EnableMask;
  if (!enabled)
    return '1';

  return (rv.init&enabled) ? '?' : (rv.data&enabled ? '1': '0');

}
void PicTrisRegister::reset(RESET_TYPE r)
{
  if (!(m_bIgnoreWDTResets && r==WDT_RESET))
    putRV(por_value);
}


//------------------------------------------------------------------------

PicPSP_TrisRegister::PicPSP_TrisRegister(Processor *pCpu, const char *pName, const char *pDesc,
                                         /*const char *tris_name, */
                                         PicPortRegister *_port, bool bIgnoreWDTResets)
  : PicTrisRegister(pCpu, pName, pDesc,_port,bIgnoreWDTResets)
{
}
// If not in PSPMODE, OBF and IBF are always clear
// When in PSPMODE, OBF and IBF can only be cleared by reading and writing
// to the PSP parallel port and are set by bus transfers.
//
void PicPSP_TrisRegister::put(unsigned int new_value)
{
  unsigned int mask = (PSP::OBF | PSP::IBF);
  unsigned int fixed;

  trace.raw(write_trace.get() | value.data);
  if (! (new_value & PSP::PSPMODE))
    fixed = 0;
  else
    fixed = value.data & mask;

  value.data = (new_value & ~mask) | fixed;
  if (m_port)
    m_port->updatePort();
}
// used by gpsim to change register value
void PicPSP_TrisRegister::put_value(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.data);

  value.data = new_value;
  if (m_port)
    m_port->updatePort();
}

unsigned int PicPSP_TrisRegister::get(void)
{
  return value.data;
}


//------------------------------------------------------------------------



PicPortBRegister::PicPortBRegister(Processor *pCpu, const char *pName, const char *pDesc,
                                   INTCON *pIntcon,
				   unsigned int numIopins, 
				   unsigned int enableMask,
				   INTCON2 *pIntcon2,
				   INTCON3 *pIntcon3)
  : PicPortRegister(pCpu, pName, pDesc, numIopins, enableMask),
    m_bRBPU(false),
    m_bIntEdge(true), 
    m_bsRBPU(0),
    m_pIntcon(pIntcon),
    m_pIntcon2(pIntcon2),
    m_pIntcon3(pIntcon3)
{
  assert(m_pIntcon);
}

PicPortBRegister::~PicPortBRegister()
{
  delete m_bsRBPU;
}
//------------------------------------------------------------------------

void PicPortBRegister::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.data);

  m_pIntcon->set_rbif(false);

//  unsigned int diff = mEnableMask & (new_value ^ value.data);
//RRR  if(diff) {
    drivingValue = new_value & mEnableMask;
    value.data = drivingValue;
    // If no stimuli are connected to the Port pins, then the driving
    // value and the driven value are the same. If there are external
    // stimuli (or perhaps internal peripherals) overdriving or overriding
    // this port, then the call to updatePort() will update 'drivenValue'
    // to its proper value.
    updatePort();
//RRR  }

}

unsigned int PicPortBRegister::get()
{
  m_pIntcon->set_rbif(false);

  return mOutputMask & rvDrivenValue.data;
}

//------------------------------------------------------------------------
// setbit
// FIXME - a sink should be created for the intf and rbif functions.

void PicPortBRegister::setbit(unsigned int bit_number, char new3State)
{
  Dprintf(("PicPortBRegister::setbit() bit=%d,val=%c bIntEdge=%d\n",bit_number,new3State, m_bIntEdge));

  // interrupt bit 0 on specified edge 
  bool bNewValue = new3State=='1' || new3State=='W';
  RegisterValue lastDrivenValue = rvDrivenValue;
  PortRegister::setbit(bit_number, new3State);

  if (m_pIntcon3)
  {
	bool drive = (lastDrivenValue.data&(1 << bit_number)) ;
	bool level;
	int intcon2 = m_pIntcon2->value.get();
	int intcon3 = m_pIntcon3->value.get();
	switch(bit_number)
	{
	case 0:
	    level = intcon2 & INTCON2::INTEDG0;
	    if ((drive != level) && (bNewValue == level)) 
	    {
    		cpu_pic->exit_sleep();
		m_pIntcon->set_intf(true);
//		if (((intcon3 & INTCON::INTE) == 0) ||
//		    (m_pIntcon->value.get() & INTCON_16::GIEH) == 0)
//		    return; // Interrupts are disabled
//		((INTCON_16 *)m_pIntcon)->set_interrupt_vector(INTERRUPT_VECTOR_HI);
//		cpu_pic->BP_set_interrupt();
	    }
	    return;
	    break;

	case 1:
	    level = intcon2 & INTCON2::INTEDG1;
	    if ((drive != level) && (bNewValue == level)) 
	    {
    		cpu_pic->exit_sleep();
		m_pIntcon3->set_int1f(true);
		if (((intcon3 & INTCON3::INT1IE) == 0) ||
		    (m_pIntcon->value.get() & INTCON_16::GIEH) == 0)
		    return; // Interrupts are disabled
		if (intcon3 & INTCON3::INT1IP) //priority interrupt
	        {
		    ((INTCON_16 *)m_pIntcon)->set_interrupt_vector(INTERRUPT_VECTOR_HI);
		    cpu_pic->BP_set_interrupt();
		}
		else if ((m_pIntcon->value.get() & INTCON_16::GIEL) != 0)
		{
		    ((INTCON_16 *)m_pIntcon)->set_interrupt_vector(INTERRUPT_VECTOR_LO);
		    cpu_pic->BP_set_interrupt();
		}
	    }
	    return;
	    break;

	case 2:
	    level = intcon2 & INTCON2::INTEDG2;
	    if ((drive != level) && (bNewValue == level)) 
	    {
    		cpu_pic->exit_sleep();
		m_pIntcon3->set_int2f(true);
		if (((intcon3 & INTCON3::INT2IE) == 0) ||
		    (m_pIntcon->value.get() & INTCON_16::GIEH) == 0)
		    return; // Interrupts are disabled
		if (intcon3 & INTCON3::INT2IP) //priority interrupt
	        {
		    ((INTCON_16 *)m_pIntcon)->set_interrupt_vector(INTERRUPT_VECTOR_HI);
		    cpu_pic->BP_set_interrupt();
		}
		else if ((m_pIntcon->value.get() & INTCON_16::GIEL) != 0)
		{
		    ((INTCON_16 *)m_pIntcon)->set_interrupt_vector(INTERRUPT_VECTOR_LO);
		    cpu_pic->BP_set_interrupt();
		}
	    }
	    return;
	    break;

	case 3:
	    level = intcon2 & INTCON2::INTEDG3;
	    if ((drive != level) && (bNewValue == level)) 
	    {
    		cpu_pic->exit_sleep();
		m_pIntcon3->set_int3f(true);
		if (((intcon3 & INTCON3::INT3IE) == 0) ||
		    (m_pIntcon->value.get() & INTCON_16::GIEH) == 0)
		    return; // Interrupts are disabled
		if (intcon2 & INTCON2::INT3IP) //priority interrupt
	        {
		    ((INTCON_16 *)m_pIntcon)->set_interrupt_vector(INTERRUPT_VECTOR_HI);
		    cpu_pic->BP_set_interrupt();
		}
		else if ((m_pIntcon->value.get() & INTCON_16::GIEL) != 0)
		{
		    ((INTCON_16 *)m_pIntcon)->set_interrupt_vector(INTERRUPT_VECTOR_LO);
		    cpu_pic->BP_set_interrupt();
		}
	    }
	    return;
	    break;
	}
  }
  if (bit_number == 0 && (((lastDrivenValue.data&1)==1)!=m_bIntEdge) 
      && (bNewValue == m_bIntEdge))
  {
    if ((m_pIntcon->get() & (INTCON::GIE | INTCON::INTE)) == INTCON::INTE)
    {
        cpu_pic->exit_sleep();
    }
    m_pIntcon->set_intf(true);
  }



 // interrupt and exit sleep level change top 4 bits on input
  unsigned int bitMask = (1<<bit_number) & 0xF0;

  if ( (lastDrivenValue.data ^ rvDrivenValue.data) & m_tris->get_value() & bitMask ) {
    
    if ((m_pIntcon->get() & (INTCON::GIE | INTCON::RBIE)) == INTCON::RBIE)
        cpu_pic->exit_sleep();
    m_pIntcon->set_rbif(true);
  }
}

class RBPUBitSink : public BitSink
{
  PicPortBRegister *m_pPortB;
public:
  RBPUBitSink(PicPortBRegister *pPortB) 
    : m_pPortB(pPortB) 
  {}
    
  void setSink(bool b)
  {
    if (m_pPortB)
      m_pPortB->setRBPU(b);
  }
};

void PicPortBRegister::assignRBPUSink(unsigned int bitPos, sfr_register *pSFR)
{
  if (pSFR && !m_bsRBPU) {
    m_bsRBPU = new RBPUBitSink(this);
    if (!pSFR->assignBitSink(bitPos, m_bsRBPU)) {
      delete m_bsRBPU;
      m_bsRBPU = 0;
    }
  }
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

// set_rbif involves RBIF,RBIE in INTCON which are the same bits as GPIF,GPIE
void PicPortGRegister::setbit(unsigned int bit_number, char new3State)
{
  // interrupt bit 2 on specified edge 
  bool bNewValue = new3State=='1' || new3State=='W';
  if (bit_number == 2 && (((rvDrivenValue.data&4)==4)!=m_bIntEdge) 
      && (bNewValue == m_bIntEdge))
    m_pIntcon->set_intf(true);

    RegisterValue lastDrivenValue = rvDrivenValue;
    PortRegister::setbit(bit_number, new3State);

 // interrupt and exit sleep for level change on bits where IOC set
    int bitMask = m_pIoc->get_value() & (1 << bit_number);

   if (verbose)
       printf("PicPortGRegister::setbit() bit=%d,val=%c IOC_bit=%x\n",bit_number,new3State, bitMask);

    if ( (lastDrivenValue.data ^ rvDrivenValue.data) & m_tris->get_value() & bitMask ) {
      cpu_pic->exit_sleep();
      m_pIntcon->set_rbif(true);
    }
}
void PicPortIOCRegister::setbit(unsigned int bit_number, char new3State)
{
    int lastDrivenValue = rvDrivenValue.data & (1 << bit_number);
    PortRegister::setbit(bit_number, new3State);
    int newDrivenValue = rvDrivenValue.data & (1 << bit_number);

   if (verbose)
       printf("PicPortIOCRegister::setbit() bit=%d,val=%c IOC_+=%x IOC_-=%x\n",bit_number,new3State, m_Iocap->get_value() & (1<<bit_number), m_Iocan->get_value() & (1<<bit_number));
    if ( newDrivenValue > lastDrivenValue) // positive edge
    {
	if ( m_tris->get_value() & (m_Iocap->get_value() & (1 << bit_number)))
	{
          cpu_pic->exit_sleep();
          m_pIntcon->set_rbif(true);
	  m_Iocaf->put(m_Iocaf->get_value() | (1 << bit_number));
	}
    }
    else if ( newDrivenValue < lastDrivenValue) // negative edge
    {
	if ( m_tris->get_value() & (m_Iocan->get_value() & (1 << bit_number)))
	{
          cpu_pic->exit_sleep();
          m_pIntcon->set_rbif(true);
	  m_Iocaf->put(m_Iocaf->get_value() | (1 << bit_number));
	}
    }
}

PicPSP_PortRegister::PicPSP_PortRegister(Processor *pCpu, const char *pName, const char *pDesc,
                                         /*const char *port_name,*/
                                         unsigned int numIopins,
                                         unsigned int enableMask)
  : PortRegister(pCpu, pName, pDesc,numIopins, false), m_tris(0), m_psp(0)
{
  setEnableMask(enableMask);
}

void PicPSP_PortRegister::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.data);
  unsigned int diff = mEnableMask & (new_value ^ value.data);

  if (m_psp && m_psp->pspmode())
  {
	m_psp->psp_put(new_value);
  }
  else if(diff) {
    drivingValue = new_value & mEnableMask;
    value.data = drivingValue;
    // If no stimuli are connected to the Port pins, then the driving
    // value and the driven value are the same. If there are external
    // stimuli (or perhaps internal peripherals) overdriving or overriding
    // this port, then the call to updatePort() will update 'drivenValue'
    // to its proper value.
    updatePort();
  }

}
unsigned int PicPSP_PortRegister::get()
{

  if (m_psp && m_psp->pspmode())
	return(m_psp->psp_get());

  return rvDrivenValue.data;
}


void PicPSP_PortRegister::setTris(PicTrisRegister *new_tris)
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
// Latch Register

PicLatchRegister::PicLatchRegister(Processor *pCpu, const char *pName, const char *pDesc,
                                   /*const char *_name, */
				   PortRegister *_port,
				   unsigned int enableMask)
  : sfr_register(pCpu, pName, pDesc),
    m_port(_port), m_EnableMask(enableMask)
{
}

void PicLatchRegister::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.data);
  value.data = new_value & m_EnableMask;
  m_port->put_value(value.data);
}
void PicLatchRegister::put_value(unsigned int new_value)
{
  value.data = new_value & m_EnableMask;
  m_port->put_value(value.data);
}
unsigned int PicLatchRegister::get()
{
    value.data = m_port->getDriving();
  trace.raw(read_trace.get()  | value.data);
  trace.raw(read_trace.geti() | value.init);

  return value.data;
}
void PicLatchRegister::setbit(unsigned int bit_number, char new_value)
{
  printf("PicLatchRegister::setbit() -- shouldn't be called\n");
}
void PicLatchRegister::setEnableMask(unsigned int nEnableMask)
{
  m_EnableMask = nEnableMask;
}

