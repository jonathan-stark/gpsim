/*
   Copyright (C) 1998-2002 T. Scott Dattalo
   Copyright (C) 2006	   Roy Rankin

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

#ifndef __P16F8X_H__
#define __P16F8X_H__

#include "p16x6x.h"

#include "pir.h"
#include "eeprom.h"
#include "comparator.h"
#include "a2dconverter.h"

/***************************************************************************
 *
 * Include file for:  P16F87, P16F88
 *
 *
 * The F8x devices are similar to 16F62x
 * 
 *
 ***************************************************************************/

class P16F8x : public P16X6X_processor
{
public:

  PIR1v2 pir1_2_reg;
  PIR2v2 pir2_2_reg;
  PIR_SET_2 pir_set_2_def;

  USART_MODULE14 usart;
  COMPARATOR_MODULE comparator;
  WDTCON       wdtcon;
  OSCCON       osccon;
  OSCTUNE      osctune;


  P16F8x(const char *_name=0, const char *desc=0);
  virtual void set_out_of_range_pm(unsigned int address, unsigned int value);

  virtual PROCESSOR_TYPE isa(){return _P16F87_;};
  virtual void create_symbols();
  virtual unsigned int register_memory_size () const { return 0x200;};

  virtual unsigned int program_memory_size() { return 0; };

  virtual void create_sfr_map();

  // The f628 (at least) I/O pins depend on the Fosc Configuration bits.
  virtual bool set_config_word(unsigned int address, unsigned int cfg_word);


  virtual void create();
  virtual void create_iopin_map();
  virtual void create_config_memory();

  virtual void set_eeprom(EEPROM *ep) {
    // Use set_eeprom_pir as P16F8x expects to have a PIR capable EEPROM
    assert(0);
  }

  virtual void set_eeprom_wide(EEPROM_WIDE *ep) {
    eeprom = ep;
  }
  virtual EEPROM_WIDE *get_eeprom() { return ((EEPROM_WIDE *)eeprom); }

  virtual bool hasSSP() { return true;}

  virtual PIR *get_pir1() { return (pir1); }
  virtual PIR *get_pir2() { return (pir2); }
  virtual PIR_SET *get_pir_set() { return (&pir_set_2_def); }
};

class P16F87 : public P16F8x
{
public:

  virtual PROCESSOR_TYPE isa(){return _P16F87_;};

  virtual unsigned int program_memory_size() const { return 0x1000; };

  P16F87(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
};

class P16F88 : public P16F87
{
public:


  ANSEL  ansel;
  ADCON0_withccp adcon0;
  ADCON1 adcon1;
  ADRES  adresh;
  ADRES  adresl;

  virtual PROCESSOR_TYPE isa(){return _P16F88_;};

  virtual unsigned int program_memory_size() const { return 0x1000; };

  virtual void create();
  virtual void create_sfr_map();


  P16F88(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
};


#endif
