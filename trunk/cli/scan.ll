%{

/*
   Copyright (C) 1998 T. Scott Dattalo

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
#include <stdlib.h>
#include <iostream>

#include <string>
#include <unistd.h>
#include <glib.h>
#ifdef _WIN32
/* declaration of isatty() */
#include <io.h>
#endif

#include "command.h"
#include "operator.h"
#include "expr.h"
#include "errors.h"
#include "parse.h"
#include "input.h"
#include "scan.h"

int state;

// gpsim uses base '0' for the base of the numbers that are read from stdin.
// This means that unless the number is prefixed with '0x' for hex or
// '0' (zero) for octal it is assumed to be base 10.

//static int numeric_base = 0;

static struct cmd_options *op = 0;
static command *cmd = 0;
static int have_parameters = 0;
static int end_of_command = 0;
extern int quit_parse;
extern int parser_spanning_lines;
extern int last_command_is_repeatable;

static string strip_trailing_whitespace (char *s);
static int handle_identifier(const string &tok, cmd_options **op );
static int process_intLiteral(char *buffer, int conversionBase);
static int process_booleanLiteral(bool value);
static int process_floatLiteral(char *buffer);
static int recognize(int token,const char *);

int cli_corba_init (char *ior_id);

#define YYDEBUG 1
#define YY_NO_UNPUT

%}

