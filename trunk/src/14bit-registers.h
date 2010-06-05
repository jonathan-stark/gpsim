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

#include <stdio.h>


class InvalidRegister;   // Forward reference

#include "trace.h"

#ifndef __14_BIT_REGISTERS_H__
#define __14_BIT_REGISTERS_H__


class _14bit_processor;

#include "breakpoints.h"

#include "rcon.h"
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
  FSR(Processor *, const char *pName, const char *pDesc=0);
  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  virtual unsigned int get();
  virtual unsigned int get_value();

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
  FSR_12(Processor *, const char *pName, 
         unsigned int _register_page_bits, unsigned int _valid_bits);

  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  virtual unsigned int get();
  virtual unsigned int get_value();

};


//---------------------------------------------------------
// Status register
//
class RCON;

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
  RCON *rcon;

  Status_register(Processor *, const char *pName, const char *pDesc=0);
  void reset(RESET_TYPE r);

  void set_rcon(RCON *p_rcon) { rcon = p_rcon;}

  virtual void put(unsigned int new_value);

  inline unsigned int get()
  {
    get_trace().raw(read_trace.get() | value.get());
    return(value.get());
  }

  // Special member function to control just the Z bit

  inline void put_Z(unsigned int new_z)
  {
    get_trace().raw(write_trace.get() | value.get());
    value.put((value.get() & ~STATUS_Z) | ((new_z) ? STATUS_Z : 0));
  }

  inline unsigned int get_Z()
  {
    get_trace().raw(read_trace.get() | value.get());
    return( ( (value.get() & STATUS_Z) == 0) ? 0 : 1);
  }


  // Special member function to control just the C bit
  void put_C(unsigned int new_c)
  {
    get_trace().raw(write_trace.get() | value.get());
    value.put((value.get() & ~STATUS_C) | ((new_c) ? STATUS_C : 0));
  }

  unsigned int get_C()
  {
    get_trace().raw(read_trace.get() | value.get());
    return( ( (value.get() & STATUS_C) == 0) ? 0 : 1);
  }

  // Special member function to set Z, C, and DC

  inline void put_Z_C_DC(unsigned int new_value, unsigned int src1, unsigned int src2)
  {
    get_trace().raw(write_trace.get() | value.get());

    value.put((value.get() & ~ (STATUS_Z | STATUS_C | STATUS_DC)) |  
	      ((new_value & 0xff)   ? 0 : STATUS_Z)   |
	      ((new_value & 0x100)  ? STATUS_C : 0)   |
	      (((new_value ^ src1 ^ src2)&0x10) ? STATUS_DC : 0));

  }

  inline void put_Z_C_DC_for_sub(unsigned int new_value, unsigned int src1, unsigned int src2)
  {

    get_trace().raw(write_trace.get() | value.get());

    value.put((value.get() & ~ (STATUS_Z | STATUS_C | STATUS_DC)) |  
	      ((new_value & 0xff)   ? 0 : STATUS_Z)   |
	      ((new_value & 0x100)  ? 0 : STATUS_C)   |
	      (((new_value ^ src1 ^ src2)&0x10) ? 0 : STATUS_DC));

  }

  inline void put_PD(unsigned int new_pd)
  {
    if (rcon)
        rcon->put_PD(new_pd);
    else
    {
        get_trace().raw(write_trace.get() | value.get());
        value.put((value.get() & ~STATUS_PD) | ((new_pd) ? STATUS_PD : 0));
    }  
}

  inline unsigned int get_PD()
  {
    if (rcon)
	return (rcon->get_PD());
    else
    {
        get_trace().raw(read_trace.get() | value.get());
        return( ( (value.get() & STATUS_PD) == 0) ? 0 : 1);
    }
  }

  inline void put_TO(unsigned int new_to)
  {
    if (rcon)
	rcon->put_TO(new_to);
    else
    {
        get_trace().raw(write_trace.get() | value.get());
        value.put((value.get() & ~STATUS_TO) | ((new_to) ? STATUS_TO : 0));
    }
  }

  inline unsigned int get_TO()
  {
    if (rcon)
	return(rcon->get_TO());
    else
    {
        get_trace().raw(read_trace.get() | value.get());
        return( ( (value.get() & STATUS_TO) == 0) ? 0 : 1);
    }
  }

  // Special member function to set Z, C, DC, OV, and N for the 18cxxx family

  // Special member function to control just the N bit
  void put_N_Z(unsigned int new_value)
  {
    get_trace().raw(write_trace.get() | value.get());
    value.put((value.get() & ~(STATUS_Z | STATUS_N)) | 
	      ((new_value & 0xff )  ? 0 : STATUS_Z)   |
	      ((new_value & 0x80) ? STATUS_N : 0));
  }

  void put_Z_C_N(unsigned int new_value)
  {
    get_trace().raw(write_trace.get() | value.get());
    value.put((value.get() & ~(STATUS_Z | STATUS_C | STATUS_N)) | 
	      ((new_value & 0xff )  ? 0 : STATUS_Z)   |
	      ((new_value & 0x100)  ? STATUS_C : 0)   |
	      ((new_value & 0x80) ? STATUS_N : 0));
  }

  inline void put_Z_C_DC_OV_N(unsigned int new_value, unsigned int src1, unsigned int src2)
  {
    get_trace().raw(write_trace.get() | value.get());
    value.put((value.get() & ~ (STATUS_Z | STATUS_C | STATUS_DC | STATUS_OV | STATUS_N)) |  
	      ((new_value & 0xff )  ? 0 : STATUS_Z)   |
	      ((new_value & 0x100)  ? STATUS_C : 0)   |
	      (((new_value ^ src1 ^ src2)&0x10) ? STATUS_DC : 0) |
	      ((new_value ^ src1) & 0x80 ? STATUS_OV : 0) |
	      ((new_value & 0x80) ? STATUS_N : 0));
  }

  inline void put_Z_C_DC_OV_N_for_sub(unsigned int new_value, unsigned int src1, unsigned int src2)
  {
    get_trace().raw(write_trace.get() | value.get());
    value.put((value.get() & ~ (STATUS_Z | STATUS_C | STATUS_DC | STATUS_OV | STATUS_N)) |  
	      ((new_value & 0xff)   ? 0 : STATUS_Z)   |
	      ((new_value & 0x100)  ? 0 : STATUS_C)   |
	      (((new_value ^ src1 ^ src2)&0x10) ? 0 : STATUS_DC) |
	      ((((src1 & ~src2 & ~new_value) | (new_value & ~src1 & src2)) & 0x80) ? STATUS_OV : 0) |
	      ((new_value & 0x80)   ? STATUS_N : 0));
  }

  // Special member function to control just the FSR mode
  void put_FSR0_mode(unsigned int new_value)
  {
    get_trace().raw(write_trace.get() | value.get());
    value.put((value.get() & ~(STATUS_FSR0_MODE)) | 
	      (new_value & 0x03 ));
  }

  unsigned int get_FSR0_mode(unsigned int new_value)
  {
    get_trace().raw(write_trace.get() | value.get());
    return( (value.get()>>STATUS_FSR0_BIT) & 0x03);
  }

  void put_FSR1_mode(unsigned int new_value)
  {
    get_trace().raw(write_trace.get() | value.get());
    value.put((value.get() & ~(STATUS_FSR1_MODE)) | 
	      (new_value & 0x03 ));
  }

  unsigned int get_FSR1_mode(unsigned int new_value)
  {
    get_trace().raw(read_trace.get() | value.get());
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

  Stack();
  virtual ~Stack() {}
  virtual void push(unsigned int);
  virtual unsigned int pop();
  virtual void reset() {pointer = 0;};  // %%% FIX ME %%% reset may need to change 
  // because I'm not sure how the stack is affected by a reset.
  virtual bool set_break_on_overflow(bool clear_or_set);
  virtual bool set_break_on_underflow(bool clear_or_set);

};


