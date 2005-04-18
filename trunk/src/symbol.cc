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
#include "symbol.h"

#include <iostream>
#include <iomanip>

#include <string>
#include <vector>

#include "../config.h"
#include "14bit-processors.h"
#include "stimuli.h"
#include "symbol_orb.h"
#include "expr.h"
#include "errors.h"
#include "protocol.h"

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

void Symbol_Table::add(Value *s)
{
  if(s)
    st.push_back(s);
}

void Symbol_Table::add_register(Register *new_reg, const char *symbol_name )
{

  if(!new_reg)
    return;

  if(symbol_name) {
    string sName(symbol_name);
    if((new_reg->name() == sName ||
       new_reg->baseName() == sName) &&  
      (find(new_reg->name()) || find(new_reg->baseName()))) {
      if(verbose)
        cout << "Warning not adding  "
	           << symbol_name
	           << " to symbol table\n because it is already in.\n";
      return;
     }
  }

  register_symbol *rs = new register_symbol(symbol_name, new_reg);

  st.push_back(rs);

}

void Symbol_Table::add_w(WREG *new_w)
{

  if(!new_w)
    return;

  w_symbol *ws = new w_symbol((char *)0, new_w);

  st.push_back(ws);

}

void Symbol_Table::add_constant(const char *_name, int value)
{

  Integer *i = new Integer(value);
  i->new_name(_name);

  st.push_back(i);

}

void Symbol_Table::add_address(const char *new_name, int value)
{

  address_symbol *as = new address_symbol(new_name,value);

  st.push_back(as);

}

void Symbol_Table::add_line_number(int address, const char *symbol_name)
{

  line_number_symbol *lns = new line_number_symbol(symbol_name,  address);

  st.push_back(lns);
}

void Symbol_Table::add_module(Module * m, const char *cPname)
{
  module_symbol *ms = new module_symbol(m,cPname);

  st.push_back(ms);

}

void Symbol_Table::remove_module(Module * m)
{
  sti = st.begin();
  Value *sym;

  while( sti != st.end()) {

    sym = *sti;
    if((typeid(sym) == typeid(module_symbol)) &&
       sym->name() == m->name()) {

      st.remove(sym);
      return;
    }

    sti++;
  }
}

Value *Symbol_Table::remove(string &s)
{
  Value *sym = find(s);
  if(sym)
    st.remove(sym);
  return sym;
}

void Symbol_Table::add(const char *new_name, const char *new_type, int value)
{


  if(new_type) {

    // ugh.. FIXME
    if ( strcmp("constant", new_type) == 0)
      add_constant(new_name, value);

  }

}

Value * Symbol_Table::find(const char *str)
{
  string s =  string(str);
  return(find(s));

}

Value * Symbol_Table::find(type_info const &symt, const char *str)
{

  string s =  string(str);
  sti = st.begin();

  while( sti != st.end()) {

    Value *val = *sti;
    
    if(val && (val->name() == s) && (typeid(val) == symt))
      return(val);

    sti++;
  }

  return 0;

}

Register * Symbol_Table::findRegister(unsigned int address)
{
  sti = st.begin();
  while( sti != st.end()) {
    Value *val = *sti;
    if(val && typeid(*val) == typeid(register_symbol)) {
      Register * pReg = ((register_symbol*)val)->getReg();
      if(pReg->address == address)
        return(pReg);
    }
    sti++;
  }
  return NULL;
}

Register * Symbol_Table::findRegister(const char *s)
{
  sti = st.begin();
  while( sti != st.end()) {
    Value *val = *sti;
    if(val && typeid(*val) == typeid(register_symbol)) {
      if(val->name() == s) {
        return ((register_symbol*)val)->getReg();
      }
    }
    sti++;
  }
  return NULL;
}

void Symbol_Table::dump_one(string *s)
{

  Value *val = find(*s);

  if(val)
    cout << val->toString() << endl;
}

void Symbol_Table::dump_one(const char *str)
{
  string s =  string(str);
  dump_one(&s);
}


void Symbol_Table::dump_all(void)
{
  cout << "  Symbol Table\n";
  sti = st.begin();

  while( sti != st.end())
    {
      Value *val = *sti;

      if(val &&(typeid(*val) != typeid(line_number_symbol)))
	cout << val->showType() << ": " << val->toString() << endl;

      sti++;
    }
}


