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
#include "cmd_manager.h"
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
Breakpoints &(*dummy_bp)(void) = get_bp;
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
int Breakpoints::set_breakpoint(BREAKPOINT_TYPES break_type, 
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
        ((_14bit_processor *)cpu)->wdt.set_breakpoint(BREAK_ON_WDT_TIMEOUT | breakpoint_number);
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


int Breakpoints::set_breakpoint(TriggerObject *bpo, Expression *pExpr)
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
  bpo->set_Expression(pExpr);

  if(get_active_cpu() != NULL)
    get_active_cpu()->NotifyBreakpointSet(bs, bpo);
  return bpn;
}

//------------------------------------------------------------------------
static BreakpointRegister_Value::BRV_Ops MapComparisonOperatorToBreakOperator(ComparisonOperator *pCompareOp) 
{
  if (pCompareOp)
    switch(pCompareOp->isa()) {
    case ComparisonOperator::eOpEq:
      return BreakpointRegister_Value::eBREquals;
    case ComparisonOperator::eOpGe:
      return BreakpointRegister_Value::eBRGreaterThenEquals;
    case ComparisonOperator::eOpGt:
      return BreakpointRegister_Value::eBRGreaterThen;
    case ComparisonOperator::eOpLe:
      return BreakpointRegister_Value::eBRLessThenEquals;
    case ComparisonOperator::eOpLt:
      return BreakpointRegister_Value::eBRLessThen;
    case ComparisonOperator::eOpNe:
      return BreakpointRegister_Value::eBRNotEquals;
    }

  return BreakpointRegister_Value::eBRInvalid;
}



//------------------------------------------------------------------------
//
int Breakpoints::set_break(gpsimObject::ObjectBreakTypes bt,
			   Register *pReg, 
			   Expression *pExpr)
{
  int iValue = -1;
  int iMask  = -1;
  bool bCompiledExpression = false;
  BreakpointRegister_Value::BRV_Ops op = BreakpointRegister_Value::eBRInvalid;

  Processor *pCpu = (pReg && pReg->get_cpu()) ? pReg->get_cpu() : get_active_cpu();

  Register *pRegInExpr = 0;
  if (pExpr) {

    /* attempt to compile expressions of these types:
     *
     *
     *             ComparisonOperator
     *                 /          \
     *              OpAnd     LiteralInteger
     *            /      \
     * register_symbol   LiteralInteger
     *
     *   --- OR --- 
     *
     *             ComparisonOperator
     *                 /          \
     *       register_symbol     LiteralInteger
     */

    ComparisonOperator *pCompareExpr = dynamic_cast<ComparisonOperator *>(pExpr);

    op  =  MapComparisonOperatorToBreakOperator(pCompareExpr);

    OpAnd* pLeftOp = pCompareExpr ? 
      dynamic_cast<OpAnd*>(pCompareExpr->getLeft()) : 0;

    LiteralSymbol *pLeftSymbol = pLeftOp ? 
      dynamic_cast<LiteralSymbol*>(pLeftOp->getLeft()) : 
      dynamic_cast<LiteralSymbol*>(pCompareExpr->getLeft()) ;

    register_symbol *pRegSym = pLeftSymbol ? 
      dynamic_cast<register_symbol*>(pLeftSymbol->GetSymbol()) : 0;

    pRegInExpr = pRegSym ? pRegSym->getReg() : 0;

    if (!pRegInExpr) {
      // Legacy code... try to cast the left most integer into a register.
      LiteralInteger *pLeftRegAsInteger = pLeftOp ? 
	dynamic_cast<LiteralInteger*>(pLeftOp->getLeft()) : 
	dynamic_cast<LiteralInteger*>(pCompareExpr->getLeft()) ;

      Integer *pRegAddress = pLeftRegAsInteger ?
	dynamic_cast<Integer*>(pLeftRegAsInteger->evaluate()) : 0;

      pRegInExpr = (pRegAddress && pCpu) ? &pCpu->rma[(int)pRegAddress->getVal()] : 0;

      delete pRegAddress;
    }

    LiteralInteger* pRightSymbol = pLeftOp ?
      dynamic_cast<LiteralInteger*>(pLeftOp->getRight()) : 0;
    Integer *pMask = pRightSymbol ?
      dynamic_cast<Integer*>(pRightSymbol->evaluate()) : 0;

    iMask = pCpu ? pCpu->register_mask() : iMask;

    gint64 i64=0;
    if (pMask) {
      pMask->get(i64);
      iMask = (int)i64;
    }


    LiteralInteger* pRightValue = pCompareExpr ?
      dynamic_cast<LiteralInteger*>(pCompareExpr->getRight()) : 0;
    Integer *pValue = pRightValue ?
      dynamic_cast<Integer*>(pRightValue->evaluate()) : 0;

    // Now check if this parsing was successful
    if (pReg == pRegInExpr && pValue) {
      bCompiledExpression = true;
      pValue->get(i64);
      iValue = (int)i64;
    }

    delete pMask;
    delete pValue;
  }

  // If there was no register passed in as an input and we failed to compile
  // the expression (and hence unable to extract a register from the expression)
  // then don't set a break.
  pReg = pReg ? pReg : pRegInExpr;
  if (!pReg)
    return -1;

  if (bt ==  gpsimObject::eBreakWrite) {
    if (bCompiledExpression) {
      delete pExpr;
      return set_breakpoint(new Break_register_write_value(pCpu, pReg->address, 0,iValue,op,iMask));
    } else
      return set_breakpoint(new Break_register_write(pCpu, pReg->address,0), pExpr);
  } else if (bt == gpsimObject::eBreakRead) {
    if (bCompiledExpression) {
      delete pExpr;
      return set_breakpoint(new Break_register_read_value(pCpu, pReg->address, 0,iValue,op,iMask));
    } else
      return set_breakpoint(new Break_register_read(pCpu, pReg->address,0), pExpr);
  }

  return -1;
}

