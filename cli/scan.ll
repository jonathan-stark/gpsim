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
#include <iostream.h>

#include <string>
#include <unistd.h>
#include <glib.h>

#include "command.h"
#include "parse.h"
#include "input.h"
#include "scan.h"

int state;

// gpsim uses base '0' for the base of the numbers that are read from stdin.
// This means that unless the number is prefixed with '0x' for hex or
// '0' (zero) for octal it is assumed to be base 10.

static int numeric_base = 0;

static struct cmd_options *op = NULL;
static command *cmd = NULL;
static int have_parameters = 0;
static int end_of_command = 0;
extern int quit_parse;
extern int parser_spanning_lines;

static string strip_trailing_whitespace (char *s);
int handle_identifier(const string &tok, cmd_options **op );

int cli_corba_init (char *ior_id);

#define YYDEBUG 1

%}

D	[0-9]
S	[ \t]
NL	((\n)|(\r\n))
SNL	({S}|{NL})
BS	(\\)
CONT	({EL}|{BS})
CCHAR	(#)
COMMENT	({CCHAR}.*{NL})
SNLCMT	({SNL}|{COMMENT})
INDIRECT (\*)
IDENT	([/_a-zA-Z\.][/_a-zA-Z0-9\.]*)
EXPON	([DdEe][+-]?{D}+)
DEC     ({D}+)
HEX     ((0[Xx])[0-9a-fA-F]+)
FLOAT	(({D}+\.?{D}*{EXPON}?)|(\.{D}+{EXPON}?))


%%

%{
// Comments. Ignore all text after a comment character
%}

{COMMENT} { 
   unput('\n');
   return IGNORED;
   }

{S} { 
   int c;
   do {
    c = yyinput();
   } while(c == ' ');

   if(c == '\n')
     return IGNORED;

   unput(c);

   }

\n  { 
      // Got an eol.
      input_mode = 0;  // assume that this is not a multi-line command.
      if(cmd != NULL) {
	if((verbose) && DEBUG_PARSER)
          cout << "EOL with " << cmd->name << '\n';
        if(cmd->can_span_lines() && have_parameters && !end_of_command ) {
	  if((verbose) && DEBUG_PARSER)
            cout << " spanning lines     \n";
          input_mode = CONTINUING_LINE;
          return SPANNING_LINES; //IGNORED; 
        } else
          return IGNORED;
      } else {
	if((verbose) && DEBUG_PARSER)
          cout << "EOL but no pending command\n";
        return IGNORED; 
      }
    }

q\n   { 
   quit_parse  =1;
   return QUIT;  }

<<EOF>> {
    //cout << "got an <<EOF>> in scan.l\n";
    //quit_parse = 1;
    return END_OF_INPUT;
  }

{DEC} {
   //yylval.i = strtoul(yytext,NULL,numeric_base);
   sscanf(yytext,"%Ld",&yylval.li);
   // printf("a number: 0x%x\n",yylval.i);
   return NUMBER;
  }

{HEX} { 
   //yylval.i = strtoul(yytext,NULL,0);
   sscanf(yytext,"%Lx",&yylval.li);
   // printf("a hex number: 0x%x\n",yylval.i);
   return NUMBER;
  }

{FLOAT} {
    sscanf(yytext,"%f",&yylval.f);
    return FLOAT_NUMBER;  
  }

%{

// The 'echo' command is handled by the lexer instead of the
// parser (like the other commands). All it does is just display
// the contents of yytext beyond the "echo".

%}

"echo".*{NL} {
   fprintf(yyout,"%s",&yytext[5]);
   return IGNORED;
  }

%{
// Indirect register access
%}

{INDIRECT} {
  // printf("got indirect\n");
  return INDIRECT;
  }

%{
// If this a command that is spanning more than one line
// then the 'end' command will finish it.
%}

"end"{S}* {
    end_of_command = 1;
    return(END_OF_COMMAND);
  }

%{
// Identifiers.  Truncate the token at the first space or tab but
// don't write directly on yytext.
%}

{IDENT}{S}* {
    string tok = strip_trailing_whitespace (yytext);
    if(strlen(tok.c_str()))
      return handle_identifier (tok,&op);
    else
      return 0;
  }

.         {
             printf("ignoring\n"); 
            // ECHO;
          }

%%

// Include these so that we don't have to link to libfl.a.

#ifdef yywrap
#undef yywrap
#endif
static int
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
      return NUMERIC_OPTION;
     case OPT_TT_STRING:
      if((verbose & 0x2) && DEBUG_PARSER)
        cout << " tt string\n";
      return STRING_OPTION;
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

  if(*op == NULL) {

    // Search the commands
    
    cmd = search_commands(s);
    if(cmd != NULL) {
      if(verbose&2)
        cout << "\n  *******\nprocessing command " << cmd->name << "\n  token value " <<
                (cmd->get_token()) << "\n *******\n";
	
      *op = cmd->get_op();
      have_parameters = 0;
      return cmd->get_token();

    } else {
      cout << " command: \"" << s << "\" was not found\n";
      //cout << " command: was not found\n";
      return IGNORED;

    }

 } else {

   // We already have the command, so search the options. 

   struct cmd_options *opt = *op;

   // We also have one or more parameters now (though they
   // may not be correct, but that's the parser's job to determine).

   have_parameters = 1;

   if((verbose) && DEBUG_PARSER)
     cout << "search options\n";

   while(opt->name != NULL)
    if(strcmp(opt->name, s.c_str()) == 0) {
      if((verbose) && DEBUG_PARSER)
        cout << "found option '" << opt->name << "'\n";
      yylval.co = opt;
      return translate_token(opt->token_type);
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
   return STRING;
 }

 return 0;

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

void init_parser(void)
{
  if((verbose & 2) && DEBUG_PARSER)
    cout << __FUNCTION__  << "()\n";
  initialize_commands();

  // Start off in a known state.
  BEGIN 0;

  // Can't have any options until we get a command.
  if(!parser_spanning_lines) {
    cmd = NULL;
    op = NULL;
    input_mode = 0;
    end_of_command = 0;
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

