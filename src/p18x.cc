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
#include <iostream>
#include <string>

#include "../config.h"
#include "p18x.h"
#include "pic-packages.h"

#include "symbol.h"

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

  //usart.init_ioport(portc);

  //  create_iopins(iopin_map, num_of_iopins);

}

//------------------------------------------------------------------------
void P18C2x2::create_iopin_map(void)
{
  cout << "p18c2x2::create_iopin_map WARNING --- not createing package \n";

#if 0
  create_pkg(28);
  assign_pin(1, NULL);

  assign_pin(2, new IO_bi_directional(porta, 0));
  assign_pin(3, new IO_bi_directional(porta, 1));
  assign_pin(4, new IO_bi_directional(porta, 2));
  assign_pin(5, new IO_bi_directional(porta, 3));
  assign_pin(6, new IO_open_collector(porta, 4));
  assign_pin(7, new IO_bi_directional(porta, 5));

  assign_pin(8, NULL); //VSS
  assign_pin(9, NULL);  // OSC
  assign_pin(10, NULL); // OSC

  assign_pin(11, new IO_bi_directional(portc, 0));
  assign_pin(12, new IO_bi_directional(portc, 1));
  assign_pin(13, new IO_bi_directional(portc, 2));
  assign_pin(14, new IO_bi_directional(portc, 3));
  assign_pin(15, new IO_bi_directional(portc, 4));
  assign_pin(16, new IO_bi_directional(portc, 5));
  assign_pin(17, new IO_bi_directional(portc, 6));
  assign_pin(18, new IO_bi_directional(portc, 7));


  assign_pin(19, NULL); //VSS
  assign_pin(20, NULL); //VDD

  assign_pin(21, new IO_bi_directional_pu(portb, 0));
  assign_pin(22, new IO_bi_directional_pu(portb, 1));
  assign_pin(23, new IO_bi_directional_pu(portb, 2));
  assign_pin(24, new IO_bi_directional_pu(portb, 3));
  assign_pin(25, new IO_bi_directional_pu(portb, 4));
  assign_pin(26, new IO_bi_directional_pu(portb, 5));
  assign_pin(27, new IO_bi_directional_pu(portb, 6));
  assign_pin(28, new IO_bi_directional_pu(portb, 7));

#endif

}
void P18C2x2::create_sfr_map(void)
{

  if(verbose)
    cout << "create_sfr_map P18C2x2\n";

  add_sfr_register(&porta,	  0xf80,0,"porta");
  add_sfr_register(&portb,	  0xf81,0,"portb");
  add_sfr_register(&portc,	  0xf82,0,"portc");

  add_sfr_register(&lata,	  0xf89,0,"lata");
  add_sfr_register(&latb,	  0xf8a,0,"latb");
  add_sfr_register(&latc,	  0xf8b,0,"latc");

  porta.latch = &lata;
  portb.latch = &latb;
  portc.latch = &latc;

  lata.port = &porta;
  latb.port = &portb;
  latc.port = &portc;

  add_sfr_register(&trisa,	  0xf92,0x7f,"trisa");
  add_sfr_register(&trisb,	  0xf93,0xff,"trisb");
  add_sfr_register(&trisc,	  0xf94,0xff,"trisc");

}

