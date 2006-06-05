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

#ifndef __P16X6X_H__
#define __P16X6X_H__

#include "14bit-processors.h"
#include "p16x8x.h"
#include "14bit-tmrs.h"
#include "intcon.h"
#include "pir.h"
#include "ssp.h"

class P16C61 : public P16X8X
{
public:

  P16C61(const char *_name=0, const char *desc=0);

  virtual PROCESSOR_TYPE isa(){return _P16C61_;};
  virtual unsigned int program_memory_size() const { return 0x400; };
  virtual void create();

  static Processor *construct(const char *name);

};


//
//   -- Define a class to contain most of the registers/peripherals 
//      of a 16x6x device (where the second `x' is >= 3
//

class P16X6X_processor :  public Pic14Bit
{
public:

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
  PIR_SET_1 pir_set_def;
  SSP_MODULE14   ssp;

  virtual unsigned int program_memory_size() const { return 0x800; };
  virtual unsigned int register_memory_size () const { return 0x100; }

  virtual void create_symbols();
  virtual void create_sfr_map();
  virtual PIR *get_pir2() { return (pir2); }
  virtual PIR *get_pir1() { return (pir1); }
  virtual PIR_SET *get_pir_set() { return (&pir_set_def); }

  P16X6X_processor(const char *_name=0, const char *desc=0);
  virtual ~P16X6X_processor();

};

/*********************************************************************
 *  class definitions for the 16c6x family of processors
 */


class P16C62 : public  P16X6X_processor
{
  public:

  P16C62(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);

  TMR2_MODULE tmr2_module;
  virtual PROCESSOR_TYPE isa(){return _P16C62_;};
  virtual void create_symbols();
  virtual void create_sfr_map();

  virtual unsigned int program_memory_size() const { return 0x800; };
  virtual void create_iopin_map();
  virtual bool hasSSP() { return true;}

  virtual void create();
};


class P16C63 : public  P16C62
{
  public:

  USART_MODULE14 usart;
  SSP_MODULE14   ssp;

  virtual PROCESSOR_TYPE isa(){return _P16C63_;};
  virtual void create_symbols();

  virtual unsigned int program_memory_size() const { return 0x1000; };


  P16C63(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  void create();
  void create_sfr_map();
};


class P16C64 : public  P16X6X_processor
{
  public:

  PicPortRegister  *m_portd;
  PicTrisRegister  *m_trisd;

  PicPortRegister  *m_porte;
  PicTrisRegister  *m_trise;

  P16C64(const char *_name=0, const char *desc=0);
  virtual ~P16C64();

  static Processor *construct(const char *name);

  TMR2_MODULE tmr2_module;
  virtual PROCESSOR_TYPE isa(){return _P16C64_;};
  virtual void create_symbols();
  void create_sfr_map();

  virtual unsigned int program_memory_size() const { return 0x800; };
  virtual void create();
  virtual void create_iopin_map();

  virtual bool hasSSP() {return true;}

};

class P16C65 : public  P16C64
{
  public:

  USART_MODULE14 usart;

  virtual PROCESSOR_TYPE isa(){return _P16C65_;};
  virtual void create_symbols();

  virtual unsigned int program_memory_size() const { return 0x1000; };


  P16C65(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  void create();
  void create_sfr_map();
};


#endif
