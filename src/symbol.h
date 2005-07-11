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

//
// symbol.h
//

#include <string>
#include <vector>
#include "value.h"

using namespace std;

#ifndef __SYMBOL_H__
#define __SYMBOL_H__



class stimulus;
class Stimulus_Node;
class WREG;
class IOPORT;
class Processor;
class Register;
class Module;
class Expression;
class symbol;
class register_symbol;

void display_symbol_file_error(int);


enum SYMBOL_TYPE
{
  SYMBOL_INVALID,
  SYMBOL_BASE_CLASS,
  SYMBOL_IOPORT,
  SYMBOL_STIMULUS_NODE,
  SYMBOL_STIMULUS,
  SYMBOL_LINE_NUMBER,
  SYMBOL_CONSTANT,
  SYMBOL_REGISTER,
  SYMBOL_ADDRESS,
  SYMBOL_SPECIAL_REGISTER,   // like W
  SYMBOL_PROCESSOR,
  SYMBOL_MODULE
};


class Symbol_Table : private vector<Value*> {
private:
  struct NameLessThan : binary_function<Value*, Value*, bool> {
    bool operator()(const Value* left, const Value* right) const {
      return left->name() < right->name();
    }
  };
  iterator FindIt(const char *s);
  iterator FindIt(string &s);
  iterator FindIt(Value *key);

public:
  Symbol_Table();
#ifndef _MSC_VER
   typedef vector<Value*> _Myt;
#endif
  typedef _Myt::iterator iterator;

  bool add(Value*);

  void add_ioport(IOPORT *ioport);
  void add_stimulus_node(Stimulus_Node *stimulus_node);
  void add_stimulus(stimulus *s);
  void add_line_number(int address, const char *symbol_name=0);
  void add_constant(const char *, int );
  register_symbol* add_register(Register *reg, const char *symbol_name=0);
  register_symbol* add_register(Register *new_reg, const char *symbol_name,
                                unsigned int uMask );
  void add_address(const char *, int );
  void add_w(WREG *w );
  void add_module(Module * m, const char *module_name);
  void remove_module(Module * m);
  void add(const char *symbol_name, const char *symbol_type, int value);
  Value *remove(string &);
  void rename(const char *pOldName, const char *pNewName);
  void dump_all(void);
  void dump_one(const char *s);
  void dump_one(string *s);
  void dump_type(type_info const&t);

  Value *find(const char *s);
  Value *find(string &s);
  Value *find(type_info const&t, const char *s);
  Register * findRegister(unsigned int address);
  Register * findRegister(const char *s);
  register_symbol * findRegisterSymbol(unsigned int uAddress);
  register_symbol * findRegisterSymbol(unsigned int uAddress,
                                       unsigned int uBitmask);
  const char * findProgramAddressLabel(unsigned int address);
  // This was intended to be used by disassembly code. I added
  // this then I figured out that it is not useful unless
  // you have a cross reference to every address that references
  // the symbol. So I'm leaving the code for the future when
  // the constant object include a list of address reference.
  const char * findConstant(unsigned int uValue, unsigned int uReferencedFromAddress);
  bool  Exist(const char *);
  void clear();
  void clear_all();
  iterator begin() {
    return _Myt::begin();
  }
  iterator end() {
    return _Myt::end();
  }
};



#if defined(_WIN32)
  #if !defined(IN_MODULE) 
  extern Symbol_Table symbol_table;
  #endif
// we are in Windows: don't access symbol_table object directly!
extern "C" Symbol_Table &get_symbol_table(void);
#else
// we are in gpsim: use of get_symbol_table() is recommended,
// even if trace object can be accessed directly.
extern Symbol_Table symbol_table;
inline Symbol_Table &get_symbol_table(void) {
  return symbol_table;
}
#endif




//------------------------------------------------------------------------
/// symbol - base class for gpsim symbols. gpsim symbols are 'Value' objects
/// that are typically associated with a Typed object that has been defined
/// by a Module. Eg. a register_symbol is a symbol that is associtated with
/// a processor's registers.

class symbol : public Value
{
public:

  virtual string toString();

  symbol(const char *);
  symbol(string &);
  virtual ~symbol();
};


class node_symbol : public symbol
{
protected:
  Stimulus_Node *stimulus_node;
public:

  node_symbol(Stimulus_Node *);
  virtual string toString();
};

