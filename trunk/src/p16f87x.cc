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


//
// p16f87x
//
//  This file supports:
//    P16F874 P16F877
//    P16F871
//

#include <stdio.h>
#include <iostream>
#include <string>

#include "../config.h"
#include "symbol.h"

#include "p16f87x.h"
#include "stimuli.h"



void P16F871::set_out_of_range_pm(int address, int value)
{

  if( (address>= 0x2100) && (address < 0x2100 + get_eeprom()->get_rom_size()))
    {
      get_eeprom()->change_rom(address - 0x2100, value);
    }
}


void P16F871::create_sfr_map(void)
{
  if(verbose)
    cout << "creating f871 registers \n";

  add_sfr_register(get_eeprom()->get_reg_eedata(),  0x10c);
  add_sfr_register(get_eeprom()->get_reg_eecon1(),  0x18c, 0);

  // Enable program memory reads and writes.
  get_eeprom()->get_reg_eecon1()->set_bits(EECON1::EEPGD);

  add_sfr_register(get_eeprom()->get_reg_eeadr(),   0x10d);
  add_sfr_register(get_eeprom()->get_reg_eecon2(),  0x18d);

  add_sfr_register(get_eeprom()->get_reg_eedatah(), 0x10e);
  add_sfr_register(get_eeprom()->get_reg_eeadrh(),  0x10f);

  add_sfr_register(&adresl,  0x9e, 0);

  adres.new_name("adresh");
  adresl.new_name("adresl");

  adcon0.adresl = &adresl;
  adcon1.valid_bits |= (ADCON1::PCFG3 | ADCON1::ADFM);

  alias_file_registers(0x80,0x80,0x80);
  alias_file_registers(0x01,0x01,0x100);
  alias_file_registers(0x82,0x84,0x80);
  alias_file_registers(0x06,0x06,0x100);
  alias_file_registers(0x8a,0x8b,0x80);
  alias_file_registers(0x100,0x100,0x80);
  alias_file_registers(0x81,0x81,0x100);
  alias_file_registers(0x102,0x104,0x80);
  alias_file_registers(0x86,0x86,0x100);
  alias_file_registers(0x10a,0x10b,0x80);


  alias_file_registers(0x20,0x7f,0x100);
  alias_file_registers(0xa0,0xbf,0x100);
  alias_file_registers(0x70,0x7f,0x80);
  alias_file_registers(0xf0,0xff,0x100);


  adcon1.Vrefhi_position[8] = 3;
  adcon1.Vrefhi_position[9] = 8;
  adcon1.Vrefhi_position[10] = 3;
  adcon1.Vrefhi_position[11] = 3;
  adcon1.Vrefhi_position[12] = 3;
  adcon1.Vrefhi_position[13] = 3;
  adcon1.Vrefhi_position[14] = 8;
  adcon1.Vrefhi_position[15] = 3;

  adcon1.Vreflo_position[8] = 2;
  adcon1.Vreflo_position[9] = 8;
  adcon1.Vreflo_position[10] = 8;
  adcon1.Vreflo_position[11] = 2;
  adcon1.Vreflo_position[12] = 2;
  adcon1.Vreflo_position[13] = 2;
  adcon1.Vreflo_position[14] = 8;
  adcon1.Vreflo_position[15] = 2;

  adcon1.configuration_bits[8] = 0xff;
  adcon1.configuration_bits[9] = 0x3f;
  adcon1.configuration_bits[10] = 0x3f;
  adcon1.configuration_bits[11] = 0x3f;
  adcon1.configuration_bits[12] = 0x1f;
  adcon1.configuration_bits[13] = 0x0f;
  adcon1.configuration_bits[14] = 0x01;
  adcon1.configuration_bits[15] = 0x0d;

}

void P16F871::create(void)
{
  if(verbose)
    cout << " f871 create \n";

  P16C74::create();

  status->rp_mask = 0x60;  // rp0 and rp1 are valid.
  indf->base_address_mask1 = 0x80; // used for indirect accesses above 0x100
  indf->base_address_mask2 = 0x1ff; // used for indirect accesses above 0x100

  P16F871::create_sfr_map();

  /*
  ((PORTC*)portc)->ssp = 0;
  ((PORTA*)porta)->ssp = 0;
  */

}



