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



#ifndef __16_BIT_PROCESSORS_H__
#define __16_BIT_PROCESSORS_H__

#include "pic-processor.h"
#include "16bit-registers.h"
#include "16bit-tmrs.h"
#include "uart.h"
#include "pic-packages.h"

     // forward references

extern instruction *disasm16 (pic_processor *cpu, unsigned int address, unsigned int inst);


class _16bit_processor : public pic_processor
{

public:
  // The 18cxxx has two address to which interrupts may be vectored. The '>> 1'
  // is because gpsim represents the program memory addresses the way the pic does
  // (and not the way the data sheet or the assembler does).
#define INTERRUPT_VECTOR_LO       (0x18 >> 1)
#define INTERRUPT_VECTOR_HI       (0x08 >> 1)

  // Configuration memory addresses. Again, like the interrupt vectors, the '>>1'
  // is because of gpsim's representation of the addresses
#define CONFIG1   (0x300000 >> 1)
#define CONFIG2   (0x300002 >> 1)
#define CONFIG3   (0x300004 >> 1)
#define CONFIG4   (0x300006 >> 1)
#define DEVID     (0x3ffffe >> 1)

  unsigned int current_disasm_address;  // Used only when .hex/.cod files are loaded
  unsigned int interrupt_vector;        // Starting address of the interrupt

  PIC_IOPORT   porta;      // So far, all 18xxx parts contain ports A,B,C
  IOPORT_TRIS  trisa;
  IOPORT_LATCH lata;

  PIC_IOPORT   portb;
  IOPORT_TRIS  trisb;
  IOPORT_LATCH latb;

  PORTC16      portc;
  IOPORT_TRIS  trisc;
  IOPORT_LATCH latc;


  INTCON_16    intcon;
  INTCON2      intcon2;
  INTCON3      intcon3;
  BSR          bsr;
  TMR0_16      tmr0l;
  TMR0H        tmr0h;
  T0CON        t0con;
  RCON         rcon;
  PIR1         pir1;
  sfr_register ipr1;
  sfr_register ipr2;
  T1CON        t1con;
  PIE          pie1;
  PIR2         pir2;
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

  OSCCON       osccon;
  LVDCON       lvdcon;
  WDTCON       wdtcon;

  sfr_register prodh,prodl;

  Fast_Stack   fast_stack;
  Indirect_Addressing  ind0;
  Indirect_Addressing  ind1;
  Indirect_Addressing  ind2;
  USART_MODULE16       usart16;
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

  void create_sfr_map(void);

  virtual void create_stack(void) {stack = new Stack16;};

  // Return the portion of pclath that is used during branching instructions
  virtual unsigned int get_pclath_branching_jump(void)
    {
      return ((pclath->value & 0x18)<<8);
    }

  // Return the portion of pclath that is used during modify PCL instructions
  virtual unsigned int get_pclath_branching_modpcl(void)
    {
      return((pclath->value & 0x1f)<<8);
    }

  virtual void option_new_bits_6_7(unsigned int);

  // Declare a set of functions that will allow the base class to
  // get information about the derived classes. NOTE, the values returned here
  // will cause errors if they are used (in some cases)
  // -- the derived classes must define their parameters appropriately.

  virtual unsigned int register_memory_size () const { return 0x1000;};

  virtual int get_pin_count(void){ 
    if(package) 
      return package->get_pin_count();
    else
      return 0;
  };

  virtual char *get_pin_name(unsigned int pin_number) {
    if(package) 
      return package->get_pin_name(pin_number);
    else
      return NULL;
  };

  virtual int get_pin_state(unsigned int pin_number) {
    if(package) 
      return package->get_pin_state(pin_number);
    else
      return 0;
  };

  virtual IOPIN *get_pin(unsigned int pin_number) {
    if(package)
      return package->get_pin(pin_number);
    else
      return NULL;
  };

  virtual void set_out_of_range_pm(int address, int value);

  virtual void create_iopin_map(void);

  virtual int  map_pm_address2index(int address) {return address/2;};
  virtual int  map_pm_index2address(int index) {return index*2;};

  static pic_processor *construct(void);
  _16bit_processor(void);
};

#define cpu16 ( (_16bit_processor *)cpu)

#endif
