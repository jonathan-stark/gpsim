/*
   Copyright (C) 1998-2003 T. Scott Dattalo

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

/*
  stuff that needs to be fixed:

  Register aliasing
  The "invalid instruction" in program memory.

*/

#include <stdio.h>
#ifdef _WIN32
#include "uxtime.h"
#else
#include <sys/time.h>
#endif
#include <time.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <list>

#include "../config.h"
#include "gpsim_def.h"
#include <unistd.h>
#include <glib.h>

#include "gpsim_classes.h"
#include "modules.h"
#include "processor.h"
#include "xref.h"

#include "fopen-path.h"

int parse_string(char *cmd_string);

#ifdef HAVE_GUI
#include <gtk/gtk.h>
extern int use_gui;
void gui_refresh(void)
{
	while(gtk_events_pending())
		gtk_main_iteration();
}
#endif

//------------------------------------------------------------------------
//
// Processor - Constructor
//

Processor::Processor(void)
{


  if(verbose)
    cout << "pic_processor constructor\n";

  files = NULL;
  pc = NULL;

  set_frequency(1.0);

  interface = new ProcessorInterface(this);

}

//-------------------------------------------------------------------
//
// init_register_memory (unsigned int memory_size)
//
// Allocate an array for holding register objects.
//

void Processor::init_register_memory (unsigned int memory_size)
{

  if(verbose)
    cout << __FUNCTION__ << " memory size: " << memory_size << '\n';

  registers = (file_register **) new char[sizeof (file_register *) * memory_size];

  if (registers  == NULL)
    {
      cout << "*** ERROR *** Out of memory - PIC register space\n";
      exit (1);
    }


  // For processors with banked memory, the register_bank cooresponds to the
  // active bank. Let this point to the beginning of the register array for
  // now.

  register_bank = registers;

  // Make all of the file registers 'undefined' (each processor derived from this base
  // class defines its own register mapping).

  for (int i = 0; i < memory_size; i++)
    registers[i] = NULL;


}

//-------------------------------------------------------------------
//
//
// create_invalid_registers
//
//   The purpose of this function is to complete the initialization
// of the file register memory by placing an instance of an 'invalid
// file register' at each 'invalid' memory location. Most of PIC's
// do not use the entire address space available, so this routine
// fills the voids.
//

void Processor::create_invalid_registers (void)
{
  int i;

  if(verbose)
    cout << "Creating invalid registers " << register_memory_size()<<"\n";

  // Now, initialize any undefined register as an 'invalid register'
  // Note, each invalid register is given its own object. This enables
  // the simulation code to efficiently capture any invalid register
  // access. Furthermore, it's possible to set break points on
  // individual invalid file registers. By default, gpsim halts whenever
  // there is an invalid file register access.

  for (i = 0; i < register_memory_size(); i++)
    {
      if (NULL == registers[i])
      {
	  registers[i] = new invalid_file_register(i);
	  registers[i]->address = 0;    // BAD_REGISTER;
	  registers[i]->alias_mask = 0;
	  registers[i]->value = 0;	// unimplemented registers are read as 0
	  registers[i]->symbol_alias = NULL;

	  registers[i]->cpu = this;
	  //If we are linking with a gui, then initialize a cross referencing
	  //pointer. This pointer is used to keep track of which gui window(s)
	  //display the register. For invalid registers this should always be null.
	  registers[i]->xref = NULL;

	}
    }

}


//-------------------------------------------------------------------
//    add_file_registers
//
//  The purpose of this member function is to allocate memory for the
// general purpose registers.
//

void Processor::add_file_registers(unsigned int start_address, unsigned int end_address, unsigned int alias_offset)
{

  int j;

  // Initialize the General Purpose Registers:

  char str[100];
  for  (j = start_address; j <= end_address; j++) {

    registers[j] = new file_register;
    if (alias_offset) {
      registers[j + alias_offset] = registers[j];
      registers[j]->alias_mask = alias_offset;
    } else
      registers[j]->alias_mask = 0;

    registers[j]->address = j;
    registers[j]->break_point = 0;
    registers[j]->value = 0;
    registers[j]->symbol_alias = NULL;

    //The default register name is simply its address
    sprintf (str, "0x%02x", j);
    registers[j]->new_name(str);

    registers[j]->cpu = this;

  }

}

