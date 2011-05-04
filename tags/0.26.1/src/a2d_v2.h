/*
   Copyright (C) 2006 T. Scott Dattalo
   Copyright (C) 2008 Roy R Rankin

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

#ifndef __A2D_V2_H__
#define __A2D_v2_H__

#include "registers.h"
#include "trigger.h"
#include "intcon.h"
#include "pir.h"

class PinModule;
class pic_processor;

class ADCON0_V2;
//---------------------------------------------------------
// ADCON1
//

class ADCON1_V2 : public sfr_register
{
public:

  enum PCFG_bits
    {
      PCFG0 = 1<<0,
      PCFG1 = 1<<1,
      PCFG2 = 1<<2,
      PCFG3 = 1<<3,
      VCFG0 = 1<<4,
      VCFG1 = 1<<5,
    };

  ADCON1_V2(Processor *pCpu, const char *pName, const char *pDesc);
  virtual void put(unsigned int new_value);

  void setChanTable(
	unsigned int m0, unsigned int m1, unsigned int m2, unsigned int m3,
	unsigned int m4, unsigned int m5, unsigned int m6, unsigned int m7,
	unsigned int m8, unsigned int m9, unsigned int m10, unsigned int m11,
	unsigned int m12, unsigned int m13, unsigned int m14, unsigned int m15);
	
  void setChannelConfiguration(unsigned int cfg, unsigned int bitMask);

  double getChannelVoltage(unsigned int channel);
  double getVrefHi();
  double getVrefLo();
  void 	 setVrefHiChannel(unsigned int channel);
  void 	 setVrefLoChannel(unsigned int channel);

  void setValidCfgBits(unsigned int m, unsigned int s);
  void setNumberOfChannels(unsigned int);
  void setIOPin(unsigned int, PinModule *);
  void setAdcon0(ADCON0_V2 *adcon0){ m_adcon0 = adcon0;}
  virtual unsigned int get_adc_configmask(unsigned int); 

private:
  PinModule **m_AnalogPins;
  unsigned int m_nAnalogChannels;
  unsigned int mValidCfgBits;
  unsigned int mCfgBitShift;
  int 		m_vrefHiChan;
  int 		m_vrefLoChan;
  unsigned int mIoMask;
  ADCON0_V2    *m_adcon0;	// if set use to get VCFG0 and VCFG1

  static const unsigned int cMaxConfigurations=16;


  /* configuration bits is an array of 8-bit masks definining whether or not
   * a given channel is analog or digital
   */
  unsigned int m_configuration_bits[cMaxConfigurations];

};
//---------------------------------------------------------
// ADCON2
//

class ADCON2_V2 : public sfr_register, public TriggerObject
{
public:

  enum
    {
      ADCS0 = 1<<0,
      ADCS1 = 1<<1,
      ADCS2 = 1<<2,
      ACQT0 = 1<<3,
      ACQT1 = 1<<4,
      ACQT2 = 1<<5,
      ADFM  = 1<<7
    };


  ADCON2_V2(Processor *pCpu, const char *pName, const char *pDesc);


  char get_nacq();
  char get_tad();
  bool get_adfm();

private:


};
//---------------------------------------------------------
// ADCON0
//

class ADCON0_V2 : public sfr_register, public TriggerObject
{
public:

  enum
    {
      ADON = 1<<0,
      GO   = 1<<1,
      CHS0 = 1<<2,
      CHS1 = 1<<3,
      CHS2 = 1<<4,
      CHS3 = 1<<5,

      VCFG0 = 1<<6,	// for 18f1220
      VCFG1 = 1<<7,	// for 18f1220
    };

  enum AD_states
    {
      AD_IDLE,
      AD_ACQUIRING,
      AD_CONVERTING
    };

  ADCON0_V2(Processor *pCpu, const char *pName, const char *pDesc);


  void start_conversion();
  void stop_conversion();
  virtual void set_interrupt();
  virtual void callback();
  void put(unsigned int new_value);
  void put_conversion();

  void setAdres(sfr_register *);
  void setAdresLow(sfr_register *);
  void setAdcon1(ADCON1_V2 *);
  void setAdcon2(ADCON2_V2 *);
  void setIntcon(INTCON *);
  void setPir(PIR1v2 *);
  void setA2DBits(unsigned int);
  void setChannel_Mask(unsigned int ch_mask) { channel_mask = ch_mask; }
  void setRCtime(double);

private:

  sfr_register *adres;
  sfr_register *adresl;
  ADCON1_V2    *adcon1;
  ADCON2_V2    *adcon2;
  INTCON       *intcon;
  PIR1v2       *pir1v2;

  double m_dSampledVoltage;
  double m_dSampledVrefHi;
  double m_dSampledVrefLo;
  double m_RCtime;
  unsigned int m_A2DScale;
  unsigned int m_nBits;
  guint64 future_cycle;
  unsigned int ad_state;
  unsigned int Tad;
  unsigned int Tacq;
  unsigned int channel_mask;
};


#endif // __A2D_V2_H__
