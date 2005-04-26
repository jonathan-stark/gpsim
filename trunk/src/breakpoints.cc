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


#include <iostream>
#include <iomanip>

#include "cmd_gpsim.h"
#include "../config.h"
#include "pic-processor.h"
#include "breakpoints.h"
#include "14bit-processors.h"
#include "xref.h"

#include "icd.h"

extern "C"{
#include "lxt_write.h"
}

#define PCPU ((Processor *)cpu)

extern guint64 simulation_start_cycle;

// Global declaration of THE breakpoint object
// create an instance of inline get_trace() method by taking its address
static Breakpoints &(*dummy_bp)(void) = get_bp;
Breakpoints bp;


//------------------------------------------------------------------------
// find_free - search the array that holds the break points for a free slot
// 
int Breakpoints::find_free(void)
{

  for(int i=0; i<MAX_BREAKPOINTS; i++) {

    if(break_status[i].type == BREAK_CLEAR)  {
      if (i + 1 > m_iMaxAllocated)
          m_iMaxAllocated = i + 1;
      return i;
    }
  }

  cout << "*** out of breakpoints\n";
  return(MAX_BREAKPOINTS);

}

//------------------------------------------------------------------------
// set_breakpoint - Set a breakpoint of a specific type.
//
unsigned int Breakpoints::set_breakpoint(BREAKPOINT_TYPES break_type, 
					 Processor *cpu,
					 unsigned int arg1, 
					 unsigned arg2, 
					 TriggerObject *f1)
{
  Register *fr;

  breakpoint_number = find_free();
  if(breakpoint_number >= MAX_BREAKPOINTS)
    return breakpoint_number;

  BreakStatus &bs = break_status[breakpoint_number];
  bs.type = break_type;
  bs.cpu  = cpu;
  bs.arg1 = arg1;
  bs.arg2 = arg2;
  bs.bpo  = f1;
  switch (break_type)
    {

    case BREAK_ON_INVALID_FR:
      fr = cpu->registers[arg1];
      return(breakpoint_number);
      break;

    case BREAK_ON_CYCLE:
      {
      guint64 cyc = arg2;
      cyc = (cyc<<32) | arg1;

      // The cycle counter does its own break points.
      if(get_cycles().set_break(cyc, f1, breakpoint_number))
	      return(breakpoint_number);
      else
	      bs.type = BREAK_CLEAR;
      }
      break;

    case BREAK_ON_STK_OVERFLOW:
      if ((cpu->GetCapabilities() & Processor::eBREAKONSTACKOVER)
        == Processor::eBREAKONSTACKOVER) {
        // pic_processor should not be referenced here
        // Should have a GetStack() virtual function in Processor class.
        // Of course then the Stack class needs to be a virtual class.
        if(((pic_processor *)(cpu))->stack->set_break_on_overflow(1))
          return (breakpoint_number);
      }
      else {
        // Need to add console object
        printf("Stack breaks not available on a %s processor\n", cpu->name().c_str());
      }
      bs.type = BREAK_CLEAR;
      break;

    case BREAK_ON_STK_UNDERFLOW:
      if ((cpu->GetCapabilities() & Processor::eBREAKONSTACKUNDER)
        == Processor::eBREAKONSTACKUNDER) {
        // pic_processor should not be referenced here
        // Should have a GetStack() virtual function in Processor class.
        // Of course then the Stack class needs to be a virtual class.
        if(((pic_processor *)(cpu))->stack->set_break_on_underflow(1))
          return (breakpoint_number);
      }
      else {
        // Need to add console object
        printf("Stack breaks not available on a %s processor\n", cpu->name().c_str());
      }
      bs.type = BREAK_CLEAR;
      break;

    case BREAK_ON_WDT_TIMEOUT:
      if ((cpu->GetCapabilities() & Processor::eBREAKONWATCHDOGTIMER)
        == Processor::eBREAKONWATCHDOGTIMER) {
        // pic_processor should not be referenced here
        // Should have a GetStack() virtual function in Processor class.
        // Of course then the Stack class needs to be a virtual class.
        ((_14bit_processor *)cpu)->wdt.break_point = BREAK_ON_WDT_TIMEOUT | breakpoint_number;
        return(breakpoint_number);
      }
      else {
        // Need to add console object
        printf("Watch dog timer breaks not available on a %s processor\n", cpu->name().c_str());
      }
    default:   // Not a valid type
      bs.type = BREAK_CLEAR;
      break;
    }
  return(MAX_BREAKPOINTS);
}


