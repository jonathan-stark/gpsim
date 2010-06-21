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
#include "16bit-registers.h"
#include "16bit-processors.h"
#include "interface.h"
#include "stimuli.h"

#include "clock_phase.h"

//--------------------------------------------------
// member functions for the BSR class
//--------------------------------------------------
//
BSR::BSR(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu,pName,pDesc),
    register_page_bits(0)
{
}

void  BSR::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());

  value.put(new_value & 0x0f);
  cpu_pic->register_bank = &cpu_pic->registers[ value.get() << 8 ];


}

void  BSR::put_value(unsigned int new_value)
{
  put(new_value);

  update();
  cpu16->indf->update();
}


//--------------------------------------------------
// member functions for the FSR class
//--------------------------------------------------
//
FSRL::FSRL(Processor *pCpu, const char *pName, const char *pDesc, Indirect_Addressing *pIAM)
  : sfr_register(pCpu,pName,pDesc),
    iam(pIAM)
{
}

void  FSRL::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());
  value.put(new_value & 0xff);
  //  iam->fsr_delta = 0;
  iam->update_fsr_value();
}

void  FSRL::put_value(unsigned int new_value)
{

  put(new_value);

  update();
  cpu16->indf->update();

}

FSRH::FSRH(Processor *pCpu, const char *pName, const char *pDesc, Indirect_Addressing *pIAM)
  : sfr_register(pCpu,pName,pDesc),
    iam(pIAM)
{
}

void  FSRH::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  value.put(new_value & 0x0f);

  iam->update_fsr_value();

}

void  FSRH::put_value(unsigned int new_value)
{

  put(new_value);

  update();
  cpu16->indf->update();
}

INDF16::INDF16(Processor *pCpu, const char *pName, const char *pDesc, Indirect_Addressing *pIAM)
  : sfr_register(pCpu,pName,pDesc),
    iam(pIAM)
{
}

void INDF16::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());

  iam->fsr_value += iam->fsr_delta;
  iam->fsr_delta = 0;

  iam->put(new_value);

}

void INDF16::put_value(unsigned int new_value)
{
  put(new_value);
  update();
}

unsigned int INDF16::get()
{

  trace.raw(read_trace.get() | value.get());
  iam->fsr_value += iam->fsr_delta;
  iam->fsr_delta = 0;

  return(iam->get());
}

unsigned int INDF16::get_value()
{
  return(iam->get_value());
}

//------------------------------------------------
// PREINC
PREINC::PREINC(Processor *pCpu, const char *pName, const char *pDesc, Indirect_Addressing *pIAM)
  : sfr_register(pCpu,pName,pDesc),
    iam(pIAM)
{
}

unsigned int PREINC::get()
{

  trace.raw(read_trace.get() | value.get());
  //trace.register_read(address,value.get());
  iam->preinc_fsr_value();

  return(iam->get());
}

unsigned int PREINC::get_value()
{
  return(iam->get_value());
}

void PREINC::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,new_value);

  iam->preinc_fsr_value();
  iam->put(new_value);
}

void PREINC::put_value(unsigned int new_value)
{
  put(new_value);

  update();

}

//------------------------------------------------
// POSTINC
POSTINC::POSTINC(Processor *pCpu, const char *pName, const char *pDesc, Indirect_Addressing *pIAM)
  : sfr_register(pCpu,pName,pDesc),
    iam(pIAM)
{
}

unsigned int POSTINC::get()
{

  trace.raw(read_trace.get() | value.get());
  iam->postinc_fsr_value();

  return(iam->get());
}

unsigned int POSTINC::get_value()
{
  return(iam->get_value());
}

void POSTINC::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());

  iam->postinc_fsr_value();
  iam->put(new_value);
}

void POSTINC::put_value(unsigned int new_value)
{
  put(new_value);

  update();


}


//------------------------------------------------
// POSTDEC
POSTDEC::POSTDEC(Processor *pCpu, const char *pName, const char *pDesc, Indirect_Addressing *pIAM)
  : sfr_register(pCpu,pName,pDesc),
    iam(pIAM)
{
}
unsigned int POSTDEC::get()
{
  trace.raw(read_trace.get() | value.get());
  iam->postdec_fsr_value();

  return(iam->get());
}

unsigned int POSTDEC::get_value()
{
  return(iam->get_value());
}

void POSTDEC::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());

  iam->postdec_fsr_value();
  iam->put(new_value);
}

