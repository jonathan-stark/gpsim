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
#include <iostream.h>
#include <string>

#include "symbol.h"

#include "p16x7x.h"
#include "stimuli.h"


//------------------------------------------------------
// ADRES
//

void ADRES::put(int new_value)
{

  if(new_value > 255)
    value = 255;
  else if (new_value < 0)
    value = 0;
  else
    value = new_value;

  trace.register_write(address,value);


}


//------------------------------------------------------
// ADCON0
//

void ADCON0::start_conversion(void)
{

  //cout << "starting A/D conversion\n";

  guint64 fc = cpu->cycles.value + Tad_2;

  if(ad_state != AD_IDLE)
    {
      // The A/D converter is either 'converting' or 'acquiring'
      // in either case, there is callback break that needs to be moved.

      stop_conversion();
      cpu->cycles.reassign_break(future_cycle, fc, this);
    }
  else
    cpu->cycles.set_break(fc, this);

  future_cycle = fc;
  ad_state = AD_ACQUIRING;

}

void ADCON0::stop_conversion(void)
{

  //cout << "stopping A/D conversion\n";

  ad_state = AD_IDLE;

}



void ADCON0::put(unsigned int new_value)
{

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

  if(new_value & ADON)
    {
      // The A/D converter is being turned on (or maybe left on)

      //if(((new_value ^ value) & GO) == GO)
      if((new_value & ~value) & GO)
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

  value = new_value;

  trace.register_write(address,value);

}

// ADCON0 callback is called when the cycle counter hits the break point that
// was set in ADCON0::put.

void ADCON0::callback(void)
{

  //cout<<"ADCON0 Callback: " << hex << cpu->cycles.value << '\n';

  //
  // The a/d converter is simulated with a state machine. 
  // 

  switch(ad_state)
    {
    case AD_IDLE:
      cout << "ignoring ad callback since ad_state is idle\n";
      break;

    case AD_ACQUIRING:
      // cout << "A/D acquiring ==> converting\n";
      acquired_value = analog_port->get_bit( (value>>3) & 0x3);
      reference = adcon1->get_Vref();
      future_cycle = cpu->cycles.value + 5*Tad_2;
      cpu->cycles.set_break(future_cycle, this);
      
      ad_state = AD_CONVERTING;

      break;

    case AD_CONVERTING:
      // cout << "A/D converting ==> idle\n";
      // cout << "--- acquired_value " << acquired_value << "  reference " << reference <<'\n';

      if(reference != 0)
	adres->put(acquired_value/reference);
      else
	adres->put(0xffffffff);  // As close to infinity as possible...

      // Clear the GO/!DONE flag.
      value &= (~GO);
      set_interrupt();

      ad_state = AD_IDLE;
    }

}

//------------------------------------------------------
//
void ADCON0::set_interrupt(void)
{
  value |= ADIF;
  intcon->peripheral_interrupt();

}

//------------------------------------------------------
//
void ADCON0_withccp::set_interrupt(void)
{

  pir->set_spareif();

}

//------------------------------------------------------

int ADCON1::get_Vref(void)
{

  // Determine where the voltage reference is located

  int ref_position = Vref_position[value & (PCFG0 | PCFG1 | PCFG2)];

  if(ref_position<8)
    {
      return(analog_port->get_bit(ref_position) / 255);
    }
  else
    {
      return(int(cpu->Vdd * MAX_ANALOG_DRIVE));
    }

}

void P16C71::create_sfr_map(void)
{

  if(verbose)
    cout << "creating c71 registers \n";

  //  P16C61::create_sfr_map();

  add_sfr_register(&adcon0, 0x08, 0);
  add_sfr_register(&adcon1, 0x88, 0);

  add_sfr_register(&adres,  0x89, 0);
  add_sfr_register(&adres,  0x09, 0);

  adcon0.analog_port = porta;
  adcon0.adres = &adres;
  adcon0.adcon1 = &adcon1;
  adcon0.intcon = &intcon_reg;
  intcon = &intcon_reg;

  adcon1.analog_port = porta;


  adcon0.new_name("adcon0");
  adcon1.new_name("adcon1");
  adres.new_name("adres");

  adcon1.Vref_position[0] = 8;
  adcon1.Vref_position[1] = 3;
  adcon1.Vref_position[2] = 8;
  adcon1.Vref_position[3] = 8;
  adcon1.Vref_position[4] = 8;
  adcon1.Vref_position[5] = 3;
  adcon1.Vref_position[6] = 8;
  adcon1.Vref_position[7] = 8;

  adcon1.configuration_bits[0] = 0xf;
  adcon1.configuration_bits[1] = 0xf;
  adcon1.configuration_bits[2] = 0x3;
  adcon1.configuration_bits[3] = 0;
  adcon1.configuration_bits[4] = 0xf;
  adcon1.configuration_bits[5] = 0xf;
  adcon1.configuration_bits[6] = 0x3;
  adcon1.configuration_bits[7] = 0;

  pic_processor::create_symbols();

}


void P16C71::create_symbols(void)
{

  //P16C61::create_symbols();

  symbol_table.add_ioport(portb->cpu, portb);
  symbol_table.add_ioport(porta->cpu, porta);


}


P16C71::P16C71(void)
{
  if(verbose)
    cout << "c71 constructor, type = " << isa() << '\n';

  //  create_iopin_map(&iopin_map, &num_of_iopins);
  //  create_iopins(iopin_map, num_of_iopins);

  //  _14bit_processor::create();
  create_sfr_map();

  name_str = "16c71";

}


//--------------------------------------

void P16C74::create_sfr_map(void)
{

  if(verbose)
    cout << "creating c74 registers \n";

  add_sfr_register(&adcon0, 0x1f, 0);
  add_sfr_register(&adcon1, 0x9f, 0);

  add_sfr_register(&adres,  0x1e, 0);

  adcon0.analog_port = porta;
  adcon0.adres = &adres;
  adcon0.adcon1 = &adcon1;
  adcon0.intcon = &intcon_reg;
  adcon0.pir = &pir1;

  pir1.valid_bits = 0xff;  // All 8-bits are valid interrupt sources.

  intcon = &intcon_reg;

  adcon1.analog_port = porta;


  adcon0.new_name("adcon0");
  adcon1.new_name("adcon1");
  adres.new_name("adres");

  adcon1.Vref_position[0] = 8;
  adcon1.Vref_position[1] = 3;
  adcon1.Vref_position[2] = 8;
  adcon1.Vref_position[3] = 8;
  adcon1.Vref_position[4] = 8;
  adcon1.Vref_position[5] = 3;
  adcon1.Vref_position[6] = 8;
  adcon1.Vref_position[7] = 8;

  adcon1.configuration_bits[0] = 0xff;
  adcon1.configuration_bits[1] = 0xff;
  adcon1.configuration_bits[2] = 0x1f;
  adcon1.configuration_bits[3] = 0x1f;
  adcon1.configuration_bits[4] = 0x0b;
  adcon1.configuration_bits[5] = 0x0b;
  adcon1.configuration_bits[6] = 0;
  adcon1.configuration_bits[7] = 0;

  // Link the A/D converter to the Capture Compare Module
  ccp2con.adcon0 = &adcon0;

  pic_processor::create_symbols();

}


void P16C74::create_symbols(void)
{

  if(verbose)
    cout << "c74 create symbols\n";

}


P16C74::P16C74(void)
{
  if(verbose)
    cout << "c74 constructor, type = " << isa() << '\n';

  create_sfr_map();

  name_str = "16c74";

}

