/*
   Copyright (C) 2010	   Roy Rankin

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

#ifndef __P16F88X_H__
#define __P16F88X_H__

#include "p16x6x.h"

/*
#include "pir.h"
#include "eeprom.h"
#include "comparator.h"
#include "a2dconverter.h"
*/


/***************************************************************************
 *
 * Include file for:  P16F887, P16F88
 *
 *
 *
 ***************************************************************************/

class P16F88x : public _14bit_processor
{
public:

  INTCON_14_PIR    intcon_reg;

  PicPortRegister  *m_porta;
  PicTrisRegister  *m_trisa;

  PicPortGRegister *m_portb;
  PicTrisRegister  *m_trisb;
  WPU              *m_wpu;
  IOC              *m_ioc;

  PicPortRegister  *m_portc;
  PicTrisRegister  *m_trisc;

  T1CON   t1con;
  PIR    *pir1;
  PIE     pie1;
  PIR    *pir2;
  PIE     pie2;
  T2CON   t2con;
  PR2     pr2;
  TMR2    tmr2;
  TMRL    tmr1l;
  TMRH    tmr1h;
  CCPCON  ccp1con;
  CCPRL   ccpr1l;
  CCPRH   ccpr1h;
  CCPCON  ccp2con;
  CCPRL   ccpr2l;
  CCPRH   ccpr2h;
  PCON    pcon;
  SSP_MODULE   ssp;
  PIR1v2 *pir1_2_reg;
  PIR2v3 *pir2_2_reg;
  PIR_SET_2 pir_set_2_def;

  OSCCON       osccon;
  OSCTUNE      osctune;
  WDTCON       wdtcon;
  USART_MODULE usart;
  CM1CON0 cm1con0;
  CM2CON0 cm2con0;
  CM2CON1 cm2con1;
  VRCON   vrcon;
  SRCON   srcon;
  ANSEL  ansel;
  ANSEL_H  anselh;
  ADCON0 adcon0;
  ADCON1 adcon1;
  ECCPAS	eccpas;
  PWM1CON	pwm1con;
  PSTRCON	pstrcon;


  sfr_register  adresh;
  sfr_register  adresl;

  PicPortRegister  *m_porte;
  PicPSP_TrisRegister  *m_trise;


  P16F88x(const char *_name=0, const char *desc=0);
  ~P16F88x();
  virtual void set_out_of_range_pm(unsigned int address, unsigned int value);

//  virtual PROCESSOR_TYPE isa(){return _P16F88x_;};
  virtual void create_symbols();
  virtual unsigned int register_memory_size () const { return 0x200;};

  virtual unsigned int program_memory_size() { return 0; };
  virtual void option_new_bits_6_7(unsigned int bits);

  virtual void create_sfr_map();

  // The f628 (at least) I/O pins depend on the Fosc Configuration bits.
  virtual bool set_config_word(unsigned int address, unsigned int cfg_word);


  virtual void create(int eesize);
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


class P16F884 : public P16F88x
{
public:

  virtual PROCESSOR_TYPE isa(){return _P16F884_;};

  virtual unsigned int program_memory_size() const { return 4096; };
  PicPSP_PortRegister  *m_portd;

  PicTrisRegister  *m_trisd;


  P16F884(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  virtual void create_symbols();
  virtual void create_sfr_map();
  virtual void create_iopin_map();
};

class P16F887 : public P16F884
{
public:

  virtual PROCESSOR_TYPE isa(){return _P16F887_;};

  virtual unsigned int program_memory_size() const { return 8192; };


  P16F887(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  virtual void create_symbols();
  virtual void create_sfr_map();
};
class P16F882 : public P16F88x
{
public:

  virtual PROCESSOR_TYPE isa(){return _P16F882_;};

  virtual unsigned int program_memory_size() const { return 2048; };


  P16F882(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  virtual void create_symbols();
  virtual void create_sfr_map();
  virtual void create_iopin_map();
};
class P16F883 : public P16F882
{
public:

  virtual PROCESSOR_TYPE isa(){return _P16F883_;};

  virtual unsigned int program_memory_size() const { return 4096; };


  P16F883(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  virtual void create_symbols();
  virtual void create_sfr_map();
};

class P16F886 : public P16F882
{
public:

  virtual PROCESSOR_TYPE isa(){return _P16F886_;};

  virtual unsigned int program_memory_size() const { return 8192; };


  P16F886(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  virtual void create_symbols();
  virtual void create_sfr_map();
};

#endif