void P18C2x2::create_symbols(void)
{
  if(verbose)
    cout << "P18C2x2 create symbols\n";

  symbol_table.add_ioport(this, &porta);
  symbol_table.add_ioport(this, &portb);
  symbol_table.add_ioport(this, &portc);

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

  trace.program_counter (pc->value);

  //usart.init_ioport(portc);
  cout << "c4x2::create usart txreg => " << usart16.txreg.name() << "\n";

}
//------------------------------------------------------------------------
void P18C4x2::create_iopin_map(void)
{

  package = new Package(40);

  if(!package)
    return;

  // Build the links between the I/O Ports and their tris registers.
  porta.tris = &trisa;
  trisa.port = &porta;

  portb.tris = &trisb;
  trisb.port = &portb;

  portc.tris = &trisc;
  trisc.port = &portc;

  portd.tris = &trisd;
  trisd.port = &portd;

  porte.tris = &trise;
  trise.port = &porte;


  porta.new_name("porta");
  portb.new_name("portb");
  portc.new_name("portc");
  portd.new_name("portd");
  porte.new_name("porte");

  trisa.new_name("trisa");
  trisb.new_name("trisb");
  trisc.new_name("trisc");
  trisd.new_name("trisd");
  trise.new_name("trise");

  // Define the valid I/O pins.
  porta.valid_iopins = 0x3f;
  portb.valid_iopins = 0xff;
  portc.valid_iopins = 0xff;
  portd.valid_iopins = 0xff;
  porte.valid_iopins = 0x07;

  package->assign_pin(1, NULL);

  package->assign_pin(2, new IO_bi_directional(&porta, 0));
  package->assign_pin(3, new IO_bi_directional(&porta, 1));
  package->assign_pin(4, new IO_bi_directional(&porta, 2));
  package->assign_pin(5, new IO_bi_directional(&porta, 3));
  package->assign_pin(6, new IO_open_collector(&porta, 4));
  package->assign_pin(7, new IO_bi_directional(&porta, 5));

  package->assign_pin(8, new IO_bi_directional(&porte, 0));
  package->assign_pin(9, new IO_bi_directional(&porte, 1));
  package->assign_pin(10, new IO_bi_directional(&porte, 2));


  package->assign_pin(11, NULL);
  package->assign_pin(12, NULL);
  package->assign_pin(13, NULL);
  package->assign_pin(14, NULL);

  cout << "Creating portc iopin\n";
  package->assign_pin(15, new IO_bi_directional(&portc, 0));
  package->assign_pin(16, new IO_bi_directional(&portc, 1));
  package->assign_pin(17, new IO_bi_directional(&portc, 2));
  package->assign_pin(18, new IO_bi_directional(&portc, 3));
  package->assign_pin(23, new IO_bi_directional(&portc, 4));
  package->assign_pin(24, new IO_bi_directional(&portc, 5));
  package->assign_pin(25, new IO_bi_directional(&portc, 6));
  package->assign_pin(26, new IO_bi_directional(&portc, 7));


  package->assign_pin(19, new IO_bi_directional(&portd, 0));
  package->assign_pin(20, new IO_bi_directional(&portd, 1));
  package->assign_pin(21, new IO_bi_directional(&portd, 2));
  package->assign_pin(22, new IO_bi_directional(&portd, 3));
  package->assign_pin(27, new IO_bi_directional(&portd, 4));
  package->assign_pin(28, new IO_bi_directional(&portd, 5));
  package->assign_pin(29, new IO_bi_directional(&portd, 6));
  package->assign_pin(30, new IO_bi_directional(&portd, 7));

  package->assign_pin(31, NULL);
  package->assign_pin(32, NULL);

  package->assign_pin(33, new IO_bi_directional_pu(&portb, 0));
  package->assign_pin(34, new IO_bi_directional_pu(&portb, 1));
  package->assign_pin(35, new IO_bi_directional_pu(&portb, 2));
  package->assign_pin(36, new IO_bi_directional_pu(&portb, 3));
  package->assign_pin(37, new IO_bi_directional_pu(&portb, 4));
  package->assign_pin(38, new IO_bi_directional_pu(&portb, 5));
  package->assign_pin(39, new IO_bi_directional_pu(&portb, 6));
  package->assign_pin(40, new IO_bi_directional_pu(&portb, 7));


  portc.ccp1con = &ccp1con;
  portc.usart = &usart16;


}
void P18C4x2::create_symbols(void)
{
  if(verbose)
    cout << "P18C4x2 create symbols\n";

  symbol_table.add_ioport(this, &porta);
  symbol_table.add_ioport(this, &portb);
  symbol_table.add_ioport(this, &portc);
  symbol_table.add_ioport(this, &portd);
  symbol_table.add_ioport(this, &porte);

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

  cout << "c4x2::create_sfr_map1 usart txreg => " << usart16.txreg.name() << "\n";

  add_sfr_register(&porta,	  0xf80,0,"porta");
  add_sfr_register(&portb,	  0xf81,0,"portb");
  add_sfr_register(&portc,	  0xf82,0,"portc");
  add_sfr_register(&portd,	  0xf83,0,"portd");
  add_sfr_register(&porte,	  0xf84,0,"porte");

  usart16.initialize_16(this,&pir1,&portc);
  cout << "c4x2::create_sfr_map2 usart txreg => " << usart16.txreg.name() << "\n";

  add_sfr_register(&lata,	  0xf89,0,"lata");
  add_sfr_register(&latb,	  0xf8a,0,"latb");
  add_sfr_register(&latc,	  0xf8b,0,"latc");
  add_sfr_register(&latd,	  0xf8c,0,"latd");
  add_sfr_register(&late,	  0xf8d,0,"late");

  porta.latch = &lata;
  portb.latch = &latb;
  portc.latch = &latc;
  portd.latch = &latd;
  porte.latch = &late;

  lata.port = &porta;
  latb.port = &portb;
  latc.port = &portc;
  latd.port = &portd;
  late.port = &porte;

  add_sfr_register(&trisa,	  0xf92,0x7f,"trisa");
  add_sfr_register(&trisb,	  0xf93,0xff,"trisb");
  add_sfr_register(&trisc,	  0xf94,0xff,"trisc");
  add_sfr_register(&trisd,	  0xf95,0xff,"trisd");
  add_sfr_register(&trise,	  0xf96,0x0f,"trise");


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

  //if(verbose)
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


//------------------------------------------------------------------------
//
// P18F442
// 

P18F442::P18F442(void)
{

  //if(verbose)
    cout << "18f442 constructor, type = " << isa() << '\n';

}

void P18F442::create(void)
{

  //if(verbose)
    cout << " 18f442 create \n";

  P18C442::create();

  //P18C4x2::create_sfr_map();
  P18F442::create_sfr_map();

}

void P18F442::create_sfr_map(void)
{


}

pic_processor * P18F442::construct(void)
{

  P18F442 *p = new P18F442;

  if(verbose)
    cout << " 18F442 construct\n";

  p->create();
  p->create_invalid_registers();
  p->pic_processor::create_symbols();
  p->name_str = "p18f442";
  symbol_table.add_module(p,p->name_str);
  //cout << "c4x2 -2 txsta assignment name: " << usart.txsta->name() << "\n";
  //cout << "c4x2 -2 txsta assignment name: " << p->usart.txsta->name() << "\n";

  return p;


}

//------------------------------------------------------------------------
//
// P18F452
// 

P18F452::P18F452(void)
{

  if(verbose)
    cout << "18f452 constructor, type = " << isa() << '\n';

}

void P18F452::create(void)
{

  if(verbose)
    cout << " 18f452 create \n";

  cout << "f452 usart txreg => " << usart16.txreg.name() << "\n";
  P18F442::create();
  P18F452::create_sfr_map();

}

void P18F452::create_sfr_map(void)
{


}

pic_processor * P18F452::construct(void)
{

  P18F452 *p = new P18F452;

  if(verbose)
    cout << " 18F452 construct\n";

  p->create();
  p->create_invalid_registers();
  p->pic_processor::create_symbols();
  p->name_str = "p18f452";

  cout << "usart txreg => " << p->usart16.txreg.name() << "\n";
  symbol_table.add_module(p,p->name_str);

  return p;


}
