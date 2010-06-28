/*
   Copyright (C) 1998-2000 Scott Dattalo

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
#include "14bit-registers.h"
#include "psp.h"     // needed for operator[] on WPU::wpu_gpio (not sure why)

#include <string>
#include "stimuli.h"

#include "xref.h"
#define PCLATH_MASK              0x1f

//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("0x%06X %s() ",cycles.get(),__FUNCTION__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

pic_processor *temp_cpu;
// FIXME file_register::put_value has a useful feature...

//
#if 0
//-----------------------------------------------------------
//  void file_register::put_value(unsigned int new_value)
//
//  put_value is used by the gui to change the contents of
// file registers. We could've let the gui use the normal
// 'put' member function to change the contents, however
// there are instances where 'put' has a cascading affect.
// For example, changing the value of an i/o port's tris
// could cause i/o pins to change states. In these cases,
// we'd like the gui to be notified of all of the cascaded
// changes. So rather than burden the real-time simulation
// with notifying the gui, I decided to create the 'put_value'
// function instead. 
//   Since this is a virtual function, derived classes have
// the option to override the default behavior.
//
// inputs:
//   unsigned int new_value - The new value that's to be
//                            written to this register
// returns:
//   nothing
//
//-----------------------------------------------------------

void file_register::put_value(unsigned int new_value)
{

  // go ahead and use the regular put to write the data.
  // note that this is a 'virtual' function. Consequently,
  // all objects derived from a file_register should
  // automagically be correctly updated.

  put(new_value);

  // Even though we just wrote a value to this register,
  // it's possible that the register did not get fully
  // updated (e.g. porta on many pics has only 5 valid
  // pins, so the upper three bits of a write are meaningless)
  // So we should explicitly tell the gui (if it's
  // present) to update its display.

  if(xref)
    {
      xref->update();

      if(cpu && address == cpu_pic->fsr->value)
	{
	  if(cpu_pic->indf->xref)
	    cpu_pic->indf->xref->update();
	}
    }
}

#endif

// Adjust internal RC oscillator frequency as per 12f675/629
// Spec sheet does not give range so assume +/- 12.5% as per 16f88
// The fact that base_freq is not 0. indicates the RC oscillator is being used
// and thus an adjustment should be made.
//
// This will work for any number of adjustment bits in byte but must be left justified
// and 1000000 centre frequency and 11111111 highest frequency
void  OSCCAL::put(unsigned int new_value)
{
  int   adj = new_value & bit_mask;
  float tune;
  trace.raw(write_trace.get() | value.get());
  value.put(adj);
  if (base_freq > 0.)
  {
    adj  = adj -  0x80;
    tune = (1. + 0.125 * adj / 0x80) * base_freq;
    cpu_pic->set_frequency(tune);
  }
}

void OSCCAL::set_freq(float new_base_freq)
{
  base_freq = new_base_freq;
  put(value.get());
}

void  OSCTUNE::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  value.put(new_value);
  osccon->set_rc_frequency();
}
// Clock is stable
void OSCCON::callback()
{
    value.put(value.get() | IOFS);
}
bool OSCCON::set_rc_frequency()
{
  double base_frequency = 31.25e3;


  if (!cpu_pic->get_int_osc())
     return false;

    unsigned int new_IRCF = (value.get() & ( IRCF0 | IRCF1 | IRCF2)) >> 4;
    switch (new_IRCF)
    {
    case 0:
	base_frequency = 31.25e3;
	break;
	
    case 1:
	base_frequency = 125e3;
	break;
	
    case 2:
	base_frequency = 250e3;
	break;
	
    case 3:
	base_frequency = 500e3;
	break;
	
    case 4:
	base_frequency = 1e6;
	break;
	
    case 5:
	base_frequency = 2e6;
	break;
	
    case 6:
	base_frequency = 4e6;
	break;
	
    case 7:
	base_frequency = 8e6;
	break;
   }
   if (osctune)
   {
       int tune;
       unsigned int osctune_value = osctune->value.get();
       tune = osctune_value & (OSCTUNE::TUN5-1);
       tune = (OSCTUNE::TUN5 & osctune_value) ? -tune : tune;
       base_frequency *= 1. + 0.125 * tune / 31.;
   }
   cpu_pic->set_frequency(base_frequency);
   if ((bool)verbose)
   {
	cout << "set_rc_frequency() : osccon=" << hex << value.get();
	if (osctune)
	    cout << " osctune=" << osctune->value.get();
	cout << " new frequency=" << base_frequency << endl;
   }
   return true;
}
void  OSCCON::put(unsigned int new_value)
{
  
  unsigned int new_IRCF = (new_value & ( IRCF0 | IRCF1 | IRCF2)) >> 4;
  unsigned int old_IRCF = (value.get() & ( IRCF0 | IRCF1 | IRCF2)) >> 4;
  trace.raw(write_trace.get() | value.get());
  value.put(new_value);

  if (set_rc_frequency())  // using internal RC Oscillator
  {
	if (old_IRCF == 0 && new_IRCF != 0) // Allow 4 ms to stabalise
	{
	    guint64 settle;

	    settle = (guint64) (get_cycles().instruction_cps() * 4e-3);
	    get_cycles().set_break(get_cycles().get() + settle, this);
	    new_value &= ~ IOFS;
	}
	else
	    new_value |= IOFS;
  }
}

void WDTCON::put(unsigned int new_value)
{
  unsigned int masked_value = new_value & valid_bits;

  trace.raw(write_trace.get() | value.get());

  value.put(masked_value);

  if (valid_bits > 1)
      cpu_pic->wdt.set_prescale(masked_value >> 1);
  cpu_pic->wdt.swdten((masked_value & SWDTEN) == SWDTEN);
}
void WDTCON::reset(RESET_TYPE r)
{
   putRV(por_value);
}
//
//--------------------------------------------------
// member functions for the FSR class
//--------------------------------------------------
//
FSR::FSR(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc)
{}

void  FSR::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());

  value.put(new_value);
}

void  FSR::put_value(unsigned int new_value)
{
  put(new_value);

  update();
  cpu_pic->indf->update();
}


unsigned int FSR::get()
{
  trace.raw(read_trace.get() | value.get());
  return(value.get());
}

unsigned int FSR::get_value()
{
  return(value.get());
}


//
//--------------------------------------------------
// member functions for the FSR_12 class
//--------------------------------------------------
//
FSR_12::FSR_12(Processor *pCpu, const char *pName, unsigned int _rpb, unsigned int _valid_bits)
  : FSR(pCpu, pName, ""),
    valid_bits(_valid_bits),
    register_page_bits(_rpb)
{}

void  FSR_12::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  value.put(new_value);

  /* The 12-bit core selects the register page using the fsr */
  cpu_pic->register_bank = &cpu_pic->registers[ value.get() & register_page_bits ];
}

