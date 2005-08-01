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

//========================================================================
//
// Pic 18C2x2
//

void P18C2x2::create()
{
  if(verbose)
    cout << "P18C2x2::create\n";

  create_iopin_map();

  _16bit_processor::create();

}

//------------------------------------------------------------------------
void P18C2x2::create_iopin_map()
{
  package = new Package(28);

  if(!package)
    return;

  // Build the links between the I/O Ports and their tris registers.

  package->assign_pin(1, 0);  // /MCLR

  package->assign_pin( 2, m_porta->addPin(new IO_bi_directional("porta0"),0));
  package->assign_pin( 3, m_porta->addPin(new IO_bi_directional("porta1"),1));
  package->assign_pin( 4, m_porta->addPin(new IO_bi_directional("porta2"),2));
  package->assign_pin( 5, m_porta->addPin(new IO_bi_directional("porta3"),3));
  package->assign_pin( 6, m_porta->addPin(new IO_open_collector("porta4"),4));
  package->assign_pin( 7, m_porta->addPin(new IO_bi_directional("porta5"),5));



  package->assign_pin(8, 0);  // Vss
  package->assign_pin(9, 0);  // OSC1

  package->assign_pin(10, m_porta->addPin(new IO_bi_directional("porta6"),6));

  package->assign_pin(11, m_portc->addPin(new IO_bi_directional("portc0"),0));
  package->assign_pin(12, m_portc->addPin(new IO_bi_directional("portc1"),1));
  package->assign_pin(13, m_portc->addPin(new IO_bi_directional("portc2"),2));
  package->assign_pin(14, m_portc->addPin(new IO_bi_directional("portc3"),3));
  package->assign_pin(15, m_portc->addPin(new IO_bi_directional("portc4"),4));
  package->assign_pin(16, m_portc->addPin(new IO_bi_directional("portc5"),5));
  package->assign_pin(17, m_portc->addPin(new IO_bi_directional("portc6"),6));
  package->assign_pin(18, m_portc->addPin(new IO_bi_directional("portc7"),7));

  package->assign_pin(19, 0);  // Vss
  package->assign_pin(20, 0);  // Vdd

  package->assign_pin(21, m_portb->addPin(new IO_bi_directional_pu("portb0"),0));
  package->assign_pin(22, m_portb->addPin(new IO_bi_directional_pu("portb1"),1));
  package->assign_pin(23, m_portb->addPin(new IO_bi_directional_pu("portb2"),2));
  package->assign_pin(24, m_portb->addPin(new IO_bi_directional_pu("portb3"),3));
  package->assign_pin(25, m_portb->addPin(new IO_bi_directional_pu("portb4"),4));
  package->assign_pin(26, m_portb->addPin(new IO_bi_directional_pu("portb5"),5));
  package->assign_pin(27, m_portb->addPin(new IO_bi_directional_pu("portb6"),6));
  package->assign_pin(28, m_portb->addPin(new IO_bi_directional_pu("portb7"),7));

  //1portc.ccp1con = &ccp1con;
  //1portc.usart = &usart16;

}
void P18C2x2::create_symbols()
{
  if(verbose)
    cout << "P18C2x2 create symbols\n";

  _16bit_processor::create_symbols();
}

P18C2x2::P18C2x2()
{

  if(verbose)
    cout << "18c2x2 constructor, type = " << isa() << '\n';


}


//------------------------------------------------------------------------
//
// P18C242
// 

P18C242::P18C242()
{

  if(verbose)
    cout << "18c242 constructor, type = " << isa() << '\n';

}

void P18C242::create()
{

  if(verbose)
    cout << " 18c242 create \n";

  P18C2x2::create();

  create_sfr_map();

}

Processor * P18C242::construct()
{

  P18C242 *p = new P18C242;

  if(verbose)
    cout << " 18c242 construct\n";

  p->new_name("p18c242");
  p->create();
  p->create_invalid_registers();
  p->create_sfr_map();
  p->create_symbols();

  symbol_table.add_module(p,p->name().c_str());

  return p;


}

//------------------------------------------------------------------------
//
// P18C252
// 

P18C252::P18C252()
{

  if(verbose)
    cout << "18c252 constructor, type = " << isa() << '\n';

}

void P18C252::create()
{

  if(verbose)
    cout << " 18c252 create \n";

  P18C242::create();


}


Processor * P18C252::construct()
{

  P18C252 *p = new P18C252;

  if(verbose)
    cout << " 18c252 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_sfr_map();
  p->create_symbols();

  p->new_name("p18c252");
  symbol_table.add_module(p,p->name().c_str());

  return p;

}







