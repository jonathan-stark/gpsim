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

#include <stdio.h>


class InvalidRegister;   // Forward reference

#include "trace.h"

#ifndef __14_BIT_REGISTERS_H__
#define __14_BIT_REGISTERS_H__


class _14bit_processor;

#include "breakpoints.h"

class stimulus;  // forward reference
class IOPIN;
class source_stimulus;
class Stimulus_Node;
class PORTB;
class pic_processor;

#include "ioports.h"

//---------------------------------------------------------
// FSR register
//

class FSR : public sfr_register
{
public:

  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  virtual unsigned int get(void);
  virtual unsigned int get_value(void);

};


//---------------------------------------------------------
// FSR_12 register - FSR for the 12-bit core processors.
//
//
class FSR_12 : public FSR
{
public:
  unsigned int valid_bits;
  unsigned int register_page_bits;   /* Only used by the 12-bit core to define
                                        the valid paging bits in the FSR. */
  FSR_12(unsigned int _register_page_bits, unsigned int _valid_bits);

  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  virtual unsigned int get(void);
  virtual unsigned int get_value(void);

};


//---------------------------------------------------------
// Status register
//

class Status_register : public sfr_register
{
public:

#define STATUS_Z_BIT   2
#define STATUS_C_BIT   0
#define STATUS_DC_BIT  1
#define STATUS_PD_BIT  3
#define STATUS_TO_BIT  4
#define STATUS_OV_BIT  3     //18cxxx
#define STATUS_N_BIT   4     //18cxxx
#define STATUS_FSR0_BIT 4     //17c7xx
#define STATUS_FSR1_BIT 6     //17c7xx
#define STATUS_Z       (1<<STATUS_Z_BIT)
#define STATUS_C       (1<<STATUS_C_BIT)
#define STATUS_DC      (1<<STATUS_DC_BIT)
#define STATUS_PD      (1<<STATUS_PD_BIT)
#define STATUS_TO      (1<<STATUS_TO_BIT)
#define STATUS_OV      (1<<STATUS_OV_BIT)
#define STATUS_N       (1<<STATUS_N_BIT)
#define STATUS_FSR0_MODE (3<<STATUS_FSR0_BIT)     //17c7xx
#define STATUS_FSR1_MODE (3<<STATUS_FSR1_BIT)     //17c7xx
#define BREAK_Z_ACCESS 2
#define BREAK_Z_WRITE  1

#define RP_MASK        0x20
  unsigned int break_point;
  unsigned int break_on_z,break_on_c;
  unsigned int rp_mask;
  unsigned int write_mask;    // Bits that instructions can modify

  Status_register(void);

  inline void put(unsigned int new_value);

  inline unsigned int get(void)
  {
    //get_trace().register_read(address,value.get());
    return(value.get());
  }

  // Special member function to control just the Z bit

  inline void put_Z(unsigned int new_z)
  {
    get_trace().register_write(address,value.get());
    value.put((value.get() & ~STATUS_Z) | ((new_z) ? STATUS_Z : 0));
  }

  inline unsigned int get_Z(void)
  {
    get_trace().register_read(address,value.get());
    return( ( (value.get() & STATUS_Z) == 0) ? 0 : 1);
  }


  // Special member function to control just the C bit
  void put_C(unsigned int new_c)
  {
    get_trace().register_write(address,value.get());
    value.put((value.get() & ~STATUS_C) | ((new_c) ? STATUS_C : 0));
  }

  unsigned int get_C(void)
  {
    get_trace().register_read(address,value.get());
    return( ( (value.get() & STATUS_C) == 0) ? 0 : 1);
  }

  // Special member function to set Z, C, and DC

  inline void put_Z_C_DC(unsigned int new_value, unsigned int src1, unsigned int src2)
  {
    get_trace().register_write(address,value.get());

    value.put((value.get() & ~ (STATUS_Z | STATUS_C | STATUS_DC)) |  
	      ((new_value & 0xff)   ? 0 : STATUS_Z)   |
	      ((new_value & 0x100)  ? STATUS_C : 0)   |
	      (((new_value ^ src1 ^ src2)&0x10) ? STATUS_DC : 0));

  }

