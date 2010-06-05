/*
   Copyright (C) 2008 Roy R Rankin
   Copyright (C) 2006 T. Scott Dattalo

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
#include "a2d_v2.h"

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
// ADCON0
//
ADCON0_V2::ADCON0_V2(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc),
    adres(0), adresl(0), adcon1(0), adcon2(0), intcon(0), pir1v2(0), ad_state(AD_IDLE),
    channel_mask(15)
{
}

/*
 * Link PIC register for High Byte A/D result
 */
void ADCON0_V2::setAdres(sfr_register *new_adres)
{
  adres = new_adres;
}
/*
 * Link PIC register for Low Byte A/D result
 */
void ADCON0_V2::setAdresLow(sfr_register *new_adresl)
{
  adresl = new_adresl;
}
/*
 * Link PIC register for ADCON1
 */
void ADCON0_V2::setAdcon1(ADCON1_V2 *new_adcon1)
{
  adcon1 = new_adcon1;
}
/*
 * Link PIC register for ADCON2
 */
void ADCON0_V2::setAdcon2(ADCON2_V2 *new_adcon2)
{
  adcon2 = new_adcon2;
}
/*
 * Link PIC register for PIR1
 */
void ADCON0_V2::setPir(PIR1v2 *new_pir1)
{
  pir1v2 = new_pir1;
}
/*
 * Link PIC register for INTCON
 */
void ADCON0_V2::setIntcon(INTCON *new_intcon)
{
  intcon = new_intcon;
}
/*
 * Set Tad time for RC source
 */
void ADCON0_V2::setRCtime(double time)
{
        m_RCtime = time;
}

/*
 * Set resolution of A2D converter
 */
void ADCON0_V2::setA2DBits(unsigned int nBits)
{
  m_A2DScale = (1<<nBits) - 1;
  m_nBits = nBits;
}

