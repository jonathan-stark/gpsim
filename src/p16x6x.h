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

#ifndef __P16X6X_H__
#define __P16X6X_H__

#include "14bit-processors.h"
#include "14bit-tmrs.h"


class INTCON_P16C6x : public INTCON
{
public:

  PIR1 *pir1;
  PIR2 *pir2;

  bool check_peripheral_interrupt(void)
    {
      if(pir2)
	return( pir1->interrupt_status() | pir2->interrupt_status());
      else
	return( pir1->interrupt_status());
    }
 INTCON_P16C6x(void)
   {
     pir2=NULL;
   }
};

class P16C61 : public  _14bit_processor, public _14bit_18pins
{
public:

  //  const int PROGRAM_MEMORY_SIZE = 0x400;

  virtual PROCESSOR_TYPE isa(void){return _P16C61_;};
  virtual void create_symbols(void);

  virtual unsigned int program_memory_size(void) const { return 0x400; };

  virtual void create_sfr_map(void);

  virtual void option_new_bits_6_7(unsigned int bits)
    {
      ((PORTB *)portb)->rbpu_intedg_update(bits);
    }

  P16C61(void);

  virtual int get_pin_count(void){return Package::get_pin_count();};
  virtual char *get_pin_name(unsigned int pin_number) {return Package::get_pin_name(pin_number);};
  virtual int get_pin_state(unsigned int pin_number) {return Package::get_pin_state(pin_number);};
  virtual IOPIN *get_pin(unsigned int pin_number) {return Package::get_pin(pin_number);};

};

class _14bit_40pins : public _40pins
{
public:

  T1CON   t1con;
  PIR1    pir1;
  PIE     pie1;
  PIR2    pir2;
  PIE     pie2;
  T2CON   t2con;
  PR2     pr2;
  TMR2    tmr2;
  TMR1L   tmr1l;
  TMR1H   tmr1h;
  CCPCON  ccp1con;
  CCPRL   ccpr1l;
  CCPRH   ccpr1h;
  CCPCON  ccp2con;
  CCPRL   ccpr2l;
  CCPRH   ccpr2h;
  INTCON_P16C6x intcon_reg;

  void create_iopin_map(void);

};

class P16C64 : public  _14bit_processor, public _14bit_40pins
{
  public:


  //  const int PROGRAM_MEMORY_SIZE = 0x800;

  TMR2_MODULE tmr2_module;

  virtual PROCESSOR_TYPE isa(void){return _P16C64_;};
  virtual void create_symbols(void);

  virtual unsigned int program_memory_size(void) const { return 0x800; };

  virtual void create_sfr_map(void);

  virtual void option_new_bits_6_7(unsigned int bits)
    {
      ((PORTB *)portb)->rbpu_intedg_update(bits);
    }

  P16C64(void);


  virtual int get_pin_count(void){return Package::get_pin_count();};
  virtual char *get_pin_name(unsigned int pin_number) {return Package::get_pin_name(pin_number);};
  virtual int get_pin_state(unsigned int pin_number) {return Package::get_pin_state(pin_number);};
  virtual IOPIN *get_pin(unsigned int pin_number) {return Package::get_pin(pin_number);};
};

class P16C65 : public  P16C64
{
  public:


  //  const int PROGRAM_MEMORY_SIZE = 0x800;


  virtual PROCESSOR_TYPE isa(void){return _P16C65_;};
  virtual void create_symbols(void);

  unsigned int program_memory_size(void) const { return 0x800; };

  virtual void create_sfr_map(void);


  P16C65(void);


};


#endif
