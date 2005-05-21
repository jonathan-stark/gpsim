/*
   Copyright (C) 1999 T. Scott Dattalo

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
#include <string>

#include "command.h"
#include "cmd_break.h"

#include "../src/pic-processor.h"
#include "../src/symbol_orb.h"
#include "../src/operator.h"

cmd_break c_break;

#define CYCLE       1
#define EXECUTION   2
#define WRITE       3
#define READ        4
#define STK_OVERFLOW  7
#define STK_UNDERFLOW 8
#define WDT           9

static cmd_options cmd_break_options[] =
{
  {"c",   CYCLE,        OPT_TT_BITFLAG},
  {"e",   EXECUTION,    OPT_TT_BITFLAG},
  {"w",   WRITE,        OPT_TT_BITFLAG},
  {"r",   READ,         OPT_TT_BITFLAG},
  {"so",  STK_OVERFLOW, OPT_TT_BITFLAG},
  {"su",  STK_UNDERFLOW,OPT_TT_BITFLAG},
  {"wdt", WDT,          OPT_TT_BITFLAG},
  {0,0,0}
};


cmd_break::cmd_break(void)
{ 
  name = "break";

  brief_doc = string("Set a break point");

  long_doc = string ("The 'break' command can be used to examine or set breakpoints.\n"
                     "gpsim supports execution style breaks, register access breaks,\n"
                     "complex expression breaks, attribute breaks, and other special breaks.\n"
                     "Program Memory breaks:\n"
                     "  break e|r|w ADDRESS [expr]\n"
                     "    Halts when the address is executed, read, or written. The ADDRESS can be \n"
                     "    a symbol or a number. If the optional expr is specified, then it must\n"
                     "    evaluate to true before the simulation will halt.\n"
                     "Register Memory breaks:\n"
                     "  break r|w REGISTER [expr]\n"
                     "    Halts when 'REGISTER' is read or written and the optional expression\n"
                     "    evaluates to true.\n"
                     "  break r|w boolean_expression\n"
                     "    older style to be deprecated..."
                     "Cycle counter breaks:"
                     "  break c VALUE\n"
                     "    Halts when the cycle counter reaches 'VALUE'.\n"
                     "Attribute breaks:\n"
                     "  break attribute\n"
                     "    Arms the breakpoint condition for those attributes that support breaks.\n"
                     "    For example, the stopwatch (help stopwatch) attribute can cause a break.\n"
                     "Miscellaneous breaks:\n"
                     "  break so   # halts on stack overflow.\n"
                     "  break su   # halts on stack underflow.\n"
                     "  break wdt  # halts on Watch Dog Timer timeout.\n"
                     "Expressions:\n"
                     "  The conditional expressions mentioned above are syntactically similar to C's\n"
                     "  expressions.\n"
                     "Examples:\n"
                     "\tbreak              # display all of the break points\n"
                     "\tbreak e 0x20       # set an execution break point at address 0x20\n"
                     "\tbreak w reg1 == 0  # break if a zero is written to register reg1\n"
                     "\tbreak w reg2 & 0x30 == 0xf0 # break if '3' is written to the\n"
                     "\t                            # upper nibble or reg2\n"
                     "\tbreak w reg3 (reg4 > 45)    # break if reg4>45 while writing to reg3\n"
                     "\tbreak c 1000000    # break on the one million'th cycle\n"
                     );

  op = cmd_break_options; 
}


void cmd_break::list(guint64 value)
{
  if(value == CMDBREAK_BAD_BREAK_NUMBER)
    bp.dump();
  else
    bp.dump1(value);
}

const char *TOO_FEW_ARGS="missing register or location\n";
const char *TOO_MANY_ARGS="too many arguments\n";

//------------------------------------------------------------------------
// Certain break options are incompatible with certain symbol types.
// E.g. it doesn't make sense to associate the 'execute' option with
// a register. 

static bool bCheckOptionCompatibility(cmd_options *co, Value *pValue)
{
  if(co && pValue) {

    if(co->value==READ || co->value==WRITE || co->value==EXECUTION) {
      Integer * pAddress = dynamic_cast<Integer*>(pValue);
      if (pAddress)
	return true;
    }

    if(co->value==READ || co->value==WRITE) {
      register_symbol* pRegSymbol = dynamic_cast<register_symbol*>(pValue);
      if (pRegSymbol)
	return true;
    }

    if(co->value==CYCLE)
      return true;

    printf("Syntax error:  %s is incompatible with the '%s' break option\n",
	   pValue->name().c_str(), co->name);

  }

  return false;
}
//------------------------------------------------------------------------
// set_break(cmd_options *co, Value *pValue, Expression *pExpr)
// 
// supports the following breaks:
//
//   break e|r|w ADDRESS_SYMBOL expression
//   break r|w REGISTER_SYMBOL expression
//
// Example:
//
// Halt execution if the interrupt routine writes to the register 'temp1':
//
//  break w temp1  PC>=InterruptStart && PC<InterruptEnd

void cmd_break::set_break(cmd_options *co, Value *pValue, Expression *pExpr)
{
  if (!bCheckOptionCompatibility(co, pValue) || !GetActiveCPU())
    return;

  // 
  int b;

  Integer * pAddress = dynamic_cast<Integer*>(pValue);
  if (pAddress != NULL) { 
    gint64 iAddress;
    pAddress->get(iAddress);
    b = bp.set_execution_break(GetActiveCPU(),iAddress);
    if (!bp.set_expression(b,pExpr))
      delete pExpr;
    return;
  }

  register_symbol* pRegSymbol = dynamic_cast<register_symbol*>(pValue);
  if (pRegSymbol) {
    b = set_break(co->value, pRegSymbol->getReg()->address);
    
    if (!bp.set_expression(b,pExpr))
      delete pExpr;
    return;
  }

}
//------------------------------------------------------------------------
//  set_break(cmd_options *co, Value *pValue)
//
//  Supports the following breaks:
//
//   break e|r|w ADDRESS_SYMBOL
//   break r|w REGISTER_SYMBOL
//
void cmd_break::set_break(cmd_options *co, Value *pValue)
{
  if (!bCheckOptionCompatibility(co, pValue))
    return;

  Integer * pAddress = dynamic_cast<Integer*>(pValue);
  if (pAddress != NULL) { 
    gint64 iAddress;
    pAddress->get(iAddress);
    set_break(co->value, iAddress);
    return;
  }

  register_symbol* pRegSymbol = dynamic_cast<register_symbol*>(pValue);
  if (pRegSymbol) {
    set_break(co->value, pRegSymbol->getReg()->address);
    return;
  }
}
//------------------------------------------------------------------------
void cmd_break::set_break(cmd_options *co, Expression *pExpr)
{

  if (!co) {
    list();
    return;
  }

  int bit_flag = co->value;
  if (!pExpr) {
    set_break(bit_flag);
    return;
  }
  ComparisonOperator *pCompareExpr = dynamic_cast<ComparisonOperator *>(pExpr);
  if (pCompareExpr != NULL) {
     
    Register * pReg = NULL;
    int  uMask = GetActiveCPU()->register_mask();
    LiteralSymbol* pLeftSymbol = dynamic_cast<LiteralSymbol*>(pCompareExpr->getLeft());
    if (pLeftSymbol != NULL) {
      register_symbol *pRegSym = dynamic_cast<register_symbol*>(pLeftSymbol->evaluate());
      pReg = pRegSym->getReg();
      delete pRegSym;
    }
    else {
      OpAnd* pLeftOp = dynamic_cast<OpAnd*>(pCompareExpr->getLeft());
      if (pLeftOp != NULL) {
        pLeftSymbol = dynamic_cast<LiteralSymbol*>(pLeftOp->getLeft());
        register_symbol *pRegSym = dynamic_cast<register_symbol*>(pLeftSymbol->evaluate());
        pReg = pRegSym->getReg();

        LiteralSymbol* pRightSymbol = dynamic_cast<LiteralSymbol*>(pLeftOp->getRight());
        Integer *pInteger = dynamic_cast<Integer*>(pRightSymbol->evaluate());
        gint64 i64;
        pInteger->get(i64);
        uMask = (int)i64;
        delete pRegSym;
        delete pInteger;
      }
    }
    if (pReg != NULL) {
      LiteralInteger* pInteger = dynamic_cast<LiteralInteger*>((LiteralInteger*)pCompareExpr->getRight());
      if (pInteger != NULL) {
        int uValue;
        Value *pInt = pInteger->evaluate();
        pInt->get(uValue);
        delete pInt;
        set_break(bit_flag, pReg->address, uValue, uMask);
      }
      else {
        cout << pCompareExpr->show() << " of type " << pCompareExpr->showType() <<
          " not allowed\n";
      }
    }
    else {
      cout << pCompareExpr->getLeft()->show() << " of type " << pCompareExpr->getLeft()->showType() <<
        " not allowed\n";
    }
  }
  else {
    cout << pExpr->show() << " of type " << pExpr->showType() <<
      " not allowed\n";
  }
  delete pExpr;
}

void cmd_break::set_break(cmd_options *co)
{

  if (!co) {
    list();
    return;
  }

  int bit_flag = co->value;
  set_break(bit_flag);
}

void cmd_break::set_break(Value *v)
{
  if(v)
    v->set_break();
}

void cmd_break::set_break(int bit_flag)
{

  if(!GetActiveCPU())
    return;

  int b;

  switch(bit_flag) {

  case STK_OVERFLOW:
    b = bp.set_stk_overflow_break(GetActiveCPU());

    if(b < MAX_BREAKPOINTS)
      cout << "break when stack over flows.  " <<
        "bp#: " << b << '\n';

    break;

  case STK_UNDERFLOW:
    b = bp.set_stk_underflow_break(GetActiveCPU());

    if(b < MAX_BREAKPOINTS)
      cout << "break when stack under flows.  " <<
        "bp#: " << b << '\n';

    break;


  case WDT:
    b = bp.set_wdt_break(GetActiveCPU());

    if(b < MAX_BREAKPOINTS)
      cout << "break when wdt times out.  " <<
        "bp#: " << b << '\n';

    break;

  default:
    cout << TOO_FEW_ARGS;
    break;
  }
}


int cmd_break::set_break(int bit_flag, guint64 v, Expression *pExpr)
{

  int b = MAX_BREAKPOINTS;
  if(!GetActiveCPU())
    return b;

  unsigned int value = (unsigned int)v;

  switch(bit_flag) {

  case CYCLE:
    b = bp.set_cycle_break(GetActiveCPU(),value);

    if(b < MAX_BREAKPOINTS)
      cout << "break at cycle: " << value << " break #: " <<  b << '\n';
    else
      cout << "failed to set cycle break\n";

    break;

  case EXECUTION:
    b = bp.set_execution_break(GetActiveCPU(), value);
    if(b < MAX_BREAKPOINTS) {
      cout << "break at address: " << value << " break #: " << b << '\n';
    }
    else
      cout << "failed to set execution break (check the address)\n";

    break;

  case WRITE:

    b = bp.set_write_break(GetActiveCPU(), value);
    if(b < MAX_BREAKPOINTS)
      cout << "break when register " << value << " is written. bp#: " << b << '\n';

    break;

  case READ:
    b = bp.set_read_break(GetActiveCPU(), value);
    if(b < MAX_BREAKPOINTS)
      cout << "break when register " << value << " is read.\n" << 
	      "bp#: " << b << '\n';

    break;

  case STK_OVERFLOW:
  case STK_UNDERFLOW:
  case WDT:
    cout << TOO_MANY_ARGS;
  }

  
  if (pExpr && (b>=MAX_BREAKPOINTS || !bp.set_expression(b,pExpr)))
    delete pExpr;

  return b;
}

void cmd_break::set_break(int bit_flag,
			  guint64 r,
			  guint64 v,
			  guint64 m)
{

  if(!GetActiveCPU())
    return;

  int b = MAX_BREAKPOINTS;
  const char *str = "err";
  unsigned int reg = (unsigned int)r;
  unsigned int value = (unsigned int)v;
  unsigned int mask = (unsigned int)m;

  switch(bit_flag) {

  case CYCLE:
  case EXECUTION:
  case STK_OVERFLOW:
  case STK_UNDERFLOW:
  case WDT:
    cout << TOO_MANY_ARGS;
    break;

  case READ:
    b = bp.set_read_value_break(GetActiveCPU(), reg,value,mask);
    str = "read from";
    break;

  case WRITE:
    b = bp.set_write_value_break(GetActiveCPU(), reg,value,mask);
    str = "written to";
    break;
  }

  if(b<MAX_BREAKPOINTS) {
    cout << "break when ";
    if(mask == 0 || mask == 0xff)
      cout << (value&0xff);
    else {
      cout << "bit pattern ";
      for(unsigned int ui=0x80; ui; ui>>=1) {
        if(ui & mask) {
          if(ui & value)
            cout << '1';
          else
            cout << '0';
        }
        else {
	        cout << 'X';
        }
      }
    }
    cout << " is " << str <<" register " << reg << '\n' << 
	    "bp#: " << b << '\n';
  }
}

