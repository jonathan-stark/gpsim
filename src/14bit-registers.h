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


class invalid_file_register;   // Forward reference

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

#include "ioports.h"

//---------------------------------------------------------
// FSR register
//

class FSR : public sfr_register
{
public:

  unsigned int register_page_bits;   /* Only used by the 12-bit core to define
                                        the valid paging bits in the FSR. */

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);

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

  unsigned int break_on_z,break_on_c;
  unsigned int rp_mask;
  unsigned int write_mask;    // Bits that instructions can modify

  Status_register(void);

  inline void put(unsigned int new_value);

  inline unsigned int get(void)
    {
      if(break_point) 
	bp.check_read_break(this);

      trace.register_read(address,value);
      return(value);
    }

  // Special member function to control just the Z bit

  inline void put_Z(unsigned int new_z)
    {
      if(break_on_z) 
	bp.check_write_break(this);

      value = (value & ~STATUS_Z) | ((new_z) ? STATUS_Z : 0);

      trace.register_write(address,value);
    }

  inline unsigned int get_Z(void)
    {
      if(break_on_z) 
	bp.check_read_break(this);

      trace.register_read(address,value);
      return( ( (value & STATUS_Z) == 0) ? 0 : 1);
    }


  // Special member function to control just the C bit
  void put_C(unsigned int new_c)
    {
      if(break_on_c) 
	bp.check_write_break(this);

      value = (value & ~STATUS_C) | ((new_c) ? STATUS_C : 0);
      trace.register_write(address,value);
    }

  unsigned int get_C(void)
    {
      if(break_on_c) 
	bp.check_read_break(this);

      trace.register_read(address,value);
      return( ( (value & STATUS_C) == 0) ? 0 : 1);
    }

  // Special member function to set Z, C, and DC

  inline void put_Z_C_DC(unsigned int new_value, unsigned int src1, unsigned int src2)
    {
      if(break_point) 
	bp.check_write_break(this);

      value = (value & ~ (STATUS_Z | STATUS_C | STATUS_DC)) |  
	((new_value & 0xff)   ? 0 : STATUS_Z)   |
	((new_value & 0x100)  ? STATUS_C : 0)   |
	(((new_value ^ src1 ^ src2)&0x10) ? STATUS_DC : 0);

      trace.register_write(address,value);
    }

  inline void put_Z_C_DC_for_sub(unsigned int new_value, unsigned int src1, unsigned int src2)
    {
      if(break_point) 
	bp.check_write_break(this);

      value = (value & ~ (STATUS_Z | STATUS_C | STATUS_DC)) |  
	((new_value & 0xff)   ? 0 : STATUS_Z)   |
	((new_value & 0x100)  ? 0 : STATUS_C)   |
	(((new_value ^ src1 ^ src2)&0x10) ? 0 : STATUS_DC);

      trace.register_write(address,value);
    }

  inline void put_PD(unsigned int new_pd)
    {
      if(break_point) 
	bp.check_write_break(this);

      value = (value & ~STATUS_PD) | ((new_pd) ? STATUS_PD : 0);

      trace.register_write(address,value);
    }

  inline unsigned int get_PD(void)
    {
      if(break_point) 
	bp.check_read_break(this);

      trace.register_read(address,value);
      return( ( (value & STATUS_PD) == 0) ? 0 : 1);
    }

  inline void put_TO(unsigned int new_to)
    {
      if(break_point) 
	bp.check_write_break(this);

      value = (value & ~STATUS_TO) | ((new_to) ? STATUS_TO : 0);

      trace.register_write(address,value);
    }

  inline unsigned int get_TO(void)
    {
      if(break_point) 
	bp.check_read_break(this);

      trace.register_read(address,value);
      return( ( (value & STATUS_TO) == 0) ? 0 : 1);
    }

  // Special member function to set Z, C, DC, OV, and N for the 18cxxx family

  // Special member function to control just the N bit
  void put_N_Z(unsigned int new_value)
    {
      if(break_point)
	bp.check_write_break(this);

      value = (value & ~(STATUS_Z | STATUS_N)) | 
	((new_value & 0xff )  ? 0 : STATUS_Z)   |
	((new_value & 0x80) ? STATUS_N : 0);

      trace.register_write(address,value);
    }

  void put_Z_C_N(unsigned int new_value)
    {
      if(break_point)
	bp.check_write_break(this);

      value = (value & ~(STATUS_Z | STATUS_C | STATUS_N)) | 
	((new_value & 0xff )  ? 0 : STATUS_Z)   |
	((new_value & 0x100)  ? STATUS_C : 0)   |
	((new_value & 0x80) ? STATUS_N : 0);

      trace.register_write(address,value);
    }

  inline void put_Z_C_DC_OV_N(unsigned int new_value, unsigned int src1, unsigned int src2)
    {
      if(break_point) 
	bp.check_write_break(this);

      value = (value & ~ (STATUS_Z | STATUS_C | STATUS_DC | STATUS_OV | STATUS_N)) |  
	((new_value & 0xff )  ? 0 : STATUS_Z)   |
	((new_value & 0x100)  ? STATUS_C : 0)   |
	(((new_value ^ src1 ^ src2)&0x10) ? STATUS_DC : 0) |
	((new_value ^ src1) & 0x80 ? STATUS_OV : 0) |
	((new_value & 0x80) ? STATUS_N : 0);

      trace.register_write(address,value);
    }

  inline void put_Z_C_DC_OV_N_for_sub(unsigned int new_value, unsigned int src1, unsigned int src2)
    {
      if(break_point) 
	bp.check_write_break(this);

      value = (value & ~ (STATUS_Z | STATUS_C | STATUS_DC)) |  
	((new_value & 0xff)   ? 0 : STATUS_Z)   |
	((new_value & 0x100)  ? 0 : STATUS_C)   |
	(((new_value ^ src1 ^ src2)&0x10) ? 0 : STATUS_DC) |
	((src1 > src2) ? STATUS_N : 0) |
	((new_value & 0x80)   ? STATUS_N : 0);

      trace.register_write(address,value);
    }

  // Special member function to control just the FSR mode
  void put_FSR0_mode(unsigned int new_value)
    {
      if(break_point)
	bp.check_write_break(this);

      value = (value & ~(STATUS_FSR0_MODE)) | 
	(new_value & 0x03 );

      trace.register_write(address,value);
    }

  unsigned int get_FSR0_mode(unsigned int new_value)
    {
      if(break_point)
	bp.check_write_break(this);

      trace.register_write(address,value);
      return( (value>>STATUS_FSR0_BIT) & 0x03);
    }

  void put_FSR1_mode(unsigned int new_value)
    {
      if(break_point)
	bp.check_write_break(this);

      value = (value & ~(STATUS_FSR1_MODE)) | 
	(new_value & 0x03 );

      trace.register_write(address,value);
    }

  unsigned int get_FSR1_mode(unsigned int new_value)
    {
      if(break_point)
	bp.check_write_break(this);

      trace.register_write(address,value);
      return( (value>>STATUS_FSR1_BIT) & 0x03);
    }

  

};