unsigned int Breakpoints::set_breakpoint(TriggerObject *bpo)
{
  int bpn = find_free();

  if(bpn >= MAX_BREAKPOINTS || !bpo->set_break()) {
    delete bpo;
    return MAX_BREAKPOINTS;
  }
  BreakStatus &bs = break_status[bpn];
  bs.bpo = bpo;
  bs.type = BREAK_MASK;   // place holder for now...
  bpo->bpn = bpn;

  if(get_active_cpu() != NULL)
    get_active_cpu()->NotifyBreakpointSet(bs, bpo);
  return bpn;
}

unsigned int  Breakpoints::set_execution_break(Processor *cpu, 
					       unsigned int address)
{

  Breakpoint_Instruction *bpi = new Breakpoint_Instruction(cpu,address,0);

  return bp.set_breakpoint(bpi);
}

unsigned int  Breakpoints::set_notify_break(Processor *cpu,
					    unsigned int address, 
					    TriggerObject *f1 = 0)
{
  trace_log.enable_logging();

  Notify_Instruction *ni = new Notify_Instruction(cpu,address,0,f1);

  return bp.set_breakpoint(ni);

}

unsigned int Breakpoints::set_profile_start_break(Processor *cpu,
						  unsigned int address,
						  TriggerObject *f1)
{
  Profile_Start_Instruction *psi = new Profile_Start_Instruction(cpu,address,0,f1);

  return bp.set_breakpoint(psi);

}

unsigned int  Breakpoints::set_profile_stop_break(Processor *cpu, 
						  unsigned int address, 
						  TriggerObject *f1)
{
  Profile_Stop_Instruction *psi = new Profile_Stop_Instruction(cpu,address,0,f1);

  return bp.set_breakpoint(psi);
}

unsigned int  Breakpoints::set_read_break(Processor *cpu, unsigned int register_number)
{
  Break_register_read *brr = new Break_register_read(cpu,register_number,0);

  return bp.set_breakpoint(brr);
}

unsigned int  Breakpoints::set_write_break(Processor *cpu, unsigned int register_number)
{
  Break_register_write *brw = new Break_register_write(cpu,register_number,0);

  return bp.set_breakpoint(brw);
}

unsigned int  Breakpoints::set_read_value_break(Processor *cpu, 
						unsigned int register_number,
						unsigned int value, 
						unsigned int mask)
{

  Break_register_read_value *brrv = new Break_register_read_value(cpu,
								  register_number,
								  0,
								  value,
								  mask);


  return bp.set_breakpoint(brrv);
}

unsigned int  Breakpoints::set_write_value_break(Processor *cpu, 
						 unsigned int register_number,
						 unsigned int value,
						 unsigned int mask)
{

  Break_register_write_value *brwv = new Break_register_write_value(cpu,
								    register_number,
								    0,
								    value,
								    mask);
  return bp.set_breakpoint(brwv);


}

unsigned int  Breakpoints::set_cycle_break(Processor *cpu,
					   guint64 future_cycle,
					   TriggerObject *f1)
{

  return(set_breakpoint (Breakpoints::BREAK_ON_CYCLE,
			 cpu, 
			 (unsigned int)(future_cycle & 0xffffffff), 
			 (unsigned int)(future_cycle>>32),
			 f1));    
}


unsigned int Breakpoints::set_stk_overflow_break(Processor *cpu)
{
  return(set_breakpoint (Breakpoints::BREAK_ON_STK_OVERFLOW, cpu, 0, 0));
}
unsigned int Breakpoints::set_stk_underflow_break(Processor *cpu)
{
  return(set_breakpoint (Breakpoints::BREAK_ON_STK_UNDERFLOW, cpu, 0, 0));
}

unsigned int  Breakpoints::set_wdt_break(Processor *cpu)
{
  if ((cpu->GetCapabilities() & Processor::eBREAKONWATCHDOGTIMER)
    == Processor::eBREAKONWATCHDOGTIMER) {
    // Set a wdt break only if one is not already set.
    if(cpu14->wdt.break_point == 0)
      return(set_breakpoint (Breakpoints::BREAK_ON_WDT_TIMEOUT, cpu, 0, 0));
  }
  else {
    // Need to add console object
    printf("Watch dog timer breaks not available on a %s processor\n", cpu->name().c_str());
  }
  return MAX_BREAKPOINTS;
}


unsigned int Breakpoints::set_notify_read(Processor *cpu,
					  unsigned int register_number)
{
  trace_log.enable_logging();

  Log_Register_Read *lrr = new Log_Register_Read(cpu,register_number,0);

  return bp.set_breakpoint(lrr);
}

unsigned int Breakpoints::set_notify_write(Processor *cpu, 
					   unsigned int register_number)
{
  trace_log.enable_logging();

  Log_Register_Write *lrw = new Log_Register_Write(cpu,register_number,0);

  return bp.set_breakpoint(lrw);

}
unsigned int Breakpoints::set_notify_read_value(Processor *cpu, 
						unsigned int register_number, 
						unsigned int value, 
						unsigned int mask)
{
  trace_log.enable_logging();

  Log_Register_Read_value *lrrv = new Log_Register_Read_value(cpu,
							      register_number,
							      0,
							      value,
							      mask);
  return bp.set_breakpoint(lrrv);

}

