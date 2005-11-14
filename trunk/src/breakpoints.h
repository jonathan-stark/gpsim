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


#ifndef  __BREAKPOINTS_H__
#define  __BREAKPOINTS_H__

#include  <iostream>
#include <iomanip>
#include <unistd.h>
#include <glib.h>
#include <string>
#include "trigger.h"
#include "pic-instructions.h"
#include "registers.h"
#include "exports.h"

using namespace std;

extern Integer *verbosity;  // in ../src/init.cc
class InvalidRegister;

class TriggerGroup : public TriggerAction
{
public:

protected:
  list<TriggerObject*> triggerList;

  virtual ~TriggerGroup(){}
};


#define MAX_BREAKPOINTS 0x400
#define BREAKPOINT_MASK (MAX_BREAKPOINTS-1)

class Breakpoint_Instruction : public AliasedInstruction , public TriggerObject
{
public:

  unsigned int address;

  virtual bool set_break(void);
  virtual Processor* get_cpu(void);
  virtual void print(void);
  virtual void clear(void);
  virtual char const * bpName() { return "Execution"; }

  Breakpoint_Instruction(Processor *new_cpu, 
			 unsigned int new_address, 
			 unsigned int bp);

  virtual INSTRUCTION_TYPES isa(void) {return BREAKPOINT_INSTRUCTION;};
  virtual void execute(void);
  virtual bool isBase() { return false;}
  virtual bool eval_Expression();
};


class Notify_Instruction : public Breakpoint_Instruction
{
  TriggerObject *callback;
 public:
  Notify_Instruction(Processor *cpu, 
		     unsigned int address, 
		     unsigned int bp, 
		     TriggerObject *cb);
  virtual INSTRUCTION_TYPES isa(void) {return NOTIFY_INSTRUCTION;};
  virtual void execute(void);
  virtual char const * bpName() { return "Notify Execution"; }

};

class Profile_Start_Instruction : public Notify_Instruction
{
 public:
  Profile_Start_Instruction(Processor *cpu, 
			    unsigned int address, 
			    unsigned int bp, 
			    TriggerObject *cb);
  virtual INSTRUCTION_TYPES isa(void) {return PROFILE_START_INSTRUCTION;};
  virtual char const * bpName() { return "Profile Start"; }
};

class Profile_Stop_Instruction : public Notify_Instruction
{
 public:
  Profile_Stop_Instruction(Processor *cpu, 
			   unsigned int address, 
			   unsigned int bp, 
			   TriggerObject *cb);
  virtual INSTRUCTION_TYPES isa(void) {return PROFILE_STOP_INSTRUCTION;};
  virtual char const * bpName() { return "Profile Stop"; }
};

//
// Assertions
// 
// Assertions are like breakpoints except that they're conditional.
// For example, a user may wish to verify that the proper register
// bank is selected while a variable is accessed.
//
class RegisterAssertion : public Breakpoint_Instruction
{
 public:
  unsigned int regAddress;
  unsigned int regMask;
  unsigned int regValue;
  bool bPostAssertion; // True if assertion is checked after instruction simulates.
  typedef bool (*PFNISASSERTIONCONDITION)(unsigned int uRegValue,
      unsigned int uRegMask, unsigned int uRegTestValue);
  PFNISASSERTIONCONDITION m_pfnIsAssertionBreak;

  enum {
    eRAEquals,
    eRANotEquals,
    eRAGreaterThen,
    eRALessThen,
    eRAGreaterThenEquals,
    eRALessThenEquals,
  };

