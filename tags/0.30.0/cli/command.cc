/*
   Copyright (C) 1998,199 T. Scott Dattalo

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
#include <vector>

#include "../config.h"

#include "../src/expr.h"
#include "../src/pic-processor.h"
#include "../src/sim_context.h"
#include "../src/trace.h"

#include "command.h"

#include "cmd_attach.h"
#include "cmd_break.h"
#include "cmd_bus.h"
#include "cmd_clear.h"
#include "cmd_disasm.h"
#include "cmd_dump.h"
#include "cmd_echo.h"
#include "cmd_frequency.h"
#include "cmd_help.h"
#include "cmd_list.h"
#include "cmd_load.h"
#include "cmd_log.h"
#include "cmd_macro.h"
#include "cmd_module.h"
#include "cmd_node.h"
#include "cmd_processor.h"
#include "cmd_quit.h"
#include "cmd_reset.h"
#include "cmd_run.h"
#include "cmd_set.h"
#include "cmd_shell.h"
#include "cmd_step.h"
#include "cmd_stimulus.h"
#include "cmd_symbol.h"
#include "cmd_trace.h"
#include "cmd_version.h"
#include "cmd_x.h"
#include "cmd_icd.h"

int quit_gpsim = 0;

command *command_list[] =
{
  &c_shell,
  &attach,
  &c_break,
  //  &c_bus,
  &clear,
  &disassemble,
  &dump,
  //  &echo,
  &frequency,
  &help,
  &c_icd,
  &c_list,
  &c_load,
  &c_log,
  &c_macro,
  &c_module,
  &c_node,
  &c_processor,
  &quit,
  &reset,
  &c_run,
  &c_set,
  &step,
  &c_stimulus,
  &c_symbol,
  &c_trace,
  &version,
  &c_x
};


command::command(const char *_name, const char *_abbr)
  : m_pName(_name), m_pAbbreviation(_abbr)
{
  op = 0;
  token_value = 0;
}
/*
command::command(struct cmd_options *options,int tv)
{
  abbreviation = 0;
  op = options;
  token_value = tv;
}
*/

int number_of_commands = sizeof(command_list) / sizeof(command  *);


command *search_commands(const string &s)
{

  int i=0;

  while(i<number_of_commands) {
    command * cmd = command_list[i];
    if(strcmp(cmd->name(), s.c_str()) == 0 ||
       (cmd->abbreviation() != 0 && strcmp(cmd->abbreviation(), s.c_str()) == 0 )) {
      return cmd;
    }

    i++;

  }

  return 0;
}

void execute_line(char *cmd)
{
  if(verbose)
    cout << "Executing a line:\n  " << cmd;
}

Processor *command::GetActiveCPU(bool bDisplayWarnings)
{
  Processor *pCpu = CSimulationContext::GetContext()->GetActiveCPU();
  if (bDisplayWarnings && !pCpu)
    cout << "No cpu has been selected\n";
  return pCpu;
}

Value *command::toValue(Expression *expr)
{
  if(expr)
    return expr->evaluate();

  return new Integer(0);
}
double command::evaluate(Expression *expr)
{
  double value = 0.0;

  try {
    if(expr) {

      Value *v = toValue(expr);

      v->get(value);

      delete v;
      delete expr;
    }

  }

  catch (Error *err) {
    if(err)
      cout << "ERROR:" << err->toString() << endl;
    delete err;
  }

  return value;
}


static gint64 evaluateToInt(Expression *expr)
{
  gint64 value = 0;

  try {
    if(expr) {

      Value *v = expr->evaluate();

      v->get(value);

      delete v;
      delete expr;
    }

  }

  catch (Error *err) {
    if(err)
      cout << "ERROR:" << err->toString() << endl;
    delete err;
  }

  return value;
}
void command::evaluate(ExprList_t *eList,
                       guint64 *parameters,
                       int *nParameters)
{
  ExprList_itor ei;

  if (!eList) {
    if(nParameters)
      *nParameters = 0;
    return;
  }

  if ( !parameters || !nParameters || !*nParameters)
    return;

  int n = 0;
  for(ei = eList->begin();
      (ei != eList->end()) && (n < *nParameters);
      ++ei, n++)
    {
      parameters[n] = evaluateToInt(*ei);
      cout << "p" << n << " = " << parameters[n] << endl;

    }

  *nParameters = n;
}

//========================================================================
// Command options

cmd_options_str::cmd_options_str(const char *new_val)
{
  if(new_val)
    str = strdup(new_val);
  else
    str = 0;
}

cmd_options_str::~cmd_options_str()
{

  if(str)
    free (str);


}


cmd_options_expr::cmd_options_expr(cmd_options *_co, Expression *_expr)
{
  co = _co;
  expr = _expr;
}

cmd_options_expr::~cmd_options_expr()
{
  delete co;
  delete expr;
}
