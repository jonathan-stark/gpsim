/*
   Copyright (C) 1998 T. Scott Dattalo
   Copyright (C) 2006 Roy R. Rankin

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
// p16f8x
//
//  This file supports:
//    PIC16F87
//    PIC16F88
//

#include <stdio.h>
#include <iostream>
#include <string>

//#include "../config.h"

#include "stimuli.h"

#include "p16f8x.h"
#include "pic-ioports.h"
#include "packages.h"
#include "symbol.h"


//========================================================================
//
// Configuration Memory for the 16F8X devices.

class Config1 : public ConfigMemory 
{
public:
  Config1(P16F8x *pCpu)
    : ConfigMemory("CONFIG1", 0x3fff, "Configuration Word", pCpu, 0x2007)
  {
  }

  enum {
    FOSC0  = 1<<0,
    FOSC1  = 1<<1,
    WDTEN  = 1<<2,
    PWRTEN = 1<<3,

    FOSC2  = 1<<4,
    MCLRE  = 1<<5,
    BOREN  = 1<<6,
    LVP    = 1<<7,

    CPD    = 1<<8,
    WRT0   = 1<<9,
    WRT1   = 1<<10,
    DEBUG  = 1<<11,

    CCPMX  = 1<<12,
    CP     = 1<<13
  };

  virtual void set(gint64 v)
  {
    gint64 oldV = getVal();

    Integer::set(v);
    if (m_pCpu) {

      gint64 diff = oldV ^ v;

      if (diff & WDTEN)
	m_pCpu->wdt.initialize(v&WDTEN == WDTEN);

    }

  }

};


//========================================================================

P16F8x::P16F8x(const char *_name, const char *desc)
  : P16X6X_processor(_name,desc),
    pir1_2_reg(&intcon_reg,&pie1), pir2_2_reg(&intcon_reg,&pie2)
{
   pir1 = &pir1_2_reg;
   pir2 = &pir2_2_reg;
}

void P16F8x::create_iopin_map()
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
  package->assign_pin( 4, m_porta->addPin(new IO_bi_directional("porta5"),5));
  package->assign_pin(15, m_porta->addPin(new IO_bi_directional("porta6"),6));
  package->assign_pin(16, m_porta->addPin(new IO_bi_directional("porta7"),7));

  package->assign_pin(5, 0);  // Vss
  package->assign_pin( 6, m_portb->addPin(new IO_bi_directional_pu("portb0"),0));
  package->assign_pin( 7, m_portb->addPin(new IO_bi_directional_pu("portb1"),1));
  package->assign_pin( 8, m_portb->addPin(new IO_bi_directional_pu("portb2"),2));
  package->assign_pin( 9, m_portb->addPin(new IO_bi_directional_pu("portb3"),3));
  package->assign_pin(10, m_portb->addPin(new IO_bi_directional_pu("portb4"),4));
  package->assign_pin(11, m_portb->addPin(new IO_bi_directional_pu("portb5"),5));
  package->assign_pin(12, m_portb->addPin(new IO_bi_directional_pu("portb6"),6));
  package->assign_pin(13, m_portb->addPin(new IO_bi_directional_pu("portb7"),7));
  package->assign_pin(14, 0);  // Vdd

  if (hasSSP()) {
    ssp.initialize(get_pir_set(),    // PIR
                   &(*m_portb)[4],   // SCK
                   &(*m_portb)[2],   // SDO
                   &(*m_portb)[1],   // SDI
                   &(*m_portb)[5]);  // SS
  }

}

void P16F8x::create_sfr_map()
{
  pir_set_2_def.set_pir1(pir1);
  pir_set_2_def.set_pir2(pir2);
 
  add_file_registers(0xa0, 0xef, 0);
  add_file_registers(0x110,0x16f,0);
  add_file_registers(0x190,0x1ef,0);

  alias_file_registers(0x70,0x7f,0x80);
  alias_file_registers(0x70,0x7f,0x100);
  alias_file_registers(0x70,0x7f,0x180);

  add_sfr_register(get_pir2(),   0x0d, RegisterValue(0,0),"pir2");
  add_sfr_register(&pie2,   0x8d, RegisterValue(0,0));
                                                                                
  pir_set_def.set_pir2(pir2);
                                                                                
  pie2.pir    = get_pir2();
  pie2.new_name("pie2");



  add_sfr_register(indf,   0x180);
  add_sfr_register(indf,   0x100);

  alias_file_registers(0x01,0x04,0x100);
  alias_file_registers(0x81,0x84,0x100);

  add_sfr_register(m_porta, 0x05);
  add_sfr_register(m_trisa, 0x85, RegisterValue(0xff,0));

  add_sfr_register(m_portb, 0x106);
  add_sfr_register(m_trisb, 0x186, RegisterValue(0xff,0));
  add_sfr_register(m_portb, 0x06);
  add_sfr_register(m_trisb, 0x86, RegisterValue(0xff,0));


  add_sfr_register(get_eeprom()->get_reg_eedata(),  0x10c);
  add_sfr_register(get_eeprom()->get_reg_eeadr(),   0x10d);
  add_sfr_register(get_eeprom()->get_reg_eedatah(),  0x10e);
  add_sfr_register(get_eeprom()->get_reg_eeadrh(),   0x10f);
  add_sfr_register(get_eeprom()->get_reg_eecon1(),  0x18c, RegisterValue(0,0));
  add_sfr_register(get_eeprom()->get_reg_eecon2(),  0x18d);

  add_sfr_register(pclath, 0x18a, RegisterValue(0,0));
  add_sfr_register(pclath, 0x10a, RegisterValue(0,0));

  add_sfr_register(&intcon_reg, 0x18b, RegisterValue(0,0));
  add_sfr_register(&intcon_reg, 0x10b, RegisterValue(0,0));
  add_sfr_register(&intcon_reg, 0x08b, RegisterValue(0,0));
  add_sfr_register(&intcon_reg, 0x00b, RegisterValue(0,0));

  usart.initialize(get_pir_set(),&(*m_portb)[2], &(*m_portb)[1],
		   new _TXREG(), new _RCREG());

  add_sfr_register(&usart.rcsta, 0x18, RegisterValue(0,0),"rcsta");
  add_sfr_register(&usart.txsta, 0x98, RegisterValue(2,0),"txsta");
  add_sfr_register(&usart.spbrg, 0x99, RegisterValue(0,0),"spbrg");
  add_sfr_register(usart.txreg,  0x19, RegisterValue(0,0),"txreg");
  add_sfr_register(usart.rcreg,  0x1a, RegisterValue(0,0),"rcreg");

  intcon = &intcon_reg;
  intcon_reg.set_pir_set(get_pir_set());

  // Link the comparator and voltage ref to porta
  comparator.initialize(get_pir_set(), &(*m_porta)[2], &(*m_porta)[0], 
	&(*m_porta)[1], &(*m_porta)[2], &(*m_porta)[3], &(*m_porta)[3],
	&(*m_porta)[4]);

  comparator.cmcon->set_configuration(1, 0, AN0, AN3, AN0, AN3, ZERO);
  comparator.cmcon->set_configuration(2, 0, AN1, AN2, AN1, AN2, ZERO);
  comparator.cmcon->set_configuration(1, 1, AN0, AN2, AN3, AN2, NO_OUT);
  comparator.cmcon->set_configuration(2, 1, AN1, AN2, AN1, AN2, NO_OUT);
  comparator.cmcon->set_configuration(1, 2, AN0, VREF, AN3, VREF, NO_OUT);
  comparator.cmcon->set_configuration(2, 2, AN1, VREF, AN2, VREF, NO_OUT);
  comparator.cmcon->set_configuration(1, 3, AN0, AN2, AN0, AN2, NO_OUT);
  comparator.cmcon->set_configuration(2, 3, AN1, AN2, AN1, AN3, NO_OUT);
  comparator.cmcon->set_configuration(1, 4, AN0, AN3, AN0, AN3, NO_OUT);
  comparator.cmcon->set_configuration(2, 4, AN1, AN2, AN1, AN2, NO_OUT);
  comparator.cmcon->set_configuration(1, 5, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);
  comparator.cmcon->set_configuration(2, 5, AN1, AN2, AN1, AN2, NO_OUT);
  comparator.cmcon->set_configuration(1, 6, AN0, AN2, AN0, AN2, OUT0);
  comparator.cmcon->set_configuration(2, 6, AN1, AN2, AN1, AN2, OUT1);
  comparator.cmcon->set_configuration(1, 7, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);
  comparator.cmcon->set_configuration(2, 7, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);

  add_sfr_register(comparator.cmcon, 0x9c, RegisterValue(7,0),"cmcon");
  add_sfr_register(&comparator.vrcon, 0x9d, RegisterValue(0,0),"cvrcon");

  add_sfr_register(&wdtcon, 0x105, RegisterValue(0,0),"wdtcon");
  add_sfr_register(&osccon, 0x8f, RegisterValue(0,0),"osccon");
  add_sfr_register(&osctune, 0x90, RegisterValue(0,0),"osctune");

  osccon.set_osctune(&osctune);
  osctune.set_osccon(&osccon);
}

void P16F8x::create_symbols()
{
  if(verbose)
    cout << "8x create symbols\n";

  Pic14Bit::create_symbols();
}

void P16F8x::set_out_of_range_pm(unsigned int address, unsigned int value)
{

  if( (address>= 0x2100) && (address < 0x2100 + get_eeprom()->get_rom_size()))
    {
      get_eeprom()->change_rom(address - 0x2100, value);
    }
}

//========================================================================
bool P16F8x::set_config_word(unsigned int address, unsigned int cfg_word)
{
  enum {
    CFG_FOSC0 = 1<<0,
    CFG_FOSC1 = 1<<1,
    CFG_FOSC2 = 1<<4,
    CFG_MCLRE = 1<<5,
    CFG_CCPMX = 1<<12
  };

  // Let the base class do most of the work:

  if (pic_processor::set_config_word(address, cfg_word)) {

    if (verbose)
        cout << "p16f88 0x" << hex << address << " setting config word 0x" << cfg_word << '\n';


    unsigned int valid_pins = m_porta->getEnableMask();

    // Careful these bits not adjacent
    switch(cfg_word & (CFG_FOSC0 | CFG_FOSC1 | CFG_FOSC2)) {

    case 0:  // LP oscillator: low power crystal is on RA6 and RA7
    case 1:     // XT oscillator: crystal/resonator is on RA6 and RA7
    case 2:     // HS oscillator: crystal/resonator is on RA6 and RA7
	(m_porta->getPin(6))->newGUIname("OSC2");
	(m_porta->getPin(7))->newGUIname("OSC1");
	break;

    case 0x13:  // ER oscillator: RA6 is CLKOUT, resistor (?) on RA7 
	(m_porta->getPin(6))->newGUIname("CLKOUT");
	(m_porta->getPin(7))->newGUIname("OSC1");
	break;

    case 3:     // EC:  RA6 is an I/O, RA7 is a CLKIN
    case 0x12:  // ER oscillator: RA6 is an I/O, RA7 is a CLKIN
        (m_porta->getPin(7))->newGUIname("CLKIN");
        valid_pins =  (valid_pins & 0x7f)|0x40;
        break;

    case 0x10:  // INTRC: Internal Oscillator, RA6 and RA7 are I/O's
        valid_pins |= 0xc0;
        break;

    case 0x11:  // INTRC: Internal Oscillator, RA7 is an I/O, RA6 is CLKOUT
        valid_pins = (valid_pins & 0xbf)|0x80;
        break;

    }

    // If the /MCLRE bit is set then RA5 is the MCLR pin, otherwise it's 
    // a general purpose I/O pin.

    if (! (cfg_word & CFG_MCLRE)) 
    {
      valid_pins |= ( 1<< 5); 		// porta5 IO port
    }
    else
    {
	(m_porta->getPin(5))->newGUIname("MCLR");
    }
    if (cfg_word & CFG_CCPMX)
	ccp1con.setIOpin(&((*m_portb)[0]));
    else
	ccp1con.setIOpin(&((*m_portb)[3]));


    //cout << " porta valid_iopins " << porta->valid_iopins << 
    //   "  tris valid io " << trisa.valid_iopins << '\n';

    if (valid_pins != m_porta->getEnableMask()) // enable new pins for IO
    {
        m_porta->setEnableMask(valid_pins);
        m_porta->setTris(m_trisa);
    }
    return true;
  }
  else if (address == 0x2008 )
  {
    cout << "p16f88 0x" << hex << address << " config word 0x" << cfg_word << '\n';
  }

  return false;
}

//========================================================================

void P16F8x::create_config_memory()
{
  m_configMemory = new ConfigMemory *[2];
  m_configMemory[0] = new Config1(this);
  m_configMemory[1] = new ConfigMemory("CONFIG2", 0,"Configuration Word",this,0x2008);
}


//========================================================================
void  P16F8x::create()
{

  create_iopin_map();

  _14bit_processor::create();

  EEPROM_WIDE *e;
  e = new EEPROM_WIDE(pir2);
  e->set_cpu(this);
  e->initialize(128);
  e->set_intcon(&intcon_reg);
  set_eeprom_wide(e);


  P16X6X_processor::create_sfr_map();

  status->rp_mask = 0x60;  // rp0 and rp1 are valid.
  indf->base_address_mask1 = 0x80; // used for indirect accesses above 0x100
  indf->base_address_mask2 = 0x1ff; // used for indirect accesses above 0x100

  P16F8x::create_sfr_map();

}

//========================================================================
//
// Pic 16F87 
//

Processor * P16F87::construct(const char *name)
{

  P16F87 *p = new P16F87(name);

  p->P16F8x::create();
  p->create_invalid_registers ();
  p->create_symbols();
  symbol_table.add_module(p,p->name().c_str());

  return p;

}

P16F87::P16F87(const char *_name, const char *desc)
  : P16F8x(_name,desc)
{
  if(verbose)
    cout << "f87 constructor, type = " << isa() << '\n';

}

//========================================================================
//
// Pic 16F88 
//

Processor * P16F88::construct(const char *name)
{

  P16F88 *p = new P16F88(name);

  p->P16F88::create();
  p->create_invalid_registers ();
  p->create_symbols();
  symbol_table.add_module(p,p->name().c_str());

  return p;

}

P16F88::P16F88(const char *_name, const char *desc)
  : P16F87(_name,desc)
{
  if(verbose)
    cout << "f88 constructor, type = " << isa() << '\n';
}

void  P16F88::create()
{
	P16F8x::create();

        P16F88::create_sfr_map();

}

void P16F88::create_sfr_map()
{

  add_sfr_register(&adresl,  0x9e, RegisterValue(0,0));
  add_sfr_register(&adresh,  0x1e, RegisterValue(0,0));
                                                                                
  add_sfr_register(&adcon0, 0x1f, RegisterValue(0,0));
  add_sfr_register(&adcon1, 0x9f, RegisterValue(0,0));
  add_sfr_register(&ansel, 0x9b, RegisterValue(0,0));
                                                                                
  adresh.new_name("adresh");
  adresl.new_name("adresl");
                                                                                
  ansel.setAdcon1(&adcon1);
  adcon0.setAdresLow(&adresl);
  adcon0.setAdres(&adresh);
  adcon0.setAdcon1(&adcon1);
  adcon0.setIntcon(&intcon_reg);
  adcon0.setA2DBits(10);
  adcon0.pir_set = &pir_set_2_def;
  adcon0.setChannel_Mask(7);
                                                                                
  adcon1.setNumberOfChannels(7);
  adcon1.setIOPin(0, &(*m_porta)[0]);
  adcon1.setIOPin(1, &(*m_porta)[1]);
  adcon1.setIOPin(2, &(*m_porta)[2]);
  adcon1.setIOPin(3, &(*m_porta)[3]);
  adcon1.setIOPin(4, &(*m_porta)[4]);
  adcon1.setIOPin(5, &(*m_portb)[6]);
  adcon1.setIOPin(6, &(*m_portb)[7]);

  adcon1.setVrefHiConfiguration(2, 3);
  adcon1.setVrefHiConfiguration(3, 3);
                                                                                
  adcon1.setVrefLoConfiguration(1, 2);
  adcon1.setVrefLoConfiguration(3, 2);

/* Channel Configuration done dynamiclly based on ansel */
                                                                                
  adcon1.setValidCfgBits(ADCON1::VCFG0 | ADCON1::VCFG1 , 4);

 // Link the A/D converter to the Capture Compare Module
  ccp2con.setADCON(&adcon0);

}

