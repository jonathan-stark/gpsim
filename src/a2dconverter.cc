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

#include "ioports.h"
#include "trace.h"
#include "gpsim_time.h"
#include "ui.h"
#include "pic-processor.h"
#include "a2dconverter.h"
#include "14bit-processors.h"

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
    channel_mask(7), channel_shift(3), GO_bit(GO)
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

  Dprintf(("starting A/D conversion at 0x%" PRINTF_GINT64_MODIFIER "x\n",get_cycles().get() ));

  if(!(value.get() & ADON) ) {
    Dprintf((" A/D converter is disabled\n"));
    stop_conversion();
    return;
  }

  guint64 fc = get_cycles().get() + (2 * Tad) /
		p_cpu->get_ClockCycles_per_Instruction();

  Dprintf(("ad_state %d fc %lx now %lx\n", ad_state, fc, get_cycles().get()))
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


void ADCON0::set_Tad(unsigned int new_value)
{
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
}

void ADCON0::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());

  set_Tad(new_value);
    
  unsigned int old_value=value.get();
  // SET: Reflect it first!
  value.put(new_value);
  if(new_value & ADON) {
    // The A/D converter is being turned on (or maybe left on)

    if((new_value & ~old_value) & GO_bit) {
      if (verbose)
	printf("starting A2D conversion\n");
      Dprintf(("starting A2D conversion\n"));
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

  Dprintf(("%d-bit result 0x%x ADFM %d\n", m_nBits, converted, get_ADFM()));

  if(adresl) {   // non-null for more than 8 bit conversion

    if(get_ADFM()) {
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

  Dprintf((" ADCON0 Callback: 0x%" PRINTF_GINT64_MODIFIER "x\n",get_cycles().get()));

  //
  // The a/d converter is simulated with a state machine. 
  // 

  switch(ad_state)
    {
    case AD_IDLE:
      Dprintf(("ignoring ad callback since ad_state is idle\n"));
      break;

    case AD_ACQUIRING:
      channel = (value.get() >> channel_shift) & channel_mask;


      m_dSampledVoltage = getChannelVoltage(channel);
      m_dSampledVrefHi  = getVrefHi();
      m_dSampledVrefLo  = getVrefLo();

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
      value.put(value.get() & (~GO_bit));
      set_interrupt();

      ad_state = AD_IDLE;
    }

}

//------------------------------------------------------
// If the ADIF bit is in the PIR1 register, call setPir() 
// in the ADC setup. Otherwise, ADIF is assumed to be in
// the ADCON0 register
//
// Chips like 10f220 do not have interupt and intcon is not defined.
// Thus no interrupt needs be generated
//
void ADCON0::set_interrupt(void)
{
  if (m_pPir)
      m_pPir->set_adif();
  else if (intcon)
  {
      value.put(value.get() | ADIF);
      intcon->peripheral_interrupt();
  }

}


//------------------------------------------------------
// ADCON0
//
ADCON0_DIF::ADCON0_DIF(Processor *pCpu, const char *pName, const char *pDesc)
	: ADCON0(pCpu, pName, pDesc)
{
}
void ADCON0_DIF::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());

  if (new_value & ADRMD)	// 10 Bit
      setA2DBits(10);
  else
      setA2DBits(12);

  set_Tad(new_value);
    
  unsigned int old_value=value.get();
  // SET: Reflect it first!
  value.put(new_value);
  if(new_value & ADON) {
    // The A/D converter is being turned on (or maybe left on)

    if((new_value & ~old_value) & GO_bit) {
      if (verbose)
	printf("starting A2D conversion\n");
      Dprintf(("starting A2D conversion\n"));
      // The 'GO' bit is being turned on, which is request to initiate
      // and A/D conversion
      start_conversion();
    }
  } else
    stop_conversion();

}
void ADCON0_DIF::put_conversion(void)
{
  int channel = adcon2->value.get() & 0x0f;
  double dRefSep = m_dSampledVrefHi - m_dSampledVrefLo;
  double dNormalizedVoltage;
  double m_dSampledVLo;

  if (channel == 0x0e) // shift AN21 to adcon0 channel
	channel = 0x15;
  if (channel == 0x0f)	// use ADNREF for V-
	m_dSampledVLo = getVrefLo();
  else
	m_dSampledVLo = getChannelVoltage(channel);

  dNormalizedVoltage = (m_dSampledVoltage - m_dSampledVLo)/dRefSep;
  dNormalizedVoltage = dNormalizedVoltage>1.0 ? 1.0 : dNormalizedVoltage;

  int converted = (int)(m_A2DScale*dNormalizedVoltage + 0.5);

  Dprintf(("put_conversion: V+:%g V-:%g Vrefhi:%g Vreflo:%g conversion:%d normV:%g\n",
	m_dSampledVoltage, m_dSampledVLo,
	   m_dSampledVrefHi,m_dSampledVrefLo,converted,dNormalizedVoltage));

  if (verbose)
	printf ("result=0x%02x\n", converted);

  Dprintf(("%d-bit result 0x%x ADFM %d\n", m_nBits, converted, get_ADFM()));

    if(!get_ADFM()) { // signed

	int sign = 0;
	if (converted < 0)
	{
		sign = 1;
		converted = -converted;
	}

	converted = ((converted << (16-m_nBits)) % 0xffff) | sign;
    }
      adresl->put(converted & 0xff);
      adres->put((converted >> 8) & 0xff);

}

ADCON1_16F::ADCON1_16F(Processor *pCpu, const char *pName, const char *pDesc)
  : ADCON1(pCpu, pName, pDesc), FVR_chan(99)
{
    valid_bits = 0x70;
}
void ADCON1_16F::put_value(unsigned int new_value)
{
    unsigned int masked_value = new_value & valid_bits;
     unsigned int Tad = 6;
    setADCnames();

    switch(masked_value & (ADCS0 | ADCS1))
    {

    case 0:
      Tad =  (new_value & ADCS2) ? 4 : 2;
      break;

    case ADCS0:
      Tad =  (new_value & ADCS2) ? 16 : 8;
      break;

    case ADCS1:
      Tad =  (new_value & ADCS2) ? 64 : 32;
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
    adcon0->set_Tad(Tad);
    if (ADFM & valid_bits)
	adfm = ADFM & masked_value;

    //RRR FIXME handle ADPREF
    value.put(new_value & valid_bits);
}
double ADCON1_16F::getVrefLo()
{
	if (ADNREF & value.get()) 
	{

  	    if (Vreflo_position[cfg_index] < m_nAnalogChannels)
    		return getChannelVoltage(Vreflo_position[cfg_index]);
	    cerr << "WARNING Vreflo pin not configured\n";
	    return -1.;
	}
	return 0.;	//Vss
}

double ADCON1_16F::getVrefHi()
{
  if (ADPREF0 & valid_bits)
  {
	unsigned int mode = value.get() & (ADPREF1 | ADPREF0);
        switch(mode)
	{
	case 0:
  	    return ((Processor *)cpu)->get_Vdd();

	case 1:
	    cerr << "WARNING reserved value for ADPREF\n";
	    return -1.;
 	   
	case 2:
  	    if (Vrefhi_position[cfg_index] < m_nAnalogChannels)
    		return getChannelVoltage(Vrefhi_position[cfg_index]);
	    cerr << "WARNING Vrefhi pin not configured\n";
	    return -1.;

	case 3:
  	    if (FVR_chan < m_nAnalogChannels)
    		return getChannelVoltage(FVR_chan);
	    cerr << "WARNING FVR_chan not set\n";
	    return -1.;
	};
	    
  }
  if (Vrefhi_position[cfg_index] < m_nAnalogChannels)
    return getChannelVoltage(Vrefhi_position[cfg_index]);

  return ((Processor *)cpu)->get_Vdd();
}
//------------------------------------------------------
// ADCON1
//
ADCON1::ADCON1(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc),
    valid_bits(0xff), adfm(false),
    m_AnalogPins(0),  m_voltageRef(0), m_nAnalogChannels(0),
    mValidCfgBits(0), mCfgBitShift(0), mIoMask(0), cfg_index(0), 
    m_ad_in_ctl(0)
{
  for (int i=0; i<(int)cMaxConfigurations; i++) {
    setChannelConfiguration(i, 0);
    setVrefLoConfiguration(i, 0xffff);
    setVrefHiConfiguration(i, 0xffff);
  }
}

ADCON1::~ADCON1()
{
  if (m_voltageRef) delete [] m_voltageRef;
  if (m_AnalogPins)
  {
    if (m_ad_in_ctl)
    {
	for (unsigned int i = 0; i < m_nAnalogChannels; i++)	
	    m_AnalogPins[i]->setControl(0);
	delete m_ad_in_ctl;
    }
	
    delete [] m_AnalogPins;
  }
}

void ADCON1::put(unsigned int new_value)
{
  unsigned int masked_value = new_value & valid_bits;
  trace.raw(write_trace.get() | value.get());
  put_value(masked_value);
}
void ADCON1::put_value(unsigned int new_value)
{
    unsigned int masked_value = new_value & valid_bits;
    cfg_index = get_cfg(masked_value);
    setADCnames();
    adfm = masked_value & ADFM;
    value.put(masked_value);
}

// Obtain new mIoMask and set pin names as per function
void ADCON1::setADCnames()
{
    unsigned int new_mask = m_configuration_bits[cfg_index];
    unsigned int diff = mIoMask ^ new_mask;

    Dprintf (( "ADCON1::setADCnames - cfg_index=%d new_mask %02X channels %d\n",
               cfg_index, new_mask, m_nAnalogChannels ));

    char newname[20];


    for(unsigned int i = 0; i < m_nAnalogChannels; i++)
    {
      if ((diff & (1 << i)) && m_AnalogPins[i] != &AnInvalidAnalogInput)
      {

        if (new_mask & (1<<i))
        {
          sprintf(newname, "an%d", i);
          m_AnalogPins[i]->AnalogReq(this, true, newname);
        }
        else
          m_AnalogPins[i]->AnalogReq(this, false, m_AnalogPins[i]->getPin().name().c_str());
      }  
    }
    mIoMask = new_mask;
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
  Dprintf(("ADCON1::getVrefHiChannel cfg=%d %d\n", cfg, Vrefhi_position[cfg]));
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
  PinModule **save = NULL;	// save existing pins when nChannels increases

  if (!nChannels || nChannels <= m_nAnalogChannels)
    return;	// do nothing if NChannels decreases

  if (m_nAnalogChannels && nChannels > m_nAnalogChannels )
        save = m_AnalogPins;

  if (m_voltageRef)
	delete [] m_voltageRef;

  m_voltageRef = new float [nChannels];
  m_AnalogPins = new PinModule *[nChannels];

  for (unsigned int i=0; i<nChannels; i++)
  {
    m_voltageRef[i] = -1.0;
    if(i < m_nAnalogChannels)
    {
        if (save)
            m_AnalogPins[i] = save[i];
    }
    else
        m_AnalogPins[i] = &AnInvalidAnalogInput;
  }
  if (save)
      delete [] save;

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

unsigned int ADCON1::get_adc_configmask(unsigned int reg)
{
    return(m_configuration_bits[get_cfg(reg)]);
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
    printf("%s:%d WARNING invalid channel number config for ADCON1 %d num %d\n",__FILE__,__LINE__, channel, m_nAnalogChannels);
   
  }
}


void ADCON1::setVoltRef(unsigned int channel, float value)
{
    if (channel < m_nAnalogChannels )
    {
	m_voltageRef[channel] = value;
    }
    else
    {
	printf("ADCON1::%s invalid channel number %d\n", __FUNCTION__, channel);
    }
}

//------------------------------------------------------
double ADCON1::getChannelVoltage(unsigned int channel)
{
  double voltage=0.0;


  if(channel < m_nAnalogChannels) {
    if ( (1<<channel) & m_configuration_bits[cfg_index] ) {
      PinModule *pm = m_AnalogPins[channel];
      if (pm != &AnInvalidAnalogInput)
      {
          voltage = pm->getPin().get_nodeVoltage();
      }
      else
      {
	cerr << "ADCON1::getChannelVoltage channel " << channel << 
		" not valid analog input\n";
	cerr << "Please raise a Gpsim bug report\n";
      }
    }
    else	// use voltage reference
    {
	Dprintf(("ADCON1::getChannelVoltage channel=%d voltage %f\n", \
		channel, m_voltageRef[channel]));
	voltage = m_voltageRef[channel];
	if (voltage < 0.0)
	{
	    cout << "ADCON1::getChannelVoltage channel " << channel <<
                " not a configured input\n";
	    voltage = 0.0;
	}
    }
  }
  else
  {
	cerr << "ADCON1::getChannelVoltage channel " << channel <<
                " >= "
		<< m_nAnalogChannels << " (number of channels)\n";
	cerr << "Please raise a Gpsim bug report\n";
  }

  return voltage;
}

double ADCON1::getVrefHi()
{
  if (Vrefhi_position[cfg_index] < m_nAnalogChannels)
    return getChannelVoltage(Vrefhi_position[cfg_index]);

  return ((Processor *)cpu)->get_Vdd();
}

double ADCON1::getVrefLo()
{
  if (Vreflo_position[cfg_index] < m_nAnalogChannels)
    return getChannelVoltage(Vreflo_position[cfg_index]);

  return 0.0;
}

//	if on is true, set pin as input regardless of TRIS state
//	else restore TRIS control
//
void ADCON1::set_channel_in(unsigned int channel, bool on)
{
    Dprintf(("channel=%d on=%d m_ad_in_ctl=%p\n", channel, on, m_ad_in_ctl));

    if (on && (m_ad_in_ctl == NULL))
	m_ad_in_ctl = new AD_IN_SignalControl();

    if (on) 
	m_AnalogPins[channel]->setControl(m_ad_in_ctl);
    else   
	m_AnalogPins[channel]->setControl(0);

    m_AnalogPins[channel]->updatePinModule();
}

ANSEL::ANSEL(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc),
    adcon1(0), anselh(0), valid_bits(0x7f)
{
}

void ANSEL::setAdcon1(ADCON1 *new_adcon1)
{
  adcon1 = new_adcon1;
}

void ANSEL::put(unsigned int new_value)
{
  unsigned int cfgmax = adcon1->getNumberOfChannels();
  unsigned int i;
  unsigned int mask = (new_value & valid_bits);


  if (anselh) mask |= anselh->value.get() << 8;
  trace.raw(write_trace.get() | value.get());
  /*
	Generate ChannelConfiguration from ansel register
  */
  for(i=0; i < cfgmax; i++)
  {
	adcon1->setChannelConfiguration(i, mask);
  }
  value.put(new_value & valid_bits);
  adcon1->setADCnames();
}


ANSEL_H::ANSEL_H(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc),
    adcon1(0), ansel(0), valid_bits(0x3f)
{
}

void ANSEL_H::setAdcon1(ADCON1 *new_adcon1)
{
  adcon1 = new_adcon1;
}

void ANSEL_H::put(unsigned int new_value)
{
  unsigned int cfgmax = adcon1->getNumberOfChannels();
  unsigned int i;
  unsigned int mask = ((new_value & valid_bits) << 8) ;
  trace.raw(write_trace.get() | value.get());

  if (ansel)
       mask |= ansel->value.get();

  /*
	Generate ChannelConfiguration from ansel register
  */
  for(i=0; i < cfgmax; i++)
  {
	adcon1->setChannelConfiguration(i, mask);
  }
  value.put(new_value & valid_bits);
  adcon1->setADCnames();
}

ANSEL_P::ANSEL_P(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc),
    adcon1(0), ansel(0), valid_bits(0x3f), cfg_mask(0)
{
}

void ANSEL_P::setAdcon1(ADCON1 *new_adcon1)
{
  adcon1 = new_adcon1;
}

void ANSEL_P::put(unsigned int new_value)
{
  unsigned int cfgmax = adcon1->getMaxCfg();
  unsigned int i;
  unsigned int mask;
  unsigned int chan = first_channel;
  trace.raw(write_trace.get() | value.get());

  new_value &= valid_bits;
  value.put(new_value);

  cfg_mask = 0;
  for(i=0; i< 8; i++)
  {
      if ((1<<i) & analog_pins)
      {
	if ((1<<i) & new_value)
	    cfg_mask |= (1<<chan);
	chan++;
      }
  }
  mask = cfg_mask;

  if (ansel)
  {
      mask |= ansel->get_mask();
  }
  if (!adcon1)
	return;
  /*
	Generate ChannelConfiguration from ansel register
  */
  for(i=0; i < cfgmax; i++)
  {
	adcon1->setChannelConfiguration(i, mask);
  }
  adcon1->setADCnames();
}


//------------------------------------------------------
// ADCON0_10
//
ADCON0_10::ADCON0_10(Processor *pCpu, const char *pName, const char *pDesc)
  : ADCON0(pCpu, pName, pDesc)
{
  GO_bit = GO;	//ADCON0 and ADCON0_10 have GO flag at different bit
  // It should take 13 CPU instructions to complete conversion
  // Tad of 6 completes in 15 
  Tad = 6;
}
void ADCON0_10::put(unsigned int new_value)
{
  unsigned int old_value=value.get();
 /* On first call of this function old_value has already been set to
 *  it's default value, but we want to call set_channel_in. First 
 *  gives us a way to do this.
 */
  static bool first = true;

  trace.raw(write_trace.get() | value.get());

  Dprintf(("ADCON0_10::put new_value=0x%02x old_value=0x%02x\n", new_value, old_value));
  
  if ((new_value ^ old_value) & ANS0 || first)
	adcon1->set_channel_in(0, (new_value & ANS0) == ANS0); 
  if ((new_value ^ old_value) & ANS1 || first )
	adcon1->set_channel_in(1, (new_value & ANS1) == ANS1); 

  first = false;

  // If ADON is clear GO cannot be set
  if ((new_value & ADON) != ADON)
	new_value &= ~GO_bit;


  // SET: Reflect it first!
  value.put(new_value);
  if(new_value & ADON) {
    // The A/D converter is being turned on (or maybe left on)

    if((new_value & ~old_value) & GO_bit) {
      if (verbose)
	printf("starting A2D conversion\n");
      // The 'GO' bit is being turned on, which is request to initiate
      // and A/D conversion
      start_conversion();
    }
  } else
    stop_conversion();

}
//------------------------------------------------------
// ADCON0_12F used in 12f675. Uses ADCON1 as virtual rather than physical
// register
//
ADCON0_12F::ADCON0_12F(Processor *pCpu, const char *pName, const char *pDesc)
  : ADCON0(pCpu, pName, pDesc)
{
  GO_bit = GO;	//ADCON0 and ADCON0_10 have GO flag at different bit
  valid_bits = 0xdf;
}



void ADCON0_12F::put(unsigned int new_value)
{
  unsigned int old_value=value.get();
  new_value &= valid_bits;	// clear unused bits
 /* On first call of this function old_value has already been set to
 *  it's default value, but we want to call set_channel_in. First 
 *  gives us a way to do this.
 */

  trace.raw(write_trace.get() | value.get());

  Dprintf(("ADCON0_12F::put new_value=0x%02x old_value=0x%02x\n", new_value, old_value));
  // tell adcon1 to use Vref 
  adcon1->set_cfg_index((new_value & VCFG) ? 2: 0);
  

  // If ADON is clear GO cannot be set
  if ((new_value & ADON) != ADON)
	new_value &= ~GO_bit;


  // SET: Reflect it first!
  value.put(new_value);
  if(new_value & ADON) {
    // The A/D converter is being turned on (or maybe left on)

    if((new_value & ~old_value) & GO_bit) {
      if (verbose)
	printf("starting A2D conversion\n");
      // The 'GO' bit is being turned on, which is request to initiate
      // and A/D conversion
      start_conversion();
    }
  } else
    stop_conversion();

}
ANSEL_12F::ANSEL_12F(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc),
    adcon1(0)
{
}


void ANSEL_12F::set_tad(unsigned int new_value)
{
   unsigned int Tad = 6;

  switch(new_value & (ADCS0 | ADCS1))
    {

    case 0:
      Tad =  (new_value & ADCS2) ? 4 : 2;
      break;

    case ADCS0:
      Tad =  (new_value & ADCS2) ? 16 : 8;
      break;

    case ADCS1:
      Tad =  (new_value & ADCS2) ? 64 : 32;
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
   Dprintf(("ANSEL_12F::set_tad %x Tad=%d\n", new_value, Tad));
  adcon0->set_Tad(Tad);

}
void ANSEL_12F::put(unsigned int new_value)
{
  unsigned int cfgmax = adcon1->getNumberOfChannels();
  unsigned int i;
  unsigned int mask;

  Dprintf(("ANSEL_12F::put %x cfgmax %d\n", new_value, cfgmax));
  trace.raw(write_trace.get() | value.get());
  /*
	Generate ChannelConfiguration from ansel register
  */
  for(i=0; i < cfgmax; i++)
  {
	mask = new_value & 0x0f;
	adcon1->setChannelConfiguration(i, mask);
  }
  /*
  * 	Convert A2D conversion times and set in adcon
  */
  set_tad(new_value & ( ADCS2 | ADCS1 | ADCS0));
  value.put(new_value & 0x7f);
  adcon1->setADCnames();
}

// catch stimulus and set channel voltage
//
a2d_stimulus::a2d_stimulus(ADCON1 * arg, int chan, const char *cPname,double _Vth, double _Zth)
  : stimulus(cPname, _Vth, _Zth)
{
        _adcon1 = arg;
	channel = chan;
}

void   a2d_stimulus::set_nodeVoltage(double v)
{
	Dprintf(("nodeVoltage =%.1f\n", v));
        nodeVoltage = v;
        _adcon1->setVoltRef(channel, v);
}
//
//--------------------------------------------------
// member functions for the FVRCON class
//--------------------------------------------------
//
FVRCON::FVRCON(Processor *pCpu, const char *pName, const char *pDesc, unsigned int bitMask, unsigned int alwaysOne)
  : sfr_register(pCpu, pName, pDesc),
	adcon1(0), daccon0(0), cmModule(0), cpscon0(0)
{
    mask_writable = bitMask;
    always_one = alwaysOne;
}

void  FVRCON::put(unsigned int new_value)
{
  unsigned int masked_value = (new_value & mask_writable) | always_one;
  trace.raw(write_trace.get() | value.get());

  value.put(masked_value);
  compute_VTemp(masked_value);
  compute_FVR_AD(masked_value);
  compute_FVR_CDA(masked_value);
}

void  FVRCON::put_value(unsigned int new_value)
{
  unsigned int masked_value = (new_value & mask_writable) | always_one;
  put(masked_value);

  update();
}


double FVRCON::compute_VTemp(unsigned int fvrcon)
{
    double ret = -1.;
    double Vt;	//Transister junction threshold voltage at core temperature

    if((fvrcon & TSEN) && cpu14e->m_cpu_temp)
    { 
        Vt = 0.659 - ( cpu14e->m_cpu_temp->getVal() + 40.) * 0.00132;
	ret = ((Processor *)cpu)->get_Vdd() - ((fvrcon&TSRNG)?4.:2.) * Vt;
	if (ret < 0.)
	{
	    ret = -1.;
	    cerr << "Warning FVRCON Vdd too low for temperature range\n";
	}
    }
    if(adcon1) adcon1->setVoltRef(VTemp_AD_chan, ret);
    return ret;
}

double FVRCON::compute_FVR_AD(unsigned int fvrcon)
{
    double ret = -1.;
    unsigned int gain = (fvrcon & (ADFVR0|ADFVR1));

    if ((fvrcon & FVREN) && gain)
	ret = 1.024 * (1 << (gain - 1));
    if (ret > ((Processor *)cpu)->get_Vdd())
    {
	cerr << "warning FVRCON FVRAD > Vdd\n";
	ret = -1.;
    }
    if(adcon1)adcon1->setVoltRef(FVRAD_AD_chan, ret);
    return ret;
}

double FVRCON::compute_FVR_CDA(unsigned int fvrcon)
{
    double ret = 0.;
    unsigned int gain = (fvrcon & (CDAFVR0|CDAFVR1))>>2;

    if ((fvrcon & FVREN) && gain)
	ret = 1.024 * (1 << (gain - 1));

    for (unsigned int i= 0; i < daccon0_list.size(); i++)
    {
	daccon0_list[i]->set_FVR_CDA_volt(ret);
    }
    if(cmModule) cmModule->set_FVR_volt(ret);
    if(cpscon0) cpscon0->set_FVR_volt(ret);
    return ret;
}
//
//--------------------------------------------------
// member functions for the DAC classes
//--------------------------------------------------
//
DACCON0::DACCON0(Processor *pCpu, const char *pName, const char *pDesc, unsigned int bitMask, unsigned int bit_res)
  : sfr_register(pCpu, pName, pDesc),
    adcon1(0), cmModule(0), cpscon0(0),
    bit_mask(bitMask), bit_resolution(bit_res),
    FVR_CDA_volt(0.)
{
	Pin_Active[0] = false;
	Pin_Active[1] = false;
}

void  DACCON0::put(unsigned int new_value)
{
  unsigned int masked_value = (new_value & bit_mask);
  trace.raw(write_trace.get() | value.get());
  value.put(masked_value);
  compute_dac(masked_value);
}

void  DACCON0::put_value(unsigned int new_value)
{
  unsigned int masked_value = (new_value & bit_mask);
  value.put(masked_value);
  compute_dac(masked_value);

  update();
}

void DACCON0::set_dcaccon1_reg(unsigned int reg)
{
    daccon1_reg = reg;
    compute_dac(value.get());
}

void  DACCON0::compute_dac(unsigned int value)
{
    double Vhigh = get_Vhigh(value);
    double Vlow = 0.;
    double Vout;

    if(value & DACEN)	// DAC is enabled
    {
	Vout = (Vhigh - Vlow) * daccon1_reg/bit_resolution - Vlow;
    }
    else if (value & DACLPS)
	Vout = Vhigh;
    else
	Vout = Vlow;

    set_dacoutpin(value & DACOE, 0, Vout);
    set_dacoutpin(value & DACOE2, 1, Vout);
	
    if (verbose)
        printf("%s-%d adcon1 %p FVRCDA_AD_chan %d Vout %.2f\n", __FUNCTION__, __LINE__, adcon1, FVRCDA_AD_chan, Vout);
    if(adcon1) adcon1->setVoltRef(FVRCDA_AD_chan, Vout);
    if(cmModule) cmModule->set_DAC_volt(Vout);
    if(cpscon0) cpscon0->set_DAC_volt(Vout);
}

void DACCON0::set_dacoutpin(bool output_enabled, int chan, double Vout)
{
    IO_bi_directional_pu *out_pin;

    if (output_pin[chan])
         out_pin = (IO_bi_directional_pu *) &(output_pin[chan]->getPin());
    else
	return;

    if (output_enabled)
    {
	if (!Pin_Active[chan])	// DACOUT going to active
	{
	    string pin_name = name().substr(0, 4);
	    if (pin_name.find("dacc", 0) == 0)
		pin_name = "dacout";
	    else if (chan == 0)
	        pin_name += "-1";
	    else
		pin_name += "-2";

	    output_pin[chan]->AnalogReq(this, true, pin_name.c_str());
	    Pin_Active[chan] = true;
	    Vth[chan] = out_pin->get_VthIn();
	    Zth[chan] = out_pin->get_ZthIn();
	    driving[chan] = out_pin->getDriving();
	    out_pin->set_ZthIn(150e3);
	    out_pin->setDriving(false);
	}

	out_pin->set_VthIn(Vout);
	out_pin->updateNode();
    }
    else if (Pin_Active[chan])	// DACOUT leaving active
    {
	output_pin[chan]->AnalogReq(this, false, output_pin[chan]->getPin().name().c_str());
	Pin_Active[chan] = false;
	out_pin->set_VthIn(Vth[chan]);
	out_pin->set_ZthIn(Zth[chan]);
	out_pin->setDriving(driving[chan]);
	out_pin->updateNode();
    }
}
 	
double DACCON0::get_Vhigh(unsigned int value)
{
    unsigned int mode = (value & (DACPSS0|DACPSS1)) >> 2;
    switch(mode)
    {
    case 0:	// Vdd
	return ((Processor *)cpu)->get_Vdd();

    case 1:	// Vref+ pin, get is from A2D setup
        if(adcon1)
	    return(adcon1->getChannelVoltage(adcon1->getVrefHiChannel(0)));
	cerr << "ERROR DACCON0 DACPSS=01b adcon1 not set\n";
	return 0.;

    case 2:	// Fixed Voltage Reference
	return FVR_CDA_volt;

    case 3:	// Reserved value
	cerr << "ERROR DACCON0 DACPSS=11b is reserved value\n";
	return 0.;

    }
    return 0.;	// cant get here
} 
	

DACCON1::DACCON1(Processor *pCpu, const char *pName, const char *pDesc, unsigned int bitMask, DACCON0 *_daccon0)
  : sfr_register(pCpu, pName, pDesc),
    bit_mask(bitMask), daccon0(_daccon0)
{
}

void  DACCON1::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  put_value(new_value);
}

void  DACCON1::put_value(unsigned int new_value)
{
  unsigned int masked_value = (new_value & bit_mask);
  value.put(masked_value);
  if (daccon0) daccon0->set_dcaccon1_reg(masked_value);

  update();
}

//------------------------------------------------------
// ADCON2_diff for A2D with differential input ie 16f178*
//
ADCON2_DIF::ADCON2_DIF(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc)
{
}

