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
// p16f62x
//
//  This file supports:
//    PIC16F627
//    PIC16F628
//    PIC16F648
//

#include <stdio.h>
#include <iostream>
#include <string>

//#include "../config.h"

#include "stimuli.h"

#include "p16f62x.h"

#include "symbol.h"

CMCON::CMCON(void)
{
  value.put(0);
}

VRCON::VRCON(void)
{
  value.put(0);
}


void P16F62x::create_iopin_map(void)
{
  package = new Package(18);
  if(!package)
    return;

  // ---- This is probably going to be moved:
  porta = new PORTA_62x;
  portb = new PORTB_62x;

  // ---- Complete the initialization for the I/O Ports

  // Build the links between the I/O Ports and their tris registers.
  porta->tris = &trisa;
  trisa.port = porta;
  trisa.valid_iopins = 0xdf;  // Bit 5 is an input only and always reads 0

  portb->tris = &trisb;
  trisb.port = portb;

  // And give them a more meaningful name.
  trisa.new_name("trisa");

  trisb.new_name("trisb");

  // Define the valid I/O pins.
  porta->valid_iopins = 0xff;
  portb->valid_iopins = 0xff;

  // Now Create the package and place the I/O pins

  package->assign_pin(17, new IO_bi_directional(porta, 0));
  package->assign_pin(18, new IO_bi_directional(porta, 1));
  package->assign_pin(1, new IO_bi_directional(porta, 2));
  package->assign_pin(2, new IO_bi_directional(porta, 3));
  package->assign_pin(3, new IO_open_collector(porta, 4));
  package->assign_pin(4, new IO_input(porta, 5));
  package->assign_pin(15, new IO_bi_directional(porta, 6));  // Assume RC mode
  package->assign_pin(16, new IO_bi_directional(porta, 7));  //   "        "

  package->assign_pin(5, 0);  // Vss
  package->assign_pin(6, new IO_bi_directional_pu(portb, 0));
  package->assign_pin(7, new IO_bi_directional_pu(portb, 1));
  package->assign_pin(8, new IO_bi_directional_pu(portb, 2));
  package->assign_pin(9, new IO_bi_directional_pu(portb, 3));
  package->assign_pin(10, new IO_bi_directional_pu(portb, 4));
  package->assign_pin(11, new IO_bi_directional_pu(portb, 5));
  package->assign_pin(12, new IO_bi_directional_pu(portb, 6));
  package->assign_pin(13, new IO_bi_directional_pu(portb, 7));
  package->assign_pin(14, 0);  // Vdd

}

void P16F62x::create_sfr_map(void)
{
 

  add_file_registers(0xc0, 0xef, 0);   // 0xa0 - 0xbf are created in the P16X6X_processor class
  add_file_registers(0x120,0x14f,0);
  alias_file_registers(0x70,0x7f,0x80);
  alias_file_registers(0x70,0x7f,0x100);
  alias_file_registers(0x70,0x7f,0x180);


  add_sfr_register(indf,   0x180);
  add_sfr_register(indf,   0x100);

  alias_file_registers(0x01,0x04,0x100);
  alias_file_registers(0x81,0x84,0x100);

  add_sfr_register(porta,   0x05);
  add_sfr_register(&trisa,  0x85, 0xff);

  add_sfr_register(portb,   0x06);
  add_sfr_register(&trisb,  0x86, 0xff);
  add_sfr_register(portb,   0x106);
  add_sfr_register(&trisb,  0x186, 0xff);

  add_sfr_register(get_eeprom()->get_reg_eedata(),  0x9a);
  add_sfr_register(get_eeprom()->get_reg_eeadr(),   0x9b);
  add_sfr_register(get_eeprom()->get_reg_eecon1(),  0x9c, 0);
  add_sfr_register(get_eeprom()->get_reg_eecon2(),  0x9d);

  add_sfr_register(pclath, 0x18a, 0);
  add_sfr_register(pclath, 0x10a, 0);

  add_sfr_register(&intcon_reg, 0x18b, 0);
  add_sfr_register(&intcon_reg, 0x10b, 0);
  add_sfr_register(&intcon_reg, 0x08b, 0);
  add_sfr_register(&intcon_reg, 0x00b, 0);

  add_sfr_register(usart.rcsta, 0x18, 0,"rcsta");
  add_sfr_register(usart.txsta, 0x98, 2,"txsta");
  add_sfr_register(usart.spbrg, 0x99, 0,"spbrg");
  add_sfr_register(usart.txreg, 0x19, 0,"txreg");
  add_sfr_register(usart.rcreg, 0x1a, 0,"rcreg");
  usart.initialize_14(this,get_pir_set(),portb,1);

  add_sfr_register(&comparator.cmcon, 0x1f, 0,"cmcon");
  add_sfr_register(&comparator.vrcon, 0x9f, 0,"vrcon");

  sfr_map = 0;
  num_of_sfrs = 0;

  intcon = &intcon_reg;
  intcon_reg.set_pir_set(get_pir_set());

  // Link the usart and portb
  ((PORTB_62x*)portb)->usart = &usart;

  // Link the comparator and porta
  ((PORTA_62x*)porta)->comparator = &comparator;

  // Link ccp1 and portb
  ((PORTB_62x*)portb)->ccp1con = &ccp1con;
}