  static bool IsAssertionEqualsBreakCondition(unsigned int uRegValue,
      unsigned int uRegMask, unsigned int uRegTestValue);
  static bool IsAssertionNotEqualsBreakCondition(unsigned int uRegValue,
      unsigned int uRegMask, unsigned int uRegTestValue);
  static bool IsAssertionGreaterThenBreakCondition(unsigned int uRegValue,
      unsigned int uRegMask, unsigned int uRegTestValue);
  static bool IsAssertionLessThenBreakCondition(unsigned int uRegValue,
      unsigned int uRegMask, unsigned int uRegTestValue);
  static bool IsAssertionGreaterThenEqualsBreakCondition(unsigned int uRegValue,
      unsigned int uRegMask, unsigned int uRegTestValue);
  static bool IsAssertionLessThenEqualsBreakCondition(unsigned int uRegValue,
      unsigned int uRegMask, unsigned int uRegTestValue);

  RegisterAssertion(Processor *new_cpu, 
		    unsigned int instAddress, 
		    unsigned int bp,
		    unsigned int _regAddress,
		    unsigned int _regMask,
		    unsigned int _regValue,
		    bool bPostAssertion=false
		    );

  RegisterAssertion(Processor *new_cpu, 
		    unsigned int instAddress, 
		    unsigned int bp,
		    unsigned int _regAddress,
		    unsigned int _regMask,
        unsigned int _operator,
		    unsigned int _regValue,
		    bool bPostAssertion=false
		    );

  virtual void execute(void);
  virtual void print(void);
  virtual char const * bpName() { return "Register Assertion"; }


};

class Breakpoints;

#if defined(IN_MODULE) && defined(_WIN32)
// we are in a module: don't access the Breakpoints object directly!
LIBGPSIM_EXPORT Breakpoints & get_bp(void);
#else
// we are in gpsim: use of get_bp() is recommended,
// even if the bp object can be accessed directly.
extern Breakpoints bp;

inline Breakpoints &get_bp(void)
{
  return bp;
}
#endif

class Breakpoints
{
public:
  enum BREAKPOINT_TYPES
    {
      BREAK_DUMP_ALL            = 0,
      BREAK_CLEAR               = 0,
      BREAK_ON_EXECUTION        = 1<<24,
      BREAK_ON_REG_READ         = 2<<24,
      BREAK_ON_REG_WRITE        = 3<<24,
      BREAK_ON_REG_READ_VALUE   = 4<<24,
      BREAK_ON_REG_WRITE_VALUE  = 5<<24,
      BREAK_ON_INVALID_FR       = 6<<24,
      BREAK_ON_CYCLE            = 7<<24,
      BREAK_ON_WDT_TIMEOUT      = 8<<24,
      BREAK_ON_STK_OVERFLOW     = 9<<24,
      BREAK_ON_STK_UNDERFLOW    = 10<<24,
      NOTIFY_ON_EXECUTION       = 11<<24,
      PROFILE_START_NOTIFY_ON_EXECUTION = 12<<24,
      PROFILE_STOP_NOTIFY_ON_EXECUTION = 13<<24,
      NOTIFY_ON_REG_READ        = 14<<24,
      NOTIFY_ON_REG_WRITE       = 15<<24,
      NOTIFY_ON_REG_READ_VALUE  = 16<<24,
      NOTIFY_ON_REG_WRITE_VALUE = 17<<24,
      BREAK_ON_ASSERTION        = 18<<24,
      BREAK_MASK                = 0xff<<24
    };

#define  GLOBAL_CLEAR         0
#define  GLOBAL_STOP_RUNNING  (1<<0)
#define  GLOBAL_INTERRUPT     (1<<1)
#define  GLOBAL_SLEEP         (1<<2)
#define  GLOBAL_PM_WRITE      (1<<3)
#define  GLOBAL_SOCKET        (1<<4)

  struct BreakStatus
  {
    BREAKPOINT_TYPES type;
    Processor *cpu;
    unsigned int arg1;
    unsigned int arg2;
    TriggerObject *bpo;
  } break_status[MAX_BREAKPOINTS];

  int m_iMaxAllocated;

  class iterator {
  public:
    iterator(int index) : iIndex(index) { }
    int iIndex;
    iterator & operator++(int) {
      iIndex++;
      return *this;
    }
    BreakStatus * operator*() {
      return &get_bp().break_status[iIndex];
    }
    bool operator!=(iterator &it) {
      return iIndex != it.iIndex;
    }
  };

