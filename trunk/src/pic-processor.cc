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
along with gpasm; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

//#include <stdio.h>
#include <stdio.h>
#include <iostream.h>
#include <iomanip.h>
#include <string>
#include <list>

#include "gpsim_def.h"
#include "pic-processor.h"
#include "picdis.h"
#include "symbol.h"
#include "stimuli.h"
#include "p16x5x.h"
#include "p16x8x.h"
#include "p16f87x.h"
#include "p16x6x.h"
#include "p16x7x.h"
#include "p12x.h"
#include "p18x.h"

#include "16bit-instructions.h" // this is only needed for pma class

#include "xref.h"

int parse_string(char *cmd_string);

SIMULATION_MODES simulation_mode;
guint64 simulation_start_cycle;


//================================================================================
// Global Declarations
//  FIXME -  move these global references somewhere else

// don't print out a bunch of garbage while initializing
int     verbose=0;

// Number of simulation cycles between calls to refresh the gui
guint64 gui_update_rate = DEFAULT_GUI_UPDATE_RATE;

//================================================================================
//
// pic_processor
//
// This file contains all (most?) of the code that simulates those features 
// common to all pic microcontrollers.
//
//

//-------------------------------------------------------------------
//
// Define a list for keeping track of the processors being simulated.
// (Recall, gpsim can simultaneously simulate more than one processor.)

list <pic_processor *> processor_list;
list <pic_processor *> :: iterator processor_iterator;

// active_cpu  is a pointer to the pic processor that is currently 'active'. 
// 'active' means that it's the one currently being simulated or the one
// currently being manipulated by the user (e.g. register dumps, break settings)

pic_processor *active_cpu=NULL;

// active_cpu_id is the id of the currently active cpu. In other words:
//  active_cpu_id == active_cpu->processor_id
// It's redundant to define this id in addition to the *active_cpu pointer.
// However, if there ever comes a day when the cli is truely separate from
// the simulator, then it would be more convenient to deal with ints than
// pointers.

static  int  active_cpu_id = 0;

// cpu_ids is a counter that increments everytime a processor is added by the
// user. If the user deletes a processor, this counter will not be affected.
// The value of this counter will be assigned to the processor's id when a
// new processor is added. It's purpose is to uniquely identifier user 
// processors.

static  int  cpu_ids = 0;

//-------------------------------------------------------------------
//
//
// this sorta sucks, but everytime a new processor is added to the
// gpsim source code, another definition needs to be specified here
// in the 'available_processors' array. 