//========================================================================
//
// Pic 18C4x2
//

void P18C4x2::create()
{
  if(verbose)
    cout << "P18C4x2::create\n";

  create_iopin_map();

  _16bit_processor::create();

}
//------------------------------------------------------------------------
void P18C4x2::create_iopin_map()
{

  package = new Package(40);

  if(!package)
    return;

  // Build the links between the I/O Ports and their tris registers.
  portd.tris = &trisd;
  trisd.port = &portd;

  porte.tris = &trise;
  trise.port = &porte;

  portd.new_name("portd");
  porte.new_name("porte");

  trisd.new_name("trisd");
  trise.new_name("trise");

  // Define the valid I/O pins.
  portd.valid_iopins = 0xff;
  porte.valid_iopins = 0x07;

  package->assign_pin(1, 0); // /MCLR

  package->assign_pin( 2, m_porta->addPin(new IO_bi_directional("porta0"),0));
  package->assign_pin( 3, m_porta->addPin(new IO_bi_directional("porta1"),1));
  package->assign_pin( 4, m_porta->addPin(new IO_bi_directional("porta2"),2));
  package->assign_pin( 5, m_porta->addPin(new IO_bi_directional("porta3"),3));
  package->assign_pin( 6, m_porta->addPin(new IO_open_collector("porta4"),4));
  package->assign_pin( 7, m_porta->addPin(new IO_bi_directional("porta5"),5));

  package->assign_pin(8, new IO_bi_directional(&porte, 0));
  package->assign_pin(9, new IO_bi_directional(&porte, 1));
  package->assign_pin(10, new IO_bi_directional(&porte, 2));


  package->assign_pin(11, 0);
  package->assign_pin(12, 0);
  package->assign_pin(13, 0);
  package->assign_pin(14, m_porta->addPin(new IO_bi_directional("porta6"),6));

  package->assign_pin(15, m_portc->addPin(new IO_bi_directional("portc0"),0));
  package->assign_pin(16, m_portc->addPin(new IO_bi_directional("portc1"),1));
  package->assign_pin(17, m_portc->addPin(new IO_bi_directional("portc2"),2));
  package->assign_pin(18, m_portc->addPin(new IO_bi_directional("portc3"),3));
  package->assign_pin(23, m_portc->addPin(new IO_bi_directional("portc4"),4));
  package->assign_pin(24, m_portc->addPin(new IO_bi_directional("portc5"),5));
  package->assign_pin(25, m_portc->addPin(new IO_bi_directional("portc6"),6));
  package->assign_pin(26, m_portc->addPin(new IO_bi_directional("portc7"),7));

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

  package->assign_pin(33, m_portb->addPin(new IO_bi_directional_pu("portb0"),0));
  package->assign_pin(34, m_portb->addPin(new IO_bi_directional_pu("portb1"),1));
  package->assign_pin(35, m_portb->addPin(new IO_bi_directional_pu("portb2"),2));
  package->assign_pin(36, m_portb->addPin(new IO_bi_directional_pu("portb3"),3));
  package->assign_pin(37, m_portb->addPin(new IO_bi_directional_pu("portb4"),4));
  package->assign_pin(38, m_portb->addPin(new IO_bi_directional_pu("portb5"),5));
  package->assign_pin(39, m_portb->addPin(new IO_bi_directional_pu("portb6"),6));
  package->assign_pin(40, m_portb->addPin(new IO_bi_directional_pu("portb7"),7));


  //1portc.ccp1con = &ccp1con;
  //1portc.usart = &usart16;


}
void P18C4x2::create_symbols()
{
  if(verbose)
    cout << "P18C4x2 create symbols\n";

  _16bit_processor::create_symbols();

  symbol_table.add_ioport(&portd);
  symbol_table.add_ioport(&porte);

}

P18C4x2::P18C4x2()
{

  if(verbose)
    cout << "18c4x2 constructor, type = " << isa() << '\n';


}


void P18C4x2::create_sfr_map()
{

  if(verbose)
    cout << "create_sfr_map P18C4x2\n";

  _16bit_processor::create_sfr_map();

  add_sfr_register(&portd,	  0xf83,RegisterValue(0,0),"portd");
  add_sfr_register(&porte,	  0xf84,RegisterValue(0,0),"porte");

  //1 usart16.initialize_16(this,&pir_set_def,&portc);

  add_sfr_register(&latd,	  0xf8c,RegisterValue(0,0),"latd");
  add_sfr_register(&late,	  0xf8d,RegisterValue(0,0),"late");

  portd.latch = &latd;
  porte.latch = &late;

  latd.port = &portd;
  late.port = &porte;

  add_sfr_register(&trisd,	  0xf95,RegisterValue(0xff,0),"trisd");
  add_sfr_register(&trise,	  0xf96,RegisterValue(0x0f,0),"trise");


}



