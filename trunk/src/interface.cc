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
#include <glib.h>

#include "gpsim_def.h"
#include "pic-processor.h"
#include "p16x8x.h"
#include "xref.h"
#include "interface.h"

unsigned int gpsim_is_initialized = 0;  // Flag to tell us when all of the init stuff is done.


extern "C" {
unsigned int gpsim_get_register_memory_size(unsigned int processor_id,REGISTER_TYPE type);
  }

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

void (*update_object) (gpointer xref,int new_value);
void (*remove_object) (gpointer xref);
void (*simulation_has_stopped) (void);
void (*new_processor) (unsigned int processor_id);
//void (*new_program)  (pic_processor *p);
void (*new_program)  (unsigned int p);

pic_processor *get_processor(unsigned int cpu_id);

unsigned int processor_has_eeprom(pic_processor *pic)
{
  return ((pic->eeprom_get_size() > 0) ? TRUE : FALSE);
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
    if(!processor_has_eeprom(pic))
      return 0;

      if(register_number>=pic->eeprom_get_size())
	  return FALSE;
      return TRUE;
  }
  
  if(register_number < pic->register_memory_size())
    {
      file_register *reg = pic->registers[register_number];

      
      while(reg->isa() == file_register::BP_REGISTER)
	{
	  reg = ((Breakpoint_Register *)reg)->replaced;
	}


      if(reg->isa() == file_register::INVALID_REGISTER)
	return 0;
      else
	return 1;
    }
  else
    printf("Warning: Request for register 0x%x in processor %s is out of range\n",register_number,pic->name());


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
    return pic->eeprom_get_register(register_number);
    break;
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

int gpsim_register_is_alias(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number)
{
  file_register *fr = gpsim_get_register( processor_id, type,  register_number);

  if(fr) 
    if(fr->address!=register_number)
      return TRUE;

  return FALSE;

}

//--------------------------------------------------------------------------

int gpsim_register_is_sfr(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number)
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

unsigned int gpsim_get_w(unsigned int processor_id)
{

  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return INVALID_VALUE;

  return pic->W.get_value();

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

  if(verbose & 0x8)
    printf("%s()\n",__FUNCTION__);

  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return 0;

  if(verbose & 0x8)
    printf("%s() - valid pic\n",__FUNCTION__);

  if(!valid_register(pic,type,0))
      return 0;
  
  if(verbose & 0x8)
    printf("%s() - valid register\n",__FUNCTION__);

  if(type == REGISTER_EEPROM)
    return pic->eeprom_get_size();
  
  if(verbose & 0x8)
    printf("%s(), register memory size 0x%x\n",__FUNCTION__, pic->register_memory_size());

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

    return 1;
}
//--------------------------------------------------------------------------
unsigned int gpsim_address_has_breakpoint(unsigned int processor_id, unsigned int address)
{
 pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return 0;

  if(pic->program_memory[address]->isa() == instruction::BREAKPOINT_INSTRUCTION)
    return 1;

  return 0;
}
//--------------------------------------------------------------------------

void  gpsim_assign_program_xref(unsigned int processor_id, unsigned int address, gpointer xref)
{

  pic_processor *pic = get_processor(processor_id);

  if(!pic)
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
void gpsim_step(unsigned int processor_id, unsigned int steps)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic || (steps==0))
    return;

  pic->step(steps);
}
//--------------------------------------------------------------------------
void gpsim_step_over(unsigned int processor_id)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return;

  pic->step_over();
}
//--------------------------------------------------------------------------
void gpsim_run(unsigned int processor_id)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return;

  pic->run();
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
}
//--------------------------------------------------------------------------
void gpsim_return(unsigned int processor_id)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return;

  // this should exit current subroutine (like 'finish' in gdb)
  puts("Return not implemented");
}
//--------------------------------------------------------------------------
void gpsim_run_to_address(unsigned int processor_id, unsigned int address)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return;

  pic->run_to_address(address);
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
unsigned int  gpsim_find_closest_address_to_line(unsigned int processor_id, unsigned int file_id, unsigned int line)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return INVALID_VALUE;

  return pic->find_closest_address_to_line(file_id, line);
}
//--------------------------------------------------------------------------
unsigned int gpsim_get_file_id(unsigned int processor_id, unsigned int address)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return INVALID_VALUE;

  pic->program_memory[address]->get_file_id();

}

//--------------------------------------------------------------------------

struct file_context * gpsim_get_file_context(unsigned int processor_id, unsigned int file_id)
{

  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return 0;

  return &pic->files[file_id];


}

//--------------------------------------------------------------------------
unsigned int gpsim_get_src_line(unsigned int processor_id, unsigned int address)
{
    unsigned int line;
    
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
    return INVALID_VALUE;

  line = pic->program_memory[address]->get_src_line();

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
      puts("no pic");
      sleep(5);
      return NULL;
  }

  return pic->program_memory[address]->name(buffer);

}

//--------------------------------------------------------------------------
unsigned int gpsim_get_opcode(unsigned int processor_id, unsigned int address)
{
  pic_processor *pic = get_processor(processor_id);

  if(!pic)
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
      printf("no xref on %d\n",pin);

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
#include <cli/command.h>

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
//	printf("load hex file \"%s\"\n",file);
	if(!pic)
	{
	    puts("No pic selected!");
	    return 0;
	}
	pic->load_hex(file);
    
    }
    else if(!strcmp(str,"cod"))
    {
//	printf("load symbol file \"%s\"\n",file);
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
//	printf("load command file \"%s\"\n",file);
	process_command_file(file);
    }
    else
    {
	printf("Unknown file extension \"%s\" \n",str);
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
void gpsim_interface_init(void)
{

  update_object = NULL;
  simulation_has_stopped = NULL;
  new_processor = NULL;
  new_program = NULL;
}
