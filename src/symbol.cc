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
along with gpasm; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

//
// symbol.cc
//
//  The file contains the code that controls all of the symbol
// stuff for gpsim. Most of the work is handled by the C++ map
// container class.
//

#include <iostream.h>
#include <iomanip.h>

#include <string>
#include <list>
#include <vector>

#include "../config.h"
#include "14bit-processors.h"
#include "stimuli.h"
#include "symbol.h"
#include "symbol_orb.h"


int open_cod_file(pic_processor **, char *);

//
// ***NOTE*** Ideally, I would like to use a the std container 'map' 
// to implement symbol tables. Unfortunately, iterators do not work
// correctly with maps in libg++ (?version 1.27?)

// Create a map for the symbol table. Note that the scope of the
// symbol table is only within symbol.cc. The symbol table is
// indexed by the symbol's name (a string). If a symbol is found
// in the table then a pointer to it is returned.

//map <string, symbol *, less<string> > st;
//map <string, symbol *, less<string> >::iterator sti;

vector <symbol *> st;
vector <symbol *>::iterator sti;

Symbol_Table symbol_table;  // There's only one instance of "the" symbol table

// Define an array for the types of symbols:
symbol_type symbol_types[] =
{

  // SYMBOL_TYPE type;  char * name_str;

  {SYMBOL_BASE_CLASS,"symbol_base"},
  {SYMBOL_IOPORT,"ioport"},
  {SYMBOL_STIMULUS_NODE,"node"},
  {SYMBOL_STIMULUS,"stimulus"},
  {SYMBOL_LINE_NUMBER,"line_number"},
  {SYMBOL_CONSTANT,"constant"},
  {SYMBOL_REGISTER,"register"},
  {SYMBOL_ADDRESS,"address"},
  {SYMBOL_PROCESSOR,"processor"},
  {SYMBOL_MODULE,"module"},

};

int num_of_symbol_types = (sizeof(symbol_types))/ (sizeof(symbol_type));

void Symbol_Table::add_ioport(pic_processor *cpu, IOPORT *_ioport)
{

  ioport_symbol *is = new ioport_symbol();

  is->name_str = _ioport->name();
  is->cpu      = _ioport->cpu;
  is->ioport   = _ioport;
  st.push_back(is);

}

void Symbol_Table::add_stimulus_node(Stimulus_Node *s)
{

  node_symbol *ns = new node_symbol();

  ns->name_str = s->name();
  ns->cpu      = NULL;
  ns->stimulus_node = s;
  st.push_back(ns);

}

void Symbol_Table::add_stimulus(stimulus *s)
{

  stimulus_symbol *ss = new stimulus_symbol();

  ss->name_str = s->name();
  ss->cpu      = NULL;
  ss->s        = s;
  st.push_back(ss);

}

void Symbol_Table::add_register(pic_processor *cpu, file_register *new_reg)
{

  if(new_reg==NULL)
    return;

  register_symbol *rs = new register_symbol();

  rs->name_str = new_reg->name();
  rs->cpu      = cpu;
  rs->reg      = new_reg;
  new_reg->symbol_alias = rs;

  st.push_back(rs);

}

void Symbol_Table::add_constant(pic_processor *cpu, char *new_name, int value)
{

  constant_symbol *sc = new constant_symbol();

  sc->cpu      = cpu;
  sc->name_str = new_name;
  sc->val      = value;
  st.push_back(sc);

}

void Symbol_Table::add_address(pic_processor *cpu, char *new_name, int value)
{

  address_symbol *as = new address_symbol();

  as->cpu      = cpu;
  as->name_str = new_name;
  as->val      = value;
  st.push_back(as);

}

void Symbol_Table::add_line_number(pic_processor *cpu, int address)
{

  line_number_symbol *lns = new line_number_symbol();

  char buf[64];
  sprintf(buf,"line_%04x",address);  //there's probably a c++ way to do this
  lns->name_str = buf;
  lns->cpu      = cpu;
  lns->address  = address;
  st.push_back(lns);

}

void Symbol_Table::add_module(Module * m, char *new_name)
{
  cout << "add module\n";

  module_symbol *ms = new module_symbol();

  ms->cpu      = m;
  ms->name_str = new_name;

  st.push_back(ms);

}

void Symbol_Table::add(pic_processor *cpu, char *new_name, char *new_type, int value)
{

  cout << "Adding new symbol " << new_name << " of type " << new_type << '\n';

  int i;

  for(i=0; i<num_of_symbol_types; i++)
    {

      if(strcmp(symbol_types[i].name_str, new_type) == 0)
	{
	  switch(symbol_types[i].type)
	    {
	    case SYMBOL_CONSTANT:
	      add_constant(cpu, new_name, value);
	      break;

	    case SYMBOL_BASE_CLASS:
	    case SYMBOL_IOPORT:
	    case SYMBOL_STIMULUS_NODE:
	    case SYMBOL_STIMULUS:
	    default:
	      cout << " need to declare a new symbol\n";
	    }
	}
    }
}

// default base constructor for a symbol is not used

symbol::symbol(void)
{
  //  cout << " a symbol table\n";

}

void symbol::print(void)
{

  cout << *name() << " type " << type_name();
  
  if(cpu)
    {
      cout << " in cpu " << cpu->name();
    }
  cout << '\n';

}


symbol * Symbol_Table::find(string *s)
{

  sti = st.begin();
  symbol *sym;

  while( sti != st.end())
    {
      sym = *sti;
      if(sym->name_str == *s)
	return(sym);

      sti++;
    }

  return NULL;

}

symbol * Symbol_Table::find(char *str)
{
  string s =  string(str);
  return(find(&s));

}

void Symbol_Table::dump_one(string *s)
{

  symbol * sym = find(s);

  if(sym)
    {
      sym->print();
    }
}

void Symbol_Table::dump_one(char *str)
{
  string s =  string(str);
  dump_one(&s);
}


void Symbol_Table::dump_all(void)
{
  cout << "  Symbol Table\n";
  sti = st.begin();

  symbol *sym;

  while( sti != st.end())
    {
      sym = *sti;
      if(sym)
	if(sym->isa() != SYMBOL_LINE_NUMBER)
	  sym->print();

      sti++;
    }
}

//--------------------------------------------

int  load_symbol_file(pic_processor **cpu, char *filename)
{
  cout << "Loading " << filename << '\n';
  return open_cod_file(cpu,  filename);

}

//*****************************************************************
// *** KNOWN CHANGE ***
//  Support functions that will get replaced by the CORBA interface.
//  

//--------------------------------------------
void symbol_dump_all(void)
{
  symbol_table.dump_all();

}
//--------------------------------------------
void symbol_dump_one(char *sym_name)
{
  symbol_table.dump_one(sym_name);
}

//--------------------------------------------
void symbol_add_one(pic_processor *cpu, char *sym_name, char *sym_type, int value)
{

  symbol_table.add(cpu, sym_name,sym_type,value);
}

//--------------------------------------------
int get_symbol_value(char *sym, int *sym_value)
{

  symbol *s = symbol_table.find(sym);

  if(s)
    {
      *sym_value = s->get_value();
      return 0;
    }

  return 1; // symbol not found

}