void POSTDEC::put_value(unsigned int new_value)
{
  put(new_value);

  update();
}


//------------------------------------------------
// PLUSW
PLUSW::PLUSW(Processor *pCpu, const char *pName, const char *pDesc, Indirect_Addressing *pIAM)
  : sfr_register(pCpu,pName,pDesc),
    iam(pIAM)
{
}

unsigned int PLUSW::get()
{
  trace.raw(read_trace.get() | value.get());

  int destination = iam->plusw_fsr_value();
  if(destination >= 0)
    return (cpu_pic->registers[destination]->get());
  else
    return 0;
}

unsigned int PLUSW::get_value()
{
  int destination = iam->plusw_fsr_value();
  if(destination >= 0)
    return (cpu_pic->registers[destination]->get_value());
  else
    return 0;
}

void PLUSW::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,new_value);

  int destination = iam->plusw_fsr_value();
  if(destination >= 0)
    cpu_pic->registers[destination]->put(new_value);
}

void PLUSW::put_value(unsigned int new_value)
{
  int destination = iam->plusw_fsr_value();
  if(destination >= 0)
    cpu_pic->registers[destination]->put_value(new_value);


  update();
  if(destination >= 0)
    cpu_pic->registers[destination]->update();
}

//------------------------------------------------

Indirect_Addressing::Indirect_Addressing(_16bit_processor *pCpu, const string &n)
  : fsrl(pCpu, (string("fsrl")+n).c_str(), "FSR Low", this),
    fsrh(pCpu, (string("fsrh")+n).c_str(), "FSR High", this),
    indf(pCpu, (string("indf")+n).c_str(), "Indirect Register", this),
    preinc(pCpu, (string("preinc")+n).c_str(), "Pre Increment Indirect", this),
    postinc(pCpu, (string("postinc")+n).c_str(), "Post Increment Indirect", this),
    postdec(pCpu, (string("postdec")+n).c_str(), "Post Decrement Indirect", this),
    plusw(pCpu, (string("plusw")+n).c_str(), "Literal Offset Indirect", this)
{
  /*
  fsrl.iam = this;
  fsrh.iam = this;
  indf.iam = this;
  preinc.iam = this;
  postinc.iam = this;
  postdec.iam = this;
  plusw.iam = this;
  */
  cpu = pCpu;

}

/*
 * put - Each of the indirect registers associated with this
 * indirect addressing class will call this routine to indirectly
 * write data.
 */
void Indirect_Addressing::put(unsigned int new_value)
{
  /*  unsigned int midbits;

  if( ((fsr_value & 0xfc7) == 0xfc3) || ((fsr_value & 0xfc4) == 0xfc4))
    {
      midbits = (fsr_value >> 3) & 0x7;
      if(midbits >= 3 && midbits <= 5)
	return;
    }
  */
  if(is_indirect_register(fsr_value))
    return;

  cpu_pic->registers[get_fsr_value()]->put(new_value);

}

/*
 * get - Each of the indirect registers associated with this
 * indirect addressing class will call this routine to indirectly
 * retrieve data.
 */
unsigned int Indirect_Addressing::get()
{
  //  unsigned int midbits;

  // See the comment in Indirect_Addressing::put about fsr address checking
  if(is_indirect_register(fsr_value))
    return 0;
  else
    /*
  if( ((fsr_value & 0xfc7) == 0xfc3) || ((fsr_value & 0xfc4) == 0xfc4))
    {
      midbits = (fsr_value >> 3) & 0x7;
      if(midbits >= 3 && midbits <= 5)
	return 0;
    }
    */

  return cpu_pic->registers[get_fsr_value()]->get();

}

/*
 * get - Each of the indirect registers associated with this
 * indirect addressing class will call this routine to indirectly
 * retrieve data.
 */
unsigned int Indirect_Addressing::get_value()
{
  /*
  unsigned int midbits;

  See the comment in Indirect_Addressing::put about fsr address checking

  if( ((fsr_value & 0xfc7) == 0xfc3) || ((fsr_value & 0xfc4) == 0xfc4))
    {
      midbits = (fsr_value >> 3) & 0x7;
      if(midbits >= 3 && midbits <= 5)
	return 0;
    }
  */
  if(is_indirect_register(fsr_value))
    return 0;
  else
    return cpu_pic->registers[get_fsr_value()]->get_value();

}

void Indirect_Addressing::put_fsr(unsigned int new_fsr)
{

  fsrl.put(new_fsr & 0xff);
  fsrh.put((new_fsr>>8) & 0x0f);

}


