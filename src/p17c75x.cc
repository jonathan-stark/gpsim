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

#include "../config.h"
#include "p17c75x.h"

void _68pins::create_iopin_map(void)
{
  // ---- This is probably going to be moved:
  porta = new PORTA;
  portb = new PORTB;
  portc = new PORTC;
  portd = new PORTD;
  porte = new PORTE;
  portf = new PORTF;
  portg = new PORTG;

  if(verbose)
    cout << "Create i/o pin map\n";
  // Build the links between the I/O Ports and their tris registers.
  porta->tris = &ddra;

  portb->tris = &ddrb;
  ddrb.port = portb;

  portc->tris = &ddrc;
  ddrc.port = portc;

  portd->tris = &ddrd;
  ddrd.port = portd;

  porte->tris = &ddre;
  ddre.port = porte;

  portf->tris = &ddrf;
  ddrf.port = portf;

  portg->tris = &ddrg;
  ddrg.port = portg;

  // And give them a more meaningful name.
  ddrb.new_name("ddrb");
  ddrc.new_name("ddrc");
  ddrd.new_name("ddrd");
  ddre.new_name("ddre");
  ddrf.new_name("ddrf");
  ddrg.new_name("ddrg");

  // Define the valid I/O pins.
  porta->valid_iopins = 0x3f;
  portb->valid_iopins = 0xff;
  portc->valid_iopins = 0xff;
  portd->valid_iopins = 0xff;
  porte->valid_iopins = 0x0f;
  portf->valid_iopins = 0xff;
  portg->valid_iopins = 0xff;


  // Now Create the package and place the I/O pins

  create_pkg(68);

  // Vdd and Vss pins
  assign_pin(2, NULL);
  assign_pin(20, NULL);
  assign_pin(37, NULL);
  assign_pin(49, NULL);
  assign_pin(19, NULL);
  assign_pin(36, NULL);
  assign_pin(53, NULL);
  assign_pin(68, NULL);

  // AVdd and AVss pins
  assign_pin(29, NULL);
  assign_pin(30, NULL);

  // NC pins
  assign_pin(1, NULL);
  assign_pin(18, NULL);
  assign_pin(35, NULL);
  assign_pin(52, NULL);

  // Test pin
  assign_pin(17, NULL);

  // Reset pin
  assign_pin(16, NULL);
  
  // Oscillator pins
  assign_pin(50, NULL);
  assign_pin(51, NULL);

  assign_pin(60, new IO_input(porta, 0));
  assign_pin(44, new IO_input(porta, 1));
  assign_pin(45, new IO_bi_directional_pu(porta, 2));
  assign_pin(46, new IO_bi_directional_pu(porta, 3));
  assign_pin(43, new IO_bi_directional(porta, 4));
  assign_pin(42, new IO_bi_directional(porta, 5));

  assign_pin(59, new IO_bi_directional(portb, 0));
  assign_pin(58, new IO_bi_directional(portb, 1));
  assign_pin(54, new IO_bi_directional(portb, 2));
  assign_pin(57, new IO_bi_directional(portb, 3));
  assign_pin(56, new IO_bi_directional(portb, 4));
  assign_pin(55, new IO_bi_directional(portb, 5));
  assign_pin(47, new IO_bi_directional(portb, 6));
  assign_pin(48, new IO_bi_directional(portb, 7));

  assign_pin(3,  new IO_bi_directional(portc, 0));
  assign_pin(67, new IO_bi_directional(portc, 1));
  assign_pin(66, new IO_bi_directional(portc, 2));
  assign_pin(65, new IO_bi_directional(portc, 3));
  assign_pin(64, new IO_bi_directional(portc, 4));
  assign_pin(63, new IO_bi_directional(portc, 5));
  assign_pin(62, new IO_bi_directional(portc, 6));
  assign_pin(61, new IO_bi_directional(portc, 7));

  assign_pin(11, new IO_bi_directional(portd, 0));
  assign_pin(10, new IO_bi_directional(portd, 1));
  assign_pin(9,  new IO_bi_directional(portd, 2));
  assign_pin(8,  new IO_bi_directional(portd, 3));
  assign_pin(7,  new IO_bi_directional(portd, 4));
  assign_pin(6,  new IO_bi_directional(portd, 5));
  assign_pin(5,  new IO_bi_directional(portd, 6));
  assign_pin(4,  new IO_bi_directional(portd, 7));

  assign_pin(12, new IO_bi_directional(porte, 0));
  assign_pin(13, new IO_bi_directional(porte, 1));
  assign_pin(14, new IO_bi_directional(porte, 2));
  assign_pin(15, new IO_bi_directional(porte, 3));

  assign_pin(28,  new IO_bi_directional(portf, 0));
  assign_pin(27, new IO_bi_directional(portf, 1));
  assign_pin(26, new IO_bi_directional(portf, 2));
  assign_pin(25, new IO_bi_directional(portf, 3));
  assign_pin(24, new IO_bi_directional(portf, 4));
  assign_pin(23, new IO_bi_directional(portf, 5));
  assign_pin(22, new IO_bi_directional(portf, 6));
  assign_pin(21, new IO_bi_directional(portf, 7));

  assign_pin(34, new IO_bi_directional(portg, 0));
  assign_pin(33, new IO_bi_directional(portg, 1));
  assign_pin(32, new IO_bi_directional(portg, 2));
  assign_pin(31, new IO_bi_directional(portg, 3));
  assign_pin(38, new IO_bi_directional(portg, 4));
  assign_pin(39, new IO_bi_directional(portg, 5));
  assign_pin(41, new IO_bi_directional(portg, 6));
  assign_pin(40, new IO_bi_directional(portg, 7));

}