class register_symbol : public symbol
{
protected:
  Register *reg;
  /** m_uMask is used to mask out bits not associated
    * with the symbol when retrieving the value from the
    * register. Masked bit values are right shifted such
    * that if the 16th bit is high in the register and
    * m_uMask = 0x80, then the symbol value returned
    * or displayed will be 0x01.
    * Likewise to set the 16th bit of our example register_symbol
    * you would set the register_symbol value to 0x01 and
    * not 0x80. No other bits in the register are changed.
    * It works as if this symbol is its own register. The 
    * encoding into the target register is hidden by register_symbol.
    * The default value of m_uMask is 0xff so that no bits are
    * masked out.
    * A new version of Symbol_Table::add_register() may be used
    * to add masked register_symbol objects to the symbol table.
    */
  unsigned int m_uMask;
  unsigned int m_uMaskShift;

  unsigned int SetMaskedValue(unsigned int uValue);

public:
  register_symbol(const register_symbol &);
  register_symbol(const char *, Register *);
  register_symbol(const char *, Register *, unsigned int uMask);
  register_symbol(Register *_reg);

  virtual symbol *copy();
  virtual string &name(void) const;
  virtual char *name(char *, int len);
  virtual string toString();

  virtual void get(int &);
  virtual void get(gint64 &);
  virtual void get(char *, int len);
  virtual void get(Packet &);

  virtual void set(int);
  virtual void set(Value *);
  virtual void set(const char *cP,int len=0);
  virtual void set(Packet &);

  void setMask(Register *pReg);
  unsigned int getAddress(void);
  unsigned int getBitmask(void);

  virtual bool compare(ComparisonOperator *compOp, Value *rvalue);

  Register *getReg();
  /// copy the object value to a user char array
  virtual char *toString(char *, int len);
  virtual char *toBitStr(char *, int len);

};

class ioport_symbol : public register_symbol
{
public:
  ioport_symbol(IOPORT *);
};

class stimulus_symbol : public symbol
{
protected:
  stimulus *s;
public:
  stimulus_symbol(stimulus *);
  virtual string &name(void) const;
  virtual char *name(char *, int len);
  virtual string toString();
  virtual void new_name(const char *);
  virtual void new_name(string &);

};

class address_symbol : public Integer
{
public:

  address_symbol(const char *, unsigned int);
  virtual string toString();
};

class line_number_symbol : public address_symbol
{
protected:
  int src_id,src_line,lst_id,lst_line,lst_page;
 public:

  line_number_symbol(const char *, unsigned int);
  void put_address(int new_address) {set(new_address);}
  void put_src_line(int new_src_line) {src_line = new_src_line;}
  void put_lst_line(int new_lst_line) {lst_line = new_lst_line;}
  void put_lst_page(int new_lst_page) {lst_page = new_lst_page;}
};

/// module_symbol - a symbol table entry for a gpsim module.
class module_symbol : public symbol
{
protected:
  Module *module;
public:
  module_symbol(Module *, const char *);
  virtual string toString();
  Module *get_module() { return module;}
  virtual symbol *copy();

  /// The set and get methods for the module_symbol take char strings
  /// as their inputs. These strings are dynamically decoded such
  /// that various internal states of the processor can be controlled.

  virtual void set(const char *cP,int len=0);
  virtual void get(char *, int len);

};

/// attribute_symbol - a symbol that is associated with a specific
/// instance of a module. This class provides a mechanism for wrapping
/// Values (or symbols) and associating them with processors or
/// modules.

class attribute_symbol : public module_symbol
{
protected:
  /// The attribute wrapped by this symbol.
  Value *attribute;
public:
  attribute_symbol(Module *, Value *);
  virtual string toString();
  virtual string description();

  /// The get and set methods will call the attribute's get and
  /// set methods.

  virtual void set(double);
  virtual void set(gint64);
  virtual void set(int);
  virtual void set(Value *);
  virtual void set(Expression *);
  virtual void set(const char *cP,int len=0);
  virtual void set(Packet &);

  virtual void get(int &);
  virtual void get(gint64 &);
  virtual void get(double &);
  virtual void get(char *, int len);
  virtual void get(Packet &);

  /// copy the object value to a user char array
  virtual char *toString(char *, int len);
  virtual char *toBitStr(char *, int len);

  virtual void set_xref(Value *);
  virtual Value *get_xref();

};

// Place W into the symbol table
class w_symbol : public register_symbol
{
 public:
  w_symbol(const char*, Register *);
};


class val_symbol : public symbol
{
 public:

  val_symbol(gpsimValue *);

  gpsimValue *val;

  virtual string toString();

  virtual void get(int &);
  virtual void get(gint64 &);
  virtual void set(int);
  virtual void set(gint64);
  virtual symbol *copy();
  virtual bool compare(ComparisonOperator *compOp, Value *rvalue);

  virtual string &name(void) const;

};

#endif  //  __SYMBOL_H__