/*
 * update_fsr_value - This routine is called by the FSRL and FSRH
 * classes. It's purpose is to update the 16-bit (actually 12-bit)
 * address formed by the concatenation of FSRL and FSRH.
 *
 */

void Indirect_Addressing::update_fsr_value()
{

  if(current_cycle != get_cycles().get())
    {
      fsr_value = (fsrh.value.get() << 8) |  fsrl.value.get();
      fsr_delta = 0;
    }
}

/*
 * preinc_fsr_value - This member function pre-increments the current
 * fsr_value. If the preinc access is a read-modify-write instruction
 * (e.g. bcf preinc0,1 ) then the increment operation should occur
 * only once. 
 */

void Indirect_Addressing::preinc_fsr_value()
{

  if(current_cycle != get_cycles().get())
    {
      fsr_value += (fsr_delta+1);
      fsr_delta = 0;
      current_cycle = get_cycles().get();
      put_fsr(fsr_value);
    }

}

void Indirect_Addressing::postinc_fsr_value()
{

  if(current_cycle != get_cycles().get())
    {
      fsr_value += fsr_delta;
      fsr_delta = 1;
      current_cycle = get_cycles().get();
      put_fsr(fsr_value+1);
      
    }
}

void Indirect_Addressing::postdec_fsr_value()
{

  if(current_cycle != get_cycles().get())
    {
      fsr_value += fsr_delta;
      fsr_delta = -1;
      current_cycle = get_cycles().get();
      put_fsr(fsr_value-1);
      
    }

}

int Indirect_Addressing::plusw_fsr_value()
{

  fsr_value += fsr_delta;
  fsr_delta = 0;
  int signExtendedW = cpu_pic->W->value.get() | ((cpu_pic->W->value.get() > 127) ? 0xf00 : 0);
  unsigned int destination = (fsr_value + signExtendedW) & _16BIT_REGISTER_MASK;
  if(is_indirect_register(destination))
    return -1;
  else
    return destination;

}

//------------------------------------------------
void Fast_Stack::init(_16bit_processor *new_cpu)
{
  cpu = new_cpu;
}

void Fast_Stack::push()
{
  w = cpu->W->value.get();
  status = cpu->status->value.get();
  bsr = cpu->bsr.value.get();

}

void Fast_Stack::pop()
{
  //cout << "popping fast stack\n";
  cpu->W->put(w);
  cpu->status->put(status);
  cpu->bsr.put(bsr);

}
//--------------------------------------------------
// member functions for the PCL base class
//--------------------------------------------------
PCL16::PCL16(Processor *pCpu, const char *pName, const char *pDesc)
  : PCL(pCpu,pName,pDesc)
{
}


unsigned int PCL16::get()
{
  value.put(cpu_pic->pc->get_value() & 0xff);
  return((value.get()+2) & 0xff);
}

unsigned int PCL16::get_value()
{
  value.put(cpu_pic->pc->get_value() & 0xff);
  return((value.get()) & 0xff);

}


//--------------------------------------------------
// Program_Counter16
// The Program_Counter16 is almost identical to Program_Counter.
// The major difference is that the PC counts by 2 in the 16bit core.
Program_Counter16::Program_Counter16(Processor *pCpu)
  : Program_Counter("pc","Program Counter", pCpu)

{
  if(verbose)
    cout << "pc constructor 16\n";
}


//--------------------------------------------------
// computed_goto - update the program counter. Anytime the pcl register is written to
//                 by the source code we'll pass through here.
//
//

void Program_Counter16::computed_goto(unsigned int new_address)
{
//  cout << "Program_Counter16::computed_goto \n";

  trace.raw(trace_other | (value<<1));

  // Use the new_address and the cached pclath
  // to generate the destination address:


  value = ( (new_address | cpu_pic->get_pclath_branching_modpcl() )>>1);
  if (value >= memory_size)
	value -= memory_size;

  // see Update pcl comment in Program_Counter::increment()
  cpu_pic->pcl->value.put((value<<1) & 0xff);

  // The instruction modifying the PCL will also increment the program counter.
  // So, pre-compensate the increment with a decrement:
  value--;

  // The computed goto is a 2-cycle operation. The first cycle occurs within 
  // the instruction (i.e. via the ::increment() method). The second cycle occurs
  // here:

  mCurrentPhase = mExecute1Cycle;
}

