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

#ifndef __P12X_H__
#define __P12X_H__

#include "12bit-processors.h"
#include "pic-packages.h"

class GPIO : public IOPORT
{
public:

  unsigned int get(void);
  void setbit(unsigned int bit_number, bool new_value);
};

/*
class OSCCAL : public sfr_register
{

}

*/

class _12bit_8pins : public Package
{
public:

  GPIO         gpio;
  IOPORT_TRIS  tris;
  sfr_register osccal;  // %%% FIX ME %%% Nothing's done with this.

  void create_iopin_map(void);

};


class P12C508 : public  _12bit_processor, public _12bit_8pins
{
  public:

  //  const int PROGRAM_MEMORY_SIZE = 0x200;

  virtual PROCESSOR_TYPE isa(void){return _P12C508_;};
  virtual void create_symbols(void);

  virtual unsigned int program_memory_size(void) const { return 0x200; };
  virtual void create_sfr_map(void);
  virtual void dump_registers(void);
  virtual void tris_instruction(unsigned int tris_register);

  P12C508(void);
  static pic_processor *construct(void);
  void create(void);

  virtual int get_pin_count(void){return Package::get_pin_count();};
  virtual char *get_pin_name(unsigned int pin_number) {return Package::get_pin_name(pin_number);};
  virtual int get_pin_state(unsigned int pin_number) {return Package::get_pin_state(pin_number);};
  virtual IOPIN *get_pin(unsigned int pin_number) {return Package::get_pin(pin_number);};

};


// A 12c509 is like a 12c508
class P12C509 : public P12C508
{
  public:


  //  const int PROGRAM_MEMORY_SIZE = 0x400;

  virtual PROCESSOR_TYPE isa(void){return _P12C509_;};

  virtual unsigned int program_memory_size(void) const { return 0x400; };

  virtual void create_sfr_map(void);


  P12C509(void);
  static pic_processor *construct(void);
  void create(void);


};

#endif //  __P12X_H__
