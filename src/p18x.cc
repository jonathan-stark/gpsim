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
along with gpsim; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#include <stdio.h>
#include <iostream.h>
#include <string>

#include "symbol.h"

#include "p18x.h"



void P18Cxx2::create_sfr_map(void)
{

  cout << "create_sfr_map P18Cxx2\n";
}

void P18Cxx2::create_symbols(void)
{
  cout << "P18Cxx2 create symbols\n";

}

//========================================================================
//
// Pic 18C
//


P18Cxx2::P18Cxx2(void)
{
  if(verbose)
    cout << "18c constructor, type = " << isa() << '\n';

  create_sfr_map();
  //  create_iopin_map(&iopin_map, &num_of_iopins);

  _16bit_processor::create();

  //  create_iopins(iopin_map, num_of_iopins);

  name_str = "18cxx2";

}



void _28pins::create_iopin_map(void)
{
  // ---- This is probably going to be moved:
  porta = new PORTA;
  portb = new PORTB;
  portc = new PORTC;


  if(verbose)
    cout << "Create i/o pin map\n";
  // Build the links between the I/O Ports and their tris registers.
  porta->tris = &trisa;
  trisa.port = porta;

  portb->tris = &trisb;
  trisb.port = portb;

  portc->tris = &trisc;
  trisc.port = portc;

  // And give them a more meaningful name.
  trisa.new_name("trisa");
  trisb.new_name("trisb");
  trisc.new_name("trisc");

  // Define the valid I/O pins.
  porta->valid_iopins = 0x3f;
  portb->valid_iopins = 0xff;
  portc->valid_iopins = 0xff;


  // Now Create the package and place the I/O pins

  create_pkg(28);

  assign_pin(1, NULL);

  assign_pin(2, new IO_bi_directional(porta, 0));
  assign_pin(3, new IO_bi_directional(porta, 1));
  assign_pin(4, new IO_bi_directional(porta, 2));
  assign_pin(5, new IO_bi_directional(porta, 3));
  assign_pin(6, new IO_open_collector(porta, 4));
  assign_pin(7, new IO_open_collector(porta, 5));

  assign_pin(8, NULL);
  assign_pin(9, NULL);
  assign_pin(10, NULL);

  assign_pin(11, new IO_bi_directional(portc, 0));
  assign_pin(12, new IO_bi_directional(portc, 1));
  assign_pin(13, new IO_bi_directional(portc, 2));
  assign_pin(14, new IO_bi_directional(portc, 3));
  assign_pin(15, new IO_bi_directional(portc, 4));
  assign_pin(16, new IO_bi_directional(portc, 5));
  assign_pin(17, new IO_bi_directional(portc, 6));
  assign_pin(18, new IO_bi_directional(portc, 7));

  assign_pin(21, new IO_bi_directional_pu(portb, 0));
  assign_pin(22, new IO_bi_directional_pu(portb, 1));
  assign_pin(23, new IO_bi_directional_pu(portb, 2));
  assign_pin(24, new IO_bi_directional_pu(portb, 3));
  assign_pin(25, new IO_bi_directional_pu(portb, 4));
  assign_pin(26, new IO_bi_directional_pu(portb, 5));
  assign_pin(27, new IO_bi_directional_pu(portb, 6));
  assign_pin(28, new IO_bi_directional_pu(portb, 7));

}

//========================================================================
//
// Pic 18C2x2
//

void P18C2x2::create(void)
{

  create_iopin_map();


  P18C2x2::create_sfr_map();
  //  create_iopin_map(&iopin_map, &num_of_iopins);

  _16bit_processor::create();

  //  create_iopins(iopin_map, num_of_iopins);

}

void P18C2x2::create_sfr_map(void)
{

  cout << "create_sfr_map P18C2x2\n";
}

void P18C2x2::create_symbols(void)
{
  cout << "P18C2x2 create symbols\n";

}

P18C2x2::P18C2x2(void)
{

  if(verbose)
    cout << "18c2x2 constructor, type = " << isa() << '\n';

}


//------------------------------------------------------------------------
//
// P18C242
// 

P18C242::P18C242(void)
{

  if(verbose)
    cout << "18c242 constructor, type = " << isa() << '\n';

}

void P18C242::create(void)
{

  cout << " 18c242 create \n";

  P18C2x2::create();

  //  P18C242::create_sfr_map();

}

pic_processor * P18C242::construct(void)
{

  P18C242 *p = new P18C242;

  cout << " 18c242 construct\n";

  p->create();

  p->name_str = "18c242";

  return p;

}

//------------------------------------------------------------------------
//
// P18C252
// 

P18C252::P18C252(void)
{

  if(verbose)
    cout << "18c252 constructor, type = " << isa() << '\n';

}

void P18C252::create(void)
{

  cout << " 18c252 create \n";

  P18C242::create();

  //  P18C252::create_sfr_map();

}

pic_processor * P18C252::construct(void)
{

  P18C252 *p = new P18C252;

  cout << " 18c252 construct\n";

  p->create();

  p->name_str = "18c252";

  return p;

}
