/*
   Copyright (C) 1998 T. Scott Dattalo

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

//-------------------------------------------------------------------
//                     interface.cc
//
// interface.cc provides a layer of code on top of the simulator 
// portion of gpsim. It's purpose is to provide an abstract interface
// that hides the details of the simulator. In addition, the interface
// is written with 'C' callable functions. Currently only the gui
// interfaces to gpsim through this layer. However, since the simulator
// 'engine' is built as a library, it's possible for other code to
// interface through here as well.
//
//-------------------------------------------------------------------

#include <stdio.h>

#include "../config.h"
#define GPSIM_VERSION VERSION 
#include "gpsim_def.h"

#include "pic-processor.h"
#include "p16x8x.h"
#include "xref.h"
#include "interface.h"
#include "trace.h"

gpointer create_interface(unsigned int processor_id, 
			  void (*interface_callback)(gpointer), 
			  gpointer interface_callback_data);

extern pic_processor *active_cpu;

unsigned int gpsim_is_initialized = 0;  // Flag to tell us when all of the init stuff is done.

int hll_mode=0; // 0 - asm, 1 - HLL

extern "C" {
unsigned int gpsim_get_register_memory_size(unsigned int processor_id,REGISTER_TYPE type);
}
extern "C" {
  extern void redisplay_prompt(void);  // in input.cc
}

//--------------------------------------------------------------------
// InterfaceObject 
//
// This class is used only within interface.cc's scope. It's purpose
// is to provide a way to call back some function not gpsim's code
// space.

class InterfaceObject : public BreakCallBack
{
public:
  pic_processor *pic;

  void (*callback_function)(gpointer);
  gpointer callback_data;

  InterfaceObject(void) {pic = NULL;};
  virtual void callback(void)
  {
      if(callback_function)
	  callback_function(callback_data);
    };

};

class CyclicBreakPoint : public InterfaceObject
{
public:
  guint64 delta_cycles;

  void set_break(void)
    {
      // FIXME - This overrides what the gui specifies
      //   for now this is okay, but if we ever want to have different
      //   update rates for various gui objects (e.g. update the ram window
      //   more frequently than the source window) then we'll need to change
      //   this.
      delta_cycles = gui_update_rate;

      pic->cycles.set_break(pic->cycles.value + delta_cycles, this);
    }
      
  virtual void callback(void)
    {
//      trace.cycle_counter(pic->cycles.value); // FIXME, temporary.
      profile_keeper.catchup();
      InterfaceObject::callback();
      set_break();
    };

};

//
// interface.cc
//
// This routine provides helper functions for accessing gpsim internals.
// Its ultimate purpose is to be replaced with some kind of dynamically
// linked thing like CORBA or sockets...
//

/*
void (*update_object) (gpointer xref,int new_value);
void (*remove_object) (gpointer xref);
void (*simulation_has_stopped) (void);
void (*new_processor) (unsigned int processor_id);
void (*new_program)  (unsigned int p);
*/


/**************************************************************************
 *
 *  Here's the gpsim interface class instantiation. It's through this class
 * that gpsim will notify the gui and/or modules of internal gpsim changes.
 *
 **************************************************************************/

gpsimInterface gi;


//--------------------------------------------------------------------------

pic_processor *get_processor(unsigned int cpu_id);

unsigned int processor_has_eeprom(pic_processor *pic)
{
  return ((pic->eeprom) ? TRUE : FALSE);
}

void  initialization_is_complete(void)
{
  gpsim_is_initialized = 1;
}

//--------------------------------------------------------------------------
//
// valid_register - local function
//  Given a pointer to a pic processor, the type of register (file or eeprom 
//  for now), and the register number, this routine will return true if the
//  register is a valid one. 'Valid' means that the register is accessable
//  by the pic software that runs on the simulated processor.
//
unsigned int valid_register(pic_processor *pic, REGISTER_TYPE type, unsigned int register_number)
{

  if(!pic) 
    return 0;

  if(type == REGISTER_EEPROM)
  {
    if(pic->eeprom && (register_number < pic->eeprom->rom_size))
      return TRUE;

    return FALSE;
  }
  
  if(register_number < pic->register_memory_size())
    {
      file_register *reg = pic->registers[register_number];

      
      while(reg->isa() == file_register::BP_REGISTER)
	{
	  reg = ((Notify_Register *)reg)->replaced;
	}


      if(reg->isa() == file_register::INVALID_REGISTER)
	return 0;
      else
	return 1;
    }
  else
    cout << "Warning: Request for register 0x " << hex << register_number
	 << " in processor " << pic->name()
	 << " is out of range\n";


  return 0;
}

//--------------------------------------------------------------------------
// file_register *gpsim_get_register(
//         unsigned int processor_id, 
//         REGISTER_TYPE type, 
//         unsigned int register_number)
//
// given a processor id, a register type, and the register number, this routine
// will return a pointer to the referenced file register. If the register doesn't
// exist, then NULL is returned.
//

file_register *gpsim_get_register(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return NULL;

  if(!valid_register(pic,type,register_number))
    return NULL;
  
  switch(type) {
  case REGISTER_RAM:
    return pic->registers[register_number];
    break;
  case REGISTER_EEPROM:
    if(pic->eeprom)
      return pic->eeprom->get_register(register_number);
    // fall through to return NULL
  default:
    return NULL;
  }

}
//--------------------------------------------------------------------------