bool Breakpoints::set_expression(unsigned int bpn, Expression *pExpr)
{
  if(bpn < MAX_BREAKPOINTS) {

    BreakStatus &bs = break_status[bpn];
    if(bs.bpo) {
      bs.bpo->set_Expression(pExpr);
      return true;
    }
  }
  return false;
}

int  Breakpoints::set_execution_break(Processor *cpu, 
				      unsigned int address,
				      Expression *pExpr)
{

  Breakpoint_Instruction *bpi = new Breakpoint_Instruction(cpu,address,0);

  return bp.set_breakpoint(bpi,pExpr);
}

int  Breakpoints::set_notify_break(Processor *cpu,
				   unsigned int address, 
				   TriggerObject *f1 = 0)
{
  GetTraceLog().enable_logging();

  Notify_Instruction *ni = new Notify_Instruction(cpu,address,0,f1);

  return bp.set_breakpoint(ni);

}

int Breakpoints::set_profile_start_break(Processor *cpu,
					 unsigned int address,
					 TriggerObject *f1)
{
  Profile_Start_Instruction *psi = new Profile_Start_Instruction(cpu,address,0,f1);

  return bp.set_breakpoint(psi);

}

int  Breakpoints::set_profile_stop_break(Processor *cpu, 
					 unsigned int address, 
					 TriggerObject *f1)
{
  Profile_Stop_Instruction *psi = new Profile_Stop_Instruction(cpu,address,0,f1);

  return bp.set_breakpoint(psi);
}

int  Breakpoints::set_read_break(Processor *cpu, unsigned int register_number)
{
  Break_register_read *brr = new Break_register_read(cpu,register_number,0);

  return bp.set_breakpoint(brr);
}

int  Breakpoints::set_write_break(Processor *cpu, unsigned int register_number)
{
  Break_register_write *brw = new Break_register_write(cpu,register_number,0);

  return bp.set_breakpoint(brw);
}

int  Breakpoints::set_read_value_break(Processor *cpu, 
				       unsigned int register_number,
				       unsigned int value, 
				       unsigned int mask)
{
  return set_read_value_break(cpu, register_number,
            BreakpointRegister_Value::eBREquals, value, mask);
}

int  Breakpoints::set_read_value_break(Processor *cpu, 
				       unsigned int register_number,
				       unsigned int op,
				       unsigned int value, 
				       unsigned int mask)
{

  Break_register_read_value *brrv = new Break_register_read_value(cpu,
                                                                  register_number,
                                                                  0,
                                                                  value,
                                                                  op,
                                                                  mask);


  return bp.set_breakpoint(brrv);
}

int  Breakpoints::set_write_value_break(Processor *cpu, 
					unsigned int register_number,
					unsigned int value,
					unsigned int mask)
{
  return set_write_value_break(cpu, register_number,
    BreakpointRegister_Value::eBREquals, value, mask);
}

int  Breakpoints::set_write_value_break(Processor *cpu, 
					unsigned int register_number,
					unsigned int op,
					unsigned int value,
					unsigned int mask)
{

  Break_register_write_value *brwv = new Break_register_write_value(cpu,
                                                                    register_number,
                                                                    0,
                                                                    value,
                                                                    op,
                                                                    mask);
  return bp.set_breakpoint(brwv);


}

int  Breakpoints::set_cycle_break(Processor *cpu,
					   guint64 future_cycle,
					   TriggerObject *f1)
{

  return(set_breakpoint (Breakpoints::BREAK_ON_CYCLE,
			 cpu, 
			 (unsigned int)(future_cycle & 0xffffffff), 
			 (unsigned int)(future_cycle>>32),
			 f1));    
}


int Breakpoints::set_stk_overflow_break(Processor *cpu)
{
  return(set_breakpoint (Breakpoints::BREAK_ON_STK_OVERFLOW, cpu, 0, 0));
}
int Breakpoints::set_stk_underflow_break(Processor *cpu)
{
  return(set_breakpoint (Breakpoints::BREAK_ON_STK_UNDERFLOW, cpu, 0, 0));
}

int  Breakpoints::set_wdt_break(Processor *cpu)
{
  if ((cpu->GetCapabilities() & Processor::eBREAKONWATCHDOGTIMER)
    == Processor::eBREAKONWATCHDOGTIMER) {
    // Set a wdt break only if one is not already set.
    if(!cpu14->wdt.hasBreak())
      return(set_breakpoint (Breakpoints::BREAK_ON_WDT_TIMEOUT, cpu, 0, 0));
  }
  else {
    // Need to add console object
    printf("Watch dog timer breaks not available on a %s processor\n", cpu->name().c_str());
  }
  return MAX_BREAKPOINTS;
}


int Breakpoints::set_notify_read(Processor *cpu,
					  unsigned int register_number)
{
  GetTraceLog().enable_logging();

  Log_Register_Read *lrr = new Log_Register_Read(cpu,register_number,0);

  return bp.set_breakpoint(lrr);
}

int Breakpoints::set_notify_write(Processor *cpu, 
					   unsigned int register_number)
{
  GetTraceLog().enable_logging();

  Log_Register_Write *lrw = new Log_Register_Write(cpu,register_number,0);

  return bp.set_breakpoint(lrw);

}
int Breakpoints::set_notify_read_value(Processor *cpu, 
						unsigned int register_number, 
						unsigned int value, 
						unsigned int mask)
{
  GetTraceLog().enable_logging();

  Log_Register_Read_value *lrrv = new Log_Register_Read_value(cpu,
							      register_number,
							      0,
							      value,
							      mask);
  return bp.set_breakpoint(lrrv);

}

int Breakpoints::set_notify_write_value(Processor *cpu,
						 unsigned int register_number,
						 unsigned int value, 
						 unsigned int mask)
{
  GetTraceLog().enable_logging();

  Log_Register_Write_value *lrwv = new Log_Register_Write_value(cpu,
								register_number,
								0,
								value,
								mask);
  return bp.set_breakpoint(lrwv);
}