void Symbol_Table::dump_type(type_info const &symt)
{


  // Now loop through the whole table and display all instances of the type of interest

  int first=1;     // On the first encounter of one, display the title

  sti = st.begin();

  while( sti != st.end()) {

    Value *sym = *sti;
    if(sym && (typeid(*sym) == symt)) {
      if(first) {
	first = 0;
	cout << "Symbol Table for \"" << sym->showType() << "\"\n";
      }

      cout << sym->toString() << endl;
    }

    sti++;
  }
  
  if(first)
    cout << "No symbols found\n";

}

bool IsClearable(Value* value)
{
  return value->isClearable();
}

void Symbol_Table::clear() {
  st.remove_if(IsClearable);
}

//--------------------------------------------

int  load_symbol_file(Processor **cpu, const char *filename)
{
  cout << "Loading " << filename << '\n';
  return open_cod_file(cpu,  filename);

}

//------------------------------------------------------------------------
// symbols
//
//

symbol::symbol(const char *_name)
{
  new_name(_name);
}
symbol::symbol(string &_name)
{
  new_name(_name);
}

symbol::~symbol(void)
{
}

//------------------------------------------------------------------------
string symbol::toString()
{
  return showType();
}

Value * Symbol_Table::find(string &s)
{
  const bool findDuplicates=false;
  sti = st.begin();

  Value *ret=0;
  while( sti != st.end()) {
      Value *val = *sti;
      if(val && val->name() == s) {
        if(!findDuplicates)
	  return val;

        if(!ret) {
	  ret = val;
	} else
	  cout << "Found duplicate:" << val->show()<<endl;
      }

      sti++;
    }

  return ret;

}

//------------------------------------------------------------------------
node_symbol::node_symbol(Stimulus_Node *_sn)
  : symbol(0) , stimulus_node(_sn)
{
  if(stimulus_node)
    new_name(stimulus_node->name());
}

string node_symbol::toString(void)
{
  return string("node:")+name();
}

//------------------------------------------------------------------------
// register_symbol
//
register_symbol::register_symbol(const char *_name, Register *_reg)
  : symbol(_name), reg(_reg)
{
  if (_name == NULL && reg != NULL) {
    name_str = _reg->name();
  }
}

register_symbol::register_symbol(Register *_reg)
  : reg(_reg), symbol(_reg->name())
{
}

string &register_symbol::name(void) {
  return name_str;
}

char *register_symbol::name(char *buf, int len) {
  return symbol::name(buf, len);
}

string register_symbol::toString()
{
  if(reg) {
    char buff[256];
    char bits[256];

    reg->toBitStr(bits,sizeof(bits));

    snprintf(buff,sizeof(buff)," [0x%x] = 0x%x = 0b",reg->address, reg->get_value());

    return name() + string(buff) + string(bits);
  }
  return string("");
}

char *register_symbol::toString(char *return_str, int len)
{
  if(!return_str)
    return 0;

  if(reg)
    return reg->toString(return_str,len);

  *return_str=0;
  return return_str;
}
char *register_symbol::toBitStr(char *return_str, int len)
{
  if(!return_str)
    return 0;

  if(reg)
    return reg->toBitStr(return_str,len);

  *return_str=0;
  return return_str;
}


void  register_symbol::get(int &i)
{
  if(reg)
    i = reg->get_value();
  else
    i = 0;
}
void  register_symbol::get(gint64 &i)
{
  if(reg)
    i = reg->get_value();
  else
    i = 0;
}
void register_symbol::get(char *buffer, int buf_size)
{
  if(buffer) {

    Register *reg = getReg();
    int v = reg ? reg->get_value() : 0;
    snprintf(buffer,buf_size,"%d",v);
  }

}

void register_symbol::get(Packet &p)
{
  if(reg) {
    unsigned int i = reg->get_value();
    p.EncodeUInt32(i);
  }

}


void register_symbol::set(int new_value)
{
  if(reg)
    reg->putRV(RegisterValue(new_value,0));
}

void register_symbol::set(Value *v)
{
  if(reg && v) {
    int i;
    v->get(i);
    reg->putRV(RegisterValue(i,0));
  }
}

void register_symbol::set(const char *buffer, int buf_size)
{
  if(buffer) {

    int i;
    int converted=0;

    // if a straight decimal conversion fails, then try hexadecimal.

    converted = sscanf(buffer, "0x%x",  &i);

    if(!converted)
      converted = sscanf(buffer, "%d",  &i);

    if(!converted)
      converted = sscanf(buffer, "$%x",  &i);

    if(converted)
      set(i);

  }

}

void register_symbol::set(Packet &p)
{
  unsigned int i;
  if(p.DecodeUInt32(i)) {
    set((int)i);
  }

}

symbol *register_symbol::copy()
{
  return new register_symbol((char *)0,reg);
}
Register *register_symbol::getReg()
{
  return reg;
}

