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

#include <iostream.h>
#include <stdio.h>


class invalid_file_register;   // Forward reference


#ifndef __16_BIT_REGISTERS_H__
#define __16_BIT_REGISTERS_H__

#include "pic-processor.h"
#include "14bit-registers.h"
#include "uart.h"

#define _16BIT_REGISTER_MASK   0xfff

class _16bit_processor;

class stimulus;  // forward reference
class IOPIN;
class source_stimulus;
class Stimulus_Node;
class PORTB;


//---------------------------------------------------------
// BSR register
//

class BSR : public sfr_register
{
public:

  unsigned int register_page_bits;

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);

};


//---------------------------------------------------------
// 
// Indirect_Addressing
//
// This class coordinates the indirect addressing on the 18cxxx
// parts. Each of the registers comprising the indirect addressing
// subsystem: FSRnL,FSRnH, INDFn, POSTINCn, POSTDECn, PREINCn, and
// PLUSWn are each individually defined as sfr_registers AND included
// in the Indirect_Addressing class. So accessing these registers
// is the same as accessing any register: through the core cpu's
// register memory. The only difference for these registers is that
// the 

class Indirect_Addressing;   // Forward reference

//---------------------------------------------------------
// FSR registers

class FSRL : public sfr_register
{
 public:
  Indirect_Addressing  *iam;  // The IA module to which FSRL belongs

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
      
};

class FSRH : public sfr_register
{
 public:
  Indirect_Addressing  *iam;  // The IA module to which FSRH belongs

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
      
};

class INDF16 : public sfr_register
{
 public:
  Indirect_Addressing  *iam;  // The IA module to which INDF belongs

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get(void);
  unsigned int get_value(void);
      
};

class PREINC : public sfr_register
{
 public:
  Indirect_Addressing  *iam;  // The IA module to which PREINC belongs

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get(void);
  unsigned int get_value(void);
      
};

class POSTINC : public sfr_register
{
 public:
  Indirect_Addressing  *iam;  // The IA module to which POSTINC belongs

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get(void);
  unsigned int get_value(void);
      
};

class POSTDEC : public sfr_register
{
 public:
  Indirect_Addressing  *iam;  // The IA module to which POSTDEC belongs

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get(void);
  unsigned int get_value(void);
      
};

class PLUSW : public sfr_register
{
 public:
  Indirect_Addressing  *iam;  // The IA module to which PLUSW belongs

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get(void);
  unsigned int get_value(void);
      
};

class Indirect_Addressing
{
public:
  _16bit_processor *cpu;

  unsigned int fsr_value;     // 16bit concatenation of fsrl and fsrh
  unsigned int fsr_state;     /* used in conjunction with the pre/post incr
			       * and decrement. This is mainly needed for
			       * those instructions that perform read-modify-
			       * write operations on the indirect registers
			       * eg. btg POSTINC1,4 . The post increment must
			       * occur after the bit is toggled and not during
			       * the read operation that's determining the 
			       * current state.
			       */
  int     fsr_delta;          /* If there's a pending update to the fsr register
			       * pair, then the magnitude of that update is
			       * stored here.
			       */
  guint64 current_cycle;      /* Stores the cpu cycle when the fsr was last
			       * changed. 
			       */
  FSRL    fsrl;
  FSRH    fsrh;
  INDF16  indf;
  PREINC  preinc;
  POSTINC postinc;
  POSTDEC postdec;
  PLUSW   plusw;

  void init(_16bit_processor *new_cpu);
  void put(unsigned int new_value);
  unsigned int get(void);
  unsigned int get_value(void);
  void put_fsr(unsigned int new_fsr);
  unsigned int get_fsr_value(void){return (fsr_value & 0xfff);};
  void update_fsr_value(void);
  void preinc_fsr_value(void);
  void postinc_fsr_value(void);
  void postdec_fsr_value(void);
  int  plusw_fsr_value(void);

