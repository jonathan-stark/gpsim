/*
   Copyright (C) 2009 Roy R. Rankin

This file is part of the libgpsim library of gpsim

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see 
<http://www.gnu.org/licenses/lgpl-2.1.html>.
*/


//
// p12f6xx
//
//  This file supports:
//    PIC12F629
//    PIC12F675
//    PIC12F683
//
//Note: unlike most other 12F processors these have 14bit instructions

#include <stdio.h>
#include <iostream>
#include <string>

#include "../config.h"
#include "symbol.h"

#include "stimuli.h"
#include "eeprom.h"

#include "p12f6xx.h"
#include "pic-ioports.h"

#include "packages.h"


//#define DEBUG
#if defined(DEBUG)
#include "../config.h"
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

//========================================================================
//
// Configuration Memory for the 12F6XX devices.

class Config12F6 : public ConfigWord
{
public:
  Config12F6(P12F629 *pCpu)
    : ConfigWord("CONFIG12F6", 0x3fff, "Configuration Word", pCpu, 0x2007)
  {
    Dprintf(("Config12F6::Config12F6 %p\n", m_pCpu));
    if (m_pCpu) 
    {
	m_pCpu->set_config_word(0x2007, 0x3fff);
    }
  }

};

// Does not match any of 3 versions in pir.h, pir.cc
// If required by any other porcessors should be moved there
//
class PIR1v12f : public PIR
{
public:

  enum {
	TMR1IF 	= 1 << 0,
	CMIF	= 1 << 3,
	ADIF	= 1 << 6,
	EEIF	= 1 << 7
  };

//------------------------------------------------------------------------

PIR1v12f(Processor *pCpu, const char *pName, const char *pDesc,INTCON *_intcon, PIE *_pie)
  : PIR(pCpu,pName,pDesc,_intcon, _pie,0)
{
  valid_bits = TMR1IF | CMIF | ADIF | EEIF;
  writable_bits = TMR1IF | CMIF | ADIF | EEIF;

}
   virtual void set_tmr1if()
  {
    put(get() | TMR1IF);
  }
  virtual void set_cmif()
  {
    trace.raw(write_trace.get() | value.get());
    value.put(value.get() | CMIF);
    if( value.get() & pie->value.get() )
      setPeripheralInterrupt();
  }

  virtual void set_eeif()
  {
    trace.raw(write_trace.get() | value.get());
    value.put(value.get() | EEIF);
    if( value.get() & pie->value.get() )
      setPeripheralInterrupt();
  }

 virtual void set_adif()
  {
    trace.raw(write_trace.get() | value.get());
    value.put(value.get() | ADIF);
    if( value.get() & pie->value.get() )
      setPeripheralInterrupt();
  }
};

//========================================================================

void P12F629::create_config_memory()
{
  m_configMemory = new ConfigMemory(this,1);
  m_configMemory->addConfigWord(0,new Config12F6(this));

};

class MCLRPinMonitor;

bool P12F629::set_config_word(unsigned int address,unsigned int cfg_word)
{
  enum {
    FOSC0  = 1<<0,
    FOSC1  = 1<<1,
    FOSC2  = 1<<2,
    WDTEN  = 1<<3,
    PWRTEN = 1<<4,
    MCLRE  = 1<<5,
    BOREN  = 1<<6,
    CP     = 1<<7,
    CPD    = 1<<8,

    BG0    = 1<<12,
    BG1    = 1<<13
  };


    if(address == config_word_address())
    {
        if ((cfg_word & MCLRE) == MCLRE)
	    assignMCLRPin(4);	// package pin 4
        else
	    unassignMCLRPin();
     
 	wdt.initialize((cfg_word & WDTEN) == WDTEN);
        if ((cfg_word & (FOSC2 | FOSC1 )) == 0x04) // internal RC OSC
	  osccal.set_freq(4e6);
       

	return(_14bit_processor::set_config_word(address, cfg_word));
	
    }
    return false;
}

P12F629::P12F629(const char *_name, const char *desc)
  : _14bit_processor(_name,desc), 
    intcon_reg(this,"intcon","Interrupt Control"),
    comparator(this),
    pie1(this,"PIE1", "Peripheral Interrupt Enable"),
    t1con(this, "t1con", "TMR1 Control"),
    tmr1l(this, "tmr1l", "TMR1 Low"),
    tmr1h(this, "tmr1h", "TMR1 High"),
    pcon(this, "pcon", "pcon"),
    osccal(this, "osccal", "Oscillator Calibration Register", 0xfc)

