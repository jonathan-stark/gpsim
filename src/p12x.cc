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
#include <iostream.h>
#include <string>

#include "../config.h"
#include "symbol.h"

#include "p12x.h"



void _12bit_8pins::create_iopin_map(void)
{


  // Build the links between the gpio and tris registers.
  gpio.tris = &tris;
  tris.port = &gpio;

  // And give them a more meaningful name.
  gpio.new_name("gpio");
  tris.new_name("tris");

  // Define the valid I/O pins.
  gpio.valid_iopins = 0x3f;

  create_pkg(8);

  assign_pin(7, new IO_bi_directional_pu(&gpio, 0));
  assign_pin(6, new IO_bi_directional_pu(&gpio, 1));
  assign_pin(5, new IO_bi_directional(&gpio, 2));
  assign_pin(4, new IO_input(&gpio, 3));
  assign_pin(3, new IO_bi_directional(&gpio, 4));
  assign_pin(2, new IO_bi_directional(&gpio, 5));

  assign_pin(1, NULL);
  assign_pin(8, NULL);


}

//--------------------------------------------------------

void P12C508::create_sfr_map(void)
{

  add_sfr_register(&indf,  0);
  add_sfr_register(&tmr0,  1);
  add_sfr_register(&pcl,   2);
  add_sfr_register(&status,3);
  add_sfr_register(&fsr,   4);
  add_sfr_register(&osccal,5);
  add_sfr_register(&gpio,  6);

  add_sfr_register(&W, 0xffffffff);
  add_sfr_register(&option_reg, 0xffffffff);

  osccal.new_name("osccal");


}

void P12C508::create_symbols(void)
{

  symbol_table.add_ioport(gpio.cpu, &gpio);

}


void P12C508::dump_registers (void)
{


  _12bit_processor::dump_registers();

  cout << "tris = " << tris.value << '\n';
  cout << "osccal = " << osccal.value  << '\n';

}


void P12C508::tris_instruction(unsigned int tris_register)
{

  tris.value = W.value;
  trace.write_TRIS(tris.value);

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

  tmr0.start();

  fsr.register_page_bits = 0;  // the 508 has only one register page (the rp bits aren't used)

  trace.program_counter (pc.value);

}


pic_processor * P12C508::construct(void)
{

  P12C508 *p = new P12C508;

  cout << " 12c508 construct\n";

  p->create();
  p->pic_processor::create_symbols();

  p->name_str = "12c508";

  return p;

}


P12C508::P12C508(void)
{
  if(verbose)
    cout << "12c508 constructor, type = " << isa() << '\n';
}


//--------------------------------------------------------

void P12C509::create_sfr_map(void)
{

}

pic_processor * P12C509::construct(void)
{

  P12C509 *p = new P12C509;

  cout << " 12c508 construct\n";

  p->create();
  p->pic_processor::create_symbols();

  p->name_str = "12c509";

  return p;

}


void P12C509::create(void)
{

  cout << " 12c508 create \n";

  P12C508::create();

  alias_file_registers(0x00,0x0f,0x20);
  add_file_registers(0x30, 0x3f, 0);

  fsr.register_page_bits = RP0;  // the 509 has two register pages (i.e. RP0 in fsr is used)
  pa_bits = PA0;                 // the 509 has two code pages (i.e. PAO in status is used)


}

P12C509::P12C509(void)
{
  if(verbose)
    cout << "12c509 constructor, type = " << isa() << '\n';
}


//--------------------------------------------------------
//
// GPIO Port

unsigned int GPIO::get(void)
{

  return(value);

}

void GPIO::setbit(unsigned int bit_number, bool new_value)
{

  //  cout << "GPIO::setbit() bit " << bit_number << " to " << new_value << '\n';

  IOPORT::setbit( bit_number,  new_value);

}