  /* bool is_indirect_register(unsigned int reg_address)
   *
   * The purpose of this routine is to determine whether or not the
   * 'reg_address' is the address of an indirect register. This is
   * used by the 'put' and 'get' functions of the indirect registers.
   * Indirect registers are forbidden access to other indirect registers.
   * (Although double indirection in a single instruction cycle would
   * be powerful!).
   *
   * The indirect registers reside at the following addresses
   * 0xfeb - 0xfef, 0xfe3 - 0xfe7, 0xfdb- 0xfdf
   * If you look at the binary representation of these ranges:
   * 1111 1110 1011, 1111 1110 1100 - 1111 1110 1111    (0xfeb,0xfec - 0xfef)
   * 1111 1110 0011, 1111 1110 0100 - 1111 1110 0111    (0xfe3,0xfe4 - 0xfe7)
   * 1111 1101 1011, 1111 1101 1100 - 1111 1101 1111    (0xfdb,0xfdc - 0xfdf)
   * ------------------------------------------------------------------------
   * 1111 11xx x011, 1111 11vv v1yy - 1111 11vv v1yy
   *
   * Then you'll notice that indirect register addresses share
   * the common bit pattern 1111 11xx x011 for the left column.
   * Furthermore, the middle 3-bits, xxx, can only be 3,4, 5.
   * The ranges in the last two columns share the bit pattern
   * 1111 11vv v1yy. The middle 3-bits, vvv, again can only be 
   * 3,4, or 5. The least two lsbs, yy, are don't cares.
   */

  inline bool is_indirect_register(unsigned int reg_address)
    {
      if( ((reg_address & 0xfc7) == 0xfc3) || ((reg_address & 0xfc4) == 0xfc4))
	{
	  unsigned midbits = (reg_address >> 3) & 0x7;
	  if(midbits >= 3 && midbits <= 5)
	    return 1;
	}
      return 0;
    }


};

//---------------------------------------------------------
class Fast_Stack
{
 public:

  unsigned int w,status,bsr;
  _16bit_processor *cpu;

  void init(_16bit_processor *new_cpu);
  void push(void);
  void pop(void);

};

//---------------------------------------------------------
// Program Counter
//

class Program_Counter16 : public Program_Counter
{
public:
  virtual void put_value(unsigned int new_value);
};


//---------------------------------------------------------
// Stack
//
class Stack16;
#define  Stack16_MASK  0x1f

class STKPTR : public sfr_register
{
 public:
  Stack16 *stack;

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get(void);
  unsigned int get_value(void);
    
};

class TOSL : public sfr_register
{
 public:
  Stack16 *stack;

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get(void);
  unsigned int get_value(void);
      
};

class TOSH : public sfr_register
{
 public:
  Stack16 *stack;

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get(void);
  unsigned int get_value(void);
      
};

class TOSU : public sfr_register
{
 public:
  Stack16 *stack;

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get(void);
  unsigned int get_value(void);
      
};


class Stack16 : public Stack
{
public:
  STKPTR stkptr;
  TOSL   tosl;
  TOSH   tosh;
  TOSU   tosu;

  Stack16(void);
  virtual void push(unsigned int);
  virtual unsigned int pop(void);
  virtual void reset(void);
  virtual unsigned int get_tos(void);
  virtual void put_tos(unsigned int);

};


class TMR0_16;
class INTCON2;

//---------------------------------------------------------
class RCON :  public sfr_register
{
public:

enum
{
  BOR  = 1<<0,
  POR  = 1<<1,
  PD   = 1<<2,
  TO   = 1<<3,
  RI   = 1<<4,
  LWRT = 1<<6,
  IPEN = 1<<7
};
};

//---------------------------------------------------------
// INTCON_16 - Interrupt control register for the 16-bit core

class INTCON_16 : public INTCON
{
public:

enum 
{
  GIEH = GIE,
  GIEL = XXIE
};

  TMR0_16 *tmr0l;
  RCON    *rcon;
  INTCON2 *intcon2;

  virtual void put(unsigned int new_value);

  void clear_gies(void);
  void set_gies(void);
  virtual void initialize(void);
};



//---------------------------------------------------------
class INTCON2 :  public sfr_register
{
public:

enum
{
  RBIP    = 1<<0,
  TMR0IP  = 1<<2,
  INTEDG2 = 1<<4,
  INTEDG1 = 1<<5,
  INTEDG0 = 1<<6,
  RBPU    = 1<<7
};
};


class INTCON3 :  public sfr_register
{
public:

enum
{
  INT1IF  = 1<<0,
  INT2IF  = 1<<1,
  INT1IE  = 1<<3,
  INT2IE  = 1<<4,
  INT1IP  = 1<<6,
  INT2IP  = 1<<7
};

};


