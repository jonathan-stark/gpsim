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
#include <iostream>
#include <string>

#include "../config.h"
#include "symbol.h"

#include "stimuli.h"

#include "p16f62x.h"

CMCON::CMCON(void)
{
  value = 0;
}

VRCON::VRCON(void)
{
  value = 0;
}


void P16F62x::create_iopin_map(void)
{

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

  create_pkg(18);

  assign_pin(17, new IO_bi_directional(porta, 0));
  assign_pin(18, new IO_bi_directional(porta, 1));
  assign_pin(1, new IO_bi_directional(porta, 2));
  assign_pin(2, new IO_bi_directional(porta, 3));
  assign_pin(3, new IO_open_collector(porta, 4));
  assign_pin(4, new IO_input(porta, 5));
  assign_pin(15, new IO_bi_directional(porta, 6));  // Assume RC mode
  assign_pin(16, new IO_bi_directional(porta, 7));  //   "        "

  assign_pin(5, NULL);  // Vss
  assign_pin(6, new IO_bi_directional_pu(portb, 0));
  assign_pin(7, new IO_bi_directional_pu(portb, 1));
  assign_pin(8, new IO_bi_directional_pu(portb, 2));
  assign_pin(9, new IO_bi_directional_pu(portb, 3));
  assign_pin(10, new IO_bi_directional_pu(portb, 4));
  assign_pin(11, new IO_bi_directional_pu(portb, 5));
  assign_pin(12, new IO_bi_directional_pu(portb, 6));
  assign_pin(13, new IO_bi_directional_pu(portb, 7));
  assign_pin(14, NULL);  // Vdd

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

  add_sfr_register(&(eeprom->eedata),  0x9a);
  add_sfr_register(&(eeprom->eeadr),   0x9b);
  add_sfr_register(&(eeprom->eecon1),  0x9c, 0);
  add_sfr_register(&(eeprom->eecon2),  0x9d);

  add_sfr_register(pclath, 0x18a, 0);
  add_sfr_register(pclath, 0x10a, 0);

  add_sfr_register(&intcon_reg, 0x18b, 0);
  add_sfr_register(&intcon_reg, 0x10b, 0);
  add_sfr_register(&intcon_reg, 0x00b, 0);

  add_sfr_register(usart.rcsta, 0x18, 0,"rcsta");
  add_sfr_register(usart.txsta, 0x98, 2,"txsta");
  add_sfr_register(usart.spbrg, 0x99, 0,"spbrg");
  add_sfr_register(usart.txreg, 0x19, 0,"txreg");
  add_sfr_register(usart.rcreg, 0x1a, 0,"rcreg");
  usart.initialize_14(this,&pir1,portb,1);

  add_sfr_register(&comparator.cmcon, 0x1f, 0,"cmcon");
  add_sfr_register(&comparator.vrcon, 0x9f, 0,"vrcon");

  sfr_map = NULL;
  num_of_sfrs = 0;

  intcon = &intcon_reg;

  // Link the usart and portb
  ((PORTB_62x*)portb)->usart = &usart;

  // Link the comparator and porta
  ((PORTA_62x*)porta)->comparator = &comparator;

  // Link ccp1 and portb
  ((PORTB_62x*)portb)->ccp1con = &ccp1con;

  pir1.valid_bits = pir1.TMR1IF | pir1.TMR2IF | pir1.CCP1IF | pir1.TXIF 
    | pir1.RCIF | pir1.CMIF | pir1.EEIF;
}

void P16F62x::create_symbols(void)
{
  if(verbose)
    cout << "8x create symbols\n";

  symbol_table.add_ioport(portb->cpu, portb);
  symbol_table.add_ioport(porta->cpu, porta);

}

void P16F62x::set_out_of_range_pm(int address, int value)
{

  if( (address>= 0x2100) && (address < 0x2100 + eeprom->rom_size))
    {
      eeprom->rom[address - 0x2100]->value = value;
    }
}
#if 0
unsigned int P16F62x::eeprom_get_value(unsigned int address)
{

  if(address<eeprom_get_size())
    return eeprom.rom[address]->get_value();
  return 0;

}
file_register *P16F62x::eeprom_get_register(unsigned int address)
{

  if(address<eeprom_get_size())
    return eeprom.rom[address];
  return NULL;

}
void  P16F62x::eeprom_put_value(unsigned int value,
				unsigned int address)
{
  if(address<eeprom_get_size())
    eeprom.rom[address]->put_value(value);

}
#endif

void  P16F62x::create(int ram_top)
{
  create_iopin_map();

  _14bit_processor::create();

  eeprom = new EEPROM_62x;
  eeprom->cpu = this;
  eeprom->initialize(128);
  ((EEPROM_62x *)eeprom)->pir1 = &pir1;

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

pic_processor * P16F627::construct(void)
{

  P16F627 *p = new P16F627;

  cout << " f628 construct\n";

  p->P16F62x::create(0x2f);
  p->create_invalid_registers ();
  p->pic_processor::create_symbols();
  p->name_str = "p16f628";
  symbol_table.add_module(p,p->name_str);

  return p;

}

P16F627::P16F627(void)
{
  if(verbose)
    cout << "f628 constructor, type = " << isa() << '\n';

}

//========================================================================
//
// Pic 16F628 
//

pic_processor * P16F628::construct(void)
{

  P16F628 *p = new P16F628;

  cout << " f628 construct\n";

  p->P16F62x::create(0x2f);
  p->create_invalid_registers ();
  p->pic_processor::create_symbols();
  p->name_str = "p16f628";
  symbol_table.add_module(p,p->name_str);

  return p;

}

P16F628::P16F628(void)
{
  if(verbose)
    cout << "f628 constructor, type = " << isa() << '\n';

}

