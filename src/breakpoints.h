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


#ifndef  __BREAKPOINTS_H__
#define  __BREAKPOINTS_H__
#include "gpsim_classes.h"
#include "pic-instructions.h"
#include "pic-registers.h"

#include  <iostream.h>

#include <unistd.h>
#include <glib.h>




#define MAX_BREAKPOINTS 0x400
#define BREAKPOINT_MASK (MAX_BREAKPOINTS-1)

class Breakpoint_Instruction : public instruction
{
public:

  pic_processor *cpu;
  unsigned int address;
  unsigned int bpn;
  instruction *replaced;

  virtual unsigned int get_opcode(void);
  virtual int get_src_line(void);
  virtual int get_hll_src_line(void);
  virtual int get_lst_line(void);
  virtual int get_file_id(void);
  virtual int get_hll_file_id(void);

  Breakpoint_Instruction(pic_processor *new_cpu, unsigned int new_address, unsigned int bp);

  virtual INSTRUCTION_TYPES isa(void) {return BREAKPOINT_INSTRUCTION;};
  virtual void execute(void);
  virtual char *name(char *);

};


//
// BreakCallBack - this object is intended to be one of the classes in
// in a multiple inheritance class declaration. It's purpose is to provide
// a mechanism by which a break point can notify an object that the break
// has occurred.
// Note that BreakCallBack is typically used in conjunction with Cycle
// Counter break points (see the CycleCounter class). 
//

class BreakCallBack
{
public:
  virtual void callback(void)
    {
      cout << "generic callback\n";
    }

  // Invoked by Cycle counter to display info about break call back.
  virtual void callback_print(void) {
    cout << " has callback\n";
  }

  // clear_break is invoked when a BreakCallBack object's
  // associated break point is cleared. 
  virtual void clear_break(void) {};
};

class Notify_Instruction : public Breakpoint_Instruction
{
    BreakCallBack *callback;
public:
    Notify_Instruction(pic_processor *cpu, unsigned int address, unsigned int bp, BreakCallBack *cb);
    virtual INSTRUCTION_TYPES isa(void) {return NOTIFY_INSTRUCTION;};
    virtual void execute(void);
};

class Profile_Start_Instruction : public Notify_Instruction
{
public:
    Profile_Start_Instruction(pic_processor *cpu, unsigned int address, unsigned int bp, BreakCallBack *cb);
    virtual INSTRUCTION_TYPES isa(void) {return PROFILE_START_INSTRUCTION;};
};

class Profile_Stop_Instruction : public Notify_Instruction
{
public:
    Profile_Stop_Instruction(pic_processor *cpu, unsigned int address, unsigned int bp, BreakCallBack *cb);
    virtual INSTRUCTION_TYPES isa(void) {return PROFILE_STOP_INSTRUCTION;};
};


class Breakpoints
{
public:

enum BREAKPOINT_TYPES
{
  BREAK_CLEAR          = 0,
  BREAK_ON_EXECUTION   = 1<<24,
  BREAK_ON_REG_READ    = 2<<24,
  BREAK_ON_REG_WRITE   = 3<<24,
  BREAK_ON_REG_READ_VALUE  = 4<<24,
  BREAK_ON_REG_WRITE_VALUE = 5<<24,
  BREAK_ON_INVALID_FR  = 6<<24,
  BREAK_ON_CYCLE       = 7<<24,
  BREAK_ON_WDT_TIMEOUT = 8<<24,
  BREAK_ON_STK_OVERFLOW  = 9<<24,
  BREAK_ON_STK_UNDERFLOW = 10<<24,
  NOTIFY_ON_EXECUTION   = 11<<24,
  PROFILE_START_NOTIFY_ON_EXECUTION = 12<<24,
  PROFILE_STOP_NOTIFY_ON_EXECUTION = 13<<24,
  NOTIFY_ON_REG_READ        = 14<<24,
  NOTIFY_ON_REG_WRITE       = 15<<24,
  NOTIFY_ON_REG_READ_VALUE  = 16<<24,
  NOTIFY_ON_REG_WRITE_VALUE = 17<<24,


  BREAK_MASK           = 0xff<<24
};

#define  GLOBAL_CLEAR         0
#define  GLOBAL_STOP_RUNNING  1
#define  GLOBAL_INTERRUPT     2
#define  GLOBAL_SLEEP         4
#define  GLOBAL_PM_WRITE      8

struct BreakStatus
{
  BREAKPOINT_TYPES type;
  pic_processor *cpu;
  unsigned int arg1;
  unsigned int arg2;
  BreakCallBack *f;
} break_status[MAX_BREAKPOINTS];

  unsigned int  global_break;

  unsigned int breakpoint_number,last_breakpoint;