  iterator begin() {
    return iterator(0);
  }

  iterator end() {
    return iterator(m_iMaxAllocated);
  }

  unsigned int  global_break;
  bool          m_bExitOnBreak;   // enabled from command line option
  void EnableExitOnBreak(bool bExitOnBreak) {
    m_bExitOnBreak = bExitOnBreak;
  }

  unsigned int breakpoint_number,last_breakpoint;


  Breakpoints(void);
  unsigned int set_breakpoint(BREAKPOINT_TYPES,Processor *,
			      unsigned int, 
			      unsigned int,
			      TriggerObject *f = 0);
  unsigned int set_breakpoint(TriggerObject *);

  unsigned int set_execution_break(Processor *cpu, unsigned int address);
  unsigned int set_notify_break(Processor *cpu, 
				unsigned int address, 
				TriggerObject *cb);
  unsigned int set_profile_start_break(Processor *cpu, 
				       unsigned int address, 
				       TriggerObject *f1 = 0);
  unsigned int set_profile_stop_break(Processor *cpu, 
				      unsigned int address, 
				      TriggerObject *f1 = 0);
  unsigned int set_read_break(Processor *cpu, unsigned int register_number);
  unsigned int set_write_break(Processor *cpu, unsigned int register_number);
  unsigned int set_read_value_break(Processor *cpu, 
				    unsigned int register_number, 
				    unsigned int value, 
				    unsigned int mask=0xff);
  unsigned int set_write_value_break(Processor *cpu,
				    unsigned int register_number,
				    unsigned int value,
				    unsigned int mask=0xff);
  unsigned int set_read_value_break(Processor *cpu, 
            unsigned int register_number, 
            unsigned int op,
            unsigned int value, 
            unsigned int mask=0xff);
  unsigned int set_write_value_break(Processor *cpu,
            unsigned int register_number,
            unsigned int op,
            unsigned int value,
            unsigned int mask=0xff);
  unsigned int set_cycle_break(Processor *cpu,
			       guint64 cycle,
			       TriggerObject *f = 0);
  unsigned int set_wdt_break(Processor *cpu);
  unsigned int set_stk_overflow_break(Processor *cpu);
  unsigned int set_stk_underflow_break(Processor *cpu);
  unsigned int check_cycle_break(unsigned int abp);

  unsigned int set_notify_read(Processor *cpu, unsigned int register_number);
  unsigned int set_notify_write(Processor *cpu, unsigned int register_number);
  unsigned int set_notify_read_value(Processor *cpu,
				     unsigned int register_number,
				     unsigned int value,
				     unsigned int mask=0xff);
  unsigned int set_notify_write_value(Processor *cpu, 
				      unsigned int register_number,
				      unsigned int value, 
				      unsigned int mask=0xff);
  bool set_expression(unsigned bp_number, Expression *pExpr);

  inline void clear_global(void) {global_break = GLOBAL_CLEAR;};
  void halt(void);
  inline bool have_halt(void) 
    { 
      return( (global_break & GLOBAL_STOP_RUNNING) != 0 );
    }
  inline void clear_halt(void) 
    {
      global_break &= ~GLOBAL_STOP_RUNNING;
    }
  inline bool have_interrupt(void) 
    { 
      return( (global_break & GLOBAL_INTERRUPT) != 0 );
    }
  inline void clear_interrupt(void) 
    {
      global_break &= ~GLOBAL_INTERRUPT;
    }
  inline void set_interrupt(void) 
    {
      global_break |= GLOBAL_INTERRUPT;
    }
  inline bool have_sleep(void) 
    { 
      return( (global_break & GLOBAL_SLEEP) != 0 );
    }
  inline void clear_sleep(void) 
    {
      global_break &= ~GLOBAL_SLEEP;
    }
  inline void set_sleep(void) 
    {
      global_break |= GLOBAL_SLEEP;
    }
  inline bool have_pm_write(void) 
    { 
      return( (global_break & GLOBAL_PM_WRITE) != 0 );
    }
  inline void clear_pm_write(void) 
    {
      global_break &= ~GLOBAL_PM_WRITE;
    }
  inline void set_pm_write(void) 
    {
      global_break |= GLOBAL_PM_WRITE;
    }
  inline bool have_socket_break()
    {
      return( (global_break & GLOBAL_SOCKET) != 0);
    }
  inline void set_socket_break()
    {
      global_break |= GLOBAL_SOCKET;
    }
  inline void clear_socket_break()
    {
      global_break &= ~GLOBAL_SOCKET;
    }

