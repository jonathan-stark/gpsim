/*
   Copyright (C) 2006 T. Scott Dattalo
   Copyright (C) 2009, 2013 Roy R. Rankin

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

#ifndef __A2DCONVERTER_H__
#define __A2DCONVERTER_H__

#include "registers.h"
#include "trigger.h"
#include "intcon.h"
#include "pir.h"
#include "comparator.h"

class PinModule;
class pic_processor;

//---------------------------------------------------------
// ADRES
//
/*
class ADRES : public sfr_register
{
public:

  void put(int new_value);
};
*/

/*
	AD_IN_SignalControl is used to set an ADC pin as input
	regardless of the setting to the TRIS register
*/
class AD_IN_SignalControl : public SignalControl
{
public:
  AD_IN_SignalControl(){}
  ~AD_IN_SignalControl(){}
  char getState() { return '1'; }
  void release() { }
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
      VCFG0 = 1<<4,   // 16f88
      VCFG1 = 1<<5,   // 16f88
      ADCS2 = 1<<6,
      ADFM  = 1<<7    // Format Result select bit 
    };

  ADCON1(Processor *pCpu, const char *pName, const char *pDesc);
  ~ADCON1();

  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  void setChannelConfiguration(unsigned int cfg, unsigned int bitMask);
  void setVrefLoConfiguration(unsigned int cfg, unsigned int channel);
  void setVrefHiConfiguration(unsigned int cfg, unsigned int channel);
  unsigned int getVrefHiChannel(unsigned int cfg);
  unsigned int getVrefLoChannel(unsigned int cfg);

  double getChannelVoltage(unsigned int channel);
  virtual double getVrefHi();
  virtual double getVrefLo();

  void setValidCfgBits(unsigned int m, unsigned int s);
  void setNumberOfChannels(unsigned int);
  unsigned int getNumberOfChannels() { return(m_nAnalogChannels);}
  void setIOPin(unsigned int, PinModule *);
  void setVoltRef(unsigned int, float);
  int  get_cfg(unsigned int);
  virtual unsigned int get_adc_configmask(unsigned int);
  virtual void set_cfg_index(unsigned int index) { cfg_index = index;}

  void set_channel_in(unsigned int channel, bool on);
  void setADCnames();
  void setValidBits(unsigned int mask) { valid_bits = mask;}
  bool getADFM() { return adfm;}
  unsigned int getMaxCfg() { return cMaxConfigurations;}

  
  

protected:

  unsigned int valid_bits;
  bool		adfm;
  PinModule **m_AnalogPins;
  float	*m_voltageRef;
  unsigned int m_nAnalogChannels;
  unsigned int mValidCfgBits;
  unsigned int mCfgBitShift;
  unsigned int mIoMask;
  unsigned int cfg_index;

  static const unsigned int cMaxConfigurations=16;

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

  // used bt setControl to set pin direction as input
  AD_IN_SignalControl *m_ad_in_ctl;
};

class ADCON0;
//---------------------------------------------------------
// ADCON1_16F 
//

class ADCON1_16F : public ADCON1
{
public:

  enum 
    {
	ADPREF0 = (1<<0),
	ADPREF1 = (1<<1),
	ADNREF  = (1<<2),
	ADCS0   = (1<<4),
	ADCS1   = (1<<5),
	ADCS2   = (1<<6),
	ADFM    = (1<<7)
    };