//--------------------------------------------------
// put_value - Change the program counter without affecting the cycle counter
//             (This is what's called if the user changes the pc.)

void Program_Counter16::put_value(unsigned int new_value)
{
  cout << "Program_Counter16::put_value 0x" << hex << new_value << '\n';

  trace.raw(trace_other | (value<<1));

  // RP - The new_value passed in is a byte address, but the Program_Counter16
  // class's internal value is a word address
  value = (new_value/2);
  if (value >= memory_size)
	value -= memory_size;

  cpu_pic->pcl->value.put(new_value & 0xfe);

// RP - removed these lines as setting the actual PC should not affect the latches
//  cpu_pic->pclath->value.put((new_value >> 8) & 0xff);
//  cpu16->pclatu.value.put((new_value >> 16) & 0xff);


  cpu_pic->pcl->update();
  cpu_pic->pclath->update();
  update();
}
//------------------------------------------------
// get_value()
//

unsigned int Program_Counter16::get_value()
{
  return value << 1;
}

//------------------------------------------------
// TOSL
TOSL::TOSL(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu,pName,pDesc)
{}

unsigned int TOSL::get()
{
  value.put(stack->get_tos() & 0xff);
  trace.raw(read_trace.get() | value.get());
  return(value.get());
}

unsigned int TOSL::get_value()
{
  value.put(stack->get_tos() & 0xff);
  return(value.get());
}

void TOSL::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  stack->put_tos( (stack->get_tos() & 0xffffff00) | (new_value & 0xff));
}

void TOSL::put_value(unsigned int new_value)
{
  stack->put_tos( (stack->get_tos() & 0xffffff00) | (new_value & 0xff));
  update();
}


//------------------------------------------------
// TOSH
TOSH::TOSH(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu,pName,pDesc)
{}

unsigned int TOSH::get()
{
  value.put((stack->get_tos() >> 8) & 0xff);
  trace.raw(read_trace.get() | value.get());
  return(value.get());

}

unsigned int TOSH::get_value()
{

  value.put((stack->get_tos() >> 8) & 0xff);
  return(value.get());

}

void TOSH::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  stack->put_tos( (stack->get_tos() & 0xffff00ff) | ( (new_value & 0xff) << 8));
}

void TOSH::put_value(unsigned int new_value)
{

  stack->put_tos( (stack->get_tos() & 0xffff00ff) | ( (new_value & 0xff) << 8));

  update();

}


//------------------------------------------------
// TOSU
TOSU::TOSU(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu,pName,pDesc)
{}
unsigned int TOSU::get()
{
  value.put((stack->get_tos() >> 16) & 0x1f);
  trace.raw(read_trace.get() | value.get());
  return(value.get());

}

unsigned int TOSU::get_value()
{

  value.put((stack->get_tos() >> 16) & 0x1f);
  return(value.get());

}

void TOSU::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  stack->put_tos( (stack->get_tos() & 0xffe0ffff) | ( (new_value & 0x1f) << 16));
}

void TOSU::put_value(unsigned int new_value)
{
  stack->put_tos( (stack->get_tos() & 0xffe0ffff) | ( (new_value & 0x1f) << 16));
  update();
}


//------------------------------------------------
// STKPTR
STKPTR::STKPTR(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu,pName,pDesc)
{}
void STKPTR::put_value(unsigned int new_value)
{
  value.put(new_value);
  update();
}


//--------------------------------------------------
//
Stack16::Stack16(Processor *pCpu)
  : stkptr(pCpu, "stkptr"),
    tosl(pCpu, "tosl", "Top of Stack low byte"),
    tosh(pCpu, "tosh", "Top of Stack high byte"),
    tosu(pCpu, "tosu", "Top of Stack upper byte")
{
  stkptr.stack = this;
  tosl.stack = this;
  tosh.stack = this;
  tosu.stack = this;

}

void Stack16::push(unsigned int address)
{

  stkptr.value.put(1+stkptr.value.get());

  if((stkptr.value.get() & Stack16_MASK) == 0)
    {
      // check the STVREN bit
      // if(STVREN) {reset(stack_over_flow); return;}
      stkptr.value.put( stkptr.value.get() | 0x9f);
    }

  // Push 21-bit address onto the stack

  contents[stkptr.value.get() & Stack16_MASK] = address << 1;

  stkptr.value.put(stkptr.value.get() & 0xdf);

}

