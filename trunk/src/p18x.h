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

#ifndef __P18X_H__
#define __P18X_H__

#include "16bit-processors.h"
//#include "p16x6x.h"

/*
class _16bit_28pins
{
public:


  PORTA        porta;
  IOPORT_TRIS  trisa;

  PORTB        portb;
  IOPORT_TRIS  trisb;

  PORTC        portc;
  IOPORT_TRIS  trisc;

  void create_iopin_map(void);

};

*/

class P18Cxx2 : public  _16bit_processor
{
public:

  P18Cxx2(void);

  virtual PROCESSOR_TYPE isa(void){return _P18Cxx2_;};
  virtual void create_symbols(void);

  virtual unsigned int program_memory_size(void) const { return 0x400; };

/*
  virtual int get_pin_count(void){return 0;};
  virtual char *get_pin_name(unsigned int pin_number) {return NULL;};
  virtual int get_pin_state(unsigned int pin_number) {return 0;};
  virtual IOPIN *get_pin(unsigned int pin_number) {return NULL;};
*/
};

class P18C2x2 : public _16bit_processor//, public _28pins
{
 public:

  P18C2x2(void);
  //static Processor *construct(void);
  void create(void);

  virtual PROCESSOR_TYPE isa(void){return _P18Cxx2_;};
  virtual void create_symbols(void);

  virtual unsigned int program_memory_size(void) const { return 0x400; };

  void create_sfr_map(void);
/*
  virtual int get_pin_count(void){return Package::get_pin_count();};
  virtual char *get_pin_name(unsigned int pin_number) {return Package::get_pin_name(pin_number);};
  virtual int get_pin_state(unsigned int pin_number) {return Package::get_pin_state(pin_number);};
  virtual IOPIN *get_pin(unsigned int pin_number) {return Package::get_pin(pin_number);};
*/
  virtual void create_iopin_map(void);

};

class P18C242 : public P18C2x2
{
 public:
  virtual PROCESSOR_TYPE isa(void){return _P18C242_;};
  P18C242(void);
  static Processor *construct(void);
  void create(void);
  void create_sfr_map(void);

  virtual unsigned int program_memory_size(void) const { return 0x2000; };

};

class P18C252 : public P18C242
{
 public:

  virtual PROCESSOR_TYPE isa(void){return _P18C252_;};
  P18C252(void);
  static Processor *construct(void);
  void create(void);
  void create_sfr_map(void);

  virtual unsigned int program_memory_size(void) const { return 0x4000; };


};

/*********************************************************************
 *  class definitions for the 18C4x2 family
 */

class P18C4x2 : public _16bit_processor //, public _14bit_40pins
{
 public:


  PIC_IOPORT   portd;
  IOPORT_TRIS  trisd;
  IOPORT_LATCH latd;

  PIC_IOPORT   porte;
  IOPORT_TRIS  trise;
  IOPORT_LATCH late;

  P18C4x2(void);
  //static Processor *construct(void);
  void create(void);

  virtual PROCESSOR_TYPE isa(void){return _P18Cxx2_;};
  virtual void create_symbols(void);

  virtual unsigned int program_memory_size(void) const { return 0x400; };

  void create_sfr_map(void);
  virtual void create_iopin_map(void);

/*
  virtual int get_pin_count(void){return Package::get_pin_count();};
  virtual char *get_pin_name(unsigned int pin_number) {return Package::get_pin_name(pin_number);};
  virtual int get_pin_state(unsigned int pin_number) {return Package::get_pin_state(pin_number);};
  virtual IOPIN *get_pin(unsigned int pin_number) {return Package::get_pin(pin_number);};
*/
};


class P18C442 : public P18C4x2
{
 public:
  virtual PROCESSOR_TYPE isa(void){return _P18C442_;};
  P18C442(void);
  static Processor *construct(void);
  void create(void);
  void create_sfr_map(void);

  virtual unsigned int program_memory_size(void) const { return 0x2000; };

};


class P18C452 : public P18C442
{
 public:
  virtual PROCESSOR_TYPE isa(void){return _P18C452_;};
  P18C452(void);
  static Processor *construct(void);
  void create(void);
  void create_sfr_map(void);

  virtual unsigned int program_memory_size(void) const { return 0x4000; };

};

class P18F442 : public P18C442
{
 public:
  virtual PROCESSOR_TYPE isa(void){return _P18F442_;};
  P18F442(void);
  static Processor *construct(void);
  void create(void);
  void create_sfr_map(void);

  virtual unsigned int program_memory_size(void) const { return 0x2000; };

};

class P18F452 : public P18F442
{
 public:
  virtual PROCESSOR_TYPE isa(void){return _P18F452_;};
  P18F452(void);
  static Processor *construct(void);
  void create(void);
  void create_sfr_map(void);

  virtual unsigned int program_memory_size(void) const { return 0x4000; };

};
#endif
