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

#include <iostream>
#include <iomanip>

#include <string>
#include <list>
#include <vector>

#include "../config.h"
#include "14bit-processors.h"
#include "value.h"
#include "stimuli.h"
#include "symbol.h"
#include "symbol_orb.h"
#include "expr.h"
#include "errors.h"

int open_cod_file(Processor **, const char *);

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

list <symbol *> st;
list <symbol *>::iterator sti;

Symbol_Table symbol_table;  // There's only one instance of "the" symbol table

// create an instance of inline get_symbol_table() method by taking its address
static Symbol_Table &(*dummy_symbol_table)(void) = get_symbol_table;

void Symbol_Table::add_ioport(IOPORT *_ioport)
{

  ioport_symbol *is = new ioport_symbol(_ioport);

  st.push_back(is);

}

void Symbol_Table::add_stimulus_node(Stimulus_Node *s)
{

  node_symbol *ns = new node_symbol(s);

  st.push_back(ns);

}

void Symbol_Table::add_stimulus(stimulus *s)
{

  stimulus_symbol *ss = new stimulus_symbol(s);

  st.push_back(ss);

}

void Symbol_Table::add(symbol *s)
{
  if(s)
    st.push_back(s);
}

void Symbol_Table::add_register(Register *new_reg, char *symbol_name )
{

  if(!new_reg)
    return;

  register_symbol *rs = new register_symbol(symbol_name, new_reg);

  new_reg->symbol_alias = rs;

  st.push_back(rs);

}

void Symbol_Table::add_w(WREG *new_w)
{

  if(!new_w)
    return;

  w_symbol *ws = new w_symbol((char *)0, new_w);

  new_w->symbol_alias = ws;

  st.push_back(ws);

}

void Symbol_Table::add_constant(char *new_name, int value)
{

  constant_symbol *sc = new constant_symbol(new_name, value);

  st.push_back(sc);

}

void Symbol_Table::add_address(char *new_name, int value)
{

  address_symbol *as = new address_symbol(new_name,value);

  st.push_back(as);

}

void Symbol_Table::add_line_number(int address, char *symbol_name)
{

  line_number_symbol *lns = new line_number_symbol(symbol_name,  address);

  st.push_back(lns);
}

void Symbol_Table::add_module(Module * m, const char *cPname)
{
  module_symbol *ms = new module_symbol(m,(char *)cPname);

  st.push_back(ms);

}

void Symbol_Table::remove_module(Module * m)
{
  sti = st.begin();
  symbol *sym;

  while( sti != st.end())
    {
      sym = *sti;
      if(sym->isa() == SYMBOL_MODULE &&
        sym->name() == m->name())
        {
          st.remove(sym);
          cout << "found and removed\n";
          return;
        }

      sti++;
    }
}

