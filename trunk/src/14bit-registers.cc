/*
   Copyright (C) 1998-2000 Scott Dattalo

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
#include <iostream.h>
#include <iomanip.h>


#include "../config.h"
#include "14bit-processors.h"
#include "interface.h"

#include <string>
#include "stimuli.h"

#include "xref.h"
#define PCLATH_MASK              0x1f


pic_processor *temp_cpu;

//--------------------------------------------------
// Member functions for the file_register base class
//--------------------------------------------------
//

file_register::file_register(void)
{
  cpu = NULL;
  name_str1 = NULL;
  new_name("file_register");
  xref = new XrefObject(&value);
}

file_register::~file_register(void)
{
  if(name_str1)
    delete(name_str1);

//  if(xref)
//    delete(xref);
}

//--------------------------------------------------
// get
//
//  Return the contents of the file register. If a
// 'break on read' break point is set, then set the
// global break point flag.

unsigned int file_register::get(void)
{
  trace.register_read(address,value);

  return(value);
}

//--------------------------------------------------
//
//  Update the contents of the file register. If a
// 'break on write' break point is set and this write
// meets the conditions for the break point then set the
// global break point flag.

void file_register::put(unsigned int new_value)
{

  value = new_value;
  trace.register_write(address,value);

}

void file_register::new_name(char *s)
{
  if(name_str1)
    delete name_str1;

  //name_str1 = new char[strlen(s)+1];
  //strcpy(name_str1,s);
  if(s)
    name_str1 = strdup(s);
  else
    name_str1 = NULL;

}

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

      if(cpu && address == cpu->fsr.value)
	{
	  if(cpu->indf.xref)
	    cpu->indf.xref->update();
	}
    }
}

//--------------------------------------------------
// member functions for the invalid_file_register class
//--------------------------------------------------
void invalid_file_register::put(unsigned int new_value)
{
  cout << "attempt write to invalid file register: address 0x" << 
    hex << address<< ", value 0x" << new_value << '\n';

  bp.halt();
  trace.register_write(address,value);

  return;
}

unsigned int invalid_file_register::get(void)
{
  cout << "attempt read from invalid file register\n";

  //  if(break_point)
  //    bp.check_invalid_fr_break(this);

  trace.register_read(address,value);

  return(0);
}



//
//--------------------------------------------------
// member functions for the FSR class
//--------------------------------------------------
//
void  FSR::put(unsigned int new_value)
{
  value = new_value;
  trace.register_write(address,value);

  /* The 12-bit core selects the register page using the fsr */
  if(cpu->base_isa() == _12BIT_PROCESSOR_)
    {
      cpu->register_bank = &cpu->registers[ value & register_page_bits ];
    }


}

void  FSR::put_value(unsigned int new_value)
{

  put(new_value);

  //#ifdef HAVE_GUI

  if(xref)
  {
      xref->update();
      if(cpu->indf.xref)
	cpu->indf.xref->update();
  }

  //#endif


}

//
//--------------------------------------------------
// member functions for the Status_register class
//--------------------------------------------------
//

//--------------------------------------------------

Status_register::Status_register(void)
{
  break_point = 0;
  break_on_z =0;
  break_on_c =0;
  address = 3;
  rp_mask = RP_MASK;
  new_name("status");
}
//--------------------------------------------------
// put

void inline Status_register::put(unsigned int new_value)
{
  value = new_value; // & STATUS_WRITABLE_BITS;

  if(cpu->base_isa() == _14BIT_PROCESSOR_)
    {
      cpu->register_bank = &cpu->registers[(value & rp_mask) << 2];
    }

  trace.register_write(address,value);
}


//--------------------------------------------------
// get
//unsigned int Status_register::get(void)

//--------------------------------------------------
// put_Z

//void Status_register::put_Z(unsigned int new_z)

//--------------------------------------------------
// get_Z
//unsigned int Status_register::get_Z(void)
//--------------------------------------------------
// put_C

//void Status_register::put_C(unsigned int new_c)

//--------------------------------------------------
// get_C
//unsigned int Status_register::get_C(void)

//--------------------------------------------------
// put_Z_C_DC