unsigned int Stack16::pop()
{
  if(stkptr.value.get() & Stack16_MASK)
    {
      // read 21-bit address from stack
      unsigned int ret = (contents[stkptr.value.get() & Stack16_MASK]) >> 1;
      stkptr.value.put(stkptr.value.get()-1);
      stkptr.value.put(stkptr.value.get() & 0x5f);
      return(ret);

    }
    // return(contents[ (--stkptr.value) & Stack16_MASK]);
  else
    {
      // check the STVREN bit
      // if(STVREN) {reset(stack_over_flow); return;}
      stkptr.value.put(0x40); // don't decrement past 0, signalize STKUNF
      return(0); // return 0 if underflow
    }
}

void Stack16::reset()
{
  stkptr.value.put( 0);
}

unsigned int Stack16::get_tos()
{

  return (contents[stkptr.value.get() & Stack16_MASK]);

}

void Stack16::put_tos(unsigned int new_tos)
{

  contents[stkptr.value.get() & Stack16_MASK] = new_tos;

}

//--------------------------------------------------
// member functions for the RCON base class
//--------------------------------------------------
RCON::RCON(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu,pName,pDesc)
{
}
//--------------------------------------------------
// member functions for the CPUSTA base class
//--------------------------------------------------
CPUSTA::CPUSTA(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu,pName,pDesc)
{
}
//--------------------------------------------------
// member functions for the T0CON base class
//--------------------------------------------------
T0CON::T0CON(Processor *pCpu, const char *pName, const char *pDesc)
  : OPTION_REG(pCpu,pName,pDesc)
{
  por_value = RegisterValue(0xff,0);
  wdtr_value = RegisterValue(0xff,0);
}

void T0CON::put(unsigned int new_value)
{

  unsigned int old_value = value.get();

  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());
  value.put(new_value);

  if( (value.get() ^ old_value) & (T08BIT | TMR0ON)) {
    cpu16->option_new_bits_6_7(value.get() & (BIT6 | BIT7));

    if(value.get() & TMR0ON) {
      unsigned int initialTmr0value = (cpu16->tmr0l.value.get() & 0xff) |
	( ((value.get() & T08BIT)==0) ? ((cpu16->tmr0l.value.get() & 0xff)<<8) : 0);
      cpu16->tmr0l.start(initialTmr0value);
    } else
      cpu16->tmr0l.stop();

  } 

  // First, check the tmr0 clock source bit to see if we are  changing from
  // internal to external (or vice versa) clocks.
  if( (value.get() ^ old_value) & T0CS)
    cpu16->tmr0l.new_clock_source();

  // %%%FIX ME%%% - can changing the state of TOSE cause the timer to
  // increment if tmr0 is being clocked by an external clock?

  // Now check the rest of the tmr0 bits.
  if( (value.get() ^ old_value) & (T0SE | PSA | PS2 | PS1 | PS0))
    cpu16->tmr0l.new_prescale();

  //cout <<"T0CON::put - new val 0x" << hex << value.get() <<'\n';
}

//--------------------------------------------------
void T0CON::initialize()
{
//    cpu16->tmr0l.new_prescale();
    cpu16->wdt.set_postscale( (value.get() & PSA) ? (value.get() & ( PS2 | PS1 | PS0 )) : 0);
    cpu16->option_new_bits_6_7(value.get() & (T0CS | BIT6 | BIT7));

}

//--------------------------------------------------

TMR0H::TMR0H(Processor *pCpu, const char *pName, const char *pDesc)
  :sfr_register(pCpu,pName,pDesc)
{
}

//--------------------------------------------------
void TMR0H::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  value.put(new_value & 0xff);
}

//--------------------------------------------------
void TMR0H::put_value(unsigned int new_value)
{
  value.put(new_value & 0xff);
}

unsigned int TMR0H::get()
{
  trace.raw(read_trace.get() | value.get());
  //trace.register_read(address,value.get());
  return(value.get());
}

unsigned int TMR0H::get_value()
{
  return(value.get());
}

//--------------------------------------------------
// TMR0_16 member functions
//
TMR0_16::TMR0_16(Processor *pCpu, const char *pName, const char *pDesc)
  : TMR0(pCpu,pName,pDesc),
    t0con(0), intcon(0), tmr0h(0), value16(0)
{
}
//--------------------------------------------------
// TMR0_16::get_prescale
//
//  If the prescaler is assigned to the WDT (and not TMR0)
//    then return 0
//  other wise
//    then return the Prescale select bits (plus 1)
//
unsigned int TMR0_16::get_prescale()
{
  if(t0con->value.get() & 0x8)
    return 0;
  else
    return ((t0con->value.get() & 7) + 1);

}