void  FSR_12::put_value(unsigned int new_value)
{

  put(new_value);

  update();
  cpu_pic->indf->update();

}


unsigned int FSR_12::get()
{
  unsigned int v = get_value();
  trace.raw(read_trace.get() | value.get());
  return(v);
}

unsigned int FSR_12::get_value()
{
  // adjust for missing bits
  //cout << "FSR_12:get_value - valid_bits 0x" << hex << valid_bits << endl;
  return ((value.get() & valid_bits) | (~valid_bits & 0xff));

}


//
//--------------------------------------------------
// member functions for the Status_register class
//--------------------------------------------------
//

//--------------------------------------------------

Status_register::Status_register(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc)
{
  rcon = NULL;
  break_point = 0;
  break_on_z =0;
  break_on_c =0;
  address = 3;
  rp_mask = RP_MASK;
  write_mask = 0xff & ~STATUS_TO & ~STATUS_PD;
  new_name("status");
}

//--------------------------------------------------
void Status_register::reset(RESET_TYPE r)
{
  switch (r) {

  case POR_RESET:
    putRV(por_value);
    put_TO(1);
    put_PD(1);
    break;

  case WDT_RESET:
    put_TO(0);
    break;

  default:
    break;
  }

}

//--------------------------------------------------
// put

void Status_register::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());

  value.put((value.get() & ~write_mask) | (new_value & write_mask));

  if(cpu_pic->base_isa() == _14BIT_PROCESSOR_)
    {
      cpu_pic->register_bank = &cpu_pic->registers[(value.get() & rp_mask) << 2];
    }

}