//-------------------------------------------------------------------
//    delete_file_registers
//
//  The purpose of this member function is to delete file registers
//

void Processor::delete_file_registers(unsigned int start_address, unsigned int end_address)
{


  //  FIXME - this function is bogus.
  // The aliased registers do not need to be searched for - the alias mask
  // can tell at what addresses a register is aliased.

#define SMALLEST_ALIAS_DISTANCE  32
  int i,j;


  for (j = start_address; j <= end_address; j++) {
    if(registers[j]) {

      if(registers[j]->alias_mask) {
	// This registers appears in more than one place. Let's find all
	// of its aliases.
	for(i=SMALLEST_ALIAS_DISTANCE; i<register_memory_size(); i+=SMALLEST_ALIAS_DISTANCE)
	  if(registers[j] == registers[i])
	    registers[i] = NULL;
      }

      delete registers[j];
      registers[j] = NULL;
    }
  }

}

//-------------------------------------------------------------------
//
// 
//    alias_file_registers
//
//  The purpose of this member function is to alias the
// general purpose registers.
//

void Processor::alias_file_registers(unsigned int start_address, unsigned int end_address, unsigned int alias_offset)
{

  int j;

  // FIXME -- it'd probably make better sense to keep a list of register addresses at
  // which a particular register appears.

  for (j = start_address; j <= end_address; j++)
    {
      if (alias_offset)
	{
	  registers[j + alias_offset] = registers[j];
	  registers[j]->alias_mask = alias_offset;
	}
    }

}



//-------------------------------------------------------------------
//
// init_program_memory(unsigned int memory_size)
//
// The purpose of this member function is to allocate memory for the
// pic's code space. The 'memory_size' parameter tells how much memory
// is to be allocated AND it should be an integer of the form of 2^n. 
// If the memory size is not of the form of 2^n, then this routine will
// round up to the next integer that is of the form 2^n.
//   Once the memory has been allocated, this routine will initialize
// it with the 'bad_instruction'. The bad_instruction is an instantiation
// of the instruction class that chokes gpsim if it is executed. Note that
// there is only one instance of 'bad_instruction'.

void Processor::init_program_memory (unsigned int memory_size)
{

  if(verbose)
    cout << "Initializing program memory: 0x"<<memory_size<<" words\n";

  if ((memory_size-1) & memory_size)
    {
      cout << "*** WARNING *** memory_size should be of the form 2^N\n";

      memory_size = (memory_size + ~memory_size) & MAX_PROGRAM_MEMORY;

      cout << "gpsim is rounding up to memory_size = " << memory_size << '\n';

    }


  // Initialize 'program_memory'. 'program_memory' is a pointer to an array of
  // pointers of type 'instruction'. This is where the simulated instructions
  // are stored.

  program_memory = (instruction **) new char[sizeof (instruction *) * memory_size];

  if (program_memory == NULL)
    {
      cout << "*** ERROR *** Out of memory for program space\n";
      exit (1);
    }


  for (int i = 0; i < memory_size; i++)
    {
      program_memory[i] = &bad_instruction;
      program_memory[i]->cpu = this;     // %%% FIX ME %%% 
    }
}

//-------------------------------------------------------------------
// init_program_memory(int address, int value)
//
// The purpose of this member fucntion is to instantiate an Instruction
// object in the program memory. The opcode is invalid, then a 'bad_instruction'
// is inserted into the program memory instead. If the address is beyond
// the program memory address space, then it may be that the 'opcode' is
// is in fact a configuration word.
//