char *gpsim_get_register_name(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number)
{
  static char buffer[128];
  char *name = NULL;
  file_register *fr = gpsim_get_register( processor_id, type,  register_number);

  if(fr)
    name = fr->name();
  if(!name)
    return NULL;
  
  if(gpsim_register_is_alias(processor_id, type, register_number))
      sprintf(buffer,"alias (%s)", name);
  else
      strcpy(buffer,name);

  return buffer;
}

//--------------------------------------------------------------------------

unsigned int gpsim_get_register_value(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number)
{
  file_register *fr = gpsim_get_register( processor_id, type,  register_number);

  if(fr)
    return fr->get_value();
  else
    return INVALID_VALUE;

}

//--------------------------------------------------------------------------

gboolean gpsim_register_is_alias(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number)
{
  file_register *fr = gpsim_get_register( processor_id, type,  register_number);

  if(fr) 
    if(fr->address!=register_number)
      return TRUE;

  return FALSE;

}

//--------------------------------------------------------------------------

gboolean gpsim_register_is_sfr(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number)
{
    pic_processor *pic = get_processor(processor_id);

    if(!valid_register(pic,type,register_number))
	return FALSE;
  
    if(type == REGISTER_EEPROM)
	return FALSE;

    if(pic->registers[register_number]->isa()==file_register::SFR_REGISTER)
	return TRUE;

    return FALSE;
}

//--------------------------------------------------------------------------

gboolean gpsim_register_is_valid(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number)
{
    pic_processor *pic = get_processor(processor_id);

    if(!valid_register(pic,type,register_number))
	return FALSE;
  
    return TRUE;
}


//--------------------------------------------------------------------------

void  gpsim_put_register_value(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number, unsigned int register_value)
{
  file_register *fr = gpsim_get_register( processor_id, type,  register_number);

  if(fr) 
    fr->put_value(register_value);

}

//--------------------------------------------------------------------------


#include <vector>
#include "symbol.h"

extern vector <symbol *> st;

static vector <symbol *>::iterator interface_sti;

void gpsim_symbol_rewind(unsigned int processor_id)
{
    interface_sti=st.begin();
}

sym *gpsim_symbol_iter(unsigned int processor_id)
{
    static sym s;

    while(interface_sti!=st.end())
    {
        if((*interface_sti)->isa() == SYMBOL_LINE_NUMBER)
        {
            interface_sti++;
            continue;
        }
	//
        strncpy(s.name,
                (*interface_sti)->name()->data(),
                (*interface_sti)->name()->length());
        s.name[(*interface_sti)->name()->length()]=0;
        s.type=(*interface_sti)->isa();
        s.value=(*interface_sti)->get_value();
        interface_sti++;
        return &s;
    }
    return NULL;
}

//--------------------------------------------------------------------------

char *gpsim_processor_get_name(unsigned int processor_id)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
      return NULL;

  return pic->name();
}

//--------------------------------------------------------------------------

unsigned int gpsim_get_pc_value(unsigned int processor_id)
{

  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return INVALID_VALUE;

  return pic->pc.value;

}

//--------------------------------------------------------------------------

void  gpsim_put_pc_value(unsigned int processor_id, unsigned int pc_value)
{

  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return;

  pic->pc.put_value(pc_value);


}

//--------------------------------------------------------------------------

unsigned int gpsim_get_status(unsigned int processor_id)
{

  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return INVALID_VALUE;

  return pic->status.get_value();

}
//--------------------------------------------------------------------------

void gpsim_put_status(unsigned int processor_id, unsigned int status_value)
{

  pic_processor *pic = get_processor(processor_id);

  if(!pic)
      return;

  pic->status.put_value(status_value);
  
  return;

}

//--------------------------------------------------------------------------

unsigned int gpsim_get_w(unsigned int processor_id)
{

  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return INVALID_VALUE;

  return pic->W.get_value();

}
//--------------------------------------------------------------------------

void gpsim_put_w(unsigned int processor_id, unsigned int w_value)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
      return;
  
  pic->W.put_value(w_value);

  return;
}

//--------------------------------------------------------------------------

unsigned int gpsim_get_cycles_lo(unsigned int processor_id)
{

  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return INVALID_VALUE;

  return (pic->cycles.value & 0xffffffff);

}

//--------------------------------------------------------------------------

guint64 gpsim_get_cycles(unsigned int processor_id)
{

  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return INVALID_VALUE;

  return (pic->cycles.value);

}

//--------------------------------------------------------------------------

double gpsim_get_inst_clock(unsigned int processor_id)
{

  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return INVALID_VALUE;

  return (pic->time_to_cycles(1.0)/4.0);

}
//--------------------------------------------------------------------------
//  guint64 gpsim_digitize_time(double time)
//  -- this is hack for now. Eventually gpsim will support "simulated real time".
//
guint64 gpsim_digitize_time(double time)
{

  if(!active_cpu)
    return 0;

  return (active_cpu->time_to_cycles(time));

}

//--------------------------------------------------------------------------
//
// gui_update_rate - a global variable defining how many simulation cycles
//                   to wait between calls to updating the gui.

guint64  gpsim_get_update_rate(void)
{
  return(gui_update_rate);
}

