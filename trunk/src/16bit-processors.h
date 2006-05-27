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
#include "value.h"

// forward references

extern instruction *disasm16 (pic_processor *cpu, unsigned int address, unsigned int inst);

class ConfigMemory;


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

  static const int CONFIG1L = 0x300000;
  static const int CONFIG1H = 0x300001;
  static const int CONFIG2L = 0x300002;
  static const int CONFIG2H = 0x300003;
  static const int CONFIG3L = 0x300004;
  static const int CONFIG3H = 0x300005;
  static const int CONFIG4L = 0x300006;
  static const int CONFIG4H = 0x300007;
  static const int CONFIG5L = 0x300008;
  static const int CONFIG5H = 0x300009;
  static const int CONFIG6L = 0x30000A;
  static const int CONFIG6H = 0x30000B;
  static const int CONFIG7L = 0x30000C;
  static const int CONFIG7H = 0x30000D;

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
  virtual void create_sfr_map(void);
  virtual void create_config_memory();

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
  //virtual void set_out_of_range_pm(unsigned int address, unsigned int value);

  virtual void create_iopin_map(void);

  virtual int  map_pm_address2index(int address) {return address/2;};
  virtual int  map_pm_index2address(int index) {return index*2;};
  virtual unsigned int get_program_memory_at_address(unsigned int address);
  virtual unsigned int get_config_word(unsigned int address);
  virtual bool set_config_word(unsigned int address, unsigned int cfg_word);
  virtual unsigned int configMemorySize() { return CONFIG7H-CONFIG1L+1; }


  static pic_processor *construct(void);
  _16bit_processor(void);

  unsigned int getCurrentDisasmAddress() { return m_current_disasm_address;}
  unsigned int getCurrentDisasmIndex()   { return m_current_disasm_address/2;}
  void setCurrentDisasmAddress(unsigned a) { m_current_disasm_address =a; }
protected:
  unsigned int m_current_disasm_address;  // Used only when .hex/.cod files are loaded
  ConfigMemory **m_configMemory;
};

#define cpu16 ( (_16bit_processor *)cpu)

//------------------------------------------------------------------------
class ConfigMemory : public Integer
{
public:
  ConfigMemory(const char *_name, unsigned int default_val, const char *desc,
	       _16bit_processor *pCpu, unsigned int addr);
  //unsigned int get();
  //virtual void set(unsigned int);

private:
  _16bit_processor *m_pCpu;
  unsigned int m_addr;
  //unsigned int m_value;
};

#endif