processor_types available_processors[] =
{
  {_PIC_PROCESSOR_, 
   "generic_pic", "generic_pic", "generic_pic", "generic_pic",
   pic_processor::construct },
  {_14BIT_PROCESSOR_,
   "14bit_pic", "14bit_pic", "14bit_pic", "14bit_pic",
   pic_processor::construct },
  {_12BIT_PROCESSOR_,
   "12bit_pic", "12bit_pic", "12bit_pic", "12bit_pic",
   pic_processor::construct },
  {_16BIT_PROCESSOR_,
   "16bit_pic", "16bit_pic", "16bit_pic", "16bit_pic",
   pic_processor::construct },
  {_P12C508_,
   "__12C508", "pic12c508",  "p12c508", "12c508",
   P12C508::construct },
  {_P12C509_,
   "__12C509", "pic12c509",  "p12c509", "12c509",
   P12C509::construct },
  {_P16C84_, 
   "__16C84",  "pic16c84",   "p16c84",  "16c84",
   P16C84::construct },
  {_P16CR83_,
   "__16CR83", "pic16cr83",  "p16cr83", "16cr83",
   P16CR83::construct },
  {_P16CR84_,
   "__16CR84", "pic16cr84",  "p16cr84", "16cr84",
   P16CR84::construct },
  {_P16F83_,
   "__16F83",   "pic16f83",   "p16f83",  "16f83",
   P16F83::construct },
  {_P16F84_,
   "__16F84",   "pic16f84",   "p16f84",  "16f84",
   P16F84::construct },
  {_P16C54_,
   "__16C54",   "pic16c54",   "p16c54",  "16c54",
   P16C54::construct },
  {_P16C61_,
   "__16C61",   "pic16c61",   "p16c61",  "16c61",
   P16C61::construct },
  {_P16C71_,
   "__16C71",   "pic16c71",   "p16c71",  "16c71",
   P16C71::construct },
  {_P16C62_,
   "__16C62",   "pic16c62",   "p16c62",  "16c62",
   pic_processor::construct },
  {_P16C62A_,
   "__16C62A", "pic16c62a",  "p16c62a", "16c62a",
   pic_processor::construct },
  {_P16CR62_,
   "__16CR62", "pic16cr62",  "p16cr62", "16cr62",
   pic_processor::construct },
  {_P16C63_,
   "__16C63",   "pic16c63",   "p16c63",  "16c63",
   pic_processor::construct },
  {_P16C64_,
   "__16C64",   "pic16c64",   "p16c64",  "16c64",
   P16C64::construct },
  {_P16C64A_,
   "__16C64A", "pic16c64a",  "p16c64a", "16c64a",
   pic_processor::construct },
  {_P16CR64_,
   "__16CR64", "pic16cr64",  "p16cr64", "16cr64",
   pic_processor::construct },
  {_P16C65A_,
   "__16C65A", "pic16c65a",  "p16c65a", "16c65a",
   P16C65::construct },
  {_P16C65_,
   "__16C65",   "pic16c65",   "p16c65",  "16c65",
    P16C65::construct },
  {_P16C74_,
   "__16C74",   "pic16c74",   "p16c74",  "16c74",
   P16C74::construct },
  {_P16F874_,
   "__16F874", "pic16f874",  "p16f874", "16f874",
   P16F874::construct },
  {_P16F877_,
   "__16F877", "pic16f877",  "p16f877", "16f877",
   P16F877::construct },
  {_P18Cxx2_,
   "__18Cxx2", "pic18cxx2",  "p18cxx2", "18cxx2",
   pic_processor::construct },
  {_P18C2x2_,
   "__18C2x2", "pic18c2x2",  "p18c2x2", "18c2x2",
   pic_processor::construct },
  {_P18C242_,
   "__18C242", "pic18c242",  "p18c242", "18c242",
   P18C242::construct },
  {_P18C252_,
   "__18C252", "pic18c252",  "p18c252", "18c252",
   P18C252::construct },
  {_P18C442_,
   "__18C442", "pic18c442",  "p18c442", "18c442",
   pic_processor::construct },
  {_P18C452_,
   "__18C452", "pic18c452",  "p18c452", "18c452",
   pic_processor::construct }

};

int number_of_available_processors = sizeof(available_processors) / sizeof(processor_types);

void dump_processor_list(void)
{

  cout << "Processor List\n";

  bool have_processors = 0;

  for (processor_iterator = processor_list.begin();  processor_iterator != processor_list.end(); processor_iterator++)
    {
      pic_processor *p = *processor_iterator;
      cout << p->name_str << '\n';
      have_processors = 1;
    }

  if(!have_processors)
    cout << "(empty)\n";
}

//---------------------------------------------------------------

void switch_active_cpu(int cpu_id)
{

  bool have_processors = 0;

  for (processor_iterator = processor_list.begin();  processor_iterator != processor_list.end(); processor_iterator++)
    {
      have_processors = 1;
      pic_processor *p = *processor_iterator;
      if(p->processor_id == cpu_id) 
	{
	  active_cpu = p;
	  active_cpu_id = cpu_id;
	  cout << "Switching cpus: ID = "<<cpu_id << " name = " <<  p->name_str << '\n';
	  break;
	}
    }

  if(!have_processors)
    cout << "(empty)\n";
}

//-------------------------------------------------------------------
//
//

void display_available_processors(void)
{

  int number_of = sizeof(available_processors) / sizeof(processor_types);
  int i,j,k,l,longest;

  for(i=0,longest=0; i<number_of; i++)
    {
      k = strlen(available_processors[i].names[1]);
      if(k>longest)
	longest = k;
    }

  k=0;
  do
    {

      for(i=0; (i<4) && (k<number_of); i++)
	{
	  cout << available_processors[k].names[1];
	  if(i<3)
	    {
	      l = longest + 2 - strlen(available_processors[k].names[1]);
	      for(j=0; j<l; j++)
		cout << ' ';
	    }
	  k++;
	}
      cout << '\n';
    } while (k < number_of);

}