void Symbol_Table::add(char *new_name, char *new_type, int value)
{


  if(new_type) {

    // ugh.. FIXME
    if ( strcmp("constant", new_type) == 0)
      add_constant(new_name, value);

  }

#if 0
  int i;
  for(i=0; i<num_of_symbol_types; i++)
    {

      if(strcmp(symbol_types[i].name_str, new_type) == 0)
	{
	  switch(symbol_types[i].type)
	    {
	    case SYMBOL_CONSTANT:
	      add_constant(new_name, value);
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
#endif

}

symbol * Symbol_Table::find(char *str)
{
  string s =  string(str);
  return(find(&s));

}

symbol * Symbol_Table::find(SYMBOL_TYPE symt, char *str)
{

  string s =  string(str);
  symbol *sym;

  sti = st.begin();

  while( sti != st.end())
    {
      sym = *sti;
      if((sym->name() == s) && (sym->isa() == symt))
	return(sym);

      sti++;
    }

  return 0;

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


void Symbol_Table::dump_type(SYMBOL_TYPE symt)
{
  // Now loop through the whole table and display all instances of the type of interest

  int first=1;     // On the first encounter of one, display the title

  sti = st.begin();

  symbol *sym;

  while( sti != st.end())
    {
      sym = *sti;
      if(sym)
	if(sym->isa() == symt) {
	  if(first) {
	    first = 0;
	    cout << "Symbol Table for \"" << sym->type_name() << "\"\n";
	  }

	  sym->print();
	}

      sti++;
    }
  
  if(first)
    cout << "No symbols found\n";

}

//--------------------------------------------

int  load_symbol_file(Processor **cpu, const char *filename)
{
  cout << "Loading " << filename << '\n';
  return open_cod_file(cpu,  filename);

}

//*****************************************************************
// *** KNOWN CHANGE ***
//  Support functions that will get replaced by the CORBA interface.
//  
#if 0
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
void symbol_add_one(char *sym_name, char *sym_type, int value)
{

  symbol_table.add(sym_name,sym_type,value);
}

//--------------------------------------------
int get_symbol_value(char *sym, int *sym_value)
{

  symbol *s = symbol_table.find(sym);

  if(s)
    {
      *sym_value = s->get_value();
      return 1;
    }

  return 0; // symbol not found

}

//--------------------------------------------
void print_symbol(char *sym)
{

  symbol *s = symbol_table.find(sym);

  if(s)
    s->print();
  else 
    cout << sym << " was not found in the symbol table\n";

}

void update_symbol_value(char *sym, int new_value)
{

  symbol *s = symbol_table.find(sym);

  if(s)
    s->put_value(new_value);
  else 
    cout << sym << " was not found in the symbol table\n";

}
#endif
//------------------------------------------------------------------------
// symbols
//
//

symbol::symbol(char *_name)
{
  if(_name)
    new_name(_name);
}
symbol::symbol(string &_name)
{
  new_name(_name);
}

symbol::~symbol(void)
{
}

symbol *symbol::copy()
{

  throw new Error("symbol can't be copied");
  return 0;
}

void symbol::set(int new_value)
{
  throw new Error("cannot assign value to symbol");
}

void symbol::set(Value *v)
{
  throw new Error("cannot assign value to symbol");
}

void symbol::get(int &i)
{
  throw new Error("cannot get value of symbol");

}

//------------------------------------------------------------------------
void symbol::print(void)
{

  cout << name() << " type " << type_name();
  
  cout << endl;

}

void symbol::assignTo(Expression *expr)
{
  try {

    if(!expr)
      throw new Error(" null expression ");

    Value *v = expr->evaluate();
    if(!v)
      throw new Error(" cannot evaluate expression ");

    set(v);

  
    delete v;
    delete expr;
  }


  catch (Error *err) {
    if(err)
      cout << "ERROR:" << err->toString() << endl;
    delete err;
  }

}

//------------------------------------------------------------------------
string symbol::toString()
{
  return showType();
}

symbol * Symbol_Table::find(string *s)
{

  sti = st.begin();
  symbol *sym;

  while( sti != st.end())
    {
      sym = *sti;
      if(sym->name() == *s)
	return(sym);

      sti++;
    }

  return 0;

}

//------------------------------------------------------------------------
node_symbol::node_symbol(Stimulus_Node *_sn)
  : symbol(0) , stimulus_node(_sn)
{
  if(stimulus_node)
    new_name(stimulus_node->name());
}

void node_symbol::print(void)
{
  if(stimulus_node) {
    cout << "node: " << stimulus_node->name() << " voltage = " << stimulus_node->get_nodeVoltage() << endl;
    stimulus *s = stimulus_node->stimuli;
    while(s) {
      cout << '\t' << s->name() << '\n';
      s = s->next;
    }
  } else
    cout << "has no attached stimuli\n";

}

//------------------------------------------------------------------------
// register_symbol
//
register_symbol::register_symbol(char *_name, Register *_reg)
  : symbol(_name), reg(_reg)
{
  if(reg)
    new_name(reg->name());
}

void register_symbol::print(void)
{
  if(reg) {
    char str[33];

    cout << name() << hex << " [0x" << reg->address << "] = 0x" 
	 << reg->get_value()
	 << " = 0b" << (reg->toBitStr(str,sizeof(str)))
	 << endl;
  }
}
/*
unsigned int register_symbol::get_value(void)
{
  if(reg)
    return reg->address;
  return 0;
}
*/

void  register_symbol::get(int &i)
{
  if(reg)
    i = reg->get_value();
  else
    i = 0;
}

void register_symbol::set(int new_value)
{
  if(reg)
    reg->put_value(new_value);
}

void register_symbol::set(Value *v)
{
  if(reg && v) {
    int i;
    v->get(i);
    reg->put_value(i);
  }
}

symbol *register_symbol::copy()
{
  return new register_symbol((char *)0,reg);
}

//------------------------------------------------------------------------
w_symbol::w_symbol(char *_name, Register *_reg)
  : register_symbol(_name, _reg)
{
}
void w_symbol::print(void)
{
  cout << reg->name() << hex << " = 0x" << reg->get_value() <<'\n';
}

//------------------------------------------------------------------------
ioport_symbol::ioport_symbol(IOPORT *_ioport)
  : symbol(0), ioport(_ioport)
{
  if(ioport)
    new_name(ioport->name());
}

symbol *ioport_symbol::copy()
{
  return new ioport_symbol(ioport);
}

void ioport_symbol::set(int new_value)
{
  if(ioport)
    ioport->put_value(new_value);
}

void ioport_symbol::set(Value *v)
{
  if(ioport && v) {
    int i;
    v->get(i);
    ioport->put_value(i);
  }
}

void  ioport_symbol::get(int &i)
{
  if(ioport)
    i = ioport->get_value();
  else
    i = 0;
}

//------------------------------------------------------------------------
constant_symbol::constant_symbol(char *_name, unsigned int _val)
  :  symbol(_name), val(_val)
{
}

void constant_symbol::print(void)
{
  cout << name() << " = 0x" << hex << val <<'\n';
}

symbol *constant_symbol::copy()
{
  return new constant_symbol((char *)name().c_str(),val);
}

void constant_symbol::get(int &i)
{
  i = val;
}
void constant_symbol::set(int i)
{
  val = i;
}

void constant_symbol::set(Value *v)
{
  if(v) {
    int i;
    v->get(i);
    val = i;
  }
}

/*
double constant_symbol::getAsDouble()
{
  double dVal = val;
  return dVal;
}
*/
//------------------------------------------------------------------------
address_symbol::address_symbol(char *_name, unsigned int _val)
  :  constant_symbol(_name,_val)

{
}
void address_symbol::print(void)
{
  cout << name() << " at address 0x" << hex << val <<'\n';
}
line_number_symbol::line_number_symbol(char *_name, unsigned int _val)
  :  address_symbol(_name,_val)
{
  if(!_name) {
    char buf[64];
    snprintf(buf,sizeof(buf), "line_%04x",val);
    new_name(buf);
  }

}
//------------------------------------------------------------------------
module_symbol::module_symbol(Module *_module, char *_name)
  : symbol(_name), module(_module)
{
}

void module_symbol::print(void)
{
  if(module) {
    cout << module->type() << "  named ";

    cout << name() << '\n';
    cout << "Attributes:\n";
    module->dump_attributes();
  }
}

//------------------------------------------------------------------------
stimulus_symbol::stimulus_symbol(stimulus *_s)
  : symbol(0), s(_s)
{
  if(s)
    new_name(s->name());

}

//------------------------------------------------------------------------
string &stimulus_symbol::name()
{
  if(s)
    return s->name();

  return Value::name();
}

//------------------------------------------------------------------------
val_symbol::val_symbol(gpsimValue *v)
  : symbol((char*)0)
{
  if(!v)
    throw string(" val_symbol");

  val = v;
}

char * val_symbol::type_name(void)
{
  return "val_symbol";
}

string val_symbol::toString()
{
  return val->toString();
}
void val_symbol::print(void)
{
  cout << type_name() << " " << val->name() << " = " << val->get_value() << endl;
}
void val_symbol::get(int &i)
{
  if (val)
    i = val->get_value();
  else
    i = 0;
}

void val_symbol::set(int new_value)
{
  val->put_value(new_value);
}
string &val_symbol::name(void)
{
  return val->name();
}
