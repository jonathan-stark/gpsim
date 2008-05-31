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
#include "pic-ioports.h"
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

  tmr1l.setIOpin(&(*m_portc)[0]);
  ssp.initialize(&pir_set_def,    // PIR
                &(*m_portc)[3],   // SCK
                &(*m_porta)[5],   // SS
                &(*m_portc)[5],   // SDO
                &(*m_portc)[4],   // SDI
                m_trisc,         // i2c tris port
		SSP_TYPE_MSSP
       );


  //1portc.usart = &usart16;

}
void P18C2x2::create_symbols()
{
  if(verbose)
    cout << "P18C2x2 create symbols\n";

  _16bit_processor::create_symbols();
}

P18C2x2::P18C2x2(const char *_name, const char *desc)
  : _16bit_processor(_name,desc)
{

  if(verbose)
    cout << "18c2x2 constructor, type = " << isa() << '\n';


}


//------------------------------------------------------------------------
//
// P18C242
// 

P18C242::P18C242(const char *_name, const char *desc)
  : P18C2x2(_name,desc)
{

  if(verbose)
    cout << "18c242 constructor, type = " << isa() << '\n';

}

void P18C242::create()
{

  if(verbose)
    cout << " 18c242 create \n";

  P18C2x2::create();

}

Processor * P18C242::construct(const char *name)
{

  P18C242 *p = new P18C242(name);

  if(verbose)
    cout << " 18c242 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();
  return p;


}

//------------------------------------------------------------------------
//
// P18C252
// 

P18C252::P18C252(const char *_name, const char *desc)
  : P18C242(_name,desc)
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


Processor * P18C252::construct(const char *name)
{

  P18C252 *p = new P18C252(name);;

  if(verbose)
    cout << " 18c252 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();
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

  package->assign_pin(1, 0); // /MCLR

  package->assign_pin( 2, m_porta->addPin(new IO_bi_directional("porta0"),0));
  package->assign_pin( 3, m_porta->addPin(new IO_bi_directional("porta1"),1));
  package->assign_pin( 4, m_porta->addPin(new IO_bi_directional("porta2"),2));
  package->assign_pin( 5, m_porta->addPin(new IO_bi_directional("porta3"),3));
  package->assign_pin( 6, m_porta->addPin(new IO_open_collector("porta4"),4));
  package->assign_pin( 7, m_porta->addPin(new IO_bi_directional("porta5"),5));

  package->assign_pin( 8, m_porte->addPin(new IO_bi_directional("porte0"),0));
  package->assign_pin( 9, m_porte->addPin(new IO_bi_directional("porte1"),1));
  package->assign_pin(10, m_porte->addPin(new IO_bi_directional("porte2"),2));


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

  package->assign_pin(19, m_portd->addPin(new IO_bi_directional("portd0"),0));
  package->assign_pin(20, m_portd->addPin(new IO_bi_directional("portd1"),1));
  package->assign_pin(21, m_portd->addPin(new IO_bi_directional("portd2"),2));
  package->assign_pin(22, m_portd->addPin(new IO_bi_directional("portd3"),3));
  package->assign_pin(27, m_portd->addPin(new IO_bi_directional("portd4"),4));
  package->assign_pin(28, m_portd->addPin(new IO_bi_directional("portd5"),5));
  package->assign_pin(29, m_portd->addPin(new IO_bi_directional("portd6"),6));
  package->assign_pin(30, m_portd->addPin(new IO_bi_directional("portd7"),7));

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

  psp.initialize(&pir_set_def,    // PIR
                m_portd,           // Parallel port
                m_trisd,           // Parallel tris
                m_trise,           // Control tris
                &(*m_porte)[0],    // NOT RD
                &(*m_porte)[1],    // NOT WR
                &(*m_porte)[2]);   // NOT CS

  tmr1l.setIOpin(&(*m_portc)[0]);

  ssp.initialize(&pir_set_def,    // PIR
                &(*m_portc)[3],   // SCK
                &(*m_porta)[5],   // SS
                &(*m_portc)[5],   // SDO
                &(*m_portc)[4],   // SDI
                m_trisc,         // i2c tris port
		SSP_TYPE_MSSP
       );
  //1portc.ccp1con = &ccp1con;
  //1portc.usart = &usart16;


}
void P18C4x2::create_symbols()
{
  if(verbose)
    cout << "P18C4x2 create symbols\n";

  _16bit_processor::create_symbols();

}

