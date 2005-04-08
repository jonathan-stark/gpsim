
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
#include <iostream>
#include <iomanip>
#include <string>
#include <list>
#include <vector>
#include <unistd.h>
#include <glib.h>
using namespace std;

#include "misc.h"
#include "command.h"

#include "cmd_attach.h"
#include "cmd_break.h"
#include "cmd_bus.h"
#include "cmd_clear.h"
#include "cmd_disasm.h"
#include "cmd_dump.h"
#include "cmd_frequency.h"
#include "cmd_help.h"
#include "cmd_list.h"
#include "cmd_load.h"
#include "cmd_log.h"
#include "cmd_node.h"
#include "cmd_macro.h"
#include "cmd_module.h"
#include "cmd_processor.h"
#include "cmd_quit.h"
#include "cmd_reset.h"
#include "cmd_run.h"
#include "cmd_set.h"
#include "cmd_step.h"
#include "cmd_shell.h"
#include "cmd_stimulus.h"
#include "cmd_symbol.h"
#include "cmd_trace.h"
#include "cmd_version.h"
#include "cmd_x.h"
#include "cmd_icd.h"
#include "../src/expr.h"
#include "../src/operator.h"

#include "../src/symbol.h"
#include "../src/stimuli.h"

extern void lexer_setMacroBodyMode(void);
extern void lexer_setInitialMode(void);
extern void lexer_InvokeMacro(Macro *m);


#define YYERROR_VERBOSE

extern char *yytext; 

int yylex(void);
int quit_parse=0;
int abort_gpsim=0;
int parser_warnings;
int parser_spanning_lines=0;
extern int use_gui;
extern int quit_state;


void init_cmd_state(void);


void yyerror(char *message)
{
  printf("***ERROR: %s while parsing:\n'%s'\n",message, yytext);
  init_cmd_state();

#if 0
  static int i = 0;

  if(i++>5)
    exit(0);
#endif

}


%}

/* Bison declarations */

%union {
  guint32              i;
  guint64             li;
  float                f;
  char                *s;
  cmd_options        *co;
  cmd_options_num   *con;
  cmd_options_str   *cos;
  cmd_options_expr  *coe;

  BinaryOperator*           BinaryOperator_P;
  Boolean*                  Boolean_P;
  Expression*               Expression_P;
  Float*                    Float_P;
  Integer*                  Integer_P;
  String*                   String_P;
  Value*                    Symbol_P;

  StringList_t             *StringList_P;
  ExprList_t               *ExprList_P;
  SymbolList_t             *SymbolList_P;

  Macro                    *Macro_P;
}


/* gpsim commands: */
%token   ABORT
%token   ATTACH
%token   BREAK
%token   BUS
%token   CLEAR
%token   DISASSEMBLE
%token   DUMP
%token   ENDM
%token   FREQUENCY
%token   HELP
%token   LOAD
%token   LOG
%token   LIST
%token   NODE
%token   MACRO
%token   MODULE
%token   PROCESSOR
%token   QUIT
%token   RESET
%token   RUN
%token   SET
%token <String_P>  SHELL 
%token   STEP
%token   STIMULUS
%token   SYMBOL
%token   TRACE
%token   gpsim_VERSION
%token   X
%token   ICD
%token   END_OF_COMMAND

%type <s>    mdef_body_
%type <s>    mdef_end

%token <s>   MACROBODY_T
%token <Macro_P>   MACROINVOCATION_T

%token <s>  STRING

%token <li>  INDIRECT

%token <li>  END_OF_INPUT

%token <co>  BIT_FLAG
%token <co>  EXPRESSION_OPTION
%token <co>  NUMERIC_OPTION
%token <co>  STRING_OPTION
%token <co>  CMD_SUBTYPE
%token <co>  SYMBOL_OPTION

/* Expression parsing stuff */
%type <BinaryOperator_P>        binary_expr
%type <Expression_P>            expr
%type <Expression_P>            literal
%type <Expression_P>            unary_expr
%type <ExprList_P>              expr_list
%type <ExprList_P>              array

%type <Expression_P>            break_mask_expr
%type <Expression_P>            break_boolean_expr

%type  <SymbolList_P>           symbol_list


%token <Integer_P>   LITERAL_INT_T
%token <Boolean_P>   LITERAL_BOOL_T
%token <Float_P>     LITERAL_FLOAT_T
%token <String_P>    LITERAL_STRING_T
%token <Symbol_P>    SYMBOL_T


