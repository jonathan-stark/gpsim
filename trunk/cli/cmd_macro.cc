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
#include <string>

#include "command.h"
#include "../src/gpsim_object.h"
#include "cmd_macro.h"
#include "expr.h"

extern int parse_string(char * str);
extern void add_string_to_input_buffer(char *s);
extern void start_new_input_stream(void);
class MacroLine {
  string text;
};

class Macro :public gpsimObject {
public:
  Macro(char *new_name);
  StringList_t arguments;
  StringList_t body;

  void invoke(StringList_t *);
  void add_argument(char *new_arg);
  void add_body(char *new_line);
  void print(void);
};


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

void Macro::invoke(StringList_t *parameters)
{
  list <string> :: iterator si;

  start_new_input_stream();

  if(body.size()) {

    for(si = body.begin();
	si != body.end();
	++si)
      add_string_to_input_buffer((char *) ((*si).c_str()));

  }

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

Macro *theMacro = 0;

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
    theMacro->invoke(0);
    cout << "invoked\n";
  } else
    cout << "No macros have been defined.\n";
}


void cmd_macro::define(char *name)
{
  if(!name)
    return;

  theMacro = new Macro(name);

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