//---------------------------------------------------------
// Program Counter
//

class Program_Counter
{
public:
  pic_processor *cpu;
  unsigned int value;              /* pc's current value */
  unsigned int memory_size_mask; 
  unsigned int reset_address;      /* Value pc gets at reset */
  unsigned int pclath_mask;        /* pclath confines PC to banks */

  Program_Counter(void);
  void increment(void);
  void skip(void);
  void jump(unsigned int new_value);
  void interrupt(unsigned int new_value);
  void computed_goto(unsigned int new_value);
  void new_address(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  void reset(void)
    { 
      value = reset_address;
      trace.program_counter(value);
    };

  unsigned int get_next(void);

  //#ifdef HAVE_GUI
  XrefObject *xref;
  //  #endif

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
// INTCON - Interrupt control register

class INTCON : public sfr_register
{
public:

enum
{
  RBIF = 1<<0,
  INTF = 1<<1,
  T0IF = 1<<2,
  RBIE = 1<<3,
  INTE = 1<<4,
  T0IE = 1<<5,
  XXIE = 1<<6,    // Processor dependent
  GIE  = 1<<7
};



  INTCON(void);
  void set_T0IF(void);

  /*
  // Bit 6 of intcon depends on the processor that's being simulated, 
  // This generic function will get called whenever interrupt flag upon which bit 6 enables
  //  becomes true. (e.g. for the c84, this routine is called when EEIF goes high.)
  */
  inline void peripheral_interrupt(void)
    {
      if(  (value & (GIE | XXIE)) == (GIE | XXIE) ) bp.set_interrupt();
    };

  inline void set_gie(void)
    {
      value |= GIE;
      put(value);
    }

  inline void set_rbif(void)
    {
      put(get() | RBIF);
    }

  inline void set_intf(void)
    {
      put(get() | INTF);
    }

  inline void set_t0if(void)
    {
      put(get() | T0IF);
    }

  inline void set_rbie(void)
    {
      put(get() | RBIE);
    }

  inline void set_inte(void)
    {
      put(get() | INTE);
    }

  inline void set_t0ie(void)
    {
      put(get() | T0IE);
    }

  inline void clear_gie(void)
    {
      put(get() & ~GIE);
    }

  virtual bool check_peripheral_interrupt(void)
    {
      return 0;
    }

  virtual void put(unsigned int new_value)
    {

//      if(break_point) 
//	bp.check_write_break(this);


      value = new_value;
      trace.register_write(address,value);

  // Now let's see if there's a pending interrupt
  // The INTCON bits are:
  // GIE | ---- | TOIE | INTE | RBIE | TOIF | INTF | RBIF
  // There are 3 sources for interrupts, TMR0, RB0/INTF
  // and RBIF (RB7:RB4 change). If the corresponding interrupt
  // flag is set AND the corresponding interrupt enable bit
  // is set AND global interrupts (GIE) are enabled, THEN
  // there's an interrupt pending.
  // note: bit6 is not handled here because it is processor
  // dependent (e.g. EEIE for x84 and ADIE for x7x).

      if(value & GIE)
        {

          if( (((value>>3)&value) & (T0IF | INTF | RBIF)) )
            {
              trace.interrupt();
              bp.set_interrupt();
            }
          else if(value & XXIE)
            {
              if(check_peripheral_interrupt())
                {
                  trace.interrupt();
                  bp.set_interrupt();
                }
            }
        }
    }

};
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
      return value & (PS0 | PS1 | PS2);
    }