void  gpsim_set_update_rate(guint64 new_rate)
{
  if(new_rate == 0)
    new_rate = DEFAULT_GUI_UPDATE_RATE;

  gui_update_rate = new_rate;
}


//--------------------------------------------------------------------------
unsigned int gpsim_get_program_memory_size(unsigned int processor_id)
{

  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return 0;
  return pic->program_memory_size();
}

//--------------------------------------------------------------------------
unsigned int gpsim_get_register_memory_size(unsigned int processor_id,REGISTER_TYPE type)
{

  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return 0;

  if(!valid_register(pic,type,0))
      return 0;
  
  if(type == REGISTER_EEPROM) {
    if(pic->eeprom)
      return pic->eeprom->rom_size;
    return 0;
  }
  return pic->register_memory_size();
}

//--------------------------------------------------------------------------
unsigned int gpsim_reg_has_breakpoint(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number)
{
  file_register *fr = gpsim_get_register( processor_id, type,  register_number);

  if(fr) 
    if(fr->isa() == file_register::BP_REGISTER)
      return TRUE;

  return FALSE;

}
//--------------------------------------------------------------------------
unsigned int gpsim_reg_set_read_breakpoint(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number)
{
 pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return INVALID_VALUE;
  return bp.set_read_break(pic,register_number);
}

//--------------------------------------------------------------------------
unsigned int gpsim_reg_set_write_breakpoint(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number)
{
 pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return INVALID_VALUE;
  return bp.set_write_break(pic,register_number);
}
//--------------------------------------------------------------------------
unsigned int gpsim_reg_set_read_value_breakpoint(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number, unsigned int value)
{
 pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return INVALID_VALUE;
  return bp.set_read_value_break(pic,register_number,value);
}

//--------------------------------------------------------------------------
unsigned int gpsim_reg_set_write_value_breakpoint(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number, unsigned int value)
{
 pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return INVALID_VALUE;
  return bp.set_write_value_break(pic,register_number,value);
}
//--------------------------------------------------------------------------
unsigned int gpsim_reg_clear_breakpoints(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number)
{
    unsigned int breakpoint_number;
    pic_processor *pic = get_processor(processor_id);
    
    if(!pic)
	return 0;

    while(pic->registers[register_number]->isa()==file_register::BP_REGISTER)
    {
      //breakpoint_number = pic->registers[register_number]->replaced->break_point & ~Breakpoints::BREAK_MASK;
      breakpoint_number = pic->registers[register_number]->break_point & ~Breakpoints::BREAK_MASK;
	bp.clear(breakpoint_number);
    }

    redisplay_prompt();

    return 1;
}
//--------------------------------------------------------------------------
unsigned int gpsim_set_log_name(unsigned int processor_id, char *filename)
{
    pic_processor *pic = get_processor(processor_id);
    
    if(!pic)
	return 0;

    if(filename!=NULL)
	trace_log.enable_logging(filename);

    return 0;
}
//--------------------------------------------------------------------------
unsigned int gpsim_reg_set_read_logging(unsigned int processor_id,
					REGISTER_TYPE type,
					unsigned int register_number)
{
    int b;
    pic_processor *pic = get_processor(processor_id);
    
    if(!pic)
	return 0;

    if(!trace_log.logging)
	trace_log.enable_logging();

    b = bp.set_notify_read(pic, register_number);

    return 0;
}
//--------------------------------------------------------------------------
unsigned int gpsim_reg_set_write_logging(unsigned int processor_id,
					 REGISTER_TYPE type,
					 unsigned int register_number)
{
    int b;
    pic_processor *pic = get_processor(processor_id);
    
    if(!pic)
	return 0;

    if(!trace_log.logging)
	trace_log.enable_logging();

    b = bp.set_notify_write(pic, register_number);

    return 0;
}
//--------------------------------------------------------------------------
unsigned int gpsim_reg_set_read_value_logging(unsigned int processor_id,
					      REGISTER_TYPE type,
					      unsigned int register_number,
					      unsigned int value,
					      unsigned int mask)
{
    int b;
    pic_processor *pic = get_processor(processor_id);
    
    if(!pic)
	return 0;

    if(!trace_log.logging)
	trace_log.enable_logging();

    b = bp.set_notify_read_value(pic, register_number, value, mask);

    return 0;
}
//--------------------------------------------------------------------------
unsigned int gpsim_reg_set_write_value_logging(unsigned int processor_id,
					       REGISTER_TYPE type,
					       unsigned int register_number,
					       unsigned int value,
					       unsigned int mask)
{
    int b;
    pic_processor *pic = get_processor(processor_id);
    
    if(!pic)
	return 0;

    if(!trace_log.logging)
	trace_log.enable_logging();

    b = bp.set_notify_write_value(pic, register_number, value, mask);

    return 0;
}
//--------------------------------------------------------------------------
unsigned int gpsim_address_has_breakpoint(unsigned int processor_id, unsigned int address)
{
 pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return 0;

  return pic->address_has_break(address);
}
//--------------------------------------------------------------------------
unsigned int gpsim_address_has_profile_start(unsigned int processor_id,
					     unsigned int address)
{
    pic_processor *pic = get_processor(processor_id);

    if(!pic)
	return 0;

    return pic->address_has_profile_start(address);
}
//--------------------------------------------------------------------------
unsigned int gpsim_address_has_profile_stop(unsigned int processor_id,
					    unsigned int address)
{
    pic_processor *pic = get_processor(processor_id);

    if(!pic)
	return 0;

    return pic->address_has_profile_stop(address);
}
//--------------------------------------------------------------------------
unsigned int gpsim_address_has_opcode(unsigned int processor_id, unsigned int address)
{
 pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return 0;
  if(pic->program_memory_size()<=address)
      return 0;

  if(pic->program_memory[address]->isa() != instruction::INVALID_INSTRUCTION)
    return 1;

  return 0;
}