//---------------------------------------------------------
// PIR1
class PIR1_16 : public sfr_register
{
 public:

enum
{
  TMR1IF = 1<<0,
  TMR2IF = 1<<1,
  CCP1IF = 1<<2,
  SSPIF  = 1<<3,
  TXIF   = 1<<4,
  RCIF   = 1<<5,
  ADIF   = 1<<6,
  PSPIF  = 1<<7
};

 unsigned int get_TXIF(void)
   {
     return value & TXIF;
   }
 void set_TXIF(void)
   {
     value |= TXIF;
     trace.register_write(address,value);
   }
 void clear_TXIF(void)
   {
     value &= ~TXIF;
     trace.register_write(address,value);
   }
 
 unsigned int get_RCIF(void)
   {
     return value & RCIF;
   }
 void set_RCIF(void)
   {
     value |= RCIF;
     trace.register_write(address,value);
   }
};

//---------------------------------------------------------
// T0CON - Timer 0 control register
class T0CON : public OPTION_REG
{
 public:

  enum {
    T08BIT = 1<<6
  };

  T0CON(void);
  void put(unsigned int new_value);
};

//---------------------------------------------------------
// TMR0 - Timer
/*
class _TMR0 : public TMR0 // public sfr_register, public BreakCallBack
{
public:

  unsigned int 
    synchronized_cycle,
    prescale,
    prescale_counter,
    last_cycle,
    future_cycle;


  virtual void callback(void) = 0;

  _TMR0(void);

  virtual void put(unsigned int new_value)=0;
  unsigned int get(void);
  virtual unsigned int get_value(void) = 0;
  void start(void);
  virtual void increment(void)=0;   // Used when tmr0 is attached to an external clock
  virtual void new_prescale(void)=0;
  void new_clock_source(void);

  virtual unsigned int get_prescale(void)=0;
  virtual void set_t0if(void)=0;
  virtual unsigned int get_t0cs(void)=0;
  virtual void initialize(void)=0;
};

  */

class TMR0H : public sfr_register
{
 public:

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get(void);
  unsigned int get_value(void);

};

class TMR0_16 : public TMR0
{
public:

  T0CON *t0con;
  INTCON *intcon;
  TMR0H  *tmr0h;

  virtual void callback(void);
  virtual void increment(void);
  virtual void put(unsigned int new_value);
  virtual unsigned int get_value(void);
  virtual unsigned int get_prescale(void);
  virtual void new_prescale(void);
  virtual void set_t0if(void);
  virtual unsigned int get_t0cs(void);
  virtual void initialize(void);
};


//---------------------------------------------------------
// uart 
class TXREG_16 : public _TXREG
{
 public:
  PIR1_16 *pir1;

  virtual bool is_empty(void);
  virtual void empty(void);
  virtual void full(void);

};

class RCREG_16 : public _RCREG
{
 public:
  PIR1_16 *pir1;

  virtual void push(unsigned int);

};

//---------------------------------------------------------
class USART_MODULE
{
 public:

  _16bit_processor *cpu;

  // Serial Port Registers
  _TXSTA       txsta;
  _RCSTA       rcsta;
  _SPBRG       spbrg;
  TXREG_16     txreg;
  RCREG_16     rcreg;

  void initialize(_16bit_processor *);

};

//-------------------------------------------------------------------
#if 0

class TABLAT : public sfr_register
{
 public:

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get(void);
  unsigned int get_value(void);

};

class TABPTRL : public sfr_register
{
 public:

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get(void);
  unsigned int get_value(void);

};

class TABPTRH : public sfr_register
{
 public:

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get(void);
  unsigned int get_value(void);

};

class TABPTRU : public sfr_register
{
 public:

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get(void);
  unsigned int get_value(void);

};

#endif

class TBL_MODULE
{
 public:
  unsigned int state;
  unsigned int internal_latch;

  _16bit_processor *cpu;

#if 0
  TABLAT    tablat;
  TABPTRL   tabptrl;
  TABPTRH   tabptrh;
  TABPTRU   tabptru;
#endif
  sfr_register   tablat,
                 tabptrl,
                 tabptrh,
                 tabptru;

  void increment(void);
  void decrement(void);
  void read(void);
  void write(void);
  void initialize(_16bit_processor *);

};


#endif
