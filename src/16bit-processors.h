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



#ifndef __16_BIT_PROCESSORS_H__
#define __16_BIT_PROCESSORS_H__

#include "pic-processor.h"
#include "pic-ioports.h"
#include "intcon.h"
#include "16bit-registers.h"
#include "16bit-tmrs.h"
#include "pir.h"
#include "uart.h"
#include "a2dconverter.h"

// forward references

extern instruction *disasm16 (pic_processor *cpu, unsigned int address, unsigned int inst);


class PicLatchRegister : public sfr_register
{
public:
  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  virtual unsigned int get(void);
  virtual void setbit(unsigned int bit_number, char new_value);

  virtual void setEnableMask(unsigned int nEnableMask);

  PicLatchRegister(const char *, PortRegister *);

protected:
  PortRegister *m_port;
  unsigned int m_EnableMask;
};

//------------------------------------------------------------------------
//
//    pic_processor
//        |
//        \__ _16bit_processor
//
// Base class for the 16bit PIC processors
//
class _16bit_processor : public pic_processor
{

public:

  // Configuration memory addresses. Again, like the interrupt vectors, the '>>1'
  // is because of gpsim's representation of the addresses
#define CONFIG1   (0x300000 >> 1)
#define CONFIG2   (0x300002 >> 1)
#define CONFIG3   (0x300004 >> 1)
#define CONFIG4   (0x300006 >> 1)
#define DEVID     (0x3ffffe >> 1)


  // So far, all 18xxx parts contain ports A,B,C
  PicPortRegister  *m_porta;
  PicTrisRegister  *m_trisa;
  PicLatchRegister *m_lata;

  PicPortRegister  *m_portb;
  PicTrisRegister  *m_trisb;
  PicLatchRegister *m_latb;

  PicPortRegister  *m_portc;
  PicTrisRegister  *m_trisc;
  PicLatchRegister *m_latc;


  ADCON0_16    adcon0;
  ADCON1       adcon1;
  ADRES        adresl;
  ADRES        adresh;
  INTCON_16    intcon;
  INTCON2      intcon2;
  INTCON3      intcon3;
  BSR          bsr;
  TMR0_16      tmr0l;
  TMR0H        tmr0h;
  T0CON        t0con;
  RCON         rcon;
  PIR1v2       pir1;
  sfr_register ipr1;
  sfr_register ipr2;
  T1CON        t1con;
  PIE          pie1;
  PIR2v2       pir2;
  PIE          pie2;
  T2CON        t2con;
  PR2          pr2;
  TMR2         tmr2;
  TMRL         tmr1l;
  TMRH         tmr1h;
  CCPCON       ccp1con;
  CCPRL        ccpr1l;
  CCPRH        ccpr1h;
  CCPCON       ccp2con;
  CCPRL        ccpr2l;
  CCPRH        ccpr2h;
  TMR3L        tmr3l;
  TMR3H        tmr3h;
  T3CON        t3con;
  PIR_SET_2    pir_set_def;

  OSCCON       osccon;
  LVDCON       lvdcon;
  WDTCON       wdtcon;

  sfr_register prodh,prodl;

  sfr_register pclatu;

  Fast_Stack   fast_stack;
  Indirect_Addressing  ind0;
  Indirect_Addressing  ind1;
  Indirect_Addressing  ind2;
  USART_MODULE16       usart16;
  Stack16              stack16;
  TBL_MODULE           tbl;
  TMR2_MODULE          tmr2_module;
  TMR3_MODULE          tmr3_module;
  SSPMODULE            ssp;



  virtual void create_symbols(void);

  void interrupt(void);
  virtual void por(void);
  virtual void create(void);// {return;};
  virtual PROCESSOR_TYPE isa(void){return _16BIT_PROCESSOR_;};
  virtual PROCESSOR_TYPE base_isa(void){return _16BIT_PROCESSOR_;};
  virtual instruction * disasm (unsigned int address, unsigned int inst)
    {
      return disasm16(this, address, inst);
    }
  /*
    There's a flaw with this approach. While this code compiles
    just fine, the callee's have trouble with it. The reason is
    that most of the callees think they're dealing with either
    a "Processor" or "pic_processor". However, as Scott Meyers
    (author of "More Effective C++") writes, we lie to the compiler.
    In most cases, we'll type cast a pic_processor object to a
    _16bit_processor when the fact of the matter is that we have
    a P18F452 or whatever. The vtables for these different classes
    pretty much gurantees that an offset into them are different.

  virtual PIR1v2 *get_pir1(void) { return (&pir1); }
  virtual PIR2v2 *get_pir2(void) { return (&pir2); }
  virtual PIR_SET_2 *get_pir_set(void) { return (&pir_set_def); }
  */
  virtual void create_sfr_map(void);

  virtual void create_stack(void) {stack = &stack16;};

  // Return the portion of pclath that is used during branching instructions
  virtual unsigned int get_pclath_branching_jump(void)
  {
    return ((pclatu.value.get()<<16) | ((pclath->value.get() & 0xf8)<<8));
  }

  // Return the portion of pclath that is used during modify PCL instructions
  virtual unsigned int get_pclath_branching_modpcl(void)
  {
    return ((pclatu.value.get()<<16) | ((pclath->value.get() & 0xff)<<8));
  }

  virtual void option_new_bits_6_7(unsigned int);

  // Declare a set of functions that will allow the base class to
  // get information about the derived classes. NOTE, the values returned here
  // will cause errors if they are used (in some cases)
  // -- the derived classes must define their parameters appropriately.

  virtual unsigned int register_memory_size () const { return 0x1000;};
  virtual void set_out_of_range_pm(unsigned int address, unsigned int value);

  virtual void create_iopin_map(void);

  virtual int  map_pm_address2index(int address) {return address/2;};
  virtual int  map_pm_index2address(int index) {return index*2;};

  static pic_processor *construct(void);
  _16bit_processor(void);

  unsigned int getCurrentDisasmAddress() { return m_current_disasm_address;}
  unsigned int getCurrentDisasmIndex()   { return m_current_disasm_address/2;}
  void setCurrentDisasmAddress(unsigned a) { m_current_disasm_address =a; }
protected:
  unsigned int m_current_disasm_address;  // Used only when .hex/.cod files are loaded

};

#define cpu16 ( (_16bit_processor *)cpu)

#endif