//--------------------------------------------------
// get
//unsigned int Status_register::get()

//--------------------------------------------------
// put_Z

//void Status_register::put_Z(unsigned int new_z)

//--------------------------------------------------
// get_Z
//unsigned int Status_register::get_Z()
//--------------------------------------------------
// put_C

//void Status_register::put_C(unsigned int new_c)

//--------------------------------------------------
// get_C
//unsigned int Status_register::get_C()

//--------------------------------------------------
// put_Z_C_DC

//--------------------------------------------------
// member functions for the INDF class
//--------------------------------------------------
INDF::INDF(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc)
{
  fsr_mask = 0x7f;           // assume a 14bit core
  base_address_mask1 = 0;    //   "          "
  base_address_mask2 = 0xff; //   "          "
}

void INDF::initialize()
{

  switch(cpu_pic->base_isa()) {

  case _12BIT_PROCESSOR_:
    fsr_mask = 0x1f;
    base_address_mask1 = 0x0;
    base_address_mask2 = 0x1f;

    break;

  case _14BIT_PROCESSOR_:
    fsr_mask = 0x7f;
    break;

  case _PIC17_PROCESSOR_:
  case _PIC18_PROCESSOR_:
    cout << "BUG: INDF::"<<__FUNCTION__<<". 16bit core uses a different class for indf.";
    break;
  default:
    cout << " BUG - invalid processor type INDF::initialize\n";
  }
    

}
void INDF::put(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());
  int reg = (cpu_pic->fsr->get_value() + //cpu_pic->fsr->value + 
	     ((cpu_pic->status->value.get() & base_address_mask1)<<1) ) &  base_address_mask2;

  // if the fsr is 0x00 or 0x80, then it points to the indf
  if(reg & fsr_mask){
    cpu_pic->registers[reg]->put(new_value);

    //(cpu_pic->fsr->value & base_address_mask2) + ((cpu_pic->status->value & base_address_mask1)<<1)
  }

}

void INDF::put_value(unsigned int new_value)
{

  // go ahead and use the regular put to write the data.
  // note that this is a 'virtual' function. Consequently,
  // all objects derived from a file_register should
  // automagically be correctly updated (which isn't
  // necessarily true if we just write new_value on top
  // of the current register value).

  put(new_value);

  update();
  int r = (cpu_pic->fsr->get_value() + //cpu_pic->fsr->value + 
	   (((cpu_pic->status->value.get() & base_address_mask1)<<1)& base_address_mask2));
  if(r & fsr_mask) 
    cpu_pic->registers[r]->update();

}


unsigned int INDF::get()
{

  trace.raw(read_trace.get() | value.get());
  //trace.register_read(address,value.get());
  int reg = (cpu_pic->fsr->get_value() +
	     ((cpu_pic->status->value.get() & base_address_mask1)<<1) ) &  base_address_mask2;
  if(reg & fsr_mask)
    return(cpu_pic->registers[reg]->get());
  else
    return(0);   // avoid infinite loop if fsr points to the indf
}

unsigned int INDF::get_value()
{
  int reg = (cpu_pic->fsr->get_value() +
	       ((cpu_pic->status->value.get() & base_address_mask1)<<1) ) &  base_address_mask2;
  if(reg & fsr_mask)
    return(cpu_pic->registers[reg]->get_value());
  else
    return(0);   // avoid infinite loop if fsr points to the indf
}



