/*
   Copyright (C) 1998-2000 T. Scott Dattalo

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

#ifndef __PROCESSORS_H__
#define __PROCESSORS_H__

#include <glib.h>

#include "gpsim_classes.h"
#include "modules.h"


/*==================================================================
 *
 * Here are a few enum definitions that probably should go somewhere
 * else, but are here for right now...
 *
 *
 * Define all of the different types of reset conditions:
 */

enum RESET_TYPE
{
  POR_RESET,
  WDT_RESET,
  SOFT_RESET
};

enum SIMULATION_MODES
{
  STOPPED,
  RUNNING,
  SLEEPING,
  SINGLE_STEPPING,
  STEPPING_OVER,
  RUNNING_OVER
};

extern SIMULATION_MODES simulation_mode;

enum PROCESSOR_STATES
{

  POR_,
  IDLE

};


enum PROCESSOR_TYPE
{
  _PIC_PROCESSOR_,
  _14BIT_PROCESSOR_,
  _12BIT_PROCESSOR_,
  _16BIT_PROCESSOR_,
  _P12C508_,
  _P12C509_,
  _P16C84_,
  _P16CR83_,
  _P16CR84_,
  _P16F83_,
  _P16F84_,
  _P16C71_,
  _P16C54_,
  _P16C61_,
  _P16C62_,
  _P16C62A_,
  _P16CR62_,
  _P16C63_,
  _P16C64_,
  _P16C64A_,
  _P16CR64_,
  _P16C65_,
  _P16C65A_,
  _P16C74_,
  _P16F874_,
  _P16F877_,
  _P18Cxx2_,
  _P18C2x2_,
  _P18C242_,
  _P18C252_,
  _P18C442_,
  _P18C452_
};

// Configuration modes.
#define  CM_WDTE 1<<0

/*==================================================================
 * FIXME - move these global references somewhere else
 */
extern int     verbose;         // If non-zero, then print diagnostics while initializing
extern guint64 gui_update_rate; // The rate (in simulation cycles) at which the gui is updated

/*==================================================================
 *
 * Here are the base class declarations for the pic processors
 */

/*
 * First, forward-declare a few class references
 */

#include "trace.h"
#include "pic-registers.h"
enum IOPIN_TYPES
{
  INPUT_ONLY,          // e.g. MCLR
  BI_DIRECTIONAL,      // most iopins
  BI_DIRECTIONAL_PU,   // same as bi_directional, but with pullup resistor. e.g. portb
  OPEN_COLLECTOR       // bit4 in porta on the 18 pin midrange devices.
};

#include "pic-instructions.h"
#include "12bit-instructions.h"
#include "14bit-registers.h"
#include "14bit-instructions.h"
#include "gpsim_interface.h"
#include "pic-packages.h"

class processor_types
{
public:

  PROCESSOR_TYPE type;
  char *names[4];
  pic_processor * (*cpu_constructor) (void);
};

#ifdef HAVE_GUI
//
// Forward reference
//

struct _gui_processor;
typedef struct _gui_processor GUI_Processor;

#endif
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
  pic_processor *cpu;

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


/*
 * Define a base class processor for the pic processor family
 *
 * All pic processors are derived from this class.
 */

class pic_processor : public Module
{
public:

  #define FILE_REGISTERS  0x100

  struct file_context *files;  // A dynamically allocated array for src file info
  int number_of_source_files;  // The number of elements allocated to that array
  int lst_file_id;

  int processor_id; // An identifier to differentiate this instantiation from others

  unsigned int config_word;        // as read from hex or cod file
  unsigned int config_modes;       // processor independent configuration bits.
  double frequency,period;
  double Vdd;
  double nominal_wdt_timeout;

#ifdef HAVE_GUI
  GUI_Processor *gp;
#endif

  WDT          wdt;

  INDF         indf;
  FSR          fsr;
  Stack         *stack;
  file_register **registers;
  file_register **register_bank;   // a pointer to the currently active register bank
  instruction   **program_memory;
  program_memory_access pma;

  Cycle_Counter cycles;
  Status_register status;
  WREG          W;
  Program_Counter pc;
  OPTION_REG   option_reg;
  PCL          pcl;
  PCLATH       pclath;

  SFR_map*     sfr_map;
  int          num_of_sfrs;
  
  TMR0         tmr0;
  int          num_of_gprs;