//--------------------------------------------------
// member functions for the INDF class
//--------------------------------------------------
INDF::INDF(void)
{
  break_point = 0;
  fsr_mask = 0x7f;       // assume a 14bit core
  base_address_mask = 0; //   "          "
  new_name("indf");
}

void INDF::initialize(void)
{

  switch(cpu->base_isa()) {

  _12BIT_PROCESSOR_:
    fsr_mask = 0x1f;
    break;

  _14BIT_PROCESSOR_:
    fsr_mask = 0x7f;
    break;

  _16BIT_PROCESSOR_:
    cout << "BUG: INDF::"<<__FUNCTION__<<". 16bit core uses a different class for indf.";
    break;
  }
    

}
void INDF::put(unsigned int new_value)
{

  trace.register_write(address,value);

  // if the fsr is 0x00 or 0x80, then it points to the indf
  if(cpu->fsr.value & fsr_mask)
    cpu->registers[cpu->fsr.value+ ((cpu->status.value & base_address_mask)<<1) ]->put(new_value);

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

  if(xref)
    {
      xref->update();
      if(cpu->fsr.value & fsr_mask) 
	{
	  int r = cpu->fsr.value + ((cpu->status.value & base_address_mask)<<1);
	  if(cpu->registers[r]->xref)
	    cpu->registers[r]->xref->update();
	}
    }

}


unsigned int INDF::get(void)
{

  trace.register_read(address,value);

  if(cpu->fsr.value & fsr_mask)
    return(cpu->registers[cpu->fsr.value + ((cpu->status.value & base_address_mask)<<1)]->get());
  else
    return(0);   // avoid infinite loop if fsr points to the indf
}

unsigned int INDF::get_value(void)
{

  if(cpu->fsr.value & fsr_mask)
    return(cpu->registers[cpu->fsr.value+ ((cpu->status.value & base_address_mask)<<1)]->get_value());
  else
    return(0);   // avoid infinite loop if fsr points to the indf
}

//--------------------------------------------------
// member functions for the OPTION base class
//--------------------------------------------------
OPTION_REG::OPTION_REG(void)
{
  break_point = 0;
  por_value = 0xff;
  wdtr_value = 0xff;
  new_name("option");
}

void OPTION_REG::put(unsigned int new_value)
{

  unsigned int old_value = value;
  value = new_value;

  // First, check the tmr0 clock source bit to see if we are  changing from
  // internal to external (or vice versa) clocks.
  if( (value ^ old_value) & T0CS)
    cpu->tmr0.new_clock_source();

  // %%%FIX ME%%% - can changing the state of TOSE cause the timer to
  // increment if tmr0 is being clocked by an external clock?

  // Now check the rest of the tmr0 bits.
  if( (value ^ old_value) & (T0SE | PSA | PS2 | PS1 | PS0))
    cpu->tmr0.new_prescale();

  if( (value ^ old_value) & (BIT6 | BIT7))
    cpu->option_new_bits_6_7(value & (BIT6 | BIT7));

  if(cpu->base_isa() == _14BIT_PROCESSOR_)
    trace.register_write(address,value);
  else
    trace.write_OPTION(value);

}



//--------------------------------------------------
// member functions for the INTCON base class
//--------------------------------------------------
INTCON::INTCON(void)
{
  break_point = 0;
  new_name("intcon");
}

void INTCON::set_T0IF(void)
{

  value |= T0IF;

  trace.register_write(address,value);

  if (value & (GIE | T0IE))
  {
    trace.interrupt();
  }
}


//--------------------------------------------------
// member functions for the PCL base class
//--------------------------------------------------
PCL::PCL(void) : sfr_register()
{
  break_point = 0;
  new_name("pcl");
}

// %%% FIX ME %%% breaks are different
void PCL::put(unsigned int new_value)
{
  //  if(break_point) 
  //  {
  //    if(bp.check_write_break(this))
  //      cpu->pc.computed_goto(new_value);
  //  }
  //  else
    cpu->pc.computed_goto(new_value);

  trace.register_write(address,value);
}