{
  m_ioc = new IOC(this, "ioc", "Interrupt-On-Change GPIO Register");
  m_gpio = new PicPortGRegister(this,"gpio","", &intcon_reg, m_ioc,8,0x3f);
  m_trisio = new PicTrisRegister(this,"trisio","", m_gpio, false);

  m_wpu = new WPU(this, "wpu", "Weak Pull-up Register", m_gpio, 0x37);

  pir1 = new PIR1v12f(this,"pir1","Peripheral Interrupt Register",&intcon_reg, &pie1);

  tmr0.set_cpu(this, m_gpio, 4, option_reg);
  tmr0.start(0);

  if(config_modes)
    config_modes->valid_bits = ConfigMode::CM_FOSC0 | ConfigMode::CM_FOSC1 | 
      ConfigMode::CM_FOSC1x | ConfigMode::CM_WDTE | ConfigMode::CM_PWRTE;

}

P12F629::~P12F629()
{
    delete e;
}
Processor * P12F629::construct(const char *name)
{

  P12F629 *p = new P12F629(name);

  p->create(0x5f, 128);
  p->create_invalid_registers ();
  p->create_symbols();
  return p;

}

void P12F629::create_symbols()
{
  pic_processor::create_symbols();
  addSymbol(W);

}
void P12F629::create_sfr_map()
{
  pir_set_def.set_pir1(pir1);

  add_sfr_register(indf,    0x00);
  alias_file_registers(0x00,0x00,0x80);

  add_sfr_register(&tmr0,   0x01);
  add_sfr_register(option_reg,  0x81, RegisterValue(0xff,0));

  add_sfr_register(pcl,     0x02, RegisterValue(0,0));
  add_sfr_register(status,  0x03, RegisterValue(0x18,0));
  add_sfr_register(fsr,     0x04);
  alias_file_registers(0x02,0x04,0x80);

  add_sfr_register(&tmr1l,  0x0e, RegisterValue(0,0),"tmr1l");
  add_sfr_register(&tmr1h,  0x0f, RegisterValue(0,0),"tmr1h");

  add_sfr_register(&pcon,   0x8e, RegisterValue(0,0),"pcon");

  add_sfr_register(&t1con,  0x10, RegisterValue(0,0));

  add_sfr_register(m_gpio, 0x05);
  add_sfr_register(m_trisio, 0x85, RegisterValue(0x3f,0));


  add_sfr_register(pclath,  0x0a, RegisterValue(0,0));

  add_sfr_register(&intcon_reg, 0x0b, RegisterValue(0,0));
  alias_file_registers(0x0a,0x0b,0x80);

  intcon = &intcon_reg;
  intcon_reg.set_pir_set(get_pir_set());

  add_sfr_register(pir1,   0x0c, RegisterValue(0,0),"pir1");

  tmr1l.tmrh = &tmr1h;
  tmr1l.t1con = &t1con;
  tmr1l.setInterruptSource(new InterruptSource(pir1, PIR1v1::TMR1IF));
  
  tmr1h.tmrl  = &tmr1l;
  t1con.tmrl  = &tmr1l;

  tmr1l.setIOpin(&(*m_gpio)[5]);
  tmr1l.setGatepin(&(*m_gpio)[4]);

  add_sfr_register(&pie1,   0x8c, RegisterValue(0,0));
  if (pir1) {
    pir1->set_intcon(&intcon_reg);
    pir1->set_pie(&pie1);
  }
  pie1.setPir(pir1);

  // Link the comparator and voltage ref to porta
  comparator.initialize(get_pir_set(), NULL, 
	&(*m_gpio)[0], &(*m_gpio)[1], 
	NULL, NULL,
	&(*m_gpio)[2], NULL);

  comparator.cmcon.set_configuration(1, 0, AN0, AN1, AN0, AN1, ZERO);
  comparator.cmcon.set_configuration(1, 1, AN0, AN1, AN0, AN1, OUT0);
  comparator.cmcon.set_configuration(1, 2, AN0, AN1, AN0, AN1, NO_OUT);
  comparator.cmcon.set_configuration(1, 3, AN1, VREF, AN1, VREF, OUT0);
  comparator.cmcon.set_configuration(1, 4, AN1, VREF, AN1, VREF, NO_OUT);
  comparator.cmcon.set_configuration(1, 5, AN1, VREF, AN0, VREF, OUT0);
  comparator.cmcon.set_configuration(1, 6, AN1, VREF, AN0, VREF, NO_OUT);
  comparator.cmcon.set_configuration(1, 7, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);
  comparator.cmcon.set_configuration(2, 0, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);
  comparator.cmcon.set_configuration(2, 1, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);
  comparator.cmcon.set_configuration(2, 2, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);
  comparator.cmcon.set_configuration(2, 3, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);
  comparator.cmcon.set_configuration(2, 4, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);
  comparator.cmcon.set_configuration(2, 5, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);
  comparator.cmcon.set_configuration(2, 6, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);
  comparator.cmcon.set_configuration(2, 7, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);

  add_sfr_register(&comparator.cmcon, 0x19, RegisterValue(0,0),"cmcon");
  add_sfr_register(&comparator.vrcon, 0x99, RegisterValue(0,0),"cvrcon");

  add_sfr_register(get_eeprom()->get_reg_eedata(),  0x9a);
  add_sfr_register(get_eeprom()->get_reg_eeadr(),   0x9b);
  add_sfr_register(get_eeprom()->get_reg_eecon1(),  0x9c, RegisterValue(0,0));
  add_sfr_register(get_eeprom()->get_reg_eecon2(),  0x9d);
  add_sfr_register(m_wpu, 0x95, RegisterValue(0x37,0),"wpu");
  add_sfr_register(m_ioc, 0x96, RegisterValue(0,0),"ioc");
  add_sfr_register(&osccal, 0x90, RegisterValue(0x80,0));


}

