/*
   Copyright (C) 1998-2002 T. Scott Dattalo

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

#ifndef __P16F62X_H__
#define __P16F62X_H__

#include "p16x6x.h"

/***************************************************************************
 *
 * Include file for:  P16F627, P16F628
 *
 *
 * The F62x devices are quite a bit different from the other PICs. The class
 * heirarchy is similar to the 16F84.
 * 
 *
 ***************************************************************************/

class CMCON : public sfr_register
{
 public:


  enum CMCON_bits
    {
      CM0 = 1<<0,
      CM1 = 1<<1,
      CM2 = 1<<2,
      CIS = 1<<3,
      C1INV = 1<<4,
      C2INV = 1<<5,
      C1OUT = 1<<6,
      C2OUT = 1<<7,
    };

  bool enabled(void) { return ((value & (CM0|CM1|CM2)) != (CM0|CM1|CM2)); };

  CMCON(void);
};

class VRCON : public sfr_register
{
 public:


  enum VRCON_bits
    {
      VR0 = 1<<0,
      VR1 = 1<<1,
      VR2 = 1<<2,
      VR3 = 1<<3,

      VRR = 1<<5,
      VROE = 1<<6,
      VREN = 1<<7
    };

  VRCON(void);
};

class COMPARATOR_MODULE
{
 public:

  IOPORT *comparator_port;

  CMCON cmcon;
  VRCON vrcon;

  bool enabled(void) { return cmcon.enabled(); };

};


class P16F62x : public P16X6X_processor,  public _14bit_18pins
{
public:

  //INTCON       intcon_reg;
  USART_MODULE14 usart;
  COMPARATOR_MODULE comparator;

  virtual void set_out_of_range_pm(int address, int value);

  virtual PROCESSOR_TYPE isa(void){return _P16F627_;};
  virtual void create_symbols(void);
  virtual unsigned int register_memory_size () const { return 0x200;};

  virtual unsigned int program_memory_size(void) { return 0; };

  virtual void create_sfr_map(void);
  virtual void option_new_bits_6_7(unsigned int bits)
    {
      ((PORTB *)portb)->rbpu_intedg_update(bits);
    }

  // The f628 (at least) I/O pins depend on the Fosc Configuration bits.
  virtual void set_config_word(unsigned int address, unsigned int cfg_word);

#if 0
  virtual unsigned int eeprom_get_size(void) {return eeprom_size;};
  virtual unsigned int eeprom_get_value(unsigned int address) ;
  virtual void eeprom_put_value(unsigned int value,
				unsigned int address);
  virtual file_register *eeprom_get_register(unsigned int address);
#endif

  virtual int get_pin_count(void){return Package::get_pin_count();};
  virtual char *get_pin_name(unsigned int pin_number) {return Package::get_pin_name(pin_number);};
  virtual int get_pin_state(unsigned int pin_number) {return Package::get_pin_state(pin_number);};
  virtual IOPIN *get_pin(unsigned int pin_number) {return Package::get_pin(pin_number);};

  virtual void create(int ram_top);
  virtual void create_iopin_map(void);
};

class P16F627 : public P16F62x
{
public:

  virtual PROCESSOR_TYPE isa(void){return _P16F627_;};

  virtual unsigned int program_memory_size(void) const { return 0x1000; };

  P16F627(void);
  static Processor *construct(void);
};

class P16F628 : public P16F627
{
public:

  virtual PROCESSOR_TYPE isa(void){return _P16F628_;};

  virtual unsigned int program_memory_size(void) const { return 0x2000; };

  P16F628(void);
  static Processor *construct(void);
};

#endif
