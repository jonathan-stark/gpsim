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

#include "../config.h"
#include "gpsim_def.h"
#include <unistd.h>
#include <glib.h>

#include "gpsim_classes.h"
#include "modules.h"
#include "processor.h"
#include "xref.h"

#include "fopen-path.h"


SIMULATION_MODES simulation_mode;


//------------------------------------------------------------------------
// active_cpu  is a pointer to the pic processor that is currently 'active'. 
// 'active' means that it's the one currently being simulated or the one
// currently being manipulated by the user (e.g. register dumps, break settings)

Processor *active_cpu=0;


//------------------------------------------------------------------------
//
// Processor - Constructor
//

Processor::Processor(void)
{


  if(verbose)
    cout << "pic_processor constructor\n";

  files = 0;
  pc = 0;

  set_frequency(1.0);

  // add the 'main' pma to the list pma context's. Processors may
  // choose to add multiple pma's to the context list. The gui
  // will build a source browser for each one of these. The purpose
  // is to provide more than one way of debugging the code. (e.g.
  // this is useful for debugging interrupt versus non-interrupt code).

  pma_context.push_back(&pma);

  // Uncomment this to get two identical source browsers in the gui:
  //  pma_context.push_back(&pma);

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

  registers = (Register **) new char[sizeof (Register *) * memory_size];

  if (registers  == 0)
    {
      cout << "*** ERROR *** Out of memory - PIC register space\n";
      exit (1);
    }


  // For processors with banked memory, the register_bank cooresponds to the
  // active bank. Let this point to the beginning of the register array for
  // now.

  register_bank = registers;

  rma.set_cpu(this);
  rma.set_Registers(registers, memory_size);
  
  // Make all of the file registers 'undefined' (each processor derived from this base
  // class defines its own register mapping).

  for (int i = 0; i < memory_size; i++)
    registers[i] = 0;


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
      if (0 == registers[i])
      {
	  registers[i] = new InvalidRegister(i);
	  registers[i]->address = 0;    // BAD_REGISTER;
	  registers[i]->alias_mask = 0;
	  registers[i]->value = 0;	// unimplemented registers are read as 0
	  registers[i]->symbol_alias = 0;

	  registers[i]->cpu = this;
	  //If we are linking with a gui, then initialize a cross referencing
	  //pointer. This pointer is used to keep track of which gui window(s)
	  //display the register. For invalid registers this should always be null.
	  registers[i]->xref = 0;

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

    registers[j] = new Register;
    if (alias_offset) {
      registers[j + alias_offset] = registers[j];
      registers[j]->alias_mask = alias_offset;
    } else
      registers[j]->alias_mask = 0;

    registers[j]->address = j;
    registers[j]->value = 0;
    registers[j]->symbol_alias = 0;

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
	    registers[i] = 0;
      }

      delete registers[j];
      registers[j] = 0;
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

  if (program_memory == 0)
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
      if(program_memory[address] == 0)
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
#if USE_OLD_FILE_CONTEXT == 1
  if(address < program_memory_size())
    {
      //pma[address].update_line_number(file_id,sline,lst_line,0,0);
      program_memory[address]->update_line_number(file_id,sline,lst_line,0,0);
      //printf("%s address=%x, sline=%d, lst_line=%d\n", __FUNCTION__,address,sline,lst_line);
      if(sline > files[file_id].max_line)
	files[file_id].max_line = sline;

      // FIX ME - what is this '+5' junk?

      if(lst_line+5 > files[lst_file_id].max_line)
	files[lst_file_id].max_line = lst_line+5;

    }
#else

  if(address < program_memory_size()) {


    program_memory[address]->update_line_number(file_id,sline,lst_line,0,0);

    printf("%s address=%x, File ID= %d, sline=%d, lst_line=%d\n", __FUNCTION__,address,file_id,sline,lst_line);

    FileContext *fc = (*files)[file_id];

    if(fc && sline > fc->max_line())
      fc->max_line(sline);

    // FIX ME - what is this '+5' junk?

    fc = (*files)[files->list_id()];
    
    if(fc && lst_line+5 > fc->max_line())
      fc->max_line(lst_line+5);

  }

#endif
}