//--------------------------------------------------
// member functions for the PCL base class
//--------------------------------------------------
PCL::PCL(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc)
{
  por_value = RegisterValue(0,0);
}

// %%% FIX ME %%% breaks are different
void PCL::put(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  cpu_pic->pc->computed_goto(new_value);
  //trace.register_write(address,value.get());
}

void PCL::put_value(unsigned int new_value)
{

  value.put(new_value & 0xff);
  cpu_pic->pc->put_value( (cpu_pic->pc->get_value() & 0xffffff00) | value.get());

  // The gui (if present) will be updated in the pc->put_value call.
}

unsigned int PCL::get()
{
  return((value.get()+1) & 0xff);
}

unsigned int PCL::get_value()
{
  value.put(cpu_pic->pc->get_value() & 0xff);
  return(value.get());
}
//------------------------------------------------------------
// PCL reset
// 
void PCL::reset(RESET_TYPE r)
{
  trace.raw(write_trace.get() | value.get());
  putRV_notrace(por_value);
}

//--------------------------------------------------
// member functions for the PCLATH base class
//--------------------------------------------------

PCLATH::PCLATH(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc)
{
}

void PCLATH::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());
  value.put(new_value & PCLATH_MASK);
}

void PCLATH::put_value(unsigned int new_value)
{
  cout << "PCLATH::put_value(" << new_value << ")\n";
  value.put(new_value & PCLATH_MASK);

  // RP - I cannot think of a single possible reason I'd want to affect the real PC here!
  //  cpu_pic->pc->put_value( (cpu_pic->pc->get_value() & 0xffff00ff) | (value.get()<<8) );

  // The gui (if present) will be updated in the pc->put_value call.
}

unsigned int PCLATH::get()
{
  trace.raw(read_trace.get() | value.get());
  //trace.register_read(address,value.get());
  return(value.get() & PCLATH_MASK);
}

//--------------------------------------------------
// member functions for the PCON base class
//--------------------------------------------------
//
PCON::PCON(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc)
{
  valid_bits = BOR | POR;
}

void PCON::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());
  value.put(new_value&valid_bits);
}


//--------------------------------------------------
Stack::Stack()
{

  stack_warnings_flag = 0;   // Do not display over/under flow stack warnings
  break_on_overflow = 0;     // Do not break if the stack over flows
  break_on_underflow = 0;    // Do not break if the stack under flows
  stack_mask = 7;            // Assume a 14 bit core.
  pointer = 0;

  for(int i=0; i<8; i++)
    contents[i] = 0;

}

//
// Stack::push
//
// push the passed address onto the stack by storing it at the current
// 

void Stack::push(unsigned int address)
{
  Dprintf(("pointer=%d address0x%x\n",pointer,address));
  // Write the address at the current point location. Note that the '& stack_mask'
  // implicitly handles the stack wrap around.

  contents[pointer & stack_mask] = address;

  // If the stack pointer is too big, then the stack has definitely over flowed.
  // However, some pic programs take advantage of this 'feature', so provide a means
  // for them to ignore the warnings.

  if(pointer++ >= (int)stack_mask) {
    if(stack_warnings_flag || break_on_overflow)
      cout << "stack overflow ";
    if(break_on_overflow)
      bp.halt();
  }

}

//
// Stack::pop
//

unsigned int Stack::pop()
{

  // First decrement the stack pointer.

  if(--pointer < 0)  {
    if(stack_warnings_flag || break_on_underflow)
      cout << "stack underflow ";
    if(break_on_underflow) 
      bp.halt();
  }


  Dprintf(("pointer=%d address0x%x\n",pointer,contents[pointer & stack_mask]));

  return(contents[pointer & stack_mask]);
}

//
//  bool Stack::set_break_on_overflow(bool clear_or_set)
//
//  Set or clear the break on overflow flag


bool Stack::set_break_on_overflow(bool clear_or_set)
{
  if(break_on_overflow == clear_or_set)
    return 0;

  break_on_overflow = clear_or_set;

  return 1;

}