//--------------------------------------------------------------------------
unsigned int gpsim_address_has_changed(unsigned int processor_id, unsigned int address)
{
 pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return 0;
  if(pic->program_memory_size()<=address)
      return 0;

    return pic->program_memory[address]->is_modified;
}

//--------------------------------------------------------------------------

void  gpsim_assign_program_xref(unsigned int processor_id, unsigned int address, gpointer xref)
{

  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return;
  if(pic->program_memory_size()<=address)
      return;

  if(pic->program_memory[address]->xref)
      pic->program_memory[address]->xref->add(xref);

}
 //--------------------------------------------------------------------------
void  gpsim_clear_register_xref(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number, gpointer xref)
{
  file_register *fr = gpsim_get_register( processor_id, type,  register_number);

  if(fr)
    if(fr->xref)
      fr->xref->clear(xref);

}

//--------------------------------------------------------------------------

void  gpsim_assign_register_xref(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number, gpointer xref)
{
  file_register *fr = gpsim_get_register( processor_id, type,  register_number);

  if(fr)
    if(fr->xref)
      fr->xref->add(xref);
}

//--------------------------------------------------------------------------

void gpsim_assign_pc_xref(unsigned int processor_id, gpointer xref)
{

  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return;
  if(pic->pc.xref)
      pic->pc.xref->add(xref);

}

//--------------------------------------------------------------------------
//
// Trace buffer interface routines
//
void gpsim_assign_trace_xref(gpointer xref)
{

  if(trace.xref)
      trace.xref->add(xref);

}

//--------------------------------------------------------------------------

void gpsim_get_current_trace(guint64 *current_cycle, int *current_index,
			     char *current_trace, int bufsize)
{

  if(current_cycle)
    *current_cycle = trace.string_cycle;
  if(current_index)
    *current_index = trace.string_index;
  if(current_trace)
    strncpy(current_trace,trace.string_buffer,bufsize);
}

int gpsim_move_to_last_trace(void)
{

  return( (trace.trace_index - 1) & TRACE_BUFFER_MASK);
}

int gpsim_get_previous_trace(int current_trace)
{
  return trace.find_previous_cycle(current_trace);
}

void gpsim_trace_dump_to_file(int number_of_instructions, FILE *f)
{
  trace.dump(number_of_instructions, f);

}




//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////
void gpsim_enable_profiling(unsigned int processor_id)
{
    profile_keeper.enable_profiling();
}

void gpsim_disable_profiling(unsigned int processor_id)
{
    profile_keeper.disable_profiling();
}

guint64 gpsim_get_cycles_used(unsigned int processor_id, unsigned int address)
{
    pic_processor *pic = get_processor(processor_id);

    if(!pic)
	return 0;

    return pic->cycles_used(address);
}

guint64 gpsim_get_register_read_accesses(unsigned int processor_id, REGISTER_TYPE type, unsigned int address)
{
    pic_processor *pic = get_processor(processor_id);

    if(!pic)
	return 0;

    return pic->register_read_accesses(address);
}

guint64 gpsim_get_register_write_accesses(unsigned int processor_id, REGISTER_TYPE type, unsigned int address)
{
    pic_processor *pic = get_processor(processor_id);

    if(!pic)
	return 0;

    return pic->register_write_accesses(address);
}