//-------------------------------------------------------------------
int find_in_available_processor_list(char * processor_type)
{

  int number_of = sizeof(available_processors) / sizeof(processor_types);
  int i,j;

  for(i=0; i<number_of; i++)
    for(j=0; j<4; j++)
      if(strcmp(processor_type,available_processors[i].names[j]) == 0)
	{
	  return(i);
	}

  return(-1);

}


//-------------------------------------------------------------------

pic_processor *get_processor(unsigned int cpu_id)
{

  if(cpu_id)
    {

      // No need in searching the list if they're requesting the active cpu.

      if(active_cpu_id == cpu_id)
	return active_cpu;

      // Search the cpu list
      for (processor_iterator = processor_list.begin();  processor_iterator != processor_list.end(); processor_iterator++)
	{
	  pic_processor *p = *processor_iterator;
	  if(p->processor_id == cpu_id) 
	    return(p);
	}

      if(gpsim_is_initialized)
	cout << "Processor ID " << hex << cpu_id << "was not found\n";

      return NULL;
    }

  //  cout << "Processor ID is NULL (and that's bad)\n";

  return NULL;
}


//-------------------------------------------------------------------
pic_processor * add_processor(char * processor_type, char * processor_new_name)
{
  if(verbose)
    cout << "Trying to add new processor '" << processor_type << "' named '" 
	 << processor_new_name << "'\n";

  int i = find_in_available_processor_list(processor_type);

  if(i>= 0)
    {

      pic_processor *p = available_processors[i].cpu_constructor();

      if(p)
	{
	  processor_list.push_back(p);
	  active_cpu = p;
	  p->processor_id = active_cpu_id = ++cpu_ids;
	  if(verbose) {
	    cout << processor_type << '\n';
	    cout << "Program Memory size " <<  p->program_memory_size() << '\n';
	    cout << "Register Memory size " <<  p->register_memory_size() << '\n';
	  }

	  // Tell the gui or any modules that are interfaced to gpsim
	  // that a new processor has been declared.
	  gi.new_processor(p->processor_id);

	  return p;
	}
      else
	{
	  cout << " unable to add a processor (BUG?)\n";
	}
    }
  else
    {
      cout << processor_type << " is not a valid processor.\n(try 'processor list' to see a list of valid processors.\n";
    }

  return(NULL);
}


//-------------------------------------------------------------------
//
pic_processor * pic_processor::construct(void)
{

  cout << " Can't create a generic pic processor\n";

  return NULL;

}
//-------------------------------------------------------------------
//
// sleep - Begin sleeping and stay asleep until something causes a wake
//

void pic_processor::sleep (void)
{
  simulation_mode = SLEEPING;

  do
    {
      cycles.increment();   // burn cycles until something wakes us
    } while(bp.have_sleep()); // %%%FIX ME%%% ===>  && !bp.have_halt());

  simulation_mode = RUNNING;

}

//-------------------------------------------------------------------
//
// sleep - Begin sleeping and stay asleep until something causes a wake
//

void pic_processor::pm_write (void)
{
  //  simulation_mode = PM_WRITE;

  do
    {
      cycles.increment();     // burn cycles until we're through writing
    } while(bp.have_pm_write());

  simulation_mode = RUNNING;

}

//-------------------------------------------------------------------
//
// run  -- Begin simulating and don't stop until there is a break.
//

void pic_processor::run (void)
{

  simulation_mode = RUNNING;

  // If the first instruction we're simulating is a break point, then ignore it.

  if(program_memory[pc.value]->isa() == instruction::BREAKPOINT_INSTRUCTION)
    {
      simulation_start_cycle = cycles.value;
    }

  do
    {
      do
	{
	  program_memory[pc.value]->execute();
	} while(!bp.global_break);

      if(bp.have_interrupt())
	interrupt();

      if(bp.have_sleep())
	sleep();

      if(bp.have_pm_write())
	pm_write();

    } while(!bp.global_break);

  bp.clear_global();
  trace.dump_last_instruction();

  simulation_mode = STOPPED;

  gi.simulation_has_stopped();

}