  void create_invalid_registers (void);
  void add_sfr_register(sfr_register *reg, unsigned int addr,
			unsigned int por_value=0,char *new_name=NULL);
  void add_file_registers(unsigned int start_address, 
			  unsigned int end_address, 
			  unsigned int alias_offset);
  void delete_file_registers(unsigned int start_address, 
			     unsigned int end_address);
  void alias_file_registers(unsigned int start_address, 
			    unsigned int end_address, 
			    unsigned int alias_offset);

  void init_program_memory(unsigned int memory_size);
  void build_program_memory(int *memory,int minaddr, int maxaddr);
  void attach_src_line(int address,int file_id,int sline,int lst_line);
  void read_src_files(void);

  // A couple of functions for manipulating  breakpoints
  int  find_closest_address_to_line(int file_id, int src_line);
  void toggle_break_at_address(int address);
  void set_break_at_line(int file_id, int src_line);
  void clear_break_at_line(int file_id, int src_line);
  void toggle_break_at_line(int file_id, int src_line);

  void init_register_memory(unsigned int memory_size);
  //  void create_iopins (const IOPIN_map iopin_map[], unsigned int num_of_iopins);
  virtual void dump_registers(void);
  virtual instruction * disasm ( unsigned int address,unsigned int inst)=0;

  // %%% FIX ME %%% remove return and added 14bit member.
  virtual void tris_instruction(unsigned int tris_register) {return;};
  virtual void create_symbols(void);
  virtual void create_stack(void) {stack = new Stack;};
  void load_hex(char *hex_file);
  void run(void);
  void run_to_address(unsigned int destination);
  void sleep(void);
  void step(unsigned int steps);
  void step_over(void);
  virtual void interrupt(void) { return; };
  void pm_write(void);

  virtual void set_config_word(unsigned int cfg_word);
  unsigned int get_config_word(void) {return config_word;};
  void set_frequency(double f) { frequency = f; if(f>0) period = 1/f; };
  unsigned int time_to_cycles( double t) 
    {if(period>0) return((int) (frequency * t)); else return 0;};
  void reset(RESET_TYPE r);
  void disassemble (int start_address, int end_address);
  void list(int file_id, int pcval, int start_line, int end_line);

  virtual void por(void);
  virtual void create(void);

  virtual unsigned int program_memory_size(void) const {return 0;};
  void init_program_memory(int address, int value);
  virtual void set_out_of_range_pm(int address, int value);
  virtual unsigned int config_word_address(void) const {return 0x2007;};

  virtual PROCESSOR_TYPE isa(void){return _PIC_PROCESSOR_;};
  virtual PROCESSOR_TYPE base_isa(void){return _PIC_PROCESSOR_;};
  virtual unsigned int register_memory_size () const { return FILE_REGISTERS;};

  /* The program_counter class calls these two functions to get the upper bits of the PC
   * for branching (e.g. goto) or modify PCL instructions (e.g. addwf pcl,f) */
  virtual unsigned int get_pclath_branching_jump(void)=0;
  virtual unsigned int get_pclath_branching_modpcl(void)=0;

  virtual void option_new_bits_6_7(unsigned int)=0;

  // There's probably a better way to specify eeprom stuff other than definining it
  // here in the base class...
  virtual unsigned int eeprom_get_size(void) {return 0;};
  virtual unsigned int eeprom_get_value(unsigned int address) {return 0;};
  virtual void eeprom_put_value(unsigned int value,
				unsigned int address)
    {return;};
  virtual file_register *eeprom_get_register(unsigned int address) { return NULL; }

  virtual int get_pin_count(void){return 0;};
  virtual char *get_pin_name(unsigned int pin_number) {return NULL;};
  virtual int get_pin_state(unsigned int pin_number) {return 0;};
  virtual IOPIN *get_pin(unsigned int pin_number) {return NULL;};

  static pic_processor *construct(void);
  pic_processor(void);
};

/*
class peripheral
{
public:

  char * name_str;
};

*/

//---------------------------------------------------------
// define a special 'invalid' register class. Accessess to
// to this class' value get 0

class invalid_file_register : public file_register
{
public:

  void put(unsigned int new_value);
  unsigned int get(void);
  invalid_file_register(unsigned int at_address);
  virtual REGISTER_TYPES isa(void) {return INVALID_REGISTER;};
};


//--------------------------------------
//
// non-class helper functions.

pic_processor *  add_processor(char * processor_type, char * processor_new_name);
void display_available_processors(void);
void dump_processor_list(void);
int find_in_available_processor_list(char * processor_type);

#endif
