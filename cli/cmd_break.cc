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
#include <sstream>
#include <assert.h>

#include "command.h"
#include "cmd_break.h"

#include "../src/cmd_gpsim.h"
#include "../src/pic-processor.h"
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


cmd_break::cmd_break()
  : command("break", "br")
{ 
  brief_doc = string("Set a break point");

  long_doc = string ("The 'break' command can be used to examine or set breakpoints.\n"
                     "gpsim supports execution style breaks, register access breaks,\n"
                     "complex expression breaks, attribute breaks, and other special breaks.\n"
                     "Program Memory breaks:\n"
                     "  break e|r|w ADDRESS [,expr] [,\"message\"]\n"
                     "    Halts when the address is executed, read, or written. The ADDRESS can be \n"
                     "    a symbol or a number. If the optional expr is specified, then it must\n"
                     "    evaluate to true before the simulation will halt. The optional message\n"
		     "    allows a description to be associated with the break."
                     "Register Memory breaks:\n"
                     "  break r|w REGISTER [,expr] [,\"message\"]\n"
                     "    Halts when 'REGISTER' is read or written and the optional expression\n"
                     "    evaluates to true.\n"
                     "  break r|w boolean_expression\n"
                     "    The boolean expression can only be of the form:\n"
		     "       a) reg & mask == value\n"
		     "       b) reg == value\n"
                     "Cycle counter breaks:\n"
                     "  break c VALUE  [,\"message\"]\n"
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
                     "\tbreak w reg3, (reg4 > 45)   # break if reg4>45 while writing to reg3\n"
                     "\tbreak c 1000000    # break on the one million'th cycle\n"
                     );

  op = cmd_break_options; 
}


void cmd_break::list(guint64 value)
{
  if(value == CMDBREAK_BAD_BREAK_NUMBER)
    get_bp().dump();
  else
    get_bp().dump1((unsigned int)value);
}

const char *TOO_FEW_ARGS="missing register or location\n";
const char *TOO_MANY_ARGS="too many arguments\n";


//------------------------------------------------------------------------
static gpsimObject::ObjectBreakTypes MapBreakActions(int co_value)
{
  switch(co_value) {
  case READ:
    return gpsimObject::eBreakRead;
  case WRITE:
    return gpsimObject::eBreakWrite;
  case EXECUTION:
    return gpsimObject::eBreakExecute;
  }
  return gpsimObject::eBreakAny;
}

//------------------------------------------------------------------------
unsigned int cmd_break::set_break(cmd_options *co, ExprList_t *pEL, bool bLog)
{
  if (!co) {
    list();
    return MAX_BREAKPOINTS;
  }

  if (!pEL || pEL->size()>3) {
    // FIXME - fix this error message
    cout << "ERROR: Bad expression for break command\n";
    return MAX_BREAKPOINTS;
  }


  ExprList_itor ei = pEL->begin();
  Expression *pFirst = *ei;
  ++ei;
  Expression *pSecond;
  if(ei != pEL->end()) {
    pSecond = *ei;
    ++ei;
  }
  else {
    pSecond = NULL;
  }
  Expression *pThird = (ei != pEL->end()) ? *ei : 0;

  LiteralString *pString=0;
  string m;
  if (pSecond) {
    pString = dynamic_cast<LiteralString*>(pSecond);
    if (pString) {
      String *pS = (String *)pString->evaluate();
      m = string(pS->getVal());
      delete pSecond;
      delete pS;
      pSecond =0;
    }
  }

  // If there is a third expression and the second expression is not
  // a string, then try to cast the third expression into a string.
  if (pThird && !pString) {
    pString = dynamic_cast<LiteralString*>(pThird);
    if (pString) {
      String *pS = (String *)pString->evaluate();
      m = string(pS->getVal());
      delete pThird;
      delete pS;
      pThird =0;
    }
  }

  if (!pFirst)
    return set_break(co->value,bLog);

  // See if the expression supports break points. If it does, the break points
  // will get set and the expressions deleted.
  int bpn = pFirst ? pFirst->set_break(MapBreakActions(co->value), 
				       (bLog ? gpsimObject::eActionLog : gpsimObject::eActionHalt),
				       pSecond) : -1;
  if (bpn == -1 && co->value!=CYCLE) {
    GetUserInterface().DisplayMessage("break cannot be set on %s\n",
      pFirst->toString().c_str());
  }
  if (bpn<0) {

    // We failed to set a break point from the first expression. 
    // It may be that we have a type of break point that is not supported
    // by the expression code.

    if (co->value==CYCLE) {
      LiteralInteger *pLitInt = dynamic_cast<LiteralInteger*>(pFirst);
      Integer *pInt = pLitInt ? dynamic_cast<Integer*>(pLitInt->evaluate()) : 0;
      guint64 ui64Val =  pInt ? (guint64)pInt->getVal() : 0;
      if (pInt)
        bpn = get_bp().set_cycle_break(GetActiveCPU(),ui64Val);
      delete pInt;
    }
  }

  if (bpn>=0) {
    if (pString)
      get_bp().set_message(bpn, m);
    get_bp().dump1(bpn);
  } else {

    delete pFirst;
    if(pSecond != 0)
      delete pSecond;
  }

  return bpn;

}