//-------------------------------------------------------------------
// read_src_files - this routine will open all of the source files 
//   associated with the project and associate their line numbers
//   with the addresses of the opcodes they generated.
//

void Processor::read_src_files(void)
{
#if USE_OLD_FILE_CONTEXT == 1

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
      if( 0 == (files[i].file_ptr = fopen_path(files[i].name,"r")))
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
#else

  // Are there any src files ?
  for(int i=0; i<files->nsrc_files(); i++) {


    FileContext *fc = (*files)[i];

    // did this src file generate any code?
    if(fc && fc->max_line() > 0) {
      
      // Create an array whose index corresponds to the
      // line number of a source file line and whose data
      // is the offset (in bytes) from the beginning of the 
      // file. (e.g. files[3].line_seek[20] references the
      // 20th line of the third source file.)

      fc->ReadSource();
    }
  }

#endif
}


//-------------------------------------------------------------------
//
// processor -- list
//
// Display the contents of either a source or list file
//
void Processor::list(int file_id, int pc_val, int start_line, int end_line)
{


  if(!files || files->nsrc_files())
    return;

  if(pc_val > program_memory_size())
    return;

  if(program_memory[pc_val]->isa() == instruction::INVALID_INSTRUCTION)
    {
      cout << "There's no code at address 0x" << hex << pc_val << '\n';
      return;
    }

  int line,pc_line;
  if(file_id)
    {
      file_id = files->list_id();
      line = program_memory[pc_val]->get_lst_line();
      pc_line = program_memory[pc->value]->get_lst_line();
    }
  else
    {
      file_id = program_memory[pc_val]->file_id;
      line = program_memory[pc_val]->get_src_line();
      pc_line = program_memory[pc->value]->get_src_line();
    }

  start_line += line;
  end_line += line;

  FileContext *fc = (*files)[file_id];
  if(fc)
    return;

  if(start_line < 0) start_line = 0;

  if(end_line > fc->max_line())
    end_line = fc->max_line();

  cout << " listing " << fc->name() << " Starting line " << start_line
       << " Ending line " << end_line << '\n';


  for(int i=start_line; i<=end_line; i++)
  {

    char buf[256];

    files->ReadLine(program_memory[i]->file_id,
		    program_memory[i]->src_line - 1,
		    buf,
		    sizeof(buf));

    if (pc_line == i)
      cout << "==>";
    else
      cout << "   ";

    cout << buf;
  }
}
//-------------------------------------------------------------------
//
// disassemble - Disassemble the contents of program memory from
// 'start_address' to 'end_address'. The instruction at the current
// PC is marked with an arrow '==>'. If an instruction has a break
// point set on it then it will be marked with a 'B'. The instruction
// mnemonics come from the class declarations for each instruction.
// However, it is possible to modify this on a per instruction basis.
// In other words, each instruction in the program memory has it's
// own instantiation. So a MOVWF at address 0x20 is different than
// one at address 0x21. It is possible to change the mnemonic of
// one without affecting the other. As of version 0.0.7 though, this
// is not implemented.
//

void Processor::disassemble (int start_address, int end_address)
{
  instruction *inst;
  int use_src_to_disasm =0;

  if(start_address < 0) start_address = 0;
  if(end_address >= pc->memory_size_mask) end_address = pc->memory_size_mask;

  char str[50];

  for(int i = start_address; i<=end_address; i++)
    {
      str[0] =0;
      if (pc->value == i)
	cout << "==>";
      else
	cout << "   ";
      inst = program_memory[i];

      // Breakpoints replace the program memory with an instruction that has
      // an opcode larger than 16 bits.

      if(program_memory[i]->opcode < 0x10000)
	{
	  cout << ' ';
	}
      else
	{
	  cout << 'B';
	  Breakpoint_Instruction *bpi =  (Breakpoint_Instruction *)program_memory[i];
	  inst = bpi->replaced;
	}

      if(files && use_src_to_disasm)
	{
	  char buf[256];

	  files->ReadLine(program_memory[i]->file_id,
			   program_memory[i]->src_line - 1,
			   buf,
			   sizeof(buf));
	  cout << buf;

	}
      else
	cout << hex << setw(4) << setfill('0') << i << "  "
	     << hex << setw(4) << setfill('0') << inst->opcode << "    "
	     << inst->name(str) << '\n';

    }
}