  bool dump1(unsigned int bp_num, int dump_type = BREAK_DUMP_ALL);
  void dump(int dump_type = BREAK_DUMP_ALL);
  void dump_traced(unsigned int b);
  void clear(unsigned int b);
  bool bIsValid(unsigned int b);
  bool bIsClear(unsigned int b);
  void set_message(unsigned int b,string &);
  void clear_all(Processor *c);
  void clear_all_set_by_user(Processor *c);
  void clear_all_register(Processor *c,unsigned int address=-1);
  void initialize_breakpoints(unsigned int memory_size);
  instruction *find_previous(Processor *cpu, 
			     unsigned int address, 
			     instruction *_this);
  int find_free(void);
};

//
// BreakPointRegister 
//
//  This class serves as the base class for register break point and logging
// classes. Register breakpoints are handled by replacing a register object
// with one of the breakpoint objects. The simulated pic code has no idea that
// breakpoints exist on a register. However, when the member functions of the
// a register are accessed, the breakpoint member functions of the classes
// described below are the ones actually invoked. Consequently, control of
// the simulation can be manipulated.
//

class BreakpointRegister : public Register, public TriggerObject
{
public:

  Register *m_replaced;       // A pointer to the register that this break replaces

  BreakpointRegister(void) : TriggerObject(0)
  { m_replaced = 0;};
  BreakpointRegister(Processor *, int, int );
  BreakpointRegister(Processor *, TriggerAction *, int, int );

  virtual REGISTER_TYPES isa(void) {return BP_REGISTER;};
  virtual string &name(void) const
    {
      if(m_replaced)
        return m_replaced->name();
      else
        return gpsimValue::name();
    };
  /* direct all accesses to the member functions of the
   * register that is being replaced. Note that we assume
   * "m_replaced" is properly initialized which it will be
   * if this object is accessed. (Why? well, we only access
   * register notify/breaks via the PIC's file register 
   * memory and never directly access them. But the only
   * way this instantiation can be accessed is if it successfully
   * replaced a file register object */

  virtual void put_value(unsigned int new_value)
    {
      m_replaced->put_value(new_value);
    }
  virtual void put(unsigned int new_value)
    {
      m_replaced->put(new_value);
    }

  virtual void putRV(RegisterValue rv)
    {
      m_replaced->putRV(rv);
    }

  virtual unsigned int get_value(void)
    {
      return(m_replaced->get_value());
    }
  virtual RegisterValue getRV(void)
    {
      return m_replaced->getRV();
    }
  virtual RegisterValue getRVN(void) {
    return m_replaced->getRVN();
  }
  virtual unsigned int get(void)
    {
      return(m_replaced->get());
    }

  virtual Register *getReg(void)
    {
      if(m_replaced)
        return m_replaced;
      else
        return this;
    }

  virtual void setbit(unsigned int bit_number, bool new_value)
    {
      m_replaced->setbit(bit_number, new_value);
    }

  virtual bool get_bit(unsigned int bit_number)
    {
      return(m_replaced->get_bit(bit_number));
    }

  virtual double get_bit_voltage(unsigned int bit_number)
    {
      return(m_replaced->get_bit_voltage(bit_number));
    }

  virtual bool hasBreak(void)
    { 
      return true;
    }

  virtual void update(void)
    {
      if(m_replaced)
        m_replaced->update();
    }

