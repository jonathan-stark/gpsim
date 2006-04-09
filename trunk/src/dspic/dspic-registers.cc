/*
   Copyright (C) 2006 T. Scott Dattalo

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

#include "dspic-registers.h"
#include "dspic-processors.h"
#include "../trace.h"

#if defined(PROPAGATE_UNKNOWNS)
bool gbPropagateUnknown=false;
#endif

namespace dspic {
  extern Trace *gTrace;              // Points to gpsim's global trace object.
  extern Cycle_Counter *gCycles;     // Points to gpsim's global cycle counter.
}

namespace dspic_registers {

  //------------------------------------------------------------------------
  // dsPicRegister

  unsigned int dsPicRegister::iMask = 0xffff;

  //--------------------------------------------------
  bool dsPicRegister::get_bit(unsigned int bit_number)
  {
    return( ((value.get() >>  (bit_number & 0x0f)) & 1 ) ? true : false);
  }


  //--------------------------------------------------
  // PCL - low word of program counter.
  //--------------------------------------------------

  PCL::PCL()
  {
    value = RegisterValue(0,0);
    por_value = value;
    new_name("pcl");
  }

  void PCL::put(unsigned int new_value)
  {
    dspic::gTrace->raw(write_trace.get() | value.get());
    cpu_dsPic->pc->computed_goto(new_value);
  }

  void PCL::put_value(unsigned int new_value)
  {
    value.put(new_value & 0xffff);
    cpu_dsPic->pc->put_value( (cpu_dsPic->pc->get_value() & 0xffff0000) | value.get());
  }

  unsigned int PCL::get()
  {
    return((value.get()+1) & 0xffff);
  }

  unsigned int PCL::get_value()
  {
    value.put(cpu_dsPic->pc->get_value() & 0xffff);
    return(value.get());
  }

  //------------------------------------------------------------------------
  //
  // Program Counter
  //

  dsPicProgramCounter::dsPicProgramCounter(dspic::dsPicProcessor *pcpu, PCL *pPCL)
    : m_pcl(pPCL), m_cpu(pcpu)
  {
    printf("dspic program counter.\n");

    set_trace_command(dspic::gTrace->allocateTraceType(new PCTraceType(pcpu,0,1)));

  }

  //--------------------------------------------------
  // jump - update the program counter. All branching instructions
  //        except computed gotos and returns go through here.

  void dsPicProgramCounter::jump(unsigned int new_address)
  {

    dspic::gTrace->raw(trace_other | (value<<1));
    value = new_address & memory_size_mask;

    m_pcl->value.put(value & 0xffff);

    dspic::gCycles->increment();
    dspic::gCycles->increment();

  }
  void dsPicProgramCounter::computed_goto(unsigned int new_address)
  {
    printf("dspic %s.\n",__FUNCTION__);

    dspic::gTrace->raw(trace_other | (value<<1));

    // Use the new_address and the cached pclath (or page select bits
    // for 12 bit cores) to generate the destination address: 

    value = (new_address >>1) & memory_size_mask;

    // see Update pcl comment in Program_Counter::increment()
    m_pcl->value.put((value<<1) & 0xffff);

    // The instruction modifying the PCL will also increment the
    // program counter. So, pre-compensate the increment with a
    // decrement:

    value--;
    dspic::gCycles->increment();
  }

  void dsPicProgramCounter::put_value(unsigned int new_value)
  {
    printf("dspic program counter::%s. (0x%x)\n",__FUNCTION__,new_value);

    dspic::gTrace->raw(trace_other | (value<<1));

    value = new_value & memory_size_mask;
    m_pcl->value.put(value & 0xff);
    //m_cpu->pclath->value.put((value >> 8) & 0xff);
    m_pcl->update();
    //cpu_pic->pclath->update();

    update();

  }

  unsigned int dsPicProgramCounter::get_value()
  {
    printf("dspic program counter::%s.\n",__FUNCTION__);

    return value*2;
  }

  //--------------------------------------------------
  // increment - update the program counter. All non-branching
  // instructions pass through here.    

  void dsPicProgramCounter::increment()
  {

    // Trace the value of the program counter before it gets changed.
    dspic::gTrace->raw(trace_increment | value);
    value = (value + 1) & memory_size_mask;

    // Update pcl. Note that we don't want to pcl.put() because that 
    // will trigger a break point if there's one set on pcl. (A read/write
    // break point on pcl should not be triggered by advancing the program
    // counter).

    m_pcl->value.put(value & 0xffff);
    dspic::gCycles->increment();
  }

  //----------------------------------------------------------------------
  // Stack 
  //----------------------------------------------------------------------
  Stack::Stack()
    : m_cpu(0)
  {
  }
  void Stack::init(dspic::dsPicProcessor *pCpu)
  {
    m_cpu = pCpu;
  }
  void Stack::push()
  {
    unsigned int pc = m_cpu->pc->get_value();
    unsigned int rm_size = m_cpu->register_memory_size()>>1;
    unsigned int tos = m_cpu->W[15].get_value();
    unsigned int tos_index = tos>>1;
    
    // Push the current PC onto the stack
    m_cpu->registers[tos_index    %rm_size]->put(pc & 0xffff);
    m_cpu->registers[(tos_index+1)%rm_size]->put((pc>>16) & 0xffff);
    m_cpu->W[15].put(tos + 4);

  }
  void Stack::pop()
  {
  }
}
