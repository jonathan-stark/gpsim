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
// p12x
//
//  This file supports:
//    PIC12C508
//    PIC12C509
//

#include <stdio.h>
#include <iostream>
#include <string>

#include "packages.h"
#include "stimuli.h"
#include "i2c-ee.h"

#include "p12x.h"

#include "symbol.h"



void P12C508::create_iopin_map(void)
{

  package = new Package(8);
  if(!package)
    return;

  // Build the links between the gpio and tris registers.
  gpio.tris = &tris;
  tris.port = &gpio;

  // And give them a more meaningful name.
  gpio.new_name("gpio");
  tris.new_name("tris");

  // Define the valid I/O pins.
  gpio.valid_iopins = 0x3f;

  package->assign_pin(7, new IO_bi_directional_pu(&gpio, 0));
  package->assign_pin(6, new IO_bi_directional_pu(&gpio, 1));
  package->assign_pin(5, new IO_bi_directional(&gpio, 2));
  package->assign_pin(4, new IO_input(&gpio, 3));
  package->assign_pin(3, new IO_bi_directional(&gpio, 4));
  package->assign_pin(2, new IO_bi_directional(&gpio, 5));

  package->assign_pin(1, 0);
  package->assign_pin(8, 0);


}

//--------------------------------------------------------
void P12C508::reset(RESET_TYPE r)
{

  tris.value = tris.por_value;
  option_reg.value = option_reg.por_value;

  pic_processor::reset(r);
  
}

void  P12C508::option_new_bits_6_7(unsigned int)
{
  if(verbose)
    cout << "p12c508 option_new_bits_6_7\n";
}

void P12C508::create_sfr_map(void)
{

  add_sfr_register(indf,   0);
  add_sfr_register(&tmr0,  1);
  add_sfr_register(pcl,   2);
  add_sfr_register(status,3);
  add_sfr_register(fsr,    4);
  add_sfr_register(&osccal,5);
  add_sfr_register(&gpio,  6);

  add_sfr_register(W, 0xffffffff);
  add_sfr_register(&option_reg, 0xffffffff, RegisterValue(0xff,0));
  add_sfr_register(&tris, 0xffffffff, RegisterValue(0x3f,0));

  osccal.new_name("osccal");


}

void P12C508::create_symbols(void)
{

  symbol_table.add_ioport(&gpio);

}


void P12C508::dump_registers (void)
{


  _12bit_processor::dump_registers();

  cout << "tris = 0x" << hex << tris.value.get() << '\n';
  cout << "osccal = 0x" << osccal.value.get()  << '\n';

}


void P12C508::tris_instruction(unsigned int tris_register)
{
//  cout << " Tris instruction\n";

  //tris.value = W.value;
  tris.put(W->value.get());
  trace.write_TRIS(tris.value.get());

}
  
void P12C508::create(void)
{

  cout << " 12c508 create \n";

  //P12C508::create();
  create_iopin_map();

  _12bit_processor::create();

  add_file_registers(0x07, 0x1f, 0);
  P12C508::create_sfr_map();
  create_invalid_registers ();

  tmr0.start(0);

  pc->reset();
  //trace.program_counter (pc->value);

}


Processor * P12C508::construct(void)
{

  P12C508 *p = new P12C508;

  cout << " 12c508 construct\n";

  p->pc->set_reset_address(0x1ff);

  p->create();
  p->pic_processor::create_symbols();

  p->name_str = "p12c508";
  symbol_table.add_module(p,p->name_str.c_str());

  return p;

}


P12C508::P12C508(void)
{
  if(verbose)
    cout << "12c508 constructor, type = " << isa() << '\n';

  if(config_modes)
    config_modes->valid_bits = config_modes->CM_FOSC0 | config_modes->CM_FOSC1 | 
      config_modes->CM_FOSC1x | config_modes->CM_WDTE | config_modes->CM_MCLRE;
}



//--------------------------------------------------------

void P12C509::create_sfr_map(void)
{

}

Processor * P12C509::construct(void)
{

  P12C509 *p = new P12C509;

  cout << " 12c508 construct\n";

  p->pc->set_reset_address(0x3ff);

  p->create();
  p->pic_processor::create_symbols();

  p->name_str = "p12c509";
  symbol_table.add_module(p,p->name_str.c_str());

  return p;

}


void P12C509::create(void)
{

  if ( verbose )
    cout << " 12c509 create \n";

  P12C508::create();

  alias_file_registers(0x00,0x0f,0x20);
  add_file_registers(0x30, 0x3f, 0);

  pa_bits = PA0;                 // the 509 has two code pages (i.e. PAO in status is used)
  indf->base_address_mask2 = 0x3F;  // RP - need this or INDF won't work right

}

P12C509::P12C509(void)
{
  if(verbose)
    cout << "12c509 constructor, type = " << isa() << '\n';
}


//--------------------------------------------------------

