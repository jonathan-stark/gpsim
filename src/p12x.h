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
#include "a2dconverter.h"


class P12_I2C_EE;

class GPIO : public PicPortRegister
{
public:
  GPIO(Processor *pCpu, const char *pName, const char *pDesc,
       unsigned int numIopins, 
       unsigned int enableMask);
  void setbit(unsigned int bit_number, char new_value);
  void setPullUp ( bool bNewPU );

private:
  bool m_bPU;
};

//--------------------------------------------------------
/*
 *         IN_SignalControl is used to set a pin as input
 *                 regardless of the setting to the TRIS register
 */
class IN_SignalControl : public SignalControl
{
public:
  IN_SignalControl(){}
  ~IN_SignalControl(){}
  char getState() { return '1'; }
  void release() { }
};

class P12bitBase : public  _12bit_processor
{
public:
  GPIO            *m_gpio;
  PicTrisRegister *m_tris;
  sfr_register osccal;  // %%% FIX ME %%% Nothing's done with this.

  virtual PROCESSOR_TYPE isa(){return _P12C508_;};
  virtual void create_symbols();

  virtual void enter_sleep();
  virtual void create_sfr_map();
  virtual void dump_registers();
  virtual void tris_instruction(unsigned int tris_register);
  virtual void reset(RESET_TYPE r);

  P12bitBase(const char *_name=0, const char *desc=0);
  virtual ~P12bitBase();
  static Processor *construct(const char *name);
  virtual void create_iopin_map();
  virtual void create_config_memory();

  virtual unsigned int fsr_valid_bits()
  {
    return 0x1f;  // Assume only 32 register addresses 
  }

  virtual unsigned int fsr_register_page_bits()
  {
    return 0;     // Assume only one register page.
  }


  virtual void option_new_bits_6_7(unsigned int);

  IN_SignalControl *m_IN_SignalControl;

};

class P12C508 : public  P12bitBase
{
public:

  P12C508(const char *_name=0, const char *desc=0);
  virtual ~P12C508();
  static Processor *construct(const char *name);
  virtual void create();
  virtual unsigned int program_memory_size() const { return 0x200; }

};

class P12F508 : public P12C508
{
public:

  P12F508(const char *_name=0, const char *desc=0);
  virtual ~P12F508();
  static Processor *construct(const char *name);
  virtual PROCESSOR_TYPE isa(){return _P12F508_;};
};

// A 12c509 is like a 12c508
class P12C509 : public P12C508
{
  public:

  virtual PROCESSOR_TYPE isa(){return _P12C509_;};

  virtual unsigned int program_memory_size() const { return 0x400; };

  virtual void create_sfr_map();

  virtual unsigned int fsr_valid_bits()
    {
      return 0x3f;  // 64 registers in all (some are actually aliased)
    }

  virtual unsigned int fsr_register_page_bits()
    {
      return 0x20;  // 509 has 2 register banks
    }

  P12C509(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  virtual void create();


};

class P12F509 : public P12C509
{
public:
  P12F509(const char *_name=0, const char *desc=0);
  virtual ~P12F509();
  static Processor *construct(const char *name);
  virtual PROCESSOR_TYPE isa(){return _P12F509_;}
};

// 12F510 - like a '509, but has an A2D and a comparator.
class P12F510 : public P12F509
{
public:
  P12F510(const char *_name=0, const char *desc=0);
  virtual ~P12F510();
  static Processor *construct(const char *name);
  virtual PROCESSOR_TYPE isa(){return _P12F510_;}
};

// A 12CE518 is like a 12c508
class P12CE518 : public P12C508
{
  public:

  virtual PROCESSOR_TYPE isa(){return _P12CE518_;};
  virtual void tris_instruction(unsigned int tris_register);

  P12CE518(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  virtual void create();
  virtual void create_iopin_map();
private:
  P12_I2C_EE *m_eeprom;
  
};


// A 12ce519 is like a 12ce518
class P12CE519 : public P12CE518
{
  public:

  virtual PROCESSOR_TYPE isa(){return _P12CE519_;};

  virtual unsigned int program_memory_size() const { return 0x400; };

  virtual void create_sfr_map();

  virtual unsigned int fsr_valid_bits()
    {
      return 0x3f;  // 64 registers in all (some are actually aliased)
    }

  virtual unsigned int fsr_register_page_bits()
    {
      return 0x20;  // 519 has 2 register banks
    }

  P12CE519(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  virtual void create();


};



//  10F200
class P10F200 : public P12bitBase
{
public:

  virtual PROCESSOR_TYPE isa(){return _P10F200_;};
  virtual unsigned int program_memory_size() const { return 0x100; };

  P10F200(const char *_name=0, const char *desc=0);
  virtual ~P10F200();

  static Processor *construct(const char *name);
  virtual void create();
  virtual void create_iopin_map();
  // GP2 can be driven by either FOSC/4, TMR 0, or the GP I/O driver
  virtual void updateGP2Source();

};


// A 10F202 is like a 10f200
class P10F202 : public P10F200
{
public:

  virtual PROCESSOR_TYPE isa(){return _P10F202_;};
  virtual unsigned int program_memory_size() const { return 0x200; };

  P10F202(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  virtual void create();

};

class CMCON0;
// A 10F204 is like a 10f200
class P10F204 : public P10F200
{
public:

  virtual PROCESSOR_TYPE isa(){return _P10F204_;};

  P10F204(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  virtual void create();
  // GP2 can be driven by either FOSC/4, COUT, TMR 0, or the GP I/O driver
  virtual void updateGP2Source();
protected:
  CMCON0 *m_cmcon0;
};

// A 10F220 is based on 10f200
class P10F220 : public P10F200
{
public:

  ADCON0_10 adcon0;
  ADCON1 adcon1;
  sfr_register  adres;

  virtual PROCESSOR_TYPE isa(){return _P10F220_;};

  P10F220(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  virtual void create();
  virtual void enter_sleep();
  virtual void exit_sleep();
protected:
};

// A 10F220 is like a 10f220
class P10F222 : public P10F220
{
public:

  virtual PROCESSOR_TYPE isa(){return _P10F222_;};

  P10F222(const char *_name=0, const char *desc=0);
  virtual unsigned int program_memory_size() const { return 0x200; };
  static Processor *construct(const char *name);
  virtual void create();
  // GP2 can be driven by either FOSC/4, TMR 0, or the GP I/O driver
  //virtual void updateGP2Source();
protected:
};

#endif //  __P12X_H__