D	[0-9]
S	[ \t]
NL	((\n)|(\r\n))
SNL	({S}|{NL})
BS	(\\)
CONT	({EL}|{BS})
CCHAR	(#)
COMMENT	({CCHAR}.*)
SNLCMT	({SNL}|{COMMENT})
INDIRECT (\*)
IDENT	([/_a-zA-Z\.][/_a-zA-Z0-9\.\-]*)
EXPON	([DdEe][+-]?{D}+)
DEC     ({D}+)
HEX1    ((0[Xx])[0-9a-fA-F]+)
HEX2    ("$"[0-9a-fA-F]+)
FLOAT	(({D}+\.?{D}*{EXPON}?)|(\.{D}+{EXPON}?))
BIN1    (0[bB][01]+)
BIN2    ([bB]\'[01]+\')

%x MULTILINE_MODE

%%

%{
// Comments. Ignore all text after a comment character
%}

{COMMENT} 
  { 
    return recognize(COMMENT_T,"comment");
  }
 

{S}+      {   /* ignore white space */ }

\n  { 
      // Got an eol.
      if(verbose)
          cout << "got EOL\n";

      input_mode = 0;  // assume that this is not a multi-line command.
      if(cmd && cmd->can_span_lines() && have_parameters && !end_of_command ) 
	{
          input_mode = CONTINUING_LINE;
        }
      else
	return recognize(EOLN_T, " end of line");
    }

q{S}+\n { /* short cut for quiting */ 
      quit_parse  =1;
      return QUIT;
    }

abort_gpsim_now {
  /* a sure way to abort a script */
   return ABORT;
   }


":"                 {return(recognize(COLON_T, ":"));}
"+"                 {return(recognize(PLUS_T,"+"));}
"-"                 {return(recognize(MINUS_T,"-"));}
"*"                 {return(recognize(MPY_T,"*"));}
"/"                 {return(recognize(DIV_T,"/"));}
"^"                 {return(recognize(XOR_T,"^"));}
"&"                 {return(recognize(AND_T,"^"));}
"|"                 {return(recognize(OR_T,"^"));}
"<<"                {return(recognize(SHL_T,"<<"));}
">>"                {return(recognize(SHR_T,">>"));}

{BIN1}              {return(process_intLiteral(&yytext[2], 2));}
{BIN2}              {return(process_intLiteral(&yytext[2], 2));}
{DEC}               {return(process_intLiteral(&yytext[0], 10));}
{HEX1}              {return(process_intLiteral(&yytext[2], 16));}
{HEX2}              {return(process_intLiteral(&yytext[1], 16));}
{FLOAT}             {return process_floatLiteral(yytext);}
"true"              {return(process_booleanLiteral(true));}
"false"             {return(process_booleanLiteral(false));}

%{

// The 'echo' command is handled by the lexer instead of the
// parser (like the other commands). All it does is just display
// the contents of yytext beyond the "echo".

%}

"echo"{S}.*{NL} {
   fprintf(yyout,"%s",&yytext[5]);
  }

"echo"{NL} {
   fprintf(yyout,"\n");
  }


%{
// Indirect register access
%}

{INDIRECT} {
  // printf("got indirect\n");
  return INDIRECT;
  }

%{
// If this is a command that is spanning more than one line
// then the 'end' command will finish it.
%}

"end"{S}* {
    if (cmd && cmd->can_span_lines() ) {
      end_of_command = 1;
      return(END_OF_COMMAND);
    }
    printf("Warning: found \"end\" while not in multiline mode\n");
  }

%{
// Identifiers.  Truncate the token at the first space or tab but
// don't write directly on yytext.
%}

{IDENT} {
  string tok = strip_trailing_whitespace (yytext);

  if(strlen(tok.c_str()))
    return handle_identifier (tok,&op);
  else
    return recognize(0,"invalid identifier");
  }

%{
/* Default is to recognize the character we are looking at as a single char */
%}
.                       {return(recognize(*yytext,"Single character"));}

%%

// Include these so that we don't have to link to libfl.a.

#ifdef yywrap
#undef yywrap
#endif
 int
yywrap (void)
{
  return 1;
}

int translate_token(int tt)
{
  switch(tt)
  {
  case OPT_TT_BITFLAG:
    if((verbose & 0x2) && DEBUG_PARSER)
      cout << " tt bit flag\n";
    return BIT_FLAG;
  case OPT_TT_NUMERIC:
    if((verbose & 0x2) && DEBUG_PARSER)
      cout << " tt numeric\n";
    return EXPRESSION_OPTION;
  case OPT_TT_STRING:
    if((verbose & 0x2) && DEBUG_PARSER)
      cout << " tt string\n";
    return STRING_OPTION;
  case OPT_TT_SUBTYPE:
    if((verbose & 0x2) && DEBUG_PARSER)
      cout << " tt bit flag\n";
    return CMD_SUBTYPE;
  }

  return 0;

}

/*************************************************************************
*
* handle_identifier
*
*  input   string &s
*          cmd_options **op
*  output  int 
*
*  1 - If `op' is NULL, then handle identifier hasn't been called
*      for the current command that's being processed. So, the
*      the string `s' is compared to all of the valid commands.
*      If it is valid, then `op' is assigned a pointer to the 
*      options associated with the command. If the string is not
*      found, then that's a syntax error and the string is ignored.
*  2 - If `op' is non-NULL, then handle_identifier has been called
*      at least once before for the command that's being processed.
*      So the string `s' is then compared to the options associated
*      with the command. If an option is not found, then the string
*      is returned to the parser (as a type STRING). This places the
*      burden of syntax checking on the parser and/or the individual
*      command.
*
*/

int handle_identifier(const string &s, cmd_options **op )
{
  int retval = 0;

  // If no command has been found so far, then the options (*op)
  // haven't been selected either (and consequently *op is null).

  if(! *op) {

    // Search the commands
    
    cmd = search_commands(s);
    if(cmd) {
      if(verbose&2)
        cout << "\n  *******\nprocessing command " << cmd->name << "\n  token value " <<
                (cmd->get_token()) << "\n *******\n";
	
      *op = cmd->get_op();
      have_parameters = 0;
      retval = cmd->get_token();
      last_command_is_repeatable = cmd->is_repeatable();
      return recognize(retval,"good command");

    } else {
      cout << " command: \"" << s << "\" was not found\n";

      return recognize(COMMENT_T,"ignoring command");

    }

 } else {

   // We already have the command, so search the options. 

   struct cmd_options *opt = *op;

   // We also have one or more parameters now (though they
   // may not be correct, but that's the parser's job to determine).

   have_parameters = 1;

   if(verbose&2)
     cout << "search options\n";

   while(opt->name)
    if(strcmp(opt->name, s.c_str()) == 0) {
      if(verbose&2)
        cout << "found option '" << opt->name << "'\n";
      yylval.co = opt;
      return recognize(translate_token(opt->token_type),"option");
    }
    else
      opt++;

   // If we get here, then the option was not found.
   // So let's check the symbols

    //cout << "search symbol list\n";
    //retval = search_symbols(s);
    //if(retval)
    //  return retval;

    //cout << "didn't find it in the symbol list\n";

   // Either 1) there's a typo or 2) the command is creating
   // a new symbol or node or something along those lines.
   // In either case, let's let the parser deal with it.

   yylval.s = strdup(s.c_str());
   return recognize(STRING,"string");
 }

 return 0;

}


/*****************************************************************
 * Process an integer literal.  This routine constructs the
 * YYSTYPE object.  The caller is responsible from returning the
 * LITERAL_INT_T token identifer to the parser.
 */
static int process_intLiteral(char *buffer, int conversionBase)
{
  char c;
  gint64 literalValue=0;
  gint64 nxtDigit;

  while (*buffer) {
    c = toupper(*buffer++);

    nxtDigit = (c) <= '9' ? c-'0' : c-'A'+10;
    if ((nxtDigit >= conversionBase) || (nxtDigit<0)) {
      /* If the next digit exceeds the base, then it's an error unless
	 this is a binary conversion and the character is a single quote */
      if(!(conversionBase == 2 && c == '\''))
	literalValue = 0;
      break;
    }
    
    literalValue *= conversionBase;
    literalValue += nxtDigit;
  }

  yylval.Integer_P = new Integer(literalValue);

  return(recognize(LITERAL_INT_T,"literal int"));
}

/*****************************************************************
 *
 */
//static int process_stringLiteral(char *stringValue)
//{
//  yylval.String_P = new String(stringValue, true);
//  return(recognize(LITERAL_STRING_T, "string literal"));
//}


/*****************************************************************
 *
 */
static int process_booleanLiteral(bool value)
{
  yylval.Boolean_P = new Boolean(value, true);
  return(recognize(LITERAL_BOOL_T, "boolean literal"));
}


/*****************************************************************
 *
 */
static int process_floatLiteral(char *buffer)
{
  double floatValue;

  errno = 0;
  floatValue = atof(buffer);
  
  if (errno != 0) {
    /* The conversion failed */
    throw new Error("Bad floating point literal");
  }

  yylval.Float_P = new Float(floatValue, true);
  return(recognize(LITERAL_FLOAT_T, "float literal"));
}


/*****************************************************************
 * 
 */
static int recognize(int token_id,const char *description)
{
  /* add optional debugging stuff here */
  if(verbose && description)
    cout << "scan: " << description << endl;

  return(token_id);
}


static string
strip_trailing_whitespace (char *s)
{

  string retval = s;

  size_t pos = retval.find_first_of (" \t");

  if (pos != string::npos)
    retval.resize (pos);

  return retval;
}

void initialize_commands(void);

void init_cmd_state(void)
{
    cmd = 0;
    op = 0;
    input_mode = 0;
    end_of_command = 0;
}

void init_parser(void)
{
  if((verbose & 2) && DEBUG_PARSER)
    cout << __FUNCTION__  << "()\n";
  initialize_commands();

  // Start off in a known state.
  BEGIN 0;

  // Can't have any options until we get a command.
  if(!parser_spanning_lines) {

    init_cmd_state();

    if((verbose & 2) && DEBUG_PARSER)
      cout << "not ";

    yyrestart (stdin);

  }
  if((verbose & 2) && DEBUG_PARSER)
    cout << "spanning lines"  << '\n';

}

// Tell us all what the current buffer is.

YY_BUFFER_STATE
current_buffer (void)
{
  return YY_CURRENT_BUFFER;
}

// Create a new buffer.

YY_BUFFER_STATE
create_buffer (FILE *f)
{
  return yy_create_buffer (f, YY_BUF_SIZE);
}

// Start reading a new buffer.

void
switch_to_buffer (YY_BUFFER_STATE buf)
{
  yy_switch_to_buffer (buf);
}

// Delete a buffer.

void
delete_buffer (YY_BUFFER_STATE buf)
{
  yy_delete_buffer (buf);
}

// Restore a buffer (for unwind-prot).

void
restore_input_buffer (void *buf)
{
  switch_to_buffer ((YY_BUFFER_STATE) buf);
}

// Delete a buffer (for unwind-prot).

void
delete_input_buffer (void *buf)
{
  delete_buffer ((YY_BUFFER_STATE) buf);
}

