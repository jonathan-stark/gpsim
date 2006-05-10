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

#ifndef __P12X_H__
#define __P12X_H__

#include "12bit-processors.h"
#include "pic-ioports.h"

class P12_I2C_EE;

class GPIO : public PicPortRegister
{
public:
  GPIO(const char *port_name, 
       unsigned int numIopins, 
       unsigned int enableMask);
  void setbit(unsigned int bit_number, bool new_value);
  void setPullUp ( bool bNewPU );

private:
  bool m_bPU;
};

class P12C508 : public  _12bit_processor
{
  public:

  GPIO            *m_gpio;
  PicTrisRegister *m_tris;
  sfr_register osccal;  // %%% FIX ME %%% Nothing's done with this.

  virtual PROCESSOR_TYPE isa(void){return _P12C508_;};
  virtual void create_symbols(void);

  virtual void enter_sleep();
  virtual unsigned int program_memory_size(void) const { return 0x200; };
  virtual void create_sfr_map(void);
  virtual void dump_registers(void);
  virtual void tris_instruction(unsigned int tris_register);
  virtual void reset(RESET_TYPE r);

  P12C508(void);
  static Processor *construct(void);
  void create(void);
  virtual void create_iopin_map(void);

  virtual unsigned int fsr_valid_bits(void)
    {
      return 0x1f;  // Assume only 32 register addresses 
    }

  virtual unsigned int fsr_register_page_bits(void)
    {
      return 0;     // Assume only one register page.
    }


  virtual void option_new_bits_6_7(unsigned int);

};


// A 12c509 is like a 12c508
class P12C509 : public P12C508
{
  public:

  virtual PROCESSOR_TYPE isa(void){return _P12C509_;};

  virtual unsigned int program_memory_size(void) const { return 0x400; };

  virtual void create_sfr_map(void);

  virtual unsigned int fsr_valid_bits(void)
    {
      return 0x3f;  // 64 registers in all (some are actually aliased)
    }

  virtual unsigned int fsr_register_page_bits(void)
    {
      return 0x20;  // 509 has 2 register banks
    }

  P12C509(void);
  static Processor *construct(void);
  void create(void);


};


// A 12CE518 is like a 12c508
class P12CE518 : public P12C508
{
  public:

  virtual PROCESSOR_TYPE isa(){return _P12CE518_;};
  virtual void tris_instruction(unsigned int tris_register);

  P12CE518();
  static Processor *construct();
  void create();
  virtual void create_iopin_map();
private:
  P12_I2C_EE *m_eeprom;
  
};


// A 12ce519 is like a 12ce518
class P12CE519 : public P12CE518
{
  public:

  virtual PROCESSOR_TYPE isa(void){return _P12CE519_;};

  virtual unsigned int program_memory_size(void) const { return 0x400; };

  virtual void create_sfr_map(void);

  virtual unsigned int fsr_valid_bits(void)
    {
      return 0x3f;  // 64 registers in all (some are actually aliased)
    }

  virtual unsigned int fsr_register_page_bits(void)
    {
      return 0x20;  // 519 has 2 register banks
    }

  P12CE519(void);
  static Processor *construct(void);
  void create(void);


};



// A 10F200 is like a 12c508
class P10F200 : public P12C508
{
  public:

  virtual PROCESSOR_TYPE isa(void){return _P10F200_;};
  virtual unsigned int program_memory_size(void) const { return 0x100; };

  P10F200(void);
  static Processor *construct(void);
  void create(void);
  virtual void create_iopin_map(void);

};


// A 10F202 is like a 10f200
class P10F202 : public P10F200
{
  public:

  virtual PROCESSOR_TYPE isa(void){return _P10F202_;};
  virtual unsigned int program_memory_size(void) const { return 0x200; };

  P10F202(void);
  static Processor *construct(void);
  void create(void);

};


#endif //  __P12X_H__