//========================================================================
//
// Pic 17C7xx
//


pic_processor * P17C7xx::construct(void)
{

  P17C7xx *p = new P17C7xx;

  cout << " 17c75x construct\n";

  p->P17C7xx::create(0x1fff);
  p->create_invalid_registers ();
  p->pic_processor::create_symbols();
  p->name_str = "17c7xx";

  return p;

}

P17C7xx::P17C7xx(void)
{
  if(verbose)
    cout << "17c7xx constructor, type = " << isa() << '\n';
  
  _16bit_processor::create();
  
  //  create_iopins(iopin_map, num_of_iopins);
  
  name_str = "17c7xx";
  
}

void  P17C7xx::create(int ram_top)
{
  cout << "p17c7xx create\n";

  create_iopin_map();
  _16bit_processor::create();

  add_file_registers(0x0, ram_top, 0);

  P17C7xx::create_sfr_map();
}




void P17C7xx::create_symbols(void)
{
}

void P17C7xx::create_sfr_map(void)
{  
  if(verbose)
    cout << "creating 17c7xx common registers\n";


  cout << "create_sfr_map P17C7xx\n";
}

//========================================================================
//
// Pic 17C75x
//

pic_processor * P17C75x::construct(void)
{

  P17C75x *p = new P17C75x;

  cout << " 17c75x construct\n";

  p->P17C75x::create(0x1fff);
  p->create_invalid_registers ();
  p->pic_processor::create_symbols();
  p->name_str = "17c75x";

  return p;

}


void P17C75x::create(int ram_top)
{
  create_iopin_map();

  _16bit_processor::create();

  P17C75x::create_sfr_map();
  P17C75x::create_symbols();

}

P17C75x::P17C75x(void)
{
  if(verbose)
    cout << "17c75x constructor, type = " << isa() << '\n';
  
  create_sfr_map();
  //  create_iopin_map(&iopin_map, &num_of_iopins);
  
  _16bit_processor::create();
  
  //  create_iopins(iopin_map, num_of_iopins);
  
  name_str = "17c75x";
  
}

void P17C75x::create_symbols(void)
{
  if(verbose)
    cout << "p17c75x create symbols\n";

  symbol_table.add_ioport(portb->cpu, portb);
  symbol_table.add_ioport(porta->cpu, porta);
  symbol_table.add_ioport(portc->cpu, portc);
  symbol_table.add_ioport(portd->cpu, portd);
  symbol_table.add_ioport(porte->cpu, porte);
  symbol_table.add_ioport(portf->cpu, portf);
  symbol_table.add_ioport(portg->cpu, portg);

}

void P17C75x::create_sfr_map(void)
{

}


//========================================================================
//
// Pic 17C756
//

pic_processor * P17C756::construct(void)
{

  P17C756 *p = new P17C756;

  cout << " 17c756 construct\n";

  p->P17C7xx::create(0x1fff);
  p->create_invalid_registers ();
  p->pic_processor::create_symbols();
  p->name_str = "17c756";

  return p;

}