int Breakpoints::check_cycle_break(unsigned int abp)
{

  cout << "cycle break: 0x" << hex << cycles.value << dec << " = " << cycles.value << endl;

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

bool Breakpoints::dump(TriggerObject *pTO)
{
  if (!pTO)
    return false;


  pTO->print();
  /*
  if (pTO->bHasExpression()) {
    cout << "    Expression:";
    pTO->printExpression();
  }
  if(pTO->message().size())
    GetUserInterface().DisplayMessage("    Message:%s\n", pTO->message().c_str());
  */

  return true;
}
bool Breakpoints::dump1(unsigned int bp_num, int dump_type)
{
  if(!bIsValid(bp_num)) {
    printf("Break point number: %d is out of range\n",bp_num);

    return false;
  }

  BreakStatus &bs = break_status[bp_num];

  if(bs.bpo) {
    switch(dump_type) {
    case BREAK_ON_EXECUTION:
      if(dynamic_cast<RegisterAssertion*>(bs.bpo) != 0) {
        // for 'break e' we skip RegisterAssertions
        // and dump user execution breaks.
	return false;
      }
      break;
    case BREAK_ON_REG_WRITE:
      if(dynamic_cast<Break_register_write *>(bs.bpo) != 0 ||
	 dynamic_cast<Break_register_write_value*>(bs.bpo) != 0) {
	// for 'break w' we dump register write classes
	break;
      }
      return false;
    case BREAK_ON_REG_READ:
      if(dynamic_cast<Break_register_read *>(bs.bpo) != 0 ||
	 dynamic_cast<Break_register_read_value*>(bs.bpo) != 0) {
	// for 'break r' we dump register read classes
	break;
      }
    default:
      break;
    }

    return dump(bs.bpo);
  } else {

    BREAKPOINT_TYPES break_type = break_status[bp_num].type;
    switch (break_type) {

    case BREAK_ON_CYCLE:
      {
	const char * pFormat = "%d: cycle 0x%" PRINTF_INT64_MODIFIER "x  = %" PRINTF_INT64_MODIFIER "d\n";

	guint64 cyc =  bs.arg2;
	cyc = (cyc <<32)  | bs.arg1;
	GetUserInterface().DisplayMessage(pFormat, bp_num, cyc, cyc);
      }
      break;

    case BREAK_ON_STK_UNDERFLOW:
    case BREAK_ON_STK_OVERFLOW:
      cout << hex << setw(0) << bp_num << ": " << bs.cpu->name() << "  ";
      cout << "stack " << ((break_type == BREAK_ON_STK_OVERFLOW)?"ov":"und") << "er flow\n";
      break;

    case BREAK_ON_WDT_TIMEOUT:
      cout << hex << setw(0) << bp_num << ": " << bs.cpu->name() << "  ";
      cout << "wdt time out\n";
      break;
    default:
      return false;
      break;

    }

  }

  return true;

}


void Breakpoints::dump(int dump_type)
{
  bool have_breakpoints = 0;
  if(dump_type != BREAK_ON_CYCLE)  {
    for(int i = 0; i<m_iMaxAllocated; i++)
      {
        if(dump1(i, dump_type))
          have_breakpoints = 1;
      }
  }
  if(dump_type == BREAK_DUMP_ALL || 
     dump_type == BREAK_ON_CYCLE)  {
    cout << "Internal Cycle counter break points" << endl;
    get_cycles().dump_breakpoints();
    have_breakpoints = 1;
    cout << endl;
  }
  if(!have_breakpoints)
    cout << "No user breakpoints are set" << endl;
}


instruction *Breakpoints::find_previous(Processor *cpu, 
					unsigned int address, 
					instruction *_this)
{
  Breakpoint_Instruction *p;
  p = (Breakpoint_Instruction*) cpu->pma->getFromAddress(address);

  if(!_this || p==_this)
    return 0;

  while(p->getReplaced()!=_this)
    {
      p=(Breakpoint_Instruction*)p->getReplaced();
    }
  return p;
}

void Breakpoints::clear(unsigned int b)
{
  if (!bIsValid(b))
    return;

  BreakStatus &bs = break_status[b];   // 

  if(bs.bpo) {
    bs.bpo->clear();
    bs.type = BREAK_CLEAR;
    get_active_cpu()->NotifyBreakpointCleared(bs, bs.bpo);
    //delete bs.bpo;  // FIXME - why does this delete cause a segv?
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
      ((_14bit_processor *)bs.cpu)->wdt.set_breakpoint(0);
    }
    break;

  default:
    bs.type = BREAK_CLEAR;
    break;

  }
  get_active_cpu()->NotifyBreakpointCleared(bs, NULL);
}

bool Breakpoints::bIsValid(unsigned int b)
{
  return b < MAX_BREAKPOINTS;
}

bool Breakpoints::bIsClear(unsigned int b)
{
  return  bIsValid(b) && break_status[b].type == BREAK_CLEAR;
}

void Breakpoints::set_message(unsigned int b,string &m)
{
  if (bIsValid(b) && break_status[b].type != BREAK_CLEAR && break_status[b].bpo)
    break_status[b].bpo->new_message(m);
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
  if(get_use_icd()) {
    icd_halt();
    return;
  }
  global_break |= GLOBAL_STOP_RUNNING;
  if(m_bExitOnBreak) {
    // Let the UI or client code choose how and
    // when to exit.
    GetUserInterface().NotifyExitOnBreak(0);
  }
}
Breakpoints::Breakpoints(void)
{
  m_iMaxAllocated = 0;
  breakpoint_number = 0;
  m_bExitOnBreak = false;

  for(int i=0; i<MAX_BREAKPOINTS; i++)
    break_status[i].type = BREAK_CLEAR;

}

//----------------------------------------------------------------------------
bool Breakpoint_Instruction::eval_Expression()
{
  if (bHasExpression())
    return !TriggerObject::eval_Expression();

  return true;
}