void Processor::init_program_memory(int address, int value)
{

  if(address < program_memory_size())
    {
      program_memory[address] = disasm(address,value);
      if(program_memory[address] == NULL)
	program_memory[address] = &bad_instruction;
      program_memory[address]->add_line_number_symbol(address);
    }
  else
    set_out_of_range_pm(address,value);  // could be e2prom

}



//-------------------------------------------------------------------
// build_program_memory - given an array of opcodes this function
// will convert them into instructions and insert them into the
// simulated program memory.
//

void Processor::build_program_memory(int *memory,int minaddr, int maxaddr)
{

  for (int i = minaddr; i <= maxaddr; i++)
    if(memory[i] != 0xffffffff)
      init_program_memory(i, memory[i]);

}

//-------------------------------------------------------------------
void Processor::set_out_of_range_pm(int address, int value)
{

  cout << "Warning::Out of range address " << address << " value " << value << endl;
  cout << "Max allowed address is " << (program_memory_size()-1) << '\n';

}


//-------------------------------------------------------------------
//
// attach_src_line - This member function establishes the one-to-one link
// between instructions and the source code that create them.

void Processor::attach_src_line(int address,int file_id,int sline,int lst_line)
{
  if(address < program_memory_size())
    {
      program_memory[address]->update_line_number(file_id,sline,lst_line,0,0);
      printf("%s address=%x, sline=%d, lst_line=%d\n", __FUNCTION__,address,sline,lst_line);
      if(sline > files[file_id].max_line)
	files[file_id].max_line = sline;

      // FIX ME - what is this '+5' junk?

      if(lst_line+5 > files[lst_file_id].max_line)
	files[lst_file_id].max_line = lst_line+5;

    }

}

//-------------------------------------------------------------------
// read_src_files - this routine will open all of the source files 
//   associated with the project and associate their line numbers
//   with the addresses of the opcodes they generated.
//

void Processor::read_src_files(void)
{
  // Are there any src files ?
  for(int i=0; i<number_of_source_files; i++) {


    // did this src file generate any code?
    if(files[i].max_line > 0) {
      
      // Create an array whose index corresponds to the
      // line number of a source file line and whose data
      // is the offset (in bytes) from the beginning of the 
      // file. (e.g. files[3].line_seek[20] references the
      // 20th line of the third source file.)
      files[i].line_seek = new int[files[i].max_line+1];
      if( NULL == (files[i].file_ptr = fopen_path(files[i].name,"r")))
	continue;
      rewind(files[i].file_ptr);

      char buf[256],*s;
      files[i].line_seek[0] = 0;
      for(int j=1; j<=files[i].max_line; j++)
	{
	  files[i].line_seek[j] = ftell(files[i].file_ptr);
	  s = fgets(buf,256,files[i].file_ptr);
	  if(s != buf)
	    break;
	}
    }
  }

}


//-------------------------------------------------------------------
int Processor::find_closest_address_to_line(int file_id, int src_line)
{

  int closest_address = -1;
  int distance = program_memory_size();

  for(int i = program_memory_size()-1; i>=0; i--) {

    // Find the closet instruction to the src file line number
    if(program_memory[i]->isa() != instruction::INVALID_INSTRUCTION &&
       program_memory[i]->get_file_id()==file_id ) {

      if(abs(program_memory[i]->get_src_line() - src_line) < distance) {
	distance = abs(program_memory[i]->get_src_line() - src_line);
	closest_address = i;
	if(distance == 0)
	  break;
      }
    }
  }

  return closest_address;

}

//-------------------------------------------------------------------
int Processor::find_closest_address_to_hll_line(int file_id, int src_line)
{

    int closest_address = -1;
    int closest_distance = 0x7fffffff;

    for(int i = program_memory_size()-1; i>=0; i--)
    {
	// Find the closet instruction to the src file line number
	if(program_memory[i]->isa() != instruction::INVALID_INSTRUCTION &&
	   program_memory[i]->get_hll_file_id()==file_id )
	{
	    if(program_memory[i]->get_hll_src_line() >= src_line &&
	       (program_memory[i]->get_hll_src_line() - src_line) <= closest_distance)
	       {
		   closest_address = i;
		   closest_distance = program_memory[i]->get_hll_src_line() - src_line;
	       }
	}
    }

    return  map_pm_index2address(closest_address);

}



