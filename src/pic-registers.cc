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

#include "clock_phase.h"

//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("0x%06X %s() ",cycles.get(),__FUNCTION__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

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
  memory_size_mask = 0;
  pclath_mask = 0x1800;    // valid pclath bits for branching in 14-bit cores 
  instruction_phase = 0;

  _xref.assign_data(this);

  trace_state = 0;
  trace_increment = 0;
  trace_branch = 0;
  trace_skip = 0;
  trace_other = 0;
  new_name("pc");
}

//--------------------------------------------------
void Program_Counter::set_trace_command(unsigned int new_command)
{
  trace_increment = new_command | (0<<16);
  trace_branch    = new_command | (1<<16);
  trace_skip      = new_command | (2<<16);
  trace_other     = new_command | (3<<16);
}
//--------------------------------------------------
// increment - update the program counter. All non-branching instructions pass through here.
//   

void Program_Counter::increment(void)
{
  Dprintf(("%d\n",value));

  // Trace the value of the program counter before it gets changed.
  trace.raw(trace_increment | value);
  value = (value + 1) & memory_size_mask;

  // Update pcl. Note that we don't want to pcl.put() because that 
  // will trigger a break point if there's one set on pcl. (A read/write
  // break point on pcl should not be triggered by advancing the program
  // counter).

  cpu_pic->pcl->value.put(value & 0xff);

#ifdef CLOCK_EXPERIMENTS
  mCurrentPhase->setNextPhase(mExecute1Cycle);
#else
  get_cycles().increment();
#endif
}

//--------------------------------------------------
// skip - Does the same thing that increment does, except that it records the operation
// in the trace buffer as a 'skip' instead of a 'pc update'.
//   

void Program_Counter::skip(void)
{
  Dprintf(("%d\n",value));

  // Trace the value of the program counter before it gets changed.
  trace.raw(trace_skip | value);


#ifdef CLOCK_EXPERIMENTS
  mExecute2ndHalf->firstHalf((value + 2) & memory_size_mask);
#else
  //value = (value + 1) & memory_size_mask;
  //cpu_pic->pcl->value.put(value & 0xff);
  //get_cycles().increment();

  value = (value + 1) & memory_size_mask;
  cpu_pic->pcl->value.put(value & 0xff);
  get_cycles().increment();

  trace.raw(trace_increment | value);

  value = (value + 1) & memory_size_mask;
  cpu_pic->pcl->value.put(value & 0xff);
  get_cycles().increment();
#endif

}

//--------------------------------------------------
// start_skip - The next instruction is going to be skipped
//   

void Program_Counter::start_skip(void)
{

}

//========================================================================
#if defined(CLOCK_EXPERIMENTS)

phaseExecute2ndHalf::phaseExecute2ndHalf(Processor *pcpu)
  : ProcessorPhase(pcpu), m_uiPC(0)
{
}
phaseExecute2ndHalf::~phaseExecute2ndHalf()
{
}

ClockPhase *phaseExecute2ndHalf::firstHalf(unsigned int uiPC)
{
  m_uiPC = uiPC;
  Dprintf(("first half of 2 cycle instruction\n"));
  mCurrentPhase->setNextPhase(this);
  return this;
}

ClockPhase *phaseExecute2ndHalf::advance()
{
  Dprintf(("second half of 2 cycle instruction\n"));
  ((pic_processor *)m_pcpu)->pc->value = m_uiPC;
  ((pic_processor *)m_pcpu)->pcl->value.put(m_uiPC&0xff);
  mCurrentPhase->setNextPhase(mExecute1Cycle);
  get_cycles().increment();
  return m_pNextPhase;
}


phaseExecuteInterrupt::phaseExecuteInterrupt(Processor *pcpu)
  : ProcessorPhase(pcpu), m_pPreviousPhase(this), m_uiPC(0)
{
}
phaseExecuteInterrupt::~phaseExecuteInterrupt()
{
}
ClockPhase *phaseExecuteInterrupt::firstHalf(unsigned int uiPC)
{
  m_uiPC = uiPC;
  Dprintf(("first half of Interrupt\n"));
  //mCurrentPhase->setNextPhase(this);
  mCurrentPhase = this;
  return this;
}

ClockPhase *phaseExecuteInterrupt::advance()
{
  Dprintf(("second half of Interrupt\n"));

  ((pic_processor *)m_pcpu)->pc->value = m_uiPC;
  ((pic_processor *)m_pcpu)->pcl->value.put(m_uiPC&0xff);
  mCurrentPhase->setNextPhase(mExecute1Cycle);
  get_cycles().increment();
  return m_pNextPhase;
  /*
  if (m_pPreviousPhase == mExecute1Cycle) {

    ((pic_processor *)m_pcpu)->pc->value = m_uiPC;
    ((pic_processor *)m_pcpu)->pcl->value.put(m_uiPC&0xff);
    mCurrentPhase->setNextPhase(mExecute1Cycle);
    get_cycles().increment();
    return m_pNextPhase;
  }
  // else...
  return (m_pPreviousPhase != this) ?  m_pPreviousPhase->advance() : mExecute1Cycle;
  */
}
void phaseExecuteInterrupt::setNextPhase(ClockPhase *pNextPhase)
{ 
  Dprintf(("Interrupt setting phase\n"));
  m_pNextPhase = pNextPhase;
}
#endif // defined(CLOCK_EXPERIMENTS)

