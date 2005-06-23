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
#include <algorithm>

#include "../config.h"
#include "14bit-processors.h"
#include "stimuli.h"
#include "symbol_orb.h"
#include "expr.h"
#include "operator.h"
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

#if defined(_WIN32)
Symbol_Table &get_symbol_table(void) {
  return symbol_table;
}
#endif

Symbol_Table::Symbol_Table() {
    reserve(500);
}

void Symbol_Table::add_ioport(IOPORT *_ioport)
{

  ioport_symbol *is = new ioport_symbol(_ioport);

  add(is);

}

void Symbol_Table::add_stimulus_node(Stimulus_Node *s)
{

  node_symbol *ns = new node_symbol(s);

  add(ns);

}

void Symbol_Table::add_stimulus(stimulus *s)
{

  stimulus_symbol *ss = new stimulus_symbol(s);

  add(ss);

}

bool Symbol_Table::add(Value *s) {
  if(s) {
    iterator it = lower_bound(begin( ), end( ),
      s, NameLessThan());
    if (it != end() &&
      (*it)->name() == s->name()) {
        printf("Symbol_Table::add(): Warning: previous symbol %s overwritten\n", s->name().c_str());
        erase(it);
//      return false;
    }
    insert(it, s);
    return true;
  }
  return false;
}

register_symbol *
Symbol_Table::add_register(Register *new_reg, const char *symbol_name)
{
  // mask of zero lets the register_symbol calculate a default mask
  return add_register(new_reg, symbol_name, 0);
}

register_symbol *
Symbol_Table::add_register(Register *new_reg, const char *symbol_name,
                           unsigned int uMask)
{

  if(!new_reg)
    return 0;

  if(symbol_name) {
    string sName(symbol_name);
    if((new_reg->name() == sName ||
       new_reg->baseName() == sName) &&  
      (find(new_reg->name()) || find(new_reg->baseName()))) {
      if(verbose)
        cout << "Warning not adding  "
	           << symbol_name
	           << " to symbol table\n because it is already in.\n";
      return 0;
     }
  }

  register_symbol *rs = new register_symbol(symbol_name, new_reg, uMask);

  add(rs);
  return rs;
}

void Symbol_Table::add_w(WREG *new_w)
{

  if(!new_w)
    return;

  w_symbol *ws = new w_symbol((char *)0, new_w);

  add(ws);

}

void Symbol_Table::add_constant(const char *_name, int value)
{

  Integer *i = new Integer(value);
  i->new_name(_name);

  add(i);

}

void Symbol_Table::add_address(const char *new_name, int value)
{

  address_symbol *as = new address_symbol(new_name,value);

  add(as);

}

void Symbol_Table::add_line_number(int address, const char *symbol_name)
{

  line_number_symbol *lns = new line_number_symbol(symbol_name,  address);

  add(lns);
}

void Symbol_Table::add_module(Module * m, const char *cPname)
{
  module_symbol *ms = new module_symbol(m,cPname);

  add(ms);

}

void Symbol_Table::remove_module(Module * m) {
  iterator sti = FindIt(m->name());
  Value *sym;

  while( sti != end()) {
    sym = *sti;
    if((typeid(sym) == typeid(module_symbol)) &&
       sym->name() == m->name()) {
      erase(sti);
      return;
    }
    sti++;
  }
}

Value *Symbol_Table::remove(string &s)
{
  iterator it = FindIt(s);
  if(it != end() && (*it)->name() == s) {
    erase(it);
    return *it;
  }
  return NULL;
}

void Symbol_Table::rename(const char *pOldName, const char *pNewName)
{
  // First make sure the old and new names are both valid.
  if (pNewName && pOldName && *pOldName && *pNewName) {
    iterator it = FindIt(pOldName);
    if(it != end()) {
      Value *pValue = *it;
      erase(it);
      pValue->gpsimObject::new_name(pNewName);
      add(pValue);
    }
  }
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
  string s(str);
  return(find(s));
}