//-------------------------------------------------------------------
int ProgramMemoryAccess::find_closest_address_to_line(int file_id, int src_line)
{
  int closest_address = -1;

  if(!cpu)
    return closest_address;

  int distance = cpu->program_memory_size();

  for(int i = cpu->program_memory_size()-1; i>=0; i--) {

    // Find the closet instruction to the src file line number
    if( (cpu->program_memory[i]->isa() != instruction::INVALID_INSTRUCTION) &&
	(cpu->program_memory[i]->get_file_id()==file_id)                    &&
	(abs(cpu->program_memory[i]->get_src_line() - src_line) < distance))

      {
	distance = abs(cpu->program_memory[i]->get_src_line() - src_line);
	closest_address = i;

	if(distance == 0)
	  return cpu->map_pm_index2address(closest_address);
      }
  }

  return cpu->map_pm_index2address(closest_address);

}
//--------------------------------------------------------------------------
//
// temporary - this could is going to get deleted as soon as file related 
// stuff gets put into its own object/class.

void ProgramMemoryAccess::set_hll_mode(int new_hll_mode)
{
  switch(new_hll_mode) {
  case ASM_MODE:
    hll_mode = ASM_MODE;
    break;
  case HLL_MODE:
    hll_mode = HLL_MODE;
  }
}
//--------------------------------------------------------------------------
unsigned int ProgramMemoryAccess::get_src_line(unsigned int address)
{
  unsigned int line;
    
  if(!cpu || cpu->program_memory_size()<=address)
    return INVALID_VALUE;

  switch(get_hll_mode()) {

  case ASM_MODE:
    line = (this->operator[](address)).get_src_line();
    break;

  case HLL_MODE:
    line = (this->operator[](address)).get_hll_src_line();
    break;
  }

  // line 1 is first line (?), so zero or less means invalid line
  if(line<=0)
    return INVALID_VALUE;

  return line;
}

//--------------------------------------------------------------------------
unsigned int ProgramMemoryAccess::get_file_id(unsigned int address)
{
    
  if(!cpu)
    return INVALID_VALUE;

  switch(get_hll_mode()) {

  case ASM_MODE:
    return (this->operator[](address)).get_file_id();
    break;

  case HLL_MODE:
    return (this->operator[](address)).get_hll_file_id();
    break;
  }

  return INVALID_VALUE;

}

//-------------------------------------------------------------------
void ProgramMemoryAccess::set_break_at_address(int address)
{
  if(hasValid_opcode(address))
    bp.set_execution_break(cpu, address);


}

//-------------------------------------------------------------------
void ProgramMemoryAccess::set_notify_at_address(int address, BreakCallBack *cb)
{
  if(hasValid_opcode(address))
    bp.set_notify_break(cpu, address, cb);

}

//-------------------------------------------------------------------
void ProgramMemoryAccess::set_profile_start_at_address(int address, BreakCallBack *cb)
{
  int pm_index = cpu->map_pm_address2index(address);

  if( pm_index >= 0  && pm_index<cpu->program_memory_size()) 
    if (cpu->program_memory[pm_index]->isa() != instruction::INVALID_INSTRUCTION)
      bp.set_profile_start_break(cpu, address, cb);
}

//-------------------------------------------------------------------
void ProgramMemoryAccess::set_profile_stop_at_address(int address, BreakCallBack *cb)
{
  if(hasValid_opcode(address))
    bp.set_profile_stop_break(cpu, address, cb);
}

