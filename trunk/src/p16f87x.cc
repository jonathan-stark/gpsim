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
//    P16F871  P16F873
//    P16F876  P16F873A
//    P14F874A P16F876A
//    P16F877A
//

#include <stdio.h>
#include <iostream>
#include <string>

#include "../config.h"
#include "symbol.h"

#include "p16f87x.h"
#include "pic-ioports.h"
#include "stimuli.h"



void P16F871::set_out_of_range_pm(unsigned int address, unsigned int value)
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
  add_sfr_register(get_eeprom()->get_reg_eecon1(),  0x18c, RegisterValue(0,0));

  // Enable program memory reads and writes.
  get_eeprom()->get_reg_eecon1()->set_bits(EECON1::EEPGD);

  add_sfr_register(get_eeprom()->get_reg_eeadr(),   0x10d);
  add_sfr_register(get_eeprom()->get_reg_eecon2(),  0x18d);

  add_sfr_register(get_eeprom()->get_reg_eedatah(), 0x10e);
  add_sfr_register(get_eeprom()->get_reg_eeadrh(),  0x10f);

  add_sfr_register(&adresl,  0x9e, RegisterValue(0,0));

  adres.new_name("adresh");
  adresl.new_name("adresl");

  adcon0.adresl = &adresl;

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


  adcon1.setValidCfgBits(ADCON1::PCFG0 | ADCON1::PCFG1 | 
			 ADCON1::PCFG2 | ADCON1::PCFG3);

  adcon1.setChannelConfiguration(8, 0xff);
  adcon1.setChannelConfiguration(9, 0x3f);
  adcon1.setChannelConfiguration(10, 0x3f);
  adcon1.setChannelConfiguration(11, 0x3f);
  adcon1.setChannelConfiguration(12, 0x1f);
  adcon1.setChannelConfiguration(13, 0x0f);
  adcon1.setChannelConfiguration(14, 0x01);
  adcon1.setChannelConfiguration(15, 0x0d);
  adcon1.setVrefHiConfiguration(8, 3);
  adcon1.setVrefHiConfiguration(10, 3);
  adcon1.setVrefHiConfiguration(11, 3);
  adcon1.setVrefHiConfiguration(12, 3);
  adcon1.setVrefHiConfiguration(13, 3);
  adcon1.setVrefHiConfiguration(15, 3);

  adcon1.setVrefLoConfiguration(8, 2);
  adcon1.setVrefLoConfiguration(11, 2);
  adcon1.setVrefLoConfiguration(12, 2);
  adcon1.setVrefLoConfiguration(13, 2);
  adcon1.setVrefLoConfiguration(15, 2);

}

void P16F871::create(void)
{
  if(verbose)
    cout << " f871 create \n";

  P16C74::create();

  EEPROM_WIDE *e;
  e = new EEPROM_WIDE;
  e->set_cpu(this);
  e->initialize(128);
  e->set_pir_set(get_pir_set());
  e->set_intcon(&intcon_reg);
  set_eeprom_wide(e);

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

  if(verbose)
    cout << " f871 construct\n";

  p->create();
  p->create_invalid_registers ();

  p->create_symbols();

  p->new_name("p16f871");
  symbol_table.add_module(p,p->name().c_str());

  return p;

}

void P16F871::create_symbols(void)
{

  if(verbose)
    cout << "f871 create symbols\n";

  Pic14Bit::create_symbols();

}

P16F871::P16F871(void)
{
  if(verbose)
    cout << "f871 constructor, type = " << isa() << '\n';
}



//-------------------------------------------------------