//-------------------------------------------------------------------
void Processor::set_break_at_address(int address)
{
  if( address >= 0)
    if(program_memory[address]->isa() != instruction::INVALID_INSTRUCTION)
      bp.set_execution_break(this, address);
}

//-------------------------------------------------------------------
void Processor::set_notify_at_address(int address, BreakCallBack *cb)
{
  if(address >= 0)
    if (program_memory[address]->isa() != instruction::INVALID_INSTRUCTION)
      bp.set_notify_break(this, address, cb);
}

//-------------------------------------------------------------------
void Processor::set_profile_start_at_address(int address, BreakCallBack *cb)
{
  if( address >= 0)
    if(program_memory[address]->isa() != instruction::INVALID_INSTRUCTION)
      bp.set_profile_start_break(this, address, cb);
}

//-------------------------------------------------------------------
void Processor::set_profile_stop_at_address(int address, BreakCallBack *cb)
{
  if( address >= 0)
    if(program_memory[address]->isa() != instruction::INVALID_INSTRUCTION)
      bp.set_profile_stop_break(this, address, cb);
}

//-------------------------------------------------------------------
int  Processor::clear_break_at_address(int address, 
				       enum instruction::INSTRUCTION_TYPES type = 
				       instruction::BREAKPOINT_INSTRUCTION)
{

  if(program_memory_size()<=address)
    return 0;

  instruction *instr = find_instruction(address,type);

  if(instr!=NULL) {
      int b = ((Breakpoint_Instruction *)instr)->bpn & BREAKPOINT_MASK;
      bp.clear( b );
      return 1;
  } 

  return 0;
    

}

//-------------------------------------------------------------------
int  Processor::clear_notify_at_address(int address)
{
  return clear_break_at_address(address,instruction::NOTIFY_INSTRUCTION);
}

//-------------------------------------------------------------------
int Processor::clear_profile_start_at_address(int address)
{
  return clear_break_at_address(address,instruction::PROFILE_START_INSTRUCTION);
}

//-------------------------------------------------------------------
int Processor::clear_profile_stop_at_address(int address)
{
  return clear_break_at_address(address,instruction::PROFILE_STOP_INSTRUCTION);
}

//-------------------------------------------------------------------
int Processor::address_has_break(int address, 
				     enum instruction::INSTRUCTION_TYPES type)
{
  if(program_memory_size()<=address)
    return 0;

  if(find_instruction(address,type)!=NULL)
    return 1;

  return 0;
}

//-------------------------------------------------------------------
int Processor::address_has_notify(int address)
{
  return address_has_break(address,instruction::NOTIFY_INSTRUCTION);
}

//-------------------------------------------------------------------
int Processor::address_has_profile_start(int address)
{
  return address_has_break(address,instruction::PROFILE_START_INSTRUCTION);
}

//-------------------------------------------------------------------
int Processor::address_has_profile_stop(int address)
{
  return address_has_break(address,instruction::PROFILE_STOP_INSTRUCTION);
}


//-------------------------------------------------------------------
void Processor::toggle_break_at_address(int address)
{
  if(address_has_break(address))
    clear_break_at_address(address);
  else
    set_break_at_address(address);
}
//-------------------------------------------------------------------

void Processor::set_break_at_line(int file_id, int src_line)
{
  int address;

  if( (address = find_closest_address_to_line(file_id, src_line)) >= 0)
      set_break_at_address(address);
}

void Processor::clear_break_at_line(int file_id, int src_line)
{

  int address;

  if( (address = find_closest_address_to_line(file_id, src_line)) >= 0)
      clear_break_at_address(address);
}

void Processor::toggle_break_at_line(int file_id, int src_line)
{


  toggle_break_at_address(find_closest_address_to_line(file_id, src_line));


}

