/*
   Copyright (C) 2000,2001 T. Scott Dattalo, Daniel Schudel, Robert Pearce

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
// p16x5x
//
//  This file supports:
//    P16C54
//    P16C55

#ifndef __P16X5X_H__
#define __P16X5X_H__

#include "12bit-processors.h"
#include "pic-packages.h"




class _12bit_18pins : public _18pins
{
public:

  void create_iopin_map(void);

};


class _12bit_28pins : public _28pins
{
public:

  void create_iopin_map(void);

};



class P16C54 : public  _12bit_processor, public _12bit_18pins
{
public:

  //  const int PROGRAM_MEMORY_SIZE = 0x400;

  virtual PROCESSOR_TYPE isa(void){return _P16C54_;};
  virtual void create_symbols(void);

  virtual unsigned int program_memory_size(void) const { return 0x200; };
  virtual unsigned int register_memory_size(void) const { return 0x20; };
  virtual unsigned int config_word_address(void) const {return 0xFFF;};

  virtual void create_sfr_map(void);

  virtual void option_new_bits_6_7(unsigned int bits)
    {
      // ((PORTB *)portb)->rbpu_intedg_update(bits);
    }

  P16C54(void);
  void create(void);

  virtual int get_pin_count(void){return Package::get_pin_count();};
  virtual char *get_pin_name(unsigned int pin_number) {return Package::get_pin_name(pin_number);};
  virtual int get_pin_state(unsigned int pin_number) {return Package::get_pin_state(pin_number);};
  virtual IOPIN *get_pin(unsigned int pin_number) {return Package::get_pin(pin_number);};
  static Processor *construct(void);
  virtual void tris_instruction(unsigned int tris_register);
  virtual unsigned int get_fsr_value ( unsigned int load_value )
    {
      return ( load_value | 0xE0 );
    }

};

class P16C55 : public  _12bit_processor, public _12bit_28pins
{
public:

  //  const int PROGRAM_MEMORY_SIZE = 0x400;

  virtual PROCESSOR_TYPE isa(void){return _P16C55_;};
  virtual void create_symbols(void);

  virtual unsigned int program_memory_size(void) const { return 0x200; };
  virtual unsigned int register_memory_size(void) const { return 0x20; };
  virtual unsigned int config_word_address(void) const {return 0xFFF;};

  virtual void create_sfr_map(void);

  virtual void option_new_bits_6_7(unsigned int bits)
    {
      // ((PORTB *)portb)->rbpu_intedg_update(bits);
    }

  P16C55(void);
  void create(void);

  virtual int get_pin_count(void){return Package::get_pin_count();};
  virtual char *get_pin_name(unsigned int pin_number) {return Package::get_pin_name(pin_number);};
  virtual int get_pin_state(unsigned int pin_number) {return Package::get_pin_state(pin_number);};
  virtual IOPIN *get_pin(unsigned int pin_number) {return Package::get_pin(pin_number);};
  static Processor *construct(void);
  virtual void tris_instruction(unsigned int tris_register);

  virtual unsigned int get_fsr_value ( unsigned int load_value )
    {
      return ( load_value | 0xE0 );
    }

};

#endif