//---------------------------------------------------------
// W register
class WTraceType;

class WREG : public sfr_register
{
public:

  WREG(Processor *, const char *pName, const char *pDesc=0);
  ~WREG();
protected:
  WTraceType *m_tt;
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

  INDF(Processor *, const char *pName, const char *pDesc=0);
  void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  unsigned int get();
  unsigned int get_value();
  virtual void initialize();
};



//---------------------------------------------------------
// PCL - Program Counter Low
//

class PCL : public sfr_register
{
public:

  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  virtual unsigned int get();
  virtual unsigned int get_value();
  virtual void reset(RESET_TYPE r);

  PCL(Processor *, const char *pName, const char *pDesc=0);
};

//---------------------------------------------------------
// PCLATH - Program Counter Latch High
//

class PCLATH : public sfr_register
{
public:
  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get();

  PCLATH(Processor *, const char *pName, const char *pDesc=0);
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

  PCON(Processor *, const char *pName, const char *pDesc=0);
};

class OSCCON;
class OSCTUNE : public  sfr_register
{
 public:

  void put(unsigned int new_value);
  virtual void set_osccon(OSCCON *new_osccon) { osccon = new_osccon;}
  unsigned int valid_bits;
                                                                                
  enum {
    TUN0 = 1<<0,
    TUN1 = 1<<1,
    TUN2 = 1<<2,
    TUN3 = 1<<3,
    TUN4 = 1<<4,
    TUN5 = 1<<5,
  };
  OSCCON *osccon;
                                                                                