//-------------------------------------------------------------------
void P12F629::set_out_of_range_pm(unsigned int address, unsigned int value)
{

  if( (address>= 0x2100) && (address < 0x2100 + get_eeprom()->get_rom_size()))
      get_eeprom()->change_rom(address - 0x2100, value);
}

void P12F629::create_iopin_map()
{

  package = new Package(8);
  if(!package)
    return;

  // Now Create the package and place the I/O pins

  package->assign_pin( 7, m_gpio->addPin(new IO_bi_directional_pu("gpio0"),0));
  package->assign_pin( 6, m_gpio->addPin(new IO_bi_directional_pu("gpio1"),1));
  package->assign_pin( 5, m_gpio->addPin(new IO_bi_directional_pu("gpio2"),2));
  package->assign_pin( 4, m_gpio->addPin(new IOPIN("gpio3"),3));
  package->assign_pin( 3, m_gpio->addPin(new IO_bi_directional_pu("gpio4"),4));
  package->assign_pin( 2, m_gpio->addPin(new IO_bi_directional_pu("gpio5"),5));

  package->assign_pin( 1, 0);
  package->assign_pin( 8, 0);


}




void  P12F629::create(int ram_top, int eeprom_size)
{

  create_iopin_map();

  _14bit_processor::create();


  e = new EEPROM_PIR(this, pir1);
  e->initialize(eeprom_size);
  e->set_intcon(&intcon_reg);
  set_eeprom(e);

  add_file_registers(0x20, ram_top, 0x80);
  P12F629::create_sfr_map();




}

//-------------------------------------------------------------------
void P12F629::enter_sleep()
{
	tmr1l.sleep();
	_14bit_processor::enter_sleep();
}

//-------------------------------------------------------------------
void P12F629::exit_sleep()
{
	tmr1l.wake();
	_14bit_processor::exit_sleep();
}

//-------------------------------------------------------------------
void P12F629::option_new_bits_6_7(unsigned int bits)
{
	Dprintf(("P12F629::option_new_bits_6_7 bits=%x\n", bits));
  m_gpio->setIntEdge ( (bits & OPTION_REG::BIT6) != OPTION_REG::BIT6); 
  m_wpu->set_wpu_pu ( (bits & OPTION_REG::BIT7) != OPTION_REG::BIT7); 
}
//========================================================================
//
// Pic 16F675
//

Processor * P12F675::construct(const char *name)
{

  P12F675 *p = new P12F675(name);

  p->create(0x5f, 128);
  p->create_invalid_registers ();
  p->create_symbols();
  return p;

}

P12F675::P12F675(const char *_name, const char *desc)
  : P12F629(_name,desc),
    ansel(this,"ansel", "Analog Select"),
    adcon0(this,"adcon0", "A2D Control 0"),
    adcon1(this,"adcon1", "A2D Control 1"),
    adresh(this,"adresh", "A2D Result High"),
    adresl(this,"adresl", "A2D Result Low")
{
}

void  P12F675::create(int ram_top, int eeprom_size)
{
  P12F629::create(ram_top, eeprom_size);
  create_sfr_map();
}


