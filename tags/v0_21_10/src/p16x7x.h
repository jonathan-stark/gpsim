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

#ifndef __P16X7X_H__
#define __P16X7X_H__


#include "p16x6x.h"    /* The '7x stuff is like '6x stuff with a/d converters */
#include "pir.h"

//---------------------------------------------------------
// ADRES
//

class ADRES : public sfr_register
{
public:

  void put(int new_value);
};


//---------------------------------------------------------
// ADCON1
//

class ADCON1 : public sfr_register
{
public:

  enum PCFG_bits
    {
      PCFG0 = 1<<0,
      PCFG1 = 1<<1,
      PCFG2 = 1<<2,
      PCFG3 = 1<<3,   // 16f87x etc.
      ADFM  = 1<<7    // Format Result select bit 
    };

  ADCON1();

  void setChannelConfiguration(unsigned int cfg, unsigned int bitMask);
  void setVrefLoConfiguration(unsigned int cfg, unsigned int channel);
  void setVrefHiConfiguration(unsigned int cfg, unsigned int channel);

  double getChannelVoltage(unsigned int channel);
  double getVrefHi();
  double getVrefLo();

  void setValidCfgBits(unsigned int m) {mValidCfgBits = m; }
  void setNumberOfChannels(unsigned int);
  void setIOPin(unsigned int, PinModule *);

private:
  PinModule **m_AnalogPins;
  unsigned int m_nAnalogChannels;
  unsigned int mValidCfgBits;

  static const int cMaxConfigurations=16;

  /* Vrefhi/lo_position - this is an array that tells which
   * channel the A/D converter's  voltage reference(s) are located. 
   * The index into the array is formed from the PCFG bits. 
   * The encoding is as follows:
   *   0-7:  analog channel containing Vref(s)
   *     8:  The reference is internal (i.e. Vdd)
   *  0xff:  The analog inputs are configured as digital inputs
   */
  unsigned int Vrefhi_position[cMaxConfigurations];
  unsigned int Vreflo_position[cMaxConfigurations];

  /* configuration bits is an array of 8-bit masks definining whether or not
   * a given channel is analog or digital
   */
  unsigned int m_configuration_bits[cMaxConfigurations];

};


//---------------------------------------------------------
// ADCON0
//

class ADCON0 : public sfr_register, public TriggerObject
{
public:

  ADRES *adres;
  ADRES *adresl;
  ADCON1 *adcon1;
  INTCON *intcon;

  enum
    {
      ADON = 1<<0,
      ADIF = 1<<1,
      GO   = 1<<2,
      CHS0 = 1<<3,
      CHS1 = 1<<4,
      ADCS0 = 1<<6,
      ADCS1 = 1<<7
    };

  enum AD_states
    {
      AD_IDLE,
      AD_ACQUIRING,
      AD_CONVERTING
    };

  ADCON0();

  void start_conversion();
  void stop_conversion();
  virtual void set_interrupt();
  virtual void callback();
  void put(unsigned int new_value);
  void put_conversion();

  void setA2DBits(unsigned int);
private:
  double m_dSampledVoltage;
  double m_dSampledVrefHi;
  double m_dSampledVrefLo;
  unsigned int m_A2DScale;
  guint64 future_cycle;
  unsigned int ad_state;
  unsigned int Tad_2;
  unsigned int channel_mask;
};

//---------------------------------------------------------
// ADCON0_withccp
//

class ADCON0_withccp : public ADCON0
{
public:

  PIR_SET_2   *pir_set;
  virtual void set_interrupt();

};


//---------------------------------------------------------

class P16C71 :  public P16C61
{
 public:

  ADCON0 adcon0;
  ADCON1 adcon1;
  ADRES  adres;

  virtual PROCESSOR_TYPE isa(){return _P16C71_;};
  virtual void create_symbols();

  virtual unsigned int program_memory_size() const { return 0x400; };
  virtual void create_sfr_map();


  P16C71();
  void create();
  static Processor *construct();

};

class P16C712 :  public P16C62
{
 public:

  ADCON0 adcon0;
  ADCON1 adcon1;
  ADRES  adres;

  virtual PROCESSOR_TYPE isa(){return _P16C712_;};

  virtual void create_sfr_map();


  P16C712();
  void create();
  static Processor *construct();

  virtual bool hasSSP() { return false; }
};

class P16C716 :  public P16C712
{
 public:

  virtual PROCESSOR_TYPE isa(){return _P16C716_;};

  virtual unsigned int program_memory_size() const { return 0x800; };

  P16C716();
  static Processor *construct();

};

class P16C72 : public P16C62 
{
 public:
  // XXX
  // This pir1_2, pir2_2 stuff is not particularly pretty.  It would be
  // better to just tell C++ to redefine pir1 and pir2 and PIR1v2 and
  // PIR2v2, but C++ only supports covariance in member function return
  // values.
  PIR1v2 pir1_2_reg;
  PIR2v2 pir2_2_reg;
  PIR_SET_2 pir_set_2_def;
  ADCON0_withccp adcon0;
  ADCON1 adcon1;
  ADRES  adres;

  virtual PROCESSOR_TYPE isa(){return _P16C72_;};
  virtual void create_symbols();
  void create_sfr_map();
  virtual PIR *get_pir1() { return (&pir1_2_reg); }
  virtual PIR *get_pir2() { return (&pir2_2_reg); }
  virtual PIR_SET *get_pir_set() { return (&pir_set_2_def); }


  P16C72();
  void create();
  static Processor *construct();

};

class P16C73 : public P16C63
{
 public:
  // XXX
  // This pir1_2, pir2_2 stuff is not particularly pretty.  It would be
  // better to just tell C++ to redefine pir1 and pir2 and PIR1v2 and
  // PIR2v2, but C++ only supports covariance in member function return
  // values.
  PIR1v2 pir1_2_reg;
  PIR2v2 pir2_2_reg;
  PIR_SET_2 pir_set_2_def;
  ADCON0_withccp adcon0;
  ADCON1 adcon1;
  ADRES  adres;

  virtual PROCESSOR_TYPE isa(){return _P16C73_;};
  virtual void create_symbols();
  void create_sfr_map();
  virtual PIR *get_pir1() { return (&pir1_2_reg); }
  virtual PIR *get_pir2() { return (&pir2_2_reg); }
  virtual PIR_SET *get_pir_set() { return (&pir_set_2_def); }

  P16C73();
  void create();
  static Processor *construct();

};

//---------------------------------------------------------

class P16C74 : public P16C65 // Not a typo, a 'c74 is more like a 'c65 then a 'c64!
{
 public:
  // XXX
  // This pir1_2, pir2_2 stuff is not particularly pretty.  It would be
  // better to just tell C++ to redefine pir1 and pir2 and PIR1v2 and
  // PIR2v2, but C++ only supports covariance in member function return
  // values.
  PIR1v2 pir1_2_reg;
  PIR2v2 pir2_2_reg;
  PIR_SET_2 pir_set_2_def;
  ADCON0_withccp adcon0;
  ADCON1 adcon1;
  ADRES  adres;

  virtual PROCESSOR_TYPE isa(){return _P16C74_;};
  virtual void create_symbols();
  void create_sfr_map();
  virtual PIR *get_pir1() { return (&pir1_2_reg); }
  virtual PIR *get_pir2() { return (&pir2_2_reg); }
  virtual PIR_SET *get_pir_set() { return (&pir_set_2_def); }

  virtual unsigned int program_memory_size() const { return 0x1000; };


  P16C74();
  void create();
  static Processor *construct();

};


#endif