void P17C756::create(void)
{

  create_iopin_map();


  P17C756::create_sfr_map();
  //  create_iopin_map(&iopin_map, &num_of_iopins);

  _16bit_processor::create();

  //  create_iopins(iopin_map, num_of_iopins);

}

void P17C756::create_sfr_map(void)
{

  cout << "create_sfr_map P17C756\n";
}

void P17C756::create_symbols(void)
{
  cout << "P17C756 create symbols\n";

}

P17C756::P17C756(void)
{

  if(verbose)
    cout << "17c756 constructor, type = " << isa() << '\n';

}


//------------------------------------------------------------------------
//
// P17C756A
// 

pic_processor * P17C756A::construct(void)
{

  P17C756A *p = new P17C756A;

  cout << " 17c756a construct\n";

  p->P17C7xx::create(0x1fff);
  //p->create_invalid_registers ();
  p->pic_processor::create_symbols();
  p->name_str = "17c756a";

  return p;

}

P17C756A::P17C756A(void)
{

  if(verbose)
    cout << "17c756a constructor, type = " << isa() << '\n';

}

void P17C756A::create(void)
{

  create_iopin_map();


  P17C756A::create_sfr_map();
  //  create_iopin_map(&iopin_map, &num_of_iopins);

  _16bit_processor::create();

  //  create_iopins(iopin_map, num_of_iopins);

  cout << " 17c756a create \n";

}


void P17C756A::create_sfr_map(void)
{

  cout << "create_sfr_map P17C756A\n";
}

void P17C756A::create_symbols(void)
{
  cout << "P17C756A create symbols\n";

}


//========================================================================
//
// Pic 17C752
//

pic_processor * P17C752::construct(void)
{

  P17C752 *p = new P17C752;

  cout << " 17c752 construct\n";

  p->P17C7xx::create(0x1fff);
  p->create_invalid_registers ();
  p->pic_processor::create_symbols();
  p->name_str = "17c752";

  return p;

}

void P17C752::create(void)
{

  create_iopin_map();


  P17C752::create_sfr_map();
  //  create_iopin_map(&iopin_map, &num_of_iopins);

  _16bit_processor::create();

  //  create_iopins(iopin_map, num_of_iopins);

}

void P17C752::create_sfr_map(void)
{

  cout << "create_sfr_map P17C752\n";
}

void P17C752::create_symbols(void)
{
  cout << "P17C752 create symbols\n";

}

P17C752::P17C752(void)
{

  if(verbose)
    cout << "17c752 constructor, type = " << isa() << '\n';

}

//========================================================================
//
// Pic 17C762
//

pic_processor * P17C762::construct(void)
{

  P17C762 *p = new P17C762;

  cout << " 17c762 construct\n";

  p->P17C7xx::create(0x1fff);
  p->create_invalid_registers ();
  p->pic_processor::create_symbols();
  p->name_str = "17c762";

  return p;

}

void P17C762::create(void)
{

  create_iopin_map();


  P17C762::create_sfr_map();
  //  create_iopin_map(&iopin_map, &num_of_iopins);

  _16bit_processor::create();

  //  create_iopins(iopin_map, num_of_iopins);

}

void P17C762::create_sfr_map(void)
{

  cout << "create_sfr_map P17C762\n";
}

void P17C762::create_symbols(void)
{
  cout << "P17C762 create symbols\n";

}

P17C762::P17C762(void)
{

  if(verbose)
    cout << "17c762 constructor, type = " << isa() << '\n';

}

//========================================================================
//
// Pic 17C766
//

pic_processor * P17C766::construct(void)
{

  P17C766 *p = new P17C766;

  cout << " 17c75x construct\n";

  p->P17C7xx::create(0x1fff);
  p->create_invalid_registers ();
  p->pic_processor::create_symbols();
  p->name_str = "17c766";

  return p;

}

void P17C766::create(void)
{

  create_iopin_map();


  P17C766::create_sfr_map();
  //  create_iopin_map(&iopin_map, &num_of_iopins);

  _16bit_processor::create();

  //  create_iopins(iopin_map, num_of_iopins);

}

void P17C766::create_sfr_map(void)
{

  cout << "create_sfr_map P17C766\n";
}

void P17C766::create_symbols(void)
{
  cout << "P17C766 create symbols\n";

}

P17C766::P17C766(void)
{

  if(verbose)
    cout << "17c766 constructor, type = " << isa() << '\n';

}