Processor * P16F871::construct(void)
{

  P16F871 *p = new P16F871;
  EEPROM_WIDE *e;

  if(verbose)
    cout << " f871 construct\n";

  e = new EEPROM_WIDE;
  e->set_cpu(p);
  e->initialize(64);
  e->set_intcon(&p->intcon_reg);

  // assign the eeprom to the processor
  p->set_eeprom_wide(e);

  p->create();
  p->create_invalid_registers ();

  p->pic_processor::create_symbols();

  p->new_name("p16f871");
  symbol_table.add_module(p,p->name().c_str());

  return p;

}

void P16F871::create_symbols(void)
{

  if(verbose)
    cout << "f871 create symbols\n";

}

P16F871::P16F871(void)
{
  if(verbose)
    cout << "f871 constructor, type = " << isa() << '\n';

  init_ssp = false;

}



//-------------------------------------------------------





void P16F873::set_out_of_range_pm(int address, int value)
{

  if( (address>= 0x2100) && (address < 0x2100 + get_eeprom()->get_rom_size()))
    {
      get_eeprom()->change_rom(address - 0x2100, value);
    }
}


void P16F873::create_sfr_map(void)
{

  if(verbose)
    cout << "creating f873 registers \n";

  add_sfr_register(get_eeprom()->get_reg_eedata(),  0x10c);
  add_sfr_register(get_eeprom()->get_reg_eecon1(),  0x18c, 0);

  // Enable program memory reads and writes.
  get_eeprom()->get_reg_eecon1()->set_bits(EECON1::EEPGD);

  add_sfr_register(get_eeprom()->get_reg_eeadr(),   0x10d);
  add_sfr_register(get_eeprom()->get_reg_eecon2(),  0x18d);

  add_sfr_register(get_eeprom()->get_reg_eedatah(), 0x10e);
  add_sfr_register(get_eeprom()->get_reg_eeadrh(),  0x10f);

  add_sfr_register(&adresl,  0x9e, 0);

  adres.new_name("adresh");
  adresl.new_name("adresl");

  adcon0.adresl = &adresl;
  adcon1.valid_bits |= (ADCON1::PCFG3 | ADCON1::ADFM);

  alias_file_registers(0x80,0x80,0x80);
  alias_file_registers(0x01,0x01,0x100);
  alias_file_registers(0x82,0x84,0x80);
  alias_file_registers(0x06,0x06,0x100);
  alias_file_registers(0x8a,0x8b,0x80);
  alias_file_registers(0x100,0x100,0x80);
  alias_file_registers(0x81,0x81,0x100);
  alias_file_registers(0x102,0x104,0x80);
  alias_file_registers(0x86,0x86,0x100);
  alias_file_registers(0x10a,0x10b,0x80);


  alias_file_registers(0x20,0x7f,0x100);
  alias_file_registers(0xa0,0xff,0x100);


  adcon1.Vrefhi_position[8] = 3;
  adcon1.Vrefhi_position[9] = 8;
  adcon1.Vrefhi_position[10] = 3;
  adcon1.Vrefhi_position[11] = 3;
  adcon1.Vrefhi_position[12] = 3;
  adcon1.Vrefhi_position[13] = 3;
  adcon1.Vrefhi_position[14] = 8;
  adcon1.Vrefhi_position[15] = 3;

  adcon1.Vreflo_position[8] = 2;
  adcon1.Vreflo_position[9] = 8;
  adcon1.Vreflo_position[10] = 8;
  adcon1.Vreflo_position[11] = 2;
  adcon1.Vreflo_position[12] = 2;
  adcon1.Vreflo_position[13] = 2;
  adcon1.Vreflo_position[14] = 8;
  adcon1.Vreflo_position[15] = 2;

  adcon1.configuration_bits[8] = 0xff;
  adcon1.configuration_bits[9] = 0x3f;
  adcon1.configuration_bits[10] = 0x3f;
  adcon1.configuration_bits[11] = 0x3f;
  adcon1.configuration_bits[12] = 0x3f;
  adcon1.configuration_bits[13] = 0x0f;
  adcon1.configuration_bits[14] = 0;
  adcon1.configuration_bits[15] = 0x0d;

}