void Breakpoint_Instruction::execute(void)
{

  if( (cpu->simulation_mode == eSM_RUNNING) && 
      (simulation_start_cycle != get_cycles().value) &&
      eval_Expression()) {

    action->action();
    trace.breakpoint( (Breakpoints::BREAK_ON_EXECUTION>>8) | address );
  }
  else
    m_replaced->execute();
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

  m_replaced = new_cpu->pma->getFromAddress(address);

  set_action(new SimpleTriggerAction(this));
}

Processor* Breakpoint_Instruction::get_cpu(void)
{ 
  return dynamic_cast<Processor *>(cpu);
}
//-------------------------------------------------------------------

bool Breakpoint_Instruction::set_break(void)
{
  if(get_use_icd())
    bp.clear_all(get_cpu());

  unsigned int uIndex = get_cpu()->map_pm_address2index(address);

  if(uIndex < get_cpu()->program_memory_size()) {

    m_replaced = get_cpu()->pma->getFromIndex(uIndex);

    get_cpu()->pma->putToIndex(uIndex, this);

    if(get_use_icd())
      icd_set_break(address);

    return true;
  }

  return false;
}

void Breakpoint_Instruction::print(void)
{
  // Output example
  // 42: p17c756  Execution at 0x0123
  const char * pLabel = get_symbol_table().
    findProgramAddressLabel(address);
  const char * pFormat = *pLabel == 0 ? "%d: %s %s at %s0x%x\n"
                                      : "%d: %s %s at %s(0x%x)\n";
  GetUserInterface().DisplayMessage(pFormat,
    bpn, cpu->name().c_str(), bpName(), pLabel, address);

  TriggerObject::print();
}

void Breakpoint_Instruction::clear(void)
{
  if(get_use_icd())
    icd_clear_break();

  get_cpu()->pma->clear_break_at_address(address, this);
  get_cpu()->pma->getFromAddress(address)->update();
}