//------------------------------------------------------------------------
w_symbol::w_symbol(const char *_name, Register *_reg)
  : register_symbol(_name, _reg)
{
}

//------------------------------------------------------------------------
ioport_symbol::ioport_symbol(IOPORT *_ioport)
  : register_symbol(0, _ioport)
{
}

//------------------------------------------------------------------------
address_symbol::address_symbol(const char *_name, unsigned int _val)
  :  Integer(_val)

{
  new_name(_name);
}

string address_symbol::toString()
{
  char buf[256];
  int i = getVal();
  snprintf(buf,sizeof(buf), " at address %d = 0x%X",i,i);
  
  return name() + string(buf);
}

line_number_symbol::line_number_symbol(const char *_name, unsigned int _val)
  :  address_symbol(_name,_val)
{
  if(!_name) {
    char buf[64];
    snprintf(buf,sizeof(buf), "line_%04x",_val);
    new_name(buf);
  }

}
//------------------------------------------------------------------------
module_symbol::module_symbol(Module *_module, const char *_name)
  : symbol(_name), module(_module)
{
}

symbol *module_symbol::copy()
{
  cout << "copying module symbol: " << name() << endl;

  return new module_symbol(module,name().c_str());
}
void module_symbol::set(const char *cP,int len)
{
  if(module)
    module->set(cP,len);
}

void module_symbol::get(char *cP, int len)
{
  if(module)
    module->get(cP,len);
}

//------------------------------------------------------------------------

attribute_symbol::attribute_symbol(Module *_module, Value *_attribute)
  : module_symbol(_module, 0) , attribute(_attribute)
{
  if(module && attribute) {
    char buf[256];

    snprintf(buf,sizeof(buf),"%s.%s",module->name().c_str(), attribute->name().c_str());
    if(verbose)
      cout << "creating attribute symbol named: " << buf << endl;
    new_name(buf);
    attribute->new_name(buf);
  } 
}

string attribute_symbol::toString()
{

  if(attribute)
    return attribute->showType()+": " + attribute->name() + " = " + attribute->toString();
  else
    return string("(null)");
}

char *attribute_symbol::toString(char *return_str, int len)
{
  if(attribute)
    return attribute->toString(return_str,len);
  else if(return_str)
    *return_str=0;

  return return_str;
}
char *attribute_symbol::toBitStr(char *return_str, int len)
{
  if(attribute)
    return attribute->toBitStr(return_str,len);
  else if(return_str)
    *return_str=0;

  return return_str;
}

string attribute_symbol::description()
{

  if(attribute)
    return attribute->description();
  else
    return string("no attribute");  // <-- this has to be an error
}

void attribute_symbol::set(double d)
{
  if(attribute)
    attribute->set(d);
}
void attribute_symbol::set(gint64 i)
{
  if(attribute)
    attribute->set(i);
}
void attribute_symbol::set(int i)
{
  if(attribute)
    attribute->set(i);
}
void attribute_symbol::set(Value *v)
{
  if(attribute)
    attribute->set(v);
}
void attribute_symbol::set(const char *cp,int len)
{
  if(attribute)
    attribute->set(cp,len);
}
void attribute_symbol::set(Expression *e)
{
  if(attribute)
    attribute->set(e);
}
void attribute_symbol::set(Packet &p)
{
  if(attribute)
    attribute->set(p);
}

void attribute_symbol::get(int &i)
{
  if(attribute)
    attribute->get(i);
}
void attribute_symbol::get(gint64 &i)
{
  if(attribute)
    attribute->get(i);
}
void attribute_symbol::get(double &d)
{
  if(attribute)
    attribute->get(d);
}
void attribute_symbol::get(char *c, int len)
{
  if(attribute)
    attribute->get(c,len);
}

void attribute_symbol::get(Packet &p)
{
  if(attribute)
    attribute->get(p);
}

string module_symbol::toString()
{
  return name();
}

void attribute_symbol::set_xref(Value *v)
{
  if(attribute) {
    attribute->set_xref(v);
  } 

  Value::set_xref(v);
}

Value *attribute_symbol::get_xref()
{

  if(attribute)
    return attribute->get_xref();

  return Value::get_xref();
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
string stimulus_symbol::toString()
{
  if(s)
    s->show();
  return name();
}
//------------------------------------------------------------------------
val_symbol::val_symbol(gpsimValue *v)
  : symbol((char*)0)
{
  if(!v)
    throw string(" val_symbol");

  val = v;
}
string val_symbol::toString()
{
  return val->toString();
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