  inline void put_Z_C_DC_for_sub(unsigned int new_value, unsigned int src1, unsigned int src2)
  {

    get_trace().register_write(address,value.get());

    value.put((value.get() & ~ (STATUS_Z | STATUS_C | STATUS_DC)) |  
	      ((new_value & 0xff)   ? 0 : STATUS_Z)   |
	      ((new_value & 0x100)  ? 0 : STATUS_C)   |
	      (((new_value ^ src1 ^ src2)&0x10) ? 0 : STATUS_DC));

  }

  inline void put_PD(unsigned int new_pd)
  {

    get_trace().register_write(address,value.get());
    value.put((value.get() & ~STATUS_PD) | ((new_pd) ? STATUS_PD : 0));
  }

  inline unsigned int get_PD(void)
  {

    get_trace().register_read(address,value.get());
    return( ( (value.get() & STATUS_PD) == 0) ? 0 : 1);
  }

  inline void put_TO(unsigned int new_to)
  {
    get_trace().register_write(address,value.get());
    value.put((value.get() & ~STATUS_TO) | ((new_to) ? STATUS_TO : 0));
  }

  inline unsigned int get_TO(void)
  {
    get_trace().register_read(address,value.get());
    return( ( (value.get() & STATUS_TO) == 0) ? 0 : 1);
  }

  // Special member function to set Z, C, DC, OV, and N for the 18cxxx family

  // Special member function to control just the N bit
  void put_N_Z(unsigned int new_value)
  {
    get_trace().register_write(address,value.get());
    value.put((value.get() & ~(STATUS_Z | STATUS_N)) | 
	      ((new_value & 0xff )  ? 0 : STATUS_Z)   |
	      ((new_value & 0x80) ? STATUS_N : 0));
  }

  void put_Z_C_N(unsigned int new_value)
  {
    get_trace().register_write(address,value.get());
    value.put((value.get() & ~(STATUS_Z | STATUS_C | STATUS_N)) | 
	      ((new_value & 0xff )  ? 0 : STATUS_Z)   |
	      ((new_value & 0x100)  ? STATUS_C : 0)   |
	      ((new_value & 0x80) ? STATUS_N : 0));
  }

  inline void put_Z_C_DC_OV_N(unsigned int new_value, unsigned int src1, unsigned int src2)
  {
    get_trace().register_write(address,value.get());
    value.put((value.get() & ~ (STATUS_Z | STATUS_C | STATUS_DC | STATUS_OV | STATUS_N)) |  
	      ((new_value & 0xff )  ? 0 : STATUS_Z)   |
	      ((new_value & 0x100)  ? STATUS_C : 0)   |
	      (((new_value ^ src1 ^ src2)&0x10) ? STATUS_DC : 0) |
	      ((new_value ^ src1) & 0x80 ? STATUS_OV : 0) |
	      ((new_value & 0x80) ? STATUS_N : 0));
  }

  inline void put_Z_C_DC_OV_N_for_sub(unsigned int new_value, unsigned int src1, unsigned int src2)
  {
    get_trace().register_write(address,value.get());
    value.put((value.get() & ~ (STATUS_Z | STATUS_C | STATUS_DC | STATUS_OV | STATUS_N)) |  
	      ((new_value & 0xff)   ? 0 : STATUS_Z)   |
	      ((new_value & 0x100)  ? 0 : STATUS_C)   |
	      (((new_value ^ src1 ^ src2)&0x10) ? 0 : STATUS_DC) |
	      (((src1 & ~src2 & ~new_value | new_value & ~src1 & src2) & 0x80) ? STATUS_OV : 0) |
	      ((new_value & 0x80)   ? STATUS_N : 0));
  }

  // Special member function to control just the FSR mode
  void put_FSR0_mode(unsigned int new_value)
  {
    get_trace().register_write(address,value.get());
    value.put((value.get() & ~(STATUS_FSR0_MODE)) | 
	      (new_value & 0x03 ));
  }

  unsigned int get_FSR0_mode(unsigned int new_value)
  {
    get_trace().register_write(address,value.get());
    return( (value.get()>>STATUS_FSR0_BIT) & 0x03);
  }