//------------------------------------------------------------------------
//
// P18C442
// 

P18C442::P18C442()
{

  if(verbose)
    cout << "18c442 constructor, type = " << isa() << '\n';

}

void P18C442::create()
{

  if(verbose)
    cout << " 18c442 create \n";

  P18C4x2::create();

}

Processor * P18C442::construct()
{

  P18C442 *p = new P18C442;

  if(verbose)
    cout << " 18c442 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_sfr_map();
  p->create_symbols();

  p->new_name("p18c442");
  symbol_table.add_module(p,p->name().c_str());

  return p;


}



//------------------------------------------------------------------------
//
// P18C452
// 

P18C452::P18C452()
{

  if(verbose)
    cout << "18c452 constructor, type = " << isa() << '\n';

}

void P18C452::create()
{

  //if(verbose)
    cout << " 18c452 create \n";

  P18C442::create();
}

Processor * P18C452::construct()
{

  P18C452 *p = new P18C452;
  p->new_name("p18c452");

  if(verbose)
    cout << " 18c452 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_sfr_map();
  p->create_symbols();

  symbol_table.add_module(p,p->name().c_str());

  return p;


}


//------------------------------------------------------------------------
//
// P18F242
// 

P18F242::P18F242()
{

  //if(verbose)
    cout << "18f242 constructor, type = " << isa() << '\n';

}

void P18F242::create()
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

}

/*
void P18F242::create_sfr_map()
{

  // Add eeprom
  add_sfr_register(get_eeprom()->get_reg_eedata(), 0xfa8);
  add_sfr_register(get_eeprom()->get_reg_eeadr(), 0xfa9);
  add_sfr_register(get_eeprom()->get_reg_eecon1(), 0xfa6, RegisterValue(0,0));
  add_sfr_register(get_eeprom()->get_reg_eecon2(), 0xfa7);

}
*/

void P18F242::set_out_of_range_pm(unsigned int address, unsigned int value)
{

  if( (address>= 0xf00000) && (address < 0xf00000 +
    get_eeprom()->get_rom_size()))
    {
      get_eeprom()->change_rom(address - 0xf00000, value);
    }
}

Processor * P18F242::construct()
{

  P18F242 *p = new P18F242;
  p->new_name("p18f242");

  if(verbose)
    cout << " 18F242 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_sfr_map();
  p->create_symbols();

  symbol_table.add_module(p,p->name().c_str());

  return p;


}


//------------------------------------------------------------------------
//
// P18F252
// 

P18F252::P18F252()
{

  if(verbose)
    cout << "18f252 constructor, type = " << isa() << '\n';

}

void P18F252::create()
{

  if(verbose)
    cout << " 18f252 create \n";

}
Processor * P18F252::construct()
{

  P18F252 *p = new P18F252;
  p->new_name("p18f252");

  if(verbose)
    cout << " 18F252 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_sfr_map();
  p->create_symbols();

  symbol_table.add_module(p,p->name().c_str());

  return p;


}

//------------------------------------------------------------------------
//
// P18F442
// 

P18F442::P18F442()
{

  //if(verbose)
    cout << "18f442 constructor, type = " << isa() << '\n';

}

void P18F442::create()
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

}
/*
void P18F442::create_sfr_map()
{

  // Add eeprom
  add_sfr_register(get_eeprom()->get_reg_eedata(), 0xfa8);
  add_sfr_register(get_eeprom()->get_reg_eeadr(), 0xfa9);
  add_sfr_register(get_eeprom()->get_reg_eecon1(), 0xfa6, RegisterValue(0,0));
  add_sfr_register(get_eeprom()->get_reg_eecon2(), 0xfa7);

}
*/
void P18F442::set_out_of_range_pm(unsigned int address, unsigned int value)
{

  if( (address>= 0xf00000) && (address < 0xf00000 +
    get_eeprom()->get_rom_size()))
    {
      get_eeprom()->change_rom(address - 0xf00000, value);
    }
}

