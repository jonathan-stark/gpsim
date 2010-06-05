/*
   Copyright (C) 1998 T. Scott Dattalo

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

#include <iostream>
#include <stdio.h>


class InvalidRegister;   // Forward reference


#ifndef __16_BIT_REGISTERS_H__
#define __16_BIT_REGISTERS_H__

#include "pic-processor.h"
#include "14bit-registers.h"
#include "14bit-tmrs.h"
#include "pir.h"
#include "uart.h"
#include "a2dconverter.h"
#include "ssp.h"
#include "rcon.h"
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
  BSR(Processor *, const char *pName, const char *pDesc=0);

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
  FSRL(Processor *, const char *pName, const char *pDesc, Indirect_Addressing *pIAM);
  void put(unsigned int new_value);
  void put_value(unsigned int new_value);

protected:
  Indirect_Addressing  *iam;
};

class FSRH : public sfr_register
{
 public:
  FSRH(Processor *, const char *pName, const char *pDesc, Indirect_Addressing *pIAM);

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
      
protected:
  Indirect_Addressing  *iam;
};

class INDF16 : public sfr_register
{
 public:
  INDF16(Processor *, const char *pName, const char *pDesc, Indirect_Addressing *pIAM);

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get();
  unsigned int get_value();
      
protected:
  Indirect_Addressing  *iam;
};

class PREINC : public sfr_register
{
public:
  PREINC(Processor *, const char *pName, const char *pDesc, Indirect_Addressing *pIAM);

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get();
  unsigned int get_value();
      
protected:
  Indirect_Addressing  *iam;
};

class POSTINC : public sfr_register
{
public:
  POSTINC(Processor *, const char *pName, const char *pDesc, Indirect_Addressing *pIAM);

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get();
  unsigned int get_value();
      
protected:
  Indirect_Addressing  *iam;
};

class POSTDEC : public sfr_register
{
public:
  POSTDEC(Processor *, const char *pName, const char *pDesc, Indirect_Addressing *pIAM);

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get();
  unsigned int get_value();
      
protected:
  Indirect_Addressing  *iam;
};

class PLUSW : public sfr_register
{
public:
  PLUSW(Processor *, const char *pName, const char *pDesc, Indirect_Addressing *pIAM);

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get();
  unsigned int get_value();
      
protected:
  Indirect_Addressing  *iam;
};

class Indirect_Addressing
{
public:
  Indirect_Addressing(_16bit_processor *cpu, const string &n);

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

  //void init(_16bit_processor *new_cpu);
  void put(unsigned int new_value);
  unsigned int get();
  unsigned int get_value();
  void put_fsr(unsigned int new_fsr);
  unsigned int get_fsr_value(){return (fsr_value & 0xfff);};
  void update_fsr_value();
  void preinc_fsr_value();
  void postinc_fsr_value();
  void postdec_fsr_value();
  int  plusw_fsr_value();

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
  void push();
  void pop();

};

//---------------------------------------------------------
class PCL16 : public PCL
{
public:

  virtual unsigned int get();
  virtual unsigned int get_value();

  PCL16(Processor *, const char *pName, const char *pDesc=0);
};

//---------------------------------------------------------
// Program Counter
//

class Program_Counter16 : public Program_Counter
{
public:
  //virtual void increment();
  //virtual void skip();
  //virtual void jump(unsigned int new_value);
  //virtual void interrupt(unsigned int new_value);
  virtual void computed_goto(unsigned int new_value);
  //virtual void new_address(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  virtual unsigned int get_value();
  //virtual unsigned int get_next();

  Program_Counter16(Processor *pCpu);
};


//---------------------------------------------------------
// Stack
//
class Stack16;
#define  Stack16_MASK  0x1f

class STKPTR : public sfr_register
{
public:
  STKPTR(Processor *, const char *pName, const char *pDesc=0);

  Stack16 *stack;
  void put_value(unsigned int new_value);
};

class TOSL : public sfr_register
{
public:
  TOSL(Processor *, const char *pName, const char *pDesc=0);

  Stack16 *stack;

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get();
  unsigned int get_value();
      
};

class TOSH : public sfr_register
{
public:
  TOSH(Processor *, const char *pName, const char *pDesc=0);

  Stack16 *stack;

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get();
  unsigned int get_value();
      
};

class TOSU : public sfr_register
{
public:
  TOSU(Processor *, const char *pName, const char *pDesc=0);

  Stack16 *stack;

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get();
  unsigned int get_value();
      
};


class Stack16 : public Stack
{
public:
  STKPTR stkptr;
  TOSL   tosl;
  TOSH   tosh;
  TOSU   tosu;

  Stack16(Processor *);
  virtual void push(unsigned int);
  virtual unsigned int pop();
  virtual void reset();
  virtual unsigned int get_tos();
  virtual void put_tos(unsigned int);

};


class TMR0_16;


//---------------------------------------------------------
class CPUSTA :  public sfr_register
{
public:

  enum
  {
    BOR      = 1<<0,
    POR      = 1<<1,
    PD       = 1<<2,
    TO       = 1<<3,
    GLINTD   = 1<<4,
    STKAV    = 1<<5,
  };
  CPUSTA(Processor *, const char *pName, const char *pDesc=0);
};


//---------------------------------------------------------
// T0CON - Timer 0 control register
class T0CON : public OPTION_REG
{
public:

  enum {
    T08BIT = 1<<6,
    TMR0ON = 1<<7
  };

  T0CON(Processor *, const char *pName, const char *pDesc=0);
  void put(unsigned int new_value);
  void initialize();
};

//---------------------------------------------------------
// TMR0 - Timer for the 16bit core.
//
// The 18cxxx extends TMR0 to a 16-bit timer. However, it maintains 
// an 8-bit mode that is compatible with the 8-bit TMR0's in the 
// 14 and 12-bit cores. The 18cxxx TMR0 reuses this code by deriving
// from the TMR0 class and providing definitions for many of the
// virtual functions.

class TMR0H : public sfr_register
{
public:

  TMR0H(Processor *, const char *pName, const char *pDesc=0);

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get();
  unsigned int get_value();

};

class TMR0_16 : public TMR0
{
public:

  TMR0_16(Processor *, const char *pName, const char *pDesc=0);

  T0CON  *t0con;
  INTCON *intcon;
  TMR0H  *tmr0h;
  unsigned int value16;

  virtual void callback();
  virtual void callback_print();

  virtual void increment();
  virtual unsigned int get();
  virtual unsigned int get_value();
  virtual void put_value(unsigned int new_value);
  virtual unsigned int get_prescale();
  virtual unsigned int max_counts();
  virtual void set_t0if();
  virtual bool get_t0cs();
  virtual void initialize();
  virtual void start(int new_value,int sync=0);
  virtual void sleep();
  virtual void wake();
};


//---------------------------------------------------------
/*
class TMR3H : public TMRH
{
public:

};

class TMR3L : public TMRL
{
public:

};
*/
class T3CON : public T1CON
{
public:
  enum
  {
    T3CCP1 = 1<<3,
    T3CCP2 = 1<<6,
  };

