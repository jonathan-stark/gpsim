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
along with gpasm; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#ifndef __PROCESSOR_H__
#define __PROCESSOR_H__
#include <unistd.h>
#include <glib.h>

#include "gpsim_classes.h"
#include "modules.h"
#include "trace.h"
#include "pic-registers.h"
#include "gpsim_time.h"
#include "gpsim_interface.h"


extern SIMULATION_MODES simulation_mode;
class GUI_Processor;
class Processor;
extern int verbose;


//---------------------------------------------------------
// The program_memory_access class is the interface used
// by objects other than the simulator to manipulate the 
// pic's program memory. For example, the breakpoint class
// modifies program memory when break points are set or
// cleared. The modification goes through here.
//
class program_memory_access :  public BreakCallBack
{
 public:
  Processor *cpu;

  unsigned int address, opcode, state;

  // breakpoint instruction pointer. This is used by get_base_instruction().
  // If an instruction has a breakpoint set on it, then get_base_instruction
  // will return a pointer to the instruction and will initialize bpi to
  // the breakpoint instruction that has replaced the one in the pic program
  // memory.
  Breakpoint_Instruction *bpi;

  void put(int addr, instruction *new_instruction);
  instruction *get(int addr);
  instruction *get_base_instruction(int addr);
  unsigned int get_opcode(int addr);

  void put_opcode(int addr, unsigned int new_opcode);
  // When a pic is replacing one of it's own instructions, this routine
  // is called.
  void put_opcode_start(int addr, unsigned int new_opcode);

  virtual void callback(void);
  program_memory_access(void)
    {
      address=opcode=state=0;
    }
};


//------------------------------------------------------------------------
//
// Processor - a generic base class for processors supported by gpsim
//

class Processor : public Module
{
public:

  struct file_context *files;  // A dynamically allocated array for src file info
  int number_of_source_files;  // The number of elements allocated to that array
  int lst_file_id;

  int processor_id;              // An identifier to differentiate this instantiation from others

#ifdef HAVE_GUI
  GUI_Processor *gp;
#endif

  double frequency,period;     // Oscillator frequency and period.

  file_register **registers;       // 
  file_register **register_bank;   //

  instruction   **program_memory;
  program_memory_access pma;


  Cycle_Counter cycles;


  void create_invalid_registers (void);
  void add_file_registers(unsigned int start_address, 
			  unsigned int end_address, 
			  unsigned int alias_offset);
  void delete_file_registers(unsigned int start_address, 
			     unsigned int end_address);
  void alias_file_registers(unsigned int start_address, 
			    unsigned int end_address, 
			    unsigned int alias_offset);
  void init_register_memory(unsigned int memory_size);


  void init_program_memory(unsigned int memory_size);
  void init_program_memory(int address, int value);
  virtual unsigned int program_memory_size(void) const {return 0;};
  void build_program_memory(int *memory,int minaddr, int maxaddr);

  virtual int  map_pm_address2index(int address) {return address;};
  virtual int  map_pm_index2address(int index) {return index;};

  //
  // Symbolic debugging
  //
  // First the source files:

  void attach_src_line(int address,int file_id,int sline,int lst_line);
  void read_src_files(void);





  // A couple of functions for manipulating  breakpoints
  virtual int  find_closest_address_to_line(int file_id, int src_line);
  virtual int  find_closest_address_to_hll_line(int file_id, int src_line);
  void set_break_at_address(int address);
  void set_notify_at_address(int address, BreakCallBack *cb);
  void set_profile_start_at_address(int address, BreakCallBack *cb);
  void set_profile_stop_at_address(int address, BreakCallBack *cb);
  int clear_break_at_address(int address,enum instruction::INSTRUCTION_TYPES type);
  int clear_notify_at_address(int address);
  int clear_profile_start_at_address(int address);
  int clear_profile_stop_at_address(int address);
  int address_has_break(int address,enum instruction::INSTRUCTION_TYPES type);
  int address_has_notify(int address);
  int address_has_profile_start(int address);
  int address_has_profile_stop(int address);
  instruction *find_instruction(int address, enum instruction::INSTRUCTION_TYPES type);
  void toggle_break_at_address(int address);
  void set_break_at_line(int file_id, int src_line);
  void clear_break_at_line(int file_id, int src_line);
  void toggle_break_at_line(int file_id, int src_line);
  void set_break_at_hll_line(int file_id, int src_line);
  void clear_break_at_hll_line(int file_id, int src_line);
  void toggle_break_at_hll_line(int file_id, int src_line);

  virtual void dump_registers(void);
  virtual instruction * disasm ( unsigned int address,unsigned int inst)=0;

  virtual void load_hex(char *hex_file){}

  void run(void);
  void run_to_address(unsigned int destination);
  virtual void sleep(void) {};
  void step(unsigned int steps);
  void step_over(void);
  virtual void step_one(void) {

  }
  virtual void interrupt(void) { return; };

  void set_frequency(double f) { frequency = f; if(f>0) period = 1/f; };
  unsigned int time_to_cycles( double t) 
    {if(period>0) return((int) (frequency * t)); else return 0;};
  virtual void reset(RESET_TYPE r) {};
  void disassemble (int start_address, int end_address) {};
  void list(int file_id, int pcval, int start_line, int end_line) {};

  virtual void por(void) {};
  virtual void create(void);

  virtual void set_out_of_range_pm(int address, int value);
  guint64 cycles_used(unsigned int address);
  guint64 register_read_accesses(unsigned int address);
  guint64 register_write_accesses(unsigned int address);

  virtual unsigned int register_memory_size () const { return 0;};

  static Processor *construct(void);

  Processor(void);
};



#endif