void P16F873::set_out_of_range_pm(unsigned int address, unsigned int value)
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
  add_sfr_register(get_eeprom()->get_reg_eecon1(),  0x18c, RegisterValue(0,0));

  // Enable program memory reads and writes.
  get_eeprom()->get_reg_eecon1()->set_bits(EECON1::EEPGD);

  add_sfr_register(get_eeprom()->get_reg_eeadr(),   0x10d);
  add_sfr_register(get_eeprom()->get_reg_eecon2(),  0x18d);

  add_sfr_register(get_eeprom()->get_reg_eedatah(), 0x10e);
  add_sfr_register(get_eeprom()->get_reg_eeadrh(),  0x10f);

  add_sfr_register(&adresl,  0x9e, RegisterValue(0,0));

  adres.new_name("adresh");
  adresl.new_name("adresl");

  adcon0.adresl = &adresl;

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


  adcon1.setValidCfgBits(ADCON1::PCFG0 | ADCON1::PCFG1 | 
			 ADCON1::PCFG2 | ADCON1::PCFG3);

  adcon1.setChannelConfiguration(8, 0x1f);
  adcon1.setChannelConfiguration(9, 0x1f);
  adcon1.setChannelConfiguration(10, 0x1f);
  adcon1.setChannelConfiguration(11, 0x1f);
  adcon1.setChannelConfiguration(12, 0x1f);
  adcon1.setChannelConfiguration(13, 0x0f);
  adcon1.setChannelConfiguration(14, 0x01);
  adcon1.setChannelConfiguration(15, 0x0d);
  adcon1.setVrefHiConfiguration(8, 3);
  adcon1.setVrefHiConfiguration(10, 3);
  adcon1.setVrefHiConfiguration(11, 3);
  adcon1.setVrefHiConfiguration(12, 3);
  adcon1.setVrefHiConfiguration(13, 3);
  adcon1.setVrefHiConfiguration(15, 3);

  adcon1.setVrefLoConfiguration(8, 2);
  adcon1.setVrefLoConfiguration(11, 2);
  adcon1.setVrefLoConfiguration(12, 2);
  adcon1.setVrefLoConfiguration(13, 2);
  adcon1.setVrefLoConfiguration(15, 2);
}

void P16F873::create(void)
{
  if(verbose)
    cout << " f873 create \n";

  P16C73::create();

  EEPROM_WIDE *e;
  e = new EEPROM_WIDE;
  e->set_cpu(this);
  e->initialize(128);
  e->set_pir_set(get_pir_set());
  e->set_intcon(&intcon_reg);
  set_eeprom_wide(e);

  status->rp_mask = 0x60;  // rp0 and rp1 are valid.
  indf->base_address_mask1 = 0x80; // used for indirect accesses above 0x100
  indf->base_address_mask2 = 0x1ff; // used for indirect accesses above 0x100

  P16F873::create_sfr_map();


}



Processor * P16F873::construct(void)
{

  P16F873 *p = new P16F873;

  if(verbose)
    cout << " f873 construct\n";

  p->new_name("p16f873");

  p->create();
  p->create_invalid_registers ();

  p->create_symbols();


  return p;

}

void P16F873::create_symbols(void)
{

  if(verbose)
    cout << "f873 create symbols\n";

  Pic14Bit::create_symbols();

}

P16F873::P16F873(void)
{
  if(verbose)
    cout << "f873 constructor, type = " << isa() << '\n';

}

void P16F873A::create(void)
{
  if(verbose)
    cout << " f873A create \n";

  P16C73::create();

  EEPROM_WIDE *e;
  e = new EEPROM_WIDE;
  e->set_cpu(this);
  e->initialize(128);
  e->set_pir_set(get_pir_set());
  e->set_intcon(&intcon_reg);
  set_eeprom_wide(e);

  status->rp_mask = 0x60;  // rp0 and rp1 are valid.
  indf->base_address_mask1 = 0x80; // used for indirect accesses above 0x100
  indf->base_address_mask2 = 0x1ff; // used for indirect accesses above 0x100

  P16F873A::create_sfr_map();

}