  ADCON1_16F(Processor *pCpu, const char *pName, const char *pDesc);
  virtual void put_value(unsigned int new_value);
  void setAdcon0 (ADCON0 *_adcon0) {adcon0 = _adcon0;}
  virtual double getVrefHi();
  virtual double getVrefLo();
  void set_FVR_chan(unsigned int chan) { FVR_chan = chan;}

private:
   ADCON0 *adcon0;
   unsigned int FVR_chan;
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
      CHS2 = 1<<5,
      ADCS0 = 1<<6,
      ADCS1 = 1<<7
    };

  enum AD_states
    {
      AD_IDLE,
      AD_ACQUIRING,
      AD_CONVERTING
    };

  ADCON0(Processor *pCpu, const char *pName, const char *pDesc);

  void start_conversion();
  void stop_conversion();
  virtual void set_interrupt();
  virtual void callback();
  void put(unsigned int new_value);
  virtual void put_conversion();

  bool getADIF() { return (value.get() & ADIF) != 0; }
  void setAdres(sfr_register *);
  void setAdresLow(sfr_register *);
  void setAdcon1(ADCON1 *);
  void setIntcon(INTCON *);
  virtual void setPir(PIR *);
  void setA2DBits(unsigned int);
  void setChannel_Mask(unsigned int ch_mask) { channel_mask = ch_mask; }
  void setChannel_shift(unsigned int ch_shift) { channel_shift = ch_shift; }
  void setGo(unsigned int go) { GO_bit = (1 << go); }
  virtual bool get_ADFM() { return adcon1->getADFM(); }
  virtual void set_Tad(unsigned int);
  virtual double getChannelVoltage(unsigned int channel) { 
	return( adcon1->getChannelVoltage(channel)); }
  virtual double getVrefHi() {
      return(m_dSampledVrefHi  = adcon1->getVrefHi());}
  virtual double getVrefLo() {
      return (m_dSampledVrefLo  = adcon1->getVrefLo());}

  void setValidBits(unsigned int mask) { valid_bits = mask;}

private:

  friend class ADCON0_10;
  friend class ADCON0_12F;
  friend class ADCON0_DIF;

  sfr_register *adres;
  sfr_register *adresl;
  ADCON1 *adcon1;
  INTCON *intcon;
  PIR    *m_pPir;

  double m_dSampledVoltage;
  double m_dSampledVrefHi;
  double m_dSampledVrefLo;
  unsigned int m_A2DScale;
  unsigned int m_nBits;
  guint64 future_cycle;
  unsigned int ad_state;
  unsigned int Tad_2;
  unsigned int Tad;
  unsigned int channel_mask;
  unsigned int channel_shift;
  unsigned int GO_bit;
  unsigned int valid_bits;
};


class ADCON2_DIF;
/* A/D converter with 12 or 10 bit differential input with ADCON2
 */

class ADCON0_DIF : public ADCON0
{
  enum
    {
      ADON = 1<<0,
      GO   = 1<<1,
      CHS0 = 1<<2,
      CHS1 = 1<<3,
      CHS2 = 1<<4,
      CHS3 = 1<<5,
      CHS4 = 1<<6,
      ADRMD = 1<<7
    };
public:
  ADCON0_DIF(Processor *pCpu, const char *pName, const char *pDesc);
  virtual void put(unsigned int new_value);
  virtual void put_conversion(void);
  void	setAdcon2(ADCON2_DIF * _adcon2) { adcon2 = _adcon2;}

private:
   ADCON2_DIF *adcon2;

};
//---------------------------------------------------------
// ADCON0_10 register for 10f22x A2D
//

class ADCON0_10 : public ADCON0
{
public:

  enum
    {
      ADON = 1<<0,
      GO   = 1<<1,
      CHS0 = 1<<2,
      CHS1 = 1<<3,
      ANS0 = 1<<6,
      ANS1 = 1<<7
    };
  void put(unsigned int new_value);
  ADCON0_10(Processor *pCpu, const char *pName, const char *pDesc);
private:
	AD_IN_SignalControl ad_pin_input;
};
//---------------------------------------------------------
// ANSEL
//

class ANSEL_H;

class ANSEL : public sfr_register
{
public:
  enum
  {
    ANS0 = 1 << 0,
    ANS1 = 1 << 1,
    ANS2 = 1 << 2,
    ANS3 = 1 << 3,
    ANS4 = 1 << 4,
    ANS5 = 1 << 5,
    ANS6 = 1 << 6,
    ANS7 = 1 << 7
  };

  ANSEL(Processor *pCpu, const char *pName, const char *pDesc);

  void setAdcon1(ADCON1 *new_adcon1);
  void setAnselh(ANSEL_H *new_anselh) { anselh = new_anselh;}
  void put(unsigned int new_val);
  void setValidBits(unsigned int mask) { valid_bits = mask;}

private:
    ADCON1 *adcon1;
    ANSEL_H *anselh;
    unsigned int valid_bits;
};
//---------------------------------------------------------
// ANSEL_H
//

