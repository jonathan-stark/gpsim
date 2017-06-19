/*
   Copyright (C) 2006 T. Scott Dattalo
   Copyright (C) 2008,2015 Roy R Rankin

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

// For A2D modules which have ADCON0, ADCON1 and ADCON2 registers

#ifndef __A2D_V2_H__
#define __A2D_v2_H__

#include "comparator.h"
#include "a2dconverter.h"
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
  ~ADCON1_V2();
  virtual void put(unsigned int new_value);

  void setChanTable(
	unsigned int m0, unsigned int m1, unsigned int m2, unsigned int m3,
	unsigned int m4, unsigned int m5, unsigned int m6, unsigned int m7,
	unsigned int m8, unsigned int m9, unsigned int m10, unsigned int m11,
	unsigned int m12, unsigned int m13, unsigned int m14, unsigned int m15);
	
  void setChannelConfiguration(unsigned int cfg, unsigned int bitMask);

  virtual PinModule *get_A2Dpin(unsigned int channel);
  virtual double getChannelVoltage(unsigned int channel);
  virtual double getVrefHi();
  virtual double getVrefLo();
  void 	 setVrefHiChannel(unsigned int channel);
  void 	 setVrefLoChannel(unsigned int channel);

  void setValidCfgBits(unsigned int m, unsigned int s);
  void setNumberOfChannels(unsigned int);
  void setIOPin(unsigned int, PinModule *);
  void setAdcon0(ADCON0_V2 *adcon0){ m_adcon0 = adcon0;}
  virtual unsigned int get_adc_configmask(unsigned int); 
  virtual void setVoltRef(double x) {;}
  virtual int getVrefHiChan() { return m_vrefHiChan;}
  virtual int getVrefLoChan() { return m_vrefLoChan;}

private:
protected:
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
  virtual void callback_print() {cout <<  name() << " has callback, ID = " << CallBackID << '\n';}
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
  void set_ctmu_stim(stimulus *_ctmu_stim);
  void attach_ctmu_stim();
  void detach_ctmu_stim();

private:

  sfr_register  *adres;
  sfr_register  *adresl;
  ADCON1_V2     *adcon1;
  ADCON2_V2     *adcon2;
  INTCON        *intcon;
  PIR1v2        *pir1v2;

  double 	m_dSampledVoltage;
  double 	m_dSampledVrefHi;
  double 	m_dSampledVrefLo;
  double 	m_RCtime;
  unsigned int 	m_A2DScale;
  unsigned int 	m_nBits;
  guint64 	future_cycle;
  unsigned int 	ad_state;
  unsigned int 	Tad;
  unsigned int 	Tacq;
  unsigned int 	channel_mask;
  stimulus     	*ctmu_stim;
  int		active_stim;	// channel with active stimulus
};

// Channels defined in ANSEL rather than ADCON1
class ADCON1_2B : public ADCON1_V2
{
public:

    enum
    {
       NVCFG0 =  1<<0,	// Negative Voltage Reference Configuration bits
       NVCFG1 =  1<<1,
       PVCFG0 =  1<<2,	// Positive Voltage Reference Configuration bits
       PVCFG1 =  1<<3,
       TRIGSEL = 1<<7,	// Special Trigger Select bit

       // the following are special A/D channels
       CTMU = 0x1d,
       DAC  = 0x1e,
       FVR_BUF2 = 0x1f
    };

    ADCON1_2B(Processor *pCpu, const char *pName, const char *pDesc);
    virtual double getChannelVoltage(unsigned int channel);
    virtual void put(unsigned int new_value);
    virtual double getVrefHi();
    virtual double getVrefLo();
    void update_ctmu(double _Vctmu) { Vctmu = _Vctmu;}
    void update_dac(double _Vdac) { Vdac = _Vdac;}
    virtual void setVoltRef(double _Vfvr_buf2) { Vfvr_buf2 = _Vfvr_buf2;}
    virtual PinModule *get_A2Dpin(unsigned int channel);
    virtual void ctmu_trigger();
    virtual void ccp_trigger();

private:
    double Vctmu;
    double Vdac;
    double Vfvr_buf2;
    
};

// ANSEL register for ADCv2B with an ANSEL per I/O port
class ANSEL_2B :  public sfr_register
{

public:

  ANSEL_2B(Processor *pCpu, const char *pName, const char *pDesc);
  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  void setIOPin(unsigned int channel, PinModule *port, ADCON1_2B *adcon1);

protected:
   PinModule *m_AnalogPins[8];
   int  analog_channel[8];
   unsigned int mask;
};

// ANSEL register for ADCv2 on devices where the ANSEL is mapped by ADC chan
class ANSEL_2A :  public ANSEL_2B
{
public:

  ANSEL_2A(Processor *pCpu, const char *pName, const char *pDesc);
  void setIOPin(unsigned int channel, PinModule *port, ADCON1_2B *adcon1);
};

class FVRCON_V2 : public sfr_register, public TriggerObject
{
public:

  enum 
  {
	FVRS0   = 1<<4,
	FVRS1   = 1<<5,
	FVRRDY	= 1<<6,
	FVREN	= 1<<7,
  };
  FVRCON_V2(Processor *, const char *pName, const char *pDesc=0, unsigned int bitMask= 0xf0);
  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  void set_adcon1(ADCON1_V2 *_adcon1) { adcon1 = _adcon1;}
  void set_cmModule(ComparatorModule2 *_cmModule) { cmModule = _cmModule;}
  void set_daccon0(DACCON0 *_daccon0) { daccon0_list.push_back(_daccon0);}
  void set_cpscon0(CPSCON0 *_cpscon0) { cpscon0 = _cpscon0;}
private:
  unsigned int mask_writable;
  virtual void callback();
  virtual void callback_print() {cout <<  name() << " has callback, ID = " << CallBackID << '\n';}
  guint64  future_cycle;
  double compute_FVR(unsigned int);
  ADCON1_V2 *adcon1;
  DACCON0 *daccon0;
  vector<DACCON0 *> daccon0_list;
  ComparatorModule2 *cmModule;
  CPSCON0 *cpscon0;
};


class DACCON0_V2 : public DACCON0
{

public:

   DACCON0_V2(Processor *pCpu, const char *pName, const char *pDesc=0, 
	unsigned int bitMask= 0xe6, unsigned int bit_res = 32) :
	DACCON0(pCpu, pName, pDesc, bitMask, bit_res)
   { adcon1 = 0;}


  virtual void set_adcon1(ADCON1_2B *_adcon1) { adcon1 = _adcon1;}
  virtual void compute_dac(unsigned int value);
  virtual double  get_Vhigh(unsigned int value);

private:
  ADCON1_2B     *adcon1;
};
#endif // __A2D_V2_H__
