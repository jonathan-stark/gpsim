/*
   Copyright (C) 1998 T. Scott Dattalo

This file is part of the libgpsim library of gpsim

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see
<http://www.gnu.org/licenses/lgpl-2.1.html>.
*/


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
Breakpoints &(*dummy_bp)() = get_bp;
Breakpoints bp;

//------------------------------------------------------------------------
// find_free - search the array that holds the break points for a free slot
//
int Breakpoints::find_free()
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
      return(breakpoint_number);
      break;

    case BREAK_ON_CYCLE:
      {
      guint64 cyc = arg2;
      cyc = (cyc<<32) | arg1;

      // The cycle counter does its own break points.
      if(get_cycles().set_break(cyc, f1, breakpoint_number)) {
        if(cpu != NULL) {
          cpu->NotifyBreakpointSet(bs, f1);
        }
	      return(breakpoint_number);
      }
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

//------------------------------------------------------------------------
//BreakTraceType *m_brt=0;

int Breakpoints::set_breakpoint(TriggerObject *bpo, Processor *pCpu, Expression *pExpr)
{
  int bpn = find_free();

  if(bpn >= MAX_BREAKPOINTS || !bpo->set_break()) {
    delete bpo;
    return MAX_BREAKPOINTS;
  }

  BreakStatus &bs = break_status[bpn];
  bs.bpo = bpo;
  bs.type = BREAK_MASK;   // place holder for now...
  bs.cpu = pCpu;
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
int Breakpoints::set_break(gpsimObject::ObjectBreakTypes bt, gpsimObject::ObjectActionTypes at,
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
     *
     *   --- OR ---
     *
     *                  OpAnd      (not implemented)
     *                 /     \
     *      register_symbol   LiteralInteger
     *
     */

    ComparisonOperator *pCompareExpr = dynamic_cast<ComparisonOperator *>(pExpr);

    op  =  MapComparisonOperatorToBreakOperator(pCompareExpr);
    if (op != BreakpointRegister_Value::eBRInvalid) {

      OpAnd* pLeftOp = dynamic_cast<OpAnd*>(pCompareExpr->getLeft());

      LiteralSymbol *pLeftSymbol = pLeftOp ?
	dynamic_cast<LiteralSymbol*>(pLeftOp->getLeft()) :
	dynamic_cast<LiteralSymbol*>(pCompareExpr->getLeft());


      Register *pRegSym = pLeftSymbol ?
       dynamic_cast<Register *>(pLeftSymbol->GetSymbol()) : 0;

      pRegInExpr = pRegSym ? pRegSym->getReg() : 0;
      //pRegInExpr = pLeftSymbol ? dynamic_cast<Register*>(pLeftSymbol) : 0;

      if (!pRegInExpr) {
	// Legacy code... try to cast the left most integer into a register.
	LiteralInteger *pLeftRegAsInteger = pLeftOp ?
	  dynamic_cast<LiteralInteger*>(pLeftOp->getLeft()) :
	  dynamic_cast<LiteralInteger*>(pCompareExpr->getLeft());

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


      LiteralInteger* pRightValue = dynamic_cast<LiteralInteger*>(pCompareExpr->getRight());
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
  }

  // If there was no register passed in as an input and we failed to compile
  // the expression (and hence unable to extract a register from the expression)
  // then don't set a break.
  if (!pReg && !pRegInExpr)
  {
     fprintf(stderr, "set_break failed - no register given and expression compile failed\n");
     return -1;
  }
  pReg = pReg ? pReg : pRegInExpr;

  if (pReg->address == AN_INVALID_ADDRESS)
  {
      fprintf(stderr, "set_break failed - invalid address\n");
      return -1;
  }

  if (bt ==  gpsimObject::eBreakWrite) {
    if (bCompiledExpression) {
      delete pExpr;
      return (at == gpsimObject::eActionLog) ?
	set_breakpoint(new Log_Register_Write_value(pCpu, pReg->address, 0,iValue,op,iMask),pCpu)
	:
	set_breakpoint(new Break_register_write_value(pCpu, pReg->address, 0,iValue,op,iMask),pCpu);
    } else
      return (at == gpsimObject::eActionLog) ?
	set_breakpoint(new Log_Register_Write(pCpu, pReg->address,0), pCpu, pExpr)
	:
	set_breakpoint(new Break_register_write(pCpu, pReg->address,0), pCpu, pExpr);
  } else if (bt == gpsimObject::eBreakRead) {
    if (bCompiledExpression) {
      delete pExpr;
      return (at == gpsimObject::eActionLog) ?
	set_breakpoint(new Log_Register_Read_value(pCpu, pReg->address, 0,iValue,op,iMask),pCpu)
	:
	set_breakpoint(new Break_register_read_value(pCpu, pReg->address, 0,iValue,op,iMask),pCpu);
    } else
      return (at == gpsimObject::eActionLog) ?
	set_breakpoint(new Log_Register_Read(pCpu, pReg->address,0), pCpu, pExpr)
	:
	set_breakpoint(new Break_register_read(pCpu, pReg->address,0), pCpu, pExpr);
  } else if (bt == gpsimObject::eBreakChange) {
    if (bCompiledExpression) {
      delete pExpr;
      return (at == gpsimObject::eActionLog) ?
	set_breakpoint(new Log_Register_Write_value(pCpu, pReg->address, 0,iValue,op,iMask),pCpu)
	:
	set_breakpoint(new Break_register_write_value(pCpu, pReg->address, 0,iValue,op,iMask),pCpu);
    } else
      return (at == gpsimObject::eActionLog) ?
	set_breakpoint(new Log_Register_Write(pCpu, pReg->address,0), pCpu, pExpr)
	:
	set_breakpoint(new Break_register_change(pCpu, pReg->address,0), pCpu, pExpr);
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
  if (!cpu || !cpu->pma || !cpu->pma->hasValid_opcode_at_address(address))
    return -1;

  Breakpoint_Instruction *bpi = new Breakpoint_Instruction(cpu,address,0);

  return bp.set_breakpoint(bpi,cpu,pExpr);
}

int  Breakpoints::set_notify_break(Processor *cpu,
				   unsigned int address,
				   TriggerObject *f1 = 0)
{
  GetTraceLog().enable_logging();

  Notify_Instruction *ni = new Notify_Instruction(cpu,address,0,f1);

  return bp.set_breakpoint(ni,cpu);

}

int Breakpoints::set_profile_start_break(Processor *cpu,
					 unsigned int address,
					 TriggerObject *f1)
{
  Profile_Start_Instruction *psi = new Profile_Start_Instruction(cpu,address,0,f1);

  return bp.set_breakpoint(psi,cpu);

}

int  Breakpoints::set_profile_stop_break(Processor *cpu,
					 unsigned int address,
					 TriggerObject *f1)
{
  Profile_Stop_Instruction *psi = new Profile_Stop_Instruction(cpu,address,0,f1);

  return bp.set_breakpoint(psi,cpu);
}

int  Breakpoints::set_read_break(Processor *cpu, unsigned int register_number)
{
  Break_register_read *brr = new Break_register_read(cpu,register_number,0);

  return bp.set_breakpoint(brr,cpu);
}

int  Breakpoints::set_write_break(Processor *cpu, unsigned int register_number)
{
  Break_register_write *brw = new Break_register_write(cpu,register_number,0);

  return bp.set_breakpoint(brw,cpu);
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
                                                                  value,
                                                                  op,
                                                                  BreakpointRegister_Value::eBREquals,
                                                                  mask);


  return bp.set_breakpoint(brrv,cpu);
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
  return bp.set_breakpoint(brwv,cpu);


}

int  Breakpoints::set_change_break(Processor *cpu, unsigned int register_number)
{
  Break_register_change *brc = new Break_register_change(cpu,register_number,0);

  return bp.set_breakpoint(brc,cpu);
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

  return bp.set_breakpoint(lrr,cpu);
}

int Breakpoints::set_notify_write(Processor *cpu,
					   unsigned int register_number)
{
  GetTraceLog().enable_logging();

  Log_Register_Write *lrw = new Log_Register_Write(cpu,register_number,0);

  return bp.set_breakpoint(lrw,cpu);

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
                                                              BreakpointRegister_Value::eBREquals,
							      mask);
  return bp.set_breakpoint(lrrv,cpu);

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
                                                                BreakpointRegister_Value::eBREquals,
								mask);
  return bp.set_breakpoint(lrwv,cpu);
}



int Breakpoints::check_cycle_break(unsigned int bpn)
{

  cout << "cycle break: 0x" << hex << get_cycles().get()
       << dec << " = " << get_cycles().get() << endl;

  halt();
  if( bpn < MAX_BREAKPOINTS)
    {
      if (break_status[bpn].bpo)
	  break_status[bpn].bpo->callback();

      //trace.breakpoint( (Breakpoints::BREAK_ON_CYCLE>>8) );
      //trace.raw(m_brt->type() | bpn);

      clear(bpn);
    }

  return(1);

}

bool Breakpoints::dump(TriggerObject *pTO)
{
  if (!pTO)
    return false;

  pTO->print();
  return true;
}
bool Breakpoints::dump1(unsigned int bp_num, int dump_type)
{
  if(!bIsValid(bp_num)) {
    printf("Break point number: %u is out of range\n", bp_num);

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

  //cout << "clearing bp:"<<dec<<b<<endl;
  if(bs.bpo) {
    bs.bpo->clear();
    bs.type = BREAK_CLEAR;
    get_active_cpu()->NotifyBreakpointCleared(bs, bs.bpo);
    delete bs.bpo;
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
  GetTraceLog().close_logfile();
  for(int i=0; i<MAX_BREAKPOINTS; i++)
    if(break_status[i].type != BREAK_CLEAR
       && break_status[i].cpu == c)
      clear(i);
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

void Breakpoints::clear_all_register(Processor *c, gint64 address)
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

void Breakpoints::halt()
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
Breakpoints::Breakpoints()
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

void Breakpoint_Instruction::execute()
{

  if( (cpu->simulation_mode == eSM_RUNNING) &&
      (simulation_start_cycle != get_cycles().get()) &&
      eval_Expression()) {

    invokeAction();

  } else
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
Breakpoint_Instruction::~Breakpoint_Instruction()
{
}

Processor* Breakpoint_Instruction::get_cpu()
{
  return dynamic_cast<Processor *>(cpu);
}
//-------------------------------------------------------------------

bool Breakpoint_Instruction::set_break()
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

void Breakpoint_Instruction::print()
{
  // Output example
  // 42: p17c756  Execution at 0x0123
  instruction *pReplaced = getReplaced();
  gpsimObject *pLineSym = pReplaced ? pReplaced->getLineSymbol() : 0;
  const char *pLabel = pLineSym ? pLineSym->name().c_str() : "no label";
  const char * pFormat = *pLabel == 0 ? "%d: %s %s at %s0x%x\n"
    : "%d: %s %s at %s(0x%x)\n";
  GetUserInterface().DisplayMessage(pFormat,
                                    bpn, cpu->name().c_str(), bpName(), pLabel, address);

  TriggerObject::print();
}

int Breakpoint_Instruction::printTraced(Trace *pTrace, unsigned int tbi,
					char *pBuf, int szBuf)

{
  if (!pBuf || !pTrace)
    return 0;

  int m;

  if (bHasExpression()) {
    char buf[256];
    printExpression(buf, sizeof(buf));
    m = snprintf(pBuf, szBuf,
		 " assertion at 0x%04x, expr:%s",address,buf);
  } else
    m = snprintf(pBuf, szBuf,
		 " execution at 0x%04x",address);

  return m>0 ? m : 0;
}

void Breakpoint_Instruction::clear()
{
  if(get_use_icd())
    icd_clear_break();

  get_cpu()->pma->clear_break_at_address(address, this);
  instruction *pInst = get_cpu()->pma->getFromAddress(address);
  /*
  cout << "about to update cleared instruction breakpoint:" << pInst
       << " this:" << this
       << "  address: 0x" << hex << address
       << endl;
  */
  pInst->update();
}

//------------------------------------------------------------------------
void Notify_Instruction::execute()
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
Notify_Instruction::~Notify_Instruction()
{
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
RegisterAssertion::~RegisterAssertion()
{
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
void RegisterAssertion::execute()
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
  unsigned int curRegValue = PCPU->rma[regAddress].get_value();
  if( m_pfnIsAssertionBreak(curRegValue, regMask, regValue) &&
      (PCPU->pc->get_phase() == 0) )
  {

    cout  << "Caught Register "
	  << (bPostAssertion ? "post " : "")
	  << "assertion "
	  << "while excuting at address 0x" << hex << address << endl;

    cout  << "register 0x"
          << hex
          << regAddress
          << " = 0x"
          << curRegValue << endl;

    cout  << "0x" << PCPU->rma[regAddress].get_value()
          << " & 0x" << regMask
          << " != 0x" << regValue << endl;

    cout  << " regAddress =0x" << regAddress
          << " regMask = 0x" << regMask
          << " regValue = 0x" << regValue << endl;

    PCPU->Debug();

    if( (PCPU->simulation_mode == eSM_RUNNING) &&
        (simulation_start_cycle != get_cycles().get())) {

      eval_Expression();
      invokeAction();
      trace.raw(m_brt->type(1) | curRegValue );

      return;
    }
  }

  // If this is not a post assertion, then the instruction executes after
  // the instruction simulates.

  if(!bPostAssertion && m_replaced)
    m_replaced->execute();

}

//------------------------------------------------------------------------------
void RegisterAssertion::print()
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
int RegisterAssertion::printTraced(Trace *pTrace, unsigned int tbi,
				   char *pBuf, int szBuf)
{
  if (!pBuf || pTrace)
    return 0;

  unsigned int valueWritten = pTrace->get(tbi+1) & 0xffff;

  int m = snprintf(pBuf, szBuf,
		   " Register Assertion PC=0x%04x, reg[0x%x]==0x%x != 0x%x",
		   address,regAddress,valueWritten,regValue);

  return m>0 ? m : 0;

}

//------------------------------------------------------------------------------
#if 0
class BreakpointRegisterAction : public TriggerAction
{
public:
  BreakpointRegisterAction(BreakpointRegister *pbpr);
  virtual ~BreakpointRegisterAction();
  virtual void action();
private:
  BreakpointRegister *m_pbpr;
};

BreakpointRegisterAction::BreakpointRegisterAction(BreakpointRegister *pbpr)
  : m_pbpr(pbpr)
{
}
BreakpointRegisterAction:: ~BreakpointRegisterAction()
{
}
void BreakpointRegisterAction::action()
{
  if(m_pbpr)
    m_pbpr->takeAction();
}
#endif
//------------------------------------------------------------------------------
/*
BreakpointRegister::BreakpointRegister()
  : TriggerObject(new BreakpointRegisterAction(this))
{
}
*/
BreakpointRegister::~BreakpointRegister()
{
}

BreakpointRegister::BreakpointRegister(Processor *_cpu,
				       TriggerAction *pTA,
				       Register *pRepl)
  : Register(_cpu, "",0), TriggerObject(pTA)
{
  setReplaced(pRepl);
}

BreakpointRegister::BreakpointRegister(Processor *_cpu, TriggerAction *ta,
                                       int _repl, int bp)
  : Register(_cpu, "",0), TriggerObject(ta)
{

  bpn = bp;
  replace(_cpu,_repl);
  address = _repl;
}

void BreakpointRegister::invokeAction()
{
  if(eval_Expression())
    TriggerObject::invokeAction();
}

void BreakpointRegister::takeAction()
{
}

void BreakpointRegister::replace(Processor *_cpu, unsigned int reg)
{
  if (_cpu) {
    cpu = _cpu;
    _cpu->rma.insertRegister(reg,this);
  }
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
  // FIXME, we don't know if this breakpoint register is actually associated
  // with the active cpu or not. It looks like we need a way for either the
  // registers to know in which array they're stored OR the Module class needs
  // to provide a 'removeRegister()' method.
  if (get_cpu()) {
    get_cpu()->rma.removeRegister(address,this);
    get_cpu()->registers[address]->update();
  }
}

bool BreakpointRegister::set_break()
{
  return true;
}

void BreakpointRegister::print()
{
  // FIXME - broke with new symbol table.
  Register * pReg = 0; // get_symbol_table().findRegister(address);
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
int BreakpointRegister::printTraced(Trace *pTrace, unsigned int tbi,
				    char *pBuf, int szBuf)
{
  if (!pBuf || pTrace)
    return 0;

  int m = snprintf(pBuf, szBuf,
		   " Breakpoint register ");
  return m>0 ? m : 0;

}

string &BreakpointRegister::name() const
{
  return m_replaced ? m_replaced->name() : gpsimObject::name();
};

void BreakpointRegister::put_value(unsigned int new_value)
{
  getReplaced()->put_value(new_value);
}
void BreakpointRegister::put(unsigned int new_value)
{
  getReplaced()->put(new_value);
}

void BreakpointRegister::putRV(RegisterValue rv)
{
  getReplaced()->putRV(rv);
}

unsigned int BreakpointRegister::get_value()
{
  return(getReplaced()->get_value());
}
RegisterValue BreakpointRegister::getRV()
{
  return getReplaced()->getRV();
}
RegisterValue BreakpointRegister::getRVN()
{
  return getReplaced()->getRVN();
}
unsigned int BreakpointRegister::get()
{
  return(getReplaced()->get());
}

Register *BreakpointRegister::getReg()
{
  return getReplaced() ? getReplaced()->getReg() : this;
}

void BreakpointRegister::setbit(unsigned int bit_number, bool new_value)
{
  getReplaced()->setbit(bit_number, new_value);
}

bool BreakpointRegister::get_bit(unsigned int bit_number)
{
  return(getReplaced()->get_bit(bit_number));
}

double BreakpointRegister::get_bit_voltage(unsigned int bit_number)
{
  return(getReplaced()->get_bit_voltage(bit_number));
}

bool BreakpointRegister::hasBreak()
{
  return true;
}

void BreakpointRegister::update()
{
  if(getReplaced())
    getReplaced()->update();
}

void BreakpointRegister::add_xref(void *an_xref)
{
  if(getReplaced())
    getReplaced()->add_xref(an_xref);
}
void BreakpointRegister::remove_xref(void *an_xref)
{
  if(getReplaced())
    getReplaced()->remove_xref(an_xref);
}


//------------------------------------------------------------------------

BreakpointRegister_Value::
BreakpointRegister_Value(Processor *_cpu,
                         int _repl,
                         int bp,
                         unsigned int bv,
                         unsigned int _operator,
                         unsigned int bm )
  :  BreakpointRegister(_cpu,0,_repl,bp )
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
BreakpointRegister_Value::~BreakpointRegister_Value()
{
//  cout << __FUNCTION__ << " destructor\n";
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

/*
void BreakpointRegister_Value::invokeAction()
{
  if(eval_Expression())
    TriggerObject::invokeAction();
}
*/
/// BreakpointRegister_Value::print() - base class function
/// would be unusual to not be over ridden.
void BreakpointRegister_Value::print()
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
int BreakpointRegister_Value::printTraced(Trace *pTrace, unsigned int tbi,
					  char *pBuf, int szBuf)

{
  if (pBuf && pTrace) {
    unsigned int valueRead = pTrace->get(tbi+1) & 0xffff;
    int m = snprintf(pBuf,szBuf," read 0x%x from reg 0x%x", valueRead, address);
    return m>0 ? m : 0;
  }

  return 0;
}


//-------------------------------------------------------------------
//
Break_register_write_value::Break_register_write_value(Processor *_cpu,
                              int _repl,
                              int bp,
                              unsigned int bv,
                              unsigned int _operator,
                              unsigned int bm ) :
    BreakpointRegister_Value(_cpu, _repl, bp, bv, _operator, bm )
{
}

Break_register_write_value::~Break_register_write_value()
{
}


void Break_register_write_value::takeAction()
{
  trace.raw(m_brt->type(1) | (getReplaced()->get_value() & 0xffffff));

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
//========================================================================
//
Break_register_read::Break_register_read(Processor *_cpu, int _repl, int bp ):
  BreakpointRegister(_cpu,0,_repl,bp )
{
}

Break_register_read::~Break_register_read()
{
}

void Break_register_read::takeAction()
{
  trace.raw(m_brt->type(1) | (getReplaced()->get_value() & 0xffffff));
  if(verbosity && verbosity->getVal()) {

    GetUserInterface().DisplayMessage(IDS_HIT_BREAK,bpn);

    string sFormattedRegAddress;
    sFormattedRegAddress = GetUserInterface().FormatRegisterAddress(getReg());
    GetUserInterface().DisplayMessage(IDS_BREAK_READING_REG,
				      sFormattedRegAddress.c_str());
  }
  bp.halt();
}

unsigned int Break_register_read::get()
{
  unsigned int v = getReplaced()->get();
  invokeAction();
  return v;
}

RegisterValue  Break_register_read::getRV()
{
  RegisterValue rv = getReplaced()->getRV();
  invokeAction();
  return rv;
}

RegisterValue  Break_register_read::getRVN()
{
  RegisterValue rv = getReplaced()->getRVN();
  invokeAction();
  return rv;
}

bool Break_register_read::get_bit(unsigned int bit_number)
{
  invokeAction();
  return(getReplaced()->get_bit(bit_number));
}

double Break_register_read::get_bit_voltage(unsigned int bit_number)
{
  return getReplaced()->get_bit_voltage(bit_number);
}

int Break_register_read::printTraced(Trace *pTrace, unsigned int tbi,
				     char *pBuf, int szBuf)

{
  if (pBuf && pTrace) {
    unsigned int valueRead = pTrace->get(tbi+1) & 0xffff;
    int m = snprintf(pBuf,szBuf," read 0x%x from reg 0x%x", valueRead, address);
    return m>0 ? m : 0;
  }
  return 0;
}

//========================================================================
//
Break_register_write::Break_register_write(Processor *_cpu, int _repl, int bp ):
  BreakpointRegister(_cpu,0,_repl,bp )
{
}


Break_register_write::~Break_register_write()
{
}
void Break_register_write::takeAction()
{

  trace.raw(m_brt->type(1) | (getReplaced()->get_value() & 0xffffff));
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

void Break_register_write::put(unsigned int new_value)
{
  getReplaced()->put(new_value);
  invokeAction();
}

void Break_register_write::putRV(RegisterValue rv)
{
  getReplaced()->putRV(rv);
  invokeAction();
}

void Break_register_write::setbit(unsigned int bit_number, bool new_value)
{
  getReplaced()->setbit(bit_number,new_value);
  invokeAction();
}

int Break_register_write::printTraced(Trace *pTrace, unsigned int tbi,
				      char *pBuf, int szBuf)

{
  if (pBuf && pTrace) {
    unsigned int valueRead = pTrace->get(tbi+1) & 0xffff;
    int m = snprintf(pBuf,szBuf," wrote 0x%x to reg 0x%x", valueRead, address);
    return m>0 ? m : 0;
  }
  return 0;
}
//========================================================================
Break_register_read_value::Break_register_read_value(Processor *_cpu,
                                                     int _repl,
                                                     int bp,
                                                     unsigned int bv,
                                                     unsigned int _operator,
                                                     unsigned int bm ) :
  BreakpointRegister_Value(_cpu, _repl, bp, bv, _operator, bm )
{
}

Break_register_read_value::~Break_register_read_value()
{
}

void Break_register_read_value::takeAction()
{
  trace.raw(m_brt->type(1) | (getReplaced()->get_value() & 0xffffff));

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

unsigned int Break_register_read_value::get()
{
  unsigned int v = getReplaced()->get();

  if(m_pfnIsBreak(v, break_mask, break_value))
    invokeAction();
  return v;
}

RegisterValue  Break_register_read_value::getRV()
{
  RegisterValue v = getReplaced()->getRV();

  if(m_pfnIsBreak(v.data, break_mask, break_value))
    invokeAction();
  return(v);
}

RegisterValue  Break_register_read_value::getRVN()
{
  RegisterValue v = getReplaced()->getRVN();

  if(m_pfnIsBreak(v.data, break_mask, break_value))
    invokeAction();
  return(v);
}

bool Break_register_read_value::get_bit(unsigned int bit_number)
{
  unsigned int v = getReplaced()->get();
  unsigned int mask = 1<<(bit_number & 7);

  if( (break_mask & mask) && (v & mask) == (break_value&mask))
    invokeAction();
  return getReplaced()->get_bit(bit_number);
}

double Break_register_read_value::get_bit_voltage(unsigned int bit_number)
{
  return getReplaced()->get_bit_voltage(bit_number);
}

int Break_register_read_value::printTraced(Trace *pTrace, unsigned int tbi,
					   char *pBuf, int szBuf)

{
  if (pBuf && pTrace) {
    unsigned int valueRead = pTrace->get(tbi+1) & 0xffff;
    int m = snprintf(pBuf,szBuf," read 0x%x from reg 0x%x", valueRead, address);
    return m>0 ? m : 0;
  }
  return 0;
}
//========================================================================


void Break_register_write_value::put(unsigned int new_value)
{
  getReplaced()->put(new_value);
  if(m_pfnIsBreak(new_value, break_mask, break_value))
    invokeAction();
}

void Break_register_write_value::putRV(RegisterValue rv)
{
  getReplaced()->putRV(rv);
  if(m_pfnIsBreak(rv.data, break_mask, break_value))
    invokeAction();
}


void Break_register_write_value::setbit(unsigned int bit_number, bool new_bit)
{
  int val_mask = 1 << bit_number;
  int new_value = ((int)new_bit) << bit_number;

  getReplaced()->setbit(bit_number,new_value ? true  : false);

  if( (val_mask & break_mask) &&
      ( ( (getReplaced()->value.get() & ~val_mask)  // clear the old bit
          | new_value)                   // set the new bit
        & break_mask) == break_value)
    invokeAction();

}
int Break_register_write_value::printTraced(Trace *pTrace, unsigned int tbi,
					   char *pBuf, int szBuf)

{
  if (pBuf && pTrace) {
    unsigned int valueRead = pTrace->get(tbi+1) & 0xffff;
    int m = snprintf(pBuf,szBuf," wrote 0x%x to reg 0x%x", valueRead, address);
    return m>0 ? m : 0;
  }

  return 0;
}


//========================================================================
//
Break_register_change::Break_register_change(Processor *_cpu, int _repl, int bp ):
  BreakpointRegister(_cpu,0,_repl,bp )
{
}

Break_register_change::~Break_register_change()
{
}
void Break_register_change::takeAction()
{

  trace.raw(m_brt->type(1) | (getReplaced()->get_value() & 0xffffff));
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

void Break_register_change::put(unsigned int new_value)
{
  unsigned int before = getReplaced()->get_value();
  getReplaced()->put(new_value);
  if (before != getReplaced()->get_value())
    invokeAction();
}

void Break_register_change::putRV(RegisterValue rv)
{
  RegisterValue before = getReplaced()->getRV_notrace();
  getReplaced()->putRV(rv);
  if (before != getReplaced()->getRV_notrace())
    invokeAction();
}

void Break_register_change::setbit(unsigned int bit_number, bool new_value)
{
  bool before = getReplaced()->get_bit(bit_number);
  getReplaced()->setbit(bit_number,new_value);
  if (before != getReplaced()->get_bit(bit_number))
    invokeAction();
}

int Break_register_change::printTraced(Trace *pTrace, unsigned int tbi,
				      char *pBuf, int szBuf)

{
  if (pBuf && pTrace) {
    unsigned int valueRead = pTrace->get(tbi+1) & 0xffff;
    int m = snprintf(pBuf,szBuf," wrote 0x%x to reg 0x%x", valueRead, address);
    return m>0 ? m : 0;
  }
  return 0;
}
//========================================================================
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

CommandAssertion::~CommandAssertion()
{
  free(command);
}

void CommandAssertion::execute()
{
  if(bPostAssertion && getReplaced())
    getReplaced()->execute();

  //printf("execute command: %s -- post = %s\n",command,(bPostAssertion?"true":"false"));

  ICommandHandler *pCli = CCommandManager::GetManager().find("gpsimCLI");
  if(pCli) {
    pCli->Execute(command, 0);
  }
  if(!bPostAssertion && getReplaced())
    getReplaced()->execute();
}

//------------------------------------------------------------------------------
void CommandAssertion::print()
{
  Breakpoint_Instruction::print();
  cout << "  execute command " << command << endl;
}
int CommandAssertion::printTraced(Trace *pTrace, unsigned int tbi,
				  char *pBuf, int szBuf)
{
  return 0;
}


//============================================================================

void Log_Register_Write::put(unsigned int new_value)
{
  getReplaced()->put(new_value);
  takeAction();
}
void Log_Register_Write::setbit(unsigned int bit_number, bool new_value)
{
  getReplaced()->setbit(bit_number,new_value);
  takeAction();
}
void Log_Register_Write::takeAction()
{
  GetTraceLog().register_write(getReg(),
			       get_cycles().get());
}
void Log_Register_Write_value::takeAction()
{
  GetTraceLog().register_write_value(getReg(),
				     get_cycles().get());
}

unsigned int Log_Register_Read::get()
{
  unsigned int v = getReplaced()->get();
  takeAction();
  return v;
}
RegisterValue  Log_Register_Read::getRV()
{
  RegisterValue rv = getReplaced()->getRV();
  takeAction();
  return rv;
}

RegisterValue  Log_Register_Read::getRVN()
{
  RegisterValue rv = getReplaced()->getRVN();
  takeAction();
  return rv;
}

bool Log_Register_Read::get_bit(unsigned int bit_number)
{
  takeAction();
  return(getReplaced()->get_bit(bit_number));
}

void Log_Register_Read::takeAction()
{
  GetTraceLog().register_read(getReg(),
			      get_cycles().get());
}


void Log_Register_Read_value::takeAction()
{
  GetTraceLog().register_read(getReg(),
			      get_cycles().get());
}