void PCL::put_value(unsigned int new_value)
{

  value = new_value;
  cpu->pc.put_value( (cpu->pc.value & 0xffffff00) | value);

  // The gui (if present) will be updated in the pc.put_value call.
}

unsigned int PCL::get(void)
{
  return((value+1) & 0xff);
}
//--------------------------------------------------
// member functions for the PCLATH base class
//--------------------------------------------------

PCLATH::PCLATH(void)
{
  break_point = 0;
  new_name("pclath");
}

void PCLATH::put(unsigned int new_value)
{
  //  if(break_point) 
  //    bp.check_write_break(this);

  value = new_value & PCLATH_MASK;

  trace.register_write(address,value);
}

void PCLATH::put_value(unsigned int new_value)
{

  value = new_value & PCLATH_MASK;
  cpu->pc.put_value( (cpu->pc.value & 0xffff00ff) | (value<<8) );

  // The gui (if present) will be updated in the pc.put_value call.
}

unsigned int PCLATH::get(void)
{
  //  if(break_point) 
  //    bp.check_read_break(this);

  trace.register_read(address,value);
  return(value & PCLATH_MASK);
}

//--------------------------------------------------
// member functions for the Program_Counter base class
//--------------------------------------------------
//

//--------------------------------------------------

Program_Counter::Program_Counter(void)
{
  if(verbose)
    cout << "pc constructor\n";
  value = 0;

  xref = new XrefObject(&value);


}

//--------------------------------------------------
// increment - update the program counter. All non-branching instructions pass through here.
//   

void Program_Counter::increment(void)
{

  value = (value + 1) & memory_size_mask;
  trace.program_counter(value);

  // Update pcl. Note that we don't want to pcl.put() because that 
  // will trigger a break point if there's one set on pcl. (A read/write
  // break point on pcl should not be triggered by advancing the program
  // counter).

  cpu->pcl.value = value & 0xff;
  cpu->cycles.increment();
}

//--------------------------------------------------
// skip - Does the same thing that increment does, except that it records the operation
// in the trace buffer as a 'skip' instead of a 'pc update'.
//   

void Program_Counter::skip(void)
{

  trace.cycle_increment();
  value = (value + 1) & memory_size_mask;
  trace.pc_skip(value);


  // Update pcl. Note that we don't want to pcl.put() because that 
  // will trigger a break point if there's one set on pcl. (A read/write
  // break point on pcl should not be triggered by advancing the program
  // counter).

  cpu->pcl.value = value & 0xff;
  cpu->cycles.increment();
}

//--------------------------------------------------
// jump - update the program counter. All branching instructions except computed gotos
//        and returns go through here.

void Program_Counter::jump(unsigned int new_address)
{

  // Use the new_address and the cached pclath (or page select bits for 12 bit cores)
  // to generate the destination address:

  value = (new_address | cpu->get_pclath_branching_jump() ) & memory_size_mask;

  trace.cycle_increment();
  trace.program_counter(value);

  cpu->pcl.value = value & 0xff;    // see Update pcl comment in Program_Counter::increment()

  cpu->cycles.increment();
  cpu->cycles.increment();
}

//--------------------------------------------------
// computed_goto - update the program counter. Anytime the pcl register is written to
//                 by the source code we'll pass through here.
//

void Program_Counter::computed_goto(unsigned int new_address)
{

  // Use the new_address and the cached pclath (or page select bits for 12 bit cores)
  // to generate the destination address:

  value = (new_address | cpu->get_pclath_branching_modpcl() ) & memory_size_mask;

  trace.cycle_increment();
  trace.program_counter(value);

  cpu->pcl.value = value & 0xff;    // see Update pcl comment in Program_Counter::increment()

  // The instruction modifying the PCL will also increment the program counter. So, pre-compensate
  // the increment with a decrement:
  value--;
  cpu->cycles.increment();
}

//--------------------------------------------------
// new_address - write a new value to the program counter. All returns pass through here.
//

void Program_Counter::new_address(unsigned int new_value)
{
  value = new_value & memory_size_mask;
  trace.cycle_increment();
  trace.program_counter(value);

  cpu->pcl.value = value & 0xff;    // see Update pcl comment in Program_Counter::increment()
  cpu->cycles.increment();
  cpu->cycles.increment();
}