unsigned int Breakpoints::set_notify_write_value(Processor *cpu,
						 unsigned int register_number,
						 unsigned int value, 
						 unsigned int mask)
{
  trace_log.enable_logging();

  Log_Register_Write_value *lrwv = new Log_Register_Write_value(cpu,
								register_number,
								0,
								value,
								mask);
  return bp.set_breakpoint(lrwv);
}



unsigned int Breakpoints::check_cycle_break(unsigned int abp)
{
  if(verbose)
    cout << "cycle break is halting sim\n";

  halt();
  if( abp < MAX_BREAKPOINTS)
    {
      if (break_status[abp].bpo)
	  break_status[abp].bpo->callback();

      trace.breakpoint( (Breakpoints::BREAK_ON_CYCLE>>8) );

      clear(abp);
    }

  return(1);

}

bool Breakpoints::dump1(unsigned int bp_num)
{

  if(break_status[bp_num].bpo) {
    break_status[bp_num].bpo->print();
    return true;
  }

  bool set_by_user = 0;

  BREAKPOINT_TYPES break_type = break_status[bp_num].type;

  switch (break_type)
    {
    case BREAK_ON_CYCLE:
      cout << hex << setw(0) << bp_num << ": " << break_status[bp_num].cpu->name() << "  ";
      {
      guint64 cyc =  break_status[bp_num].arg2;
      cyc = (cyc <<32)  | break_status[bp_num].arg1;
      cout << "cycle " << hex << setw(16) << setfill('0') <<  cyc << '\n';
      }
      set_by_user = 1;
      break;

    case BREAK_ON_STK_UNDERFLOW:
    case BREAK_ON_STK_OVERFLOW:
      cout << hex << setw(0) << bp_num << ": " << break_status[bp_num].cpu->name() << "  ";
      cout << "stack " << ((break_type == BREAK_ON_STK_OVERFLOW)?"ov":"und") << "er flow\n";
      set_by_user = 1;
      break;

    case BREAK_ON_WDT_TIMEOUT:
      cout << hex << setw(0) << bp_num << ": " << break_status[bp_num].cpu->name() << "  ";
      cout << "wdt time out\n";
      set_by_user = 1;
      break;
    default:
      break;

    }

  return(set_by_user);

}


void Breakpoints::dump(void)
{
  bool have_breakpoints = 0;
  for(int i = 0; i<m_iMaxAllocated; i++)
    {
      if(dump1(i))
        have_breakpoints = 1;
    }

  cout << "Internal Cycle counter break points" << endl;
  get_cycles().dump_breakpoints();
  cout << endl;

  if(!have_breakpoints)
    cout << "No user breakpoints are set" << endl;
}


instruction *Breakpoints::find_previous(Processor *cpu, 
					unsigned int address, 
					instruction *_this)
{
  Breakpoint_Instruction *p;
  p = (Breakpoint_Instruction*) cpu->pma->get(address);

  if(!_this || p==_this)
    return 0;

  while(p->replaced!=_this)
    {
      p=(Breakpoint_Instruction*)p->replaced;
    }
  return p;
}

void Breakpoints::clear(unsigned int b)
{

  if(b<MAX_BREAKPOINTS) {

    BreakStatus &bs = break_status[b];   // 

    if(bs.bpo) {

      bs.bpo->clear();
      bs.type = BREAK_CLEAR;
      get_active_cpu()->NotifyBreakpointCleared(bs, bs.bpo);
      delete bs.bpo;  // FIXME - why does this delete cause a segv?
      bs.bpo = 0;
      return;
    }

    switch (bs.type) {

    case BREAK_ON_CYCLE:
      bs.type = BREAK_CLEAR;
      //cout << "Cleared cycle breakpoint number " << b << '\n';
      break;

    case BREAK_ON_STK_OVERFLOW:
      bs.type = BREAK_CLEAR;
      if ((bs.cpu->GetCapabilities() & Processor::eSTACK)
        == Processor::eSTACK) {
        if(((pic_processor *)(bs.cpu))->stack->set_break_on_overflow(0))
          cout << "Cleared stack overflow break point.\n";
        else
          cout << "Stack overflow break point is already cleared.\n";
      }
      break;

    case BREAK_ON_STK_UNDERFLOW:
      bs.type = BREAK_CLEAR;
      if ((bs.cpu->GetCapabilities() & Processor::eSTACK)
        == Processor::eSTACK) {
        if(((pic_processor *)(bs.cpu))->stack->set_break_on_underflow(0))
          cout << "Cleared stack underflow break point.\n";
        else
          cout << "Stack underflow break point is already cleared.\n";
      }
      break;

    case BREAK_ON_WDT_TIMEOUT:
      bs.type = BREAK_CLEAR;
      if ((bs.cpu->GetCapabilities() & Processor::eBREAKONWATCHDOGTIMER)
        == Processor::eBREAKONWATCHDOGTIMER) {
        cout << "Cleared wdt timeout breakpoint number " << b << '\n';
        ((_14bit_processor *)bs.cpu)->wdt.break_point = 0;
      }
      break;

    default:
      bs.type = BREAK_CLEAR;
      break;

    }
    get_active_cpu()->NotifyBreakpointCleared(bs, NULL);
  }
}

