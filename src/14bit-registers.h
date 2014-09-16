/*
   Copyright (C) 1998 T. Scott Dattalo
   Copyright (C) 2013 Roy R. Rankin
		 

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
#include "intcon.h"

class stimulus;  // forward reference
class IOPIN;
class source_stimulus;
class Stimulus_Node;
class PORTB;
class pic_processor;

#include "ioports.h"

//---------------------------------------------------------
// BORCON register
//

class BORCON : public sfr_register
{
public:
  BORCON(Processor *, const char *pName, const char *pDesc=0);

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);

};
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

  Stack(Processor *);
  virtual ~Stack() {}
  virtual bool push(unsigned int);
  virtual bool stack_overflow();
  virtual bool stack_underflow();
  virtual unsigned int pop();
  virtual void reset(RESET_TYPE r) {pointer = 0;};  // %%% FIX ME %%% reset may need to change 
  // because I'm not sure how the stack is affected by a reset.
  virtual bool set_break_on_overflow(bool clear_or_set);
  virtual bool set_break_on_underflow(bool clear_or_set);
  virtual unsigned int get_tos();
  virtual void put_tos(unsigned int);


  bool STVREN;
  Processor *cpu;

};

class STKPTR : public sfr_register
{
public:

  enum {
	STKUNF = 1<<6,
	STKOVF = 1<<7
  };
  STKPTR(Processor *, const char *pName, const char *pDesc=0);

  Stack *stack;
  void put_value(unsigned int new_value);
  void put(unsigned int new_value);
};

class TOSL : public sfr_register
{
public:
  TOSL(Processor *, const char *pName, const char *pDesc=0);

  Stack *stack;

  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  virtual unsigned int get();
  virtual unsigned int get_value();
      
};

class TOSH : public sfr_register
{
public:
  TOSH(Processor *, const char *pName, const char *pDesc=0);

  Stack *stack;

  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  virtual unsigned int get();
  virtual unsigned int get_value();
      
};
//
// Stack for enhanced 14 bit porcessors
//
class Stack14E : public Stack
{
public:
  STKPTR stkptr;
  TOSL   tosl;
  TOSH   tosh;

  Stack14E(Processor *);
  ~Stack14E();

  virtual void reset(RESET_TYPE r);
  virtual unsigned int pop();
  virtual bool push(unsigned int address);
  virtual bool stack_overflow();
  virtual bool stack_underflow();

#define NO_ENTRY 0x20
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

class Indirect_Addressing14;   // Forward reference

//---------------------------------------------------------
// FSR registers

class FSRL14 : public sfr_register
{
public:
  FSRL14(Processor *, const char *pName, const char *pDesc, Indirect_Addressing14 *pIAM);
  void put(unsigned int new_value);
  void put_value(unsigned int new_value);

protected:
  Indirect_Addressing14  *iam;
};

class FSRH14 : public sfr_register
{
 public:
  FSRH14(Processor *, const char *pName, const char *pDesc, Indirect_Addressing14 *pIAM);

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
      
protected:
  Indirect_Addressing14  *iam;
};

class INDF14 : public sfr_register
{
 public:
  INDF14(Processor *, const char *pName, const char *pDesc, Indirect_Addressing14 *pIAM);

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get();
  unsigned int get_value();
      
protected:
  Indirect_Addressing14  *iam;
};

class Indirect_Addressing14
{
public:
  Indirect_Addressing14(pic_processor *cpu, const string &n);

  pic_processor *cpu;

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
  FSRL14    fsrl;
  FSRH14    fsrh;
  INDF14    indf;

  //void init(_16bit_processor *new_cpu);
  void put(unsigned int new_value);
  unsigned int get();
  unsigned int get_value();
  void put_fsr(unsigned int new_fsr);
  unsigned int get_fsr_value(){return (fsr_value & 0xfff);};
  void update_fsr_value();

  /* bool is_indirect_register(unsigned int reg_address)
   *
   * The purpose of this routine is to determine whether or not the
   * 'reg_address' is the address of an indirect register. This is
   * used by the 'put' and 'get' functions of the indirect registers.
   * Indirect registers are forbidden access to other indirect registers.
   * (Although double indirection in a single instruction cycle would
   * be powerful!).
   */

  inline bool is_indirect_register(unsigned int reg_address)
    {
	unsigned int bank_address = reg_address % 0x80;
      if(bank_address == 0 || bank_address == 1 || bank_address == 4 ||
	 bank_address == 5 || bank_address == 6 || bank_address == 7)
	return 1;
      return 0;
    }


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
    BOR = 1<<0,   // clear on Brown Out Reset
    POR = 1<<1,    // clear on Power On Reset
    RI  = 1<<2,	   // clear on Reset instruction 
    RMCLR = 1<<3,  // clear if hardware MCLR occurs
    SBOREN = 1<<4, //  Software BOR Enable bit
    ULPWUE = 1<<5,  // Ultra Low-Power Wake-up Enable bit
    STKUNF = 1<<6,  // Stack undeflow
    STKOVF = 1<<7   // Stack overflow
  };

  unsigned int valid_bits;

  void put(unsigned int new_value);

  PCON(Processor *, const char *pName, const char *pDesc=0, 
			unsigned int bitMask=0x03);
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
    : sfr_register(pCpu,pName,pDesc), valid_bits(6), osccon(0)
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
  float base_freq;
                                                                                
  OSCCAL(Processor *pCpu, const char *pName, const char *pDesc, unsigned int bitMask)
    : sfr_register(pCpu,pName,pDesc), base_freq(0.)
  {
      mValidBits=bitMask;  // Can't use initialiser for parent class members 
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
    : sfr_register(pCpu,pName,pDesc), valid_bits(7), osctune(0)
  {
  }
};