//--------------------------------------------------
// get_next - get the next address that is just pass the current one
//            (used by 'call' to obtain the return address)

unsigned int Program_Counter::get_next(void)
{

  return( (value + cpu->program_memory[value]->instruction_size()) & memory_size_mask);

}


//--------------------------------------------------
// put_value - Change the program counter without affecting the cycle counter
//             (This is what's called if the user changes the pc.)

void Program_Counter::put_value(unsigned int new_value)
{
  value = new_value & memory_size_mask;
  cpu->pcl.value = value & 0xff;
  cpu->pclath.value = (new_value >> 8) & PCLATH_MASK;

  if(xref)
    {
      if(cpu->pcl.xref)
	cpu->pcl.xref->update();
      if(cpu->pclath.xref)
	cpu->pclath.xref->update();
	xref->update();
    }
  

}


//--------------------------------------------------
Stack::Stack(void)
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

  // Write the address at the current point location. Note that the '& stack_mask'
  // implicitly handles the stack wrap around.

  contents[pointer & stack_mask] = address;

  // If the stack pointer is too big, then the stack has definitely over flowed.
  // However, some pic programs take advantage of this 'feature', so provide a means
  // for them to ignore the warnings.

  if(pointer++ >= stack_mask) {
    if(stack_warnings_flag || break_on_overflow)
      cout << "stack overflow ";
    if(break_on_overflow)
      bp.halt();
  }

}

//
// Stack::pop
//