//-------------------------------------------------------------------

void Processor::set_break_at_hll_line(int file_id, int src_hll_line)
{
  int address;

  if( (address = find_closest_address_to_hll_line(file_id, src_hll_line)) >= 0)
      set_break_at_address(address);
}

void Processor::clear_break_at_hll_line(int file_id, int src_hll_line)
{

  int address;

  if( (address = find_closest_address_to_hll_line(file_id, src_hll_line)) >= 0)
      clear_break_at_address(address);
}

void Processor::toggle_break_at_hll_line(int file_id, int src_hll_line)
{


  toggle_break_at_address(find_closest_address_to_hll_line(file_id, src_hll_line));


}



//-------------------------------------------------------------------
//
Processor * Processor::construct(void)
{

  cout << " Can't create a generic processor\n";

  return NULL;

}




//-------------------------------------------------------------------
//
// run  -- Begin simulating and don't stop until there is a break.
//
void Processor::run (void)
{

  cout << __FUNCTION__ << endl;
}

//-------------------------------------------------------------------
//
// step - Simulate one (or more) instructions. If a breakpoint is set
// at the current PC-> 'step' will go right through it. (That's supposed
// to be a feature.)
//

void Processor::step (unsigned int steps)
{
  if(simulation_mode != STOPPED) {
    if(verbose)
      cout << "Ignoring step request because simulation is not stopped\n";
    return;
  }

  simulation_mode = SINGLE_STEPPING;
  do
    {

      if(bp.have_sleep() || bp.have_pm_write())
	{
	  // If we are sleeping or writing to the program memory (18cxxx only)
	  // then step one cycle - but don't execute any code  

	  cycles.increment();
	  trace.dump(1);

	}
      else if(bp.have_interrupt())
	{
	  interrupt();
	}
      else
	{

	  step_one();
	  trace.cycle_counter(cycles.value);
	  trace.dump_last_instruction();

	} 

    }
  while(!bp.have_halt() && --steps>0);

  bp.clear_halt();
  simulation_mode = STOPPED;

  gi.simulation_has_stopped();
}

//-------------------------------------------------------------------
//
// step_over - In most cases, step_over will simulate just one instruction.
// However, if the next instruction is a branching one (e.g. goto, call, 
// return, etc.) then a break point will be set after it and gpsim will
// begin 'running'. This is useful for stepping over time-consuming calls.
//

void Processor::step_over (void)
{
  step(1); // Try one step
}


//-------------------------------------------------------------------

void Processor::run_to_address (unsigned int destination)
{ 
  
  
  if(simulation_mode != STOPPED) {
    if(verbose)
      cout << "Ignoring run-to-address request because simulation is not stopped\n";
    return;
  }
  // Set a temporary break point
  
  unsigned int bp_num = bp.set_execution_break(this, destination);
  run();
  bp.clear(bp_num);
     
}    

//-------------------------------------------------------------------
//
// 
//    create
//
//  The purpose of this member function is to 'create' a pic processor.
// Since this is a base class member function, only those things that
// are common to all pics are created.

void Processor::create (void)
{
  cout << " a generic processor cannot be created " << __FILE__ << __LINE__ <<endl;
  exit(1);
}

//-------------------------------------------------------------------
void Processor::dump_registers (void)
{
  parse_string("dump");

}


//-------------------------------------------------------------------
instruction *Processor::find_instruction(int address, enum instruction::INSTRUCTION_TYPES type)
{
    instruction *p;

    if(program_memory_size()<=address)
	return NULL;

    p=program_memory[address];

    if(p==NULL)
        return NULL;

    if(p->isa()==instruction::INVALID_INSTRUCTION)
        return NULL;

    for(;;)
    {
	if(p->isa()==type)
	    return p;

	switch(p->isa())
	{
	case instruction::MULTIWORD_INSTRUCTION:
	case instruction::INVALID_INSTRUCTION:
	case instruction::NORMAL_INSTRUCTION:
            return NULL;
	case instruction::BREAKPOINT_INSTRUCTION:
	case instruction::NOTIFY_INSTRUCTION:
	case instruction::PROFILE_START_INSTRUCTION:
	case instruction::PROFILE_STOP_INSTRUCTION:
	    p=((Breakpoint_Instruction *)p)->replaced;
            break;
	}

    }

    return NULL;
}


