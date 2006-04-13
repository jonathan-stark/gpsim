/*
   Copyright (C) 2006 T. Scott Dattalo

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

#ifndef __A2DCONVERTER_H__
#define __A2DCONVERTER_H__

#include "registers.h"
#include "trigger.h"
#include "intcon.h"
#include "pir.h"

class PinModule;

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

  bool getADIF() { return (value.get() & ADIF) != 0; }
  void setAdres(ADRES *);
  void setAdresLow(ADRES *);
  void setAdcon1(ADCON1 *);
  void setIntcon(INTCON *);
  void setA2DBits(unsigned int);
private:

  ADRES *adres;
  ADRES *adresl;
  ADCON1 *adcon1;
  INTCON *intcon;

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



#endif // __A2DCONVERTER_H__
