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

#include "ioports.h"
#include "trace.h"
#include "gpsim_time.h"
#include "ui.h"
#include "pic-processor.h"
#include "a2dconverter.h"

#define p_cpu ((Processor *)cpu)

static PinModule AnInvalidAnalogInput;

//#define DEBUG
#if defined(DEBUG)
#include "../config.h"
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

//------------------------------------------------------
// ADRES
//
/*
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

*/
//------------------------------------------------------
// ADCON0
//
ADCON0::ADCON0(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc),
    adres(0), adresl(0), adcon1(0), intcon(0), m_pPir(0), ad_state(AD_IDLE),
    channel_mask(7)
{
}

/*
 * Link PIC register for High Byte A/D result
 */
void ADCON0::setAdres(sfr_register *new_adres)
{
  adres = new_adres;
}
/*
 * Link PIC register for Low Byte A/D result
 */
void ADCON0::setAdresLow(sfr_register *new_adresl)
{
  adresl = new_adresl;
}
/*
 * Link PIC register for ADCON1
 */
void ADCON0::setAdcon1(ADCON1 *new_adcon1)
{
  adcon1 = new_adcon1;
}
/*
 * Link PIC register for INTCON
 */
void ADCON0::setIntcon(INTCON *new_intcon)
{
  intcon = new_intcon;
}
/*
 * Link PIC register for PIR
 * If set ADIF in PIR otherwise ADIF in ADCON0
 */
void ADCON0::setPir(PIR *pPir)
{
  m_pPir = pPir;
}

/*
 * Set resolution of A2D converter
 */
void ADCON0::setA2DBits(unsigned int nBits)
{
  m_A2DScale = (1<<nBits) - 1;
  m_nBits = nBits;
}

void ADCON0::start_conversion(void)
{

  Dprintf(("starting A/D conversion\n"));

  if(!(value.get() & ADON) ) {
    Dprintf((" A/D converter is disabled\n"));
    stop_conversion();
    return;
  }

  guint64 fc = get_cycles().get() + (2 * Tad) /
		p_cpu->get_ClockCycles_per_Instruction();

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

  Dprintf(("stopping A/D conversion\n"));

  ad_state = AD_IDLE;

}



void ADCON0::put(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());

  // Get the A/D Conversion Clock Select bits
  // 
  // This switch case will get the ADCS bits and set the Tad, or The A/D
  // converter clock period. Tad is the number of the oscillator periods
  //  rather instruction cycle periods. ADCS2 only exists on some processors,
  // but should be 0 where it is not used.

  switch(new_value & (ADCS0 | ADCS1))
    {

    case 0:
      Tad =  (adcon1->value.get() & ADCON1::ADCS2) ? 4 : 2;
      break;

    case ADCS0:
      Tad =  (adcon1->value.get() & ADCON1::ADCS2) ? 16 : 8;
      break;

    case ADCS1:
      Tad =  (adcon1->value.get() & ADCON1::ADCS2) ? 64 : 32;
      break;

    case (ADCS0|ADCS1):	// typical 4 usec, convert to osc cycles
      if (cpu)
      {
         Tad = (unsigned int)(4.e-6  * p_cpu->get_frequency());
	 Tad = Tad < 2? 2 : Tad;
      }
      else
	 Tad = 6;
      break;

    }
    
  unsigned int old_value=value.get();
  // SET: Reflect it first!
  value.put(new_value);
  if(new_value & ADON) {
    // The A/D converter is being turned on (or maybe left on)

    if((new_value & ~old_value) & GO) {
      if (verbose)
	printf("starting A2D conversion\n");
      // The 'GO' bit is being turned on, which is request to initiate
      // and A/D conversion
      start_conversion();
    }
  } else
    stop_conversion();

}