%token EQU_T

%token AND_T
%token COLON_T
%token COMMENT_T
%token DIV_T
%token EOLN_T
%token MINUS_T
%token MPY_T
%token OR_T
%token PLUS_T
%token SHL_T
%token SHR_T
%token XOR_T



//%type  <li>   _register
%type  <co>  bit_flag
%type  <co>  cmd_subtype

%type  <con> numeric_option
%type  <cos> string_option
%type  <coe> expression_option
%type  <StringList_P>              string_list


// Here are token definitions for expression operators
%nonassoc COLON_T
%left     PLUS_T MINUS_T XOR_T OR_T AND_T
%left     MPY_T DIV_T
%left     SHL_T SHR_T

%left     LOR_T
%left     LAND_T
%left     EQ_T NE_T
%left     LT_T LE_T GT_T GE_T MIN_T MAX_T ABS_T

%nonassoc IND_T

%left     BIT_T BITS_T
%right    LOW_T HIGH_T LADDR_T WORD_T
%nonassoc INDEXED_T

%right    LNOT_T ONESCOMP_T UNARYOP_PREC
%right    POW_T

%left     REG_T

%%
/* Grammar rules */

/*
input:

      {
	// Command is complete. Prepare for the next command.
	cout << " command has been parsed\n";
        init_cmd_state();
      }
     | list_of_commands
      {
	// Command is complete. Prepare for the next command.
	cout << " command has been parsed\n";

      }

*/
list_of_commands:
         //rol  { cout << " list_of_commands: rol\n";}
      cmd rol  { 
        init_cmd_state();

      }
      | list_of_commands  cmd rol
      { 
        init_cmd_state();
      }
       ;

cmd:
     /* empty */
     | aborting
     | attach_cmd
     | break_cmd
     | bus_cmd
     | clear_cmd
     | disassemble_cmd
     | dump_cmd
     | eval_cmd
     | frequency_cmd
     | help_cmd
     | list_cmd
     | log_cmd
     | load_cmd
     | node_cmd
     | macro_cmd
     | module_cmd
     | processor_cmd
     | quit_cmd
     | reset_cmd
     | run_cmd
     | set_cmd
     | step_cmd
     | shell_cmd
     | stimulus_cmd
     | symbol_cmd
     | trace_cmd
     | version_cmd
     | x_cmd
     | icd_cmd
     | END_OF_INPUT
     {
       //if(verbose&2)
         cout << "got an END_OF_INPUT\n";
        /* If we're processing a command file then quit parsing 
         * when we run out of input */
	 //if(Gcmd_file_ref_count)
       	 //quit_parse = 1;
       YYABORT;
     }
     | error {
       init_cmd_state();
       yyclearin;
       //yyerrok;
       YYABORT;
     }

   ;

/*****************************************************************
 * All lines must terminate with a 'rol' (Rest Of Line) - even empty ones! 
 */
rol
        : opt_comment EOLN_T
        ;


opt_comment
        : /* empty */
        | COMMENT_T
        ;


aborting: ABORT
          {
       	  abort_gpsim = 1;
          quit_parse = 1;
          YYABORT;
          }
          ;
            
attach_cmd
          : ATTACH string_list          {attach.attach($2); delete $2;}
          | ATTACH symbol_list          {attach.attach($2); delete $2;}
          ;

break_cmd
          : BREAK                             {c_break.list();}
          | BREAK bit_flag SYMBOL_T           {c_break.set_break($2,$3);}
          | BREAK bit_flag LITERAL_INT_T      {c_break.set_break($2,$3);}
          | BREAK bit_flag break_boolean_expr {c_break.set_break($2,$3);}
          | BREAK bit_flag                    {c_break.set_break($2);}
          ;

bus_cmd
          : BUS                         {c_bus.list_busses();}
          | BUS string_list             {c_bus.add_busses($2); delete $2;}
          ;

clear_cmd: CLEAR expr                   {clear.clear($2);}
          ;

disassemble_cmd
          : DISASSEMBLE                 {disassemble.disassemble(0);}
          | DISASSEMBLE expr            {disassemble.disassemble($2);}
          ;

dump_cmd: 
          DUMP                          {dump.dump(2);}
          | DUMP bit_flag               {dump.dump($2->value);}
          ;

eval_cmd:
	  SYMBOL_T {c_symbol.dump_one($1);}
          | SYMBOL_T EQU_T expr { $1->set($3);}
          ;

