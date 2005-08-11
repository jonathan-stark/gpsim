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
along with gpasm; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


//
// p16x7x
//
//  This file supports:
//    P16C71


#include <stdio.h>
#include <iostream>
#include <string>

#include "../config.h"
#include "symbol.h"

#include "p16x7x.h"
#include "stimuli.h"

//#define DEBUG_AD


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
  : ad_state(AD_IDLE)
{
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

  for (int i=0; i<m_nAnalogChannels; i++)
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

//------------------------------------------------------


void P16C71::create_sfr_map(void)
{

  if(verbose)
    cout << "creating c71 registers \n";

  add_sfr_register(&adcon0, 0x08, RegisterValue(0,0));
  add_sfr_register(&adcon1, 0x88, RegisterValue(0,0));

  add_sfr_register(&adres,  0x89, RegisterValue(0,0));
  add_sfr_register(&adres,  0x09, RegisterValue(0,0));

  adcon1.setValidCfgBits(ADCON1::PCFG0 | ADCON1::PCFG1);
  adcon1.setNumberOfChannels(4);
  adcon1.setIOPin(0, &(*m_porta)[0]);
  adcon1.setIOPin(1, &(*m_porta)[1]);
  adcon1.setIOPin(2, &(*m_porta)[2]);
  adcon1.setIOPin(3, &(*m_porta)[3]);
  adcon1.setChannelConfiguration(0, 0x0f);
  adcon1.setChannelConfiguration(1, 0x0f);
  adcon1.setChannelConfiguration(2, 0x03);
  adcon1.setChannelConfiguration(3, 0x00);
  adcon1.setVrefHiConfiguration(1, 3);

  adcon0.adres = &adres;
  adcon0.adresl = 0;
  adcon0.adcon1 = &adcon1;
  adcon0.intcon = &intcon_reg;
  intcon = &intcon_reg;

}


void P16C71::create_symbols(void)
{
}

void P16C71::create(void)
{

  P16C61::create();

  create_sfr_map();

}

Processor * P16C71::construct(void)
{

  P16C71 *p = new P16C71;

  cout << " c71 construct\n";

  p->create();
  p->create_invalid_registers ();
  p->pic_processor::create_symbols();

  p->new_name("p16c71");
  symbol_table.add_module(p,p->name().c_str());

  return p;

}


P16C71::P16C71(void)
{
  if(verbose)
    cout << "c71 constructor, type = " << isa() << '\n';
}


//--------------------------------------

void P16C712::create_sfr_map(void)
{

  if(verbose)
    cout << "creating c712/6 registers \n";

  /* Extra timers and Capture/Compare are like in 16x63 => 16X6X code */
  P16X6X_processor::create_sfr_map();

  /* The A/D section is similar to 16x71, but not equal */
  add_sfr_register(&adcon0, 0x1F, RegisterValue(0,0));
  add_sfr_register(&adcon1, 0x9F, RegisterValue(0,0));

  add_sfr_register(&adres,  0x1E, RegisterValue(0,0));

  //1adcon0.analog_port = porta;
  adcon0.adres = &adres;
  adcon0.adresl = 0;
  adcon0.adcon1 = &adcon1;
  adcon0.intcon = &intcon_reg;
  //adcon0.channel_mask = 3;
  intcon = &intcon_reg;

  adcon1.setValidCfgBits(ADCON1::PCFG0 | ADCON1::PCFG1| ADCON1::PCFG2);
  adcon1.setNumberOfChannels(4);
  adcon1.setIOPin(0, &(*m_porta)[0]);
  adcon1.setIOPin(1, &(*m_porta)[1]);
  adcon1.setIOPin(2, &(*m_porta)[2]);
  adcon1.setIOPin(3, &(*m_porta)[3]);
  adcon1.setChannelConfiguration(0, 0x0f);
  adcon1.setChannelConfiguration(1, 0x0f);
  adcon1.setChannelConfiguration(2, 0x0f);
  adcon1.setChannelConfiguration(3, 0x0f);
  adcon1.setChannelConfiguration(4, 0x0b);
  adcon1.setChannelConfiguration(5, 0x0b);
  adcon1.setChannelConfiguration(6, 0x00);
  adcon1.setChannelConfiguration(7, 0x00);
  adcon1.setVrefHiConfiguration(1, 3);
  adcon1.setVrefHiConfiguration(3, 3);
  adcon1.setVrefHiConfiguration(5, 3);

  adcon0.new_name("adcon0");
  adcon1.new_name("adcon1");
  adres.new_name("adres");
}



void P16C712::create(void)
{

  if(verbose)
    cout << " c712/6 create \n";
  create_iopin_map(); /* 14 bits 18 pins connections */
  _14bit_processor::create();
  create_sfr_map();
  //1ccp1con.iopin = portb->pins[2];

}

Processor * P16C712::construct(void)
{

  P16C712 *p = new P16C712;

  cout << " c712 construct\n";

  p->create();
  p->create_invalid_registers ();
  p->pic_processor::create_symbols();

  p->new_name("p16c712");
  symbol_table.add_module(p,p->name().c_str());

  return p;

}


P16C712::P16C712(void)
{
  if(verbose)
    cout << "c712 constructor, type = " << isa() << '\n';

}


//--------------------------------------

Processor * P16C716::construct(void)
{

  P16C716 *p = new P16C716;

  cout << " c716 construct\n";

  p->create();
  p->create_invalid_registers ();
  p->pic_processor::create_symbols();

  p->new_name("p16c716");
  symbol_table.add_module(p,p->name().c_str());

  return p;

}


P16C716::P16C716(void)
{
  if(verbose)
    cout << "c716 constructor, type = " << isa() << '\n';

}


//--------------------------------------

void P16C72::create_sfr_map(void)
{

  if(verbose)
    cout << "creating c72 registers \n";

  // Parent classes just set PIR version 1
  pir_set_2_def.set_pir1(&pir1_2_reg);
  pir_set_2_def.set_pir2(&pir2_2_reg);

  add_sfr_register(&adcon0, 0x1f, RegisterValue(0,0));
  add_sfr_register(&adcon1, 0x9f, RegisterValue(0,0));

  add_sfr_register(&adres,  0x1e, RegisterValue(0,0));

  adcon0.adres = &adres;
  adcon0.adresl = 0;
  adcon0.adcon1 = &adcon1;
  adcon0.intcon = &intcon_reg;
  // adcon0.pir_set = get_pir_set();
  adcon0.pir_set = &pir_set_2_def;
  //adcon0.channel_mask = 7;  // even though there are only 5 inputs...

  intcon = &intcon_reg;

  adcon1.setValidCfgBits(ADCON1::PCFG0 | ADCON1::PCFG1| ADCON1::PCFG2);
  adcon1.setNumberOfChannels(5);
  adcon1.setIOPin(0, &(*m_porta)[0]);
  adcon1.setIOPin(1, &(*m_porta)[1]);
  adcon1.setIOPin(2, &(*m_porta)[2]);
  adcon1.setIOPin(3, &(*m_porta)[3]);
  adcon1.setIOPin(4, &(*m_porta)[5]);
  adcon1.setChannelConfiguration(0, 0x1f);
  adcon1.setChannelConfiguration(1, 0x1f);
  adcon1.setChannelConfiguration(2, 0x1f);
  adcon1.setChannelConfiguration(3, 0x1f);
  adcon1.setChannelConfiguration(4, 0x0b);
  adcon1.setChannelConfiguration(5, 0x0b);
  adcon1.setChannelConfiguration(6, 0x00);
  adcon1.setChannelConfiguration(7, 0x00);
  adcon1.setVrefHiConfiguration(1, 3);
  adcon1.setVrefHiConfiguration(3, 3);
  adcon1.setVrefHiConfiguration(5, 3);

  adcon0.new_name("adcon0");
  adcon1.new_name("adcon1");
  adres.new_name("adres");

  // Link the A/D converter to the Capture Compare Module
  ccp2con.setADCON(&adcon0);
}


void P16C72::create_symbols(void)
{

  if(verbose)
    cout << "c72 create symbols\n";

}


void P16C72::create(void)
{

  P16C62::create();

  P16C72::create_sfr_map();

}

Processor * P16C72::construct(void)
{

  P16C72 *p = new P16C72;

  cout << " c72 construct\n";

  p->create();
  p->create_invalid_registers ();
  p->pic_processor::create_symbols();


  p->new_name("p16c72");
  symbol_table.add_module(p,p->name().c_str());

  return p;

}


P16C72::P16C72(void)
{
  if(verbose)
    cout << "c72 constructor, type = " << isa() << '\n';

}


//--------------------------------------

void P16C73::create_sfr_map(void)
{

  if(verbose)
    cout << "creating c73 registers \n";

  // Parent classes just set PIR version 1
  pir_set_2_def.set_pir1(&pir1_2_reg);
  pir_set_2_def.set_pir2(&pir2_2_reg);

  add_sfr_register(&adcon0, 0x1f, RegisterValue(0,0));
  add_sfr_register(&adcon1, 0x9f, RegisterValue(0,0));

  add_sfr_register(&adres,  0x1e, RegisterValue(0,0));

  //1adcon0.analog_port = porta;
  //2adcon0.analog_port2 = 0;
  adcon0.adres = &adres;
  adcon0.adresl = 0;
  adcon0.adcon1 = &adcon1;
  adcon0.intcon = &intcon_reg;
  // adcon0.pir_set = get_pir_set();
  adcon0.pir_set = &pir_set_2_def;
  //adcon0.channel_mask = 7;  // even though there are only 5 inputs...

  intcon = &intcon_reg;

  //1adcon1.analog_port = porta;
  adcon1.setValidCfgBits(ADCON1::PCFG0 | ADCON1::PCFG1| ADCON1::PCFG2);

  adcon1.setNumberOfChannels(5);
  adcon1.setIOPin(0, &(*m_porta)[0]);
  adcon1.setIOPin(1, &(*m_porta)[1]);
  adcon1.setIOPin(2, &(*m_porta)[2]);
  adcon1.setIOPin(3, &(*m_porta)[3]);
  adcon1.setIOPin(4, &(*m_porta)[5]);
  adcon1.setChannelConfiguration(0, 0x1f);
  adcon1.setChannelConfiguration(1, 0x1f);
  adcon1.setChannelConfiguration(2, 0x1f);
  adcon1.setChannelConfiguration(3, 0x1f);
  adcon1.setChannelConfiguration(4, 0x0b);
  adcon1.setChannelConfiguration(5, 0x0b);
  adcon1.setChannelConfiguration(6, 0x00);
  adcon1.setChannelConfiguration(7, 0x00);
  adcon1.setVrefHiConfiguration(1, 3);
  adcon1.setVrefHiConfiguration(3, 3);
  adcon1.setVrefHiConfiguration(5, 3);


  adcon0.new_name("adcon0");
  adcon1.new_name("adcon1");
  adres.new_name("adres");

  // Link the A/D converter to the Capture Compare Module
  ccp2con.setADCON(&adcon0);
}


void P16C73::create_symbols(void)
{

  if(verbose)
    cout << "c73 create symbols\n";

}


void P16C73::create(void)
{

  P16C63::create();

  P16C73::create_sfr_map();

}

Processor * P16C73::construct(void)
{

  P16C73 *p = new P16C73;

  cout << " c73 construct\n";

  p->create();
  p->create_invalid_registers ();
  p->pic_processor::create_symbols();

  p->new_name("p16c73");
  symbol_table.add_module(p,p->name().c_str());

  return p;

}


P16C73::P16C73(void)
{
  if(verbose)
    cout << "c73 constructor, type = " << isa() << '\n';


}

//------------------------------------------------------------
//
//           16C74
//

void P16C74::create_sfr_map(void)
{

  if(verbose)
    cout << "creating c74 registers \n";

  // Parent classes just set PIR version 1
  pir_set_2_def.set_pir1(&pir1_2_reg);
  pir_set_2_def.set_pir2(&pir2_2_reg);

  add_sfr_register(&adcon0, 0x1f, RegisterValue(0,0));
  add_sfr_register(&adcon1, 0x9f, RegisterValue(0,0));

  add_sfr_register(&adres,  0x1e, RegisterValue(0,0));

  //1adcon0.analog_port = porta;
  //1adcon0.analog_port2 = porte;
  adcon0.adres = &adres;
  adcon0.adresl = 0;
  adcon0.adcon1 = &adcon1;
  adcon0.intcon = &intcon_reg;
  // adcon0.pir_set = get_pir_set();
  adcon0.pir_set = &pir_set_2_def;
  //adcon0.channel_mask = 7;

  intcon = &intcon_reg;


  adcon1.setValidCfgBits(ADCON1::PCFG0 | ADCON1::PCFG1 | ADCON1::PCFG2);
  adcon1.setNumberOfChannels(8);
  adcon1.setIOPin(0, &(*m_porta)[0]);
  adcon1.setIOPin(1, &(*m_porta)[1]);
  adcon1.setIOPin(2, &(*m_porta)[2]);
  adcon1.setIOPin(3, &(*m_porta)[3]);
  adcon1.setIOPin(4, &(*m_porta)[5]);
  adcon1.setIOPin(5, &(*m_porte)[0]);
  adcon1.setIOPin(6, &(*m_porte)[1]);
  adcon1.setIOPin(7, &(*m_porte)[2]);
  adcon1.setChannelConfiguration(0, 0xff);
  adcon1.setChannelConfiguration(1, 0xff);
  adcon1.setChannelConfiguration(2, 0x1f);
  adcon1.setChannelConfiguration(3, 0x1f);
  adcon1.setChannelConfiguration(4, 0x0b);
  adcon1.setChannelConfiguration(5, 0x0b);
  adcon1.setChannelConfiguration(6, 0x00);
  adcon1.setChannelConfiguration(7, 0x00);
  adcon1.setVrefHiConfiguration(1, 3);
  adcon1.setVrefHiConfiguration(3, 3);
  adcon1.setVrefHiConfiguration(5, 3);


  adcon0.new_name("adcon0");
  adcon1.new_name("adcon1");
  adres.new_name("adres");

  // Link the A/D converter to the Capture Compare Module
  ccp2con.setADCON(&adcon0);
}


void P16C74::create_symbols(void)
{

  if(verbose)
    cout << "c74 create symbols\n";

}


void P16C74::create(void)
{

  P16C65::create();

  P16C74::create_sfr_map();

}

Processor * P16C74::construct(void)
{

  P16C74 *p = new P16C74;

  cout << " c74 construct\n";

  p->create();
  p->create_invalid_registers ();
  p->pic_processor::create_symbols();

  p->new_name("p16c74");
  symbol_table.add_module(p,p->name().c_str());

  return p;

}


P16C74::P16C74(void)
{
  if(verbose)
    cout << "c74 constructor, type = " << isa() << '\n';
}