  Breakpoints(void);
  unsigned int set_breakpoint(BREAKPOINT_TYPES,pic_processor *,unsigned int, unsigned int,BreakCallBack *f = NULL);
  unsigned int set_execution_break(pic_processor *cpu, unsigned int address);
  unsigned int set_notify_break(pic_processor *cpu, unsigned int address, BreakCallBack *cb);
  unsigned int set_profile_start_break(pic_processor *cpu, unsigned int address, BreakCallBack *f1 = NULL);
  unsigned int set_profile_stop_break(pic_processor *cpu, unsigned int address, BreakCallBack *f1 = NULL);
  unsigned int set_read_break(pic_processor *cpu, unsigned int register_number);
  unsigned int set_write_break(pic_processor *cpu, unsigned int register_number);
  unsigned int set_read_value_break(pic_processor *cpu, unsigned int register_number, unsigned int value, unsigned int mask=0xff);
  unsigned int set_write_value_break(pic_processor *cpu, unsigned int register_number, unsigned int value, unsigned int mask=0xff);
  unsigned int set_cycle_break(pic_processor *cpu, guint64 cycle,BreakCallBack *f = NULL);
  unsigned int set_wdt_break(pic_processor *cpu);
  unsigned int set_stk_overflow_break(pic_processor *cpu);
  unsigned int set_stk_underflow_break(pic_processor *cpu);
  unsigned int check_write_break(file_register *fr);
  unsigned int check_read_break(file_register *fr);
  unsigned int check_break(file_register *fr);
  unsigned int check_invalid_fr_break(invalid_file_register *fr);
  unsigned int check_cycle_break(unsigned int abp);

  unsigned int set_notify_read(pic_processor *cpu, unsigned int register_number);
  unsigned int set_notify_write(pic_processor *cpu, unsigned int register_number);
  unsigned int set_notify_read_value(pic_processor *cpu, unsigned int register_number, unsigned int value, unsigned int mask=0xff);
  unsigned int set_notify_write_value(pic_processor *cpu, unsigned int register_number, unsigned int value, unsigned int mask=0xff);

  inline void clear_global(void) {global_break = GLOBAL_CLEAR;};
  inline void halt(void) { global_break |= GLOBAL_STOP_RUNNING;};
  inline bool have_halt(void) { return( (global_break & GLOBAL_STOP_RUNNING) != 0 );};
  inline void clear_halt(void) {global_break &= ~GLOBAL_STOP_RUNNING;};
  inline bool have_interrupt(void) { return( (global_break & GLOBAL_INTERRUPT) != 0 );};
  inline void clear_interrupt(void) {global_break &= ~GLOBAL_INTERRUPT;};
  inline void set_interrupt(void) {global_break |= GLOBAL_INTERRUPT;};
  inline bool have_sleep(void) { return( (global_break & GLOBAL_SLEEP) != 0 );};
  inline void clear_sleep(void) {global_break &= ~GLOBAL_SLEEP;};
  inline void set_sleep(void) {global_break |= GLOBAL_SLEEP;};
  inline bool have_pm_write(void) { return( (global_break & GLOBAL_PM_WRITE) != 0 );};
  inline void clear_pm_write(void) {global_break &= ~GLOBAL_PM_WRITE;};
  inline void set_pm_write(void) {global_break |= GLOBAL_PM_WRITE;};


  bool dump1(unsigned int bp_num);
  void dump(void);
  void dump_traced(unsigned int b);
  void clear(unsigned int b);
  void clear_all(pic_processor *c);
  void clear_all_set_by_user(pic_processor *c);
  void initialize_breakpoints(unsigned int memory_size);
  instruction *find_previous(pic_processor *cpu, unsigned int address, instruction *_this);

};

extern Breakpoints bp;

//
// Notify_Register 
//
//  This class serves as the base class for register break point and logging
// classes. Register breakpoints are handled by replacing a register object
// with one of the breakpoint objects. The simulated pic code has no idea that
// breakpoints exist on a register. However, when the member functions of the
// a register are accessed, the breakpoint member functions of the classes
// described below are the ones actually invoked. Consequently, control of
// the simulation can be manipulated.
//

class Notify_Register : public file_register
{
public:
  file_register *replaced;   // A pointer to the register that this break replaces
  Notify_Register *next; /* If multiple breaks are set on one register,
			      * then this will point to the next one.  */

  Notify_Register(void){ replaced = NULL; next = NULL;};
  Notify_Register(pic_processor *, int, int );

  virtual REGISTER_TYPES isa(void) {return BP_REGISTER;};
  virtual char *name(void)
    {
      if(replaced)
	return replaced->name();
    };
  /* direct all accesses to the member functions of the
   * register that is being replaced. Note that we assume
   * "replaced" is properly initialized which it will be
   * if this object is accessed. (Why? well, we only access
   * register notify/breaks via the PIC's file register 
   * memory and never directly access them. But the only
   * way this instantiation can be accessed is if it successfully
   * replaced a file register object */

