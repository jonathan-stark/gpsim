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
along with gpsim; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#ifndef __P17C75X_H__
#define __P17C75X_H__

#include "16bit-processors.h"
#include "p16x6x.h"


class P17C7xx : public  _16bit_processor, public _16bit_68pins
{
public:

  CPUSTA cpusta;
  
  P17C7xx(void);

  static pic_processor *construct(void);
  virtual PROCESSOR_TYPE isa(void){return _P17C7xx_;};
  virtual void create_symbols(void);
  virtual void create(int ram_top);

  virtual void create_sfr_map(void);
  virtual unsigned int program_memory_size(void) const { return 0x400; };

  //  void create_sfr_map(void);

  virtual int get_pin_count(void){return 0;};
  virtual char *get_pin_name(unsigned int pin_number) {return NULL;};
  virtual int get_pin_state(unsigned int pin_number) {return 0;};
  virtual IOPIN *get_pin(unsigned int pin_number) {return NULL;};

};

class P17C75x : public P17C7xx
{
 public:

  P17C75x(void);
  static pic_processor *construct(void);
  virtual void create(int ram_top);
  virtual void create_sfr_map(void);
  
  virtual PROCESSOR_TYPE isa(void){return _P17C75x_;};
  virtual void create_symbols(void);
  
  virtual unsigned int program_memory_size(void) const { return 0x4000; };
  
  virtual int get_pin_count(void){return Package::get_pin_count();}; 
  virtual char *get_pin_name(unsigned int pin_number) {return Package::get_pin_name(pin_number);}; 
  virtual int get_pin_state(unsigned int pin_number) {return Package::get_pin_state(pin_number);};
  virtual IOPIN *get_pin(unsigned int pin_number) {return Package::get_pin(pin_number);}; 

  
};

class P17C752 : public P17C75x
{
 public:
  virtual PROCESSOR_TYPE isa(void){return _P17C752_;};
  P17C752(void);
  static pic_processor *construct(void);
  void create(void);
  //  void create_sfr_map(void);

  void create_sfr_map(void);
  void create_symbols(void);
  virtual unsigned int program_memory_size(void) const { return 0x2000; };
  virtual unsigned int register_memory_size(void) const { return 0x800; };

};

class P17C756 : public P17C75x
{
 public:

  virtual PROCESSOR_TYPE isa(void){return _P17C756_;};
  void create_sfr_map(void);
  void create_symbols(void);

  P17C756(void);
  static pic_processor *construct(void);
  void create(void);
  //  void create_sfr_map(void);

  virtual unsigned int program_memory_size(void) const { return 0x4000; };
  virtual unsigned int register_memory_size(void) const { return 0x800; };


};

class P17C756A : public P17C75x
{
 public:

  virtual PROCESSOR_TYPE isa(void){return _P17C756A_;};
  void create_sfr_map(void);
  void create_symbols(void);

  P17C756A(void);
  static pic_processor *construct(void);
  void create(void);
  //  void create_sfr_map(void);

  virtual unsigned int program_memory_size(void) const { return 0x4000; };
  virtual unsigned int register_memory_size(void) const { return 0x800; };


};

class P17C762 : public P17C75x
{
 public:

  virtual PROCESSOR_TYPE isa(void){return _P17C762_;};
  void create_sfr_map(void);
  void create_symbols(void);

  P17C762(void);
  static pic_processor *construct(void);
  void create(void);
  //  void create_sfr_map(void);

  virtual unsigned int program_memory_size(void) const { return 0x4000; };
  virtual unsigned int register_memory_size(void) const { return 0x800; };


};

class P17C766 : public P17C75x
{
 public:

  virtual PROCESSOR_TYPE isa(void){return _P17C766_;};
  void create_sfr_map(void);
  void create_symbols(void);

  P17C766(void);
  static pic_processor *construct(void);
  void create(void);
  //  void create_sfr_map(void);

  virtual unsigned int program_memory_size(void) const { return 0x4000; };
  virtual unsigned int register_memory_size(void) const { return 0x800; };


};

#endif
