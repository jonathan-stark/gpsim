
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

#ifndef __PIC_REGISTERS_H__
#define __PIC_REGISTERS_H__

class pic_processor;
class symbol;
class XrefObject;

#include "gpsim_classes.h"
#include "registers.h"

#include "breakpoints.h"


//------------------------------------------------------------
class file_register : public Register
{
 public:

  file_register(void);
  ~file_register(void);
  virtual void put_value(unsigned int new_value);

/*
  virtual void put(unsigned int new_value);
  virtual void new_name(char *);
  virtual void setbit(unsigned int bit_number, bool new_value);
  virtual void setbit_value(unsigned int bit_number, bool new_value);
  virtual int get_bit(unsigned int bit_number);
  virtual int get_bit_voltage(unsigned int bit_number);
*/
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
