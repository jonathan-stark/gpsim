/*
   Copyright (C) 2004 T. Scott Dattalo

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


#include "trigger.h"
#include "../config.h"
#include "value.h"
#include "expr.h"
#include "errors.h"
#include "breakpoints.h"
#include "ui.h"
#include "trace.h"
#include <iostream>
#include <iomanip>

extern Integer *verbosity;  // in ../src/init.cc

using namespace std;

static TriggerAction DefaultTrigger;



//------------------------------------------------------------------------
//
class BreakTraceType : public TraceType
{
public:

  BreakTraceType()
    : TraceType(2, "Break")
  {
  }

  virtual TraceObject *decode(unsigned int tbi);
  virtual int dump_raw(Trace *,unsigned tbi, char *buf, int bufsize);
};

TraceType *TriggerObject::m_brt=0;

//------------------------------------------------------------------------
class BreakTraceObject : public TraceObject
{
public:
  BreakTraceObject(unsigned int bpn);
  virtual void print(FILE *);
private:
  unsigned int m_bpn;
};


//------------------------------------------------------------------------
BreakTraceObject::BreakTraceObject(unsigned int bpn)
  : TraceObject(), m_bpn(bpn)
{
}

void BreakTraceObject::print(FILE *fp)
{
  fprintf(fp, "  BREAK: #");
  Breakpoints::BreakStatus *bs = bp.get(m_bpn);
  TriggerObject *bpo = bs ? bs->bpo : 0;
  if (bpo)
    bpo->print();
  else
    fprintf(fp,"%u\n", m_bpn);
}



//------------------------------------------------------------------------
TraceObject *BreakTraceType::decode(unsigned int tbi)
{
  TraceObject *to;
  if (get_trace().type(tbi) == type())
  {
      to = new BreakTraceObject(get_trace().get(tbi) );
      to->print(stdout);
      return to;
  }
  else
    return 0;
}
int BreakTraceType::dump_raw(Trace *pTrace,unsigned tbi, char *buf, int bufsize)
{
  int n = TraceType::dump_raw(pTrace, tbi,buf,bufsize);

  buf += n;
  bufsize -= n;

  unsigned int bpn = trace.get(tbi) & 0xffffff;
  Breakpoints::BreakStatus *bs = bp.get(bpn);

  TriggerObject *bpo = bs ? bs->bpo : 0;
  int m = snprintf(buf, bufsize, "  BREAK: #%u %s",
	       bpn, (bpo ? bpo->bpName() : ""));
  m = m>0 ? m : 0;
  buf += m;
  bufsize -= m;

  return (m+n+ ((bs && bs->bpo)?bs->bpo->printTraced(pTrace,tbi, buf, bufsize):0));
}

//------------------------------------------------------------------------
// TriggerAction
//
TriggerAction::TriggerAction()
{
}

TriggerAction::~TriggerAction()
{
}

bool TriggerAction::evaluate()
{
  action();
  return true;
}

bool TriggerAction::getTriggerState()
{
  return false;
}

void TriggerAction::action()
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

void SimpleTriggerAction::action()
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
  // If a trace type has not been allocated yet, then allocate:
  if (!m_brt) {
    m_brt = new BreakTraceType();
    get_trace().allocateTraceType(m_brt);
  }

  m_PExpr = 0;

  if(ta)
    set_action(ta);
  else
    set_action(&DefaultTrigger);
}

TriggerObject::~TriggerObject()
{
  /*
  cout << "Trigger Object destructor\n";
  if (m_PExpr)
    cout << "deleting expression "<< m_PExpr->toString() << endl;
  */
  delete m_PExpr;

  if (m_action != &DefaultTrigger)
    delete m_action;

}

void TriggerObject::callback()
{
  cout << "generic callback\n";
}

void TriggerObject::callback_print()
{
  cout << " has callback, ID =  0x" << CallBackID << '\n';
}

void  TriggerObject::clear_trigger()
{
}

int TriggerObject::find_free()
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

void TriggerObject::print()
{
  char buf[256];
  buf[0]=0;
  printExpression(buf, sizeof(buf));
  if (buf[0]) {
    GetUserInterface().DisplayMessage("    Expr:%s\n", buf);

  }
  if (message().size())
    GetUserInterface().DisplayMessage("    Message:%s\n", message().c_str());

}

int TriggerObject::printExpression(char *pBuf, int szBuf)
{
  if (!m_PExpr || !pBuf)
    return 0;
  *pBuf = 0;
  m_PExpr->toString(pBuf,szBuf);
  return strlen(pBuf);
}

int TriggerObject::printTraced(Trace *pTrace, unsigned int tbi,
			       char *pBuf, int szBuf)

{
  return 0;
}

void TriggerObject::clear()
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

      if(v) {
        v->get(bRet);
	delete v;
      }
    }
    catch (Error *Perr) {
      if(Perr)
        cout << "ERROR:" << Perr->toString() << endl;
      delete Perr;
    }

    return bRet;
  }

  return true;
}
//------------------------------------------------------------------------
void TriggerObject::invokeAction()
{
  trace.raw(m_brt->type() | bpn);
  m_action->action();
}

//-------------------------------------------------------------------
void TriggerObject::new_message(const char *s)
{
  m_sMessage = string(s);
}


void TriggerObject::new_message(string &new_message)
{
  m_sMessage = new_message;
}