//
//  bool Stack::set_break_on_underflow(bool clear_or_set)
//
//  Set or clear the break on underflow flag


bool Stack::set_break_on_underflow(bool clear_or_set)
{
  if(break_on_underflow == clear_or_set)
    return 0;

  break_on_underflow = clear_or_set;

  return 1;

}


//========================================================================
class WReadTraceObject : public RegisterReadTraceObject
{
public:
  WReadTraceObject(Processor *_cpu, RegisterValue trv);
  virtual void print(FILE *fp);
};

class WWriteTraceObject : public RegisterWriteTraceObject
{
public:
  WWriteTraceObject(Processor *_cpu, RegisterValue trv);
  virtual void print(FILE *fp);
};


class WTraceType : public ProcessorTraceType
{
public:
  WTraceType(Processor *_cpu, 
	     unsigned int s)
    : ProcessorTraceType(_cpu,s,"W reg")
  {}

  TraceObject *decode(unsigned int tbi);
};



//========================================================================
WWriteTraceObject::WWriteTraceObject(Processor *_cpu, RegisterValue trv) 
  : RegisterWriteTraceObject(_cpu,0,trv)
{
  pic_processor *pcpu = dynamic_cast<pic_processor *>(cpu);

  if(pcpu) {
    to = cpu_pic->W->trace_state;
    cpu_pic->W->trace_state = from;
  }

}

void WWriteTraceObject::print(FILE *fp)
{
  char sFrom[16];
  char sTo[16];

  fprintf(fp, "  Wrote: 0x%s to W was 0x%s\n",
	  to.toString(sTo,sizeof(sTo)),
	  from.toString(sFrom,sizeof(sFrom)));

}

//========================================================================
WReadTraceObject::WReadTraceObject(Processor *_cpu, RegisterValue trv) 
  : RegisterReadTraceObject(_cpu,0,trv)
{
  pic_processor *pcpu = dynamic_cast<pic_processor *>(cpu);

  if(pcpu) {
    to = cpu_pic->W->trace_state;
    cpu_pic->W->trace_state = from;
  }

}

void WReadTraceObject::print(FILE *fp)
{
  char sFrom[16];

  fprintf(fp, "  Read: 0x%s from W\n",
	  from.toString(sFrom,sizeof(sFrom)));

}

//========================================================================
TraceObject * WTraceType::decode(unsigned int tbi)
{

  unsigned int tv = trace.get(tbi);

  RegisterValue rv = RegisterValue(tv & 0xff,0);
  TraceObject *wto;

  if (tv & (1<<22)) 
    wto = new WReadTraceObject(cpu, rv);
  else
    wto = new WWriteTraceObject(cpu, rv);

  return wto;
}


WREG::WREG(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu,pName,pDesc)
{
  if(cpu) {
    unsigned int trace_command = trace.allocateTraceType(m_tt = new WTraceType(get_cpu(),1));
    RegisterValue rv(trace_command+(0<<22), trace_command+(2<<22));
    set_write_trace(rv);
    rv = RegisterValue(trace_command+(1<<22), trace_command+(3<<22));
    set_read_trace (rv);
  }

}

WREG::~WREG()
{
  delete m_tt;
}


void WPU::put(unsigned int new_value)
{
    unsigned int masked_value = new_value & bit_mask;
    int i;

    trace.raw(write_trace.get() | value.get());

    value.put(masked_value);
    for(i = 0; i < 8; i++)
    {
	if((1<<i) & bit_mask)
	{
	    (&(*wpu_gpio)[i])->getPin().update_pullup((((1<<i) & masked_value) && wpu_pu )? '1' : '0', true);
	}
    }
}
/*
 * enable/disable all WPU  pullups
 */
void WPU::set_wpu_pu(bool pullup_enable)
{
    if (pullup_enable != wpu_pu)
    {
	wpu_pu = pullup_enable;
        put(value.get());	// change pull-ups based on value of WPU and gpio_pu
    }
}

