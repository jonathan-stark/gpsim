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
#include "uart.h"

     // forward references
class _16bit_processor;
class IOPIN;

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

  //  PCLATH       pclath;
  INTCON_16    intcon;
  INTCON2      intcon2;
  INTCON3      intcon3;
  BSR          bsr;
  TMR0_16      tmr0l;
  TMR0H        tmr0h;
  T0CON        t0con;
  RCON         rcon;
  PIR1_16      pir1;

  sfr_register prodh,prodl;

  Fast_Stack   fast_stack;
  Indirect_Addressing  ind0;
  Indirect_Addressing  ind1;
  Indirect_Addressing  ind2;
  USART_MODULE16       usart;
  TBL_MODULE           tbl;

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
      return ((pclath.value & 0x18)<<8);
    }

  // Return the portion of pclath that is used during modify PCL instructions
  virtual unsigned int get_pclath_branching_modpcl(void)
    {
      return((pclath.value & 0x1f)<<8);
    }

  virtual void option_new_bits_6_7(unsigned int);

  // Declare a set of functions that will allow the base class to
  // get information about the derived classes. NOTE, the values returned here
  // will cause errors if they are used (in some cases)
  // -- the derived classes must define their parameters appropriately.

  virtual unsigned int register_memory_size () const { return 0x1000;};
  virtual int get_pin_count(void){return 0;};
  virtual char *get_pin_name(unsigned int pin_number) {return NULL;};
  virtual int get_pin_state(unsigned int pin_number) {return 0;};
  virtual void set_out_of_range_pm(int address, int value);

};

#define cpu16 ( (_16bit_processor *)cpu)

#endif