// construct function is identical to 12C508 version ??
Processor * P12CE518::construct(void)
{

  P12CE518 *p = new P12CE518;

  if(verbose)
    cout << " 12ce518 construct\n";

  p->pc->set_reset_address(0x1ff);

  p->create();

  if(verbose)
    cout << " ... create symbols\n";
  p->pic_processor::create_symbols();

  p->name_str = "p12ce518";
  symbol_table.add_module(p,p->name_str.c_str());

  return p;

}


void P12CE518::create_iopin_map(void)
{
  P12C508::create_iopin_map();

  // Define the valid I/O pins.
  gpio.valid_iopins = 0xff;

}

void P12CE518::create(void)
{
  Stimulus_Node *scl, *sda;
  I2C_EE * e;

  if(verbose)
    cout << " 12ce518 create \n";

  P12C508::create();

  if(verbose)
    cout << "  adding serial EE\n";

  e = new I2C_EE();
  e->set_cpu(this);
  e->initialize ( 0x10 );

  // GPIO bits 6 and 7 are not bonded to physical pins, but are tied
  // to the internal I2C device.
  gpio.valid_iopins |= 0xc0;
  gpio.num_iopins = 8;

  RegisterValue por_value(0xc0,0x00);
  gpio.value       = por_value;
  gpio.por_value   = por_value;
  gpio.wdtr_value  = por_value;
  gpio.put(0xc0);

  if(verbose)
    cout << " ... create additional (internal) I/O\n";
  scl = new Stimulus_Node ( "EE_SCL" );

  scl->attach_stimulus ( new IO_bi_directional(&gpio,7) );

  sda = new Stimulus_Node ( "EE_SDA" );

  IO_open_collector *io_sda = new IO_open_collector(&gpio,6);

  // enable the pullup resistor.
  if(io_sda)
    io_sda->update_pullup(true);

  sda->attach_stimulus (io_sda);

  e->attach ( scl, sda );

  set_eeprom(e);

  // Kludge to force top two bits to be outputs
  tris.value.put(0xff);
  tris.put(0x3f);
  
}

P12CE518::P12CE518(void)
{
  if(verbose)
    cout << "12CE518 constructor, type = " << isa() << '\n';

  if(config_modes)
    config_modes->valid_bits = config_modes->CM_FOSC0 | config_modes->CM_FOSC1 | 
      config_modes->CM_FOSC1x | config_modes->CM_WDTE | config_modes->CM_MCLRE;
}

void P12CE518::tris_instruction(unsigned int tris_register)
{
    unsigned int w_val;

//  cout << " Tris instruction\n";

  //tris.value = W.value;
  w_val = W->value.get();
  tris.put ( w_val & 0x3F );     // top two bits always output
  trace.write_TRIS(w_val);

}

void P12CE518::set_eeprom(I2C_EE *e)
{
  eeprom = e; 

  //  eeprom->cpu = this;

  ema.set_cpu(this);
  ema.set_Registers(e->rom, e->rom_size);

}
  

//--------------------------------------------------------

void P12CE519::create_sfr_map(void)
{

}

Processor * P12CE519::construct(void)
{

  P12CE519 *p = new P12CE519;

  cout << " 12ce519 construct\n";

  p->pc->set_reset_address(0x3ff);

  p->create();
  p->pic_processor::create_symbols();

  p->name_str = "p12ce519";
  symbol_table.add_module(p,p->name_str.c_str());

  return p;

}


void P12CE519::create(void)
{
  if ( verbose )
    cout << " 12ce519 create \n";

  P12CE518::create();

  alias_file_registers(0x00,0x0f,0x20);
  add_file_registers(0x30, 0x3f, 0);

  pa_bits = PA0;                 // the 519 has two code pages (i.e. PAO in status is used)
  indf->base_address_mask2 = 0x3F;  // RP - need this or INDF won't work right

}


P12CE519::P12CE519(void)
{
  if(verbose)
    cout << "12ce519 constructor, type = " << isa() << '\n';
}



//--------------------------------------------------------
//
// GPIO Port


void GPIO::setbit(unsigned int bit_number, bool new_value)
{
  unsigned int old_value = value.get();

  if(verbose&2)
    cout << "GPIO::setbit() bit " << bit_number << " to " << new_value << '\n';

  IOPORT::setbit( bit_number,  new_value);

  int diff = old_value ^ value.get(); // The difference between old and new

  // If gpio bit 0,1 or 3 changed states AND
  // ~GPWU is low (wake up on change is enabled) AND
  // the processor is sleeping.
  //    Then wake 
  if( diff & 0x0b)
    {
      if( ((cpu12->option_reg.value.get() & 0x80) == 0) && bp.have_sleep()) {

	if(verbose)
	  cout << "IO bit changed while the processor was sleeping,\n\
so the processor is waking up\n";

	cpu12->status->put(cpu12->status->get() | 0x80);  // Set GPWUF flag
	bp.clear_sleep();                 // Wake up the processor.

	cpu12->pc->reset();


      }
    }


}