  virtual void put_value(unsigned int new_value)
    {
      replaced->put_value(new_value);
    }
  virtual void put(unsigned int new_value)
    {
      replaced->put(new_value);
    }

  virtual unsigned int get_value(void)
    {
      return(replaced->get_value());
    }
  virtual unsigned int get(void)
    {
      return(replaced->get());
    }

  virtual void setbit(unsigned int bit_number, bool new_value)
    {
      return(replaced->setbit(bit_number, new_value));
    }

  virtual int get_bit(unsigned int bit_number)
    {
      return(replaced->get_bit(bit_number));
    }

  virtual int get_bit_voltage(unsigned int bit_number)
    {
      return(replaced->get_bit_voltage(bit_number));
    }

  void replace(pic_processor *_cpu, unsigned int reg);
  unsigned int clear(unsigned int bp_num);

};

class Notify_Register_Value : public Notify_Register
{
public:

  unsigned int break_value, break_mask, last_value;

  Notify_Register_Value(void)
    { 
      replaced = NULL;
      break_value = 0;
      break_mask = 0;
      last_value = 0;
    }

  Notify_Register_Value(pic_processor *_cpu, int _repl, int bp, int bv, int bm ):
    Notify_Register(_cpu,_repl,bp ) 
    { 
      break_value = bv;
      break_mask = bm;
      if(replaced)
	last_value = replaced->get_value();

    };

};


class Break_register_read : public Notify_Register
{
public:


  Break_register_read(void){ };
  Break_register_read(pic_processor *_cpu, int _repl, int bp ):
    Notify_Register(_cpu,_repl,bp ) { };


  unsigned int get(void);
  int get_bit(unsigned int bit_number);
  int get_bit_voltage(unsigned int bit_number);

};

class Break_register_write : public Notify_Register
{
public:


  Break_register_write(void){ };
  Break_register_write(pic_processor *_cpu, int _repl, int bp ):
    Notify_Register(_cpu,_repl,bp ) { };
  void put(unsigned int new_value);
  void setbit(unsigned int bit_number, bool new_value);

};

class Break_register_read_value : public Notify_Register_Value
{
public:

  Break_register_read_value(void){ };
  Break_register_read_value(pic_processor *_cpu, int _repl, int bp, int bv, int bm ) :
    Notify_Register_Value(_cpu,  _repl, bp, bv, bm ) { };

  unsigned int get(void);
  int get_bit(unsigned int bit_number);
  int get_bit_voltage(unsigned int bit_number);

};

class Break_register_write_value : public Notify_Register_Value
{
public:

  Break_register_write_value(void){ };
  Break_register_write_value(pic_processor *_cpu, int _repl, int bp, int bv, int bm ) :
    Notify_Register_Value(_cpu,  _repl, bp, bv, bm ) { };

  void put(unsigned int new_value);
  void setbit(unsigned int bit_number, bool new_value);
};

class Log_Register_Write : public Notify_Register
{
 public:

  Log_Register_Write(void){ };
  Log_Register_Write(pic_processor *_cpu, int _repl, int bp ):
    Notify_Register(_cpu,_repl,bp ) { };
  void put(unsigned int new_value);
  void setbit(unsigned int bit_number, bool new_value);

};

class Log_Register_Read : public Notify_Register
{
public:


  Log_Register_Read(void){ };
  Log_Register_Read(pic_processor *_cpu, int _repl, int bp ):
    Notify_Register(_cpu,_repl,bp ) { };
  unsigned int get(void);
  int get_bit(unsigned int bit_number);
  int get_bit_voltage(unsigned int bit_number);
};

class Log_Register_Read_value : public Notify_Register_Value
{
public:

  Log_Register_Read_value(void){ };
  Log_Register_Read_value(pic_processor *_cpu, int _repl, int bp, int bv, int bm ) :
    Notify_Register_Value(_cpu,  _repl, bp, bv, bm ) { };
  unsigned int get(void);
  int get_bit(unsigned int bit_number);
  int get_bit_voltage(unsigned int bit_number);
};

class Log_Register_Write_value : public Notify_Register_Value
{
public:

  Log_Register_Write_value(void){ };
  Log_Register_Write_value(pic_processor *_cpu, int _repl, int bp, int bv, int bm ) :
    Notify_Register_Value(_cpu,  _repl, bp, bv, bm ) { };

  void put(unsigned int new_value);

};

#ifdef HAVE_GUI
class GuiCallBack: public BreakCallBack
{
public:
  virtual void callback(void);

  gpointer gui_callback_data;  // Data to be passed back to the gui

  // A pointer to the gui call back function
  void  (*gui_callback) (gpointer gui_callback_data);
  void set_break(int, void (*)(gpointer),gpointer );

  GuiCallBack(void);
};
#endif // HAVE_GUI

#endif   //  __BREAKPOINTS_H__
