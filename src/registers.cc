/*
   Copyright (C) 1998-2003 Scott Dattalo

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


#include "../config.h"
#include "14bit-processors.h"
#include "interface.h"

//#include <string>
//#include "stimuli.h"

#include "xref.h"
#include "pic-registers.h"

//--------------------------------------------------
// Member functions for the file_register base class
//--------------------------------------------------
//

Register::Register(void)
{
  cpu = NULL;
  name_str1 = NULL;
  new_name("file_register");
  xref = new XrefObject(&value);
  read_access_count=0;
  write_access_count=0;
  bit_mask = 7;

}

Register::~Register(void)
{
  if(name_str1)
    delete(name_str1);

//  if(xref)
//    delete(xref);
}

//------------------------------------------------------------
// get()
//
//  Return the contents of the file register.
// (note - breakpoints on file register reads
//  are not checked here. Instead, a breakpoint
//  object replaces those instances of file 
//  registers for which we wish to monitor.
//  So a file_register::get call will invoke
//  the breakpoint::get member function. Depending
//  on the type of break point, this get() may
//  or may not get called).

unsigned int Register::get(void)
{
  trace.register_read(address,value);
  return(value);
}

//------------------------------------------------------------
// put()
//
//  Update the contents of the register.
//  See the comment above in file_register::get()
//  with respect to break points
//

void Register::put(unsigned int new_value)
{
  value = new_value;
  trace.register_write(address,value);
}


int Register::get_bit(unsigned int bit_number)
{

  return( (value >>  (bit_number & 0x07)) & 1 );

}

int Register::get_bit_voltage(unsigned int bit_number)
{
  if(get_bit(bit_number))
    return +1000;
  else
    return -1000;
}
//--------------------------------------------------
// set_bit
//
//  set a single bit in a register. Note that this
// is really not intended to be used on the file_register
// class. Instead, setbit is a place holder for high level
// classes that overide this function
void Register::setbit(unsigned int bit_number, bool new_value)
{
  if(bit_number < bit_mask) {
    value = (value & ~(1<<bit_number)) | (1<<bit_number);
    trace.register_write(address,value);
  }
}

void Register::setbit_value(unsigned int bit_number, bool new_value)
{
  setbit(bit_number,new_value);
}


void Register::new_name(char *s)
{
  if(name_str1)
    delete name_str1;

  if(s)
    name_str1 = strdup(s);
  else
    name_str1 = NULL;

}



//-----------------------------------------------------------
//  void Register::put_value(unsigned int new_value)
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

void Register::put_value(unsigned int new_value)
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
    xref->update();

}


//------------------------------------------------------------------------
invalid_file_register::invalid_file_register(unsigned int at_address)
{

  char name_str[100];
  sprintf (name_str, "invalid fr  0x%02x", at_address);
  new_name(name_str);
}



//------------------------------------------------------------------------
// member functions for the Program_Counter base class
//------------------------------------------------------------------------
//

//--------------------------------------------------

Program_Counter::Program_Counter(void)
{
  if(verbose)
    cout << "pc constructor\n";

  reset_address = 0;
  value = 0;
  pclath_mask = 0x1800;    // valid pclath bits for branching in 14-bit cores 

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

  cpu_pic->pcl->value = value & 0xff;
  cycles.increment();
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

  cpu_pic->pcl->value = value & 0xff;
  cycles.increment();
}

//--------------------------------------------------
// jump - update the program counter. All branching instructions except computed gotos
//        and returns go through here.

void Program_Counter::jump(unsigned int new_address)
{

  // Use the new_address and the cached pclath (or page select bits for 12 bit cores)
  // to generate the destination address:

  //value = (new_address | cpu_pic->get_pclath_branching_jump() ) & memory_size_mask;
  value = new_address & memory_size_mask;

    // see Update pcl comment in Program_Counter::increment()

  cpu_pic->pcl->value = value & 0xff;
  
  cycles.increment();
  trace.program_counter(value);
  cycles.increment();

}

//--------------------------------------------------
// interrupt - update the program counter. Like a jump, except pclath is ignored.
//

void Program_Counter::interrupt(unsigned int new_address)
{

  // Use the new_address and the cached pclath (or page select bits for 12 bit cores)
  // to generate the destination address:

  value = new_address & memory_size_mask;

  cpu_pic->pcl->value = value & 0xff;    // see Update pcl comment in Program_Counter::increment()
  
  cycles.increment();
  
  trace.cycle_increment(); 
  trace.program_counter(value);

  cycles.increment();

}

//--------------------------------------------------
// computed_goto - update the program counter. Anytime the pcl register is written to
//                 by the source code we'll pass through here.
//

void Program_Counter::computed_goto(unsigned int new_address)
{

  // Use the new_address and the cached pclath (or page select bits for 12 bit cores)
  // to generate the destination address:

  value = (new_address | cpu_pic->get_pclath_branching_modpcl() ) & memory_size_mask;

  trace.cycle_increment();
  trace.program_counter(value);

  // see Update pcl comment in Program_Counter::increment()

  cpu_pic->pcl->value = value & 0xff;

  // The instruction modifying the PCL will also increment the program counter.
  // So, pre-compensate the increment with a decrement:
  value--;
  cycles.increment();
}

//--------------------------------------------------
// new_address - write a new value to the program counter. All returns pass through here.
//

void Program_Counter::new_address(unsigned int new_value)
{
  value = new_value & memory_size_mask;
  trace.cycle_increment();
  trace.program_counter(value);

  // see Update pcl comment in Program_Counter::increment()

  cpu_pic->pcl->value = value & 0xff;
  cycles.increment();
  cycles.increment();
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
  // FIXME 
#define PCLATH_MASK              0x1f

  value = new_value & memory_size_mask;
  cpu_pic->pcl->value = value & 0xff;
  cpu_pic->pclath->value = (new_value >> 8) & PCLATH_MASK;

  trace.program_counter(value);

  if(xref)
    {
      if(cpu_pic->pcl->xref)
	cpu_pic->pcl->xref->update();
      if(cpu_pic->pclath->xref)
	cpu_pic->pclath->xref->update();
	xref->update();
    }
  

}

void Program_Counter::reset(void)
{ 
  value = reset_address;
  trace.program_counter(value);
}