void P16F873::create(void)
{
  if(verbose)
    cout << " f873 create \n";

  P16C73::create();

  status->rp_mask = 0x60;  // rp0 and rp1 are valid.
  indf->base_address_mask1 = 0x80; // used for indirect accesses above 0x100
  indf->base_address_mask2 = 0x1ff; // used for indirect accesses above 0x100

  P16F873::create_sfr_map();


}



Processor * P16F873::construct(void)
{

  P16F873 *p = new P16F873;
  EEPROM_WIDE *e;

  if(verbose)
    cout << " f873 construct\n";

  e = new EEPROM_WIDE;
  e->set_cpu(p);
  e->initialize(128);
  e->set_intcon(&p->intcon_reg);

  // assign the eeprom to the processor
  p->set_eeprom_wide(e);

  p->create();
  p->create_invalid_registers ();

  p->pic_processor::create_symbols();

  p->ssp.initialize_14(p,p->get_pir_set(),p->portc,3,4,5,p->porta,5,SSP_TYPE_MSSP);

  p->new_name("p16f873");
  symbol_table.add_module(p,p->name().c_str());

  return p;

}

void P16F873::create_symbols(void)
{

  if(verbose)
    cout << "f873 create symbols\n";

  init_ssp = true;
  
}

P16F873::P16F873(void)
{
  if(verbose)
    cout << "f873 constructor, type = " << isa() << '\n';

}

//-------------------------------------------------------

void P16F874::set_out_of_range_pm(int address, int value)
{

  if( (address>= 0x2100) && (address < 0x2100 + get_eeprom()->get_rom_size()))
    {
      get_eeprom()->change_rom(address - 0x2100, value);
    }
}


void P16F874::create_sfr_map(void)
{
  if(verbose)
    cout << "creating f874 registers \n";

  add_sfr_register(get_eeprom()->get_reg_eedata(),  0x10c);
  add_sfr_register(get_eeprom()->get_reg_eecon1(),  0x18c, 0);

  // Enable program memory reads and writes.
  get_eeprom()->get_reg_eecon1()->set_bits(EECON1::EEPGD);

  add_sfr_register(get_eeprom()->get_reg_eeadr(),   0x10d);
  add_sfr_register(get_eeprom()->get_reg_eecon2(),  0x18d);

  add_sfr_register(get_eeprom()->get_reg_eedatah(), 0x10e);
  add_sfr_register(get_eeprom()->get_reg_eeadrh(),  0x10f);

  add_sfr_register(&adresl,  0x9e, 0);

  adres.new_name("adresh");
  adresl.new_name("adresl");

  adcon0.adresl = &adresl;
  adcon1.valid_bits |= (ADCON1::PCFG3 | ADCON1::ADFM);

  alias_file_registers(0x80,0x80,0x80);
  alias_file_registers(0x01,0x01,0x100);
  alias_file_registers(0x82,0x84,0x80);
  alias_file_registers(0x06,0x06,0x100);
  alias_file_registers(0x8a,0x8b,0x80);
  alias_file_registers(0x100,0x100,0x80);
  alias_file_registers(0x81,0x81,0x100);
  alias_file_registers(0x102,0x104,0x80);
  alias_file_registers(0x86,0x86,0x100);
  alias_file_registers(0x10a,0x10b,0x80);


  alias_file_registers(0x20,0x7f,0x100);
  alias_file_registers(0xa0,0xff,0x100);


  adcon1.Vrefhi_position[8] = 3;
  adcon1.Vrefhi_position[9] = 8;
  adcon1.Vrefhi_position[10] = 3;
  adcon1.Vrefhi_position[11] = 3;
  adcon1.Vrefhi_position[12] = 3;
  adcon1.Vrefhi_position[13] = 3;
  adcon1.Vrefhi_position[14] = 8;
  adcon1.Vrefhi_position[15] = 3;

  adcon1.Vreflo_position[8] = 2;
  adcon1.Vreflo_position[9] = 8;
  adcon1.Vreflo_position[10] = 8;
  adcon1.Vreflo_position[11] = 2;
  adcon1.Vreflo_position[12] = 2;
  adcon1.Vreflo_position[13] = 2;
  adcon1.Vreflo_position[14] = 8;
  adcon1.Vreflo_position[15] = 2;

  adcon1.configuration_bits[8] = 0xff;
  adcon1.configuration_bits[9] = 0x3f;
  adcon1.configuration_bits[10] = 0x3f;
  adcon1.configuration_bits[11] = 0x3f;
  adcon1.configuration_bits[12] = 0x3f;
  adcon1.configuration_bits[13] = 0x0f;
  adcon1.configuration_bits[14] = 0;
  adcon1.configuration_bits[15] = 0x0d;

}