frequency_cmd
          : FREQUENCY                   {frequency.print();}
          | FREQUENCY expr              {frequency.set($2);}
          ;

help_cmd
          : HELP                        {help.help(); }
          | HELP LITERAL_STRING_T       {help.help($2->getVal()); /*free($2);*/}
          | HELP SYMBOL_T               {help.help($2);}
          ;

list_cmd
          : LIST                        {c_list.list();}
          | LIST bit_flag               {c_list.list($2);}
          ;

load_cmd: LOAD bit_flag LITERAL_STRING_T
          {
            quit_parse = c_load.load($2->value,$3->getVal()) == 0;
            free($3);

            if(quit_parse)
            {
              quit_parse = 0;
              YYABORT;
            }
	        }
          ;

log_cmd
          : LOG                         {c_log.log();}
          | LOG bit_flag                {c_log.log($2,0,0);}
//          | LOG bit_flag LITERAL_STRING_T         {c_log.log($2,$3->getVal(),0);}
//          | LOG bit_flag LITERAL_STRING_T expr_list {c_log.log($2,$3->getVal(),$4);}
          | LOG bit_flag expr_list      {c_log.log($2,0,$3);}
          ;


node_cmd
          : NODE                        {c_node.list_nodes();}
          | NODE string_list            {c_node.add_nodes($2);  delete $2;}
          ;

module_cmd
          : MODULE                      {c_module.module();}
          | MODULE bit_flag             {c_module.module($2);}
          | MODULE string_option        {c_module.module($2,(list <string> *)0);}
          | MODULE string_option string_list
                                        {c_module.module($2, $3);}
          ;


processor_cmd: PROCESSOR
          {
	    c_processor.processor();
	  }
          | PROCESSOR bit_flag
	  {
	    c_processor.processor($2->value);
	  }
          | PROCESSOR LITERAL_STRING_T
	  {
	    c_processor.processor($2->getVal(),0);
	  }
          | PROCESSOR LITERAL_STRING_T LITERAL_STRING_T
	  { 
            c_processor.processor($2->getVal(),$3->getVal());
          }

          ;

quit_cmd: QUIT
          { 
            quit_parse = 1;
	    YYABORT;
          }
	  | QUIT expr
	  {
            quit_parse = 1;
	    //quit_state = $2;  // FIXME need to evaluate expr
	    YYABORT;
	  }
          ;

reset_cmd: 
          RESET                         { reset.reset(); }
          ;

run_cmd: 
          RUN                           { c_run.run();}
          ;

set_cmd
          : SET                         {c_set.set();}
          | SET bit_flag                {c_set.set($2->value,0);}
          | SET bit_flag expr           {c_set.set($2->value,$3);}
          ;

step_cmd
          : STEP                        {step.step(1);}
          | STEP expr                   {step.step($2);}
          | STEP bit_flag               {step.over();}
          ;

shell_cmd
          : SHELL                       {c_shell.shell($1); delete $1;}
          ;

stimulus_cmd: STIMULUS
          {
          c_stimulus.stimulus();
          }
          | STIMULUS cmd_subtype
          {
          c_stimulus.stimulus($2->value);
          }
          | stimulus_cmd stimulus_opt
          {
          /* do nothing */
          }
          | stimulus_cmd  END_OF_COMMAND
          { 
            if(verbose)
              cout << " end of stimulus command\n";
            c_stimulus.end();
          }
          ;

stimulus_opt: 
	  
	  expression_option
          {
            if(verbose)
              cout << "parser sees stimulus with numeric option\n";
	    c_stimulus.stimulus($1);
	  }
          | stimulus_opt bit_flag
          {
            if(verbose)
              cout << "parser sees stimulus with bit flag: " << $2->value << '\n';
	    c_stimulus.stimulus($2->value);
	  }
          | stimulus_opt string_option
          {
            if(verbose)
              cout << "parser sees stimulus with string option\n";
	    c_stimulus.stimulus($2);
	  }
          | stimulus_opt array
          { 
            if(verbose)
              cout << "parser sees stimulus with an array\n";
	    c_stimulus.stimulus($2);
	  }
          ;


symbol_cmd
          : SYMBOL                      {c_symbol.dump_all();}
          | SYMBOL LITERAL_STRING_T     {c_symbol.dump_one($2->getVal());}
          | SYMBOL SYMBOL_T             {c_symbol.dump_one($2);}
          ;


