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
#include <iostream>
#include <iomanip>


#include "../config.h"
#include "14bit-processors.h"
#include "interface.h"
#include "pic-registers.h"

//--------------------------------------------------
void WDT::update(void)
{
  if(wdte){

    value = cpu->time_to_cycles(timeout);
    prescale = cpu->option_reg.get_psa() ? (cpu->option_reg.get_prescale()) : 0;

    if(future_cycle) {

      guint64 fc = cycles.value + value * (1<<prescale);

      //cout << "WDT::update:  moving break from " << future_cycle << " to " << fc << '\n';

      cycles.reassign_break(future_cycle, fc, this);
      future_cycle = fc;

    } else {
    
      future_cycle = cycles.value + value * (1<<prescale);

      cycles.set_break(future_cycle, this);
    }
  }

}

//--------------------------------------------------
// WDT::put - shouldn't be called?
//

void WDT::put(unsigned int new_value)
{
  value = new_value;

  update();

}

void WDT::initialize(bool enable, double _timeout)
{
  break_point = 0;
  wdte = enable;
  timeout = _timeout;
  warned = 0;

  if(verbose)
    cout << " WDT init called "<< ( (enable) ? "enabling\n" :", but disabling WDT\n");

  if(wdte)
    {
      cout << "Enabling WDT " << " timeout = " << timeout << " seconds\n";
      value = cpu->time_to_cycles(timeout);
      prescale = cpu->option_reg.get_psa() ? (cpu->option_reg.get_prescale()) : 0;

      future_cycle = cycles.value + value * (1<<prescale);

      cycles.set_break(future_cycle, this);

    }

}

void WDT::callback(void)
{


  if(wdte) {
    cout<<"WDT timeout: " << hex << cycles.value << '\n';

    future_cycle = 0;
    update();

    // The TO bit gets cleared when the WDT times out.
    cpu->status->put_TO(0);

    if(break_point)
      bp.halt();
    else if(bp.have_sleep()) {
      bp.clear_sleep();
    }else
      cpu->reset(WDT_RESET);
  }

}

void WDT::clear(void)
{
  if(wdte)
    update();
  else
    {
      if(!warned)
	{
	  warned = 1;
	  cout << "The WDT is not enabled - clrwdt has no effect!\n";
	}
    }

}

void WDT::start_sleep(void)
{

  if(wdte) {
    prescale = 0;

    guint64 fc = cycles.value + value * (1<<prescale);

    //cout << "WDT::start_sleep:  moving break from " << future_cycle << " to " << fc << '\n';

    cycles.reassign_break(future_cycle, fc, this);

    future_cycle = fc;
  }
}

void WDT::new_prescale(void)
{

  update();

}

void WDT::callback_print(void)
{

  cout << "WDT\n";
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
  _xref.assign_data(this);

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

  cpu_pic->pcl->value.put(value & 0xff);
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

  cpu_pic->pcl->value.put( value & 0xff);
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

  cpu_pic->pcl->value.put(value & 0xff);
  
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

  cpu_pic->pcl->value.put(value & 0xff);    // see Update pcl comment in Program_Counter::increment()
  
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

  cpu_pic->pcl->value.put(value & 0xff);

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

  cpu_pic->pcl->value.put(value & 0xff);
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
  cpu_pic->pcl->value.put(value & 0xff);
  cpu_pic->pclath->value.put((new_value >> 8) & PCLATH_MASK);

  trace.program_counter(value);

  cpu_pic->pcl->update();
  cpu_pic->pclath->update();
  update();
}

void Program_Counter::reset(void)
{ 
  value = reset_address;
  trace.program_counter(value);
}


//========================================================================
//
// Helper registers
//
 
PCHelper::PCHelper(ProgramMemoryAccess *new_pma)
{
  pma = new_pma;
  new_name("PC");
}

void PCHelper::put_value(unsigned int new_value)
{
  if(pma)
    pma->set_PC(new_value);
}

unsigned int PCHelper::get_value(void)
{
 if(pma)
    return pma->get_PC();

 return 0;
}