//
//  dump_traced
//  Called by the trace class to display a breakpoint that is in the
// trace buffer.

void Breakpoints::dump_traced(unsigned int b)
{

  BREAKPOINT_TYPES break_type = (BREAKPOINT_TYPES) ((b & 0xff0000) << 8);

  switch (break_type)
    {
    case BREAK_ON_EXECUTION:
      cout << "execution at "<< hex << setw(4) << setfill('0') <<  (b & 0xffff) << '\n';
      break;

    case BREAK_ON_REG_WRITE:
      cout << "reg write: " << hex << setw(2) << setfill('0') <<  (b & 0xff) << '\n';
      break;

    case BREAK_ON_REG_WRITE_VALUE:
      cout << "wrote " << hex << setw(2) << setfill('0') <<  ((b & 0xff00)>>8) << 
	" to register " << hex << setw(2) << setfill('0') <<  (b & 0xff) << '\n';
      break;

    case BREAK_ON_REG_READ:
      cout << "reg write: " << hex << setw(2) << setfill('0') <<  (b & 0xff) << '\n';
      break;

    case BREAK_ON_REG_READ_VALUE:
      cout << "read " << hex << setw(2) << setfill('0') <<  ((b & 0xff00)>>8) << 
	" from register " << hex << setw(2) << setfill('0') <<  (b & 0xff) << '\n';
      break;

    case BREAK_ON_CYCLE:
      cout << "cycle " << '\n';
      break;

    case BREAK_ON_WDT_TIMEOUT:
      cout << "wdt time out\n";
      break;

    default:
      cout << "unknown\n";
    }



}


// Clear all break points that are set for a specific processor
// This only be called when a processor is being removed and not when a user 
// wants to clear the break points. Otherwise, internal break points like
// invalid register accesses will get cleared.

void Breakpoints::clear_all(Processor *c)
{
  for(int i=0; i<MAX_BREAKPOINTS; i++)
    {
      if(break_status[i].type != BREAK_CLEAR)
      	clear(i);
    }
}

void Breakpoints::clear_all_set_by_user(Processor *c)
{

  for(int i=0; i<MAX_BREAKPOINTS; i++)
    {
      if((c == break_status[i].cpu) && (break_status[i].type != BREAK_ON_INVALID_FR))
	clear(i);
    }

}

//--------------------------------------------------
// Clear all of the break points that are set on a register
//
// FIXME -- this tacitly assumes "register memory". Thus it's
// not possible to use this function on EEPROM or module registers.

void Breakpoints::clear_all_register(Processor *c,unsigned int address)
{

  if(!c || address<0 || address > c->register_memory_size())
    return;


  while(c->registers[address]->isa()==Register::BP_REGISTER) {

    BreakpointRegister *nr = dynamic_cast<BreakpointRegister *>(c->registers[address]);

    if(!nr)
      return;

    bp.clear(nr->bpn & ~Breakpoints::BREAK_MASK);
  }
}

void Breakpoints::halt(void)
{
  if(use_icd) {
    icd_halt();
    return;
  }
  global_break |= GLOBAL_STOP_RUNNING;
}
Breakpoints::Breakpoints(void)
{
  m_iMaxAllocated = 0;
  breakpoint_number = 0;

  for(int i=0; i<MAX_BREAKPOINTS; i++)
    break_status[i].type = BREAK_CLEAR;

}

//----------------------------------------------------------------------------
void Breakpoint_Instruction::execute(void)
{

  if( (cpu->simulation_mode == RUNNING) && (simulation_start_cycle != get_cycles().value)) {

    if(eval_Expression()) {
      action->action();
      trace.breakpoint( (Breakpoints::BREAK_ON_EXECUTION>>8) | address );
    }
  } else {
    replaced->execute();
  }

}

Breakpoint_Instruction::Breakpoint_Instruction(Processor *new_cpu, 
					       unsigned int new_address,
					       unsigned int bp)
  : TriggerObject(0)
{
  cpu = new_cpu;
  address = new_address;
  opcode = 0xffffffff;
  bpn = bp;

  replaced = new_cpu->pma->get(address);

  set_action(new SimpleTriggerAction(this));
}