void TMR0_16::set_t0if()
{
  intcon->set_t0if();
}

bool TMR0_16::get_t0cs()
{
 return (t0con->value.get() & 0x20) != 0;
}

void TMR0_16::initialize()
{
  t0con = &cpu16->t0con;
  intcon = &cpu16->intcon;
  tmr0h  = &cpu16->tmr0h;
}

unsigned int TMR0_16::max_counts()
{

  if(t0con->value.get() & T0CON::T08BIT)
    return 0x100;
  else
    return 0x10000;

}
void TMR0_16::start(int restart_value, int sync)
{
  m_pOptionReg = t0con;
  TMR0::start(restart_value, sync);
}

void TMR0_16::put_value(unsigned int new_value)
{
  value.put(new_value & 0xff);
  value16 = (new_value & 0xff) | (tmr0h ? (tmr0h->get_value()<<8)  : 0);
  if(t0con->value.get() & T0CON::TMR0ON) {
    if(t0con->value.get() & T0CON::T08BIT)
      TMR0::put_value(new_value);
    else
      start(value16);
  } else {
    // TMR0 is not enabled
  }
}


// %%%FIX ME%%% 
void TMR0_16::increment()
{
  //  cout << "_TMR0 increment because of external clock ";
  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());

  if(--prescale_counter == 0)
    {
      prescale_counter = prescale;

      if(t0con->value.get() & T0CON::T08BIT)
	{
	  if(value.get() == 255)
	    {
	      value.put(0);
	      set_t0if();
	    }
	  else
	    value.put(value.get()+1);
	}
      else
	{
	  if(value.get() == 255)
	    {
	      value.put(0);
	      if(tmr0h->value.get() == 255)
		{
		  tmr0h->put(0);
		  set_t0if();
		}
	      else
		tmr0h->value.put(tmr0h->value.get()+1);

	    }
	  else
	    {
	      value.put(value.get()+1);
	    }
	}

    }
  //  cout << value << '\n';
}


unsigned int TMR0_16::get_value()
{
  if(t0con->value.get() & T0CON::TMR0ON) {

    // If TMR0L:H is configured as an 8-bit timer, then treat as an 8-bit timer
    if(t0con->value.get() & T0CON::T08BIT) {
      if (tmr0h)
	tmr0h->put_value( (value16>>8)&0xff);

      return(TMR0::get_value());

    }
    value16 = (int) ((get_cycles().get() - last_cycle)/ prescale);

    value.put(value16 & 0xff);
  }
  return(value.get());
  
}

unsigned int TMR0_16::get()
{

  trace.raw(read_trace.get() | value.get());

  get_value();

  if(t0con->value.get() & T0CON::T08BIT)
    return value.get();

  // reading the low byte of tmr0 latches in the high byte.
  tmr0h->put_value((value16 >> 8)&0xff);
  return value.get();
}

void TMR0_16::callback()
{

  //cout<<"_TMR0 rollover: " << hex << cycles.value << '\n';
  if((t0con->value.get() & T0CON::TMR0ON) == 0) {
    cout << " tmr0 isn't turned on\n";
    return;
  }

  TMR0::callback();   // Let the parent class handle the lower eight bits

  //Now handle the upper 8 bits:

  if(future_cycle &&
     !(t0con->value.get() & T0CON::T08BIT)) 
    {
      // 16-bit mode
      tmr0h->put_value(0);
    }


}

void TMR0_16::callback_print()
{
  cout << "TMR0_16 " << name() << " CallBack ID " << CallBackID << '\n';
}