  virtual void add_xref(void *an_xref)
    {
      if(m_replaced)
        m_replaced->add_xref(an_xref);
    }

  virtual void remove_xref(void *an_xref)
    {
      if(m_replaced)
        m_replaced->remove_xref(an_xref);
    }

  void replace(Processor *_cpu, unsigned int reg);
  virtual bool set_break(void);
  unsigned int clear(unsigned int bp_num);
  virtual void print(void);
  virtual void clear(void);

};

class BreakpointRegister_Value : public BreakpointRegister
{
public:

  unsigned int break_value, break_mask;
  unsigned int m_uDefRegMask;
  string m_sOperator;

  BreakpointRegister_Value(void)
    { 
      m_replaced = 0;
      break_value = 0;
      break_mask = 0;
    }

  BreakpointRegister_Value(Processor *_cpu, 
        int _repl, 
        int bp, 
        unsigned int bv, 
        unsigned int bm );

  BreakpointRegister_Value(Processor *_cpu, 
         TriggerAction *pTA,
        int _repl, 
        int bp, 
        unsigned int bv, 
        unsigned int bm );

  BreakpointRegister_Value(Processor *_cpu, 
        int _repl, 
        int bp, 
        unsigned int bv, 
        unsigned int _operator,
        unsigned int bm );

  typedef bool (*PFNISBREAKCONDITION)(unsigned int uRegValue,
      unsigned int uRegMask, unsigned int uRegTestValue);
  PFNISBREAKCONDITION m_pfnIsBreak;

  enum {
    eBREquals,
    eBRNotEquals,
    eBRGreaterThen,
    eBRLessThen,
    eBRGreaterThenEquals,
    eBRLessThenEquals,
  };

  static bool IsEqualsBreakCondition(unsigned int uRegValue,
      unsigned int uRegMask, unsigned int uRegTestValue);
  static bool IsNotEqualsBreakCondition(unsigned int uRegValue,
      unsigned int uRegMask, unsigned int uRegTestValue);
  static bool IsGreaterThenBreakCondition(unsigned int uRegValue,
      unsigned int uRegMask, unsigned int uRegTestValue);
  static bool IsLessThenBreakCondition(unsigned int uRegValue,
      unsigned int uRegMask, unsigned int uRegTestValue);
  static bool IsGreaterThenEqualsBreakCondition(unsigned int uRegValue,
      unsigned int uRegMask, unsigned int uRegTestValue);
  static bool IsLessThenEqualsBreakCondition(unsigned int uRegValue,
      unsigned int uRegMask, unsigned int uRegTestValue);

  virtual void print(void);

};


class Break_register_read : public BreakpointRegister, TriggerAction
{
public:

  Break_register_read(Processor *_cpu, int _repl, int bp ):
    BreakpointRegister(_cpu, _repl,bp ) { 
    set_action(this);
  }

  virtual unsigned int get(void);
  virtual RegisterValue getRV(void);
  virtual RegisterValue getRVN(void);
  virtual bool get_bit(unsigned int bit_number);
  virtual double get_bit_voltage(unsigned int bit_number);
  virtual char const * bpName() { return "register read"; }

  // TriggerAction overrides
  virtual void action(void);
};

class Break_register_write : public BreakpointRegister, TriggerAction
{
public:
  Break_register_write(Processor *_cpu, int _repl, int bp ):
    BreakpointRegister(_cpu,_repl,bp ) { 
    set_action(this);
  }
  virtual void put(unsigned int new_value);
  virtual void putRV(RegisterValue rv);
  virtual void setbit(unsigned int bit_number, bool new_value);
  virtual char const * bpName() { return "register write"; }

  // TriggerAction overrides
  virtual void action(void);
};

class Break_register_read_value : public BreakpointRegister_Value, TriggerAction
{
public:
//  Break_register_read_value(void){ };
  Break_register_read_value(Processor *_cpu, 
			    int _repl, 
			    int bp, 
			    unsigned int bv, 
			    unsigned int bm );