void P12F675::create_sfr_map()
{
  //
  //  adcon1 is not a register on the 12f675, but it is used internally
  //  to perform the ADC conversions
  //
  add_sfr_register(&adresl,  0x9e, RegisterValue(0,0));
  add_sfr_register(&adresh,  0x1e, RegisterValue(0,0));

  add_sfr_register(&adcon0, 0x1f, RegisterValue(0,0));
  add_sfr_register(&ansel, 0x9f, RegisterValue(0x0f,0));


  ansel.setAdcon1(&adcon1);
  ansel.setAdcon0(&adcon0);
  adcon0.setAdresLow(&adresl);
  adcon0.setAdres(&adresh);
  adcon0.setAdcon1(&adcon1);
  adcon0.setIntcon(&intcon_reg);
  adcon0.setA2DBits(10);
  adcon0.setPir(pir1);
  adcon0.setChannel_Mask(3);
  adcon0.setChannel_shift(2);

  adcon1.setNumberOfChannels(4);

  adcon1.setIOPin(0, &(*m_gpio)[0]);
  adcon1.setIOPin(1, &(*m_gpio)[1]);
  adcon1.setIOPin(2, &(*m_gpio)[2]);
  adcon1.setIOPin(3, &(*m_gpio)[4]);

  adcon1.setVrefHiConfiguration(2, 1);

/* Channel Configuration done dynamiclly based on ansel */

   adcon1.setValidCfgBits(ADCON1::VCFG0 | ADCON1::VCFG1 , 4);


}
//========================================================================
//
// Pic 16F683
//

Processor * P12F683::construct(const char *name)
{

  P12F683 *p = new P12F683(name);

  p->create(0x7f, 256);
  p->create_invalid_registers ();
  p->create_symbols();
  return p;

}

P12F683::P12F683(const char *_name, const char *desc)
  : P12F675(_name,desc),
   t2con(this, "t2con", "TMR2 Control"),
    pr2(this, "pr2", "TMR2 Period Register"),
    tmr2(this, "tmr2", "TMR2 Register"),
    ccp1con(this, "ccp1con", "Capture Compare Control"),
    ccpr1l(this, "ccpr1l", "Capture Compare 1 Low"),
    ccpr1h(this, "ccpr1h", "Capture Compare 1 High"),
    wdtcon(this, "wdtcon", "WDT Control", 0x1f),
    osccon(this, "osccon", "OSC Control"),
    osctune(this, "osctune", "OSC Tune")

{
}

void  P12F683::create(int ram_top, int eeprom_size)
{
  P12F629::create(0, eeprom_size);

  add_file_registers(0x20, 0x6f, 0);
  add_file_registers(0xa0, 0xbf, 0);
  add_file_registers(0x70, 0x7f, 0x80);

  create_sfr_map();
}


void P12F683::create_sfr_map()
{
  P12F675::create_sfr_map();

  add_sfr_register(&tmr2,   0x11, RegisterValue(0,0));
  add_sfr_register(&t2con,  0x12, RegisterValue(0,0));
  add_sfr_register(&pr2,    0x92, RegisterValue(0xff,0));

  add_sfr_register(&ccpr1l,  0x13, RegisterValue(0,0));
  add_sfr_register(&ccpr1h,  0x14, RegisterValue(0,0));
  add_sfr_register(&ccp1con, 0x15, RegisterValue(0,0));
  add_sfr_register(&wdtcon, 0x18, RegisterValue(0x08,0),"wdtcon");
  add_sfr_register(&osccon, 0x8f, RegisterValue(0,0),"osccon");
  add_sfr_register(&osctune, 0x90, RegisterValue(0,0),"osctune");

  osccon.set_osctune(&osctune);
  osctune.set_osccon(&osccon);





  t2con.tmr2  = &tmr2;
  tmr2.pir_set   = get_pir_set();
  tmr2.pr2    = &pr2;
  tmr2.t2con  = &t2con;
  tmr2.ccp1con = &ccp1con;
  pr2.tmr2    = &tmr2;


  ccp1con.setCrosslinks(&ccpr1l, pir1, &tmr2);
  ccp1con.setIOpin(&((*m_gpio)[2]));
  ccpr1l.ccprh  = &ccpr1h;
  ccpr1l.tmrl   = &tmr1l;
  ccpr1h.ccprl  = &ccpr1l;

  comparator.cmcon.new_name("cmcon0");
  comparator.cmcon.set_tmrl(&tmr1l);
  comparator.cmcon1.set_tmrl(&tmr1l);
  add_sfr_register(&comparator.cmcon1, 0x1a, RegisterValue(2,0),"cmcon1");
  wdt.set_timeout(1./31000.);

}
