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

#ifndef __P16X8X_H__
#define __P16X8X_H__

#include "14bit-processors.h"

/***************************************************************************
 *
 * Include file for:  P16C84, P16F84, P16F83, P16CR83, P16CR84
 *
 * The x84 processors have a 14-bit core, eeprom, and are in an 18-pin
 * package. The class taxonomy is:
 *
 *   pic_processor                  Package
 *      |-> 14bit_processor           |-> _18_pins
 *             |                            |-> _14bit_18pins
 *             |----------\ /-----------------------|
 *                         |
 *                         |- P16C8x
 *                              |->P16C84
 *                              |->P16F84
 *                              |->P16C83
 *                              |->P16CR83
 *                              |->P16CR84
 *
 ***************************************************************************/

class P16C8x : public  _14bit_processor, public _14bit_18pins
{
public:

  virtual void set_out_of_range_pm(int address, int value);

  virtual PROCESSOR_TYPE isa(void){return _P16C84_;};
  virtual void create_symbols(void);

  virtual unsigned int program_memory_size(void) { return 0; };

  virtual void create_sfr_map(void);
  virtual void option_new_bits_6_7(unsigned int bits)
    {
      ((PORTB *)portb)->rbpu_intedg_update(bits);
    }

  virtual unsigned int eeprom_get_size(void) {return eeprom_size;};
  virtual unsigned int eeprom_get_value(unsigned int address) ;
  virtual void eeprom_put_value(unsigned int value,
				unsigned int address);

  virtual int get_pin_count(void){return Package::get_pin_count();};
  virtual char *get_pin_name(unsigned int pin_number) {return Package::get_pin_name(pin_number);};
  virtual int get_pin_state(unsigned int pin_number) {return Package::get_pin_state(pin_number);};

};

class P16C84 : public P16C8x
{
public:


  virtual PROCESSOR_TYPE isa(void){return _P16C84_;};

  virtual unsigned int program_memory_size(void) const { return 0x400; };

  P16C84(void);

};

class P16F84 : public P16C8x
{
public:

  virtual PROCESSOR_TYPE isa(void){return _P16F84_;};

  virtual unsigned int program_memory_size(void) const { return 0x400; };

  P16F84(void);

};

class P16CR84 : public P16F84
{
public:

  virtual PROCESSOR_TYPE isa(void){return _P16CR84_;};

  P16CR84(void) {  name_str = "p16cr84"; };

};



class P16F83 : public P16C8x
{
public:

  virtual PROCESSOR_TYPE isa(void){return _P16F83_;};

  virtual unsigned int program_memory_size(void) const { return 0x200; };

  P16F83(void);

};

class P16CR83 : public P16F83
{
public:

  virtual PROCESSOR_TYPE isa(void){return _P16CR83_;};

  P16CR83(void) {  name_str = "p16cr83"; };

};


#endif