//------------------------------------------------------------------------
// set_break(cmd_options *co, 
//           Expression *pExpr1,
//           Expression *pExpr2)
//
// Given two expressions, this function will call the set

unsigned int cmd_break::set_break(cmd_options *co, 
				  Expression *pExpr1,
				  Expression *pExpr2,
				  bool bLog)

{

  if (!co) {
    list();
    return MAX_BREAKPOINTS;
  }

  unsigned int bit_flag = co->value;
  if (!pExpr1)
    return set_break(bit_flag,bLog);

  // See if the expression supports break points. If it does, the break points
  // will get set and the expressions deleted.
  int i = pExpr1 ? pExpr1->set_break(MapBreakActions(co->value), 
				     (bLog ? gpsimObject::eActionLog : gpsimObject::eActionHalt), pExpr2) : -1;
  if (i>=0) {
    get_bp().dump1(i);
    return i;
  }

  unsigned int b = MAX_BREAKPOINTS;
  delete pExpr1;
  delete pExpr2;

  return b;

}


unsigned int cmd_break::set_break(cmd_options *co, bool bLog)
{

  if (!co) {
    list();
    return MAX_BREAKPOINTS;
  }

  int bit_flag = co->value;
  return set_break(bit_flag, bLog);
}

unsigned int cmd_break::set_break(int bit_flag, bool bLog)
{
  unsigned int b = MAX_BREAKPOINTS;

  if(!GetActiveCPU())
    return b;

  switch(bit_flag) {

  case STK_OVERFLOW:
    b = get_bp().set_stk_overflow_break(GetActiveCPU());

    if(b < MAX_BREAKPOINTS)
      cout << "break when stack over flows.  " <<
        "bp#: " << b << '\n';

    break;

  case STK_UNDERFLOW:
    b = get_bp().set_stk_underflow_break(GetActiveCPU());

    if(b < MAX_BREAKPOINTS)
      cout << "break when stack under flows.  " <<
        "bp#: " << b << '\n';

    break;


  case WDT:
    b = get_bp().set_wdt_break(GetActiveCPU());

    if(b < MAX_BREAKPOINTS)
      cout << "break when wdt times out.  " <<
        "bp#: " << b << '\n';

    break;

  case CYCLE:
    get_bp().dump(Breakpoints::BREAK_ON_CYCLE);
    break;
  case EXECUTION:
    get_bp().dump(Breakpoints::BREAK_ON_EXECUTION);
    break;
  case WRITE:
    get_bp().dump(Breakpoints::BREAK_ON_REG_WRITE);
    break;
  case READ:
    get_bp().dump(Breakpoints::BREAK_ON_REG_READ);
    break;
  default:
    cout << TOO_FEW_ARGS;
    break;
  }

  return b;
}
 // attribute breakpoints 	 
unsigned int cmd_break::set_break(gpsimObject *v)
{
  if (v)
    v->set_break();

  return MAX_BREAKPOINTS;
}