P18C4x2::P18C4x2(const char *_name, const char *desc)
  : _16bit_processor(_name,desc)
{

  if(verbose)
    cout << "18c4x2 constructor, type = " << isa() << '\n';

  m_portd = new PicPSP_PortRegister(this,"portd","",8,0xff);
  m_trisd = new PicTrisRegister(this,"trisd","", (PicPortRegister *)m_portd, true);
  m_latd  = new PicLatchRegister(this,"latd","",m_portd);

  m_porte = new PicPortRegister(this,"porte","",8,0x07);
  m_trise = new PicPSP_TrisRegister(this,"trise","", m_porte, true);
  m_late  = new PicLatchRegister(this,"late","",m_porte);

}


void P18C4x2::create_sfr_map()
{

  if(verbose)
    cout << "create_sfr_map P18C4x2\n";

  _16bit_processor::create_sfr_map();

  RegisterValue porv(0,0);

  add_sfr_register(m_portd,       0xf83,porv);
  add_sfr_register(m_porte,       0xf84,porv);

  add_sfr_register(m_latd,        0xf8c,porv);
  add_sfr_register(m_late,        0xf8d,porv);

  add_sfr_register(m_trisd,       0xf95,RegisterValue(0xff,0));
  add_sfr_register(m_trise,       0xf96,RegisterValue(0x07,0));

  // rest of configureation in parent class
  adcon1.setNumberOfChannels(8);
  adcon1.setIOPin(5, &(*m_porte)[0]);
  adcon1.setIOPin(6, &(*m_porte)[1]);
  adcon1.setIOPin(7, &(*m_porte)[2]);

  //1 usart16.initialize_16(this,&pir_set_def,&portc);

}



//------------------------------------------------------------------------
//
// P18C442
// 

P18C442::P18C442(const char *_name, const char *desc)
  : P18C4x2(_name,desc)
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

Processor * P18C442::construct(const char *name)
{

  P18C442 *p = new P18C442(name);

  if(verbose)
    cout << " 18c442 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();
  return p;


}



//------------------------------------------------------------------------
//
// P18C452
// 

P18C452::P18C452(const char *_name, const char *desc)
  : P18C442(_name,desc)
{

  if(verbose)
    cout << "18c452 constructor, type = " << isa() << '\n';

}

void P18C452::create()
{

  if(verbose)
    cout << " 18c452 create \n";

  P18C442::create();
}

Processor * P18C452::construct(const char *name)
{

  P18C452 *p = new P18C452(name);

  if(verbose)
    cout << " 18c452 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();
  return p;


}


//------------------------------------------------------------------------
//
// P18F242
// 

P18F242::P18F242(const char *_name, const char *desc)
  : P18C242(_name,desc)
{

  if(verbose)
    cout << "18f242 constructor, type = " << isa() << '\n';

}

