/*
   Copyright (C) 2009 Roy R. Rankin

This file is part of the libgpsim library of gpsim

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see 
<http://www.gnu.org/licenses/lgpl-2.1.html>.
*/

#ifndef __P12F629_H__
#define __P12F629_H__

#include "14bit-processors.h"
#include "14bit-tmrs.h"
#include "intcon.h"
#include "pir.h"
#include "pie.h"
#include "eeprom.h"
#include "comparator.h"
#include "a2dconverter.h"

class WPU;
class IOC;
class PicPortGRegister;

class P12F629 : public _14bit_processor
{
public:
  INTCON_14_PIR    intcon_reg;
  ComparatorModule comparator;
  PIR_SET_1 pir_set_def;
  PIE     pie1;
  PIR    *pir1;
  T1CON   t1con;
  TMRL    tmr1l;
  TMRH    tmr1h;
  PCON    pcon;
  OSCCAL  osccal;
  EEPROM_PIR *e;

  PicPortGRegister  *m_gpio;
  PicTrisRegister  *m_trisio;
  WPU		   *m_wpu;
  IOC		   *m_ioc;

  virtual PIR *get_pir2() { return (NULL); }
  virtual PIR *get_pir1() { return (pir1); }
  virtual PIR_SET *get_pir_set() { return (&pir_set_def); }


  virtual PROCESSOR_TYPE isa(){return _P12F629_;};
  P12F629(const char *_name=0, const char *desc=0);
  ~P12F629();
  static Processor *construct(const char *name);
  virtual void create_sfr_map();
  virtual void create_symbols();
  virtual void set_out_of_range_pm(unsigned int address, unsigned int value);
  virtual void create_iopin_map();
  virtual void create(int ram_top, int eeprom_size);
  virtual unsigned int register_memory_size () const { return 0x100; }
  virtual void option_new_bits_6_7(unsigned int bits);
  virtual unsigned int program_memory_size() const { return 0x400; }
  virtual void create_config_memory();
  virtual bool set_config_word(unsigned int address,unsigned int cfg_word);
  virtual void enter_sleep();
  virtual void exit_sleep();


};


class P12F675 : public P12F629
{
public:

  ANSEL_12F  ansel;
  ADCON0_12F adcon0;
  ADCON1 adcon1;
  sfr_register  adresh;
  sfr_register  adresl;


  virtual PROCESSOR_TYPE isa(){return _P12F675_;};

  virtual void create(int ram_top, int eeprom_size);
  virtual unsigned int program_memory_size() const { return 0x400; };

  P12F675(const char *_name=0, const char *desc=0);
  ~P12F675() {};
  static Processor *construct(const char *name);
  virtual void create_sfr_map();
};

class P12F683 : public P12F675
{
public:
  T2CON   t2con;
  PR2     pr2;
  TMR2    tmr2;
  CCPCON  ccp1con;
  CCPRL   ccpr1l;
  CCPRH   ccpr1h;
  WDTCON  wdtcon;
  OSCCON  osccon;
  OSCTUNE  osctune;


  virtual PROCESSOR_TYPE isa(){return _P12F683_;};

  virtual void create(int ram_top, int eeprom_size);
  virtual unsigned int program_memory_size() const { return 0x800; };

  P12F683(const char *_name=0, const char *desc=0);
  ~P12F683() {};
  static Processor *construct(const char *name);
  virtual void create_sfr_map();
};

#endif