Processor* Breakpoint_Instruction::get_cpu(void)
{ 
  return dynamic_cast<Processor *>(cpu);
}
//-------------------------------------------------------------------
void Breakpoint_Instruction::new_message(char *s)
{

  message_str = string(s);
}


void Breakpoint_Instruction::new_message(string &new_message)
{
  message_str = new_message;
}


unsigned int Breakpoint_Instruction::get_opcode(void)
{ 
  return(replaced->get_opcode());
}
int Breakpoint_Instruction::get_src_line(void)
{
  return(replaced->get_src_line());
}
int Breakpoint_Instruction::get_hll_src_line(void)
{
  return(replaced->get_hll_src_line());
}
int Breakpoint_Instruction::get_lst_line(void)
{
  return(replaced->get_lst_line());
}

int Breakpoint_Instruction::get_file_id(void)
{
  return(replaced->get_file_id());
}
int Breakpoint_Instruction::get_hll_file_id(void)
{
  return(replaced->get_hll_file_id());
}

char * Breakpoint_Instruction::name(char *return_str,int len)
{

  return(replaced->name(return_str,len));
}

bool Breakpoint_Instruction::set_break(void)
{
  if(use_icd)
    bp.clear_all(get_cpu());

  if(address < get_cpu()->program_memory_size()) {

    replaced = get_cpu()->pma->get(address);

    get_cpu()->pma->put(address, this);

    if(use_icd)
      icd_set_break(address);

    return true;
  }

  return false;
}

void Breakpoint_Instruction::print(void)
{
  cout << hex << setw(0) << bpn << ": " << cpu->name() << "  ";
  cout << bpName() << " at " << hex << setw(4) << setfill('0') <<  address << '\n';
  if(message_str.size())
    cout << "   " << message_str << endl;

}

void Breakpoint_Instruction::clear(void)
{
  if(use_icd)
    icd_clear_break();

  get_cpu()->pma->clear_break_at_address(address, this);
  (*get_cpu()->pma)[address].update();

}

//------------------------------------------------------------------------
void Notify_Instruction::execute(void)
{
    if(callback)
	callback->callback();

    replaced->execute();
}

Notify_Instruction::Notify_Instruction(Processor *cpu, 
				       unsigned int address, 
				       unsigned int bp, 
				       TriggerObject *cb) : 
  Breakpoint_Instruction(cpu, address,bp)
{
    callback=cb;
    
}
//------------------------------------------------------------------------
Profile_Start_Instruction::Profile_Start_Instruction(Processor *cpu, 
						     unsigned int address, 
						     unsigned int bp, 
						     TriggerObject *cb) : 
  Notify_Instruction(cpu, address, bp, cb)
{
    
}

Profile_Stop_Instruction::Profile_Stop_Instruction(Processor *cpu, 
						   unsigned int address, 
						   unsigned int bp, 
						   TriggerObject *cb) : 
  Notify_Instruction(cpu, address, bp, cb)
{
    
}
//------------------------------------------------------------------------------
RegisterAssertion::RegisterAssertion(Processor *cpu,
				     unsigned int address,
				     unsigned int bp,
				     unsigned int _regAddress,
				     unsigned int _regMask,
				     unsigned int _regValue,
				     bool _bPostAssertion) :
  Breakpoint_Instruction(cpu, address,bp),
  regAddress(_regAddress),
  regMask(_regMask),
  regValue(_regValue),
  bPostAssertion(_bPostAssertion)
{

}

//------------------------------------------------------------------------------
void RegisterAssertion::execute(void)
{
  // For "post" assertions, the instruction is simulated first
  // and then the register assertion is checked.

  if(bPostAssertion && replaced)
    replaced->execute();

  // If the assertion is true, and the "phase" of the instruction is
  // '0' then halt the simulation. Note, the reason for checking "phase"
  // is to ensure the assertion applies to the the proper cycle of a 
  // multi-cycle instruction. For example, an assertion applied to a
  // a "GOTO" instruction should only get checked before the instruction
  // executes if it's a pre-assertion or after it completes if it's a
  // post assertion.

  if( (((PCPU->rma[regAddress].get_value()) & regMask) != regValue) &&
      (PCPU->pc->get_phase() == 0) )
  {

    cout << "Caught Register assertion ";
    cout << "while excuting at address " << address << endl;

    cout << "register 0x" 
	 << hex 
	 << regAddress
	 << " = 0x"
	 << PCPU->rma[regAddress].get_value() << endl;

    cout << "0x" << PCPU->rma[regAddress].get_value()
	 << " & 0x" << regMask 
	 << " != 0x" << regValue << endl;

    cout << " regAddress =0x" << regAddress
	 << " regMask = 0x" << regMask 
	 << " regValue = 0x" << regValue << endl;

    PCPU->Debug();

    if( (PCPU->simulation_mode == RUNNING) && 
	(simulation_start_cycle != get_cycles().value)) {

      eval_Expression();
      trace.breakpoint( (Breakpoints::BREAK_ON_EXECUTION>>8) | address );

      return;
    }
  }
  
  // If this is not a post assertion, then the instruction executes after
  // the instruction simulates.

  if(!bPostAssertion && replaced)
    replaced->execute();

}

