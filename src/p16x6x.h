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
#include "intcon.h"
#include "pir.h"
#include "ssp.h"

class P16C61 : public P16C8x
{
public:

  virtual PROCESSOR_TYPE isa(void){return _P16C61_;};
  virtual void create_symbols(void);

  virtual unsigned int program_memory_size(void) const { return 0x400; };

  virtual void create_sfr_map(void);

  P16C61(void);
  virtual void create(void);

  static Processor *construct(void);

};


//
//   -- Define a class to contain most of the registers/peripherals 
//      of a 16x6x device (where the second `x' is >= 3
//

class P16X6X_processor :  public _14bit_processor
{
public:

  PIC_IOPORT   *porta;
  IOPORT_TRIS  trisa;

  PIC_IOPORT   *portb;
  IOPORT_TRIS  trisb;

  PIC_IOPORT   *portc;
  IOPORT_TRIS  trisc;

  T1CON   t1con;
  PIR1v1  pir1_reg;
  PIE     pie1;
  PIR2v1  pir2_reg;
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
  INTCON_14_PIR intcon_reg;
  PIR_SET_1 pir_set_def;

  // Only used in some models. It is initialized based on the value of
  // init_ssp. Only the sfrs are added, the derived class must still
  // call initialize_14 on the SSP_MODULE.
  // This doesn't seem like a good way to do it, but some proccessor
  // classes that shouldn't have an SSP are derived from classes that
  // should.
  SSP_MODULE14   ssp;
  bool init_ssp;


  virtual unsigned int program_memory_size(void) const { return 0x800; };

  virtual void create_sfr_map(void);
  /*  virtual void option_new_bits_6_7(unsigned int bits)
    {
      //((PORTB *)portb)->rbpu_intedg_update(bits);
    }
  */
  virtual PIR *get_pir2(void) { return (&pir2_reg); }
  virtual PIR *get_pir1(void) { return (&pir1_reg); }
  virtual PIR_SET *get_pir_set(void) { return (&pir_set_def); }

  P16X6X_processor(void);

};

/*********************************************************************
 *  class definitions for the 16c6x family of processors
 */


class P16C62 : public  P16X6X_processor
{
  public:


  TMR2_MODULE tmr2_module;
  virtual PROCESSOR_TYPE isa(void){return _P16C62_;};
  virtual void create_symbols(void);
  void create_sfr_map(void);

  virtual unsigned int program_memory_size(void) const { return 0x800; };
  /*
  virtual void option_new_bits_6_7(unsigned int bits)
    {
      ((PORTB *)portb)->rbpu_intedg_update(bits);
    }
  */
  P16C62(void);
  static Processor *construct(void);
  virtual void create_iopin_map(void);

  void create(void);
};


class P16C63 : public  P16C62
{
  public:

  USART_MODULE14 usart;
  SSP_MODULE14   ssp;

  virtual PROCESSOR_TYPE isa(void){return _P16C63_;};
  virtual void create_symbols(void);

  virtual unsigned int program_memory_size(void) const { return 0x1000; };


  P16C63(void);
  static Processor *construct(void);
  void create(void);
  void create_sfr_map(void);
};


class P16C64 : public  P16X6X_processor
{
  public:

  PIC_IOPORT   *portd;
  IOPORT_TRIS  trisd;

  PIC_IOPORT   *porte;
  IOPORT_TRIS  trise;


  TMR2_MODULE tmr2_module;
  virtual PROCESSOR_TYPE isa(void){return _P16C64_;};
  virtual void create_symbols(void);
  void create_sfr_map(void);

  virtual unsigned int program_memory_size(void) const { return 0x800; };
  /*
  virtual void option_new_bits_6_7(unsigned int bits)
    {
      ((PORTB *)portb)->rbpu_intedg_update(bits);
    }
  */
  P16C64(void);
  static Processor *construct(void);
  void create(void);
  virtual void create_iopin_map(void);

};

class P16C65 : public  P16C64
{
  public:

  USART_MODULE14 usart;

  virtual PROCESSOR_TYPE isa(void){return _P16C65_;};
  virtual void create_symbols(void);

  virtual unsigned int program_memory_size(void) const { return 0x1000; };


  P16C65(void);
  static Processor *construct(void);
  void create(void);
  void create_sfr_map(void);
};


#endif
