
/*
   Copyright (C) 1998-2000 T. Scott Dattalo

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

#ifndef __REGISTERS_H__
#define __REGISTERS_H__

class pic_processor;
class symbol;
class XrefObject;

#include "gpsim_classes.h"
#include "breakpoints.h"
#include "trace.h"


//---------------------------------------------------------
// Base class for a file register.
class Register
{
public:

  enum REGISTER_TYPES
  {
    INVALID_REGISTER,
    GENERIC_REGISTER,
    FILE_REGISTER,
    SFR_REGISTER,
    BP_REGISTER
  };

  char *name_str1;
  unsigned int value,address,break_point;

  // If non-zero, the alias_mask describes all address at which
  // this file register appears. The assumption (that is true so
  // far for all pic architectures) is that the aliased register
  // locations differ by one bit. For example, the status register
  // appears at addresses 0x03 and 0x83 in the 14-bit core. 
  // Consequently, alias_mask = 0x80 and address (above) is equal
  // to 0x03.

  unsigned int alias_mask;

  unsigned int por_value;  // power on reset value

  unsigned int bit_mask;   // = 7 for 8-bit registers, = 15 for 16-bit registers.

  symbol *symbol_alias;
  Processor *cpu;

  guint64 read_access_count;
  guint64 write_access_count;


  // If we are linking with a gui, then here are a
  // few declarations that are used to send data to it.
  // This is essentially a singly-linked list of pointers
  // to structures. The structures provide information
  // such as where the data is located, the type of window
  // it's in, and also the way in which the data is presented
  
  XrefObject *xref;


  Register(void);
  ~Register(void);


  // Register access functions
  virtual unsigned int get(void);
  virtual void put(unsigned int new_value);

  

  /* put_value is the same as put(), but some extra stuff like
   * interfacing to the gui is done. (It's more efficient than
   * burdening the run time performance with (unnecessary) gui
   * crap.)
   */

  virtual void put_value(unsigned int new_value);

  /* same as get(), but no trace is performed */
  virtual unsigned int get_value(void) { return(value); }


  virtual char *name(void) { return(name_str1);};
  virtual void new_name(char *);
  virtual REGISTER_TYPES isa(void) {return GENERIC_REGISTER;};
  virtual void reset(RESET_TYPE r) { return; };

  /* 
     setbit functions are not really intended for general purpose
     registers (although they can be). Instead, they provide place
     holders which are over-ridden by IO ports. The purpose is to
     provide an abstract way in which break points can be set
     on individual IO pin changes.
  */
  virtual void setbit(unsigned int bit_number, bool new_value);
  virtual void setbit_value(unsigned int bit_number, bool new_value);

  /*
    like setbit, getbit is used mainly for breakpoints.
   */
  virtual int get_bit(unsigned int bit_number);
  virtual int get_bit_voltage(unsigned int bit_number);
};

//------------------------------------------------------------
class file_register : public Register
{
 public:

  file_register(void);
  ~file_register(void);

  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  virtual char *name(void) { return(name_str1);};
  virtual void new_name(char *);
  virtual REGISTER_TYPES isa(void) {return GENERIC_REGISTER;};
  virtual void reset(RESET_TYPE r) { return; };
  virtual void setbit(unsigned int bit_number, bool new_value);
  virtual void setbit_value(unsigned int bit_number, bool new_value);
  virtual int get_bit(unsigned int bit_number);
  virtual int get_bit_voltage(unsigned int bit_number);

};
//---------------------------------------------------------
// define a special 'invalid' register class. Accessess to
// to this class' value get 0

class invalid_file_register : public file_register
{
public:

  void put(unsigned int new_value);
  unsigned int get(void);
  invalid_file_register(unsigned int at_address);
  virtual REGISTER_TYPES isa(void) {return INVALID_REGISTER;};
};


//---------------------------------------------------------
// Base class for a special function register.
class sfr_register : public file_register
{
public:
  sfr_register() :file_register(){}
  unsigned int wdtr_value; // wdt or mclr reset value

  virtual REGISTER_TYPES isa(void) {return SFR_REGISTER;};
  virtual void initialize(void) {return;};

  virtual void reset(RESET_TYPE r) {
    switch (r) {

    case POR_RESET:
      value = por_value;
      break;

    case WDT_RESET:
      value = wdtr_value;
      break;
    case SOFT_RESET:
      break;
    }

  }
};


class SFR_map                   // Special Function Register (e.g. status)
{
public:
  int address;			// Base address of the register

  sfr_register *sfr_reg;        // a pointer to it

  int por_value;
};





#endif