  inline unsigned int get_psa(void)
    {
      return value & PSA;
    }

  inline unsigned int get_t0cs(void)
    {
      return value & T0CS;
    }

  inline unsigned int get_t0se(void)
    {
      return value & T0SE;
    }

  void put(unsigned int new_value);

};


//---------------------------------------------------------
// PCL - Program Counter Low
//

class PCL : public sfr_register
{
public:

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get(void);

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

class EEPROM;    // forward reference to the EE prom peripheral
class EECON2;    // forward reference for EECON1
//---------------------------------------------------------
// Watch Dog Timer
//

class WDT : public BreakCallBack
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

//---------------------------------------------------------
// EECON1 - EE control register 1
//

class EECON1 : public sfr_register
{
public:
enum
{

 RD    = (1<<0),
 WR    = (1<<1),
 WREN  = (1<<2),
 WRERR = (1<<3),
 EEIF  = (1<<4),
 EEPGD = (1<<7)
};

 #define EECON1_VALID_BITS  (RD | WR | WREN | WRERR | EEIF)
  unsigned int valid_bits;
 
  EEPROM *eeprom;

  void put(unsigned int new_value);
  unsigned int get(void);

  EECON1(void);
};

//
// EECON2 - EE control register 2
//

class EECON2 : public sfr_register
{
public:

enum EE_STATES
{
  EENOT_READY,
  EEHAVE_0x55,
  EEREADY_FOR_WRITE,
  EEUNARMED,
  EEREAD
} eestate;

  EEPROM *eeprom;

  void put(unsigned int new_value);
  unsigned int get(void);
  void ee_reset(void) { eestate = EENOT_READY;};

  EECON2(void);
};

//
// EEDATA - EE data register
//

class EEDATA : public sfr_register
{
public:

  EEPROM *eeprom;

  void put(unsigned int new_value);
  unsigned int get(void);

  EEDATA(void);
};

//
// EEADR - EE address register
//

class EEADR : public sfr_register
{
public:

  EEPROM *eeprom;

  void put(unsigned int new_value);
  unsigned int get(void);

  EEADR(void);
};


class EEPROM :  public BreakCallBack
{
public:

#define  EPROM_WRITE_TIME  20

  char * name_str;
  _14bit_processor *cpu;

  EECON1 eecon1;            // The EEPROM consists of 4 control registers
  EECON2 eecon2;            // on the F84 and 6 on the F877
  EEDATA eedata;
  EEADR  eeadr;

  file_register **rom;          //  and the data area.
  unsigned int rom_size;
  unsigned int wr_adr,wr_data;  // latched adr and data for eewrites.
  unsigned int rd_adr;          // latched adr for eereads.
  unsigned int abp;             // break point number that's set during eewrites

  EEPROM(void);
  void reset(RESET_TYPE);
  virtual void callback(void);
  virtual void start_write(void);
  virtual void write_is_complete(void);
  virtual void start_program_memory_read(void);  
  virtual void initialize(unsigned int new_rom_size);
  virtual file_register *get_register(unsigned int address);

  void dump(void);

};

class PIR1;
class EEPROM_62x : public EEPROM
{
 public:

  PIR1 *pir1;

  // the 16f628 eeprom is identical to the 16f84 eeprom except
  // for the size and the location of EEIF. The size is taken
  // care of when the '628 is constructed, the EEIF is taken
  // care of here:

  virtual void write_is_complete(void);

};

class EEPROM_87x : public EEPROM
{
 public:

  EEDATA eedatah;
  EEADR  eeadrh;

  virtual void start_write(void);
  virtual void callback(void);
  virtual void start_program_memory_read(void);
  virtual void initialize(unsigned int new_rom_size);
};

#endif
