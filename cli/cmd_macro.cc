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
#include "expr.h"

extern int parse_string(char * str);
extern void add_string_to_input_buffer(char *s,Macro *);
extern void start_new_input_stream(void);
class MacroLine {
  string text;
};

map <const string, Macro *> macro_map;

Macro::Macro(char *_name)
{
  new_name(_name);

  cout << "defining a new macro named: " << name() << endl;

}

void Macro::add_argument(char *new_arg)
{
  if(new_arg)
    arguments.push_back(string(new_arg));


  cout << "defining a paramter named: " << new_arg << endl;

}

void Macro::add_body(char *new_line)
{
  if(!new_line)
    return;

  body.push_back(string(new_line));

  cout << "macro body: " << new_line << endl;

}

void Macro::invoke()
{
  list <string> :: iterator si;

  start_new_input_stream();

  if(body.size()) {

    for(si = body.begin();
	si != body.end();
	++si)
      add_string_to_input_buffer((char *) ((*si).c_str()), this);

  }

}

int Macro::substituteParameter(string &s)
{

  if(arguments.size()) {

    list <string> :: iterator asi;
    list <string> :: iterator psi;

    for(asi = arguments.begin(), psi = parameters.begin();
	asi != arguments.end();
	++asi, ++psi)
      if(*asi == s) {
	
	start_new_input_stream();
	add_string_to_input_buffer((char *) ((*psi).c_str()), 0);
	cout << "Found match, replacing "<<*asi << " with " << *psi<<endl;
	return 1;
      }
  }

  return 0;
}

int Macro::nParameters()
{
  return arguments.size();
}

void Macro::prepareForInvocation()
{
  parameters.clear();
}

void Macro::add_parameter(char *s)
{
  parameters.push_back(string(s));
}

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
Macro *gCurrentMacro = 0;     // Used during macro invocation


Macro *isMacro(const string &s)
{

  map<string, Macro *>::iterator mi = macro_map.find(s);

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

cmd_macro::cmd_macro(void)
{ 
  name = "macro";

    brief_doc = string("macro definition and listing");

    long_doc = string ("\nMacro listing:"
		       "macro\n\n"
		       "\tmacro -- display the names of the currently defined macros\n"
		       "\t         (use the symbol command to see a particular macro definition)\n"
		       "\nMacro defining:"
		       "\nname macro [arg1, arg2, ...]\n"
		       "macro body\n"
		       "endm\n\n"
);

  op = cmd_macro_options; 
}


void cmd_macro::list(void)
{
  if(theMacro) {
    theMacro->print();
    cout << "invoking\n";
    theMacro->invoke();
    cout << "invoked\n";
  } else
    cout << "No macros have been defined.\n";
}


void cmd_macro::define(char *name)
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

void cmd_macro::add_parameter(char *parameter)
{
  if(!parameter || !theMacro)
    return;

  theMacro->add_argument(parameter);

}

void cmd_macro::add_body(char *line)
{
  if(!line)
    return;

  theMacro->add_body(line);

}

void cmd_macro::end_define(char *opt_name)
{
  cout << "ending macro definition\n";
}
