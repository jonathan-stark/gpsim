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
  virtual int get_lst_line(void);
  virtual int get_file_id(void);

  Breakpoint_Instruction(pic_processor *new_cpu, unsigned int new_address, unsigned int bp);

  virtual INSTRUCTION_TYPES isa(void) {return BREAKPOINT_INSTRUCTION;};
  virtual void execute(void);
  virtual char *name(char *);

};


class BreakCallBack
{
public:
  virtual void callback(void)
    {
      cout << "generic callback\n";
    }

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
  unsigned int set_read_break(pic_processor *cpu, unsigned int register_number);
  unsigned int set_write_break(pic_processor *cpu, unsigned int register_number);
  unsigned int set_read_value_break(pic_processor *cpu, unsigned int register_number, unsigned int value);
  unsigned int set_write_value_break(pic_processor *cpu, unsigned int register_number, unsigned int value);
  unsigned int set_cycle_break(pic_processor *cpu, guint64 cycle,BreakCallBack *f = NULL);
  unsigned int set_wdt_break(pic_processor *cpu);
  unsigned int check_write_break(file_register *fr);
  unsigned int check_read_break(file_register *fr);
  unsigned int check_break(file_register *fr);
  unsigned int check_invalid_fr_break(invalid_file_register *fr);
  unsigned int check_cycle_break(unsigned int abp);

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

};

extern Breakpoints bp;

class Breakpoint_Register : public file_register
{
public:
  file_register *replaced;   // A pointer to the register that this break replaces
  Breakpoint_Register *next; /* If multiple breaks are set on one register,
			      * then this will point to the next one.  */

  virtual REGISTER_TYPES isa(void) {return BP_REGISTER;};
  virtual char *name(void)
    {
      if(replaced)
	return replaced->name();
    };
  virtual void put_value(unsigned int new_value)=0;
  virtual void put(unsigned int new_value)=0;
  virtual unsigned int get(void)=0;
  virtual unsigned int get_value(void)=0;
  void replace(pic_processor *_cpu, unsigned int reg);
  unsigned int clear(unsigned int bp_num);

};

class Break_register_read : public Breakpoint_Register
{
public:


  Break_register_read(void){ replaced = NULL;}
  void put(unsigned int new_value)
    {
      replaced->put(new_value);
    }
  unsigned int get(void);
  unsigned int get_value(void)
    {
      return(replaced->get_value());
    }

  virtual void put_value(unsigned int new_value)
    {
      replaced->put_value(new_value);
    }

};

class Break_register_write : public Breakpoint_Register
{
public:


  Break_register_write(void){ replaced = NULL;}
  void put(unsigned int new_value);
  unsigned int get(void)
    {
      return(replaced->get());
    }
  unsigned int get_value(void)
    {
      return(replaced->get_value());
    }
  virtual void put_value(unsigned int new_value)
    {
      replaced->put_value(new_value);
    }

};

class Break_register_read_value : public Breakpoint_Register
{
public:

  unsigned int break_value, break_mask;

  Break_register_read_value(void){ replaced = NULL;}
  void put(unsigned int new_value)
    {
      replaced->put(new_value);
    }
  unsigned int get(void);
  unsigned int get_value(void)
    {
      return(replaced->get_value());
    }
  virtual void put_value(unsigned int new_value)
    {
      replaced->put_value(new_value);
    }

};

class Break_register_write_value : public Breakpoint_Register
{
public:

  unsigned int break_value, break_mask;

  Break_register_write_value(void){ replaced = NULL;}
  void put(unsigned int new_value);
  unsigned int get(void)
    {
      return(replaced->get());
    }
  unsigned int get_value(void)
    {
      return(replaced->get_value());
    }
  virtual void put_value(unsigned int new_value)
    {
      replaced->put_value(new_value);
    }

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