trace_cmd: 
          TRACE                         { c_trace.trace(); }
          | TRACE expr                  { c_trace.trace($2); }
          | TRACE numeric_option        { c_trace.trace($2); }
          | TRACE string_option         { c_trace.trace($2); }
          | TRACE bit_flag              { c_trace.trace($2); }
          | TRACE expression_option     { c_trace.trace($2); }
          ;

version_cmd: gpsim_VERSION              {version.version();}
          ;

x_cmd
          : X                           { c_x.x();}
          | X expr                      { c_x.x($2); }
          ;

icd_cmd: 
          ICD                           { c_icd.icd(); }
          | ICD string_option           { c_icd.icd($2); }
          ;


//------------------------------------------------------------
//  macro definitions
//
//  Syntax:
//
// name macro [arg1, arg2, ...]
//   macro body
// endm
//
// 'name' is the macro name
// 'macro' is a keyword
// arg1,arg2,... are optional arguments
// 'macro body' - any valid gpsim command
// 'endm' is a keyword
//

macro_cmd:
	  MACRO                         { c_macro.list();}
         | macrodef_directive           { }
         | MACROINVOCATION_T            { lexer_InvokeMacro($1); }

         ;

macrodef_directive
        : LITERAL_STRING_T MACRO
                                        {c_macro.define($1->getVal());}
          opt_mdef_arglist rol
                                        {lexer_setMacroBodyMode();}

          mdef_body mdef_end            
        ;

opt_mdef_arglist
        : /* empty */
        | STRING                        {c_macro.add_parameter($1);}
        | opt_mdef_arglist ',' STRING   {c_macro.add_parameter($3);}
        ;


mdef_body
        : /* empty */
        | mdef_body_                    {; }
        ;

mdef_body_
        : MACROBODY_T                   {c_macro.add_body($1);}
        | mdef_body_ MACROBODY_T        {c_macro.add_body($2);}
        ;

mdef_end
        : ENDM                          {c_macro.end_define();}
        | LITERAL_STRING_T ENDM                   {c_macro.end_define($1->getVal()); }
        ;


// Indirect addressing is supported with the indirect
// operator '*'. E.g. If register 0x20 contains 0x2e
// then *0x20 means that the contents of register 0x2e
// are referenced.

//
//indirect: INDIRECT _register
//	{
//	  if(verbose)
//            printf(" indirect register *%d",(int)$2);
//	  $$ = $2;
//        }
//        ;
//
//_register: NUMBER
//      {
//	if(verbose)
//         printf("  --- register %d\n", (int)$1);
//      }
//      ;

bit_flag: BIT_FLAG
      {
	 $$ = $1;
      }
      ;

cmd_subtype: CMD_SUBTYPE 
      {
	 $$ = $1;
      }
      ;

expression_option: EXPRESSION_OPTION expr { $$ = new cmd_options_expr($1,$2); }
        ;

numeric_option: NUMERIC_OPTION expr
        { 

	  $$ = new cmd_options_num;
	  $$->co = $1;
	}
        ;

string_option: STRING_OPTION LITERAL_STRING_T
        { 
	  $$ = new cmd_options_str($2->getVal());
	  $$->co  = $1;
          if(verbose&2)
	    cout << " name " << $$->co->name << " value " << $$->str << " got a string option \n"; 
	}
        ;

string_list
        : LITERAL_STRING_T                        {$$ = new StringList_t(); $$->push_back($1->getVal());}
        | string_list LITERAL_STRING_T            {$1->push_back($2->getVal());}
        ;

symbol_list
  : SYMBOL_T {$$ = new SymbolList_t(); $$->push_back($1); }
  | symbol_list SYMBOL_T {$1->push_back($2);}
  ;

//----------------------------------------
// Expression parsing

expr    : binary_expr                   {$$=$1;}
        | unary_expr                    {$$=$1;}
        ;

array   : '{' expr_list '}'             {$$=$2;}
        ;

expr_list
        : expr                          {$$ = new ExprList_t(); $$->push_back($1);}
        | expr_list ',' expr                {$1->push_back($3);}
        ;

