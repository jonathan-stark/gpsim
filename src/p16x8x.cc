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
// p16x8x
//
//  This file supports:
//    PIC16C84
//    PIC16CR84
//    PIC16F84
//    PIC16F83
//    PIC16CR83
//

#include <stdio.h>
#include <iostream.h>
#include <string>

#include "../config.h"
#include "symbol.h"

#include "stimuli.h"

#include "p16x8x.h"



void _14bit_18pins::create_iopin_map(void)
{

  // ---- This is probably going to be moved:
  porta = new PORTA;
  portb = new PORTB;

  // ---- Complete the initialization for the I/O Ports

  // Build the links between the I/O Ports and their tris registers.
  porta->tris = &trisa;
  trisa.port = porta;

  portb->tris = &trisb;
  trisb.port = portb;

  // And give them a more meaningful name.
  trisa.new_name("trisa");

  trisb.new_name("trisb");

  // Define the valid I/O pins.
  porta->valid_iopins = 0x1f;
  portb->valid_iopins = 0xff;

  // Now Create the package and place the I/O pins

  create_pkg(18);

  assign_pin(17, new IO_bi_directional(porta, 0));
  assign_pin(18, new IO_bi_directional(porta, 1));
  assign_pin(1, new IO_bi_directional(porta, 2));
  assign_pin(2, new IO_bi_directional(porta, 3));
  assign_pin(3, new IO_open_collector(porta, 4));
  assign_pin(4, NULL);
  assign_pin(5, NULL);
  assign_pin(6, new IO_bi_directional_pu(portb, 0));
  assign_pin(7, new IO_bi_directional_pu(portb, 1));
  assign_pin(8, new IO_bi_directional_pu(portb, 2));
  assign_pin(9, new IO_bi_directional_pu(portb, 3));
  assign_pin(10, new IO_bi_directional_pu(portb, 4));
  assign_pin(11, new IO_bi_directional_pu(portb, 5));
  assign_pin(12, new IO_bi_directional_pu(portb, 6));
  assign_pin(13, new IO_bi_directional_pu(portb, 7));
  assign_pin(14, NULL);
  assign_pin(15, NULL);
  assign_pin(16, NULL);



}


void P16C8x::create_sfr_map(void)
{
 
  add_sfr_register(indf,   0x80);
  add_sfr_register(indf,   0x00);

  add_sfr_register(&tmr0,  0x01);
  add_sfr_register(&option_reg,  0x81, 0xff);

  add_sfr_register(&pcl,    0x02, 0);
  add_sfr_register(&status, 0x03, 0x18);
  add_sfr_register(fsr,     0x04);
  alias_file_registers(0x02,0x04,0x80);

  add_sfr_register(porta,   0x05);
  add_sfr_register(&trisa,  0x85, 0x3f);

  add_sfr_register(portb,   0x06);
  add_sfr_register(&trisb,  0x86, 0xff);

  add_sfr_register(&(eeprom->eedata),  0x08);
  add_sfr_register(&(eeprom->eecon1),  0x88, 0);

  add_sfr_register(&(eeprom->eeadr),   0x09);
  add_sfr_register(&(eeprom->eecon2),  0x89);

  add_sfr_register(&pclath, 0x8a, 0);
  add_sfr_register(&pclath, 0x0a, 0);

  add_sfr_register(&intcon_reg, 0x8b, 0);
  add_sfr_register(&intcon_reg, 0x0b, 0);

  sfr_map = NULL;
  num_of_sfrs = 0;

  intcon = &intcon_reg;


}

void P16C8x::create_symbols(void)
{
  if(verbose)
    cout << "8x create symbols\n";

  symbol_table.add_ioport(portb->cpu, portb);
  symbol_table.add_ioport(porta->cpu, porta);

}

void P16C8x::set_out_of_range_pm(int address, int value)
{

  if( (address>= 0x2100) && (address < 0x2100 + eeprom->rom_size))
    {
      eeprom->rom[address - 0x2100]->value = value;
    }
}

#if 0
unsigned int P16C8x::eeprom_get_value(unsigned int address)
{

  if(address<eeprom->rom_size)
    return eeprom->rom[address]->get_value();
  return 0;

}
file_register *P16C8x::eeprom_get_register(unsigned int address)
{

  if(address<eeprom_get_size())
    return eeprom.rom[address];
  return NULL;

}
void  P16C8x::eeprom_put_value(unsigned int value,
				unsigned int address)
{
  if(address<eeprom_get_size())
    eeprom.rom[address]->put_value(value);

}
#endif

void  P16C8x::create(int ram_top)
{
  create_iopin_map();

  _14bit_processor::create();

  eeprom = new EEPROM;
  eeprom->cpu = this;
  eeprom->initialize(EEPROM_SIZE);

  add_file_registers(0x0c, ram_top, 0x80);
  P16C8x::create_sfr_map();

}

//========================================================================
//
// Pic 16C84 
//

pic_processor * P16C84::construct(void)
{

  P16C84 *p = new P16C84;

  cout << " c84 construct\n";

  p->P16C8x::create(0x2f);
  p->create_invalid_registers ();
  p->pic_processor::create_symbols();
  p->name_str = "p16c84";
  symbol_table.add_module(p,p->name_str);

  return p;

}

P16C84::P16C84(void)
{
  if(verbose)
    cout << "c84 constructor, type = " << isa() << '\n';

}



//========================================================================
//
// Pic 16F84 
//


pic_processor * P16F84::construct(void)
{

  P16F84 *p = new P16F84;

  cout << " c84 construct\n";

  p->P16C8x::create(0x4f);
  p->create_invalid_registers ();
  p->pic_processor::create_symbols();
  p->name_str = "p16f84";
  symbol_table.add_module(p,p->name_str);

  return p;

}

P16F84::P16F84(void)
{
  if(verbose)
    cout << "f84 constructor, type = " << isa() << '\n';
}

//========================================================================
//
// Pic 16F83
//

P16F83::P16F83(void)
{
  if(verbose)
    cout << "f83 constructor, type = " << isa() << '\n';

  name_str = "p16f83";
}

pic_processor * P16F83::construct(void)
{

  P16F83 *p = new P16F83;

  cout << " f83 construct\n";

  p->P16C8x::create(0x2f);
  p->create_invalid_registers ();
  p->pic_processor::create_symbols();
  p->name_str = "p16f83";
  symbol_table.add_module(p,p->name_str);

  return p;

}

pic_processor * P16CR83::construct(void)
{

  cout << " cr83 construct\n";

  return NULL;

}

pic_processor * P16CR84::construct(void)
{

  cout << " cr84 construct\n";

  return NULL;

}

