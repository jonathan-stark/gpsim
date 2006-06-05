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
#include "eeprom.h"

#include "p16x8x.h"
#include "pic-ioports.h"

#include "packages.h"

P16X8X::P16X8X(const char *_name, const char *desc)
  : Pic14Bit(_name,desc)
{
}

void P16X8X::create_sfr_map()
{
  Pic14Bit::create_sfr_map();

  add_sfr_register(get_eeprom()->get_reg_eedata(),  0x08);
  add_sfr_register(get_eeprom()->get_reg_eecon1(),  0x88, RegisterValue(0,0));

  add_sfr_register(get_eeprom()->get_reg_eeadr(),   0x09);
  add_sfr_register(get_eeprom()->get_reg_eecon2(),  0x89);


}

//-------------------------------------------------------------------
void P16X8X::set_out_of_range_pm(unsigned int address, unsigned int value)
{

  if( (address>= 0x2100) && (address < 0x2100 + get_eeprom()->get_rom_size()))
      get_eeprom()->change_rom(address - 0x2100, value);
}

void P16X8X::create_iopin_map()
{

  package = new Package(18);
  if(!package)
    return;

  // Now Create the package and place the I/O pins

  package->assign_pin(17, m_porta->addPin(new IO_bi_directional("porta0"),0));
  package->assign_pin(18, m_porta->addPin(new IO_bi_directional("porta1"),1));
  package->assign_pin( 1, m_porta->addPin(new IO_bi_directional("porta2"),2));
  package->assign_pin( 2, m_porta->addPin(new IO_bi_directional("porta3"),3));
  package->assign_pin( 3, m_porta->addPin(new IO_open_collector("porta4"),4));

  package->assign_pin( 4, 0);
  package->assign_pin( 5, 0);
  package->assign_pin( 6, m_portb->addPin(new IO_bi_directional_pu("portb0"),0));
  package->assign_pin( 7, m_portb->addPin(new IO_bi_directional_pu("portb1"),1));
  package->assign_pin( 8, m_portb->addPin(new IO_bi_directional_pu("portb2"),2));
  package->assign_pin( 9, m_portb->addPin(new IO_bi_directional_pu("portb3"),3));
  package->assign_pin(10, m_portb->addPin(new IO_bi_directional_pu("portb4"),4));
  package->assign_pin(11, m_portb->addPin(new IO_bi_directional_pu("portb5"),5));
  package->assign_pin(12, m_portb->addPin(new IO_bi_directional_pu("portb6"),6));
  package->assign_pin(13, m_portb->addPin(new IO_bi_directional_pu("portb7"),7));
  package->assign_pin(14, 0);
  package->assign_pin(15, 0);
  package->assign_pin(16, 0);
}




void  P16X8X::create(int ram_top)
{
  EEPROM *e;
  create_iopin_map();

  _14bit_processor::create();

  e = new EEPROM;
  e->set_cpu(this);
  e->initialize(EEPROM_SIZE);

  e->set_intcon(&intcon_reg);

  set_eeprom(e);

  add_file_registers(0x0c, ram_top, 0x80);
  P16X8X::create_sfr_map();

}

//========================================================================
//
// Pic 16C84 
//

Processor * P16C84::construct(const char *name)
{

  P16C84 *p = new P16C84(name);

  p->P16X8X::create(0x2f);
  p->create_invalid_registers ();
  p->pic_processor::create_symbols();
  symbol_table.add_module(p,p->name().c_str());

  return p;

}

P16C84::P16C84(const char *_name, const char *desc)
  : P16X8X(_name,desc)
{
}



//========================================================================
//
// Pic 16F84 
//


Processor * P16F84::construct(const char *name)
{

  P16F84 *p = new P16F84(name);

  p->P16X8X::create(0x4f);
  p->create_invalid_registers ();
  p->pic_processor::create_symbols();
  symbol_table.add_module(p,p->name().c_str());

  return p;

}

P16F84::P16F84(const char *_name, const char *desc)
  : P16X8X(_name,desc)
{
}

//========================================================================
//
// Pic 16F83
//

P16F83::P16F83(const char *_name, const char *desc)
  : P16X8X(_name,desc)
{
}

Processor * P16F83::construct(const char *name)
{

  P16F83 *p = new P16F83(name);;

  p->P16X8X::create(0x2f);
  p->create_invalid_registers ();
  p->pic_processor::create_symbols();
  symbol_table.add_module(p,p->name().c_str());

  return p;

}

//========================================================================
P16CR83::P16CR83(const char *_name, const char *desc)
  : P16F83(_name,desc)
{
}

Processor * P16CR83::construct(const char *name)
{
  return 0;

}

//========================================================================
P16CR84::P16CR84(const char *_name, const char *desc)
  : P16F84(_name,desc)
{
}

Processor * P16CR84::construct(const char *name)
{
  return 0;
}