binary_expr
        : expr   PLUS_T      expr       {$$ = new OpAdd($1, $3);}
        | expr   MINUS_T     expr       {$$ = new OpSub($1, $3);}
        | expr   MPY_T       expr       {$$ = new OpMpy($1, $3);}
        | expr   DIV_T       expr       {$$ = new OpDiv($1, $3);}
        | expr   AND_T       expr       {$$ = new OpAnd($1, $3);}
        | expr   OR_T        expr       {$$ = new OpOr($1, $3);}
        | expr   XOR_T       expr       {$$ = new OpXor($1, $3);}
        | expr   SHL_T       expr       {$$ = new OpShl($1, $3);}
        | expr   SHR_T       expr       {$$ = new OpShr($1, $3);}
        | expr   EQ_T        expr       {$$ = new OpEq($1, $3);}
        | expr   NE_T        expr       {$$ = new OpNe($1, $3);}
        | expr   LT_T        expr       {$$ = new OpLt($1, $3);}
        | expr   GT_T        expr       {$$ = new OpGt($1, $3);}
        | expr   LE_T        expr       {$$ = new OpLe($1, $3);}
        | expr   GE_T        expr       {$$ = new OpGe($1, $3);}
        | expr   LAND_T      expr       {$$ = new OpLogicalAnd($1, $3);}
        | expr   LOR_T       expr       {$$ = new OpLogicalOr($1, $3);}
        | expr   COLON_T     expr       {$$ = new OpAbstractRange($1, $3);}
        ;

// break w A == 2
// break r A == 2
// break r A & 0xff == 2
// break w A & 0x1f == 5

break_mask_expr 
        : SYMBOL_T                                        {$$ = new LiteralSymbol($1);}
        | SYMBOL_T          AND_T     LITERAL_INT_T       {$$ = new OpAnd(new LiteralSymbol($1), new LiteralSymbol($3));}
        | REG_T '(' LITERAL_INT_T ')'                     {
          cout<<"reg expression\n";
$$ = new LiteralInteger($3);}
        ;

break_boolean_expr
        : break_mask_expr   EQ_T      LITERAL_INT_T       {$$ = new OpEq( $1, new LiteralSymbol($3));}
//        | break_mask_expr   NE_T      LITERAL_INT_T       {$$ = new OpNe( $1, new LiteralSymbol($3));}
//        | break_mask_expr   LT_T      LITERAL_INT_T       {$$ = new OpLt( $1, new LiteralSymbol($3));}
//        | break_mask_expr   GT_T      LITERAL_INT_T       {$$ = new OpGt( $1, new LiteralSymbol($3));}
//        | break_mask_expr   LE_T      LITERAL_INT_T       {$$ = new OpLe( $1, new LiteralSymbol($3));}
//        | break_mask_expr   GE_T      LITERAL_INT_T       {$$ = new OpGe( $1, new LiteralSymbol($3));}
        ;
        
unary_expr
        : literal                       {$$=$1;}
        | PLUS_T      unary_expr   %prec UNARYOP_PREC   {$$ = new OpPlus($2);}
        | MINUS_T     unary_expr   %prec UNARYOP_PREC   {$$ = new OpNegate($2);}
        | ONESCOMP_T  unary_expr   %prec UNARYOP_PREC   {$$ = new OpOnescomp($2);}
        | LNOT_T      unary_expr   %prec UNARYOP_PREC   {$$ = new OpLogicalNot($2);}
        | MPY_T       unary_expr   %prec UNARYOP_PREC   {$$ = new OpIndirect($2);}
        | '(' expr ')'                                  {$$=$2;}

        ;

literal : LITERAL_INT_T                 {$$ = new LiteralInteger($1);}
        | LITERAL_BOOL_T                {$$ = new LiteralBoolean($1);}
        | LITERAL_STRING_T              {$$ = new LiteralString($1);}
        | LITERAL_FLOAT_T               {$$ = new LiteralFloat($1);}
        | SYMBOL_T                      {$$ = new LiteralSymbol($1);}

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
  // c_bus.token_value = BUS;
  clear.token_value = CLEAR;
  disassemble.token_value = DISASSEMBLE;
  dump.token_value = DUMP;
  frequency.token_value = FREQUENCY;
  help.token_value = HELP;
  c_list.token_value = LIST;
  c_load.token_value = LOAD;
  c_log.token_value = LOG;
  c_macro.token_value = MACRO;
  c_module.token_value = MODULE;
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
  c_icd.token_value = ICD;
  c_shell.token_value = SHELL;

  initialized = 1;

  parser_spanning_lines = 0;
  parser_warnings = 1; // display parser warnings.
}
