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

#if 0
  virtual unsigned int eeprom_get_size(void) {return eeprom_size;};
  virtual unsigned int eeprom_get_value(unsigned int address) ;
  virtual void eeprom_put_value(unsigned int value,
				unsigned int address);
  virtual file_register *eeprom_get_register(unsigned int address);
#endif

  P16F873(void);
  static pic_processor *construct(void);

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
#if 0
  virtual unsigned int eeprom_get_size(void) {return eeprom_size;};
  virtual unsigned int eeprom_get_value(unsigned int address) ;
  virtual void eeprom_put_value(unsigned int value,
				unsigned int address);
  virtual file_register *eeprom_get_register(unsigned int address);
#endif

  P16F874(void);
  static pic_processor *construct(void);
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
  static pic_processor *construct(void);
};

#endif
