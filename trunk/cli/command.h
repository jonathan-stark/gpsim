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

#if !defined (__COMMAND_H_)
#define __COMMAND_H_
#include <string>
#include <list>
using namespace std;
#include <unistd.h>
#include <glib.h>
#include "misc.h"
#include "expr.h"
#include "../config.h"
#include "../src/gpsim_def.h"

class Processor;

enum COMMAND_MODES
{
  NO_COMMAND,
  COMMAND_PENDING,
  COMMAND_ERROR
};

// Token types for command options
#define OPT_TT_BITFLAG  1   // 
#define OPT_TT_NUMERIC  2   // indicates that a numeric parameter
                            // is associated with the option
#define OPT_TT_STRING   3   // indicates that a string parameter
                            // is associated with the option
#define OPT_TT_SUBTYPE  4   // indicates that this command can
                            // be subtyped (e.g. the stimulus command
                            // is subtyped by the type of stimulus
                            // that is being created).

void init_parser(void);

class command
{

public:
  static Processor  *cpu;
  struct cmd_options *op;
  char   *name;
  string brief_doc;
  string long_doc;

  int  token_value;

  virtual bool can_span_lines(void) {return 0;};

  command(void);
  command(struct cmd_options *options,int tv);
  virtual ~command() {}

  struct cmd_options *get_op(void)
    {
      return op;
    };

  void new_processor(Processor *p);

  int get_token(void) {return token_value;};

  bool have_cpu(bool display_warning);

  // Assume command is not repeatable
  virtual int is_repeatable(void) { return 0; };
  virtual double evaluate(Expression *);
  virtual void evaluate(ExprList_t *eList,guint64 *, int *);

};

class cmd_options_expr
{
 public:
  
  cmd_options_expr(cmd_options *, Expression *);
  ~cmd_options_expr();

  cmd_options *co;
  Expression *expr;
};

extern command *command_list[];
extern int number_of_commands;
extern command *search_commands(const string &s);
extern int quit_gpsim;
extern void execute_line(char *);

#define DEBUG_PARSER 0

//========================================
// typedefs 

typedef list<string> StringList_t;

#endif