void P16F874::create(void)
{
  if(verbose)
    cout << " f874 create \n";

  P16C74::create();

  status->rp_mask = 0x60;  // rp0 and rp1 are valid.
  indf->base_address_mask1 = 0x80; // used for indirect accesses above 0x100
  indf->base_address_mask2 = 0x1ff; // used for indirect accesses above 0x100

  P16F874::create_sfr_map();


}



Processor * P16F874::construct(void)
{

  P16F874 *p = new P16F874;
  EEPROM_WIDE *e;

  if(verbose)
    cout << " f874 construct\n";

  e = new EEPROM_WIDE;
  e->set_cpu(p);
  e->initialize(128);
  e->set_intcon(&p->intcon_reg);

  // assign the eeprom to the processor
  p->set_eeprom_wide(e);

  p->create();
  p->create_invalid_registers ();

  p->pic_processor::create_symbols();

  p->ssp.initialize_14(p,p->get_pir_set(),p->portc,3,4,5,p->porta,5,SSP_TYPE_MSSP);

  p->new_name("p16f874");
  symbol_table.add_module(p,p->name().c_str());

  return p;

}

void P16F874::create_symbols(void)
{

  if(verbose)
    cout << "f874 create symbols\n";

  init_ssp = true;
  
}

P16F874::P16F874(void)
{
  if(verbose)
    cout << "f874 constructor, type = " << isa() << '\n';

}



void P16F877::create_sfr_map(void)
{

  if(verbose)
    cout << "creating f877 registers \n";

}

void P16F877::create(void)
{

  if(verbose)
    cout << " f877 create \n";

  P16F874::create();
  add_file_registers(0x110, 0x16f, 0);
  add_file_registers(0x190, 0x1ef, 0);
  delete_file_registers(0xf0,0xff);
  alias_file_registers(0x70,0x7f,0x80);
  alias_file_registers(0x70,0x7f,0x100);
  alias_file_registers(0x70,0x7f,0x180);

  P16F877::create_sfr_map();


}



Processor * P16F877::construct(void)
{

  P16F877 *p = new P16F877;
  EEPROM_WIDE *e;

  if(verbose)
    cout << " f877 construct\n";

  e = new EEPROM_WIDE;
  e->set_cpu(p);
  e->initialize(256);
  e->set_intcon(&p->intcon_reg);
  p->set_eeprom_wide(e);

  p->create();
  p->create_invalid_registers ();

  p->pic_processor::create_symbols();

  p->ssp.initialize_14(p,p->get_pir_set(),p->portc,3,4,5,p->porta,5,SSP_TYPE_MSSP);

  p->new_name("p16f877");
  symbol_table.add_module(p,p->name().c_str());

  return p;

}

void P16F877::create_symbols(void)
{

  if(verbose)
    cout << "f877 create symbols\n";

}

P16F877::P16F877(void)
{
  if(verbose)
    cout << "f877 constructor, type = " << isa() << '\n';

  init_ssp = true;

}
