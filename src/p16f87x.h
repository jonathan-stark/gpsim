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
along with gpsim; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#ifndef __P16F87X_H__
#define __P16F87X_H__

#include "p16x7x.h"

#include "eeprom.h"

class IOPORT;


class P16F873 : public P16C73
{

 public:

 ADRES  adresl;

  virtual void set_out_of_range_pm(int address, int value);

  virtual PROCESSOR_TYPE isa(void){return _P16F873_;};
  virtual unsigned int program_memory_size(void) const { return 0x1000; };
  virtual void create_symbols(void);
  void create_sfr_map(void);
  void create(void);
  virtual unsigned int register_memory_size () const { return 0x200;};

  P16F873(void);

  virtual void set_eeprom(EEPROM *ep) {
    // use set_eeprom_wide as P16F873 expect a wide EEPROM
    assert(0);
  }
  virtual void set_eeprom_wide(EEPROM_WIDE *ep) {
    eeprom = ep;
  }
  virtual EEPROM_WIDE *get_eeprom(void) { return ((EEPROM_WIDE *)eeprom); }
  static Processor *construct(void);

};




class P16F874 : public P16C74
{
 public:
  ADRES  adresl;

  virtual void set_out_of_range_pm(int address, int value);

  virtual PROCESSOR_TYPE isa(void){return _P16F874_;};
  virtual unsigned int program_memory_size(void) const { return 0x1000; };
  virtual void create_symbols(void);
  void create_sfr_map(void);
  void create(void);
  virtual unsigned int register_memory_size () const { return 0x200;};

  P16F874(void);
  static Processor *construct(void);

  virtual void set_eeprom(EEPROM *ep) {
    // use set_eeprom_wide as P16F873 expect a wide EEPROM
    assert(0);
  }
  virtual void set_eeprom_wide(EEPROM_WIDE *ep) {
    eeprom = ep;
  }
  virtual EEPROM_WIDE *get_eeprom(void) { return ((EEPROM_WIDE *)eeprom); }
};

class P16F877 : public P16F874
{
 public:
  virtual PROCESSOR_TYPE isa(void){return _P16F877_;};
  virtual unsigned int program_memory_size(void) const { return 0x2000; };
  virtual void create_symbols(void);
  void create_sfr_map(void);
  void create(void);

  P16F877(void);
  static Processor *construct(void);
};

#endif