class OSCSTAT : public  sfr_register
{
 public:
  void put(unsigned int new_value){;}

  enum
  {
	HFIOFS = 1<<0,
	LFIOFR = 1<<1,
	MFIOFR = 1<<2,
	HFIOFL = 1<<3,
	HFIOFR = 1<<4,
	OSTS   = 1<<5,
	PLLR   = 1<<6,
	T1OSCR = 1<<7
  };
  OSCSTAT(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu,pName,pDesc) {}
};
class OSCCON_2 : public  sfr_register,  public TriggerObject
{
 public:
  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  virtual void callback();
  virtual bool set_rc_frequency();
  virtual void set_osctune(OSCTUNE *new_osctune) { osctune = new_osctune;}
  virtual void set_oscstat(OSCSTAT *_oscstat) { oscstat = _oscstat;}
  virtual void set_config(unsigned int cfg_fosc, bool cfg_ieso);
  virtual void set_callback();
  virtual void wake();
  unsigned int 	valid_bits;
  unsigned int 	mode;
  guint64	next_callback;
  OSCTUNE 	*osctune;
  OSCSTAT 	*oscstat;
  unsigned int  config_fosc;	// FOSC bits from config word
  bool		config_ieso;	//internal/external switchover bit from config word

  enum MODE
  {
	LF = 0,
	MF,
	HF,
	T1OSC,
	EC,	// external clock, always stable
	OST,	// use on startup timer
	PLL = 0x10
  };
                                                                                
  enum {
    SCS0   = 1<<0,
    SCS1   = 1<<1,
    IRCF0  = 1<<3,
    IRCF1  = 1<<4,
    IRCF2  = 1<<5,
    IRCF3  = 1<<6,
    SPLLEN = 1<<7
  };
                                                                                
  OSCCON_2(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu,pName,pDesc), valid_bits(0xfb), 
	next_callback(0), osctune(0), oscstat(0)
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

 IOC(Processor *pCpu, const char *pName, const char *pDesc, unsigned int _valid_bits = 0xff)
    : sfr_register(pCpu,pName,pDesc)
  {
      mValidBits=_valid_bits;
  }

  virtual void put(unsigned int new_value)
  {
    unsigned int masked_value = new_value & mValidBits;

    get_trace().raw(write_trace.get() | value.get());
    value.put(masked_value);
  }

};

// Interrupt-On-Change  Register
class IOCxF : public IOC
{
public:

  IOCxF(Processor *pCpu, const char *pName, const char *pDesc, unsigned int _valid_bits = 0xff)
    : IOC(pCpu,pName,pDesc, _valid_bits), intcon(0)
  {
  }

  void set_intcon(INTCON *new_value) { intcon = new_value; }
  void put(unsigned int new_value);

protected:
    INTCON  *intcon;

};

class PicPortRegister;
// WPU set weak pullups on pin by pin basis
//
class WPU  : public  sfr_register
{

public:
  PicPortRegister *wpu_gpio;
  bool wpu_pu;

  void put(unsigned int new_value);
  void set_wpu_pu(bool pullup_enable);

  WPU(Processor *pCpu, const char *pName, const char *pDesc, PicPortRegister* gpio, unsigned int mask=0x37)
    : sfr_register(pCpu,pName,pDesc), wpu_gpio(gpio), wpu_pu(false)
  {
      mValidBits=mask;  // Can't use initialiser for parent class members 
  }
};

class CPSCON1;
class T1CON_G;
class CPS_stimulus;

class CPSCON0  : public  sfr_register,  public TriggerObject
{

public:

  enum {
	T0XCS	= 1<<0,		// Timer0 External Clock Source Select bit
	CPSOUT	= 1<<1,		// Capacitive Sensing Oscillator Status bit
	CPSRNG0	= 1<<2,		// Capacitive Sensing Current Range bits
	CPSRNG1	= 1<<3,		
	CPSRM	= 1<<6,		// Capacitive Sensing Reference Mode bit
	CPSON	= 1<<7		// CPS Module Enable bit
	};