//--------------------------------------------------
// jump - update the program counter. All branching instructions except computed gotos
//        and returns go through here.

void Program_Counter::jump(unsigned int new_address)
{
  Dprintf(("%d\n",value));

  // Trace the value of the program counter before it gets changed.
  trace.raw(trace_branch | value);

  // Use the new_address and the cached pclath (or page select bits for 12 bit cores)
  // to generate the destination address:


  // see Update pcl comment in Program_Counter::increment()
  
#ifdef CLOCK_EXPERIMENTS
  mExecute2ndHalf->firstHalf(new_address & memory_size_mask);
#else
  value = new_address & memory_size_mask;
  cpu_pic->pcl->value.put(value & 0xff);
  get_cycles().increment();
  get_cycles().increment();
#endif

}

//--------------------------------------------------
// interrupt - update the program counter. Like a jump, except pclath is ignored.
//

void Program_Counter::interrupt(unsigned int new_address)
{
  Dprintf(("%d\n",value));

#ifdef CLOCK_EXPERIMENTS
  if (mCurrentPhase != mExecute1Cycle) {

    Dprintf((" Ignoring this interrupt\n"));
    return;
  }

  // Trace the value of the program counter before it gets changed.
  trace.raw(trace_branch | value);

  mExecuteInterrupt->firstHalf(new_address & memory_size_mask);
#else

  // Trace the value of the program counter before it gets changed.
  trace.raw(trace_branch | value);

  // Use the new_address and the cached pclath (or page select bits for 12 bit cores)
  // to generate the destination address:

  value = new_address & memory_size_mask;

  cpu_pic->pcl->value.put(value & 0xff);    // see Update pcl comment in Program_Counter::increment()
  
  get_cycles().increment();
  get_cycles().increment();
#endif

}

//--------------------------------------------------
// computed_goto - update the program counter. Anytime the pcl register is written to
//                 by the source code we'll pass through here.
//

void Program_Counter::computed_goto(unsigned int new_address)
{

  Dprintf(("%d\n",value));

  // Trace the value of the program counter before it gets changed.
  trace.raw(trace_other | value);

  // Use the new_address and the cached pclath (or page select bits for 12 bit cores)
  // to generate the destination address:

  value = (new_address | cpu_pic->get_pclath_branching_modpcl() ) & memory_size_mask;

  // see Update pcl comment in Program_Counter::increment()

  cpu_pic->pcl->value.put(value & 0xff);

  // The instruction modifying the PCL will also increment the program counter.
  // So, pre-compensate the increment with a decrement:
  value--;

  // The computed goto is a 2-cycle operation. The first cycle occurs within 
  // the instruction (i.e. via the ::increment() method). The second cycle occurs
  // here:

#ifdef CLOCK_EXPERIMENTS
  mCurrentPhase = mExecute1Cycle;
#else
  get_cycles().increment();
#endif
}

//--------------------------------------------------
// new_address - write a new value to the program counter. All returns pass through here.
//

void Program_Counter::new_address(unsigned int new_address)
{
  Dprintf(("%d\n",value));

  // Trace the value of the program counter before it gets changed.
  trace.raw(trace_branch | value);

#ifdef CLOCK_EXPERIMENTS
  mExecute2ndHalf->firstHalf(new_address & memory_size_mask);
#else
  value = new_address & memory_size_mask;

  // see Update pcl comment in Program_Counter::increment()

  cpu_pic->pcl->value.put(value & 0xff);

  get_cycles().increment();
  get_cycles().increment();
#endif
}

//--------------------------------------------------
// get_next - get the next address that is just pass the current one
//            (used by 'call' to obtain the return address)

unsigned int Program_Counter::get_next(void)
{

  return( (value + cpu_pic->program_memory[value]->instruction_size()) & memory_size_mask);

}


//--------------------------------------------------
// put_value - Change the program counter without affecting the cycle counter
//             (This is what's called if the user changes the pc.)

void Program_Counter::put_value(unsigned int new_value)
{
  // FIXME 
#define PCLATH_MASK              0x1f

  trace.raw(trace_other | value);

  value = new_value & memory_size_mask;
  cpu_pic->pcl->value.put(value & 0xff);
  cpu_pic->pclath->value.put((new_value >> 8) & PCLATH_MASK);

  cpu_pic->pcl->update();
  cpu_pic->pclath->update();
  update();
}

void Program_Counter::reset(void)
{ 
  //trace.program_counter(value);  //FIXME
  value = reset_address;
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