//-------------------------------------------------------------------
//
// step - Simulate one (or more) instructions. If a breakpoint is set
// at the current PC, 'step' will go right through it. (That's supposed
// to be a feature.)
//

void pic_processor::step (unsigned int steps)
{


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

	  program_memory[pc.value]->execute();
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

void pic_processor::step_over (void)
{

  unsigned int saved_pc = pc.value;

  step(1); // Try one step

  if( ! ( (pc.value >= saved_pc) && (pc.value <= saved_pc+2) ) )
    {
      if(program_memory[pc.value]->isa() == instruction::BREAKPOINT_INSTRUCTION)
	return;
      else
	{
	  // Set a break point at the instruction just past the one over which we are stepping
	  unsigned int bp_num = bp.set_execution_break(this, saved_pc + 1);
	  run();
	  bp.clear(bp_num);
	}
    }

  // note that we don't need to tell the gui to update its windows since
  // that is already done by step() or run().

}


//-------------------------------------------------------------------

void pic_processor::run_to_address (unsigned int destination)
{ 
  
  unsigned int saved_pc = pc.value;
  
  // Set a temporary break point
  
  unsigned int bp_num = bp.set_execution_break(this, destination);
  run();
  bp.clear(bp_num);
     
}    


//-------------------------------------------------------------------
//
// reset - reset the pic based on the desired reset type.
//

void pic_processor::reset (RESET_TYPE r)
{

  if(r == SOFT_RESET)
    {
      trace.reset(r);
      pc.reset();
      gi.simulation_has_stopped();
      cout << " --- Soft Reset (not fully implemented)\n";
      return;
    }

  cout << " --- Reset\n";

  for(int i=0; i<register_memory_size(); i++)
    {
      if(registers[i]->isa() == file_register::SFR_REGISTER)
	{
	  sfr_register *reg = (sfr_register *)registers[i];
	  if(r == POR_RESET)
	    reg->value = reg->por_value;
	  else if (r==WDT_RESET)
	    reg->value = reg->wdtr_value;
	}
    }


  trace.reset(r);
  pc.reset();
  stack->reset();
  bp.clear_global();

  if(r == POR_RESET)
    {
      status.put_TO(1);
      status.put_PD(1);

      if(verbose) {
	cout << "POR\n";
	cout << "config_modes = " << config_modes << '\n';
      }

      wdt.initialize( (config_modes & CM_WDTE) != 0, nominal_wdt_timeout);
    }
  else if (r==WDT_RESET)
    status.put_TO(0);

  gi.simulation_has_stopped();
}

//-------------------------------------------------------------------
//
// por - power on reset %%% FIX ME %%% needs lots of work...
//

void pic_processor::por(void)
{

  wdt.initialize( (config_modes & WDTE) != 0, nominal_wdt_timeout);

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

void pic_processor::disassemble (int start_address, int end_address)
{
  instruction *inst;
  int use_src_to_disasm =0;

  if(start_address < 0) start_address = 0;
  if(end_address >= pc.memory_size_mask) end_address = pc.memory_size_mask;

  char str[50];

  for(int i = start_address; i<=end_address; i++)
    {
      if (pc.value == i)
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

      if((files != NULL) && (use_src_to_disasm))
	{
	  int fid = program_memory[i]->file_id;
	  char buf[256];
	  //cout << " in " << files[fid].name << " @line " 
	  //     << program_memory[i]->src_line << '\n';
	  fseek(files[fid].file_ptr, 
		files[fid].line_seek[program_memory[i]->src_line - 1],
		SEEK_SET);
	  fgets(buf, 256, files[fid].file_ptr);
	  cout << buf;
	}
      else
	cout << hex << setw(4) << setfill('0') << i << "  "
	     << hex << setw(4) << setfill('0') << inst->opcode << "    "
	     << inst->name(str) << '\n';

    }
}

//-------------------------------------------------------------------
//
// pic_processor -- list
//
// Display the contents of either a source or list file
//
void pic_processor::list(int file_id, int pc_val, int start_line, int end_line)
{


  if((number_of_source_files == 0) || (files == NULL))
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
      file_id = lst_file_id;
      line = program_memory[pc_val]->get_lst_line();
      pc_line = program_memory[pc.value]->get_lst_line();
    }
  else
    {
      file_id = program_memory[pc_val]->file_id;
      line = program_memory[pc_val]->get_src_line();
      pc_line = program_memory[pc.value]->get_src_line();
    }

  start_line += line;
  end_line += line;

  if(start_line < 0) start_line = 0;
  if(end_line > files[file_id].max_line)
    end_line = files[file_id].max_line;

  cout << " listing " << files[file_id].name << " Starting line " << start_line
       << " Ending line " << end_line << '\n';

  // Make sure that the file is open
  if(files[file_id].file_ptr == NULL)
    {
      if(files[file_id].name != NULL)
	files[file_id].file_ptr = fopen(files[file_id].name,"r");

      if(files[file_id].file_ptr == NULL)
	{
	  if(files[file_id].name != NULL)
	    cout << files[file_id].name;
	  else
	    cout << ".lst file";

	  cout  << " was not found\n";

	  return;
	}
    }

  for(int i=start_line; i<=end_line; i++)
    {
      char buf[256];
      fseek(files[file_id].file_ptr, 
	    files[file_id].line_seek[i],
	    SEEK_SET);
      fgets(buf, 256, files[file_id].file_ptr);

      if (pc_line == i)
	cout << "==>";
      else
	cout << "   ";

      cout << buf;
    }
}

//-------------------------------------------------------------------
//
// pic_processor -- constructor
//

pic_processor::pic_processor(void)
{

  if(verbose)
    cout << "pic_processor constructor\n";
  files = NULL;
  config_modes = 0;

}


invalid_file_register::invalid_file_register(unsigned int at_address)
{

  char name_str[100];
  sprintf (name_str, "invalid fr  0x%02x", at_address);
  new_name(name_str);
}

//-------------------------------------------------------------------
//
// 
//    create
//
//  The purpose of this member function is to 'create' a pic processor.
// Since this is a base class member function, only those things that
// are common to all pics are created.

void pic_processor :: create (void)
{

  init_program_memory (program_memory_size());

  init_register_memory (register_memory_size());

  //  create_invalid_registers ();
  create_symbols();
  create_stack();

  // Now, initialize the core stuff:
  pc.cpu = this;
  //  cycles.cpu = this;
  wdt.cpu = this;

  W.new_name("W");
  fsr.new_name("fsr");

  register_bank = &registers[0];  // Define the active register bank 
  W.value = 0;

  set_frequency(10e6);            // 
  nominal_wdt_timeout = 18e-3;    // 18ms according to the data sheet (no prescale)
  Vdd = 5.0;                      // Assume 5.0 volt power supply

  trace.program_counter (pc.value);


}

//-------------------------------------------------------------------
//    add_file_registers
//
//  The purpose of this member function is to allocate memory for the
// general purpose registers.
//

void pic_processor::add_file_registers(unsigned int start_address, unsigned int end_address, unsigned int alias_offset)
{

  int j;

  // Initialize the General Purpose Registers:

  char str[100];
  for (j = start_address; j <= end_address; j++)
    {
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

void pic_processor::delete_file_registers(unsigned int start_address, unsigned int end_address)
{

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

void pic_processor::alias_file_registers(unsigned int start_address, unsigned int end_address, unsigned int alias_offset)
{

  int j;

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
//
// create_invalid_registers
//
//   The purpose of this function is to complete the initialization
// of the file register memory by placing an instance of an 'invalid
// file register' at each 'invalid' memory location. Most of PIC's
// do not use the entire address space available, so this routine
// fills the voids.
//

void pic_processor::create_invalid_registers (void)
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

  //for (i = 0; i < 16*FILE_REGISTERS; i++)
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
//
// add_sfr_register
//
// The purpose of this routine is to add one special function register
// to the file registers. If the sfr has a physical address (like the 
// status or tmr0 registers) then a pointer to that register will be
// placed in the file register map. 

void pic_processor::add_sfr_register(sfr_register *reg, unsigned int addr,
				      unsigned int por_value=0, char *new_name=NULL)
{

  reg->cpu         = this;
  if(addr < register_memory_size())
    {
      registers[addr] = reg;
      registers[addr]->address = addr;
      registers[addr]->alias_mask = 0;
      if(new_name)
	registers[addr]->new_name(new_name);

    }

  reg->value       = por_value;
  reg->por_value   = por_value;
  reg->wdtr_value  = por_value;
  reg->break_point = 0;              // %%% FIX ME %%% Is this still needed?
  reg->initialize();
}

//-------------------------------------------------------------------
//
// init_program_memory
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

void pic_processor::init_program_memory (unsigned int memory_size)
{

  if(verbose)
    cout << "Initializing program memory: 0x"<<memory_size<<" words\n";
  pc.memory_size_mask = memory_size - 1;
  if ((pc.memory_size_mask) & memory_size)
    {
      cout << "*** WARNING *** memory_size should be of the form 2^N\n";

      memory_size = (memory_size + ~memory_size) & MAX_PROGRAM_MEMORY;
      pc.memory_size_mask = memory_size - 1;

      cout << "gpsim is rounding up to memory_size = " << memory_size << '\n';

    }

  // Initialize 'program_memory'. 'program_memory' is a pointer to an array of
  // pointers of type 'instruction'. This is where the simulated instructions
  // are stored.

  program_memory = (instruction **) new char[sizeof (instruction *) * memory_size];

  pma.cpu = this;

  if (program_memory == NULL)
    {
      cout << "*** ERROR *** I can't get enough memory for the PIC program space\n";
      exit (1);
    }


  for (int i = 0; i < memory_size; i++)
    {
      program_memory[i] = &bad_instruction;
      program_memory[i]->cpu = this;     // %%% FIX ME %%% 
    }
}

//-------------------------------------------------------------------
//
void pic_processor::init_register_memory (unsigned int memory_size)
{

  if(verbose)
    cout << __FUNCTION__ << " memory size: " << memory_size << '\n';

  // Allocate enough memory for the entire register space (e.g. 256 registers for 14-bit core)

  //registers = (file_register **) new char[sizeof (file_register *) * 16*FILE_REGISTERS];
  registers = (file_register **) new char[sizeof (file_register *) * memory_size];

  if (registers  == NULL)
    {
      cout << "*** ERROR *** I can't get enough memory for the PIC register space\n";
      exit (1);
    }

  // The register_bank points to the currently active bank of registers as defined
  // by the paging bits in the status register. Most of the pics have only 2 banks,
  // however some of the 5x parts have 4 and there's a possibility that this could
  // increase to 8 (if it hasn't already).

  register_bank = registers;

  // Make all of the file registers 'undefined' (each processor derived from this base
  // class defines its own register mapping).

  //for (int i = 0; i < 16*FILE_REGISTERS; i++)
  for (int i = 0; i < memory_size; i++)
    registers[i] = NULL;


}
//-------------------------------------------------------------------
//
// Add a symbol table entry for each one of the sfr's
//

void pic_processor::create_symbols (void)
{

  if(verbose)
    cout << __FUNCTION__ << " register memory size = " << register_memory_size() << '\n';

  for(int i = 0; i<register_memory_size(); i++)
    {
      switch (registers[i]->isa()) {
      case file_register::SFR_REGISTER:
	if(!symbol_table.find(registers[i]->name()))
	  symbol_table.add_register(this, registers[i]);
      }
    }
}

//-------------------------------------------------------------------
void pic_processor::dump_registers (void)
{
  parse_string("dump");

}

//-------------------------------------------------------------------
void pic_processor::attach_src_line(int address,int file_id,int sline,int lst_line)
{

  if(address < program_memory_size())
    {
      program_memory[address]->update_line_number(file_id,sline,lst_line);

      if(sline > files[file_id].max_line)
	files[file_id].max_line = sline;

      if(lst_line+5 > files[lst_file_id].max_line)
	files[lst_file_id].max_line = lst_line+5;

    }

}

//-------------------------------------------------------------------
// read_src_files - this routine will open all of the source files 
//   associated with the project and associate their line numbers
//   with the addresses of the opcodes they generated.
//

void pic_processor::read_src_files(void)
{
  // Are there any src files ?
  for(int i=0; i<number_of_source_files; i++)
    {

      // did this src file generate any code?
      if(files[i].max_line > 0)
	{
	  // Create an array whose index corresponds to the
	  // line number of a source file line and whose data
	  // is the offset (in bytes) from the beginning of the 
	  // file. (e.g. files[3].line_seek[20] references the
	  // 20th line of the third source file.)
	  files[i].line_seek = new int[files[i].max_line];
	  if( NULL == (files[i].file_ptr = fopen(files[i].name,"r")))
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
int pic_processor::find_closest_address_to_line(int file_id, int src_line)
{

  int closest_address = -1;

  for(int i = program_memory_size()-1; i>=0; i--)
    {
      // Find the closet instruction to the src file line number
	if(program_memory[i]->isa() != instruction::INVALID_INSTRUCTION &&
	   program_memory[i]->get_file_id()==file_id )
	{
	  if(program_memory[i]->get_src_line() >= src_line)
	    closest_address = i;
	}
    }

  return closest_address;

}

//-------------------------------------------------------------------

void pic_processor::toggle_break_at_address(int address)
{

  int b;

  if( address >= 0)
    {

      if(program_memory[address]->isa() == instruction::BREAKPOINT_INSTRUCTION)
	{
	  b = ((Breakpoint_Instruction *)(program_memory[address]))->bpn & BREAKPOINT_MASK;
	  bp.clear( b );
	  //cout << "cleared ";
	}
      else
	{
	  b = bp.set_execution_break(this, address);
	  //cout << "set ";
	}

    }
}

//-------------------------------------------------------------------

void pic_processor::set_break_at_line(int file_id, int src_line)
{
  int address;

  if( (address = find_closest_address_to_line(file_id, src_line)) >= 0)
    {

      int b = bp.set_execution_break(this, address);

      if(b < MAX_BREAKPOINTS)
	{
	  //cout << "break at address: " << address << " break #: " << b << '\n';
//	    if(program_memory[address]->xref)
//		program_memory[address]->xref->update();
	}
      else
	cout << "failed to set execution break (check the address)\n";

    }

}

void pic_processor::clear_break_at_line(int file_id, int src_line)
{

  int address;
  int b;

  if( (address = find_closest_address_to_line(file_id, src_line)) >= 0)
    {

      if(program_memory[address]->isa() == instruction::BREAKPOINT_INSTRUCTION)
	{
	  b = ((Breakpoint_Instruction *)(program_memory[address]))->bpn & BREAKPOINT_MASK;
	  bp.clear( b );
	  //cout << "cleared break at address: " << address << " break #: " << b << '\n';

//	    if(program_memory[address]->xref)
//		program_memory[address]->xref->update();
	}
      else
	cout << "break point wasn't found at address: 0x" << address << '\n';
    }
}

void pic_processor::toggle_break_at_line(int file_id, int src_line)
{


  toggle_break_at_address(find_closest_address_to_line(file_id, src_line));


}

//-------------------------------------------------------------------
void    pic_processor::set_out_of_range_pm(int address, int value)
{

  cout << "Warning::Out of range address " << address << " value " << value << endl;
  cout << "Max allowed address is " << (program_memory_size()-1) << '\n';

}

//-------------------------------------------------------------------
void pic_processor::init_program_memory(int address, int value)
{
  if(address < program_memory_size())
    {
      program_memory[address] = disasm(address,value);
      if(program_memory[address] == NULL)
	program_memory[address] = &bad_instruction;
      program_memory[address]->add_line_number_symbol(address);
    }
  else if(address == config_word_address())
    {
      set_config_word(value);
    }
  else
    set_out_of_range_pm(address,value);  // could be e2prom

}

//-------------------------------------------------------------------
void pic_processor::build_program_memory(int *memory,int minaddr, int maxaddr)
{

  /*  if(maxaddr > program_memory_size()) {
    cout << "pic_processor::build_program_memory - maxaddr is tooooo big\n";
    cout << hex << maxaddr << "  " << program_memory_size() << '\n';
    return;
  }
  */

  for (int i = minaddr; i <= maxaddr; i++)
    {
      if(memory[i] != 0xffffffff)
	init_program_memory(i, memory[i]);

    }

}
//-------------------------------------------------------------------

void pic_processor::set_config_word(unsigned int cfg_word)
{
  config_word = cfg_word;

  // Clear all of the configuration bits in config_modes and then
  // reset each of them based on the config bits in cfg_word:
  config_modes &= ~(CM_WDTE);
  config_modes |= ( (cfg_word & WDTE) ? CM_WDTE : 0);
  cout << " setting cfg_word and cfg_modes " << hex << config_word << "  " << config_modes << '\n';
}

//-------------------------------------------------------------------
//
// load_hex
//
extern int readihex16 (pic_processor *cpu, FILE * file);

void pic_processor::load_hex (char *hex_file)
{

  FILE *inputfile = fopen (hex_file, "r");

  if(verbose)
    cout << "load hex\n";

  if (inputfile == NULL)
    {
      cout << "Error: Couldn't open " << hex_file << '\n';
      return;
    }

  //  int *memory = new int [MAXPICSIZE];
  //int minaddr, maxaddr;

  // Fill the memory array with bogus data
  //for(int i = 0; i<MAXPICSIZE; i++)
  //  memory[i] = 0xffffffff;

  set_config_word(0xffff);  // assume no configuration word is in the hex file.

  if (!readihex16 (this, inputfile))
    {
      // No errors were found in the hex file.

      fclose (inputfile);

      //build_program_memory(memory, minaddr, maxaddr);

      // Tell the gui that we've got some code.
      gi.new_program(processor_id);

      //disassemble(minaddr, maxaddr);
      if(verbose)
	cout << "Configuration word = 0x"  << setw(4) << setfill('0') << get_config_word() << '\n';

      set_frequency(10e6);
      //set_config_word(::config_word);
      reset(POR_RESET);

      simulation_mode = STOPPED;

      if(verbose)
	cycles.dump_breakpoints();
    }

      //  delete(memory);

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
  instruction *inst;

  inst = get(addr);

  if(!inst)
    return inst;

  if(inst->isa() != instruction::BREAKPOINT_INSTRUCTION)
    return inst;

  int i = 0;

  do {
    i++;
    bpi = (Breakpoint_Instruction *) inst;
    inst = bpi->replaced;
  } while( (inst->isa() == instruction::BREAKPOINT_INSTRUCTION) && (i<10));

  return inst;
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

void program_memory_access::put_opcode_start(int addr, unsigned int new_opcode)
{

  if( (addr < cpu->program_memory_size()) && (state == 0))
    {
      state = 1;
      address = addr;
      opcode = new_opcode;
      cpu->cycles.set_break_delta(40000, this);
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

  if(prev) {
    if(prev->isa() == instruction::MULTIWORD_INSTRUCTION) {
      ((multi_word_instruction *)prev)->initialized = 0;
    }
  }
  

  new_inst->update_line_number(old_inst->get_file_id(), 
			       old_inst->get_src_line(), 
			       old_inst->get_lst_line());

  new_inst->xref = old_inst->xref;

  if(b) 
    b->replaced = new_inst;
  else
    cpu->program_memory[addr] = new_inst;

  if(cpu->program_memory[addr]->xref)
      cpu->program_memory[addr]->xref->update();
  
  delete(old_inst);
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void program_memory_access::callback(void)
{

  if(state)
    {
      state = 0;
      //cout << __FUNCTION__ << " address= " << address << ", opcode= " << opcode << '\n';
      //cpu->program_memory[address]->opcode = opcode;
      put_opcode(address,opcode);
      trace.opcode_write(address,opcode);
      bp.clear_pm_write();
    }

}
