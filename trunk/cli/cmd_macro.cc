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


#include <iostream>
#include <iomanip>
#include <map>

#include "command.h"
#include "cmd_macro.h"

extern int parse_string(const char * str);
extern void add_string_to_input_buffer(const char *s,Macro *);
extern void start_new_input_stream(void);
class MacroLine {
  string text;
};

map <const string, Macro *> macro_map;

Macro::Macro(const char *_name)
{
  new_name(_name);

  if(verbose & 4)
    cout << "defining a new macro named: " << name() << endl;

}

//----------------------------------------
//add_argument(char *new_arg)
//
// Add a new argument to the macro. This is called when
// the macro is being defined.

void Macro::add_argument(const char *new_arg)
{
  if(new_arg)
    arguments.push_back(string(new_arg));


  if(verbose & 4) {
    cout << "defining a paramter named: " << new_arg << endl;
  }
}

//----------------------------------------
//add_body(char *new_line)
//
// Add a new line to the macro body. This is called when
// the macro is being defined. These same lines will get
// redirected to the lexer input stream when the macro
// is invoked.

void Macro::add_body(const char *new_line)
{
  if(!new_line)
    return;

  body.push_back(string(new_line));

  if(verbose & 4) {
    cout << "macro body: " << new_line << endl;
  }
}

//----------------------------------------
// invoke()
//
// Invoke a macro by copying its body to the lexer input 
// stream. 
void Macro::invoke()
{
  list <string> :: iterator si;

  start_new_input_stream();

  if(body.size()) {

    for(si = body.begin();
	si != body.end();
	++si)
      add_string_to_input_buffer( (*si).c_str(), this);
  }

  add_string_to_input_buffer("endm\n", this);

}
//----------------------------------------
// substituteParameter(const string &s, string &replaced)
//
// Given a string 's', this function will search to see if
// this is the name of one of the macro parameters. If it
// is, then the text that should replace that parameter is
// is returned in the string 'replaced'.
//
// This routine gets called when the lexer comes across an
// undefined identifier while it is trying to expand a macro.
//

int Macro::substituteParameter(const string &s, string &replaced)
{

  if(arguments.size()) {

    list <string> :: iterator asi;
    list <string> :: iterator psi;

    for(asi = arguments.begin(), psi = parameters.begin();
	asi != arguments.end();
	++asi, ++psi)
      if(*asi == s) {
	
	replaced = *psi;

	if(verbose&4)
	  cout << "Found match, replacing "<<*asi << " with " << *psi<<endl;
	return 1;
      }
  }

  return 0;
}

//----------------------------------------
int Macro::nParameters()
{
  return arguments.size();
}

//----------------------------------------
// prepareForInvocation()
//
// If this macro has been invoked before, then
// there is some historical garbage sitting in the parameter
// list. (Recall, the parameters will substitute the macro
// arguments).

void Macro::prepareForInvocation()
{
  parameters.clear();
}

//----------------------------------------
// add_parameter(char *s)
//
// Add a new macro parameter. This is called when a macro
// is invoked. The parameters are matched up with the macro
// arguments (i.e. the first parameter will substitute the
// first argument).

void Macro::add_parameter(const char *s)
{
  parameters.push_back(string(s));
}

//----------------------------------------
// print()
//
void Macro::print()
{
  cout << name() << " macro ";

  list <string> :: iterator si;

  if(arguments.size()) {

    for(si = arguments.begin();
	si != arguments.end();
	++si)
      cout << *si << " ";

  }

  cout << endl;


  if(body.size()) {

    for(si = body.begin();
	si != body.end();
	++si)
      cout << "  " << *si;

  }

  cout << "endm\n";

}

// hack....
static Macro *theMacro = 0;   // Used during macro definition

Macro *isMacro(const string &s)
{

  map<const string, Macro *>::iterator mi = macro_map.find(s);

  if(mi != macro_map.end())
    return (*mi).second;

  return 0;
}
//========================================================================

cmd_macro c_macro;


static cmd_options cmd_macro_options[] =
{
 {0,0,0}
};

cmd_macro::cmd_macro()
  : command("macro",0)
{ 

  brief_doc = string("macro definition and listing");

  long_doc = string ("\nListing Macros:\n\n"
		       "\tmacro -- display the names of the currently defined macros\n"
		       "\t         (use the symbol command to see a particular macro definition)\n"
		       "\nDefining Macros:\n"
		       "\nname macro [arg1, arg2, ...]\n"
		       "macro body\n"
		       "endm\n\n"
		       "Example:\n\n"
		       "s macro n, regs\n"
		       "echo Step and Show\n"
		       "step n\n"
		       "x regs\n"
		       "endm\n\n"
		       "Invoke by\n\n"
		       "gpsim> s 5, 1:10\n"
		       " (note that the parameters must be separated by commas)\n"
);

  op = cmd_macro_options; 
}


void cmd_macro::list()
{
  if(macro_map.size()) {
    map<const string, Macro *>::iterator mi;
    for (mi=macro_map.begin(); mi!=macro_map.end(); ++mi) 

      mi->second->print();
  } else
    cout << "No macros have been defined.\n";
}


void cmd_macro::define(const char *name)
{
  if(!name)
    return;

  map<const string, Macro *>::iterator mi = macro_map.find(string(name));

  if(mi != macro_map.end()) {
    cout << "macro '" << name << "' is already defined\n";
    return;
  }

  theMacro = new Macro(name);

  macro_map[theMacro->name()] = theMacro;
}

void cmd_macro::add_parameter(const char *parameter)
{
  if(!parameter || !theMacro)
    return;

  theMacro->add_argument(parameter);

}

void cmd_macro::add_body(const char *line)
{
  if(!line)
    return;

  theMacro->add_body(line);

}

void cmd_macro::end_define(const char *opt_name)
{
  if(verbose & 4) {
    GetUserInterface().GetConsole().Printf(
      "ending macro definition of '%s'\n", theMacro->name().c_str());
  }
  theMacro = 0;
}