void ADCON0::put_conversion(void)
{
  double dRefSep = m_dSampledVrefHi - m_dSampledVrefLo;
  double dNormalizedVoltage;

  dNormalizedVoltage = (dRefSep>0.0) ? 
    (m_dSampledVoltage - m_dSampledVrefLo)/dRefSep : 0.0;
  dNormalizedVoltage = dNormalizedVoltage>1.0 ? 1.0 : dNormalizedVoltage;

  unsigned int converted = (unsigned int)(m_A2DScale*dNormalizedVoltage + 0.5);

  Dprintf(("put_conversion: Vrefhi:%g Vreflo:%g conversion:%d normV:%g\n",
	   m_dSampledVrefHi,m_dSampledVrefLo,converted,dNormalizedVoltage));

  if (verbose)
	printf ("result=0x%02x\n", converted);

  Dprintf(("%d-bit result 0x%x\n", m_nBits, converted));

  if(adresl) {   // non-null for more than 8 bit conversion

    if(adcon1->value.get() & ADCON1::ADFM) {
      adresl->put(converted & 0xff);
      adres->put( (converted >> 8) & 0x3);
    } else {
      adresl->put((converted << 6) & 0xc0);
      adres->put( (converted >> 2) & 0xff);
    }

  } else {

    adres->put((converted ) & 0xff);

  }

}

// ADCON0 callback is called when the cycle counter hits the break point that
// was set in ADCON0::put.