//------------------------------------------------------------------------------
void RegisterAssertion::print(void)
{
  Breakpoint_Instruction::print();
  cout << "  break when register 0x" << regAddress 
       << " ANDed with 0x"  << regMask << " equals 0x" << regValue << endl;
}
//------------------------------------------------------------------------------
BreakpointRegister::BreakpointRegister(Processor *_cpu, TriggerAction *ta,
                                       int _repl, int bp)
  : TriggerObject(ta)

{

  bpn = bp;
  replace(_cpu,_repl);
  address = _repl;
}

BreakpointRegister::BreakpointRegister(Processor *_cpu, int _repl, int bp)
  : TriggerObject(0)

{

  bpn = bp;
  replace(_cpu,_repl);
  address = _repl;
}

void BreakpointRegister::replace(Processor *_cpu, unsigned int reg)
{
  Register *fr = _cpu->registers[reg];

  cpu = _cpu;
  _cpu->registers[reg] = this;
  replaced = fr;
  address=fr->address;
  
  update();

}
  
unsigned int BreakpointRegister::clear(unsigned int bp_num)
{
  clear();
  return 1;
}

/// BreakpointRegister::clear() will delete itself from the
/// chain of BreakpointRegister objects.
/// All derived classes that override this function need to
/// call this function of this base class.

//  Note: There should be a RegisterChain class and this code
//  should exist in the RegisterChain class. get_cpu()->registers
// would then be an array of RegisterChains.
void BreakpointRegister::clear(void) {
  BreakpointRegister *br = dynamic_cast<BreakpointRegister *>(get_cpu()->registers[address]);
  if (br == this) {
    // at the head of the chain
    get_cpu()->registers[address] = replaced;
  }
  else {
    BreakpointRegister *pLast = br;
    // Find myself in the chain
    while(br != NULL) {
      if (br == this) {
        // found
        pLast->replaced = replaced;
        // for good measure
        replaced = NULL;
        break;
      }
      else {
        pLast = br;
        br = dynamic_cast<BreakpointRegister *>(br->replaced);
      }
    }
  }
  get_cpu()->registers[address]->update();
  return;
}

bool BreakpointRegister::set_break(void)
{
  return true;
}

void BreakpointRegister::print(void)
{
  cout << hex << setw(0) << bpn << ": " << cpu->name() << "  ";
  cout << bpName() << ": 0x" << hex <<  address << endl;

}
//-------------------------------------------------------------------
BreakpointRegister_Value::BreakpointRegister_Value(
    Processor *_cpu, 
    int _repl, 
    int bp, 
    unsigned int bv, 
    unsigned int bm ) :
  BreakpointRegister(_cpu,_repl,bp ) 
{ 
  break_value = bv;
  break_mask = bm;

  int regMask = (0x100 << (_cpu->register_size()-1)) - 1;

  if(break_mask == 0)
    break_mask = regMask;
}

BreakpointRegister_Value::BreakpointRegister_Value(
    Processor *_cpu, 
    TriggerAction * pTA,
    int _repl, 
    int bp, 
    unsigned int bv, 
    unsigned int bm ) :
  BreakpointRegister(_cpu,pTA,_repl,bp ) 
{ 
  break_value = bv;
  break_mask = bm;

  int regMask = (0x100 << (_cpu->register_size()-1)) - 1;

  if(break_mask == 0)
    break_mask = regMask;
}

void BreakpointRegister_Value::print(void)
{
  cout << hex << setw(0) << bpn << ": " << cpu->name() << "  ";
  cout << bpName() << ": address=0x" << hex <<  address 
       << "  value=0x" << break_value << "  mask=0x" << break_mask << endl;
  /*
  cout << hex << setw(0) << bp_num << ": " << break_status[bp_num].cpu->name_str << "  ";
  cout << "reg write. " << ( (break_type == BREAK_ON_REG_WRITE_VALUE) ?  "Break" : "Log") 
	   << " when 0x" << hex  
	   <<  (break_status[bp_num].arg2 & 0xff)
	   << " is written to register 0x" << break_status[bp_num].arg1 << '\n';
  */
}
//-------------------------------------------------------------------
//
void Break_register_read::TA::action(void) {
  if(verbosity && verbosity->getVal())
    GetUserInterface().DisplayMessage(IDS_BREAK_READING_REG, m_uAddress);
  bp.halt();
}