void P16F873A::create_sfr_map(void)
{

  if(verbose)
    cout << "creating f873A registers \n";

  add_sfr_register(get_eeprom()->get_reg_eedata(),  0x10c);
  add_sfr_register(get_eeprom()->get_reg_eecon1(),  0x18c, RegisterValue(0,0));

  // Enable program memory reads and writes.
  get_eeprom()->get_reg_eecon1()->set_bits(EECON1::EEPGD);

  add_sfr_register(get_eeprom()->get_reg_eeadr(),   0x10d);
  add_sfr_register(get_eeprom()->get_reg_eecon2(),  0x18d);

  add_sfr_register(get_eeprom()->get_reg_eedatah(), 0x10e);
  add_sfr_register(get_eeprom()->get_reg_eeadrh(),  0x10f);

  add_sfr_register(&adresl,  0x9e, RegisterValue(0,0));

                                                                                
  // Link the comparator and voltage ref to porta
  comparator.initialize(get_pir_set(), 
	&(*m_porta)[2], &(*m_porta)[0],
        &(*m_porta)[1], &(*m_porta)[2], 
	&(*m_porta)[3], &(*m_porta)[4], &(*m_porta)[5]);

  add_sfr_register(comparator.cmcon, 0x9c, RegisterValue(7,0),"cmcon");
  add_sfr_register(&comparator.vrcon, 0x9d, RegisterValue(0,0),"vrcon");

  adres.new_name("adresh");
  adresl.new_name("adresl");

  adcon0.adresl = &adresl;

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


  adcon1.setValidCfgBits(ADCON1::PCFG0 | ADCON1::PCFG1 | 
			 ADCON1::PCFG2 | ADCON1::PCFG3);

  adcon1.setChannelConfiguration(8, 0x1f);
  adcon1.setChannelConfiguration(9, 0x1f);
  adcon1.setChannelConfiguration(10, 0x1f);
  adcon1.setChannelConfiguration(11, 0x1f);
  adcon1.setChannelConfiguration(12, 0x1f);
  adcon1.setChannelConfiguration(13, 0x0f);
  adcon1.setChannelConfiguration(14, 0x01);
  adcon1.setChannelConfiguration(15, 0x0d);
  adcon1.setVrefHiConfiguration(8, 3);
  adcon1.setVrefHiConfiguration(10, 3);
  adcon1.setVrefHiConfiguration(11, 3);
  adcon1.setVrefHiConfiguration(12, 3);
  adcon1.setVrefHiConfiguration(13, 3);
  adcon1.setVrefHiConfiguration(15, 3);

  adcon1.setVrefLoConfiguration(8, 2);
  adcon1.setVrefLoConfiguration(11, 2);
  adcon1.setVrefLoConfiguration(12, 2);
  adcon1.setVrefLoConfiguration(13, 2);
  adcon1.setVrefLoConfiguration(15, 2);
}
Processor * P16F873A::construct(void)
{

  P16F873A *p = new P16F873A;

  if(verbose)
    cout << " f873A construct\n";

  p->new_name("p16f873A");

  p->create();
  p->create_invalid_registers ();

  p->create_symbols();


  return p;

}
P16F873A::P16F873A(void)
{
  if(verbose)
    cout << "f873A constructor, type = " << isa() << '\n';

}



Processor * P16F876::construct(void)
{

  P16F876 *p = new P16F876;

  if(verbose)
    cout << " f876 construct\n";

  p->new_name("p16f876");

  p->create();
  p->create_invalid_registers ();

  p->create_symbols();

  symbol_table.add_module(p,p->name().c_str());

  return p;

}


void P16F876::create_sfr_map(void)
{

  if(verbose)
    cout << "creating f876 registers \n";

}

void P16F876::create(void)
{

  if(verbose)
    cout << " f876 create \n";

  P16F873::create();

  add_file_registers(0x110, 0x16f, 0);
  add_file_registers(0x190, 0x1ef, 0);
  delete_file_registers(0xf0,0xff);
  alias_file_registers(0x70,0x7f,0x80);
  alias_file_registers(0x70,0x7f,0x100);
  alias_file_registers(0x70,0x7f,0x180);

  P16F876::create_sfr_map();

}
void P16F876::create_symbols(void)
{

  if(verbose)
    cout << "f876 create symbols\n";
  Pic14Bit::create_symbols();
}

P16F876::P16F876(void)
{
  if(verbose)
    cout << "f876 constructor, type = " << isa() << '\n';
}

Processor * P16F876A::construct(void)
{

  P16F876A *p = new P16F876A;

  if(verbose)
    cout << " f876A construct\n";

  p->new_name("p16f876A");

  p->create();
  p->create_invalid_registers ();

  p->create_symbols();

  symbol_table.add_module(p,p->name().c_str());

  return p;

}


void P16F876A::create_sfr_map(void)
{

  if(verbose)
    cout << "creating f876A registers \n";

}

void P16F876A::create(void)
{

  if(verbose)
    cout << " f876A create \n";

  P16F873A::create();

  add_file_registers(0x110, 0x16f, 0);
  add_file_registers(0x190, 0x1ef, 0);
  delete_file_registers(0xf0,0xff);
  alias_file_registers(0x70,0x7f,0x80);
  alias_file_registers(0x70,0x7f,0x100);
  alias_file_registers(0x70,0x7f,0x180);

  P16F876A::create_sfr_map();

}

P16F876A::P16F876A(void)
{
  if(verbose)
    cout << "f876A constructor, type = " << isa() << '\n';
}


//-------------------------------------------------------

