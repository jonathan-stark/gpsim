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

#include "a2dconverter.h"
#include "ioports.h"
#include "trace.h"
#include "gpsim_time.h"
#include "ui.h"
#include "processor.h"

static PinModule AnInvalidAnalogInput;


//------------------------------------------------------
// ADRES
//

void ADRES::put(int new_value)
{

  trace.raw(write_trace.get() | value.get());

  if(new_value > 255)
    value.put(255);
  else if (new_value < 0)
    value.put(0);
  else
    value.put(new_value);
}


//------------------------------------------------------
// ADCON0
//
ADCON0::ADCON0()
  : adres(0), adresl(0), adcon1(0), intcon(0), ad_state(AD_IDLE)
{
}

void ADCON0::setAdres(ADRES *new_adres)
{
  adres = new_adres;
}
void ADCON0::setAdresLow(ADRES *new_adresl)
{
  adresl = new_adresl;
}
void ADCON0::setAdcon1(ADCON1 *new_adcon1)
{
  adcon1 = new_adcon1;
}
void ADCON0::setIntcon(INTCON *new_intcon)
{
  intcon = new_intcon;
}

void ADCON0::setA2DBits(unsigned int nBits)
{
  m_A2DScale = (1<<nBits) - 1;
}

void ADCON0::start_conversion(void)
{

  #ifdef DEBUG_AD
  cout << "starting A/D conversion\n";
  #endif

  if(!(value.get() & ADON) ) {
    //cout << " A/D converter is disabled\n";
    stop_conversion();
    return;
  }

  guint64 fc = get_cycles().value + Tad_2;

  if(ad_state != AD_IDLE)
    {
      // The A/D converter is either 'converting' or 'acquiring'
      // in either case, there is callback break that needs to be moved.

      stop_conversion();
      get_cycles().reassign_break(future_cycle, fc, this);
    }
  else
    get_cycles().set_break(fc, this);

  future_cycle = fc;
  ad_state = AD_ACQUIRING;

}

void ADCON0::stop_conversion(void)
{

  #ifdef DEBUG_AD
  cout << "stopping A/D conversion\n";
  #endif
  ad_state = AD_IDLE;

}



void ADCON0::put(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());

  // Get the A/D Conversion Clock Select bits
  // 
  // This switch case will get the ADCS bits and set the Tad, or The A/D
  // converter clock period. The A/D clock period is faster than the instruction
  // cycle period when the two ADCS bits are zero. However, this is handled by
  // dealing with twice the Tad, or Tad_2. Now if the ADCS bits are zero, Tad_2
  // will equal Tinst. It turns out that the A/D converter takes an even number
  // of Tad periods to make a conversion.

  switch(new_value & (ADCS0 | ADCS1))
    {

    case 0:
      Tad_2 = 1;
      break;

    case ADCS0:
      Tad_2 = 2;
      break;

    case ADCS1:
      Tad_2 = 32;

    case (ADCS0|ADCS1):
      Tad_2 = 3;   // %%% FIX ME %%% I really need to implement 'absolute time'

    }
    
  unsigned int old_value=value.get();
  // SET: Reflect it first!
  value.put(new_value);
  if(new_value & ADON)
    {
      // The A/D converter is being turned on (or maybe left on)

      //if(((new_value ^ value) & GO) == GO)
      if((new_value & ~old_value) & GO)
	{
	  // The 'GO' bit is being turned on, which is request to initiate
	  // and A/D conversion
	  start_conversion();
	}
    }
  else
    {
      stop_conversion();
    }

}

void ADCON0::put_conversion(void)
{
  double dRefSep = m_dSampledVrefHi - m_dSampledVrefLo;
  double dNormalizedVoltage;

  dNormalizedVoltage = (dRefSep>0.0) ? 
    (m_dSampledVoltage - m_dSampledVrefLo)/dRefSep : 0.0;
  dNormalizedVoltage = dNormalizedVoltage>1.0 ? 1.0 : dNormalizedVoltage;

  unsigned int converted = (unsigned int)(m_A2DScale*dNormalizedVoltage);

  if (verbose)
    printf("put_conversion: Vrefhi:%g Vreflo:%g conversion:%d normV:%g\n",
	   m_dSampledVrefHi,m_dSampledVrefLo,converted,dNormalizedVoltage);

  if(adresl) {   // non-null for 16f877


    if(verbose)
      cout << __FUNCTION__ << "() 10-bit result " << (converted &0x3ff)  << '\n';
    if(adcon1->value.get() & ADCON1::ADFM) {
      adresl->put(converted & 0xff);
      adres->put( (converted >> 8) & 0x3);
    } else {
      adresl->put((converted << 6) & 0xc0);
      adres->put( (converted >> 2) & 0xff);
    }

  } else {

    if(verbose)
      cout << __FUNCTION__ << "() 8-bit result " << ((converted) &0xff)  << '\n';

    adres->put((converted ) & 0xff);

  }

}