void ADCON0::callback(void)
{
  int channel;

  Dprintf((" ADCON0 Callback: 0x%"PRINTF_INT64_MODIFIER"x\n",cycles.value));

  //
  // The a/d converter is simulated with a state machine. 
  // 

  switch(ad_state)
    {
    case AD_IDLE:
      Dprintf(("ignoring ad callback since ad_state is idle\n"));
      break;

    case AD_ACQUIRING:
      channel = (value.get() >> 3) & channel_mask;

      m_dSampledVoltage = adcon1->getChannelVoltage(channel);
      m_dSampledVrefHi  = adcon1->getVrefHi();
      m_dSampledVrefLo  = adcon1->getVrefLo();

      Dprintf(("Acquiring channel:%d V=%g reflo=%g refhi=%g\n",
	       channel,m_dSampledVoltage,m_dSampledVrefLo,m_dSampledVrefHi));

      future_cycle = get_cycles().get() + (m_nBits * Tad)/p_cpu->get_ClockCycles_per_Instruction();
      get_cycles().set_break(future_cycle, this);
      if (verbose)
	printf("A/D %d bits channel:%d Vin=%g Refhi=%g Reflo=%g ", m_nBits,
	    channel,m_dSampledVoltage,m_dSampledVrefHi,m_dSampledVrefLo);
      
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
// If the ADIF bit is in the PIR1 register, call setPir() 
// in the ADC setup. Otherwise, ADIF is assumed to be in
// the ADCON0 register
//
void ADCON0::set_interrupt(void)
{
  if (m_pPir)
      m_pPir->set_adif();
  else
  {
      value.put(value.get() | ADIF);
      intcon->peripheral_interrupt();
  }

}



//------------------------------------------------------
// ADCON1
//
ADCON1::ADCON1(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc),
    m_AnalogPins(0), m_nAnalogChannels(0),
    mValidCfgBits(0), mCfgBitShift(0)
{
  for (int i=0; i<(int)cMaxConfigurations; i++) {
    setChannelConfiguration(i, 0);
    setVrefLoConfiguration(i, 0xffff);
    setVrefHiConfiguration(i, 0xffff);
  }
}

/*
 * If A2D uses PCFG, call for each PCFG value (cfg 0 to 15) with
 * each set bit of bitMask indicating port is an analog port
 * (either A2D input port or Vref). Processors which use an A2D
 * method that uses ANSEL register will not call this.
 *
 * As an example, for the following Port Configuration Control bit:
 * PCFG   AN7   AN6   AN5   AN4   AN3   AN2   AN1   AN0
 * ----   ---- ----- -----  ----- ----- ----- ----- -----
 * 1100   D    D     D      A     Vref+ Vref- A     A
 *
 *  then call setChannelConfiguration with cfg = 12 , bitMask = 0x1f
 * */
void ADCON1::setChannelConfiguration(unsigned int cfg, unsigned int bitMask)
{
  if (cfg < cMaxConfigurations) 
    m_configuration_bits[cfg] = bitMask;
}

unsigned int ADCON1::getVrefLoChannel(unsigned int cfg)
{
  if (cfg < cMaxConfigurations)
    return(Vreflo_position[cfg]);
  return(0xffff);
}
unsigned int ADCON1::getVrefHiChannel(unsigned int cfg)
{
  if (cfg < cMaxConfigurations)
    return(Vrefhi_position[cfg]);
  return(0xffff);
}

/*
 * Call for each configuration mode that uses an І/O pin as Vref-
 * to declare which port is being used for this.
 */
void ADCON1::setVrefLoConfiguration(unsigned int cfg, unsigned int channel)
{
  if (cfg < cMaxConfigurations) 
    Vreflo_position[cfg] = channel;
}

/*
 * Call for each configuration mode that uses an І/O pin as Vref+
 * to declare which port is being used for this.
 */
void ADCON1::setVrefHiConfiguration(unsigned int cfg, unsigned int channel)
{
  if (cfg < cMaxConfigurations) 
    Vrefhi_position[cfg] = channel;
}

/*
 * Number of A2D channels processor supports
 */
void ADCON1::setNumberOfChannels(unsigned int nChannels)
{
  PinModule **save = NULL;

  if (!nChannels || nChannels <= m_nAnalogChannels)
    return;

  if (m_nAnalogChannels && nChannels > m_nAnalogChannels )
        save = m_AnalogPins;

  m_AnalogPins = new PinModule *[nChannels];

  for (unsigned int i=0; i<nChannels; i++)
  {
    if(i < m_nAnalogChannels)
    {
        if (save)
            m_AnalogPins[i] = save[i];
    }
    else
        m_AnalogPins[i] = &AnInvalidAnalogInput;
  }
  if (save)
        delete save;

  m_nAnalogChannels = nChannels;
}

/*
 * Configure use of adcon1 register
 * 	The register is first anded with mask and then shifted
 * 	right shift bits. The result being either PCFG or VCFG
 * 	depending on the type of a2d being used.
 */ 
void ADCON1::setValidCfgBits(unsigned int mask, unsigned int shift)
{
    mValidCfgBits = mask;
    mCfgBitShift = shift;
}

int ADCON1::get_cfg(unsigned int reg)
{
    return((reg & mValidCfgBits) >> mCfgBitShift);
}

/*
 * Associate a processor I/O pin with an A2D channel
 */
void ADCON1::setIOPin(unsigned int channel, PinModule *newPin)
{
  if (channel < m_nAnalogChannels && 
      m_AnalogPins[channel] == &AnInvalidAnalogInput && newPin!=0) {
    m_AnalogPins[channel] = newPin;
  } else {
    printf("%s:%d WARNING invalid channel number config for ADCON1\n",__FILE__,__LINE__);
  }
}


//------------------------------------------------------
double ADCON1::getChannelVoltage(unsigned int channel)
{
  double voltage=0.0;
  if(channel <= m_nAnalogChannels) {
    if ( (1<<channel) & m_configuration_bits[get_cfg(value.data)]) {
      PinModule *pm = m_AnalogPins[channel];
      if (pm != &AnInvalidAnalogInput)
          voltage = pm->getPin().get_nodeVoltage();
      else
      {
	cout << "ADCON1::getChannelVoltage channel " << channel << 
		" not valid analog input\n";
	voltage = 0.;
      }
    }
    else
    {
	cout << "ADCON1::getChannelVoltage channel " << channel <<
                " not a configured input\n";
    }
  }

  return voltage;
}

double ADCON1::getVrefHi()
{
  if (Vrefhi_position[get_cfg(value.data)] < m_nAnalogChannels)
    return getChannelVoltage(Vrefhi_position[get_cfg(value.data)]);

  return ((Processor *)cpu)->get_Vdd();
}

double ADCON1::getVrefLo()
{
  if (Vreflo_position[get_cfg(value.data)] < m_nAnalogChannels)
    return getChannelVoltage(Vreflo_position[get_cfg(value.data)]);

  return 0.0;
}

ANSEL::ANSEL(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc),
    adcon1(0)
{
}

void ANSEL::setAdcon1(ADCON1 *new_adcon1)
{
  adcon1 = new_adcon1;
}

void ANSEL::put(unsigned int new_value)
{
  unsigned int cfgmax = adcon1->get_cfg(0xff)+1;
  unsigned int i;
  unsigned int mask;
  trace.raw(write_trace.get() | value.get());
  /*
	Generate ChannelConfiguration from ansel register
  */
  for(i=0; i < cfgmax; i++)
  {
	mask = new_value;
	if (adcon1->getVrefHiChannel(i) < 16)
		mask |= 1 << adcon1->getVrefHiChannel(i);
	if (adcon1->getVrefLoChannel(i) < 16)
		mask |= 1 << adcon1->getVrefLoChannel(i);

	adcon1->setChannelConfiguration(i, mask);
  }
  value.put(new_value);
}

