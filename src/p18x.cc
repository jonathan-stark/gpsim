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
#include "packages.h"
#include "stimuli.h"
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

  create_iopin_map();

  _16bit_processor::create();

  trace.program_counter (pc->value);

}

//------------------------------------------------------------------------
void P18C2x2::create_iopin_map(void)
{
  package = new Package(28);

  if(!package)
    return;

  // Build the links between the I/O Ports and their tris registers.
  porta.tris = &trisa;
  trisa.port = &porta;

  portb.tris = &trisb;
  trisb.port = &portb;

  portc.tris = &trisc;
  trisc.port = &portc;

  porta.new_name("porta");
  portb.new_name("portb");
  portc.new_name("portc");

  trisa.new_name("trisa");
  trisb.new_name("trisb");
  trisc.new_name("trisc");

  // Define the valid I/O pins.
  porta.valid_iopins = 0x3f;
  portb.valid_iopins = 0xff;
  portc.valid_iopins = 0xff;

  package->assign_pin(1, 0);  // /MCLR

  package->assign_pin(2, new IO_bi_directional(&porta, 0));
  package->assign_pin(3, new IO_bi_directional(&porta, 1));
  package->assign_pin(4, new IO_bi_directional(&porta, 2));
  package->assign_pin(5, new IO_bi_directional(&porta, 3));
  package->assign_pin(6, new IO_open_collector(&porta, 4));
  package->assign_pin(7, new IO_bi_directional(&porta, 5));



  package->assign_pin(8, 0);  // Vss
  package->assign_pin(9, 0);  // OSC1

  package->assign_pin(10, new IO_bi_directional(&porta, 6));

  package->assign_pin(11, new IO_bi_directional(&portc, 0));
  package->assign_pin(12, new IO_bi_directional(&portc, 1));
  package->assign_pin(13, new IO_bi_directional(&portc, 2));
  package->assign_pin(14, new IO_bi_directional(&portc, 3));
  package->assign_pin(15, new IO_bi_directional(&portc, 4));
  package->assign_pin(16, new IO_bi_directional(&portc, 5));
  package->assign_pin(17, new IO_bi_directional(&portc, 6));
  package->assign_pin(18, new IO_bi_directional(&portc, 7));


  package->assign_pin(19, 0);  // Vss
  package->assign_pin(20, 0);  // Vdd

  package->assign_pin(21, new IO_bi_directional_pu(&portb, 0));
  package->assign_pin(22, new IO_bi_directional_pu(&portb, 1));
  package->assign_pin(23, new IO_bi_directional_pu(&portb, 2));
  package->assign_pin(24, new IO_bi_directional_pu(&portb, 3));
  package->assign_pin(25, new IO_bi_directional_pu(&portb, 4));
  package->assign_pin(26, new IO_bi_directional_pu(&portb, 5));
  package->assign_pin(27, new IO_bi_directional_pu(&portb, 6));
  package->assign_pin(28, new IO_bi_directional_pu(&portb, 7));

  portc.ccp1con = &ccp1con;
  portc.usart = &usart16;

}
void P18C2x2::create_sfr_map(void)
{

  if(verbose)
    cout << "create_sfr_map P18C2x2\n";

  add_sfr_register(&porta,	  0xf80,0,"porta");
  add_sfr_register(&portb,	  0xf81,0,"portb");
  add_sfr_register(&portc,	  0xf82,0,"portc");

  usart16.initialize_16(this,&pir_set_def,&portc);

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

Processor * P18C242::construct(void)
{

  P18C242 *p = new P18C242;

  if(verbose)
    cout << " 18c242 construct\n";

  p->create();
  p->create_invalid_registers();
  p->pic_processor::create_symbols();

  p->new_name("p18c242");
  symbol_table.add_module(p,p->name().c_str());

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



Processor * P18C252::construct(void)
{

  P18C252 *p = new P18C252;

  if(verbose)
    cout << " 18c252 construct\n";

  p->create();
  p->create_invalid_registers();
  p->pic_processor::create_symbols();

  p->new_name("p18c252");
  symbol_table.add_module(p,p->name().c_str());

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

  package->assign_pin(1, 0); // /MCLR

  package->assign_pin(2, new IO_bi_directional(&porta, 0));
  package->assign_pin(3, new IO_bi_directional(&porta, 1));
  package->assign_pin(4, new IO_bi_directional(&porta, 2));
  package->assign_pin(5, new IO_bi_directional(&porta, 3));
  package->assign_pin(6, new IO_open_collector(&porta, 4));
  package->assign_pin(7, new IO_bi_directional(&porta, 5));

  package->assign_pin(8, new IO_bi_directional(&porte, 0));
  package->assign_pin(9, new IO_bi_directional(&porte, 1));
  package->assign_pin(10, new IO_bi_directional(&porte, 2));


  package->assign_pin(11, 0);
  package->assign_pin(12, 0);
  package->assign_pin(13, 0);
  package->assign_pin(14, 0);

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

  package->assign_pin(31, 0);
  package->assign_pin(32, 0);

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

  add_sfr_register(&porta,	  0xf80,0,"porta");
  add_sfr_register(&portb,	  0xf81,0,"portb");
  add_sfr_register(&portc,	  0xf82,0,"portc");
  add_sfr_register(&portd,	  0xf83,0,"portd");
  add_sfr_register(&porte,	  0xf84,0,"porte");

  //usart16.initialize_16(this,get_pir_set(),&portc);
  usart16.initialize_16(this,&pir_set_def,&portc);

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

Processor * P18C442::construct(void)
{

  P18C442 *p = new P18C442;

  if(verbose)
    cout << " 18c442 construct\n";

  p->create();
  p->create_invalid_registers();
  p->pic_processor::create_symbols();

  p->new_name("p18c442");
  symbol_table.add_module(p,p->name().c_str());

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


Processor * P18C452::construct(void)
{

  P18C452 *p = new P18C452;

  if(verbose)
    cout << " 18c452 construct\n";

  p->create();
  p->create_invalid_registers();
  p->pic_processor::create_symbols();

  p->new_name("p18c452");
  symbol_table.add_module(p,p->name().c_str());

  return p;


}


//------------------------------------------------------------------------
//
// P18F242
// 

P18F242::P18F242(void)
{

  //if(verbose)
    cout << "18f242 constructor, type = " << isa() << '\n';

}

void P18F242::create(void)
{
  EEPROM_PIR *e;

  //if(verbose)
    cout << " 18f242 create \n";

  P18C242::create();

  e = new EEPROM_PIR;
  e->set_cpu(this);
  // We might want to pass this value in for larger eeproms
  e->initialize(256);
  //e->set_pir_set(get_pir_set());
  e->set_pir_set(&pir_set_def);
  e->set_intcon(&intcon);

  // assign this eeprom to the processor
  set_eeprom_pir(e);

  P18F242::create_sfr_map();

}

void P18F242::create_sfr_map(void)
{

  // Add eeprom
  add_sfr_register(get_eeprom()->get_reg_eedata(), 0xfa8);
  add_sfr_register(get_eeprom()->get_reg_eeadr(), 0xfa9);
  add_sfr_register(get_eeprom()->get_reg_eecon1(), 0xfa6, 0);
  add_sfr_register(get_eeprom()->get_reg_eecon2(), 0xfa7);

}

void P18F242::set_out_of_range_pm(unsigned int address, unsigned int value)
{

  if( (address>= 0xf00000) && (address < 0xf00000 +
    get_eeprom()->get_rom_size()))
    {
      get_eeprom()->change_rom(address - 0xf00000, value);
    }
}

Processor * P18F242::construct(void)
{

  P18F242 *p = new P18F242;

  if(verbose)
    cout << " 18F242 construct\n";

  p->create();
  p->create_invalid_registers();
  p->pic_processor::create_symbols();

  p->new_name("p18f242");
  symbol_table.add_module(p,p->name().c_str());

  return p;


}


//------------------------------------------------------------------------
//
// P18F252
// 

P18F252::P18F252(void)
{

  if(verbose)
    cout << "18f252 constructor, type = " << isa() << '\n';

}

void P18F252::create(void)
{

  if(verbose)
    cout << " 18f252 create \n";

  P18F242::create();
  P18F252::create_sfr_map();

}

void P18F252::create_sfr_map(void)
{


}

Processor * P18F252::construct(void)
{

  P18F252 *p = new P18F252;

  if(verbose)
    cout << " 18F252 construct\n";

  p->create();
  p->create_invalid_registers();
  p->pic_processor::create_symbols();

  p->new_name("p18f252");
  symbol_table.add_module(p,p->name().c_str());

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
  EEPROM_PIR *e;

  //if(verbose)
    cout << " 18f442 create \n";

  P18C442::create();

  e = new EEPROM_PIR;
  e->set_cpu(this);
  // We might want to pass this value in for larger eeproms
  e->initialize(256);
  //e->set_pir_set(get_pir_set());
  e->set_pir_set(&pir_set_def);
  e->set_intcon(&intcon);

  // assign this eeprom to the processor
  set_eeprom_pir(e);

  //P18C4x2::create_sfr_map();
  P18F442::create_sfr_map();

}

void P18F442::create_sfr_map(void)
{

  // Add eeprom
  add_sfr_register(get_eeprom()->get_reg_eedata(), 0xfa8);
  add_sfr_register(get_eeprom()->get_reg_eeadr(), 0xfa9);
  add_sfr_register(get_eeprom()->get_reg_eecon1(), 0xfa6, 0);
  add_sfr_register(get_eeprom()->get_reg_eecon2(), 0xfa7);

}

void P18F442::set_out_of_range_pm(unsigned int address, unsigned int value)
{

  if( (address>= 0xf00000) && (address < 0xf00000 +
    get_eeprom()->get_rom_size()))
    {
      get_eeprom()->change_rom(address - 0xf00000, value);
    }
}

Processor * P18F442::construct(void)
{

  P18F442 *p = new P18F442;

  if(verbose)
    cout << " 18F442 construct\n";

  p->create();
  p->create_invalid_registers();
  p->pic_processor::create_symbols();

  p->new_name("p18f442");
  symbol_table.add_module(p,p->name().c_str());

  return p;


}


//------------------------------------------------------------------------
//
// P18F258
// 

P18F248::P18F248(void)
{

  if(verbose)
    cout << "18f248 constructor, type = " << isa() << '\n';

}

void P18F248::create(void)
{

  if(verbose)
    cout << " 18f248 create \n";

  P18F442::create();
  P18F248::create_sfr_map();

}

void P18F248::create_sfr_map(void)
{


}

Processor * P18F248::construct(void)
{

  P18F248 *p = new P18F248;

  if(verbose)
    cout << " 18F248 construct\n";

  p->create();
  p->create_invalid_registers();
  p->pic_processor::create_symbols();

  p->new_name("p18f248");
  symbol_table.add_module(p,p->name().c_str());

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

  P18F442::create();
  P18F452::create_sfr_map();

}

void P18F452::create_sfr_map(void)
{


}

Processor * P18F452::construct(void)
{

  P18F452 *p = new P18F452;

  if(verbose)
    cout << " 18F452 construct\n";

  p->create();
  p->create_invalid_registers();
  p->pic_processor::create_symbols();

  p->new_name("p18f452");
  symbol_table.add_module(p,p->name().c_str());

  return p;


}


//------------------------------------------------------------------------
//
// P18F1220
// 

Processor * P18F1220::construct(void)
{

  P18F1220 *p = new P18F1220;

  if(verbose)
    cout << " 18F1220 construct\n";

  p->create();
  p->create_invalid_registers();
  p->pic_processor::create_symbols();

  p->new_name("p18f1220");
  symbol_table.add_module(p,p->name().c_str());

  return p;
}

void P18F1220::create(void)
{
  if(verbose)
    cout << "P18F1220::create\n";

  create_iopin_map();

  _16bit_processor::create();

  create_sfr_map();


}
//------------------------------------------------------------------------
void P18F1220::create_iopin_map(void)
{

  package = new Package(18);

  if(!package)
    return;

  // Build the links between the I/O Ports and their tris registers.
  porta.tris = &trisa;
  trisa.port = &porta;

  portb.tris = &trisb;
  trisb.port = &portb;

  porta.new_name("porta");
  portb.new_name("portb");

  trisa.new_name("trisa");
  trisb.new_name("trisb");

  // Define the valid I/O pins.
  porta.valid_iopins = 0xff;
  portb.valid_iopins = 0xff;

  package->assign_pin(1, new IO_bi_directional(&porta, 0));
  package->assign_pin(2, new IO_bi_directional(&porta, 1));
  package->assign_pin(6, new IO_bi_directional(&porta, 2));
  package->assign_pin(7, new IO_bi_directional(&porta, 3));
  package->assign_pin(4, new IO_open_collector(&porta, 5));
  package->assign_pin(3, new IO_bi_directional(&porta, 4));
  package->assign_pin(15, new IO_bi_directional(&porta, 6));
  package->assign_pin(16, new IO_bi_directional(&porta, 7));

  package->assign_pin(8, new IO_bi_directional(&portb, 0));
  package->assign_pin(9, new IO_bi_directional(&portb, 1));
  package->assign_pin(17, new IO_bi_directional(&portb, 2));
  package->assign_pin(18, new IO_bi_directional(&portb, 3));
  package->assign_pin(10, new IO_bi_directional(&portb, 4));
  package->assign_pin(11, new IO_bi_directional(&portb, 5));
  package->assign_pin(12, new IO_bi_directional(&portb, 6));
  package->assign_pin(13, new IO_bi_directional(&portb, 7));

  package->assign_pin(5, 0);
  package->assign_pin(14, 0);

}
void P18F1220::create_symbols(void)
{
  if(verbose)
    cout << "P18F1220 create symbols\n";

  symbol_table.add_ioport(this, &porta);
  symbol_table.add_ioport(this, &portb);

}

P18F1220::P18F1220(void)
{

  if(verbose)
    cout << "18F1220 constructor, type = " << isa() << '\n';


}


void P18F1220::create_sfr_map(void)
{

  if(verbose)
    cout << "create_sfr_map P18F1220\n";

  add_sfr_register(&porta,	  0xf80,0,"porta");
  add_sfr_register(&portb,	  0xf81,0,"portb");

  //usart16.initialize_16(this,get_pir_set(),&porta);
  usart16.initialize_16(this,&pir_set_def,&porta);

  add_sfr_register(&lata,	  0xf89,0,"lata");
  add_sfr_register(&latb,	  0xf8a,0,"latb");

  porta.latch = &lata;
  portb.latch = &latb;

  lata.port = &porta;
  latb.port = &portb;

  add_sfr_register(&trisa,	  0xf92,0x7f,"trisa");
  add_sfr_register(&trisb,	  0xf93,0xff,"trisb");

}



//------------------------------------------------------------------------
//
// P18Fx320
// 

P18F1320::P18F1320(void)
{

  //if(verbose)
    cout << "18f1320 constructor, type = " << isa() << '\n';

}

void P18F1320::create(void)
{

  //if(verbose)
    cout << " 18fx320 create \n";

  P18F1220::create();

  P18F1220::create_sfr_map();


}

Processor * P18F1320::construct(void)
{

  P18F1320 *p = new P18F1320;

  if(verbose)
    cout << " 18F1320 construct\n";

  p->create();
  p->create_invalid_registers();
  p->pic_processor::create_symbols();

  p->new_name("p18f1320");
  symbol_table.add_module(p,p->name().c_str());

  return p;


}