void TMR0_16::sleep()
{
   if (verbose)
	cout << "TMR0_16::sleep state=" << state << "\n";

    if((state & RUNNING))
    {
        TMR0::stop();
        state = SLEEPING;
    }

}
void TMR0_16::wake()
{
    if (verbose)
	cout << "TMR0_16::wake state=" << state << "\n";

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

//--------------------------------------------------
// T3CON
T3CON::T3CON(Processor *pCpu, const char *pName, const char *pDesc)
  : T1CON(pCpu,pName,pDesc),
    ccpr1l(0),ccpr2l(0),tmr1l(0)
{}

void T3CON::put(unsigned int new_value)
{
  int diff = (value.get() ^ new_value);

  if(diff & (T3CCP1 |  T3CCP2)) {
    switch(new_value & (T3CCP1 |  T3CCP2)) {
    case 0:
      ccpr1l->assign_tmr(tmr1l);   // Both CCP modules use TMR1 as their source
      ccpr2l->assign_tmr(tmr1l);
      break;
    case T3CCP1:
      ccpr1l->assign_tmr(tmr1l);   // CCP1 uses TMR1
      ccpr2l->assign_tmr(tmrl);    // CCP2 uses TMR3
      break;
    default:
      ccpr1l->assign_tmr(tmrl);    // Both CCP modules use TMR3 as their source
      ccpr2l->assign_tmr(tmrl);
    } 
  }

  // Let the T1CON class deal with everything else.
  T1CON::put(new_value);

}

//--------------------------------------------------
// TMR3_MODULE
//
// 

TMR3_MODULE::TMR3_MODULE()
{

  t3con = 0;
  pir_set = 0;

}

void TMR3_MODULE::initialize(T3CON *t3con_, PIR_SET *pir_set_)
{

  t3con = t3con_;
  pir_set  = pir_set_;

}


//-------------------------------------------------------------------
//
//  Table Reads and Writes
//
// The 18cxxx family provides a peripheral that will allow the program
// memory to read and write to itself. 
//
//-------------------------------------------------------------------

TBL_MODULE::TBL_MODULE(_16bit_processor *pCpu)
  : cpu(pCpu),
    tablat(pCpu,"tablat"),
    tabptrl(pCpu,"tabptrl"),
    tabptrh(pCpu,"tabptrh"),
    tabptru(pCpu,"tabptru")
{
}

//void TBL_MODULE::initialize(_16bit_processor *new_cpu)
//{
//  cpu = new_cpu;
//}

//-------------------------------------------------------------------
//  void TBL_MODULE::increment()
//
//  This function increments the 24-bit ptr that is formed by the
// concatenation of tabptrl,tabptrh, and tabptru. It is called by
// the TBLRD and TBLWT pic instructions when the auto-increment
// operand is specified (e.g. TBLWT *+ )
//
//
// Inputs:  none
// Outputs: none
//
//-------------------------------------------------------------------
void TBL_MODULE::increment()
{

  if(tabptrl.value.get() >= 0xff) {
    tabptrl.put(0);
    if(tabptrh.value.get() >= 0xff) {
      tabptrh.put(0);
      tabptru.put(tabptru.value.get() + 1);
    } else {
      tabptrh.put(tabptrh.value.get() + 1);
    }
  }
  else
    tabptrl.put(tabptrl.value.get() + 1);


}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
void TBL_MODULE::decrement()
{

  if(tabptrl.value.get() == 0) {
    tabptrl.put(0xff);
    if(tabptrh.value.get() == 0) {
      tabptrh.put(0xff);
      tabptru.put(tabptru.value.get() - 1);
    } else {
      tabptrh.put(tabptrh.value.get() - 1);
    }
  }
  else
    tabptrl.put(tabptrl.value.get() - 1);

}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
void TBL_MODULE::read()
{
  unsigned int tabptr,opcode;

  tabptr = 
    ( (tabptru.value.get() & 0xff) << 16 ) |
    ( (tabptrh.value.get() & 0xff) << 8 )  |
    ( (tabptrl.value.get() & 0xff) << 0 );

  opcode = cpu_pic->pma->get_rom(tabptr & 0xfffffe);

  if(tabptr & 1)
    {
      tablat.put((opcode >> 8) & 0xff);
      internal_latch = (internal_latch & 0x00ff) | (opcode & 0xff00);
    }
  else
    {
      tablat.put((opcode >> 0) & 0xff);
      internal_latch = (internal_latch & 0xff00) | (opcode & 0x00ff);
    }

}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
void TBL_MODULE::write()
{

  unsigned int tabptr;

  tabptr = 
    ( (tabptru.value.get() & 0xff) << 16 ) |
    ( (tabptrh.value.get() & 0xff) << 8 )  |
    ( (tabptrl.value.get() & 0xff) << 0 );

  if(tabptr & 1)
    {
      // Long write
      internal_latch = (internal_latch & 0x00ff) | ((tablat.value.get()<<8) & 0xff00);
      cpu_pic->pma->put_opcode_start(tabptr & 0xfffffe, internal_latch);
    }
  else
    {
      // Short Write
      internal_latch = (internal_latch & 0xff00) | (tablat.value.get() & 0x00ff);
    }


}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
LVDCON::LVDCON(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName,pDesc),
    valid_bits(0x3f)
{
}


#if 0
//-------------------------------------------------------------------
//
// PORTC16
//-------------------------------------------------------------------


PORTC16::PORTC16()
{
  usart = 0;
  ccp1con = 0;
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
unsigned int PORTC16::get()
{
  unsigned int old_value;

  old_value = value.get();

  IOPORT::get();

  int diff = old_value ^ value.get(); // The difference between old and new

  // 
  if( ccp1con && (diff & CCP1) ) {
    ccp1con->new_edge(value.get() & CCP1);
  }
  // if this cpu has a usart and there's been a change detected on
  // the RX pin, then we need to notify the usart
  if( usart && (diff & RX))
    usart->new_rx_edge(value.get() & RX);

  return(value.get());
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void PORTC16::setbit(unsigned int bit_number, bool new_value)
{

  unsigned int old_value = value.get();

  //cout << "PORTC16::setbit() bit " << bit_number << " to " << new_value << '\n';

  IOPORT::setbit( bit_number,  new_value);

  int diff = old_value ^ value.get(); // The difference between old and new

  if(ccp1con && ( diff & CCP1) ) {
    ccp1con->new_edge(value.get() & CCP1);
  }

  if( usart && (diff & RX))
    usart->new_rx_edge(value.get() & RX);

}

//-------------------------------------------------------------------
//  PORTC16::put(unsigned int new_value)
//
//  inputs:  new_value - here's where the I/O port is written (e.g.
//                       gpsim is executing a MOVWF IOPORT,F instruction.)
//  returns: none
//
//  The I/O Port is updated with the new value. If there are any stimuli
// attached to the I/O pins then they will be updated as well.
// This is the same as PIC_IOPORT::put(), but check for the transmit
// enable bit of the usart in addition to the TRISC register.
//
//-------------------------------------------------------------------

void PORTC16::put(unsigned int new_value)
{
  if(new_value > 255)
    cout << "PORTC16::put value >255\n";
  // The I/O Ports have an internal latch that holds the state of the last
  // write, even if the I/O pins are configured as inputs. If the tris port
  // changes an I/O pin from an input to an output, then the contents of this
  // internal latch will be placed onto the external I/O pin.

  internal_latch = new_value;

  // update only those bits that are really outputs
  int mack_value = tris->value.get();
  if (usart && usart->txsta.get() & _TXSTA::TXEN)
      mack_value |= 1 << 6;

  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());
  value.put((new_value & ~mack_value) | (value.get() & mack_value));

  // Update the stimuli - if there are any
  if(stimulus_mask)
    update_stimuli();

}

//-------------------------------------------------------------------
// PORTC16::update_pin_directions(unsigned int new_tris)
//
//  Whenever a new value is written to a tris register, then we need
// to update the stimuli associated with the I/O pins. This is true
// even if the I/O pin are not connected to external stimuli (like a
// square wave). An example scenario would be like changing a port b
// pin from an output that's driving low to an input. If there's no
// stimulus attached to the port b I/O pin then the pull up (if enabled)
// will pull the I/O pin high.
// This is the same as PIC_IOPORT::update_pin_directions(), but check for the
// transmit enable bit of the usart in addition to the TRISC register.
//
//-------------------------------------------------------------------
void PORTC16::update_pin_directions(unsigned int new_tris)
{

  if(!tris)
    return;

  unsigned int diff = tris->value.get() ^ new_tris;

  if (usart && usart->txsta.get() & _TXSTA::TXEN)
      diff &= ~(1 << 6); // don't update TX pin if TXEN

  if(diff)
    {
      // Update the I/O port value to that of the internal latch
      value.put((value.get() & ~diff) | (internal_latch & diff));

      // Go through and update the direction of the I/O pins
      int i,m;
      for(i = 0, m=1; i<num_iopins; i++, m <<= 1)
	if(m & diff & valid_iopins)
	  {
	  pins[i]->update_direction(m & (~new_tris));
	  //cout << __FUNCTION__ << " name " << pins[i]->name() << " pin number " << i << '\n';
	  }
      // Now, update the nodes to which the(se) pin(s) may be attached

      guint64 time = get_cycles().get();
      for(i = 0, m=1; i<num_iopins; i++, m <<= 1)
	if(stimulus_mask & m & diff)
          if(pins[i]->snode!=0)
            pins[i]->snode->update(time);
    }
}

#endif // 0
