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
#include "processor.h"

//------------------------------------------------------------------------
//
// Processor - Constructor
//

Processor::Processor(void)
{


  if(verbose)
    cout << "pic_processor constructor\n";

  pc = NULL; //new Program_Counter();

  files = NULL;

  eeprom = NULL;
  config_modes = create_ConfigMode();
  set_frequency(DEFAULT_PIC_CLOCK);
  pll_factor = 0;
  // Test code for logging to disk:
  trace_log.switch_cpus(this);


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

void pic_processor::alias_file_registers(unsigned int start_address, unsigned int end_address, unsigned int alias_offset)
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

  // The memory_size_mask is used by the branching instructions 

  pc->memory_size_mask = memory_size - 1;

  // Initialize 'program_memory'. 'program_memory' is a pointer to an array of
  // pointers of type 'instruction'. This is where the simulated instructions
  // are stored.

  program_memory = (instruction **) new char[sizeof (instruction *) * memory_size];

  pma.cpu = this;

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
//
Processor * Processor::construct(void)
{

  cout << " Can't create a generic processor\n";

  return NULL;

}


//-------------------------------------------------------------------
//
// sleep - Begin sleeping and stay asleep until something causes a wake
//

void Processor::sleep (void)
{
  simulation_mode = SLEEPING;

  if(!bp.have_sleep())
    return;

  do
    {
      cycles.increment();   // burn cycles until something wakes us
    } while(bp.have_sleep() && !bp.have_halt());

  if(!bp.have_sleep())
    pc->increment();

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

int realtime_mode=0;
int realtime_mode_with_gui=0;
extern void update_gui(void);

class RealTimeBreakPoint : public BreakCallBack
{
public:
  pic_processor *cpu;
  struct timeval tv_start;
  guint64 cycle_start;
  guint64 future_cycle;
  int warntimer;
  int period;

  RealTimeBreakPoint(void)
  {
    cpu = NULL;
    warntimer = 1;
    period = 1;
    future_cycle = 0;
  }

  void start(pic_processor *active_cpu) 
  {
    if(!active_cpu)
      return;

    // Grab the system time and record the simulated pic's time.
    // We'll then set a break point a short time in the future
    // and compare how the two track.

    cpu = active_cpu;

    gettimeofday(&tv_start,NULL);

    cycle_start=cpu->cycles.value;

    guint64 fc = cycle_start+100;

    cout << "real time start : " << future_cycle << '\n';

    if(future_cycle)
      cpu->cycles.reassign_break(future_cycle, fc, this);
    else
      cpu->cycles.set_break(fc, this);

    future_cycle = fc;

  }

  void stop(void)
  {

    // Clear any pending break point.
    cout << "real time stop : " << future_cycle << '\n';

    if(future_cycle) {
      cout << " real time clearing\n";
      cpu->cycles.clear_break(this);
      future_cycle = 0;
    }
      
  }

  void callback(void)
  {
    gint64 system_time;
    double diff;
    struct timeval tv;

    // We just hit the break point. A few moments ago we 
    // grabbed a snap shot of the system time and the simulated
    // pic's time. Now we're going to compare the two deltas and
    // see how well they've tracked. If the host is running
    // way faster than the PIC, we'll put the host to sleep 
    // briefly.


    gettimeofday(&tv,NULL);

    system_time = (tv.tv_sec-tv_start.tv_sec)*1000000+(tv.tv_usec-tv_start.tv_usec); // in micro-seconds

    diff = system_time - ((cpu->cycles.value-cycle_start)*4.0e6*cpu->period);

    guint64  idiff;
    if( diff < 0 )
    {
	// we are simulating too fast

        idiff = (guint64)(-diff/4);

	if(idiff>1000)
	    period -= idiff/500;
	if(period<1)
            period=1;

	// Then sleep for a while
	if(idiff)
	  usleep(idiff);
    }
    else
    {
      idiff = (guint64)(diff/4);

	if(idiff>1000)
	    period+=idiff/500;
	if(period>10000)
            period=10000;

	if(idiff>1000000)
	{
	    // we are simulating too slow
	    if(warntimer<10)
		warntimer++;
	    else
	    {
		warntimer=0;
		puts("Processor is too slow for realtime mode!");
	    }
	}
	else
            warntimer=0;
    }

    guint64 delta_cycles= (guint64)(100*period*cpu->frequency/4000000);
    if(delta_cycles<1)
      delta_cycles=1;

    // Look at realtime_mode_with_gui and update the gui if true
    if(realtime_mode_with_gui)
    {
	update_gui();
    }


    guint64 fc = cpu->cycles.value + delta_cycles;

    if(future_cycle)
      cpu->cycles.reassign_break(future_cycle, fc, this);
    else
      cpu->cycles.set_break(fc, this);

    future_cycle = fc;

  }

};

RealTimeBreakPoint realtime_cbp;


//-------------------------------------------------------------------
//
// run  -- Begin simulating and don't stop until there is a break.
//
void pic_processor::run (void)
{
  if(use_icd)
  {
      simulation_mode=RUNNING;
      icd_run();
      while(!icd_stopped())
      {
#ifdef HAVE_GUI
	  if(use_gui)
	      gui_refresh();
#endif
      }
      simulation_mode=STOPPED;
      disassemble(pc->get_value(), pc->get_value());
      gi.simulation_has_stopped();
      return;
  }


  if(simulation_mode != STOPPED) {
    if(verbose)
      cout << "Ignoring run request because simulation is not stopped\n";
    return;
  }

  simulation_mode = RUNNING;

  if(realtime_mode)
    realtime_cbp.start(active_cpu);

  // If the first instruction we're simulating is a break point, then ignore it.

  if(find_instruction(pc->value,instruction::BREAKPOINT_INSTRUCTION)!=NULL)
    {
      simulation_start_cycle = cycles.value;
    }

  do
    {
      do
	{
	  program_memory[pc->value]->execute();
	} while(!bp.global_break);

      if(bp.have_interrupt())
	interrupt();

      if(bp.have_sleep())
	sleep();

      if(bp.have_pm_write())
	pm_write();

    } while(!bp.global_break);

  if(realtime_mode)
    realtime_cbp.stop();

  bp.clear_global();
  trace.cycle_counter(cycles.value);
  trace.dump_last_instruction();

  simulation_mode = STOPPED;

  gi.simulation_has_stopped();

}

//-------------------------------------------------------------------
//
// step - Simulate one (or more) instructions. If a breakpoint is set
// at the current PC-> 'step' will go right through it. (That's supposed
// to be a feature.)
//

void pic_processor::step (unsigned int steps)
{

  if(use_icd)
  {
      if(steps!=1)
      {
	  cout << "Can only step one step in ICD mode"<<endl;
      }
      icd_step();
      pc->get_value();
      disassemble(pc->value, pc->value); // FIXME, don't want this in HLL ICD mode.
      gi.simulation_has_stopped();
      return;
  }

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

	  program_memory[pc->value]->execute();
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

void pic_processor::step_over (void)
{

  unsigned int saved_pc = pc->value;

  if(simulation_mode != STOPPED) {
    if(verbose)
      cout << "Ignoring step-over request because simulation is not stopped\n";
    return;
  }

  step(1); // Try one step

  if( ! ( (pc->value >= saved_pc) && (pc->value <= saved_pc+2) ) )
    {
        if(find_instruction(pc->value,instruction::BREAKPOINT_INSTRUCTION)!=NULL)
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
  
  unsigned int saved_pc = pc->value;
  
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
// reset - reset the pic based on the desired reset type.
//

void pic_processor::reset (RESET_TYPE r)
{

  if(use_icd)
  {
      puts("RESET");
      icd_reset();
      disassemble(pc->get_value(), pc->get_value());
      gi.simulation_has_stopped();
      return;
  }

  if(r == SOFT_RESET)
    {
      trace.reset(r);
      pc->reset();
      gi.simulation_has_stopped();
      cout << " --- Soft Reset (not fully implemented)\n";
      return;
    }

  cout << " --- Reset\n";

  for(int i=0; i<register_memory_size(); i++)
    registers[i]->reset(r);


  trace.reset(r);
  pc->reset();
  stack->reset();
  bp.clear_global();

  if(r == POR_RESET)
    {
      status->put_TO(1);
      status->put_PD(1);

      if(verbose) {
	cout << "POR\n";
	if(config_modes) config_modes->print();
      }
      if(config_modes)
	wdt.initialize( config_modes->get_wdt() , nominal_wdt_timeout);
    }
  else if (r==WDT_RESET)
    status->put_TO(0);

  gi.simulation_has_stopped();
}

//-------------------------------------------------------------------
//
// por - power on reset %%% FIX ME %%% needs lots of work...
//

void pic_processor::por(void)
{
  if(config_modes)
    wdt.initialize( config_modes->get_wdt(), nominal_wdt_timeout);

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
  if(end_address >= pc->memory_size_mask) end_address = pc->memory_size_mask;

  char str[50];

  for(int i = start_address; i<=end_address; i++)
    {
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

  if(start_line < 0) start_line = 0;
  if(end_line > files[file_id].max_line)
    end_line = files[file_id].max_line;

  cout << " listing " << files[file_id].name << " Starting line " << start_line
       << " Ending line " << end_line << '\n';

  // Make sure that the file is open
  if(files[file_id].file_ptr == NULL)
    {
      if(files[file_id].name != NULL)
	files[file_id].file_ptr = fopen_path(files[file_id].name,"r");

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

  pc = NULL; //new Program_Counter();

  files = NULL;

  eeprom = NULL;
  config_modes = create_ConfigMode();
  set_frequency(DEFAULT_PIC_CLOCK);
  pll_factor = 0;
  // Test code for logging to disk:
  trace_log.switch_cpus(this);
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

void pic_processor::create (void)
{

  init_program_memory (program_memory_size());

  init_register_memory (register_memory_size());

  //  create_invalid_registers ();
  create_symbols();
  create_stack();

  // Now, initialize the core stuff:
  pc->cpu = this;
  //  cycles.cpu = this;
  wdt.cpu = this;

  W = new WREG;
  W->cpu=this;
  pcl = new PCL;
  pclath = new PCLATH;
  status = new Status_register;
  //FIXME more
  
  W->new_name("W");
  
  fsr = new FSR;
  indf = new INDF;

  fsr->new_name("fsr");

  register_bank = &registers[0];  // Define the active register bank 
  W->value = 0;

  //set_frequency(10e6);            // 
  nominal_wdt_timeout = 18e-3;    // 18ms according to the data sheet (no prescale)

  Vdd = 5.0;                      // Assume 5.0 volt power supply

  trace.program_counter (pc->value);


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
				      unsigned int por_value, char *new_name)
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

  // now add a special symbol for W
  symbol_table.add_w(this, W);

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
      program_memory[address]->update_line_number(file_id,sline,lst_line,0,0);

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
int pic_processor::find_closest_address_to_hll_line(int file_id, int src_line)
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
void pic_processor::set_break_at_address(int address)
{
  if( address >= 0)
    {
	if(program_memory[address]->isa() != instruction::INVALID_INSTRUCTION)
	    bp.set_execution_break(this, address);
    }
}

//-------------------------------------------------------------------
void pic_processor::set_notify_at_address(int address, BreakCallBack *cb)
{
  if( address >= 0)
    {
	if(program_memory[address]->isa() != instruction::INVALID_INSTRUCTION)
	{
	    bp.set_notify_break(this, address, cb);
	}
    }
}

//-------------------------------------------------------------------
void pic_processor::set_profile_start_at_address(int address, BreakCallBack *cb)
{
  if( address >= 0)
    {
	if(program_memory[address]->isa() != instruction::INVALID_INSTRUCTION)
	{
	    bp.set_profile_start_break(this, address, cb);
	}
    }
}

//-------------------------------------------------------------------
void pic_processor::set_profile_stop_at_address(int address, BreakCallBack *cb)
{
  if( address >= 0)
    {
	if(program_memory[address]->isa() != instruction::INVALID_INSTRUCTION)
	{
	    bp.set_profile_stop_break(this, address, cb);
	}
    }
}

//-------------------------------------------------------------------
instruction *pic_processor::find_instruction(int address, enum instruction::INSTRUCTION_TYPES type)
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

void pic_processor::clear_break_at_address(int address)
{
    instruction *instr;
    int b;

    if(program_memory_size()<=address)
	return;

    instr=find_instruction(address,instruction::BREAKPOINT_INSTRUCTION);
    if(instr!=NULL)
    {
	b = ((Breakpoint_Instruction *)instr)->bpn & BREAKPOINT_MASK;
	bp.clear( b );
    }
    else
    {
	cout << "failed to clear execution break\n";
    }
}

//-------------------------------------------------------------------
void pic_processor::clear_notify_at_address(int address)
{
    instruction *instr;
    int b;

    if(program_memory_size()<=address)
	return;

    instr=find_instruction(address,instruction::NOTIFY_INSTRUCTION);
    if(instr!=NULL)
    {
	b = ((Breakpoint_Instruction *)instr)->bpn & BREAKPOINT_MASK;
	bp.clear( b );
    }
    else
    {
	cout << "failed to clear execution break\n";
    }
}

//-------------------------------------------------------------------
void pic_processor::clear_profile_start_at_address(int address)
{
    instruction *instr;
    int b;

    if(program_memory_size()<=address)
	return;

    instr=find_instruction(address,instruction::PROFILE_START_INSTRUCTION);
    if(instr!=NULL)
    {
	b = ((Breakpoint_Instruction *)instr)->bpn & BREAKPOINT_MASK;
	bp.clear( b );
    }
    else
    {
	cout << "failed to clear execution break\n";
    }
}

//-------------------------------------------------------------------
void pic_processor::clear_profile_stop_at_address(int address)
{
    instruction *instr;
    int b;

    if(program_memory_size()<=address)
	return;

    instr=find_instruction(address,instruction::PROFILE_STOP_INSTRUCTION);
    if(instr!=NULL)
    {
	b = ((Breakpoint_Instruction *)instr)->bpn & BREAKPOINT_MASK;
	bp.clear( b );
    }
    else
    {
	cout << "failed to clear execution break\n";
    }
}

//-------------------------------------------------------------------
int pic_processor::address_has_break(int address)
{
    instruction *instr;
    if(program_memory_size()<=address)
	return 0;
    instr=find_instruction(address,instruction::BREAKPOINT_INSTRUCTION);
    if(instr!=NULL)
        return 1;
    return 0;
}

//-------------------------------------------------------------------
int pic_processor::address_has_notify(int address)
{
    instruction *instr;
    if(program_memory_size()<=address)
	return 0;
    instr=find_instruction(address,instruction::NOTIFY_INSTRUCTION);
    if(instr!=NULL)
        return 1;
    return 0;
}

//-------------------------------------------------------------------
int pic_processor::address_has_profile_start(int address)
{
    instruction *instr;
    if(program_memory_size()<=address)
	return 0;
    instr=find_instruction(address,instruction::PROFILE_START_INSTRUCTION);
    if(instr!=NULL)
        return 1;
    return 0;
}

//-------------------------------------------------------------------
int pic_processor::address_has_profile_stop(int address)
{
    instruction *instr;
    if(program_memory_size()<=address)
	return 0;
    instr=find_instruction(address,instruction::PROFILE_STOP_INSTRUCTION);
    if(instr!=NULL)
        return 1;
    return 0;
}


//-------------------------------------------------------------------
void pic_processor::toggle_break_at_address(int address)
{
    if(address_has_break(address))
	clear_break_at_address(address);
    else
        set_break_at_address(address);
}
//-------------------------------------------------------------------

void pic_processor::set_break_at_line(int file_id, int src_line)
{
  int address;

  if( (address = find_closest_address_to_line(file_id, src_line)) >= 0)
      set_break_at_address(address);
}

void pic_processor::clear_break_at_line(int file_id, int src_line)
{

  int address;

  if( (address = find_closest_address_to_line(file_id, src_line)) >= 0)
      clear_break_at_address(address);
}

void pic_processor::toggle_break_at_line(int file_id, int src_line)
{


  toggle_break_at_address(find_closest_address_to_line(file_id, src_line));


}

//-------------------------------------------------------------------

void pic_processor::set_break_at_hll_line(int file_id, int src_hll_line)
{
  int address;

  if( (address = find_closest_address_to_hll_line(file_id, src_hll_line)) >= 0)
      set_break_at_address(address);
}

void pic_processor::clear_break_at_hll_line(int file_id, int src_hll_line)
{

  int address;

  if( (address = find_closest_address_to_hll_line(file_id, src_hll_line)) >= 0)
      clear_break_at_address(address);
}

void pic_processor::toggle_break_at_hll_line(int file_id, int src_hll_line)
{


  toggle_break_at_address(find_closest_address_to_hll_line(file_id, src_hll_line));


}

//-------------------------------------------------------------------
void    pic_processor::set_out_of_range_pm(int address, int value)
{

  cout << "Warning::Out of range address " << address << " value " << value << endl;
  cout << "Max allowed address is " << (program_memory_size()-1) << '\n';

}

//-------------------------------------------------------------------
guint64 pic_processor::cycles_used(unsigned int address)
{
    return program_memory[address]->cycle_count;
}

//-------------------------------------------------------------------
guint64 pic_processor::register_read_accesses(unsigned int address)
{
    return registers[address]->read_access_count;
}

//-------------------------------------------------------------------
guint64 pic_processor::register_write_accesses(unsigned int address)
{
    return registers[address]->write_access_count;
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
      cout << "** SETTING CONFIG address = 0x"<<hex<< address << "  value = 0x"<<value<<'\n';
      set_config_word(address, value);
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

void pic_processor::set_config_word(unsigned int address,unsigned int cfg_word)
{
  config_word = cfg_word;

  // Clear all of the configuration bits in config_modes and then
  // reset each of them based on the config bits in cfg_word:
  //config_modes &= ~(CM_WDTE);
  //config_modes |= ( (cfg_word & WDTE) ? CM_WDTE : 0);
  //cout << " setting cfg_word and cfg_modes " << hex << config_word << "  " << config_modes << '\n';

  if((address == config_word_address()) && config_modes)
    config_modes->config_mode = (config_modes->config_mode & ~7) | (cfg_word & 7);

  if(verbose && config_modes)
    config_modes->print();

}

//-------------------------------------------------------------------
//
// load_hex
//
extern int readihex16 (pic_processor *cpu, FILE * file);

void pic_processor::load_hex (char *hex_file)
{

  FILE *inputfile = fopen_path (hex_file, "r");

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

  set_config_word(config_word_address(),0xffff);  // assume no configuration word is in the hex file.

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

  if(prev) {
    if(prev->isa() == instruction::MULTIWORD_INSTRUCTION) {
      ((multi_word_instruction *)prev)->initialized = 0;
    }
  }
  

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

//-------------------------------------------------------------------
//  ConfigMode
//
void ConfigMode::print(void)
{


  if(config_mode & CM_FOSC1x) {
    // Internal Oscillator type processor 

    switch(config_mode& (CM_FOSC0 | CM_FOSC1)) {  // Lower two bits are the clock type
    case 0: cout << "LP"; break;
    case CM_FOSC0: cout << "XT"; break;
    case CM_FOSC1: cout << "Internal RC"; break;
    case (CM_FOSC0|CM_FOSC1): cout << "External RC"; break;

    } 
  }else {
    switch(config_mode& (CM_FOSC0 | CM_FOSC1)) {  // Lower two bits are the clock type
    case 0: cout << "LP"; break;
    case CM_FOSC0: cout << "XT"; break;
    case CM_FOSC1: cout << "HS"; break;
    case (CM_FOSC0|CM_FOSC1): cout << "RC"; break;
    }
  }

  cout << " oscillator\n";

  if(valid_bits & CM_WDTE) 
    cout << " WDT is " << (get_wdt() ? "enabled\n" : "disabled\n");

  if(valid_bits & CM_MCLRE) 
    cout << "MCLR is " << (get_mclre() ? "enabled\n" : "disabled\n");

  if(valid_bits & CM_CP0) {

    if(valid_bits & CM_CP1) {
      cout << "CP0 is " << (get_cp0() ? "high\n" : "low\n");
      cout << "CP1 is " << (get_cp1() ? "high\n" : "low\n");
    } else {

      cout << "code protection is " << (get_cp0() ? "enabled\n" : "disabled\n");

    }
  }


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