void P16F62x::create_symbols(void)
{
  if(verbose)
    cout << "62x create symbols\n";

  symbol_table.add_ioport(this, portb);
  symbol_table.add_ioport(this, porta);

}

void P16F62x::set_out_of_range_pm(unsigned int address, unsigned int value)
{

  if( (address>= 0x2100) && (address < 0x2100 + get_eeprom()->get_rom_size()))
    {
      get_eeprom()->change_rom(address - 0x2100, value);
    }
}

//========================================================================
void P16F62x::set_config_word(unsigned int address, unsigned int cfg_word)
{
  enum {
    CFG_FOSC0 = 1<<0,
    CFG_FOSC1 = 1<<1,
    CFG_FOSC2 = 1<<4,
    CFG_MCLRE = 1<<5
  };

  // Let the base class do most of the work:

  cout << "p16f628 setting config word 0x" << hex << cfg_word << '\n';

  pic_processor::set_config_word(address, cfg_word);

  switch(cfg_word & (CFG_FOSC0 | CFG_FOSC1 | CFG_FOSC2)) {

  case 0:  // LP oscillator: low power crystal is on RA6 and RA7
  case 1:  // XT oscillator: crystal/resonator is on RA6 and RA7
  case 2:  // HS oscillator: crystal/resonator is on RA6 and RA7
  case 7:  // ER oscillator: RA6 is CLKOUT, resistor (?) on RA7 

    porta->valid_iopins &= 0x3f;
    break;

  case 3:  // EC:  RA6 is an I/O, RA7 is a CLKIN
  case 6:  // ER oscillator: RA6 is an I/O, RA7 is a CLKIN

    porta->valid_iopins &= 0x7f;
    porta->valid_iopins |= 0x40;
    break;

  case 4:  // INTRC: Internal Oscillator, RA6 and RA7 are I/O's

    porta->valid_iopins |= 0xc0;
    break;

  case 5:  // INTRC: Internal Oscillator, RA7 is an I/O, RA6 is CLKOUT
    porta->valid_iopins &= 0xbf;
    porta->valid_iopins |= 0x80;
    break;

  }

  // If the /MCLRE bit is set then RA5 is the MCLR pin, otherwise it's 
  // a general purpose input-only pin.

  unsigned int m = (cfg_word & CFG_MCLRE) ? 0 : (1<<5);
  
  porta->valid_iopins |= m;
  porta->valid_iopins &= ~m;

  trisa.valid_iopins |= m;
  trisa.valid_iopins &= ~m;

  cout << " porta valid_iopins " << porta->valid_iopins << 
    "  tris valid io " << trisa.valid_iopins << '\n';
}

//========================================================================
void  P16F62x::create(int ram_top, unsigned int eeprom_size)
{
  EEPROM_PIR *e;

  create_iopin_map();

  _14bit_processor::create();

  e = new EEPROM_PIR;
  e->set_cpu(this);
  e->initialize(eeprom_size);
  e->set_pir_set(get_pir_set());
  e->set_intcon(&intcon_reg);

  // assign this eeprom to the processor
  set_eeprom_pir(e);

  P16X6X_processor::create_sfr_map();

  status->rp_mask = 0x60;  // rp0 and rp1 are valid.
  indf->base_address_mask1 = 0x80; // used for indirect accesses above 0x100
  indf->base_address_mask2 = 0x1ff; // used for indirect accesses above 0x100

  P16F62x::create_sfr_map();

  // Build the links between the I/O Pins and the internal peripherals
  ccp1con.iopin = portb->pins[3];
}

//========================================================================
//
// Pic 16F627 
//

Processor * P16F627::construct(void)
{

  P16F627 *p = new P16F627;

  cout << " f627 construct\n";

  p->P16F62x::create(0x2f, 128);
  p->create_invalid_registers ();
  p->pic_processor::create_symbols();
  p->new_name("p16f627");
  symbol_table.add_module(p,p->name().c_str());

  return p;

}

P16F627::P16F627(void)
{
  if(verbose)
    cout << "f627 constructor, type = " << isa() << '\n';

}

//========================================================================
//
// Pic 16F628 
//

Processor * P16F628::construct(void)
{

  P16F628 *p = new P16F628;

  cout << " f628 construct\n";

  p->P16F62x::create(0x2f, 128);
  p->create_invalid_registers ();
  p->pic_processor::create_symbols();
  p->new_name("p16f628");
  symbol_table.add_module(p,p->name().c_str());

  return p;

}

P16F628::P16F628(void)
{
  if(verbose)
    cout << "f628 constructor, type = " << isa() << '\n';

}


//========================================================================
//
// Pic 16F648 
//

pic_processor * P16F648::construct(void)
{

  P16F648 *p = new P16F648;

  cout << " f648 construct\n";

  p->P16F62x::create(0x2f, 256);
  p->create_sfr_map();
  p->create_invalid_registers ();
  p->pic_processor::create_symbols();
  p->new_name("p16f648");
  symbol_table.add_module(p,p->name().c_str());

  return p;

}

P16F648::P16F648(void)
{
  if(verbose)
    cout << "f648 constructor, type = " << isa() << '\n';

}

void P16F648::create_sfr_map(void)
{
 
  add_file_registers(0x150,0x16f,0);
}