void P18F242::create()
{
  EEPROM_PIR *e;

  if(verbose)
    cout << " 18f242 create \n";

  e = new EEPROM_PIR(this,&pir2);

  // We might want to pass this value in for larger eeproms
  e->initialize(256);
  //e->set_pir_set(&pir_set_def);
  e->set_intcon(&intcon);

  // assign this eeprom to the processor
  set_eeprom_pir(e);

  P18C242::create();
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

Processor * P18F242::construct(const char *name)
{

  P18F242 *p = new P18F242(name);

  if(verbose)
    cout << " 18F242 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_sfr_map();
  p->create_symbols();
  return p;


}


//------------------------------------------------------------------------
//
// P18F252
// 

P18F252::P18F252(const char *_name, const char *desc)
  : P18F242(_name,desc)

{

  if(verbose)
    cout << "18f252 constructor, type = " << isa() << '\n';

}

void P18F252::create()
{

  if(verbose)
    cout << " 18f252 create \n";
  P18F242::create();

}
Processor * P18F252::construct(const char *name)
{

  P18F252 *p = new P18F252(name);

  if(verbose)
    cout << " 18F252 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();
  return p;


}

//------------------------------------------------------------------------
//
// P18F442
// 

P18F442::P18F442(const char *_name, const char *desc)
  : P18C442(_name,desc)

{

  if(verbose)
    cout << "18f442 constructor, type = " << isa() << '\n';

}

void P18F442::create()
{
  EEPROM_PIR *e;

  if(verbose)
    cout << " 18f442 create \n";

  e = new EEPROM_PIR(this,&pir2);

  // We might want to pass this value in for larger eeproms
  e->initialize(256);
  //e->set_pir_set(get_pir_set());
  //e->set_pir_set(&pir_set_def);
  e->set_intcon(&intcon);

  // assign this eeprom to the processor
  set_eeprom_pir(e);

  P18C442::create();
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

Processor * P18F442::construct(const char *name)
{

  P18F442 *p = new P18F442(name);

  if(verbose)
    cout << " 18F442 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();
  return p;
}


//------------------------------------------------------------------------
//
// P18F258
// 

P18F248::P18F248(const char *_name, const char *desc)
  : P18F242(_name,desc)
{

  if(verbose)
    cout << "18f248 constructor, type = " << isa() << '\n';

}

void P18F248::create()
{

  if(verbose)
    cout << " 18f248 create \n";

  P18F242::create();
}

Processor * P18F248::construct(const char *name)
{

  P18F248 *p = new P18F248(name);

  if(verbose)
    cout << " 18F248 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();
  return p;
}


//------------------------------------------------------------------------
//
// P18F458
// 

P18F448::P18F448(const char *_name, const char *desc)
  : P18F442(_name,desc)
{

  if(verbose)
    cout << "18f448 constructor, type = " << isa() << '\n';

}

void P18F448::create()
{

  if(verbose)
    cout << " 18f448 create \n";

  P18F442::create();
}

Processor * P18F448::construct(const char *name)
{

  P18F448 *p = new P18F448(name);

  if(verbose)
    cout << " 18F448 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();
  return p;
}


//------------------------------------------------------------------------
//
// P18F452
// 

P18F452::P18F452(const char *_name, const char *desc)
  : P18F442(_name,desc)
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

Processor * P18F452::construct(const char *name)
{

  P18F452 *p = new P18F452(name);

  if(verbose)
    cout << " 18F452 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();
  return p;
}


//------------------------------------------------------------------------
//
// P18F2455
// 

P18F2455::P18F2455(const char *_name, const char *desc)
  : P18F442(_name,desc)

{

  if(verbose)
    cout << "18f2455 constructor, type = " << isa() << '\n';

  m_trisc = new PicTrisRegister(this,"trisc","", (PicPortRegister *)m_portc, true);
}

void P18F2455::create()
{
  P18F442::create();

  if(verbose)
    cout << " 18f2455 create \n";

  package->destroy_pin(18);
  package->assign_pin(18, 0);          // Vusb

  /* The MSSP/I2CC pins are different on this chip. */
  ssp.initialize(&pir_set_def,         // PIR
                &(*m_portb)[1],                // SCK
                &(*m_porta)[5],                // SS
               &(*m_portc)[7],         // SDO
                &(*m_portb)[0],                // SDI
               m_trisb,                // i2c tris port
               SSP_TYPE_MSSP
       );

}

Processor * P18F2455::construct(const char *name)
{

  P18F2455 *p = new P18F2455(name);

  if(verbose)
    cout << " 18F2455 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();
  return p;
}


//------------------------------------------------------------------------

P18Fxx20::P18Fxx20(const char *_name, const char *desc)
  : _16bit_processor(_name,desc)
{
}

//------------------------------------------------------------------------
//
// P18F1220
// 

Processor * P18F1220::construct(const char *name)
{

  P18F1220 *p = new P18F1220(name);

  if(verbose)
    cout << " 18F1220 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();
  return p;
}

void P18F1220::create()
{
  if(verbose)
    cout << "P18F1220::create\n";

  create_iopin_map();

  _16bit_processor::create();

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

P18F1220::P18F1220(const char *_name, const char *desc)
  : P18Fxx20(_name,desc)
{

  if(verbose)
    cout << "18F1220 constructor, type = " << isa() << '\n';


}


//------------------------------------------------------------------------
//
// P18Fx320
// 

P18F1320::P18F1320(const char *_name, const char *desc)
  : P18F1220(_name,desc)
{

  if(verbose)
    cout << "18f1320 constructor, type = " << isa() << '\n';

}

void P18F1320::create()
{

  if(verbose)
    cout << " 18fx320 create \n";

  P18F1220::create();


}

Processor * P18F1320::construct(const char *name)
{

  P18F1320 *p = new P18F1320(name);

  if(verbose)
    cout << " 18F1320 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();
  return p;
}







//========================================================================
//
// Pic 18C2x21
//

void P18F2x21::create()
{
  if(verbose)
    cout << "P18F2x21::create\n";

  create_iopin_map();

  _16bit_processor::create();

}

//------------------------------------------------------------------------
void P18F2x21::create_iopin_map()
{
  package = new Package(28);

  if(!package)
    return;

  // Build the links between the I/O Ports and their tris registers.

  package->assign_pin( 1, m_porte->addPin(new IO_bi_directional("porte3"),3));

  package->assign_pin( 2, m_porta->addPin(new IO_bi_directional("porta0"),0));
  package->assign_pin( 3, m_porta->addPin(new IO_bi_directional("porta1"),1));
  package->assign_pin( 4, m_porta->addPin(new IO_bi_directional("porta2"),2));
  package->assign_pin( 5, m_porta->addPin(new IO_bi_directional("porta3"),3));
  package->assign_pin( 6, m_porta->addPin(new IO_open_collector("porta4"),4));
  package->assign_pin( 7, m_porta->addPin(new IO_bi_directional("porta5"),5));



  package->assign_pin(8, 0);  // Vss
  package->assign_pin(9, m_porta->addPin(new IO_bi_directional("porta7"),7));  // OSC1

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

  tmr1l.setIOpin(&(*m_portc)[0]);
  ssp.initialize(&pir_set_def,    // PIR
                &(*m_portc)[3],   // SCK
                &(*m_porta)[5],   // SS
                &(*m_portc)[5],   // SDO
                &(*m_portc)[4],   // SDI
                m_trisc,         // i2c tris port
		SSP_TYPE_MSSP
       );


  //1portc.usart = &usart16;
}

// Missing :
//      OSCTUNE at 0xF9B
//      SPBRGH  at 0xFB0
//      BAUDCON at 0xFB8
//      ADCON2  at 0xFC0

void P18F2x21::create_symbols()
{
  if(verbose)
    cout << "P18F2x21 create symbols\n";

  _16bit_processor::create_symbols();
}

P18F2x21::P18F2x21(const char *_name, const char *desc)
  : _16bit_processor(_name,desc),
//    osctune(this, "osctune", "OSC Tune"),
    comparator(this)
{

  if(verbose)
    cout << "18c2x21 constructor, type = " << isa() << '\n';

  m_porte = new PicPortRegister(this,"porte","",8,0x08);
//  m_trise = new PicPSP_TrisRegister(this,"trise","", m_porte, true);
//  m_late  = new PicLatchRegister(this,"late","",m_porte);

}

void P18F2x21::create_sfr_map()
{

  if(verbose)
    cout << "create_sfr_map P18F2x21\n";

  _16bit_processor::create_sfr_map();

  RegisterValue porv(0,0);

  add_sfr_register(m_porte,       0xf84,porv);

//  add_sfr_register(&osctune,      0xf9b,porv);

  // rest of configuration in parent class
  adcon1.setNumberOfChannels(12);
  adcon1.setIOPin(8, &(*m_portb)[2]);
  adcon1.setIOPin(9, &(*m_portb)[3]);
  adcon1.setIOPin(10, &(*m_portb)[1]);
  adcon1.setIOPin(11, &(*m_portb)[4]);
  adcon1.setIOPin(12, &(*m_portb)[0]);

  // Link the comparator and voltage ref to porta
  comparator.initialize(&pir_set_def, &(*m_porta)[2], &(*m_porta)[0], 
	&(*m_porta)[1], &(*m_porta)[2], &(*m_porta)[3], &(*m_porta)[3],
	&(*m_porta)[4]);

  comparator.cmcon.set_configuration(1, 0, AN0, AN3, AN0, AN3, ZERO);
  comparator.cmcon.set_configuration(2, 0, AN1, AN2, AN1, AN2, ZERO);
  comparator.cmcon.set_configuration(1, 1, AN0, AN3, AN0, AN3, OUT0);
  comparator.cmcon.set_configuration(2, 1, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);
  comparator.cmcon.set_configuration(1, 2, AN0, AN3, AN0, AN3, NO_OUT);
  comparator.cmcon.set_configuration(2, 2, AN1, AN2, AN1, AN2, NO_OUT);
  comparator.cmcon.set_configuration(1, 3, AN0, AN3, AN0, AN3, OUT0);
  comparator.cmcon.set_configuration(2, 3, AN1, AN2, AN1, AN2, OUT1);
  comparator.cmcon.set_configuration(1, 4, AN0, AN3, AN0, AN3, NO_OUT);
  comparator.cmcon.set_configuration(2, 4, AN1, AN3, AN1, AN3, NO_OUT);
  comparator.cmcon.set_configuration(1, 5, AN0, AN3, AN0, AN3, OUT0);
  comparator.cmcon.set_configuration(2, 5, AN1, AN3, AN1, AN3, OUT1);
  comparator.cmcon.set_configuration(1, 6, AN0, VREF, AN3, VREF, NO_OUT);
  comparator.cmcon.set_configuration(2, 6, AN1, VREF, AN2, VREF, NO_OUT);
  comparator.cmcon.set_configuration(1, 7, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);
  comparator.cmcon.set_configuration(2, 7, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);

  add_sfr_register(&comparator.cmcon, 0xfb4, RegisterValue(7,0),"cmcon");
  add_sfr_register(&comparator.vrcon, 0xfb5, RegisterValue(0,0),"cvrcon");

  //1 usart16.initialize_16(this,&pir_set_def,&portc);

}

void P18F2x21::set_out_of_range_pm(unsigned int address, unsigned int value)
{

  if( (address>= 0xf00000) && (address < 0xf00000 +
    get_eeprom()->get_rom_size()))
    {
      get_eeprom()->change_rom(address - 0xf00000, value);
    }
}


//------------------------------------------------------------------------
//
// P18F2321
// 

P18F2321::P18F2321(const char *_name, const char *desc)
  : P18F2x21(_name,desc)
{

  if(verbose)
    cout << "18F2321 constructor, type = " << isa() << '\n';

}

void P18F2321::create()
{
  EEPROM_PIR *e;

  if(verbose)
    cout << " 18F2321 create \n";

  e = new EEPROM_PIR(this,&pir2);

  // We might want to pass this value in for larger eeproms
  e->initialize(256);
  //e->set_pir_set(&pir_set_def);
  e->set_intcon(&intcon);

  // assign this eeprom to the processor
  set_eeprom_pir(e);

  P18F2x21::create();

}

Processor * P18F2321::construct(const char *name)
{

  P18F2321 *p = new P18F2321(name);

  if(verbose)
    cout << " 18F2321 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();

  if(verbose&2)
    cout << " 18F2321 construct completed\n";
  return p;
}