unsigned int Stack::pop(void)
{

  // First decrement the stack pointer.

  if(--pointer < 0)  {
    if(stack_warnings_flag || break_on_underflow)
      cout << "stack underflow ";
    if(break_on_underflow) 
      bp.halt();
  }


  
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



//--------------------------------------------------
// WREG
//

unsigned int WREG::get(void)
{
  //  if(break_point) 
  //    bp.check_read_break(this);

  trace.read_W(value);

  return(value);
}

void WREG::put(unsigned int new_value)
{

  //  if(break_point) 
  //    bp.check_write_break(this);

  value = new_value;
  trace.write_W(value);

}

WREG::WREG(void)
{
  break_point = 0;
  new_name("W");
}


//----------------------------------------------------------
//
// EE PROM
//
// There are many conditions that need to be verified against a real part:
//    1) what happens if RD and WR are set at the same time?
//       > the simulator ignores both the read and the write.
//    2) what happens if a RD is initiated while data is being written?
//       > the simulator ignores the read
//    3) what happens if EEADR or EEDATA are changed while data is being written?
//       > the simulator will update these registers with the new values that
//         are written to them, however the write operation will be unaffected.
//    4) if WRERR is set, will this prevent a valid write sequence from being initiated?
//       > the simulator views WRERR as a status bit
//    5) if a RD is attempted after the eeprom has been prepared for a write
//       will this affect the RD or write?
//       > The simulator will proceed with the read and leave the write-enable state alone.
//    6) what happens if WREN goes low while a write is happening?
//       > The simulator will complete the write and WREN will be cleared.

void EECON1::put(unsigned int new_value)
{

  //  if(break_point) 
  //    bp.check_write_break(this);


  new_value &= valid_bits;
  
  //cout << "EECON1::put new_value " << new_value << "  valid_bits " << valid_bits << '\n';
  if(new_value & WREN)
    {
      if(eeprom->eecon2->eestate == EECON2::EEUNARMED)
	{
	  eeprom->eecon2->eestate = EECON2::EENOT_READY;
	  value |= WREN;

	}

      // WREN is true and EECON2 is armed (which means that we've passed through here
      // once before with WREN true). Initiate an eeprom write only if WR is true and
      // RD is false AND EECON2 is ready

      else if( (new_value & WR) && !(new_value & RD)  && (eeprom->eecon2->eestate == EECON2::EEREADY_FOR_WRITE))
	{
	  value |= WR;
	  eeprom->start_write();
	}

      //    else cout << "EECON1: write ignored " << new_value << "  (WREN is probably already set)\n";

    }
  else
    {
      // WREN is low so inhibit eeprom writes:

      eeprom->eecon2->eestate = EECON2::EEUNARMED;
      //cout << "EECON1: write is disabled\n";

    }

  //value = (new_value & ~(EEIF | WRERR | WREN)) | (value & (EEIF | WRERR | WREN)) ;
  value = (value & (RD | WR)) | new_value;

  if ( (value & RD) && !( value & WR) )
    {
      if(value & EEPGD) {
	eeprom->eecon2->eestate = EECON2::EEREAD;
	eeprom->start_program_memory_read();
	//cout << "eestate " << eeprom->eecon2->eestate << '\n';
	// read program memory
      } else {
	//eeprom->eedata->value = eeprom->rom[eeprom->eeadr->value]->get();
	eeprom->eecon2->eestate = EECON2::EEREAD;
	eeprom->callback();
	value &= ~RD;
      }
    }
  

  trace.register_write(address,value);

}

unsigned int EECON1::get(void)
{

  trace.register_read(address,value);

  return(value);
}

EECON1::EECON1(void)
{
  new_name("eecon1");
  valid_bits = EECON1_VALID_BITS;
}





void EECON2::put(unsigned int new_value)
{


  if( (eestate == EENOT_READY) && (0x55 == new_value))
    {
      eestate = EEHAVE_0x55;
    }
  else if ( (eestate == EEHAVE_0x55) && (0xaa == new_value))
    {
      eestate = EEREADY_FOR_WRITE;
    }
  else if ((eestate == EEHAVE_0x55) || (eestate == EEREADY_FOR_WRITE))
    {
      eestate == EENOT_READY;
    }

  trace.register_write(address,new_value);

}

unsigned int EECON2::get(void)
{

  trace.register_read(address,0);

  return(0);
}

EECON2::EECON2(void)
{
  new_name("eecon2");
  break_point = 0;
}





unsigned int EEDATA::get(void)
{
  trace.register_read(address,value);

  return(value);
}

void EEDATA::put(unsigned int new_value)
{

  value = new_value;
  trace.register_write(address,value);

}

EEDATA::EEDATA(void)
{
  new_name("eedata");
  break_point = 0;
}




unsigned int EEADR::get(void)
{

  trace.register_read(address,value);

  return(value);
}

void EEADR::put(unsigned int new_value)
{

  value = new_value;
  trace.register_write(address,value);

}


EEADR::EEADR(void)
{
  new_name("eeadr");
  break_point = 0;
}



// EEPROM - Peripheral
//
//  This object emulates the 14-bit core's EEPROM/FLASH peripheral (such as the 16c84).
//
//  It's main purpose is to provide a means by which the control registers may communicate.
// 

EEPROM::EEPROM(void)
{

  rom_size = 0;
  eecon1  = NULL;
  eecon2  = NULL;
  eedata  = NULL;
  eedatah = NULL;
  eeadr   = NULL;
  eeadrh  = NULL;


}


void EEPROM::start_write(void)
{
  //cout << "start the eewrite\n";
  cpu->cycles.set_break(cpu->cycles.value + EPROM_WRITE_TIME, this);

  wr_adr = eeadr->value;
  if(eeadrh)
    wr_adr = (eeadrh->value << 8) + wr_adr;

  wr_data = eedata->value;
  if(eedatah)
    wr_data = (eedatah->value << 8) + wr_data;

}

void EEPROM::start_program_memory_read(void)
{
  //cout << "start the program memory read\n";
  //cout << "eestate (in spmr) " << eecon2->eestate << '\n';


  rd_adr = eeadr->value;
  if(eeadrh)
    rd_adr = (eeadrh->value << 8) + rd_adr;

  cpu->cycles.set_break(cpu->cycles.value + 2, this);

}

void EEPROM::callback(void)
{
  //cout << "eeprom call back\n";
  
  unsigned int eecon1_value, i;

  eecon1_value = eecon1->get();

  switch(eecon2->eestate) {
  case EECON2::EEREAD:
    //cout << "eeread\n";
    eecon1->value = (eecon1_value  & (~eecon1->RD));
    eecon2->eestate = EECON2::EEUNARMED;
    if(eecon1->value & EECON1::EEPGD) {
      // read program memory
      
      i = eeadr->value;
      if(eeadrh)
	i |= (eeadrh->value << 8);

      i = cpu->pma.get_opcode(i);
      //cout << "read " << i << " from program memory\n";
      eedata->value = i & 0xff;
      if(eedatah)
	eedatah->value = (i>>8) & 0xff;

    } else {
      eedata->value = rom[eeadr->value]->get();
    }

    eecon1->value &= (~EECON1::RD);
    break;
  case EECON2::EEREADY_FOR_WRITE:
    //cout << "eewrite\n";

    if(wr_adr < rom_size)
      rom[wr_adr]->value = wr_data;
    else
      cout << "EEPROM wr_adr is out of range " << wr_adr << '\n';

    eecon1->value = (eecon1_value  & (~eecon1->WR)) | eecon1->EEIF;

    cpu->intcon->peripheral_interrupt();

    if (eecon1_value & eecon1->WREN)
      eecon2->eestate = EECON2::EENOT_READY;
    else
      eecon2->eestate = EECON2::EEUNARMED;
    break;

    eecon1->value &= (~EECON1::WR);
  default:
    cout << "EEPROM::callback() bad eeprom state " << eecon2->eestate << '\n';
  }
}


void EEPROM::reset(RESET_TYPE by)
{

  switch(by)
    {
    case POR_RESET:
      eecon1->value = 0;          // eedata & eeadr are undefined at power up
      eecon2->eestate = EECON2::EEUNARMED;
    }

}

void EEPROM::initialize(unsigned int new_rom_size, 
			EECON1 *con1,  EECON2 *con2, 
			EEDATA *data,  EEADR *adr,
			EEDATA *datah=NULL, EEADR *adrh=NULL)
{

  rom_size = new_rom_size;
  cpu->eeprom_size = new_rom_size;

  // Save the pointers to all of the control registers
  
  eecon1  = con1;
  eecon2  = con2;
  eedata  = data;
  eeadr   = adr;
  eedatah = datah;
  eeadrh  = adrh;

  // Let the control registers have a pointer to the peripheral in which they belong.

  eecon1->eeprom = this;
  eecon2->eeprom = this;
  eedata->eeprom = this;
  eeadr->eeprom  = this;
  if(eedatah)
    eedatah->eeprom = this;
  if(eeadrh)
    eeadrh->eeprom  = this;


  // Create the rom

  rom = (file_register **) new char[sizeof (file_register *) * rom_size];


  // Initialize the rom

  char str[100];
  for (int i = 0; i < rom_size; i++)
    {

      rom[i] = new file_register;
      rom[i]->address = i;
      rom[i]->break_point = 0;
      rom[i]->value = 0;
      rom[i]->alias_mask = 0;

      sprintf (str, "eeprom reg 0x%02x", i);
      rom[i]->new_name(str);

    }

  //??? FIXME:
  reset(POR_RESET);

}

void EEPROM::dump(void)
{
  unsigned int i, j, reg_num,v;

  cout << "     " << hex;

  // Column labels
  for (i = 0; i < 16; i++)
    cout << setw(2) << setfill('0') <<  i << ' ';

  cout << '\n';

  for (i = 0; i < rom_size/16; i++)
    {
      cout << setw(2) << setfill('0') <<  i << ":  ";

      for (j = 0; j < 16; j++)
	{
	  reg_num = i * 16 + j;
	  if(reg_num < rom_size)
	    {
	      v = rom[reg_num]->get_value();
	      cout << setw(2) << setfill('0') <<  v << ' ';
	    }
	  else
	    cout << "-- ";
	}
      cout << "   ";

      for (j = 0; j < 16; j++)
	{
	  reg_num = i * 16 + j;
	  if(reg_num < rom_size)
	    {
	      v = rom[reg_num]->get_value();
	      if( (v >= ' ') && (v <= 'z'))
		cout.put(v);
	      else
		cout.put('.');
	    }
	}

      cout << '\n';

    }
}