//-------------------------------------------------------------------
int ProgramMemoryAccess::clear_break_at_address(int address, 
				       enum instruction::INSTRUCTION_TYPES type = 
				       instruction::BREAKPOINT_INSTRUCTION)
{

  if( address >= 0  && address<cpu->program_memory_size()) {

    instruction *instr = find_instruction(address,type);

    if(instr!=0) {
      int b = ((Breakpoint_Instruction *)instr)->bpn & BREAKPOINT_MASK;
      bp.clear( b );
      return 1;
    } 
  }

  return 0;
}

//-------------------------------------------------------------------
int ProgramMemoryAccess::clear_notify_at_address(int address)
{
  return clear_break_at_address(address,instruction::NOTIFY_INSTRUCTION);
}

//-------------------------------------------------------------------
int ProgramMemoryAccess::clear_profile_start_at_address(int address)
{
  return clear_break_at_address(address,instruction::PROFILE_START_INSTRUCTION);
}

//-------------------------------------------------------------------
int ProgramMemoryAccess::clear_profile_stop_at_address(int address)
{
  return clear_break_at_address(address,instruction::PROFILE_STOP_INSTRUCTION);
}

//-------------------------------------------------------------------
int ProgramMemoryAccess::address_has_break(int address, 
					     enum instruction::INSTRUCTION_TYPES type)
{
  if(find_instruction(address,type)!=0)
    return 1;

  return 0;
}

//-------------------------------------------------------------------
int ProgramMemoryAccess::address_has_notify(int address)
{
  return address_has_break(address,instruction::NOTIFY_INSTRUCTION);
}

//-------------------------------------------------------------------
int ProgramMemoryAccess::address_has_profile_start(int address)
{
  return address_has_break(address,instruction::PROFILE_START_INSTRUCTION);
}

//-------------------------------------------------------------------
int ProgramMemoryAccess::address_has_profile_stop(int address)
{
  return address_has_break(address,instruction::PROFILE_STOP_INSTRUCTION);
}


//-------------------------------------------------------------------
void ProgramMemoryAccess::toggle_break_at_address(int address)
{
  if(address_has_break(address))
    clear_break_at_address(address);
  else
    set_break_at_address(address);
}
//-------------------------------------------------------------------

void ProgramMemoryAccess::set_break_at_line(int file_id, int src_line)
{
  int address;

  if( (address = find_closest_address_to_line(file_id, src_line)) >= 0)
      set_break_at_address(address);
}

void ProgramMemoryAccess::clear_break_at_line(int file_id, int src_line)
{

  int address;

  if( (address = find_closest_address_to_line(file_id, src_line)) >= 0)
      clear_break_at_address(address);
}

void ProgramMemoryAccess::toggle_break_at_line(int file_id, int src_line)
{
  toggle_break_at_address(find_closest_address_to_line(file_id, src_line));
}

