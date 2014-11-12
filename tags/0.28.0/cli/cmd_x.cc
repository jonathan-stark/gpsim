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
#include <typeinfo>

#include "command.h"
#include "cmd_x.h"
#include "cmd_dump.h"

#include "../src/cmd_gpsim.h"
#include "../src/pic-processor.h"
#include "../src/symbol.h"


cmd_x c_x;

static cmd_options cmd_x_options[] =
{
  {0,0,0}
};


cmd_x::cmd_x()
  : command("x",0)
{ 

  brief_doc = string("[deprecated] examine and/or modify memory");

  long_doc = string ("\nx examine command -- deprecated\n\
\tInstead of the using a special command to examine and modify\n\
\tvariables, it's possible to directly access them using gpsim's\n\
\texpression parsing. For example, to examine a variable:\n\
gpsim> my_variable\n\
my_variable [0x27] = 0x00 = 0b00000000\n\
\tTo modify a variable\n\
gpsim> my_variable = 10\n\
\tIt's also possible to assign the value of register to another\n\
gpsim> my_variable = porta\n\
\tOr to assign the results of an expression:\n\
gpsim> my_variable = (porta ^ portc) & 0x0f\n");

  /*
  long_doc = string ("\nx [file_register] [new_value]\n\
\toptions:\n\
\t\tfile_register - ram location to be examined or modified.\n\
\t\tnew_value - the new value written to the file_register.\n\
\t\tif no options are specified, then the entire contents\n\
\t\tof the file registers will be displayed (dump).\n\
");
  */
  op = cmd_x_options; 
}


void cmd_x::x(void)
{
  dump.dump(cmd_dump::DUMP_RAM);
  if(GetActiveCPU())
    GetActiveCPU()->dump_registers();
}

void cmd_x::x(int reg, Expression *pExpr)
{
  if(!GetActiveCPU())
    return;

  if(reg<0 || (reg >= (int)GetActiveCPU()->register_memory_size()) )
    {
      GetUserInterface().DisplayMessage("bad file register\n");
      return;
    }

  Register *pReg = GetActiveCPU()->registers[reg];
  RegisterValue rvCurrent(pReg->getRVN());
  if(pExpr == NULL) {
    char str[33];
    // Display value
    const char * pAddr = GetUserInterface().FormatRegisterAddress(
      reg, GetActiveCPU()->m_uAddrMask);
    const char * pValue = GetUserInterface().FormatValue(
      rvCurrent.data, GetActiveCPU()->register_mask());
    GetUserInterface().DisplayMessage("%s[%s] = %s = 0b%s\n",
      pReg->name().c_str(), pAddr, pValue,
      pReg->toBitStr(str,sizeof(str)));
  }
  else {
    // Assign value
    Value *pValue = pExpr->evaluate();
    if(pValue != NULL) {
      Integer * pInt = dynamic_cast<Integer*>(pValue);
      if(pValue != NULL) {
        char str[33];
        pReg->toBitStr(str,sizeof(str));
        RegisterValue value(
          GetActiveCPU()->register_mask() & (unsigned int)pInt->getVal(), 0);
        pReg->putRV(value);
        // Notify listeners
        pReg->update();
        // Display new value
        x(reg);
        // Display old value
        const char * pValue = GetUserInterface().FormatValue(
          (gint64)rvCurrent.get(), GetActiveCPU()->register_mask());
        GetUserInterface().DisplayMessage("was %s = 0b%s\n",
          pValue, str);
      }
      else {
        GetUserInterface().DisplayMessage(
          "Error: the expression did not evaluate to on integer");
      }
      delete pValue;
    }
    else {
      GetUserInterface().DisplayMessage("Error evaluating the expression");
    }
    delete pExpr;
  }
}

void cmd_x::x(char *reg_name)
{
  cout << "this command is deprecated. "
       << "Type '" << reg_name << "' at the command line to display the contents of a register.\n";
  // get_symbol_table().dump_one(reg_name);
}

void cmd_x::x(char *reg_name, int val)
{
  cout << "this command is deprecated. use: \n  symbol_name = value\n\ninstead\n";
  //  update_symbol_value(reg_name,val);
}

void cmd_x::x(Expression *expr)
{
  /*
  try {

    Value *v = toValue(expr);
    cout << v->toString() << endl;

    if(typeid(register_symbol) == typeid(*v) ||
      (typeid(LiteralSymbol) == typeid(*expr) &&
      !((LiteralSymbol*)expr)->toString().empty() )) {
      // v->toString() dumped the value to cout
    }
    else if(typeid(Integer) == typeid(*v)) {
        int i;
        v->get(i);
        x(i);
    }
    else if(typeid(AbstractRange) == typeid(*v)) {
      unsigned int i = v->get_leftVal();
      while (i<=v->get_rightVal()) {
        x(i);
        i++;
      }
    }
  
    delete v;
    delete expr;
  }


  catch (Error *err) {
    if(err)
      cout << "ERROR:" << err->toString() << endl;
    delete err;
  }
  */

  delete expr;
}