void P16F874::set_out_of_range_pm(unsigned int address, unsigned int value)
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
  add_sfr_register(get_eeprom()->get_reg_eecon1(),  0x18c, RegisterValue(0,0));

  // Enable program memory reads and writes.
  get_eeprom()->get_reg_eecon1()->set_bits(EECON1::EEPGD);

  add_sfr_register(get_eeprom()->get_reg_eeadr(),   0x10d);
  add_sfr_register(get_eeprom()->get_reg_eecon2(),  0x18d);

  add_sfr_register(get_eeprom()->get_reg_eedatah(), 0x10e);
  add_sfr_register(get_eeprom()->get_reg_eeadrh(),  0x10f);

  add_sfr_register(&adresl,  0x9e, RegisterValue(0,0));

  adres.new_name("adresh");
  adresl.new_name("adresl");

  adcon0.adresl = &adresl;

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

  adcon1.setValidCfgBits(ADCON1::PCFG0 | ADCON1::PCFG1 | 
			 ADCON1::PCFG2 | ADCON1::PCFG3);

  adcon1.setChannelConfiguration(8, 0xff);
  adcon1.setChannelConfiguration(9, 0x3f);
  adcon1.setChannelConfiguration(10, 0x3f);
  adcon1.setChannelConfiguration(11, 0x3f);
  adcon1.setChannelConfiguration(12, 0x1f);
  adcon1.setChannelConfiguration(13, 0x0f);
  adcon1.setChannelConfiguration(14, 0x01);
  adcon1.setChannelConfiguration(15, 0x0d);
  adcon1.setVrefHiConfiguration(8, 3);
  adcon1.setVrefHiConfiguration(10, 3);
  adcon1.setVrefHiConfiguration(11, 3);
  adcon1.setVrefHiConfiguration(12, 3);
  adcon1.setVrefHiConfiguration(13, 3);
  adcon1.setVrefHiConfiguration(15, 3);

  adcon1.setVrefLoConfiguration(8, 2);
  adcon1.setVrefLoConfiguration(11, 2);
  adcon1.setVrefLoConfiguration(12, 2);
  adcon1.setVrefLoConfiguration(13, 2);
  adcon1.setVrefLoConfiguration(15, 2);


}

void P16F874::create(void)
{
  if(verbose)
    cout << " f874 create \n";

  P16C74::create();

  EEPROM_WIDE *e;
  e = new EEPROM_WIDE;
  e->set_cpu(this);
  e->initialize(128);
  e->set_pir_set(get_pir_set());
  e->set_intcon(&intcon_reg);
  set_eeprom_wide(e);

  status->rp_mask = 0x60;  // rp0 and rp1 are valid.
  indf->base_address_mask1 = 0x80; // used for indirect accesses above 0x100
  indf->base_address_mask2 = 0x1ff; // used for indirect accesses above 0x100

  P16F874::create_sfr_map();


}



Processor * P16F874::construct(void)
{

  P16F874 *p = new P16F874;

  if(verbose)
    cout << " f874 construct\n";

  p->new_name("p16f874");


  p->create();
  p->create_invalid_registers ();

  p->create_symbols();

  return p;

}

void P16F874::create_symbols(void)
{

  if(verbose)
    cout << "f874 create symbols\n";
  Pic14Bit::create_symbols();

}

P16F874::P16F874(void)
{
  if(verbose)
    cout << "f874 constructor, type = " << isa() << '\n';

}


//-------------------------------------------------------

void P16F874A::set_out_of_range_pm(unsigned int address, unsigned int value)
{

  if( (address>= 0x2100) && (address < 0x2100 + get_eeprom()->get_rom_size()))
    {
      get_eeprom()->change_rom(address - 0x2100, value);
    }
}


