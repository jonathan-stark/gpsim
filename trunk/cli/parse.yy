
%{
/* Parser for gpsim
   Copyright (C) 1999 Scott Dattalo

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

#include <stdio.h>
#include <iostream.h>
#include <iomanip.h>
#include <string>
#include <vector>
#include <glib.h>

#include "misc.h"
#include "command.h"

#include "cmd_attach.h"
#include "cmd_break.h"
#include "cmd_clear.h"
#include "cmd_disasm.h"
#include "cmd_dump.h"
#include "cmd_help.h"
#include "cmd_list.h"
#include "cmd_load.h"
#include "cmd_node.h"
#include "cmd_processor.h"
#include "cmd_quit.h"
#include "cmd_reset.h"
#include "cmd_run.h"
#include "cmd_set.h"
#include "cmd_step.h"
#include "cmd_stimulus.h"
#include "cmd_symbol.h"
#include "cmd_trace.h"
#include "cmd_version.h"
#include "cmd_x.h"

#define YYERROR_VERBOSE
void yyerror(char *message)
{
  printf("***ERROR: %s\n",message);
  //exit(1);
}

int yylex(void);
int quit_parse;
int parser_warnings;
int parser_spanning_lines=0;

char_list *str_list_head;
char_list *str_list;

void free_char_list(char_list *);
 
%}

/* Bison declarations */

%union {
  guint32              i;
  guint64             li;
  float                f;
  char                *s;
  cmd_options        *co;
  cmd_options_num   *con;
  cmd_options_float *cof;
  cmd_options_str   *cos;
}


/* gpsim commands: */
%token <s>  ATTACH
%token <s>  BREAK
%token <s>  CLEAR
%token <s>  DISASSEMBLE
%token <s>  DUMP
%token <s>  HELP
%token <s>  LOAD
%token <s>  LIST
%token <s>  NODE
%token <s>  PROCESSOR
%token <s>  QUIT
%token <s>  RESET
%token <s>  RUN
%token <s>  SET
%token <s>  STEP
%token <s>  STIMULUS
%token <s>  SYMBOL
%token <s>  TRACE
%token <s>  gpsim_VERSION
%token <s>  X
%token <s>  END_OF_COMMAND
%token <s>  IGNORED
%token <s>  SPANNING_LINES

/*%token <i>  NODE_SYM STIMULUS_SYM*/

%token <s>  STRING

%token <li>  INDIRECT

%token <li>  END_OF_INPUT

%token <co>  BIT_FLAG
%token <co>  NUMERIC_OPTION
%token <co>  STRING_OPTION

%token <li>  NUMBER
%token <f>   FLOAT_NUMBER

%type  <li>   _register
%type  <co>  bit_flag
%type  <li>   indirect
%type  <con> numeric_option
%type  <cof> numeric_float_option
%type  <cos> string_option

%%
/* Grammar rules */
cmd: acmd
      {
         //cout << "got something\n";
      }
    | acmd IGNORED
      {
        //cout << "got something followed by something to ignore\n";
	
	YYABORT;
      }
      ;

acmd: ignored
     | attach_cmd
     | break_cmd
     | clear_cmd
     | disassemble_cmd
     | dump_cmd
     | help_cmd
     | list_cmd
     | load_cmd
     | node_cmd
     | processor_cmd
     | quit_cmd
     | reset_cmd
     | run_cmd
     | set_cmd
     | step_cmd
     | stimulus_cmd
     | symbol_cmd
     | trace_cmd
     | version_cmd
     | x_cmd
     | spanning_lines
     | END_OF_INPUT
     {
       if(verbose&2)
         cout << "got an END_OF_INPUT\n";
       quit_parse = 1;
       YYABORT;
     }
   ;

ignored: IGNORED
          {
            //if(parser_warnings || (verbose & 2 ))
            if(verbose & 2)
              cout << "parser is ignoring input\n";

            if(!parser_spanning_lines) {
              if(verbose & 2)
                cout << "  parser is aborting current input stream\n";

	      YYABORT;
            }
          }
          ;

spanning_lines:  SPANNING_LINES
          {
            if(verbose)
              cout << "parser is spanning lines\n";
            parser_spanning_lines = 1;
          }
          ;

attach_cmd: ATTACH string_list
          {
            if(verbose&2)
	      cout << "attach command with a string list\n";
	    attach.attach(str_list_head);
	    free_char_list(str_list_head);
            YYABORT;

	  }
          ;

break_cmd: BREAK
          { c_break.list(); }
          | BREAK bit_flag 
          { 
	    cmd_options *opt = $2;
	    c_break.set_break(opt->value); 
	  }
          | BREAK bit_flag _register
          { c_break.set_break($2->value,$3); }
          | BREAK bit_flag _register NUMBER
          { c_break.set_break($2->value,$3,$4); }
          | BREAK bit_flag STRING
          { c_break.set_break($2->value,$3); }
          | BREAK bit_flag STRING NUMBER
          { c_break.set_break($2->value,$3,$4); }
          ;

clear_cmd: CLEAR NUMBER
          { clear.clear($2); }
          ;

disassemble_cmd: DISASSEMBLE
          { disassemble.disassemble(-10, 5)}
          |  DISASSEMBLE NUMBER
          { disassemble.disassemble(0, $2)}
          |  DISASSEMBLE NUMBER NUMBER
          { disassemble.disassemble(-$2,$3)}
          ;

dump_cmd: DUMP
          { dump.dump(2);}
          | DUMP bit_flag
          { dump.dump($2->value);}
          ;


help_cmd: HELP
          { help.help(); }
          | HELP STRING
          { help.help($2); free($2); }
          ;

list_cmd: LIST
          { c_list.list();}
          | LIST indirect
          { printf("got a list with an indirect reference %d\n",$2);}
          | LIST bit_flag
          { 
	    cmd_options *opt = $2;
	    //cout << "  --- list with bit flag " << opt->name << '\n';
	    c_list.list($2);
	  }
           ;

load_cmd: LOAD bit_flag STRING
          {
	    c_load.load($2->value,$3);
	    //cout << "load completed\n\n";
	    if(quit_parse)
	      {
		quit_parse = 0;
		YYABORT;
	      }
	  }
          ;

node_cmd: NODE
          { 
	    c_node.list_nodes();
	  }
          | NODE string_list
          {
	    //cout << "node command with a string list\n";
	    c_node.add_nodes(str_list_head);
	    free_char_list(str_list_head);
            YYABORT;
          }
          ;

processor_cmd: PROCESSOR
          { c_processor.processor(); YYABORT;}
          | PROCESSOR bit_flag
	  { c_processor.processor($2->value); YYABORT;}
          | PROCESSOR STRING
	  { c_processor.processor($2,NULL); YYABORT; }
          | PROCESSOR STRING STRING
	  { 
            c_processor.processor($2,$3);
            YYABORT;
          }

          ;

quit_cmd: QUIT
          { 
            printf("got a quit\n");
            quit_parse = 1;
          }
          ;

reset_cmd: RESET
          { reset.reset(); }
          ;

run_cmd: RUN
          { c_run.run(); }
          ;

set_cmd:      SET
          { 
            c_set.set();
          }
            | SET bit_flag
          {
            c_set.set($2->value,1);
          }
            | SET bit_flag NUMBER
          {
            c_set.set($2->value,$3);
          }
            | SET numeric_option
          {
	    c_set.set($2);
	  }
          ;

step_cmd: STEP
          { step.step(1); }
          | STEP NUMBER
          { step.step($2); }
          | STEP bit_flag
          { step.over(); }
          ;

stimulus_cmd: STIMULUS
          {
            if(verbose)
              cout << "parser sees stimulus\n";
	    c_stimulus.stimulus();
	  }
          | STIMULUS NUMBER
          { 
            if(verbose)
              cout << "parser sees stimulus with number: " << $2 << '\n';

	    c_stimulus.stimulus($2);
	  }
          | STIMULUS FLOAT_NUMBER
          { 
            if(verbose)
              cout << "parser sees stimulus with float number: " << $2 << '\n';

	    c_stimulus.stimulus($2);
	  }

          | STIMULUS stimulus_opt END_OF_COMMAND
          { 
	    //cout << " end of stimulus command\n";
	    c_stimulus.end();
            parser_spanning_lines = 0;
	  }
          ;

stimulus_opt: 
          {
            if(verbose)
              cout << "parser sees stimulus(in _opt)\n"; // << $1->value << '\n';
	    //c_stimulus.stimulus($1->value);
	  }
          | stimulus_opt SPANNING_LINES
          {
            if(verbose)
              cout << "parser is ignoring spanned line in stimulus\n";
            //YYABORT;
          }
          | stimulus_opt bit_flag
          {
            if(verbose)
              cout << "parser sees stimulus with bit flag: " << $2->value << '\n';
	    c_stimulus.stimulus($2->value);
	  }
          | stimulus_opt numeric_option
          {
            if(verbose)
              cout << "parser sees stimulus with numeric option\n";
	    c_stimulus.stimulus($2);
	  }
          | stimulus_opt numeric_float_option
          {
            if(verbose)
              cout << "parser sees stimulus with numeric float option\n";
	    c_stimulus.stimulus($2);
	  }
          | stimulus_opt string_option
          {
            if(verbose)
              cout << "parser sees stimulus with string option\n";
	    c_stimulus.stimulus($2);
	  }
          | stimulus_opt NUMBER
          { 
            if(verbose)
              cout << "parser sees stimulus with number\n";
	    c_stimulus.data_point($2);
	  }
          | stimulus_opt FLOAT_NUMBER
          { 
            if(verbose)
              cout << "parser sees stimulus with floating point number\n";
	    c_stimulus.data_point($2);
	  }
          ;

symbol_cmd: SYMBOL
          {
	    c_symbol.dump_all();
	  }
          | SYMBOL STRING
          {
	    c_symbol.dump_one($2);
	  }
          | SYMBOL STRING STRING NUMBER
          {
	    c_symbol.add_one($2,$3,$4);
	  }
          ;


trace_cmd: TRACE
          {
	    c_trace.trace();
	  }
          | TRACE NUMBER
          {
	    c_trace.trace($2);
	  }
          ;

version_cmd: gpsim_VERSION
          {
	    version.version();
	  }
          ;

x_cmd: X
          {
	    c_x.x(); YYABORT;
	  }
          | X NUMBER
          {
	    c_x.x($2);
	  }
          | X _register NUMBER
          {
	    c_x.x($2,$3);
	  }
          | X STRING
          {
	    c_x.x($2);
	  }
          | X STRING NUMBER
          {
	    c_x.x($2,$3);
	  }
          ;

// Indirect addressing is supported with the indirect
// operator '*'. E.g. If register 0x20 contains 0x2e
// then *0x20 means that the contents of register 0x2e
// are referenced.

indirect: INDIRECT _register
	{
	  if(verbose)
            printf(" indirect register *%d",$2);
	  $$ = $2;
        }
        ;

_register: NUMBER
       {
	if(verbose)
         printf("  --- register %d\n", $1);
       }
      ;

bit_flag: BIT_FLAG
       {
	 $$ = $1;
	 //cout << "  --- bit_flag " << $$->name << '\n';
       }
      ;

numeric_option: NUMERIC_OPTION NUMBER
        { 
	  //cout << $1->name;
	  $$ = new cmd_options_num;
	  $$->co = $1;
	  $$->n  = $2;
          if(verbose&2)
	    cout << "name " << $$->co->name << " value " << $$->n << " got a numeric option \n"; 
	}
        ;

numeric_float_option:  NUMERIC_OPTION FLOAT_NUMBER
        { 
	  //cout << $1->name;
	  $$ = new cmd_options_float;
	  $$->co = $1;
	  $$->f  = $2;
          if(verbose&2)
	    cout << "name " << $$->co->name << " value " << $$->f << " got a numeric option \n"; 
	}
        ;

string_option: STRING_OPTION STRING
        { 
	  //cout << $1->name;
	  $$ = new cmd_options_str;
	  $$->co  = $1;
	  $$->str = strdup($2);
          if(verbose&2)
	    cout << " name " << $$->co->name << " value " << $$->str << " got a string option \n"; 
	}
        ;

string_list: STRING
        {
	  str_list = (char_list *) malloc(sizeof(char_list)); //new(char_list);
	  str_list_head = str_list;
	  str_list->name = strdup($1);
	  str_list->next = NULL;
	  if(verbose&2)
	    cout << "got a string. added " << str_list->name << '\n';
	}
        | string_list STRING
        {
	  str_list->next = (char_list *) malloc(sizeof(char_list)); //new(char_list);
	  str_list = str_list->next;
	  str_list->name = strdup($2);
	  str_list->next = NULL;
	  if(verbose&2)
	    cout << " -- have a list of strings. added " << str_list->name << '\n';
	}
        ;


%%

       // parsing is over 

//--------------------------
// This initialization could be done by the compiler. However
// it requires two passes through because the token values are
// defined by the parser output (eg. y.tab.h) while at the same
// time the parser depends on the .h files in which these classes
// are defined.

void initialize_commands(void)
{
  static bool initialized = 0;

  if(initialized)
    return;

  if(verbose)
    cout << __FUNCTION__ << "()\n";

  attach.token_value = ATTACH;
  c_break.token_value = BREAK;
  clear.token_value = CLEAR;
  disassemble.token_value = DISASSEMBLE;
  dump.token_value = DUMP;
  help.token_value = HELP;
  c_list.token_value = LIST;
  c_load.token_value = LOAD;
  c_node.token_value = NODE;
  c_processor.token_value = PROCESSOR;
  quit.token_value = QUIT;
  reset.token_value = RESET;
  c_run.token_value = RUN;
  c_set.token_value = SET;
  step.token_value = STEP;
  c_stimulus.token_value = STIMULUS;
  c_symbol.token_value = SYMBOL;
  c_trace.token_value = TRACE;
  version.token_value = gpsim_VERSION;
  c_x.token_value = X;

  initialized = 1;

  parser_spanning_lines = 0;
  parser_warnings = 1; // display parser warnings.
}

void free_char_list(char_list *chl)
{
  char_list *old_node;

  while(chl)
    {

      old_node = chl;
      chl = chl->next;

      free (old_node->name);
      free (old_node);

    }

}
