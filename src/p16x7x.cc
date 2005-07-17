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

  int converted;


  if(adresl) {   // non-null for 16f877

    if(reference != 0)
      converted = (int)((1023*acquired_value)/reference);
    else
      converted = 0xffffffff;  // As close to infinity as possible...

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

    if(reference != 0)
      converted = (int)(255*acquired_value/reference);
    else
      converted = 0xffffffff;  // As close to infinity as possible...

    #ifndef DEBUG_AD
    if(verbose)
    #endif
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
      #ifdef DEBUG_AD
      printf("Valor: 0x%02X >>3 0x%02X máscara: 0x%02X => 0x%02X\n",value.get(),value.get()>>3,channel_mask,channel);
      #endif

      if(channel <= 4) {
	// analog channels 0-4 map to porta
	acquired_value = analog_port->get_bit_voltage( channel);
      } else {
	// analog channels 5-7 map to porte
	if(analog_port2)
	  acquired_value = analog_port2->get_bit_voltage( channel - 5);
	else
	  acquired_value = 0.0;
      }

      reference = adcon1->get_Vref();
      #ifdef DEBUG_AD
      cout << "A/D acquiring ==> converting channel " << channel << " voltage "
      << acquired_value << ", Vref = " << reference << '\n';
      #endif

      future_cycle = get_cycles().value + 5*Tad_2;
      get_cycles().set_break(future_cycle, this);
      
      ad_state = AD_CONVERTING;

      break;

    case AD_CONVERTING:
      #ifdef DEBUG_AD
      cout << "A/D converting ==> idle\n";
      cout << "--- acquired_value " << acquired_value << "  reference " << reference <<'\n';
      #endif

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

double ADCON1::get_Vref(void)
{


  double vrefhi,vreflo;

  if ( Vrefhi_position[value.get() & valid_bits] ==  3)
    vrefhi = analog_port->get_bit_voltage(3);
  else
    vrefhi = get_cpu()->get_Vdd();


  if ( Vreflo_position[value.get() & valid_bits] ==  2)
    vreflo = analog_port->get_bit_voltage(2);
  else
    vreflo = 0;

  //cout << "AD reading ref hi 0x" << hex <<  vrefhi << " low 0x" << vreflo << '\n';

  return (vrefhi - vreflo);

}

void P16C71::create_sfr_map(void)
{

  if(verbose)
    cout << "creating c71 registers \n";

  add_sfr_register(&adcon0, 0x08, RegisterValue(0,0));
  add_sfr_register(&adcon1, 0x88, RegisterValue(0,0));

  add_sfr_register(&adres,  0x89, RegisterValue(0,0));
  add_sfr_register(&adres,  0x09, RegisterValue(0,0));

  //1adcon0.analog_port = porta;
  adcon0.adres = &adres;
  adcon0.adresl = 0;
  adcon0.adcon1 = &adcon1;
  adcon0.intcon = &intcon_reg;
  adcon0.channel_mask = 3;
  intcon = &intcon_reg;

  //1adcon1.analog_port = porta;
  adcon1.valid_bits = ADCON1::PCFG1 | ADCON1::PCFG2;

  adcon0.new_name("adcon0");
  adcon1.new_name("adcon1");
  adres.new_name("adres");

  adcon1.Vrefhi_position[0] = 8;
  adcon1.Vrefhi_position[1] = 3;
  adcon1.Vrefhi_position[2] = 8;
  adcon1.Vrefhi_position[3] = 8;

  adcon1.Vreflo_position[0] = 8;
  adcon1.Vreflo_position[1] = 8;
  adcon1.Vreflo_position[2] = 8;
  adcon1.Vreflo_position[3] = 8;

  adcon1.configuration_bits[0] = 0xf;
  adcon1.configuration_bits[1] = 0xf;
  adcon1.configuration_bits[2] = 0x3;
  adcon1.configuration_bits[3] = 0;

  // c71x only has 4 analog configurations. The gpsim analog module
  // supports 16 different configurations, so modulo duplicate the first
  // 4 positions to the remaining 12.

  for(int i=4; i<16; i++) {
    adcon1.Vrefhi_position[i] = adcon1.Vrefhi_position[i&3];
    adcon1.Vreflo_position[i] = adcon1.Vreflo_position[i&3];
    adcon1.configuration_bits[i] = adcon1.configuration_bits[i&3];
  }

}


void P16C71::create_symbols(void)
{
  //symbol_table.add_register(m_portb);
  //symbol_table.add_ioport(porta);
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
  adcon0.channel_mask = 3;
  intcon = &intcon_reg;

  //1adcon1.analog_port = porta;
  adcon1.valid_bits = ADCON1::PCFG0 | ADCON1::PCFG1 | ADCON1::PCFG2;

  adcon0.new_name("adcon0");
  adcon1.new_name("adcon1");
  adres.new_name("adres");

  adcon1.Vrefhi_position[0] = 8;
  adcon1.Vrefhi_position[1] = 3;
  adcon1.Vrefhi_position[2] = 8;
  adcon1.Vrefhi_position[3] = 3;
  adcon1.Vrefhi_position[4] = 8;
  adcon1.Vrefhi_position[5] = 3;
  adcon1.Vrefhi_position[6] = 8;
  adcon1.Vrefhi_position[7] = 8;

  int i;
  for (i=0; i<8; i++)
      adcon1.Vreflo_position[i] = 8;

  adcon1.configuration_bits[0] = 0xf;
  adcon1.configuration_bits[1] = 0xf;
  adcon1.configuration_bits[2] = 0xf;
  adcon1.configuration_bits[3] = 0xf;
  adcon1.configuration_bits[4] = 0xB;
  adcon1.configuration_bits[5] = 0xB;
  adcon1.configuration_bits[6] = 0;
  adcon1.configuration_bits[7] = 0;

  // c712 only has 8 analog configurations. The gpsim analog module
  // supports 16 different configurations, so modulo duplicate the first
  // 8 positions to the remaining 8.

  for(int i=8; i<16; i++) {
    adcon1.Vrefhi_position[i] = adcon1.Vrefhi_position[i&7];
    adcon1.Vreflo_position[i] = adcon1.Vreflo_position[i&7];
    adcon1.configuration_bits[i] = adcon1.configuration_bits[i&7];
  }

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

  init_ssp = false;
  
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

  init_ssp = false;
  
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

  //1adcon0.analog_port = porta;
  adcon0.analog_port2 = 0;
  adcon0.adres = &adres;
  adcon0.adresl = 0;
  adcon0.adcon1 = &adcon1;
  adcon0.intcon = &intcon_reg;
  // adcon0.pir_set = get_pir_set();
  adcon0.pir_set = &pir_set_2_def;
  adcon0.channel_mask = 7;  // even though there are only 5 inputs...

  intcon = &intcon_reg;

  //1adcon1.analog_port = porta;
  adcon1.valid_bits = ADCON1::PCFG0 | ADCON1::PCFG1 | ADCON1::PCFG2;


  adcon0.new_name("adcon0");
  adcon1.new_name("adcon1");
  adres.new_name("adres");

  adcon1.Vrefhi_position[0] = 8;
  adcon1.Vrefhi_position[1] = 3;
  adcon1.Vrefhi_position[2] = 8;
  adcon1.Vrefhi_position[3] = 8;
  adcon1.Vrefhi_position[4] = 8;
  adcon1.Vrefhi_position[5] = 3;
  adcon1.Vrefhi_position[6] = 8;
  adcon1.Vrefhi_position[7] = 8;

  adcon1.Vreflo_position[0] = 8;
  adcon1.Vreflo_position[1] = 8;
  adcon1.Vreflo_position[2] = 8;
  adcon1.Vreflo_position[3] = 8;
  adcon1.Vreflo_position[4] = 8;
  adcon1.Vreflo_position[5] = 8;
  adcon1.Vreflo_position[6] = 8;
  adcon1.Vreflo_position[7] = 8;

  adcon1.configuration_bits[0] = 0xff;
  adcon1.configuration_bits[1] = 0xff;
  adcon1.configuration_bits[2] = 0x1f;
  adcon1.configuration_bits[3] = 0x1f;
  adcon1.configuration_bits[4] = 0x0b;
  adcon1.configuration_bits[5] = 0x0b;
  adcon1.configuration_bits[6] = 0;
  adcon1.configuration_bits[7] = 0;

  // c72 only has 8 analog configurations. The gpsim analog module
  // supports 16 different configurations, so duplicate the first
  // 8 positions to the remaining 8.

  for(int i=8; i<16; i++) {
    adcon1.Vrefhi_position[i] = adcon1.Vrefhi_position[i&7];
    adcon1.Vreflo_position[i] = adcon1.Vreflo_position[i&7];
    adcon1.configuration_bits[i] = adcon1.configuration_bits[i&7];
  }

  // Link the A/D converter to the Capture Compare Module
  ccp2con.adcon0 = &adcon0;


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

  
  //1p->ssp.initialize_14(p,p->get_pir_set(),p->portc,3,4,5,p->porta,5,SSP_TYPE_BSSP);

  p->new_name("p16c72");
  symbol_table.add_module(p,p->name().c_str());

  return p;

}


P16C72::P16C72(void)
{
  if(verbose)
    cout << "c72 constructor, type = " << isa() << '\n';

  init_ssp = true;
  
  //  create_sfr_map();
  //  name_str = "p16c72";
  //  symbol_table.add_module(p,p->name_str);

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
  adcon0.analog_port2 = 0;
  adcon0.adres = &adres;
  adcon0.adresl = 0;
  adcon0.adcon1 = &adcon1;
  adcon0.intcon = &intcon_reg;
  // adcon0.pir_set = get_pir_set();
  adcon0.pir_set = &pir_set_2_def;
  adcon0.channel_mask = 7;  // even though there are only 5 inputs...

  intcon = &intcon_reg;

  //1adcon1.analog_port = porta;
  adcon1.valid_bits = ADCON1::PCFG0 | ADCON1::PCFG1 | ADCON1::PCFG2;


  adcon0.new_name("adcon0");
  adcon1.new_name("adcon1");
  adres.new_name("adres");

  adcon1.Vrefhi_position[0] = 8;
  adcon1.Vrefhi_position[1] = 3;
  adcon1.Vrefhi_position[2] = 8;
  adcon1.Vrefhi_position[3] = 8;
  adcon1.Vrefhi_position[4] = 8;
  adcon1.Vrefhi_position[5] = 3;
  adcon1.Vrefhi_position[6] = 8;
  adcon1.Vrefhi_position[7] = 8;

  adcon1.Vreflo_position[0] = 8;
  adcon1.Vreflo_position[1] = 8;
  adcon1.Vreflo_position[2] = 8;
  adcon1.Vreflo_position[3] = 8;
  adcon1.Vreflo_position[4] = 8;
  adcon1.Vreflo_position[5] = 8;
  adcon1.Vreflo_position[6] = 8;
  adcon1.Vreflo_position[7] = 8;

  adcon1.configuration_bits[0] = 0xff;
  adcon1.configuration_bits[1] = 0xff;
  adcon1.configuration_bits[2] = 0x1f;
  adcon1.configuration_bits[3] = 0x1f;
  adcon1.configuration_bits[4] = 0x0b;
  adcon1.configuration_bits[5] = 0x0b;
  adcon1.configuration_bits[6] = 0;
  adcon1.configuration_bits[7] = 0;

  // c73 only has 8 analog configurations. The gpsim analog module
  // supports 16 different configurations, so duplicate the first
  // 8 positions to the remaining 8.

  for(int i=8; i<16; i++) {
    adcon1.Vrefhi_position[i] = adcon1.Vrefhi_position[i&7];
    adcon1.Vreflo_position[i] = adcon1.Vreflo_position[i&7];
    adcon1.configuration_bits[i] = adcon1.configuration_bits[i&7];
  }

  // Link the A/D converter to the Capture Compare Module
  ccp2con.adcon0 = &adcon0;


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

  //1p->ssp.initialize_14(p,p->get_pir_set(),p->portc,3,4,5,p->porta,5,SSP_TYPE_BSSP);

  p->new_name("p16c73");
  symbol_table.add_module(p,p->name().c_str());

  return p;

}


P16C73::P16C73(void)
{
  if(verbose)
    cout << "c73 constructor, type = " << isa() << '\n';

  init_ssp = true;

  //  create_sfr_map();
  //  name_str = "p16c73";
  //  symbol_table.add_module(p,p->name_str);

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
  adcon0.analog_port2 = porte;
  adcon0.adres = &adres;
  adcon0.adresl = 0;
  adcon0.adcon1 = &adcon1;
  adcon0.intcon = &intcon_reg;
  // adcon0.pir_set = get_pir_set();
  adcon0.pir_set = &pir_set_2_def;
  adcon0.channel_mask = 7;

  intcon = &intcon_reg;

  //1adcon1.analog_port = porta;
  adcon1.valid_bits = ADCON1::PCFG0 | ADCON1::PCFG1 | ADCON1::PCFG2;


  adcon0.new_name("adcon0");
  adcon1.new_name("adcon1");
  adres.new_name("adres");

  adcon1.Vrefhi_position[0] = 8;
  adcon1.Vrefhi_position[1] = 3;
  adcon1.Vrefhi_position[2] = 8;
  adcon1.Vrefhi_position[3] = 8;
  adcon1.Vrefhi_position[4] = 8;
  adcon1.Vrefhi_position[5] = 3;
  adcon1.Vrefhi_position[6] = 8;
  adcon1.Vrefhi_position[7] = 8;

  adcon1.Vreflo_position[0] = 8;
  adcon1.Vreflo_position[1] = 8;
  adcon1.Vreflo_position[2] = 8;
  adcon1.Vreflo_position[3] = 8;
  adcon1.Vreflo_position[4] = 8;
  adcon1.Vreflo_position[5] = 8;
  adcon1.Vreflo_position[6] = 8;
  adcon1.Vreflo_position[7] = 8;

  adcon1.configuration_bits[0] = 0xff;
  adcon1.configuration_bits[1] = 0xff;
  adcon1.configuration_bits[2] = 0x1f;
  adcon1.configuration_bits[3] = 0x1f;
  adcon1.configuration_bits[4] = 0x0b;
  adcon1.configuration_bits[5] = 0x0b;
  adcon1.configuration_bits[6] = 0;
  adcon1.configuration_bits[7] = 0;

  // c74 only has 8 analog configurations. The gpsim analog module
  // supports 16 different configurations, so duplicate the first
  // 8 positions to the remaining 8.

  for(int i=8; i<16; i++) {
    adcon1.Vrefhi_position[i] = adcon1.Vrefhi_position[i&7];
    adcon1.Vreflo_position[i] = adcon1.Vreflo_position[i&7];
    adcon1.configuration_bits[i] = adcon1.configuration_bits[i&7];
  }

  // Link the A/D converter to the Capture Compare Module
  ccp2con.adcon0 = &adcon0;


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

  //1p->ssp.initialize_14(p,p->get_pir_set(),p->portc,3,4,5,p->porta,5,SSP_TYPE_BSSP);

  p->new_name("p16c74");
  symbol_table.add_module(p,p->name().c_str());

  return p;

}


P16C74::P16C74(void)
{
  if(verbose)
    cout << "c74 constructor, type = " << isa() << '\n';

  init_ssp = true;

  //  create_sfr_map();
  //  name_str = "p16c74";
  //  symbol_table.add_module(p,p->name_str);

}

