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
#include "p18x.h"

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

  //  cout << "PIR " << pir1.name() << '\n';

  //create_sfr_map();
  //  create_iopin_map(&iopin_map, &num_of_iopins);

  //_16bit_processor::create();

  //  create_iopins(iopin_map, num_of_iopins);

  name_str = "p18cxx2";

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

  assign_pin(19, NULL);
  assign_pin(20, NULL);

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
  if(verbose)
    cout << "P18C2x2::create\n";

  //  cout << "PIR " << pir1.name() << '\n';

  create_iopin_map();


  //  cout << "after create_iopin_mapPIR " << pir1.name() << '\n';

  //P18C2x2::create_sfr_map();
  //  create_iopin_map(&iopin_map, &num_of_iopins);
  //cout << "after create_sfr_map PIR " << pir1.name() << '\n';

  _16bit_processor::create();

  //  create_iopins(iopin_map, num_of_iopins);

}

void P18C2x2::create_sfr_map(void)
{

  if(verbose)
    cout << "create_sfr_map P18C2x2\n";

  add_sfr_register(porta,	  0xf80,0,"porta");
  add_sfr_register(portb,	  0xf81,0,"portb");
  add_sfr_register(portc,	  0xf82,0,"portc");

#if 0
  add_sfr_register(&lata,	  0xf89,0,"lata");
  add_sfr_register(&latb,	  0xf8a,0,"latb");
  add_sfr_register(&latc,	  0xf8b,0,"latc");
#endif

  add_sfr_register(&trisa,	  0xf92,0,"trisa");
  add_sfr_register(&trisb,	  0xf93,0,"trisb");
  add_sfr_register(&trisc,	  0xf94,0,"trisc");

}

void P18C2x2::create_symbols(void)
{
  if(verbose)
    cout << "P18C2x2 create symbols\n";

  symbol_table.add_ioport(porta->cpu, porta);
  symbol_table.add_ioport(portb->cpu, portb);
  symbol_table.add_ioport(portc->cpu, portc);

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

  if(verbose)
    cout << " 18c242 create \n";

  P18C2x2::create();

  P18C2x2::create_sfr_map();
  P18C242::create_sfr_map();

}

void P18C242::create_sfr_map(void)
{


}

pic_processor * P18C242::construct(void)
{

  P18C242 *p = new P18C242;

  if(verbose)
    cout << " 18c242 construct\n";

  p->create();
  p->create_invalid_registers();
  p->pic_processor::create_symbols();
  p->name_str = "p18c242";
  symbol_table.add_module(p,p->name_str);

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

  if(verbose)
    cout << " 18c252 create \n";

  P18C242::create();

  P18C2x2::create_sfr_map();
  P18C252::create_sfr_map();

}

void P18C252::create_sfr_map(void)
{


}



pic_processor * P18C252::construct(void)
{

  P18C252 *p = new P18C252;

  if(verbose)
    cout << " 18c252 construct\n";

  p->create();
  p->create_invalid_registers();
  p->pic_processor::create_symbols();

  p->name_str = "p18c252";
  symbol_table.add_module(p,p->name_str);

  return p;

}







//========================================================================
//
// Pic 18C4x2
//

void P18C4x2::create(void)
{
  if(verbose)
    cout << "P18C4x2::create\n";

  create_iopin_map();
  _16bit_processor::create();


}

void P18C4x2::create_symbols(void)
{
  if(verbose)
    cout << "P18C4x2 create symbols\n";

  symbol_table.add_ioport(porta->cpu, porta);
  symbol_table.add_ioport(portb->cpu, portb);
  symbol_table.add_ioport(portc->cpu, portc);
  symbol_table.add_ioport(portc->cpu, portd);
  symbol_table.add_ioport(portc->cpu, porte);

}

P18C4x2::P18C4x2(void)
{

  if(verbose)
    cout << "18c4x2 constructor, type = " << isa() << '\n';


}


void P18C4x2::create_sfr_map(void)
{

  if(verbose)
    cout << "create_sfr_map P18C4x2\n";

  add_sfr_register(porta,	  0xf80,0,"porta");
  add_sfr_register(portb,	  0xf81,0,"portb");
  add_sfr_register(portc,	  0xf82,0,"portc");
  add_sfr_register(portd,	  0xf83,0,"portd");
  add_sfr_register(porte,	  0xf84,0,"porte");

#if 0
  add_sfr_register(&lata,	  0xf89,0,"lata");
  add_sfr_register(&latb,	  0xf8a,0,"latb");
  add_sfr_register(&latc,	  0xf8b,0,"latc");
  add_sfr_register(&latd,	  0xf8c,0,"latd");
  add_sfr_register(&late,	  0xf8d,0,"late");
#endif

  add_sfr_register(&trisa,	  0xf92,0,"trisa");
  add_sfr_register(&trisb,	  0xf93,0,"trisb");
  add_sfr_register(&trisc,	  0xf94,0,"trisc");
  add_sfr_register(&trisd,	  0xf95,0,"trisd");
  add_sfr_register(&trise,	  0xf96,0,"trise");


}



//------------------------------------------------------------------------
//
// P18C442
// 

P18C442::P18C442(void)
{

  if(verbose)
    cout << "18c442 constructor, type = " << isa() << '\n';

}

void P18C442::create(void)
{

  if(verbose)
    cout << " 18c442 create \n";

  P18C4x2::create();

  P18C4x2::create_sfr_map();
  P18C442::create_sfr_map();

}

void P18C442::create_sfr_map(void)
{


}

pic_processor * P18C442::construct(void)
{

  P18C442 *p = new P18C442;

  if(verbose)
    cout << " 18c442 construct\n";

  p->create();
  p->create_invalid_registers();
  p->pic_processor::create_symbols();
  p->name_str = "p18c442";
  symbol_table.add_module(p,p->name_str);

  return p;


}



//------------------------------------------------------------------------
//
// P18C452
// 

P18C452::P18C452(void)
{

  if(verbose)
    cout << "18c452 constructor, type = " << isa() << '\n';

}

void P18C452::create(void)
{

  if(verbose)
    cout << " 18c452 create \n";

  P18C442::create();

  P18C4x2::create_sfr_map();
  P18C452::create_sfr_map();

}

void P18C452::create_sfr_map(void)
{


}


pic_processor * P18C452::construct(void)
{

  P18C452 *p = new P18C452;

  if(verbose)
    cout << " 18c452 construct\n";

  p->create();
  p->create_invalid_registers();
  p->pic_processor::create_symbols();
  p->name_str = "p18c452";
  symbol_table.add_module(p,p->name_str);

  return p;


}