//--------------------------------------------------------------------------
void gpsim_step(unsigned int processor_id, unsigned int steps)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic || (steps==0))
    return;

  pic->step(steps);
  redisplay_prompt();

}
//--------------------------------------------------------------------------
void gpsim_step_over(unsigned int processor_id)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return;

  pic->step_over();
  redisplay_prompt();

}
//--------------------------------------------------------------------------
void gpsim_hll_step(unsigned int processor_id)
{
    unsigned int initial_line;
    unsigned int initial_pc;

    pic_processor *pic = get_processor(processor_id);

    if(!pic)
	return;

    // pic->step(1) until pc==initial_pc, or source line has changed.

    initial_line = gpsim_get_hll_src_line(processor_id,
					  gpsim_get_pc_value(processor_id));
    initial_pc = gpsim_get_pc_value(processor_id);

    while(1)
    {
	pic->step(1);
	if(gpsim_get_pc_value(processor_id)==initial_pc)
	    break;
	if(gpsim_get_hll_src_line(processor_id, gpsim_get_pc_value(processor_id))
	   != initial_line)
	    break;
    }
}
//--------------------------------------------------------------------------
void gpsim_hll_step_over(unsigned int processor_id)
{
    unsigned int initial_line;
    unsigned int address;
    unsigned int initial_stack_depth;
    unsigned int initial_pc;
    unsigned int initial_id;

    pic_processor *pic = get_processor(processor_id);

    if(!pic)
	return;

    initial_line = gpsim_get_hll_src_line(processor_id,
					  gpsim_get_pc_value(processor_id));

    initial_stack_depth = pic->stack->pointer&pic->stack->stack_mask;

    initial_pc = gpsim_get_pc_value(processor_id);
    initial_id = gpsim_get_file_id(processor_id, initial_pc);

    while(1)
    {
	gpsim_hll_step(processor_id);

	if(initial_stack_depth < pic->stack->pointer&pic->stack->stack_mask)
	{
	    gpsim_finish(processor_id);
	}

	if(gpsim_get_pc_value(processor_id)==initial_pc)
	    break;
	if(gpsim_get_hll_src_line(processor_id, gpsim_get_pc_value(processor_id))
	   != initial_line)
	{
	    if(gpsim_get_file_id(processor_id, gpsim_get_pc_value(processor_id))==initial_id)
		break;
	}
    }

}
//--------------------------------------------------------------------------
void gpsim_run(unsigned int processor_id)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return;

  pic->run();
  redisplay_prompt();

}
//--------------------------------------------------------------------------
void gpsim_stop(unsigned int processor_id)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return;

  bp.halt();
}
//--------------------------------------------------------------------------
void gpsim_reset(unsigned int processor_id)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return;

  if(pic)
    pic->reset(POR_RESET);

  redisplay_prompt();

}
//--------------------------------------------------------------------------
void gpsim_finish(unsigned int processor_id)
{
    pic_processor *pic = get_processor(processor_id);
    unsigned int return_address;

  if(!pic)
    return;

  return_address = pic->stack->contents[pic->stack->pointer-1 & pic->stack->stack_mask];

  gpsim_run_to_address(processor_id, return_address);
}
//--------------------------------------------------------------------------
void gpsim_run_to_address(unsigned int processor_id, unsigned int address)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return;

  pic->run_to_address(address);

  redisplay_prompt();

}
//--------------------------------------------------------------------------
unsigned int gpsim_get_stack_size(unsigned int processor_id)
{
    pic_processor *pic = get_processor(processor_id);

    if(!pic)
	return 0;

    return pic->stack->pointer&pic->stack->stack_mask;
}
//--------------------------------------------------------------------------
unsigned int gpsim_get_stack_value(unsigned int processor_id, unsigned int address)
{
    pic_processor *pic = get_processor(processor_id);

    if(!pic)
	return 0xffff;

    return pic->stack->contents[address & pic->stack->stack_mask];
}
//--------------------------------------------------------------------------
void gpsim_set_read_break_at_address(unsigned int processor_id,
				     unsigned int address)
{
    gpsim_toggle_break_at_address(processor_id, address);
    puts("Implement this in interface.cc");sleep(1);
}
//--------------------------------------------------------------------------
void gpsim_set_write_break_at_address(unsigned int processor_id,
				      unsigned int address)
{
    gpsim_toggle_break_at_address(processor_id, address);
    puts("Implement this in interface.cc");sleep(1);
}
//--------------------------------------------------------------------------
void gpsim_set_execute_break_at_address(unsigned int processor_id,
					unsigned int address)
{
    pic_processor *pic = get_processor(processor_id);

    if(!pic)
	return;

    pic->set_break_at_address(address);
}
//--------------------------------------------------------------------------
void gpsim_set_notify_point_at_address(unsigned int processor_id,
				       unsigned int address,
				       void (*cb)(gpointer),
				       gpointer data)
{
    BreakCallBack *callback;

    pic_processor *pic = get_processor(processor_id);

    if(!pic)
	return;

    callback = (BreakCallBack*)create_interface(processor_id, cb, data);

    pic->set_notify_at_address(address, callback);
}
//--------------------------------------------------------------------------
void gpsim_set_profile_start_at_address(unsigned int processor_id,
					unsigned int address,
					void (*cb)(gpointer),
					gpointer data)
{
    BreakCallBack *callback;

    pic_processor *pic = get_processor(processor_id);

    if(!pic)
	return;

    callback = (BreakCallBack*)create_interface(processor_id, cb, data);

    pic->set_profile_start_at_address(address, callback);
}
//--------------------------------------------------------------------------
void gpsim_set_profile_stop_at_address(unsigned int processor_id,
				       unsigned int address,
				       void (*cb)(gpointer),
				       gpointer data)
{
    BreakCallBack *callback;

    pic_processor *pic = get_processor(processor_id);

    if(!pic)
	return;

    callback = (BreakCallBack*)create_interface(processor_id, cb, data);

    pic->set_profile_stop_at_address(address, callback);
}
//--------------------------------------------------------------------------
void gpsim_clear_profile_start_at_address(unsigned int processor_id,
					  unsigned int address)
{
    pic_processor *pic = get_processor(processor_id);

    if(!pic)
	return;

    while(gpsim_address_has_profile_start(processor_id,address))
	pic->clear_profile_start_at_address(address);
}

//--------------------------------------------------------------------------
void gpsim_clear_profile_stop_at_address(unsigned int processor_id,
					 unsigned int address)
{
    pic_processor *pic = get_processor(processor_id);

    if(!pic)
	return;

    while(gpsim_address_has_profile_stop(processor_id,address))
	pic->clear_profile_stop_at_address(address);
}