//-------------------------------------------------------------------
//
Processor * Processor::construct(void)
{

  cout << " Can't create a generic processor\n";

  return 0;

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
//
// finish
//
// this method really only applies to processors with stacks.

void Processor::finish(void)
{

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
  //  parse_string("dump");

}


//-------------------------------------------------------------------
instruction &ProgramMemoryAccess::operator [] (int address)
{
  //cout << "pma[0x"<< hex << address << "]\n";
  if(!cpu ||cpu->program_memory_size()<=address  || address<0)
    return bad_instruction;

  return *cpu->program_memory[cpu->map_pm_address2index(address)];
}

//-------------------------------------------------------------------
instruction *ProgramMemoryAccess::find_instruction(int address, enum instruction::INSTRUCTION_TYPES type)
{

  if(cpu->program_memory_size()<=address  || address<0)
    return 0;

  instruction &q = this->operator[](address);
  if(q.isa()==instruction::INVALID_INSTRUCTION)
    return 0;


  instruction *p = &q;

  for(;;)
    {
      if(p->isa()==type)
	return p;

      switch(p->isa())
	{
	case instruction::MULTIWORD_INSTRUCTION:
	case instruction::INVALID_INSTRUCTION:
	case instruction::NORMAL_INSTRUCTION:
	  return 0;
	case instruction::BREAKPOINT_INSTRUCTION:
	case instruction::NOTIFY_INSTRUCTION:
	case instruction::PROFILE_START_INSTRUCTION:
	case instruction::PROFILE_STOP_INSTRUCTION:
	  p=((Breakpoint_Instruction *)p)->replaced;
	  break;
	}

    }

  return 0;
}


//-------------------------------------------------------------------
guint64 Processor::cycles_used(unsigned int address)
{
    return program_memory[address]->cycle_count;
}


//-------------------------------------------------------------------
//-------------------------------------------------------------------
//

ProgramMemoryAccess::ProgramMemoryAccess(void)
{
  _address, _opcode, _state = 0;
  hll_mode = ASM_MODE;
}

void ProgramMemoryAccess::init(Processor *new_cpu)
{
  cpu = new_cpu;
  
}

void ProgramMemoryAccess::name(string & new_name)
{
  name_str = new_name;
}

void ProgramMemoryAccess::put(int address, instruction *new_instruction)
{

  if(!new_instruction)
    return;


  if(hasValid_opcode(address)) {

    cpu->program_memory[cpu->map_pm_address2index(address)] = new_instruction;

    if(new_instruction->xref)
      new_instruction->xref->update();
  }

}

instruction *ProgramMemoryAccess::get(int address)
{
  if(address < cpu->program_memory_size())
    return &(this->operator[](address));
  else
    return 0;

}

// like get, but will ignore instruction break points 
instruction *ProgramMemoryAccess::get_base_instruction(int address)
{
    instruction *p;

    p=get(address);

    if(p==0)
        return 0;

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
    return 0;
}

//----------------------------------------
// get_opcode - return an opcode from program memory.
//              If the address is out of range return 0.

unsigned int ProgramMemoryAccess::get_opcode(int addr)
{

  if(addr < cpu->program_memory_size())
    return(cpu->program_memory[addr]->get_opcode());
  else
    return 0;
}


//----------------------------------------
// get_opcode_name - return an opcode name from program memory.
//                   If the address is out of range return 0;

char *ProgramMemoryAccess::get_opcode_name(int addr, char *buffer, int size)
{

  if(addr < cpu->program_memory_size())
    return cpu->program_memory[addr]->name(buffer);

  *buffer = 0;
  return 0;
}

//----------------------------------------
// Get the current value of the program counter.
unsigned int ProgramMemoryAccess::get_PC(void)
{
  if(cpu && cpu->pc)
    return cpu->pc->get_value();

  return 0;
}

void ProgramMemoryAccess::put_opcode_start(int addr, unsigned int new_opcode)
{

  if( (addr < cpu->program_memory_size()) && (_state == 0))
    {
      _state = 1;
      _address = addr;
      _opcode = new_opcode;
      cycles.set_break_delta(40000, this);
      bp.set_pm_write();
    }

}

void ProgramMemoryAccess::put_opcode(int addr, unsigned int new_opcode)
{
  int i;

  if( !(addr>=0) && (addr < cpu->program_memory_size()))
    return;


  instruction *old_inst = get_base_instruction(addr);
  instruction *new_inst = cpu->disasm(addr,new_opcode);

  if(new_inst==0)
  {
      puts("FIXME, in ProgramMemoryAccess::put_opcode");
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

void  ProgramMemoryAccess::assign_xref(unsigned int address, gpointer xref)
{

  instruction &q = this->operator[](address);
  if(q.isa()==instruction::INVALID_INSTRUCTION)
    return;

  if(q.xref)
    q.xref->add(xref);
}

//--------------------------------------------------------------------------
void ProgramMemoryAccess::step(unsigned int steps)
{

  if(!cpu)
    return;

  switch(get_hll_mode()) {

  case ASM_MODE:
    cpu->step(steps);
    break;

  case HLL_MODE:
    {
      unsigned int initial_line = cpu->pma.get_src_line(cpu->pc->get_value());
      unsigned int initial_pc = cpu->pc->get_value();

      while(1)
	{
	  cpu->step(1);

	  if(cpu->pc->get_value()==initial_pc)
	    break;

	  if(get_src_line(cpu->pc->get_value())
	     != initial_line)
	    break;
	}

      break;
    }
  }
}

//--------------------------------------------------------------------------
void ProgramMemoryAccess::step_over(void)
{

  if(!cpu)
    return;

  switch(get_hll_mode()) {

  case ASM_MODE:
    cpu->step_over();
    break;

  case HLL_MODE:
    {

#if 0
      //
      // High level language step-overs are only supported for 
      // pic processors
      //

      pic_processor *pic = dynamic_cast<pic_processor *>cpu;

      if(!pic) {
	cout << "step-over is not supported for non-PIC processors\n";
	return;
      }
      unsigned int initial_line = cpu->pma.get_src_line(cpu->pc->get_value());
      unsigned int initial_pc = cpu->pc->get_value();
      unsigned int initial_stack_depth = pic->stack->pointer&pic->stack->stack_mask;

      while(1)
	{
	  step(1);

	  if(initial_stack_depth < pic->stack->pointer&pic->stack->stack_mask)
	    gpsim_finish(processor_id);

	  if(pic->pc->get_value()==initial_pc)
	    break;

	  if(get_src_line(initial_pc) != initial_line)
	    {
	      if(gpsim_get_file_id(processor_id, pic->pc->get_value()))
		break;
	    }
	}
      break;
#endif
      cout << "HLL Step is not supported\n";
    }
  }

}

//--------------------------------------------------------------------------
void ProgramMemoryAccess::run(void)
{
  cpu->run();

}

//--------------------------------------------------------------------------
void ProgramMemoryAccess::stop(void)
{
  bp.halt();
}

//--------------------------------------------------------------------------
void ProgramMemoryAccess::finish(void)
{
  cpu->finish();
}


//--------------------------------------------------------------------------

bool  ProgramMemoryAccess::hasValid_opcode(unsigned int address)
{

  if((this->operator[](address)).isa() != instruction::INVALID_INSTRUCTION)
    return true;

  return false;
}
//--------------------------------------------------------------------------

bool  ProgramMemoryAccess::isModified(unsigned int address)
{

  if((address < cpu->program_memory_size()) && 
     cpu->program_memory[address]->is_modified)
    return true;

  return false;
}

//========================================================================
// Register Memory Access 

RegisterMemoryAccess::RegisterMemoryAccess(void)
{
  cpu = 0;
  registers = 0;
  nRegisters = 0;
}

//--------------------------------------------------------------------------

Register *RegisterMemoryAccess::get_register(unsigned int address)
{

  if(!cpu || !registers || nRegisters<=address)
    return 0;

  Register *reg = registers[address];

  // If there are breakpoints set on the register, then drill down
  // through them until we get to the real register.

  while(reg->isa() == Register::BP_REGISTER)
    reg = ((Notify_Register *)reg)->replaced;


  return reg;

}

//--------------------------------------------------------------------------
void RegisterMemoryAccess::set_cpu(Processor *p)
{ 
  cpu = p;
}


//--------------------------------------------------------------------------
void RegisterMemoryAccess::set_Registers(Register **_registers, int _nRegisters) 
{ 
  nRegisters = _nRegisters; 
  registers = _registers;
}
//-------------------------------------------------------------------
bool RegisterMemoryAccess::hasBreak(int address)
{

  if(!cpu || !registers || nRegisters<=address)
    return false;

  return registers[address]->isa() == Register::BP_REGISTER;

}

static InvalidRegister AnInvalidRegister;

//-------------------------------------------------------------------
Register &RegisterMemoryAccess::operator [] (int address)
{

  if(!registers || get_size()<=address  || address<0)
    return AnInvalidRegister;

  return *registers[address];
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
  if (processor_list == 0)
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

  return 0;

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


//------------------------------------------------------------------------

FileContext::FileContext(string &new_name, FILE *_fptr)
{
  name_str = new_name;
  fptr = _fptr;
  line_seek = 0;
  _max_line =0;
}

FileContext::FileContext(char *new_name, FILE *_fptr)
{
  name_str = string(new_name);
  fptr = _fptr;
  line_seek = 0;
  _max_line =0;
}

FileContext::~FileContext(void)
{

}

//----------------------------------------
// ReadSource
//
// This will open the file for this FileContext
// and fill the line_seek vector with the file 
// positions corresponding to the start of every
// source line in the file.
//
// e.g. lineseek[20] describes where the 20'th source
// line is in the file.

void FileContext::ReadSource(void)
{

  if( (max_line() < 0) || name_str.length() == 0)
    return;

  const char *str = name_str.c_str();

  // If the file is not open yet, then try to open it.
  if(!fptr)
    fptr = fopen_path(str,"r");

  // If the file still isn't open, then we have a problem
  // FIXME - should some corrective action be taken?
  if(!fptr) {
    cout << "Unable to open " << str << endl;
    return;
  }
  if(line_seek)
    delete line_seek;

  line_seek = new vector<int>(max_line()+1);


  std::rewind(fptr);

  char buf[256],*s;
  (*line_seek)[0] = 0;
  for(int j=1; j<=max_line(); j++) {

    (*line_seek)[j] = ftell(fptr);
    s = fgets(buf,256,fptr);

    if(s != buf)
      break;
  }
}

//----------------------------------------
// ReadLine
// 
// Read one line from a source file.

char *FileContext::ReadLine(int line_number, char *buf, int nBytes)
{

  if(!fptr)
    return 0;

  fseek(fptr, 
	(*line_seek)[line_number],
	SEEK_SET);
  return fgets(buf, nBytes, fptr);

}

//----------------------------------------
//
char *FileContext::gets(char *buf, int nBytes)
{
  if(!fptr)
    return 0;

  return fgets(buf, nBytes, fptr);

}

//----------------------------------------
void FileContext::rewind(void)
{
  if(fptr)
    fseek(fptr,0,SEEK_SET);
}

//----------------------------------------
void FileContext::open(const char *mode)
{
  if(!fptr)
    fptr = fopen_path(name_str.c_str(), mode);

}

//------------------------------------------------------------------------


Files::Files(int nFiles)
{

  vpfile = new vector<FileContext *>(nFiles);
  lastFile=0;

}

Files::~Files(void)
{

  //  for(int i=0; i<vpfile->size(); i++

}

int Files::Find(string &fname)
{
  if(lastFile) {

    for (int i = 0; i < lastFile; i++) {

      if ((*vpfile)[i]->name() == fname)
	return i;

    }
  }

  return -1;

}

int Files::Add(string &new_name, FILE *fptr)
{

  (*vpfile)[lastFile] = new FileContext(new_name,fptr);

  lastFile++;

  cout << "Added new file named: " << new_name 
       << "  id = " << lastFile << endl;

  return lastFile-1;
}

int Files::Add(char *new_name, FILE *fptr)
{

  (*vpfile)[lastFile] = new FileContext(new_name,fptr);

  lastFile++;

  cout << "Added new file named: " << new_name 
       << "  id = " << lastFile << endl;

  return lastFile-1;
}

FileContext *Files::operator [] (int file_id)
{

  if(file_id >= lastFile)
    return 0;

#define GCC_VERSION 296
#if GCC_VERSION == 296
  return (*vpfile)[file_id];   // sigh...
#else
  return (*vpfile).at(file_id);
#endif
}

char *Files::ReadLine(int file_id, int line_number, char *buf, int nBytes)
{
  FileContext *fc = operator[](file_id);
  if(fc)
    return fc->ReadLine(line_number, buf, nBytes);

  return 0;
}

//----------------------------------------
//
char *Files::gets(int file_id, char *buf, int nBytes)
{
  FileContext *fc = operator[](file_id);
  if(fc)

  return fc->gets(buf, nBytes);

}

//----------------------------------------
void Files::rewind(int file_id)
{
  FileContext *fc = operator[](file_id);
  if(fc)
    return fc->rewind();

}