//-------------------------------------------------------------------
guint64 Processor::cycles_used(unsigned int address)
{
    return program_memory[address]->cycle_count;
}

//-------------------------------------------------------------------
guint64 Processor::register_read_accesses(unsigned int address)
{
    return registers[address]->read_access_count;
}

//-------------------------------------------------------------------
guint64 Processor::register_write_accesses(unsigned int address)
{
    return registers[address]->write_access_count;
}




//-------------------------------------------------------------------
//-------------------------------------------------------------------
//
void program_memory_access::put(int addr, instruction *new_instruction)
{

  cpu->program_memory[addr] = new_instruction;

  if(cpu->program_memory[addr]->xref)
      cpu->program_memory[addr]->xref->update();


}

instruction *program_memory_access::get(int addr)
{
  if(addr < cpu->program_memory_size())
    return(cpu->program_memory[addr]);
  else
    return NULL;

}

// like get, but will ignore instruction break points 
instruction *program_memory_access::get_base_instruction(int addr)
{
    instruction *p;

    p=get(addr);

    if(p==NULL)
        return NULL;

    for(;;)
    {
	switch(p->isa())
	{
	case instruction::MULTIWORD_INSTRUCTION:
	case instruction::INVALID_INSTRUCTION:
	case instruction::NORMAL_INSTRUCTION:
            return p;
	case instruction::BREAKPOINT_INSTRUCTION:
	case instruction::NOTIFY_INSTRUCTION:
	case instruction::PROFILE_START_INSTRUCTION:
	case instruction::PROFILE_STOP_INSTRUCTION:
	    p=((Breakpoint_Instruction *)p)->replaced;
            break;
	}

    }
    return NULL;
}

unsigned int program_memory_access::get_opcode(int addr)
{

  //  cout << __FUNCTION__ << "  get_opcode: 0x" << cpu->program_memory[addr]->get_opcode() <<
  //  " opcode (direct): 0x" << cpu->program_memory[addr]->opcode << '\n';

  if(addr < cpu->program_memory_size())
    return(cpu->program_memory[addr]->get_opcode());
  else
    return 0;
}


//----------------------------------------
// Get the current value of the program counter.
unsigned int program_memory_access::get_PC(void)
{
  return 0;
}

void program_memory_access::put_opcode_start(int addr, unsigned int new_opcode)
{

  if( (addr < cpu->program_memory_size()) && (state == 0))
    {
      state = 1;
      address = addr;
      opcode = new_opcode;
      cycles.set_break_delta(40000, this);
      bp.set_pm_write();
    }

}

void program_memory_access::put_opcode(int addr, unsigned int new_opcode)
{
  int i;

  if( !(addr>=0) && (addr < cpu->program_memory_size()))
    return;


  instruction *old_inst = get_base_instruction(addr);
  instruction *new_inst = cpu->disasm(addr,new_opcode);

  if(new_inst==NULL)
  {
      puts("FIXME, in program_memory_access::put_opcode");
      return;
  }
  
  if(!old_inst) {
    put(addr,new_inst);
    return;
  }

  if(old_inst->isa() == instruction::INVALID_INSTRUCTION) {
    put(addr,new_inst);
    return;
  }

  // Now we need to make sure that the instruction we are replacing is
  // not a multi-word instruction. The 12 and 14 bit cores don't have
  // multi-word instructions, but the 16 bit cores do. If we are replacing
  // the second word of a multiword instruction, then we only need to
  // 'uninitialize' it. 

  // if there was a breakpoint set at addr, save a pointer to the breakpoint.
  Breakpoint_Instruction *b=bpi;
  instruction *prev = get_base_instruction(addr-1);

  if(prev)
    prev->initialize(false);

  new_inst->update_line_number(old_inst->get_file_id(), 
			       old_inst->get_src_line(), 
			       old_inst->get_lst_line(),
			       old_inst->get_hll_src_line(),
			       old_inst->get_hll_file_id());

  new_inst->xref = old_inst->xref;

  if(b) 
    b->replaced = new_inst;
  else
    cpu->program_memory[addr] = new_inst;

  cpu->program_memory[addr]->is_modified=1;
  
  if(cpu->program_memory[addr]->xref)
      cpu->program_memory[addr]->xref->update();
  
  delete(old_inst);
}