Value * Symbol_Table::find(type_info const &symt, const char *str)
{
  string s(str);
  iterator sti = FindIt(str);
  while( sti != end()) {
    Value *val = *sti;
    if(val && (val->name() == s) && (typeid(val) == symt))
      return(val);
    sti++;
  }
  return 0;
}

Register * Symbol_Table::findRegister(unsigned int address)
{
  iterator sti = begin();
  while( sti != end()) {
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
  iterator sti = FindIt(s); // .begin();
  while( sti != end()) {
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
    cout << val->name() << " = " << val->toString() << endl;
}

void Symbol_Table::dump_one(const char *str)
{
  string s =  string(str);
  dump_one(&s);
}


void Symbol_Table::dump_all(void)
{
  cout << "  Symbol Table\n";
  iterator sti = begin();
  iterator last;
  while( sti != end()) {
    Value *val = *sti;
    if(val &&(typeid(*val) != typeid(line_number_symbol)))
      cout << val->name() << " = " << val->toString() << endl;
      //cout << val->name() << ": " << val->showType() << endl;
    last = sti;
    sti++;
    if(sti != end() && (*last)->name() == (*sti)->name()) {
      cout << "***************** Duplicate Found ***********" << endl;
    }
  }
}

void Symbol_Table::dump_type(type_info const &symt)
{
  // Now loop through the whole table and display all instances of the type of interest
  int first=1;     // On the first encounter of one, display the title
  iterator sti = begin();
  while( sti != end()) {
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
  iterator it;
  iterator itEnd = end();
  int i = 0;
  for(it = begin(); it != itEnd;) {
    Value *value = *it;
    if(value->isClearable()) {
      delete value;
      erase(it);
    }
    else {
      it++;
    }
    i++;
  }
//  remove_if(begin(), end(), IsClearable);
}

void Symbol_Table::clear_all() {
  iterator it;
  iterator itEnd = end();
  for(it = begin(); it != itEnd; it++) {
    Value *value = *it;
    delete *it;
  }
  _Myt::clear();
}

Value * Symbol_Table::find(string &s)
{
  const bool findDuplicates=false;
  iterator sti = FindIt(s); // .begin();

  Value *ret=0;
  while( sti != end()) {
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


Symbol_Table::iterator
Symbol_Table::FindIt(const char *pszKey) {
  Value KeyValue(pszKey, "key value");
  return FindIt(&KeyValue);
}

Symbol_Table::iterator
Symbol_Table::FindIt(string &sKey) {
  Value KeyValue(sKey.c_str(), "key value");
  return FindIt(&KeyValue);
}

Symbol_Table::iterator
Symbol_Table::FindIt(Value *key) {
  iterator it = lower_bound(begin( ), end( ),
    key, NameLessThan());
  if (it != end() &&
    (*it)->name() == key->name()) {
    return it;
  }
  return end();
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

// Count LSB that are off
static int BitShiftCount( unsigned int uMask ) {
  unsigned int uBitMaskCount = 0;
  if( uMask != 0 ) {
    for ( int i = 0; i < 16; i++ ) {
      if( (uMask & (0x1 << i)) == 0 )
        uBitMaskCount++;
      else
        break;
    }
  }
  return uBitMaskCount;
}

//------------------------------------------------------------------------
// register_symbol
//
register_symbol::register_symbol(const register_symbol & regsym)
  : symbol(regsym.name_str.c_str()), reg(regsym.reg)
{
  m_uMask = regsym.m_uMask;
  m_uMaskShift = regsym.m_uMaskShift;

  if (name_str.empty()) {
    name_str = regsym.reg->name();
  }
}

register_symbol::register_symbol(const char *_name, Register *_reg)
  : symbol(_name), reg(_reg)
{
  setMask(reg);
  if (_name == NULL && reg != NULL) {
    name_str = _reg->name();
  }
}

register_symbol::register_symbol(const char *_name, Register *_reg,
                                 unsigned int uMask)
  : symbol(_name), reg(_reg)
{
  if(uMask == 0) {
    setMask(reg);
  }
  else {
    m_uMask = uMask;
    m_uMaskShift = BitShiftCount(m_uMask);
  }
  if (_name == NULL && reg != NULL) {
    name_str = _reg->name();
  }
}

register_symbol::register_symbol(Register *_reg)
  : symbol(_reg->name()), reg(_reg)
{
  setMask(reg);
}

void register_symbol::setMask(Register *pReg) {
  m_uMask = 0xff;
  for(unsigned int i = 1; i < pReg->register_size(); i++) {
    m_uMask <<= 8;
    m_uMask |= 0xff;
  }
  m_uMaskShift = BitShiftCount(m_uMask);
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
    Processor *pProc = reg->get_cpu();

    reg->toBitStr(bits,sizeof(bits));

    int iDigits = pProc->register_size() * 2;
    // turn off masked bits in dwRead
    unsigned int uValue = reg->get_value() & m_uMask;
    uValue  = uValue >> m_uMaskShift;
    snprintf(buff,sizeof(buff)," [0x%x] BITS 0x%0*x = 0x%0*x = 0b",
      reg->address, iDigits, m_uMask,
      iDigits, uValue);

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
  if(reg) {
    i = reg->get_value() & m_uMask;
    i >>= m_uMaskShift;
  }
  else
    i = 0;
}
void  register_symbol::get(gint64 &i)
{
  if(reg) {
    i = reg->get_value() & m_uMask;
    i >>= m_uMaskShift;
  }
  else
    i = 0;
}
void register_symbol::get(char *buffer, int buf_size)
{
  if(buffer) {
    int v;
    get(v);
    snprintf(buffer,buf_size,"%d",v);
  }

}

void register_symbol::get(Packet &p)
{
  if(reg) {
    int i;
    get(i);
    p.EncodeUInt32(i);
  }

}


void register_symbol::set(int new_value)
{
  if(reg)
    reg->putRV(RegisterValue(SetMaskedValue(new_value),0));
}

void register_symbol::set(Value *v)
{
  if(reg && v) {
    int i;
    v->get(i);
    reg->putRV(RegisterValue(SetMaskedValue(i),0));
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

unsigned int register_symbol::SetMaskedValue(unsigned int uValue) {
  
  Register *reg = getReg();
  unsigned int uCurrentValue = reg ? reg->get_value() & m_uMask: 0;
  // turn off masked bits in uCurrentValue
  uCurrentValue &= ~m_uMask;
  // turn off non-mask bits in uValue
  uValue = uValue << m_uMaskShift;
  uValue &= m_uMask;
  // map the passed in value into the target
  uCurrentValue |= ((long)uValue);
  return uCurrentValue;
}

void register_symbol::set(Packet &p)
{
  unsigned int i;
  if(p.DecodeUInt32(i)) {
    set((int)i);
  }

}

bool register_symbol::compare(ComparisonOperator *compOp, Value *rvalue)
{
  if(!compOp || !rvalue)
    return false;

  gint64 i,r;

  get(i);
  rvalue->get(r);

  if(i < r)
    return compOp->less();

  if(i > r)
    return compOp->greater();

  return compOp->equal();
}

symbol *register_symbol::copy()
{
  return new register_symbol(*this);
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
  int i = (int)getVal();
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

void val_symbol::get(gint64 &i)
{
  if (val)
    i = val->get_value();
  else
    i = 0;
}
void val_symbol::set(int new_value)
{
  if(val)
    val->put_value(new_value);
}
void val_symbol::set(gint64 new_value)
{
  if(val) 
    val->put_value((int)new_value);
}
string &val_symbol::name(void)
{
  return val->name();
}
symbol *val_symbol::copy()
{
  return new val_symbol(val);
}
bool val_symbol::compare(ComparisonOperator *compOp, Value *rvalue)
{
  if(!compOp || !rvalue)
    return false;

  gint64 i,r;

  get(i);
  rvalue->get(r);

  if(i < r)
    return compOp->less();

  if(i > r)
    return compOp->greater();

  return compOp->equal();
}