void ADCON0_V2::start_conversion(void)
{
  guint64 fc = get_cycles().get();

  Dprintf(("starting A/D conversion\n"));

  if(!(value.get() & ADON) ) {
    Dprintf((" A/D converter is disabled\n"));
    stop_conversion();
    return;
  }

  // Get the A/D Conversion Clock Select bits
  //
  // This switch case will get the ADCS bits and set the Tad, or The A/D
  // converter clock period. Tad is the number of the oscillator periods
  //  rather instruction cycle periods.

  Tad = adcon2->get_tad();
  Tacq = adcon2->get_nacq();

  Dprintf(("\tTad = %d Tacq = %d\n", Tad, Tacq));

  if (Tad == 0) // RC time source
  {
        if (cpu)
        {
           Tad = (m_RCtime * p_cpu->get_frequency());
           Tad = Tad < 2 ? 2 : Tad;
        }
        else
           Tad = 6;
  }

  if (Tacq == 0)
    fc += 1;    // if Tacq is 0,  go to acqusition on next clock cycle
  else
    fc += (Tacq * Tad) / p_cpu->get_ClockCycles_per_Instruction();

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

void ADCON0_V2::stop_conversion(void)
{

  Dprintf(("stopping A/D conversion\n"));

  ad_state = AD_IDLE;

}



void ADCON0_V2::put(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());

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

void ADCON0_V2::put_conversion(void)
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

    if(adcon2->value.get() & ADCON2_V2::ADFM) {
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

// ADCON0_V2 callback is called when the cycle counter hits the break point that
// was set in ADCON0_V2::put.

void ADCON0_V2::callback(void)
{
  int channel;

  Dprintf((" ADCON0_V2 Callback: 0x%"PRINTF_INT64_MODIFIER"x\n",get_cycles().get()));

  //
  // The a/d converter is simulated with a state machine.
  //

  switch(ad_state)
    {
    case AD_IDLE:
      Dprintf(("ignoring ad callback since ad_state is idle\n"));
      break;

    case AD_ACQUIRING:
      channel = (value.get() >> 2) & channel_mask;

      m_dSampledVoltage = adcon1->getChannelVoltage(channel);
      m_dSampledVrefHi  = adcon1->getVrefHi();
      m_dSampledVrefLo  = adcon1->getVrefLo();

      Dprintf(("Acquiring channel:%d V=%g reflo=%g refhi=%g\n",
               channel,m_dSampledVoltage,m_dSampledVrefLo,m_dSampledVrefHi));

      future_cycle = get_cycles().get() + ((m_nBits + 1) * Tad)/p_cpu->get_ClockCycles_per_Instruction();
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
//
void ADCON0_V2::set_interrupt(void)
{
  pir1v2->set_adif();
  intcon->peripheral_interrupt();

}



//------------------------------------------------------
// ADCON1
//
ADCON1_V2::ADCON1_V2(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc),
    m_AnalogPins(0), m_nAnalogChannels(0),
    mValidCfgBits(0), mCfgBitShift(0), m_vrefHiChan(-1),
    m_vrefLoChan(-1),  mIoMask(0), m_adcon0(0)


{
  for (int i=0; i<(int)cMaxConfigurations; i++) {
    setChannelConfiguration(i, 0);
  }
}


void ADCON1_V2::put(unsigned int new_value)
{
    unsigned int new_mask = get_adc_configmask(new_value);
    unsigned int diff = mIoMask ^ new_mask;

    Dprintf (( "ADCON1_V2::put ( %02X ) - new_mask %02X\n", new_value, new_mask ));
    trace.raw(write_trace.get() | value.get());


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
	    {
	      m_AnalogPins[i]->AnalogReq(this, false, m_AnalogPins[i]->getPin().name().c_str());
	    }
          }  
	}
	mIoMask = new_mask;
	value.put(new_value);
}
/*
 * Set the channel used for Vref+ when VCFG0 is set
 */
void ADCON1_V2::setVrefHiChannel(unsigned int channel)
{
        m_vrefHiChan = channel;
}

/*
 * Set the channel used for Vref- when VCFG1 is set
 */
void ADCON1_V2::setVrefLoChannel(unsigned int channel)
{
        m_vrefLoChan = channel;
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
void ADCON1_V2::setChannelConfiguration(unsigned int cfg, unsigned int bitMask)
{
  if (cfg < cMaxConfigurations)
    m_configuration_bits[cfg] = bitMask;
}

/*
 * Performs same function as setChannelConfiguration, but defines
 * all entiries in configuration table in one call.
 */

void ADCON1_V2::setChanTable(
        unsigned int m0, unsigned int m1, unsigned int m2, unsigned int m3,
        unsigned int m4, unsigned int m5, unsigned int m6, unsigned int m7,
        unsigned int m8, unsigned int m9, unsigned int m10, unsigned int m11,
        unsigned int m12, unsigned int m13, unsigned int m14, unsigned int m15)
{
        m_configuration_bits[0] = m0;
        m_configuration_bits[1] = m1;
        m_configuration_bits[2] = m2;
        m_configuration_bits[3] = m3;
        m_configuration_bits[4] = m4;
        m_configuration_bits[5] = m5;
        m_configuration_bits[6] = m6;
        m_configuration_bits[7] = m7;
        m_configuration_bits[8] = m8;
        m_configuration_bits[9] = m9;
        m_configuration_bits[10] = m10;
        m_configuration_bits[11] = m11;
        m_configuration_bits[12] = m12;
        m_configuration_bits[13] = m13;
        m_configuration_bits[14] = m14;
        m_configuration_bits[15] = m15;
}





/*
 * Number of A2D channels processor supports
 */
void ADCON1_V2::setNumberOfChannels(unsigned int nChannels)
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
 *      The register is first anded with mask and then shifted
 *      right shift bits. The result being either PCFG or VCFG
 *      depending on the type of a2d being used.
 */
void ADCON1_V2::setValidCfgBits(unsigned int mask, unsigned int shift)
{
    mValidCfgBits = mask;
    mCfgBitShift = shift;
}

/*
 * get_adc_configmask() is called with the value of the adcon1 register
 *
 * if the configuration bit mask is less than 16, the confiiguration bit table
 * is used to determine if the channel is an analog port.
 *
 * Otherwise, each bit in the adcon1 register indicates that the port is
 * digital(1) or analog(0) aka the 18f1220.
 *
 * */
unsigned int ADCON1_V2::get_adc_configmask(unsigned int reg)
{

    if (mValidCfgBits <= 0xf) // use config bit table
    {
      return (m_configuration_bits[(reg >>  mCfgBitShift) & mValidCfgBits]);
    }
    else // register directly gives Analog ports (18f1220)
    {
      return (~(reg >> mCfgBitShift) & mValidCfgBits);
    }
}

/*
 * Associate a processor I/O pin with an A2D channel
 */
void ADCON1_V2::setIOPin(unsigned int channel, PinModule *newPin)
{

  if (channel < m_nAnalogChannels &&
      m_AnalogPins[channel] == &AnInvalidAnalogInput && newPin!=0) {
    m_AnalogPins[channel] = newPin;
  } else {
    printf("WARNING %s channel %d, cannot set IOpin\n",__FUNCTION__, channel);
    if (m_AnalogPins[channel] != &AnInvalidAnalogInput)
        printf("Pin Already assigned\n");
    else if (channel > m_nAnalogChannels)
        printf("channel %d >= number of channels %d\n", channel,  m_nAnalogChannels);
  }
}


//------------------------------------------------------
double ADCON1_V2::getChannelVoltage(unsigned int channel)
{
  double voltage=0.0;
  if(channel <= m_nAnalogChannels) {
    if ( (1<<channel) & get_adc_configmask(value.data) ) {
      PinModule *pm = m_AnalogPins[channel];
      if (pm != &AnInvalidAnalogInput)
          voltage = pm->getPin().get_nodeVoltage();
      else
      {
        cout << "ADCON1_V2::getChannelVoltage channel " << channel <<
                " not a valid pin\n";
        voltage = 0.;
      }
    }
    else
    {
        cout << "ADCON1_V2::getChannelVoltage channel " << channel <<
                " not analog\n";
    }
  }
  else
  {
        cout << "ADCON1_V2::getChannelVoltage channel " << channel <<
                " > m_nAnalogChannels " << m_nAnalogChannels << "\n";
  }

  return voltage;
}

double ADCON1_V2::getVrefHi()
{

  assert(m_vrefHiChan >= 0);    // m_vrefHiChan has not been set
  if ( (m_adcon0 && (m_adcon0->value.data & ADCON0_V2::VCFG0)) ||
          ( !m_adcon0 && (value.data & VCFG0))) // Use Vref+
        return(getChannelVoltage(m_vrefHiChan));

  return ((Processor *)cpu)->get_Vdd();
}

double ADCON1_V2::getVrefLo()
{

  assert(m_vrefLoChan >= 0);    // m_vrefLoChan has not been set
  if ( (m_adcon0 && (m_adcon0->value.data & ADCON0_V2::VCFG1)) ||
          ( !m_adcon0 && (value.data & VCFG1))) // Use Vref-
    return getChannelVoltage(m_vrefLoChan);

  return 0.0;
}



//------------------------------------------------------
// ADCON0
//
ADCON2_V2::ADCON2_V2(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc)
{
}
char ADCON2_V2::get_nacq()
{
  static char acq_tab[8] = { 0, 2, 4, 6, 8, 12, 16, 20};
  return(acq_tab[((value.get() & (ACQT2 | ACQT1 | ACQT0)) >> 3) ]);
}
char ADCON2_V2::get_tad()
{
  static char adcs_tab[8] = { 2, 8, 32, 0, 4, 16, 64, 0};
  return(adcs_tab[(value.get() & (ADCS2 | ADCS1 | ADCS0)) ]);
}
bool ADCON2_V2::get_adfm()
{
   return((value.get() & ADFM) == ADFM);
}