void Break_register_write::TA::action(void) {
  if(verbosity && verbosity->getVal())
    GetUserInterface().DisplayMessage(IDS_BREAK_WRITING_REG, m_uAddress);
  bp.halt();
}

void Break_register_read_value::TA::action(void) {
  if(verbosity && verbosity->getVal())
    GetUserInterface().DisplayMessage(IDS_BREAK_READING_REG_VALUE,
      m_uAddress, m_uValue);
  bp.halt();
}

void Break_register_write_value::TA::action(void) {
  if(verbosity && verbosity->getVal())
    GetUserInterface().DisplayMessage(IDS_BREAK_WRITING_REG_VALUE,
      m_uAddress, m_uValue);
  bp.halt();
}

unsigned int Break_register_read::get(void)
{

  if(eval_Expression()) {
    action->action();
    trace.breakpoint( (Breakpoints::BREAK_ON_REG_READ>>8) 
		      | address);
  }
  return(replaced->get());

}

RegisterValue  Break_register_read::getRV(void)
{
  if(eval_Expression()) {
    action->action();
    trace.breakpoint( (Breakpoints::BREAK_ON_REG_READ>>8) 
		      | address);
  }
  return(replaced->getRV());
}

RegisterValue  Break_register_read::getRVN(void)
{
  if(eval_Expression()) {
    action->action();
    trace.breakpoint( (Breakpoints::BREAK_ON_REG_READ>>8) 
		      | address);
  }
  return(replaced->getRVN());
}

bool Break_register_read::get_bit(unsigned int bit_number)
{
  if(eval_Expression()) {
    action->action();
    trace.breakpoint( (Breakpoints::BREAK_ON_REG_READ>>8) 
		      | address);
  }
  return(replaced->get_bit(bit_number));
}

double Break_register_read::get_bit_voltage(unsigned int bit_number)
{
  return replaced->get_bit_voltage(bit_number);
}



void Break_register_write::put(unsigned int new_value)
{
  replaced->put(new_value);
  if(eval_Expression()){
    action->action();
    trace.breakpoint( (Breakpoints::BREAK_ON_REG_WRITE>>8) 
		      | (replaced->address)  );
  }
}

void Break_register_write::putRV(RegisterValue rv)
{
  replaced->putRV(rv);
  if(eval_Expression()){
    action->action();
    trace.breakpoint( (Breakpoints::BREAK_ON_REG_WRITE>>8) 
		      | (replaced->address)  );
  }
}

void Break_register_write::setbit(unsigned int bit_number, bool new_value)
{
  replaced->setbit(bit_number,new_value);
  if(eval_Expression()) {
    action->action();
    trace.breakpoint( (Breakpoints::BREAK_ON_REG_WRITE>>8) 
		      | (replaced->address)  );
  }
}

unsigned int Break_register_read_value::get(void)
{
  unsigned int v = replaced->get();

  if( (v & break_mask) == break_value)
    if(eval_Expression()) {
      action->action();
      trace.breakpoint( (Breakpoints::BREAK_ON_REG_READ>>8) 
			| address);
    }
  return v;
}

RegisterValue  Break_register_read_value::getRV(void)
{
  RegisterValue v = replaced->getRV();

  if( (v.data & break_mask) == break_value)
    if(eval_Expression()) {
      action->action();
      trace.breakpoint( (Breakpoints::BREAK_ON_REG_READ>>8) 
			| address);
    }
  return(v);
}

RegisterValue  Break_register_read_value::getRVN(void)
{
  RegisterValue v = replaced->getRVN();

  if( (v.data & break_mask) == break_value)
    if(eval_Expression()) {
      action->action();
      trace.breakpoint( (Breakpoints::BREAK_ON_REG_READ>>8) 
			| address);
    }
  return(v);
}

bool Break_register_read_value::get_bit(unsigned int bit_number)
{
  unsigned int v = replaced->get();
  unsigned int mask = 1<<(bit_number & 7);

  if( (break_mask & mask) && (v & mask) == (break_value&mask))
    if(eval_Expression()) {
      action->action();
      trace.breakpoint( (Breakpoints::BREAK_ON_REG_READ>>8) 
			| address);
    }
  return replaced->get_bit(bit_number);
}

double Break_register_read_value::get_bit_voltage(unsigned int bit_number)
{
  return replaced->get_bit_voltage(bit_number);
}