//------------------------------------------------------------------------
void Notify_Instruction::execute(void)
{
    if(callback)
	callback->callback();

    m_replaced->execute();
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
  bPostAssertion(_bPostAssertion),
  m_pfnIsAssertionBreak(IsAssertionEqualsBreakCondition) {
}

RegisterAssertion::RegisterAssertion(Processor *cpu,
				     unsigned int address,
				     unsigned int bp,
				     unsigned int _regAddress,
				     unsigned int _regMask,
				     unsigned int _operator,
				     unsigned int _regValue,
				     bool _bPostAssertion) :
  Breakpoint_Instruction(cpu, address,bp),
  regAddress(_regAddress),
  regMask(_regMask),
  regValue(_regValue),
  bPostAssertion(_bPostAssertion)
{
  switch(_operator) {
  case eRAEquals:
    m_pfnIsAssertionBreak = IsAssertionEqualsBreakCondition;
    break;
  case eRANotEquals:
    m_pfnIsAssertionBreak = IsAssertionNotEqualsBreakCondition;
    break;
  case eRAGreaterThen:
    m_pfnIsAssertionBreak = IsAssertionGreaterThenBreakCondition;
    break;
  case eRALessThen:
    m_pfnIsAssertionBreak = IsAssertionLessThenBreakCondition;
    break;
  case eRAGreaterThenEquals:
    m_pfnIsAssertionBreak = IsAssertionGreaterThenEqualsBreakCondition;
    break;
  case eRALessThenEquals:
    m_pfnIsAssertionBreak = IsAssertionLessThenEqualsBreakCondition;
    break;
  default:
    assert(false);
    break;
  }

}

bool RegisterAssertion::IsAssertionEqualsBreakCondition(unsigned int uRegValue,
  unsigned int uRegMask, unsigned int uRegTestValue) {
  return (uRegValue & uRegMask) != uRegTestValue;
}

bool RegisterAssertion::IsAssertionNotEqualsBreakCondition(unsigned int uRegValue,
  unsigned int uRegMask, unsigned int uRegTestValue) {
  return (uRegValue & uRegMask) == uRegTestValue;
}

bool RegisterAssertion::IsAssertionGreaterThenBreakCondition(unsigned int uRegValue,
  unsigned int uRegMask, unsigned int uRegTestValue) {
  return (uRegValue & uRegMask) <= uRegTestValue;
}

bool RegisterAssertion::IsAssertionLessThenBreakCondition(unsigned int uRegValue,
  unsigned int uRegMask, unsigned int uRegTestValue) {
  return (uRegValue & uRegMask) >= uRegTestValue;
}

bool RegisterAssertion::IsAssertionGreaterThenEqualsBreakCondition(unsigned int uRegValue,
  unsigned int uRegMask, unsigned int uRegTestValue) {
  return (uRegValue & uRegMask) < uRegTestValue;
}

bool RegisterAssertion::IsAssertionLessThenEqualsBreakCondition(unsigned int uRegValue,
  unsigned int uRegMask, unsigned int uRegTestValue) {
  return (uRegValue & uRegMask) > uRegTestValue;
}

//------------------------------------------------------------------------------
void RegisterAssertion::execute(void)
{
  // For "post" assertions, the instruction is simulated first
  // and then the register assertion is checked.

  if(bPostAssertion && m_replaced)
    m_replaced->execute();

  // If the assertion is true, and the "phase" of the instruction is
  // '0' then halt the simulation. Note, the reason for checking "phase"
  // is to ensure the assertion applies to the the proper cycle of a 
  // multi-cycle instruction. For example, an assertion applied to a
  // a "GOTO" instruction should only get checked before the instruction
  // executes if it's a pre-assertion or after it completes if it's a
  // post assertion.

  if( m_pfnIsAssertionBreak(PCPU->rma[regAddress].get_value(), regMask, regValue) &&
      (PCPU->pc->get_phase() == 0) )
  {

    cout  << "Caught Register assertion ";
    cout  << "while excuting at address " << address << endl;

    cout  << "register 0x" 
          << hex 
          << regAddress
          << " = 0x"
          << PCPU->rma[regAddress].get_value() << endl;

    cout  << "0x" << PCPU->rma[regAddress].get_value()
          << " & 0x" << regMask 
          << " != 0x" << regValue << endl;

    cout  << " regAddress =0x" << regAddress
          << " regMask = 0x" << regMask 
          << " regValue = 0x" << regValue << endl;

    PCPU->Debug();

    if( (PCPU->simulation_mode == eSM_RUNNING) && 
        (simulation_start_cycle != get_cycles().value)) {

      eval_Expression();
      action->action();
      trace.breakpoint( (Breakpoints::BREAK_ON_EXECUTION>>8) | address );

      return;
    }
  }
  
  // If this is not a post assertion, then the instruction executes after
  // the instruction simulates.

  if(!bPostAssertion && m_replaced)
    m_replaced->execute();

}

//------------------------------------------------------------------------------
void RegisterAssertion::print(void)
{
  Breakpoint_Instruction::print();
  Register & pReg = PCPU->rma[regAddress];
  string & sName = pReg.name();
  const char * pFormat = sName.empty()
    ? "  break when register %s0x%x ANDed with 0x%x equals 0x%x\n"
    : "  break when register %s(0x%x) ANDed with 0x%x equals 0x%x\n" ;
  GetUserInterface().DisplayMessage(pFormat,
    sName.c_str(), regAddress, regMask, regValue);
  TriggerObject::print();

}
//------------------------------------------------------------------------------
BreakpointRegister::BreakpointRegister()
  : TriggerObject(0), m_replaced(0)
{
}

BreakpointRegister::BreakpointRegister(Processor *_cpu, 
				       TriggerAction *pTA,
				       Register *pRepl)
  : TriggerObject(pTA), m_replaced(pRepl)
{
  if (_cpu) {

  }

}

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
  m_replaced = fr;
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
void BreakpointRegister::clear() 
{
  BreakpointRegister *br = dynamic_cast<BreakpointRegister *>(get_cpu()->registers[address]);
  if (br == this) {
    // at the head of the chain
    get_cpu()->registers[address] = m_replaced;
  }
  else {
    BreakpointRegister *pLast = br;
    // Find myself in the chain
    while(br != NULL) {
      if (br == this) {
        // found
        pLast->m_replaced = m_replaced;
        // for good measure
        m_replaced = NULL;
        break;
      }
      else {
        pLast = br;
        br = dynamic_cast<BreakpointRegister *>(br->m_replaced);
      }
    }
  }
  get_cpu()->registers[address]->update();
  return;
}

bool BreakpointRegister::set_break()
{
  return true;
}

void BreakpointRegister::print()
{
  Register * pReg = get_symbol_table().findRegister(address);
  if (pReg)
    GetUserInterface().DisplayMessage("%d: %s  %s: %s(0x%x)\n",
				      bpn, cpu->name().c_str(), 
				      bpName(), pReg->name().c_str(), 
				      address);
  else 
    GetUserInterface().DisplayMessage("%d:  %s: reg(0x%x)\n",
				      bpn, bpName(), 
				      address);
  TriggerObject::print();


}
string &BreakpointRegister::name(void) const
{
  return m_replaced ? m_replaced->name() : gpsimValue::name();
};

void BreakpointRegister::put_value(unsigned int new_value)
{
  m_replaced->put_value(new_value);
}
void BreakpointRegister::put(unsigned int new_value)
{
  m_replaced->put(new_value);
}

void BreakpointRegister::putRV(RegisterValue rv)
{
  m_replaced->putRV(rv);
}

unsigned int BreakpointRegister::get_value(void)
{
  return(m_replaced->get_value());
}
RegisterValue BreakpointRegister::getRV(void)
{
  return m_replaced->getRV();
}
RegisterValue BreakpointRegister::getRVN(void)
{
  return m_replaced->getRVN();
}
unsigned int BreakpointRegister::get(void)
{
  return(m_replaced->get());
}

Register *BreakpointRegister::getReg(void)
{
  return m_replaced ? m_replaced->getReg() : this; 
}

void BreakpointRegister::setbit(unsigned int bit_number, bool new_value)
{
  m_replaced->setbit(bit_number, new_value);
}

bool BreakpointRegister::get_bit(unsigned int bit_number)
{
  return(m_replaced->get_bit(bit_number));
}

double BreakpointRegister::get_bit_voltage(unsigned int bit_number)
{
  return(m_replaced->get_bit_voltage(bit_number));
}

bool BreakpointRegister::hasBreak(void)
{ 
  return true;
}

void BreakpointRegister::update(void)
{
  if(m_replaced)
    m_replaced->update();
}

void BreakpointRegister::add_xref(void *an_xref)
{
  if(m_replaced)
    m_replaced->add_xref(an_xref);
}
void BreakpointRegister::remove_xref(void *an_xref)
{
  if(m_replaced)
    m_replaced->remove_xref(an_xref);
}


//------------------------------------------------------------------------

//-------------------------------------------------------------------
BreakpointRegister_Value::BreakpointRegister_Value(
    Processor *_cpu, 
    int _repl, 
    int bp, 
    unsigned int bv, 
    unsigned int bm ) :
  BreakpointRegister(_cpu,_repl,bp ) 
{ 
  m_uDefRegMask = _cpu->register_mask();
  break_value = bv;
  break_mask = bm;
  m_pfnIsBreak = IsEqualsBreakCondition;
  m_sOperator = "==";

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
  m_uDefRegMask = _cpu->register_mask();
  break_value = bv;
  break_mask = bm;
  m_pfnIsBreak = IsEqualsBreakCondition;
  m_sOperator = "==";

  int regMask = (0x100 << (_cpu->register_size()-1)) - 1;

  if(break_mask == 0)
    break_mask = regMask;
}

BreakpointRegister_Value::BreakpointRegister_Value(
    Processor *_cpu, 
    int _repl, 
    int bp, 
    unsigned int bv, 
    unsigned int _operator,
    unsigned int bm ) :
  BreakpointRegister(_cpu,_repl,bp ) 
{
  m_uDefRegMask = _cpu->register_mask();
  break_value = bv;
  break_mask = bm;
  
  switch(_operator) {
  case eBREquals:
    m_pfnIsBreak = IsEqualsBreakCondition;
    m_sOperator = "==";
    break;
  case eBRNotEquals:
    m_pfnIsBreak = IsNotEqualsBreakCondition;
    m_sOperator = "!=";
    break;
  case eBRGreaterThen:
    m_pfnIsBreak = IsGreaterThenBreakCondition;
    m_sOperator = ">";
    break;
  case eBRLessThen:
    m_pfnIsBreak = IsLessThenBreakCondition;
    m_sOperator = "<";
    break;
  case eBRGreaterThenEquals:
    m_pfnIsBreak = IsGreaterThenEqualsBreakCondition;
    m_sOperator = ">=";
    break;
  case eBRLessThenEquals:
    m_pfnIsBreak = IsLessThenEqualsBreakCondition;
    m_sOperator = "<=";
    break;
  default:
    assert(false);
    break;
  }

  int regMask = (0x100 << (_cpu->register_size()-1)) - 1;

  if(break_mask == 0)
    break_mask = regMask;
}

bool BreakpointRegister_Value::IsEqualsBreakCondition(unsigned int uRegValue,
  unsigned int uRegMask, unsigned int uRegTestValue) {
  return (uRegValue & uRegMask) == uRegTestValue;
}

bool BreakpointRegister_Value::IsNotEqualsBreakCondition(unsigned int uRegValue,
  unsigned int uRegMask, unsigned int uRegTestValue) {
  return (uRegValue & uRegMask) != uRegTestValue;
}

bool BreakpointRegister_Value::IsGreaterThenBreakCondition(unsigned int uRegValue,
  unsigned int uRegMask, unsigned int uRegTestValue) {
  return (uRegValue & uRegMask) > uRegTestValue;
}

bool BreakpointRegister_Value::IsLessThenBreakCondition(unsigned int uRegValue,
  unsigned int uRegMask, unsigned int uRegTestValue) {
  return (uRegValue & uRegMask) < uRegTestValue;
}

bool BreakpointRegister_Value::IsGreaterThenEqualsBreakCondition(unsigned int uRegValue,
  unsigned int uRegMask, unsigned int uRegTestValue) {
  return (uRegValue & uRegMask) >= uRegTestValue;
}

bool BreakpointRegister_Value::IsLessThenEqualsBreakCondition(unsigned int uRegValue,
  unsigned int uRegMask, unsigned int uRegTestValue) {
  return (uRegValue & uRegMask) <= uRegTestValue;
}

/// BreakpointRegister_Value::print(void) - base class function
/// would be unusual to not be over ridden.
void BreakpointRegister_Value::print(void)
{
  Register *pReg = getReg();
  string & sName = pReg->name();
  const char * pFormat = sName.empty()
    ? "%d: %s  %s: break when register %s0x%x ANDed with 0x%x %s 0x%x\n"
    : "%d: %s  %s: break when register %s(0x%x) ANDed with 0x%x %s 0x%x\n" ;
  GetUserInterface().DisplayMessage(pFormat,
				    bpn,cpu->name().c_str(), bpName(),
				    sName.c_str(), pReg->address, break_mask, 
				    m_sOperator.c_str(),break_value);
  TriggerObject::print();

}


//-------------------------------------------------------------------
//
void Break_register_read::action(void)
{
  if(verbosity && verbosity->getVal()) {

    GetUserInterface().DisplayMessage(IDS_HIT_BREAK,bpn);

    string sFormattedRegAddress;
    sFormattedRegAddress = GetUserInterface().FormatRegisterAddress(getReg());
    GetUserInterface().DisplayMessage(IDS_BREAK_READING_REG,
				      sFormattedRegAddress.c_str());
  }
  bp.halt();
}

void Break_register_write::action(void) 
{
  if(verbosity && verbosity->getVal()) {
    GetUserInterface().DisplayMessage(IDS_HIT_BREAK,bpn);
    string sFormattedRegAddress;
    sFormattedRegAddress = GetUserInterface().FormatRegisterAddress(
      address, 0);
    GetUserInterface().DisplayMessage(IDS_BREAK_WRITING_REG,
      sFormattedRegAddress.c_str());
  }
  bp.halt();
}

void Break_register_read_value::action()
{
  if(verbosity && verbosity->getVal()) {

    GetUserInterface().DisplayMessage(IDS_HIT_BREAK,bpn);

    string sFormattedRegAddress;
    sFormattedRegAddress = GetUserInterface().FormatRegisterAddress(getReg());

    if(break_mask != m_uDefRegMask) {
      sFormattedRegAddress += " & ";
      sFormattedRegAddress += GetUserInterface().FormatLabeledValue("",
        break_mask);
    }

    GetUserInterface().DisplayMessage(IDS_BREAK_READING_REG_OP_VALUE,
				      sFormattedRegAddress.c_str(), 
				      m_sOperator.c_str(),
				      break_value);
  }
  bp.halt();
}

Break_register_write_value::Break_register_write_value(Processor *_cpu, 
                              int _repl, 
                              int bp, 
                              unsigned int bv, 
                              unsigned int bm ) :
    BreakpointRegister_Value(_cpu, _repl, bp, bv, eBREquals, bm )
{
  set_action(this);
}

Break_register_write_value::Break_register_write_value(Processor *_cpu, 
                              int _repl, 
                              int bp, 
                              unsigned int bv, 
                              unsigned int _operator,
                              unsigned int bm ) :
    BreakpointRegister_Value(_cpu, _repl, bp, bv, _operator, bm )
{
    set_action(this);
}

void Break_register_write_value::action() 
{
  if(verbosity && verbosity->getVal()) {

    GetUserInterface().DisplayMessage(IDS_HIT_BREAK,bpn);

    string sFormattedRegAddress;
    sFormattedRegAddress = GetUserInterface().FormatRegisterAddress(getReg());

    if(break_mask != m_uDefRegMask) {
      sFormattedRegAddress += " & ";
      sFormattedRegAddress += GetUserInterface().FormatLabeledValue("",
        break_mask);
    }

    GetUserInterface().DisplayMessage(IDS_BREAK_WRITING_REG_OP_VALUE,
				      sFormattedRegAddress.c_str(), 
				      m_sOperator.c_str(),
				      break_value);

  }
  bp.halt();
}

unsigned int Break_register_read::get(void)
{

  if(eval_Expression()) {
    TriggerObject::action->action();
    trace.breakpoint( (Breakpoints::BREAK_ON_REG_READ>>8) 
		      | address);
  }
  return(m_replaced->get());

}

RegisterValue  Break_register_read::getRV(void)
{
  if(eval_Expression()) {
    TriggerObject::action->action();
    trace.breakpoint( (Breakpoints::BREAK_ON_REG_READ>>8) 
		      | address);
  }
  return(m_replaced->getRV());
}

RegisterValue  Break_register_read::getRVN(void)
{
  if(eval_Expression()) {
    TriggerObject::action->action();
    trace.breakpoint( (Breakpoints::BREAK_ON_REG_READ>>8) 
		      | address);
  }
  return(m_replaced->getRVN());
}

bool Break_register_read::get_bit(unsigned int bit_number)
{
  if(eval_Expression()) {
    TriggerObject::action->action();
    trace.breakpoint( (Breakpoints::BREAK_ON_REG_READ>>8) 
		      | address);
  }
  return(m_replaced->get_bit(bit_number));
}

double Break_register_read::get_bit_voltage(unsigned int bit_number)
{
  return m_replaced->get_bit_voltage(bit_number);
}



void Break_register_write::put(unsigned int new_value)
{
  m_replaced->put(new_value);
  if(eval_Expression()){
    TriggerObject::action->action();
    trace.breakpoint( (Breakpoints::BREAK_ON_REG_WRITE>>8) 
		      | (m_replaced->address)  );
  }
}

void Break_register_write::putRV(RegisterValue rv)
{
  m_replaced->putRV(rv);
  if(eval_Expression()){
    TriggerObject::action->action();
    trace.breakpoint( (Breakpoints::BREAK_ON_REG_WRITE>>8) 
		      | (m_replaced->address)  );
  }
}

void Break_register_write::setbit(unsigned int bit_number, bool new_value)
{
  m_replaced->setbit(bit_number,new_value);
  if(eval_Expression()) {
    TriggerObject::action->action();
    trace.breakpoint( (Breakpoints::BREAK_ON_REG_WRITE>>8) 
		      | (m_replaced->address)  );
  }
}

Break_register_read_value::Break_register_read_value(Processor *_cpu, 
			    int _repl, 
			    int bp, 
			    unsigned int bv, 
			    unsigned int bm ) :
    BreakpointRegister_Value(_cpu, _repl, bp, bv, eBREquals, bm ) {
  set_action(this);
}

Break_register_read_value::Break_register_read_value(Processor *_cpu, 
			    int _repl, 
			    int bp, 
			    unsigned int bv, 
          unsigned int _operator,
			    unsigned int bm ) :
    BreakpointRegister_Value(_cpu, _repl, bp, bv, _operator, bm ) {
  set_action(this);
}

unsigned int Break_register_read_value::get(void)
{
  unsigned int v = m_replaced->get();

  if(m_pfnIsBreak(v, break_mask, break_value)) {
    TriggerObject::action->action();
    trace.breakpoint( (Breakpoints::BREAK_ON_REG_READ>>8) 
		| address);
  }
  return v;
}

RegisterValue  Break_register_read_value::getRV(void)
{
  RegisterValue v = m_replaced->getRV();

  if(m_pfnIsBreak(v.data, break_mask, break_value)) {
    TriggerObject::action->action();
    trace.breakpoint( (Breakpoints::BREAK_ON_REG_READ>>8) 
	  | address);
  }
  return(v);
}

RegisterValue  Break_register_read_value::getRVN(void)
{
  RegisterValue v = m_replaced->getRVN();

  if(m_pfnIsBreak(v.data, break_mask, break_value)) {
    TriggerObject::action->action();
    trace.breakpoint( (Breakpoints::BREAK_ON_REG_READ>>8) 
		| address);
  }
  return(v);
}

bool Break_register_read_value::get_bit(unsigned int bit_number)
{
  unsigned int v = m_replaced->get();
  unsigned int mask = 1<<(bit_number & 7);

  if( (break_mask & mask) && (v & mask) == (break_value&mask)) {
    TriggerObject::action->action();
    trace.breakpoint( (Breakpoints::BREAK_ON_REG_READ>>8) 
		| address);
  }
  return m_replaced->get_bit(bit_number);
}

double Break_register_read_value::get_bit_voltage(unsigned int bit_number)
{
  return m_replaced->get_bit_voltage(bit_number);
}



void Break_register_write_value::put(unsigned int new_value)
{

  if(m_pfnIsBreak(new_value, break_mask, break_value)) {
    TriggerObject::action->action();
    trace.breakpoint( (Breakpoints::BREAK_ON_REG_WRITE>>8) 
		| address);
  }
  m_replaced->put(new_value);
}

void Break_register_write_value::putRV(RegisterValue rv)
{
  
//  if((rv.data & break_mask) == break_value) {
  if(m_pfnIsBreak(rv.data, break_mask, break_value)) {
    TriggerObject::action->action();
    trace.breakpoint( (Breakpoints::BREAK_ON_REG_WRITE>>8) 
		| (m_replaced->address)  );
  }

  m_replaced->putRV(rv);
}


void Break_register_write_value::setbit(unsigned int bit_number, bool new_bit)
{
  int val_mask = 1 << bit_number;
  int new_value = ((int)new_bit) << bit_number;

  if( (val_mask & break_mask) &&
      ( ( (m_replaced->value.get() & ~val_mask)  // clear the old bit
          | new_value)                   // set the new bit
        & break_mask) == break_value)
  {
      TriggerObject::action->action();
      trace.breakpoint( (Breakpoints::BREAK_ON_REG_WRITE>>8) 
			| address);
  }

  m_replaced->setbit(bit_number,new_value ? true  : false);

}
//------------------------------------------------------------------------
// CommandAssertion
//
// Associates a gpsim command with an instruction. I.e. when the simulated
// instruction is executed, the gpsim command will execute first and then
// the instruction is simulated.


CommandAssertion::CommandAssertion(Processor *new_cpu, 
                                   unsigned int instAddress, 
                                   unsigned int bp,
                                   const char *_command,
                                   bool _bPostAssertion)
  : Breakpoint_Instruction(new_cpu, instAddress, bp), bPostAssertion(_bPostAssertion)
{
  int len = (int)strlen(_command);
  command = (char *)malloc(len+3);
  strcpy(command,_command);
  command[len]   = '\n';
  command[len+1] = 0;
  command[len+2] = 0;
}

void CommandAssertion::execute()
{
  if(bPostAssertion && m_replaced)
    m_replaced->execute();

  //printf("execute command: %s -- post = %s\n",command,(bPostAssertion?"true":"false"));

  ICommandHandler *pCli = CCommandManager::GetManager().find("gpsimCLI");
  if(pCli) {
    pCli->Execute(command, 0);
  }
  if(!bPostAssertion && m_replaced)
    m_replaced->execute();
}

//------------------------------------------------------------------------------
void CommandAssertion::print()
{
  Breakpoint_Instruction::print();
  cout << "  execute command " << command << endl;
}


//============================================================================


// Log_Register_write::put
//  Here, register writes are captured and stored into the GetTraceLog().buffer.
// where they can be written to a file

void Log_Register_Write::put(unsigned int new_value)
{
    int v;

  // First perform the write operation:

  m_replaced->put(new_value);

#if 1
  // Finally, record the value that was written to the register.
  // Note that 'get_value' is used instead of directly referencing
  // the register's value. This is because the actual value written
  // differ from the value that was attempted to be written. (E.g.
  // this only happens in special function registers).

  v = m_replaced->get_value();

#else

  // another option is to log the value the simulated pic was trying
  // to write. I'm not sure which is more useful.

  v = new_value;

#endif

  GetTraceLog().register_write(m_replaced->address, v, get_cycles().value);

}

void Log_Register_Write::putRV(RegisterValue rv)
{
  m_replaced->putRV(rv);
  GetTraceLog().register_write(m_replaced->address, rv.data, get_cycles().value);
}

void Log_Register_Write::setbit(unsigned int bit_number, bool new_value)
{

  m_replaced->setbit(bit_number,new_value);
  
  GetTraceLog().register_write( m_replaced->address, m_replaced->get_value(), get_cycles().value);

}

unsigned int Log_Register_Read::get(void)
{
  int v = m_replaced->get();
  GetTraceLog().register_read(m_replaced->address, v, get_cycles().value);
  return v;

}

RegisterValue Log_Register_Read::getRV(void)
{
  RegisterValue rv = m_replaced->getRV();
  GetTraceLog().register_read(m_replaced->address, rv.data, get_cycles().value);
  return rv;

}

RegisterValue Log_Register_Read::getRVN(void)
{
  RegisterValue rv = m_replaced->getRVN();
  GetTraceLog().register_read(m_replaced->address, rv.data, get_cycles().value);
  return rv;

}

bool Log_Register_Read::get_bit(unsigned int bit_number)
{
  bool v = m_replaced->get_bit(bit_number);
  GetTraceLog().register_read(m_replaced->address, v, get_cycles().value);
  return v;

}

double Log_Register_Read::get_bit_voltage(unsigned int bit_number)
{
  return m_replaced->get_bit_voltage(bit_number);
}

unsigned int Log_Register_Read_value::get(void)
{
  unsigned int v = m_replaced->get();

  if( (v & break_mask) == break_value)
    {
      GetTraceLog().register_read_value(m_replaced->address, v, get_cycles().value);
    }

  return v;
}

RegisterValue Log_Register_Read_value::getRV(void)
{
  RegisterValue rv = m_replaced->getRV();

  if( (rv.data & break_mask) == break_value)
    {
      GetTraceLog().register_read_value(m_replaced->address, rv.data, get_cycles().value);
    }

  return rv;
}

bool Log_Register_Read_value::get_bit(unsigned int bit_number)
{
  unsigned int v = m_replaced->get();
  unsigned int mask = 1<<(bit_number & 7);

  if( (break_mask & mask) && (v & mask) == (break_value&mask))
    GetTraceLog().register_read_value(m_replaced->address, v, get_cycles().value);

  return m_replaced->get_bit(bit_number);
}

double Log_Register_Read_value::get_bit_voltage(unsigned int bit_number)
{
  return m_replaced->get_bit_voltage(bit_number);
}

void Log_Register_Write_value::put(unsigned int new_value)
{

  if((new_value & break_mask) == break_value)
    {
      GetTraceLog().register_write_value(m_replaced->address, break_value, get_cycles().value);
    }
  m_replaced->put(new_value);
}

void Log_Register_Write_value::putRV(RegisterValue new_rv)
{

  if((new_rv.data & break_mask) == break_value)
    {
      GetTraceLog().register_write_value(m_replaced->address, break_value, get_cycles().value);
    }
  m_replaced->putRV(new_rv);
}