  CCPRL *ccpr1l;
  CCPRL *ccpr2l;
  TMRL  *tmr1l; 

  T3CON(Processor *pCpu, const char *pName, const char *pDesc=0);
  virtual void put(unsigned int new_value);

};

//---------------------------------------------------------
//
// TMR3_MODULE
//
// 

class TMR3_MODULE
{
public:

  _16bit_processor *cpu;
  char * name_str;

  T3CON *t3con;
  PIR_SET  *pir_set;

  TMR3_MODULE();
  void initialize(T3CON *t1con, PIR_SET *pir_set);

};

//-------------------------------------------------------------------
#if 0

class TABLAT : public sfr_register
{
 public:

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get();
  unsigned int get_value();

};

class TABPTRL : public sfr_register
{
 public:

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get();
  unsigned int get_value();

};

class TABPTRH : public sfr_register
{
 public:

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get();
  unsigned int get_value();

};

class TABPTRU : public sfr_register
{
 public:

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get();
  unsigned int get_value();

};

#endif

class TBL_MODULE
{
public:
  TBL_MODULE(_16bit_processor *pCpu);

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

  void increment();
  void decrement();
  void read();
  void write();
  //void initialize(_16bit_processor *);
};




//////////////////////////////////////////
//////////////////////////////////////////
//   vapid Place holders
//////////////////////////////////////////
//////////////////////////////////////////


class LVDCON : public  sfr_register
{
public:
  unsigned int valid_bits;

  enum {
    LVDL0 = 1<<0,
    LVDL1 = 1<<1,
    LVDL2 = 1<<2,
    LVDL3 = 1<<3,
    LVDEN = 1<<4,
    IRVST = 1<<5,
  };

  LVDCON(Processor *, const char *pName, const char *pDesc=0);
};

#endif