//--------------------------------------------------------------------------

void  program_memory_access::assign_xref(unsigned int address, gpointer xref)
{

  if(cpu->program_memory_size()<=address)
    return;

  if(cpu->program_memory[address] && cpu->program_memory[address]->xref)
      cpu->program_memory[address]->xref->add(xref);

}

//--------------------------------------------------------------------------

bool  program_memory_access::isValid_opcode(unsigned int address)
{

  if((address < cpu->program_memory_size()) && 
     (cpu->program_memory[address]->isa() != instruction::INVALID_INSTRUCTION))
    return true;

  return false;
}
//--------------------------------------------------------------------------

bool  program_memory_access::isModified(unsigned int address)
{

  if((address < cpu->program_memory_size()) && 
     cpu->program_memory[address]->is_modified)
    return true;

  return false;
}
//========================================================================
// Processor Constructor

list <ProcessorConstructor *> *ProcessorConstructor::processor_list;

ProcessorConstructor::ProcessorConstructor(  Processor * (*_cpu_constructor) (void),
			 char *name1, 
			 char *name2, 
			 char *name3,
			 char *name4) 
{
  if (processor_list == NULL)
    processor_list = new list <ProcessorConstructor *>;

  cpu_constructor = _cpu_constructor;  // Pointer to the processor constructor
  names[0] = name1;                    // First name
  names[1] = name2;                    //  and three aliases...
  names[2] = name3;
  names[3] = name4;

  // Add the processor to the list of supported processors:

  processor_list->push_back(this);

}


//------------------------------------------------------------
// find -- search through the list of supported processors for
//         the one matching 'name'.


ProcessorConstructor *ProcessorConstructor::find(char *name)
{


  list <ProcessorConstructor *> :: iterator processor_iterator;

  for (processor_iterator = processor_list->begin();
       processor_iterator != processor_list->end(); 
       processor_iterator++) {

    ProcessorConstructor *p = *processor_iterator;

    for(int j=0; j<nProcessorNames; j++)
      if(p->names[j] && strcmp(name,p->names[j]) == 0)
	return p;
  }

  return NULL;

}

//------------------------------------------------------------
// dump() --  Print out a list of all of the processors
//

void ProcessorConstructor::dump(void)
{

  list <ProcessorConstructor *> :: iterator processor_iterator;

  const int nPerRow = 4;   // Number of names to print per row.

  int i,j,k,longest;

  ProcessorConstructor *p;


  // loop through all of the processors and find the 
  // one with the longest name

  longest = 0;

  for (processor_iterator = processor_list->begin();  
       processor_iterator != processor_list->end(); 
       processor_iterator++) {

    p = *processor_iterator;

    k = strlen(p->names[1]);
    if(k>longest)
      longest = k;

  }


  // Print the name of each processor.

  for (processor_iterator = processor_list->begin();  
       processor_iterator != processor_list->end(); ) {

    for(i=0; i<nPerRow && processor_iterator != processor_list->end(); i++) {

      p = *processor_iterator++;
      cout << p->names[1];

      if(i<nPerRow-1) {

	// if this is not the last processor in the column, then
	// pad a few spaces to align the columns.

	k = longest + 2 - strlen(p->names[1]);
	for(j=0; j<k; j++)
	  cout << ' ';
      }
    }
    cout << '\n';
  } 

}