//--------------------------------------------------------------------------
void gpsim_clear_breakpoints_at_address(unsigned int processor_id,
					unsigned int address)
{
    pic_processor *pic = get_processor(processor_id);

    if(!pic)
	return;

    while(gpsim_address_has_breakpoint(processor_id,address))
	pic->clear_break_at_address(address);
}
//--------------------------------------------------------------------------
void gpsim_toggle_break_at_address(unsigned int processor_id, unsigned int address)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return;

  pic->toggle_break_at_address(address);
}
//--------------------------------------------------------------------------
void gpsim_toggle_break_at_line(unsigned int processor_id, unsigned int file_id, unsigned int line)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return;

  pic->toggle_break_at_line(file_id, line);
}
//--------------------------------------------------------------------------
void gpsim_toggle_break_at_hll_line(unsigned int processor_id, unsigned int file_id, unsigned int line)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return;

  pic->toggle_break_at_hll_line(file_id, line);
}
//--------------------------------------------------------------------------
unsigned int  gpsim_find_closest_address_to_line(unsigned int processor_id, unsigned int file_id, unsigned int line)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return INVALID_VALUE;

  return pic->find_closest_address_to_line(file_id, line);
}
//--------------------------------------------------------------------------
unsigned int  gpsim_find_closest_address_to_hll_line(unsigned int processor_id, unsigned int file_id, unsigned int line)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return INVALID_VALUE;

  return pic->find_closest_address_to_hll_line(file_id, line);
}
//--------------------------------------------------------------------------
unsigned int gpsim_get_file_id(unsigned int processor_id, unsigned int address)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return INVALID_VALUE;
  if(pic->program_memory_size()<=address)
      return INVALID_VALUE;

  if(gpsim_get_hll_mode(processor_id))
      return pic->program_memory[address]->get_hll_file_id();
  else
      return pic->program_memory[address]->get_file_id();

}

//--------------------------------------------------------------------------

struct file_context * gpsim_get_file_context(unsigned int processor_id, unsigned int file_id)
{

  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return NULL;

  return &pic->files[file_id];


}

//--------------------------------------------------------------------------
unsigned int gpsim_get_src_line(unsigned int processor_id, unsigned int address)
{
    unsigned int line;
    
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return INVALID_VALUE;
  if(pic->program_memory_size()<=address)
      return INVALID_VALUE;

  line = pic->program_memory[address]->get_src_line();

  // line 1 is first line (?), so zero or less means invalid line
  if(line<=0)
      return INVALID_VALUE;

  return line;
}

//--------------------------------------------------------------------------
unsigned int gpsim_get_hll_src_line(unsigned int processor_id, unsigned int address)
{
    unsigned int line;
    
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return INVALID_VALUE;
  if(pic->program_memory_size()<=address)
      return INVALID_VALUE;

  line = pic->program_memory[address]->get_hll_src_line();

  // line 1 is first line (?), so zero or less means invalid line
  if(line<=0)
      return INVALID_VALUE;

  return line;
}

//--------------------------------------------------------------------------
unsigned int gpsim_get_number_of_source_files(unsigned int processor_id)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return 0;

  return pic->number_of_source_files;

}

//--------------------------------------------------------------------------
char *gpsim_get_opcode_name(unsigned int processor_id, unsigned int address, char *buffer)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
  {
      return NULL;
  }
  if(pic->program_memory_size()<=address)
      return NULL;

  return pic->program_memory[address]->name(buffer);

}

//--------------------------------------------------------------------------
unsigned int gpsim_get_opcode(unsigned int processor_id, unsigned int address)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return 0;
  if(pic->program_memory_size()<=address)
      return 0;

  //  return pic->program_memory[address]->get_opcode();
  return pic->pma.get_opcode(address);

}

//--------------------------------------------------------------------------
void gpsim_put_opcode(unsigned int processor_id, unsigned int address, unsigned int opcode)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return;

  return pic->pma.put_opcode(address,opcode);

}

//--------------------------------------------------------------------------
// create_interface
//
// Interfaces to gpsim are created to simplify the interactions between
// gpsim and the external world (e.g. the gui code for now). 

gpointer create_interface(unsigned int processor_id, 
			  void (*interface_callback)(gpointer), 
			  gpointer interface_callback_data)
{
  InterfaceObject *io;

  io = new InterfaceObject();

  if(!io)
    return NULL;

  io->pic = get_processor(processor_id);

  if(!io->pic) {
    delete io;
    return NULL;
  }

  io->callback_function = interface_callback;
  io->callback_data = interface_callback_data;
  
  return((gpointer)io);

}

//--------------------------------------------------------------------------
gpointer gpsim_set_cyclic_break_point( unsigned int processor_id, 
				   void (*interface_callback)(gpointer), 
				   gpointer interface_callback_data,
				   guint64 cycle)
{

  CyclicBreakPoint *cbp  = new CyclicBreakPoint();

  if(!cbp)
    return NULL;

  cbp->pic = get_processor(processor_id);

  if(!cbp->pic) {
    delete cbp;
    return NULL;
  }

  cbp->callback_function = interface_callback;
  cbp->callback_data = interface_callback_data;
  cbp->delta_cycles = cycle;
  cbp->set_break();
  
  return((gpointer)cbp);
}

//---------------------------------------------------------------------------
//  gpsim_set_cyclic_break_point2 same as gpsim_set_cyclic_break_point (no '2')
// except that it's assumed the active_pic is the one that will supply the
// cycle counter for the break point