  void put_FSR1_mode(unsigned int new_value)
  {
    get_trace().register_write(address,value.get());
    value.put((value.get() & ~(STATUS_FSR1_MODE)) | 
	      (new_value & 0x03 ));
  }

  unsigned int get_FSR1_mode(unsigned int new_value)
  {
    get_trace().register_read(address,value.get());
    return( (value.get()>>STATUS_FSR1_BIT) & 0x03);
  }

  

};


#include "gpsim_time.h"

//---------------------------------------------------------
// Stack
//

class Stack
{
public:
  unsigned int contents[32];       /* the stack array */ 
  int pointer;                     /* the stack pointer */
  unsigned int stack_mask;         /* 1 for 12bit, 7 for 14bit, 31 for 16bit */
  bool stack_warnings_flag;        /* Should over/under flow warnings be printed? */
  bool break_on_overflow;          /* Should over flow cause a break? */
  bool break_on_underflow;         /* Should under flow cause a break? */

  Stack(void);
  virtual ~Stack() {}
  virtual void push(unsigned int);
  virtual unsigned int pop(void);
  virtual void reset(void) {pointer = 0;};  // %%% FIX ME %%% reset may need to change 
  // because I'm not sure how the stack is affected by a reset.
  virtual bool set_break_on_overflow(bool clear_or_set);
  virtual bool set_break_on_underflow(bool clear_or_set);

};


//---------------------------------------------------------
// W register

class WREG : public sfr_register
{
public:

  void put(unsigned int new_value);
  unsigned int get(void);
  WREG(void);
};

#include "tmr0.h"

//---------------------------------------------------------
// INDF

class INDF : public sfr_register
{
public:
  unsigned int fsr_mask;
  unsigned int base_address_mask1;
  unsigned int base_address_mask2;

  INDF(void);
  void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  unsigned int get(void);
  unsigned int get_value(void);
  virtual void initialize(void);
};

//---------------------------------------------------------
// OPTION_REG - 

class OPTION_REG : public sfr_register
{
public:

enum
  {
    PS0    = 1<<0,
    PS1    = 1<<1,
    PS2    = 1<<2,
    PSA    = 1<<3,
    T0SE   = 1<<4,
    T0CS   = 1<<5,
    BIT6   = 1<<6,
    BIT7   = 1<<7
  };

  unsigned int prescale;


  OPTION_REG(void);

  inline unsigned int get_prescale(void)
    {
      return value.get() & (PS0 | PS1 | PS2);
    }

  inline unsigned int get_psa(void)
    {
      return value.get() & PSA;
    }

  inline unsigned int get_t0cs(void)
    {
      return value.get() & T0CS;
    }

  inline unsigned int get_t0se(void)
    {
      return value.get() & T0SE;
    }

  void put(unsigned int new_value);

};


//---------------------------------------------------------
// PCL - Program Counter Low
//

class PCL : public sfr_register
{
public:

  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  virtual unsigned int get(void);
  virtual unsigned int get_value(void);

  PCL(void);
};

//---------------------------------------------------------
// PCLATH - Program Counter Latch High
//

class PCLATH : public sfr_register
{
public:
  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get(void);

  PCLATH(void);
};

//---------------------------------------------------------
// PCON - Power Control/Status Register
//
class PCON : public sfr_register
{
 public:

  enum {
    BOR = 1<<0,   // Brown Out Reset
    POR = 1<<1    // Power On Reset
  };

  unsigned int valid_bits;

  void put(unsigned int new_value);

  PCON(void);
};

//---------------------------------------------------------
// Watch Dog Timer
//

class WDT : public BreakpointObject
{
public:
  pic_processor *cpu;           // The cpu to which this wdt belongs.

  unsigned int
    value,
    prescale,
    break_point;
  guint64
    future_cycle;

  double timeout;   // When no prescaler is assigned
  bool   wdte;
  bool   warned;

  void put(unsigned int new_value);
  virtual void initialize(bool enable, double _timeout);
  void clear(void);
  virtual void callback(void);
  virtual void start_sleep(void);
  virtual void new_prescale(void);
  virtual void update(void);
  virtual void callback_print(void);
};


#endif
