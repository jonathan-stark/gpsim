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

#include "symbol.h"

#include "p12x.h"



void _12bit_8pins::create_iopin_map(IOPIN_map ** iopin_map_ptr, int * num_of_iopins_ptr)
{

  IOPIN_map im[] =
  {
    // pin, port, bit in port, type
    {7, &gpio, 0, BI_DIRECTIONAL_PU},
    {6, &gpio, 1, BI_DIRECTIONAL_PU},
    {5, &gpio, 2, BI_DIRECTIONAL},
    {4, &gpio, 3, INPUT_ONLY},
    {3, &gpio, 4, BI_DIRECTIONAL},
    {2, &gpio, 5, BI_DIRECTIONAL},

  };

  // Create a place for permanently storing the I/O pin mapping.

  IOPIN_map *iopin_map = (IOPIN_map *) new char[sizeof (im)];

  int num_of_iopins = sizeof (im) / sizeof (IOPIN_map);

  *iopin_map_ptr = iopin_map;
  *num_of_iopins_ptr = num_of_iopins;

  // Save the map.
  for(int i=0; i<num_of_iopins; i++)
    iopin_map[i] = im[i];


       // ---- Complete the initialization for the I/O Ports

  // Build the links between the gpio and tris registers.
  gpio.tris = &tris;
  tris.port = &gpio;

  // And give them a more meaningful name.
  gpio.new_name("gpio");
  tris.new_name("tris");

  // Define the valid I/O pins.
  gpio.valid_iopins = 0x3f;


}

//--------------------------------------------------------

void P12C508::create_sfr_map(void)
{

  add_file_registers(0x07, 0x1f, 0);

  sfr_map = NULL;
  num_of_sfrs = 0;
  

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

  pic_processor::create_symbols();

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
  

// const GPR_map P12C508::gpr_map[];

P12C508::P12C508(void)
{
  if(verbose)
    cout << "12c508 constructor, type = " << isa() << '\n';
  //  create_sfr_map();
  create_iopin_map(&iopin_map, &num_of_iopins);
  create_iopins(iopin_map, num_of_iopins);

  sfr_map = NULL;
  num_of_sfrs = 0;
 
  _12bit_processor::create();
  create_sfr_map();
  tmr0.start();

  fsr.register_page_bits = 0;  // the 508 has only one register page (the rp bits aren't used)

  // Build the links between the I/O Pins and the internal peripherals
  //ccp1con.iopin = portc.pins[2];


  name_str = "12c508";

  trace.program_counter (pc.value);

}


//--------------------------------------------------------

void P12C509::create_sfr_map(void)
{
  add_file_registers(0x10, 0x1f, 0);
  add_file_registers(0x30, 0x3f, 0);
  alias_file_registers(0x07,0x0f,0x20);

  add_sfr_register(&indf,  0x20);
  add_sfr_register(&tmr0,  0x21);
  add_sfr_register(&pcl,   0x22);
  add_sfr_register(&status,0x23);
  add_sfr_register(&fsr,   0x24);
  add_sfr_register(&osccal,0x25);
  add_sfr_register(&gpio,  0x26);

  pic_processor::create_symbols();

}

P12C509::P12C509(void)
{
  if(verbose)
    cout << "12c509 constructor, type = " << isa() << '\n';
  // create_sfr_map();
  create_iopin_map(&iopin_map, &num_of_iopins);
  create_iopins(iopin_map, num_of_iopins);

  sfr_map = NULL;
  num_of_sfrs = 0;
 
  _12bit_processor::create();

  P12C508::create_sfr_map();
  create_sfr_map();
  tmr0.start();

  fsr.register_page_bits = RP0;  // the 509 has two register pages (i.e. RP0 in fsr is used)
  pa_bits = PA0;                 // the 509 has two code pages (i.e. PAO in status is used)

  // Build the links between the I/O Pins and the internal peripherals

  name_str = "12c509";

  trace.program_counter (pc.value);

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