gpointer gpsim_set_cyclic_break_point2(
				   void (*interface_callback)(gpointer), 
				   gpointer interface_callback_data,
				   guint64 cycle)
{

  CyclicBreakPoint *cbp  = new CyclicBreakPoint();

  if(!cbp)
    return NULL;

  cbp->pic = active_cpu;

  if(!cbp->pic) {
    delete cbp;
    return NULL;
  }

  cbp->callback_function = interface_callback;
  cbp->callback_data = interface_callback_data;
  cbp->delta_cycles = cycle;
  cbp->set_break();
  
  return((gpointer)cbp);
}

pic_processor *gpsim_get_active_cpu(void)
{
  return active_cpu;
}

guint64 gpsim_get_current_time(void)
{

  if(active_cpu)
    return active_cpu->cycles.value;
  
  return 0;

}
void  gpsim_set_break_delta(guint64 delta, BreakCallBack *f=NULL)
{
  if(active_cpu)
    active_cpu->cycles.set_break_delta(delta, f);

}

void  gpsim_set_break(guint64 next_cycle, BreakCallBack *f=NULL)
{
  if(active_cpu)
    active_cpu->cycles.set_break(next_cycle, f);

}
//---------------------------------------------------------------------------
//   char *gpsim_get_version(char *dest)
//---------------------------------------------------------------------------
char *gpsim_get_version(char *dest, int max_len)
{

  return( strncpy(dest, GPSIM_VERSION, max_len));

}

//---------------------------------------------------------------------------
//   int gpsim_get_hll_mode(unsigned int processor_id)
//---------------------------------------------------------------------------
int gpsim_get_hll_mode(unsigned int processor_id)
{
  return hll_mode;
}

//---------------------------------------------------------------------------
//   int gpsim_set_hll_mode(unsigned int processor_id, int mode)
//---------------------------------------------------------------------------
int gpsim_set_hll_mode(unsigned int processor_id, int mode)
{
  if(mode==0)
    hll_mode=0;
  if(mode==1)
    hll_mode=1;
  return hll_mode;
}

//---------------------------------------------------------------------------
// pin interface functions
//---------------------------------------------------------------------------
void  gpsim_assign_pin_xref(unsigned int processor_id, unsigned int pin, gpointer xref)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return;

  IOPIN *iopin = pic->get_pin(pin);

  if(!iopin)
    return;

  if(iopin->xref)
      iopin->xref->add(xref);
  else
    cout << "no xref on " << pin << '\n';

}


unsigned int  gpsim_package_pin_count(unsigned int processor_id)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return 0;

  return pic->get_pin_count();

}

char *gpsim_pin_get_name(unsigned int processor_id, unsigned int pin)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return 0;

  return pic->get_pin_name( pin );
}

unsigned int  gpsim_pin_get_value(unsigned int processor_id, unsigned int pin)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return 0;

  IOPIN *iopin = pic->get_pin(pin);

  if(iopin==NULL)
    return 0;

  //return iopin->get_voltage(0);
  return iopin->get_state();
}
void  gpsim_pin_toggle(unsigned int processor_id, unsigned int pin)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return;

  IOPIN *iopin = pic->get_pin(pin);

  if(iopin==NULL)
    return;

  iopin->toggle();

}

unsigned int  gpsim_pin_get_dir(unsigned int processor_id, unsigned int pin)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return 0;

  IOPIN *iopin = pic->get_pin(pin);

  if(!iopin)
    return 0;

  if(iopin->get_direction() == IOPIN::DIR_INPUT)
    return 1;
  else
    return 0;

}
void  gpsim_pin_set_dir(unsigned int processor_id, unsigned int pin, unsigned int new_dir)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return;

  IOPIN *iopin = pic->get_pin(pin);

  if(iopin==NULL)
    return;

  iopin->change_direction( new_dir & 1);//( (new_dir) ?  IOPIN::DIR_INPUT :  IOPIN::DIR_OUTPUT));
}

extern void process_command_file(char * file_name);
extern int open_cod_file(pic_processor **, char *);
#include "../cli/command.h"

int gpsim_open(unsigned int processor_id, char *file)
{
    char *str;
    pic_processor *pic = get_processor(processor_id);

    str = strrchr(file,'.');
    if(str==NULL)
    {
//	puts("found no dot in file!");
	return 0;
    }
    str++;
    if(!strcmp(str,"hex"))
    {

	if(!pic)
	{
	    puts("No pic selected!");
	    return 0;
	}
	pic->load_hex(file);
    
    }
    else if(!strcmp(str,"cod"))
    {

	int i;
	i=load_symbol_file(&pic, file);

	if(i)
	{
	    cout << "found a fatal error in the symbol file " << file <<'\n';
	    return 0;
	}

	// FIXME: questionable
	command_list[0]->cpu=pic;
	trace.switch_cpus(pic);
      
    }
    else if(!strcmp(str,"stc"))
    {

	process_command_file(file);
    }
    else
    {
      cout << "Unknown file extension \"" << str <<"\" \n";
	return 0;
    }

    return 1;
}