  void put(unsigned int new_value);
  void set_chan(unsigned int _chan);
  void calculate_freq();
  void set_pin(unsigned int _chan, PinModule *_pin) { pin[_chan] = _pin;}
  void set_DAC_volt(double);
  void set_FVR_volt(double);
  void callback();

  CPSCON0(Processor *pCpu, const char *pName, const char *pDesc=0);
  ~CPSCON0();

  TMR0	*m_tmr0;
  T1CON_G  *m_t1con_g;

private:
  unsigned int 	chan;
  PinModule 	*pin[16];
  double	DAC_voltage;
  double	FVR_voltage;
  guint64	future_cycle;
  int		period;
  CPS_stimulus  *cps_stimulus;

};
class CPSCON1  : public  sfr_register
{

public:

  void put(unsigned int new_value);

  CPSCON1(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc), m_cpscon0(0)
  {
	mValidBits = 0x03;
  }

  CPSCON0	*m_cpscon0;
};
class CPS_stimulus : public stimulus
{
   public:

	CPS_stimulus(CPSCON0 *arg, const char *n=0,
           double _Vth=0.0, double _Zth=1e12
           );

    CPSCON0 *m_cpscon0;

     virtual void   set_nodeVoltage(double v);

};

class SR_MODULE;

// SR LATCH CONTROL 0 REGISTER
class SRCON0  : public  sfr_register
{

public:
  enum {
      SRPR   = 1<<0,	// Pulse Reset Input of the SR Latch bit
      SRPS   = 1<<1,	// Pulse Set Input of the SR Latch bit
      SRNQEN = 1<<2,	// Latch Not Q Output Enable bit
      SRQEN  = 1<<3,	// Latch Q Output Enable bit
      SRCLK0 = 1<<4,    // Latch Clock Divider bits
      SRCLK1 = 1<<5,    // Latch Clock Divider bits
      SRCLK2 = 1<<6,    // Latch Clock Divider bits
      SRLEN  = 1<<7,	// Latch Enable bit
      CLKMASK = SRCLK0|SRCLK1|SRCLK2,
      CLKSHIFT = 4

  };

    SRCON0(Processor *pCpu, const char *pName, const char *pDesc, SR_MODULE *_sr_module);
    void put(unsigned int new_value);

private:
   SR_MODULE 	*m_sr_module;
};


//  SR LATCH CONTROL 1 REGISTER
//
class SRCON1  : public  sfr_register
{

public:
  enum {
      SRRC1E  = 1<<0,	// Latch C1 Reset Enable bit
      SRRC2E  = 1<<1,	// Latch C2 Reset Enable bit
      SRRCKE  = 1<<2,   // Latch Reset Clock Enable bit
      SRRPE   = 1<<3,	// Latch Peripheral Reset Enable bit
      SRSC1E  = 1<<4,	// Latch C1 Set Enable bit
      SRSC2E  = 1<<5,	// Latch C2 Set Enable bit
      SRSCKE  = 1<<6,   // Latch Set Clock Enable bit
      SRSPE   = 1<<7    // Latch Peripheral Set Enable bit
  };

    SRCON1(Processor *pCpu, const char *pName, const char *pDesc, SR_MODULE *m_sr_module);
    void put(unsigned int new_value);
    void set_ValidBits(unsigned int validbits) { mValidBits = validbits;}

private:
   SR_MODULE 	*m_sr_module;
   unsigned int	mValidBits;
};

class SRinSink;

class SR_MODULE: public TriggerObject
{

public:

    SR_MODULE(Processor *);
    ~SR_MODULE();

    void	update();
    SRCON0 	srcon0;
    SRCON1 	srcon1;

    void 	pulse_reset() { state_reset = true;}
    void 	pulse_set() { state_set = true;}
    void	clock_diff(unsigned int);
    void	clock_enable();
    void	clock_disable();
    void	syncC1out(bool val);
    void	syncC2out(bool val);
    void	setPins(PinModule *, PinModule *,PinModule *);
    void	setState(char);
    void	Qoutput();
    void	NQoutput();

protected:

    void   callback();

    Processor   *cpu;
    guint64	future_cycle;
    bool	state_set;
    bool	state_reset;
    bool	state_Q;
    unsigned int srclk;
    bool	syncc1out;	// Synced output from comparator 1
    bool	syncc2out;	// Synced output from comparator 2
    PinModule 	*SRI_pin;
    PinModule 	*SRQ_pin;
    PinModule 	*SRNQ_pin;
    bool	SRI;		// state of input pin
    SRinSink	*m_SRinSink;
    PeripheralSignalSource *m_SRQsource;
    PeripheralSignalSource *m_SRNQsource;
};

#endif