  Break_register_read_value(Processor *_cpu, 
			    int _repl, 
			    int bp, 
			    unsigned int bv,
          unsigned int _operator,
			    unsigned int bm );

  virtual unsigned int get(void);
  virtual RegisterValue getRV(void);
  virtual RegisterValue getRVN(void);
  virtual bool get_bit(unsigned int bit_number);
  virtual double get_bit_voltage(unsigned int bit_number);
  virtual char const * bpName() { return "register read value"; }

  // TriggerAction overrides
  virtual void action(void);
};

class Break_register_write_value : public BreakpointRegister_Value, TriggerAction
{
public:
  Break_register_write_value(Processor *_cpu, 
			     int _repl, 
			     int bp, 
			     unsigned int bv, 
			     unsigned int bm );

  Break_register_write_value(Processor *_cpu, 
			     int _repl, 
			     int bp, 
			     unsigned int bv, 
           unsigned int _operator,
			     unsigned int bm );

  virtual void put(unsigned int new_value);
  virtual void putRV(RegisterValue rv);
  virtual void setbit(unsigned int bit_number, bool new_value);
  virtual char const * bpName() { return "register write value"; }

  // TriggerAction overrides
  virtual void action(void);
};

class CommandAssertion : public Breakpoint_Instruction
{
public:
  bool bPostAssertion; // True if assertion is checked after instruction simulates.

  CommandAssertion(Processor *new_cpu, 
                   unsigned int instAddress, 
                   unsigned int bp,
                   const char *_command,
                   bool bPostAssertion
                   );

  virtual void execute(void);
  virtual void print(void);
  virtual char const * bpName() { return "Register Assertion"; }
private:
  char *command;
};

class Log_Register_Write : public Break_register_write
{
 public:

//  Log_Register_Write(void){ };
  Log_Register_Write(Processor *_cpu, int _repl, int bp ):
    Break_register_write(_cpu,_repl,bp ) { };
  virtual void put(unsigned int new_value);
  virtual void putRV(RegisterValue rv);
  virtual void setbit(unsigned int bit_number, bool new_value);
  virtual char const * bpName() { return "log register write"; }

};

class Log_Register_Read : public Break_register_read
{
public:


//  Log_Register_Read(void){ };
  Log_Register_Read(Processor *_cpu, int _repl, int bp ):
    Break_register_read(_cpu,_repl,bp ) { };
  virtual unsigned int get(void);
  virtual RegisterValue getRV(void);
  virtual RegisterValue getRVN(void);
  virtual bool get_bit(unsigned int bit_number);
  virtual double get_bit_voltage(unsigned int bit_number);
  virtual char const * bpName() { return "log register read"; }

};

class Log_Register_Read_value : public BreakpointRegister_Value
{
public:

  Log_Register_Read_value(void){ };
  Log_Register_Read_value(Processor *_cpu, 
			  int _repl, 
			  int bp, 
			  unsigned int bv, 
			  unsigned int bm ) :
    BreakpointRegister_Value(_cpu,  _repl, bp, bv, bm ) { };
  virtual unsigned int get(void);
  virtual RegisterValue getRV(void);
  virtual bool get_bit(unsigned int bit_number);
  virtual double get_bit_voltage(unsigned int bit_number);
  virtual char const * bpName() { return "log register read value"; }
};

class Log_Register_Write_value : public BreakpointRegister_Value
{
public:

  Log_Register_Write_value(void){ };
  Log_Register_Write_value(Processor *_cpu, 
			   int _repl, 
			   int bp, 
			   unsigned int bv, 
			   unsigned int bm ) :
    BreakpointRegister_Value(_cpu,  _repl, bp, bv, bm ) { };

  virtual void put(unsigned int new_value);
  virtual void putRV(RegisterValue rv);
  virtual char const * bpName() { return "log register write value"; }

};

#ifdef HAVE_GUI
class GuiCallBack: public TriggerObject
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