//--------------------------------------------------------------------------
//
// callback registration functions
//
//
//--------------------------------------------------------------------------
Interface::Interface(gpointer new_object=NULL)
{

  interface_id = 0;
  objectPTR = new_object;

  update_object = NULL;
  simulation_has_stopped = NULL;
  new_processor = NULL;
  new_program = NULL;

}

Interface *get_interface(unsigned int interface_id)
{

  GSList *interface_list = gi.interfaces;

  while(interface_list) {

    if(interface_list->data) {
      Interface *an_interface = (struct Interface *)(interface_list->data);


      if(an_interface->get_id() == interface_id)
	return an_interface;

    }

    interface_list = interface_list->next;
  }

  return NULL;

}

unsigned int gpsim_register_interface(gpointer new_object)
{
  Interface *an_interface = new Interface(new_object);

  return gi.add_interface(an_interface);

}


void gpsim_register_update_object(unsigned int interface_id,
				  void (*update_object) (gpointer xref,int new_value) )
{
  Interface *an_interface = get_interface(interface_id);

  if(an_interface)
    an_interface->update_object = update_object;
  
}

void gpsim_register_remove_object(unsigned int interface_id, 
				  void (*remove_object) (gpointer xref))
{
  Interface *an_interface = get_interface(interface_id);

  if(an_interface)
    an_interface->remove_object = remove_object;

}

void gpsim_register_new_processor(unsigned int interface_id, 
				  void (*new_processor) (unsigned int processor_id))
{
  Interface *an_interface = get_interface(interface_id);

  if(an_interface)
    an_interface->new_processor = new_processor;
  
}
void gpsim_register_simulation_has_stopped(unsigned int interface_id, 
					   void (*simulation_has_stopped) (gpointer new_object))
{
  Interface *an_interface = get_interface(interface_id);

  if(an_interface)
    an_interface->simulation_has_stopped = simulation_has_stopped;
  
}
void gpsim_register_new_program(unsigned int interface_id, 
				void (*new_program)  (unsigned int processor_id))
{
  Interface *an_interface = get_interface(interface_id);

  if(an_interface)
    an_interface->new_program = new_program;
  
}


//--------------------------------------------------------------------------
//
// gpsimInterface
//
// Here are where the member functions for the gpsimInterface class are
// defined.
//
// The gpsimInterface class contains a singly-linked-list of Interface objects.
// Interface objects are structures that primarily contain pointers to a whole
// bunch of functions. The purpose is to have some external entity, like the 
// gui code, define where these functions point. gpsim will then use these
// functions as a means of notifying the gui when something has changed.
// In addition to the gui, this class also provides the support for interfacing
// to modules. When a module is loaded from a module library, a new Interface
// object is created for it. The module will be given the opportunity to
// register functions (e.g. provide pointers to functions) that gpsim can
// then call.
//
//--------------------------------------------------------------------------


gpsimInterface::gpsimInterface (void )
{

  interfaces = NULL;

}

//--------------------------------------------------------------------------
//
// A xref, or cross reference, object is an arbitrary thing that gpsim
// will pass back to the gui or module. The gui (or module) will then
// interpret the contents of the xref and possibly update some state
// with 'new_value'. An example is when one of the pic registers changes;
// if there's a xref object associated with the register gpsim will
// then notify the gui (or module) through the xref.

void gpsimInterface::update_object (gpointer xref,int new_value)
{

  GSList *interface_list = interfaces;

  while(interface_list) {

    if(interface_list->data) {
      Interface *an_interface = (struct Interface *)(interface_list->data);

      if(an_interface->update_object)
	an_interface->update_object(xref,new_value);
    }

    interface_list = interface_list->next;
  }

}

void gpsimInterface::remove_object (gpointer xref)
{


  GSList *interface_list = interfaces;

  while(interface_list) {

    if(interface_list->data) {
      Interface *an_interface = (struct Interface *)(interface_list->data);

      if(an_interface->remove_object)
	an_interface->remove_object(xref);
    }

    interface_list = interface_list->next;
  }


}
void gpsimInterface::simulation_has_stopped (void)
{

  GSList *interface_list = interfaces;

  profile_keeper.catchup();

  while(interface_list) {

    if(interface_list->data) {
      Interface *an_interface = (struct Interface *)(interface_list->data);

      if(an_interface->simulation_has_stopped)
	an_interface->simulation_has_stopped(an_interface->objectPTR);
    }

    interface_list = interface_list->next;
  }

}
void gpsimInterface::new_processor (unsigned int processor_id)
{

  GSList *interface_list = interfaces;

  while(interface_list) {

    if(interface_list->data) {
      Interface *an_interface = (struct Interface *)(interface_list->data);

      if(an_interface->new_processor)
	an_interface->new_processor(processor_id);
    }

    interface_list = interface_list->next;
  }

}

void gpsimInterface::new_program  (unsigned int processor_id)
{

  GSList *interface_list = interfaces;

  while(interface_list) {

    if(interface_list->data) {
      Interface *an_interface = (struct Interface *)(interface_list->data);

      if(an_interface->new_program)
	an_interface->new_program(processor_id);
    }

    interface_list = interface_list->next;
  }

}

unsigned int  gpsimInterface::add_interface  (Interface *new_interface)
{

  interface_seq_number++;

  new_interface->set_id(interface_seq_number);

  gi.interfaces = g_slist_append(gi.interfaces, new_interface);


  return interface_seq_number;
}