Processor * P18F442::construct()
{

  P18F442 *p = new P18F442;
  p->new_name("p18f442");

  if(verbose)
    cout << " 18F442 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_sfr_map();
  p->create_symbols();

  symbol_table.add_module(p,p->name().c_str());

  return p;


}


//------------------------------------------------------------------------
//
// P18F258
// 

P18F248::P18F248()
{

  if(verbose)
    cout << "18f248 constructor, type = " << isa() << '\n';

}

void P18F248::create()
{

  if(verbose)
    cout << " 18f248 create \n";

  P18F442::create();
}

Processor * P18F248::construct()
{

  P18F248 *p = new P18F248;
  p->new_name("p18f248");

  if(verbose)
    cout << " 18F248 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_sfr_map();
  p->create_symbols();

  symbol_table.add_module(p,p->name().c_str());

  return p;


}


//------------------------------------------------------------------------
//
// P18F452
// 

P18F452::P18F452()
{

  if(verbose)
    cout << "18f452 constructor, type = " << isa() << '\n';

}

void P18F452::create()
{

  if(verbose)
    cout << " 18f452 create \n";

  P18F442::create();
}

Processor * P18F452::construct()
{

  P18F452 *p = new P18F452;
  p->new_name("p18f452");

  if(verbose)
    cout << " 18F452 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_sfr_map();
  p->create_symbols();

  symbol_table.add_module(p,p->name().c_str());

  return p;


}


//------------------------------------------------------------------------
//
// P18F1220
// 

Processor * P18F1220::construct()
{

  P18F1220 *p = new P18F1220;
  p->new_name("p18f1220");

  if(verbose)
    cout << " 18F1220 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_sfr_map();
  p->create_symbols();

  symbol_table.add_module(p,p->name().c_str());

  return p;
}

void P18F1220::create()
{
  if(verbose)
    cout << "P18F1220::create\n";

  create_iopin_map();

  _16bit_processor::create();

  create_sfr_map();


}
//------------------------------------------------------------------------
void P18F1220::create_iopin_map()
{

  package = new Package(18);

  if(!package)
    return;

  package->assign_pin( 1, m_porta->addPin(new IO_bi_directional("porta0"),0));
  package->assign_pin( 2, m_porta->addPin(new IO_bi_directional("porta1"),1));
  package->assign_pin( 6, m_porta->addPin(new IO_bi_directional("porta2"),2));
  package->assign_pin( 7, m_porta->addPin(new IO_bi_directional("porta3"),3));
  package->assign_pin( 3, m_porta->addPin(new IO_bi_directional("porta4"),4));
  package->assign_pin( 4, m_porta->addPin(new IO_open_collector("porta5"),5));
  package->assign_pin(15, m_porta->addPin(new IO_bi_directional("porta6"),6));
  package->assign_pin(16, m_porta->addPin(new IO_bi_directional("porta7"),7));

  package->assign_pin( 8, m_portb->addPin(new IO_bi_directional_pu("portb0"),0));
  package->assign_pin( 9, m_portb->addPin(new IO_bi_directional_pu("portb1"),1));
  package->assign_pin(17, m_portb->addPin(new IO_bi_directional_pu("portb2"),2));
  package->assign_pin(18, m_portb->addPin(new IO_bi_directional_pu("portb3"),3));
  package->assign_pin(10, m_portb->addPin(new IO_bi_directional_pu("portb4"),4));
  package->assign_pin(11, m_portb->addPin(new IO_bi_directional_pu("portb5"),5));
  package->assign_pin(12, m_portb->addPin(new IO_bi_directional_pu("portb6"),6));
  package->assign_pin(13, m_portb->addPin(new IO_bi_directional_pu("portb7"),7));

  package->assign_pin(5, 0);
  package->assign_pin(14, 0);

}

P18F1220::P18F1220()
{

  if(verbose)
    cout << "18F1220 constructor, type = " << isa() << '\n';


}


//------------------------------------------------------------------------
//
// P18Fx320
// 

P18F1320::P18F1320()
{

  //if(verbose)
    cout << "18f1320 constructor, type = " << isa() << '\n';

}

void P18F1320::create()
{

  //if(verbose)
    cout << " 18fx320 create \n";

  P18F1220::create();


}

Processor * P18F1320::construct()
{

  P18F1320 *p = new P18F1320;

  if(verbose)
    cout << " 18F1320 construct\n";

  p->new_name("p18f1320");
  p->create();
  p->create_invalid_registers();
  p->create_sfr_map();
  p->create_symbols();

  symbol_table.add_module(p,p->name().c_str());

  return p;


}