void Break_register_write_value::put(unsigned int new_value)
{

  if((new_value & break_mask) == break_value)
    if(eval_Expression()) {
      action->action();
      trace.breakpoint( (Breakpoints::BREAK_ON_REG_WRITE>>8) 
			| address);
    }
  replaced->put(new_value);
}

void Break_register_write_value::putRV(RegisterValue rv)
{
  
  if((rv.data & break_mask) == break_value)
    if(eval_Expression()) {
      action->action();
      trace.breakpoint( (Breakpoints::BREAK_ON_REG_WRITE>>8) 
			| (replaced->address)  );
    }

  replaced->putRV(rv);
}


void Break_register_write_value::setbit(unsigned int bit_number, bool new_bit)
{
  int val_mask = 1 << bit_number;
  int new_value = ((int)new_bit) << bit_number;

  if( (val_mask & break_mask) &&
      ( ((replaced->value.get() & ~val_mask)  // clear the old bit
	 | new_value)                   // set the new bit
	& break_mask) == break_value)
    {
      if(eval_Expression()) {
        action->action();
	trace.breakpoint( (Breakpoints::BREAK_ON_REG_WRITE>>8) 
			  | address);
      }
    }

  replaced->setbit(bit_number,new_value ? true  : false);

}

//============================================================================


// Log_Register_write::put
//  Here, register writes are captured and stored into the trace_log.buffer.
// where they can be written to a file

void Log_Register_Write::put(unsigned int new_value)
{
    int v;

  // First perform the write operation:

  replaced->put(new_value);

#if 1
  // Finally, record the value that was written to the register.
  // Note that 'get_value' is used instead of directly referencing
  // the register's value. This is because the actual value written
  // differ from the value that was attempted to be written. (E.g.
  // this only happens in special function registers).

  v = replaced->get_value();

#else

  // another option is to log the value the simulated pic was trying
  // to write. I'm not sure which is more useful.

  v = new_value;

#endif

  trace_log.register_write(replaced->address, v, get_cycles().value);

}

void Log_Register_Write::putRV(RegisterValue rv)
{
  replaced->putRV(rv);
  trace_log.register_write(replaced->address, rv.data, get_cycles().value);
}

void Log_Register_Write::setbit(unsigned int bit_number, bool new_value)
{

  replaced->setbit(bit_number,new_value);
  
  trace_log.register_write( replaced->address, replaced->get_value(), get_cycles().value);

}

unsigned int Log_Register_Read::get(void)
{
  int v = replaced->get();
  trace_log.register_read(replaced->address, v, get_cycles().value);
  return v;

}

RegisterValue Log_Register_Read::getRV(void)
{
  RegisterValue rv = replaced->getRV();
  trace_log.register_read(replaced->address, rv.data, get_cycles().value);
  return rv;

}

RegisterValue Log_Register_Read::getRVN(void)
{
  RegisterValue rv = replaced->getRVN();
  trace_log.register_read(replaced->address, rv.data, get_cycles().value);
  return rv;

}

bool Log_Register_Read::get_bit(unsigned int bit_number)
{
  bool v = replaced->get_bit(bit_number);
  trace_log.register_read(replaced->address, v, get_cycles().value);
  return v;

}

double Log_Register_Read::get_bit_voltage(unsigned int bit_number)
{
  return replaced->get_bit_voltage(bit_number);
}

unsigned int Log_Register_Read_value::get(void)
{
  unsigned int v = replaced->get();

  if( (v & break_mask) == break_value)
    {
      trace_log.register_read_value(replaced->address, v, get_cycles().value);
    }

  return v;
}

RegisterValue Log_Register_Read_value::getRV(void)
{
  RegisterValue rv = replaced->getRV();

  if( (rv.data & break_mask) == break_value)
    {
      trace_log.register_read_value(replaced->address, rv.data, get_cycles().value);
    }

  return rv;
}

bool Log_Register_Read_value::get_bit(unsigned int bit_number)
{
  unsigned int v = replaced->get();
  unsigned int mask = 1<<(bit_number & 7);

  if( (break_mask & mask) && (v & mask) == (break_value&mask))
    trace_log.register_read_value(replaced->address, v, get_cycles().value);

  return replaced->get_bit(bit_number);
}

double Log_Register_Read_value::get_bit_voltage(unsigned int bit_number)
{
  return replaced->get_bit_voltage(bit_number);
}

void Log_Register_Write_value::put(unsigned int new_value)
{

  if((new_value & break_mask) == break_value)
    {
      trace_log.register_write_value(replaced->address, break_value, get_cycles().value);
    }
  replaced->put(new_value);
}

void Log_Register_Write_value::putRV(RegisterValue new_rv)
{

  if((new_rv.data & break_mask) == break_value)
    {
      trace_log.register_write_value(replaced->address, break_value, get_cycles().value);
    }
  replaced->putRV(new_rv);
}