// ADCON0 callback is called when the cycle counter hits the break point that
// was set in ADCON0::put.

void ADCON0::callback(void)
{
  int channel;
  #ifdef DEBUG_AD
  cout<<"ADCON0 Callback: " << hex << cycles.value << '\n';
  #endif

  //
  // The a/d converter is simulated with a state machine. 
  // 

  switch(ad_state)
    {
    case AD_IDLE:
      cout << "ignoring ad callback since ad_state is idle\n";
      break;

    case AD_ACQUIRING:
      channel = (value.get() >> 3) & channel_mask;

      m_dSampledVoltage = adcon1->getChannelVoltage(channel);
      m_dSampledVrefHi  = adcon1->getVrefHi();
      m_dSampledVrefLo  = adcon1->getVrefLo();

      future_cycle = get_cycles().value + 5*Tad_2;
      get_cycles().set_break(future_cycle, this);
      
      ad_state = AD_CONVERTING;

      break;

    case AD_CONVERTING:

      put_conversion();

      // Clear the GO/!DONE flag.
      value.put(value.get() & (~GO));
      set_interrupt();

      ad_state = AD_IDLE;
    }

}

//------------------------------------------------------
//
void ADCON0::set_interrupt(void)
{
  value.put(value.get() | ADIF);
  intcon->peripheral_interrupt();

}

//------------------------------------------------------
//
void ADCON0_withccp::set_interrupt(void)
{

  pir_set->set_adif();

}


//------------------------------------------------------
// ADCON1
//
ADCON1::ADCON1()
  : m_AnalogPins(0), m_nAnalogChannels(0),
    mValidCfgBits(0)
{
  for (int i=0; i<cMaxConfigurations; i++) {
    setChannelConfiguration(i, 0);
    setVrefLoConfiguration(i, 0xffff);
    setVrefHiConfiguration(i, 0xffff);
  }
}

void ADCON1::setChannelConfiguration(unsigned int cfg, unsigned int bitMask)
{
  if (cfg < cMaxConfigurations) 
    m_configuration_bits[cfg] = bitMask;
}

void ADCON1::setVrefLoConfiguration(unsigned int cfg, unsigned int channel)
{
  if (cfg < cMaxConfigurations) 
    Vreflo_position[cfg] = channel;
}

void ADCON1::setVrefHiConfiguration(unsigned int cfg, unsigned int channel)
{
  if (cfg < cMaxConfigurations) 
    Vrefhi_position[cfg] = channel;
}

void ADCON1::setNumberOfChannels(unsigned int nChannels)
{
  if (m_nAnalogChannels || !nChannels)
    return;

  m_nAnalogChannels = nChannels;
  m_AnalogPins = new PinModule *[m_nAnalogChannels];

  for (unsigned int i=0; i<m_nAnalogChannels; i++)
    m_AnalogPins[i] = &AnInvalidAnalogInput;
}

void ADCON1::setIOPin(unsigned int channel, PinModule *newPin)
{
  if (m_AnalogPins[channel] == &AnInvalidAnalogInput && newPin!=0) {
    m_AnalogPins[channel] = newPin;
  }
}


//------------------------------------------------------
double ADCON1::getChannelVoltage(unsigned int channel)
{
  double voltage=0.0;
  if(channel <= m_nAnalogChannels) {
    if ( (1<<channel) & m_configuration_bits[value.data & mValidCfgBits]) {
      PinModule *pm = m_AnalogPins[channel];
      voltage = (pm != &AnInvalidAnalogInput) ? 
	pm->getPin().get_nodeVoltage() : 0.0;
    }
  }

  return voltage;
}

double ADCON1::getVrefHi()
{
  if (Vrefhi_position[value.data & mValidCfgBits] < m_nAnalogChannels)
    return getChannelVoltage(Vrefhi_position[value.data & mValidCfgBits]);

  return ((Processor *)cpu)->get_Vdd();
}

double ADCON1::getVrefLo()
{
  if (Vreflo_position[value.data & mValidCfgBits] < m_nAnalogChannels)
    return getChannelVoltage(Vreflo_position[value.data & mValidCfgBits]);

  return 0.0;
}

