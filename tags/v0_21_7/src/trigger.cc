/*
   Copyright (C) 2004 T. Scott Dattalo

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


#include "trigger.h"
#include "../config.h"
#include "value.h"
#include "expr.h"
#include "errors.h"
#include "breakpoints.h"

#include <iostream>
#include <iomanip>

extern Integer *verbosity;  // in ../src/init.cc

using namespace std;

static TriggerAction DefaultTrigger;


//------------------------------------------------------------------------
// TriggerAction
//
TriggerAction::TriggerAction()
{
}

bool TriggerAction::evaluate(void)
{
  action();
  return true;
}

bool TriggerAction::getTriggerState(void)
{
  return false;
}

void TriggerAction::action(void)
{
  if(verbosity && verbosity->getVal())
    cout << "Hit a Breakpoint!\n";
  bp.halt();
}



//------------------------------------------------------------------------
// SimpleTriggerAction
//
// For most cases... A single trigger action coupled with a single trigger
// object
SimpleTriggerAction::SimpleTriggerAction(TriggerObject *_to)
  : TriggerAction(), to(_to)
{
}

void SimpleTriggerAction::action(void)
{
  TriggerAction::action();
  if(to && verbosity && verbosity->getVal())
    to->print();
}




//------------------------------------------------------------------------
TriggerObject::TriggerObject()
{
  m_PExpr = 0;
  set_action(&DefaultTrigger);
}

TriggerObject::TriggerObject(TriggerAction *ta)
{
  m_PExpr = 0;

  if(ta)
    set_action(ta);
  else
    set_action(&DefaultTrigger);
}

void TriggerObject::callback(void)
{
  cout << "generic callback\n";
}

void TriggerObject::callback_print(void)
{
  cout << " has callback, ID =  " << CallBackID << '\n';
}

int TriggerObject::find_free(void)
{
  bpn = bp.find_free();

  if(bpn < MAX_BREAKPOINTS) {

    bp.break_status[bpn].type = Breakpoints::BREAK_CLEAR;
    bp.break_status[bpn].cpu  = 0; //get_cpu();
    bp.break_status[bpn].arg1 = 0;
    bp.break_status[bpn].arg2 = 0;
    bp.break_status[bpn].bpo  = this;
  }

  return bpn;

}

void TriggerObject::print(void)
{
  cout << "Generic breakpoint " << bpn << endl;
}
void TriggerObject::printExpression()
{
  if (m_PExpr)
    cout << m_PExpr->toString() << endl;
}

void TriggerObject::clear(void)
{
  cout << "clear Generic breakpoint " << bpn << endl;
}

void TriggerObject::set_Expression(Expression *newExpression)
{
  delete m_PExpr;
  m_PExpr = newExpression;
}

bool TriggerObject::eval_Expression()
{
  if(m_PExpr) {
    bool bRet = true;

    try {
      Value *v = m_PExpr->evaluate();

      if(v)
        v->get(bRet);
    }
    catch (Error *Perr) {
      if(Perr)
        cout << "ERROR:" << Perr->toString() << endl;
      delete Perr;
    }

    return bRet;
  }

  return false;
}

//-------------------------------------------------------------------
void TriggerObject::new_message(char *s)
{
  m_sMessage = string(s);
}


void TriggerObject::new_message(string &new_message)
{
  m_sMessage = new_message;
}