class ANSEL_H : public sfr_register
{
public:
  enum
  {
    ANS8 = 1 << 0,
    ANS9 = 1 << 1,
    ANS10 = 1 << 2,
    ANS11 = 1 << 3,
    ANS12 = 1 << 4,
    ANS13 = 1 << 5,
  };

  ANSEL_H(Processor *pCpu, const char *pName, const char *pDesc);

  void setAdcon1(ADCON1 *new_adcon1);
  void setAnsel(ANSEL *new_ansel) { ansel = new_ansel;}
  void put(unsigned int new_val);
  void setValidBits(unsigned int mask) { valid_bits = mask;}

private:
    ADCON1 *adcon1;
    ANSEL *ansel;
    unsigned int valid_bits;
};

//
//	ANSEL_P is an analog select register associated
//	with a port. 
class ANSEL_P : public sfr_register
{
public:
  ANSEL_P(Processor *pCpu, const char *pName, const char *pDesc);
  void setAdcon1(ADCON1 *new_adcon1);
  void setAnsel(ANSEL_P *new_ansel) { ansel = new_ansel;}
  void put(unsigned int new_val);
  void setValidBits(unsigned int mask) { valid_bits = mask;}
  void config(unsigned int pins, unsigned int first_chan)
	{ analog_pins = pins; first_channel = first_chan;}
  unsigned int get_mask() { return cfg_mask;}

private:
    ADCON1 *adcon1;
    ANSEL_P *ansel;
    unsigned int valid_bits;	// register bit mask
    unsigned int analog_pins;	// bit map of analog port pins
    unsigned int first_channel;	// channel number for LSB of analog_pins
    unsigned int cfg_mask;	// A2D mask this port only
};
//---------------------------------------------------------
// ADCON0_12F register for 12f675 A2D
//

class ADCON0_12F : public ADCON0
{
public:

  enum
    {
      ADON = 1<<0,
      GO   = 1<<1,
      CHS0 = 1<<2,
      CHS1 = 1<<3,
      CHS2 = 1<<4,
      VCFG = 1<<6,
      ADFM = 1<<7
    };
  void put(unsigned int new_value);
  virtual bool get_ADFM() { return(value.get() & ADFM); }
  virtual void set_Tad(unsigned int _tad) { Tad = _tad; }
  ADCON0_12F(Processor *pCpu, const char *pName, const char *pDesc);
private:
  AD_IN_SignalControl ad_pin_input;
};
//---------------------------------------------------------
// ANSEL_12F
//

class ANSEL_12F : public sfr_register
{
public:
  enum
  {
    ANS0 = 1 << 0,
    ANS1 = 1 << 1,
    ANS2 = 1 << 2,
    ANS3 = 1 << 3,
    ADCS0 = 1 << 4,
    ADCS1 = 1 << 5,
    ADCS2 = 1 << 6
  };

  ANSEL_12F(Processor *pCpu, const char *pName, const char *pDesc);

  void setAdcon0(ADCON0_12F *new_adcon0) { adcon0 = new_adcon0; }
  void setAdcon1(ADCON1 *new_adcon1) { adcon1 = new_adcon1; }
  void put(unsigned int new_val);
  void set_tad(unsigned int);

private:
    ADCON1 *adcon1;
    ADCON0_12F *adcon0;
};
// set voltage from stimulus
class a2d_stimulus : public stimulus
{
   public:

	a2d_stimulus(ADCON1 *arg, int chan, const char *n=0,
           double _Vth=0.0, double _Zth=1e12
           );

    ADCON1 *_adcon1;
    int	   channel;

     virtual void   set_nodeVoltage(double v);

};


//---------------------------------------------------------
// FVRCON register for Fixed Voltage Reference
//

class DACCON0;	

class FVRCON : public sfr_register
{
public:

  enum 
  {
	ADFVR0 	= 1<<0,
	ADFVR1 	= 1<<1,
	CDAFVR0 = 1<<2,
	CDAFVR1 = 1<<3,
	TSRNG	= 1<<4,
	TSEN	= 1<<5,
	FVRRDY	= 1<<6,
	FVREN	= 1<<7,
  };
  FVRCON(Processor *, const char *pName, const char *pDesc=0, unsigned int bitMask= 0xff, unsigned int alwaysOne = 0);
  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  void set_adcon1(ADCON1 *_adcon1) { adcon1 = _adcon1;}
  void set_cmModule(ComparatorModule2 *_cmModule) { cmModule = _cmModule;}
  void set_daccon0(DACCON0 *_daccon0) { daccon0_list.push_back(_daccon0);}
  void set_cpscon0(CPSCON0 *_cpscon0) { cpscon0 = _cpscon0;}
  void set_VTemp_AD_chan(unsigned int _chan) {VTemp_AD_chan = _chan;}
  void set_FVRAD_AD_chan(unsigned int _chan) {FVRAD_AD_chan = _chan;}
private:
  double compute_VTemp(unsigned int);	//Voltage of core temperature setting
  double compute_FVR_AD(unsigned int);	//Voltage reference for ADC
  double compute_FVR_CDA(unsigned int);	//Voltage reference for Comparators, DAC, CPS
  ADCON1 *adcon1;
  DACCON0 *daccon0;
  vector<DACCON0 *> daccon0_list;
  ComparatorModule2 *cmModule;
  CPSCON0 *cpscon0;
  unsigned int VTemp_AD_chan;
  unsigned int FVRAD_AD_chan;
  unsigned int mask_writable;
  unsigned int always_one;	// bits in register that are always 1
};

//
//  DAC module
//

class DACCON1;
class DACCON0 : public sfr_register
{
public:
  enum
  {
	DACPSS0	= (1<<2),
	DACPSS1	= (1<<3),
	DACOE2	= (1<<4),
	DACOE	= (1<<5),
	DACLPS  = (1<<6),
	DACEN	= (1<<7),
  };

  DACCON0(Processor *, const char *pName, const char *pDesc=0, unsigned int bitMask= 0xe6, unsigned int bit_res = 32);
  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  void set_adcon1(ADCON1 *_adcon1) { adcon1 = _adcon1;}
  void set_cpscon0(CPSCON0 *_cpscon0) { cpscon0 = _cpscon0;}
  void set_cmModule(ComparatorModule2 *_cmModule) { cmModule = _cmModule;}
  void set_FVRCDA_AD_chan(unsigned int _chan) {FVRCDA_AD_chan = _chan;}
  void set_dcaccon1_reg(unsigned int reg);
  void set_FVR_CDA_volt(double volt) { FVR_CDA_volt = volt;}
  void setDACOUT(PinModule *pin1, PinModule *pin2 = NULL){ output_pin[0] = pin1; output_pin[1] = pin2;}
  //void setDACOUT(IO_bi_directional_pu *_pin){ pin = _pin;}

private:
  void  set_dacoutpin(bool output_enabled, int chan, double Vout);
  void	compute_dac(unsigned int value);
  double get_Vhigh(unsigned int value);
  ADCON1	*adcon1;
  ComparatorModule2 *cmModule;
  vector<ComparatorModule2 *> cmModule_list;
  CPSCON0	*cpscon0;
  unsigned int  FVRCDA_AD_chan;
  unsigned int  bit_mask;
  unsigned int  bit_resolution;
  unsigned int  daccon1_reg;
  double	FVR_CDA_volt;
  bool		Pin_Active[2];
  double	Vth[2];
  double	Zth[2];
  bool		driving[2];
  PinModule	*output_pin[2];
  IOPIN *pin;

};

class DACCON1 : public sfr_register
{
public:
  DACCON1(Processor *, const char *pName, const char *pDesc=0, unsigned int bitMask= 0x1f, DACCON0 *_daccon0 = 0);
  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  void set_daccon0(DACCON0 *_daccon0) { daccon0 = _daccon0;}

private:
    unsigned int bit_mask;
    DACCON0	*daccon0;
};

class ADCON2_DIF : public sfr_register, public TriggerObject
{
public:

  enum
    {
      CHSNS0   = 1<<0,
      CHSNS1   = 1<<1,
      CHSNS2   = 1<<2,
      CHSNS3   = 1<<3,
      TRIGSEL0 = 1<<4,
      TRIGSEL1 = 1<<5,
      TRIGSEL2 = 1<<6,
      TRIGSEL3 = 1<<7,
    };


  ADCON2_DIF(Processor *pCpu, const char *pName, const char *pDesc);

private:


};
#endif // __A2DCONVERTER_H__