  OSCTUNE(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu,pName,pDesc), valid_bits(6)
  {
  }
};


// This class is used to trim the frequency of the internal RC clock
//  111111 - Max freq
//  100000 - no adjustment
//  000000 - mix freq
class OSCCAL : public  sfr_register
{
 public:

  void put(unsigned int new_value);
  void set_freq(float base_freq);
  unsigned int bit_mask;
  float base_freq;
                                                                                
  OSCCAL(Processor *pCpu, const char *pName, const char *pDesc, unsigned int bitMask)
    : sfr_register(pCpu,pName,pDesc), bit_mask(bitMask), base_freq(0.)
  {
  }
};

class OSCCON : public  sfr_register,  public TriggerObject
{
 public:
  void put(unsigned int new_value);
  virtual void callback();
  virtual bool set_rc_frequency();
  virtual void set_osctune(OSCTUNE *new_osctune) { osctune = new_osctune;}
  unsigned int valid_bits;
  OSCTUNE *osctune;
                                                                                
  enum {
    SCS0 = 1<<0,
    SCS1 = 1<<1,
    IOFS = 1<<2,
    OSTS = 1<<3,
    IRCF0 = 1<<4,
    IRCF1 = 1<<5,
    IRCF2 = 1<<6
  };
                                                                                
  OSCCON(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu,pName,pDesc), valid_bits(7)
  {
  }
};

class WDTCON : public  sfr_register
{
 public:
                                                                                
  unsigned int valid_bits;
                                                                                
  enum {
    WDTPS3 = 1<<4,
    WDTPS2 = 1<<3,
    WDTPS1 = 1<<2,
    WDTPS0 = 1<<1,
    SWDTEN = 1<<0
  };
                                                                                
  WDTCON(Processor *pCpu, const char *pName, const char *pDesc, unsigned int bits)
    : sfr_register(pCpu,pName,pDesc), valid_bits(bits)
  {
  }
  virtual void put(unsigned int new_value);
  virtual void reset(RESET_TYPE r);
                                                                                
};




// Interrupt-On-Change GPIO Register
class IOC :  public sfr_register
{
public:
 unsigned int bit_mask;

 IOC(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu,pName,pDesc), bit_mask(0x3f)
  {
  }

  void put(unsigned int new_value)
  {
    unsigned int masked_value = new_value & bit_mask;

    get_trace().raw(write_trace.get() | value.get());
    value.put(masked_value);
  }

};

class PicPortRegister;
// WPU set weak pullups on pin by pin basis
//
class WPU  : public  sfr_register
{

public:
  unsigned int bit_mask;
  PicPortRegister *wpu_gpio;
  bool wpu_pu;

  void put(unsigned int new_value);
  void set_wpu_pu(bool pullup_enable);

  WPU(Processor *pCpu, const char *pName, const char *pDesc, PicPortRegister* gpio, unsigned int mask=0x37)
    : sfr_register(pCpu,pName,pDesc), bit_mask(mask), wpu_gpio(gpio), wpu_pu(false)
  {
  }
};
#endif