void P16F874A::create_sfr_map(void)
{
  if(verbose)
    cout << "creating f874A registers \n";

  add_sfr_register(get_eeprom()->get_reg_eedata(),  0x10c);
  add_sfr_register(get_eeprom()->get_reg_eecon1(),  0x18c, RegisterValue(0,0));

  // Enable program memory reads and writes.
  get_eeprom()->get_reg_eecon1()->set_bits(EECON1::EEPGD);

  add_sfr_register(get_eeprom()->get_reg_eeadr(),   0x10d);
  add_sfr_register(get_eeprom()->get_reg_eecon2(),  0x18d);

  add_sfr_register(get_eeprom()->get_reg_eedatah(), 0x10e);
  add_sfr_register(get_eeprom()->get_reg_eeadrh(),  0x10f);

  add_sfr_register(&adresl,  0x9e, RegisterValue(0,0));
                                                                                

  // Link the comparator and voltage ref to porta
  comparator.initialize(get_pir_set(), 
	&(*m_porta)[2], &(*m_porta)[0],
        &(*m_porta)[1], &(*m_porta)[2], 
	&(*m_porta)[3], &(*m_porta)[4], &(*m_porta)[5]);

  add_sfr_register(comparator.cmcon, 0x9c, RegisterValue(7,0),"cmcon");
  add_sfr_register(&comparator.vrcon, 0x9d, RegisterValue(0,0),"vrcon");

  adres.new_name("adresh");
  adresl.new_name("adresl");

  adcon0.adresl = &adresl;

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

  adcon1.setValidCfgBits(ADCON1::PCFG0 | ADCON1::PCFG1 | 
			 ADCON1::PCFG2 | ADCON1::PCFG3);

  adcon1.setChannelConfiguration(8, 0xff);
  adcon1.setChannelConfiguration(9, 0x3f);
  adcon1.setChannelConfiguration(10, 0x3f);
  adcon1.setChannelConfiguration(11, 0x3f);
  adcon1.setChannelConfiguration(12, 0x1f);
  adcon1.setChannelConfiguration(13, 0x0f);
  adcon1.setChannelConfiguration(14, 0x01);
  adcon1.setChannelConfiguration(15, 0x0d);
  adcon1.setVrefHiConfiguration(8, 3);
  adcon1.setVrefHiConfiguration(10, 3);
  adcon1.setVrefHiConfiguration(11, 3);
  adcon1.setVrefHiConfiguration(12, 3);
  adcon1.setVrefHiConfiguration(13, 3);
  adcon1.setVrefHiConfiguration(15, 3);

  adcon1.setVrefLoConfiguration(8, 2);
  adcon1.setVrefLoConfiguration(11, 2);
  adcon1.setVrefLoConfiguration(12, 2);
  adcon1.setVrefLoConfiguration(13, 2);
  adcon1.setVrefLoConfiguration(15, 2);


}

void P16F874A::create(void)
{
  if(verbose)
    cout << " f874A create \n";

  P16C74::create();

  EEPROM_WIDE *e;
  e = new EEPROM_WIDE;
  e->set_cpu(this);
  e->initialize(128);
  e->set_pir_set(get_pir_set());
  e->set_intcon(&intcon_reg);
  set_eeprom_wide(e);

  status->rp_mask = 0x60;  // rp0 and rp1 are valid.
  indf->base_address_mask1 = 0x80; // used for indirect accesses above 0x100
  indf->base_address_mask2 = 0x1ff; // used for indirect accesses above 0x100

  P16F874A::create_sfr_map();


}



Processor * P16F874A::construct(void)
{

  P16F874A *p = new P16F874A;

  if(verbose)
    cout << " f874A construct\n";

  p->new_name("p16f874A");

  p->create();
  p->create_invalid_registers ();

  p->create_symbols();

  return p;

}

void P16F874A::create_symbols(void)
{

  if(verbose)
    cout << "f874A create symbols\n";
  Pic14Bit::create_symbols();

}

P16F874A::P16F874A(void)
{
  if(verbose)
    cout << "f874A constructor, type = " << isa() << '\n';

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

  if(verbose)
    cout << " f877 construct\n";

  p->new_name("p16f877");

  p->create();
  p->create_invalid_registers ();

  p->create_symbols();

  return p;

}

void P16F877::create_symbols(void)
{

  if(verbose)
    cout << "f877 create symbols\n";
  Pic14Bit::create_symbols();

}

P16F877::P16F877(void)
{
  if(verbose)
    cout << "f877 constructor, type = " << isa() << '\n';
}

void P16F877A::create_sfr_map(void)
{

  if(verbose)
    cout << "creating f877A registers \n";

}

void P16F877A::create(void)
{

  if(verbose)
    cout << " f877A create \n";

  P16F874A::create();

  add_file_registers(0x110, 0x16f, 0);
  add_file_registers(0x190, 0x1ef, 0);
  delete_file_registers(0xf0,0xff);
  alias_file_registers(0x70,0x7f,0x80);
  alias_file_registers(0x70,0x7f,0x100);
  alias_file_registers(0x70,0x7f,0x180);

  P16F877A::create_sfr_map();


}



Processor * P16F877A::construct(void)
{

  P16F877A *p = new P16F877A;

  if(verbose)
    cout << " f877A construct\n";
  p->new_name("p16f877A");

  p->create();
  p->create_invalid_registers ();

  p->create_symbols();

  return p;

}

void P16F877A::create_symbols(void)
{

  if(verbose)
    cout << "f877A create symbols\n";
  Pic14Bit::create_symbols();

}

P16F877A::P16F877A(void)
{
  if(verbose)
    cout << "f877A constructor, type = " << isa() << '\n';
}
