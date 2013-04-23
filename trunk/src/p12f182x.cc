/*
   Copyright (C) 2013 Roy R. Rankin

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
// p12f182x
//
//  This file supports:
//    PIC12F1822
//    PIC12F1823
//
//Note: unlike most other 12F processors these have extended 14bit instructions

#include <stdio.h>
#include <iostream>
#include <string>

#include "../config.h"
#include "symbol.h"

#include "stimuli.h"
#include "eeprom.h"

#include "p12f182x.h"

#include "pic-ioports.h"

#include "packages.h"


//#define DEBUG
#if defined(DEBUG)
#include "../config.h"
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif


// Does not match any of 3 versions in pir.h, pir.cc
// If required by any other porcessors should be moved there
//
class PIR1v1822 : public PIR
{
public:

  enum {
    TMR1IF  = 1<<0,
    TMR2IF  = 1<<1,
    CCP1IF  = 1<<2,
    SSPIF   = 1<<3,
    TXIF    = 1<<4,
    RCIF    = 1<<5,
    ADIF    = 1<<6,
    TMR1GF  = 1<<7
  };

//------------------------------------------------------------------------

PIR1v1822(Processor *pCpu, const char *pName, const char *pDesc,INTCON *_intcon, PIE *_pie)
  : PIR(pCpu,pName,pDesc,_intcon, _pie,0)
{
  valid_bits = TMR1IF | TMR2IF | CCP1IF | SSPIF | TXIF | RCIF | ADIF | TMR1GF;
  writable_bits = TMR1IF | TMR2IF | CCP1IF | SSPIF | ADIF | TMR1GF;

}
  virtual void set_tmr1if()
  {
    trace.raw(write_trace.get() | value.get());
    value.put(value.get() | TMR1IF);
    if( value.get() & pie->value.get() )
      setPeripheralInterrupt();
  }
  virtual void set_tmr2if()
  {
    trace.raw(write_trace.get() | value.get());
    value.put(value.get() | TMR2IF);
    if( value.get() & pie->value.get() )
      setPeripheralInterrupt();
  }

  void set_sspif(void)
  {
    trace.raw(write_trace.get() | value.get());
    value.put(value.get() | SSPIF);
    if( value.get() & pie->value.get() )
      setPeripheralInterrupt();
  }



};

class PIR2v1822 : public PIR
{
public:

  enum {
    BCLIF   = 1<<3,
    EEIF    = 1<<4,
    C1IF    = 1<<5,
    C2IF    = 1<<6, // not 12f1822
    OSFIF   = 1<<7
  };

//------------------------------------------------------------------------

PIR2v1822(Processor *pCpu, const char *pName, const char *pDesc,INTCON *_intcon, PIE *_pie)
  : PIR(pCpu,pName,pDesc,_intcon, _pie,0)
{
  valid_bits = BCLIF | EEIF | C1IF | OSFIF;
  writable_bits = BCLIF | EEIF | C1IF | OSFIF;

}

  void set_c1if(void)
  {
    trace.raw(write_trace.get() | value.get());
    value.put(value.get() | C1IF);
    if( value.get() & pie->value.get() )
      setPeripheralInterrupt();
  }
  void set_c2if(void)
  {
    trace.raw(write_trace.get() | value.get());
    value.put(value.get() | C2IF);
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
};
//========================================================================


P12F1822::P12F1822(const char *_name, const char *desc)
  : _14bit_e_processor(_name,desc), 
    comparator(this),
    pie1(this,"PIE1", "Peripheral Interrupt Enable"),
    pie2(this,"PIE2", "Peripheral Interrupt Enable"),
    t2con(this, "t2con", "TMR2 Control"),
    pr2(this, "pr2", "TMR2 Period Register"),
    tmr2(this, "tmr2", "TMR2 Register"),
    t1con(this, "t1con", "TMR1 Control"),
    tmr1l(this, "tmr1l", "TMR1 Low"),
    tmr1h(this, "tmr1h", "TMR1 High"),
    ccp1con(this, "ccp1con", "Capture Compare Control"),
    ccpr1l(this, "ccpr1l", "Capture Compare 1 Low"),
    ccpr1h(this, "ccpr1h", "Capture Compare 1 High"),
    ansela(this, "ansela", "Analog Select"),
    adcon0(this,"adcon0", "A2D Control 0"),
    adcon1(this,"adcon1", "A2D Control 1"),
    adresh(this,"adresh", "A2D Result High"),
    adresl(this,"adresl", "A2D Result Low"),
    osccal(this, "osccal", "Oscillator Calibration Register", 0xfc)


{
  m_iocaf = new IOC(this, "iocaf", "Interrupt-On-Change flag Register");
  m_iocap = new IOC(this, "iocap", "Interrupt-On-Change positive edge");
  m_iocan = new IOC(this, "iocan", "Interrupt-On-Change negative edge");
  m_porta = new PicPortIOCRegister(this,"porta","", &intcon_reg, m_iocap, m_iocan, m_iocaf, 8,0x3f);
  m_trisa = new PicTrisRegister(this,"trisa","", m_porta, false, 0x37);
  m_lata  = new PicLatchRegister(this,"lata","",m_porta, 0x37);

  tmr0.set_cpu(this, m_porta, 4, option_reg);
  tmr0.start(0);


  m_wpua = new WPU(this, "wpua", "Weak Pull-up Register", m_porta, 0x37);

  pir1 = new PIR1v1822(this,"pir1","Peripheral Interrupt Register",&intcon_reg, &pie1);
  pir2 = new PIR2v1822(this,"pir2","Peripheral Interrupt Register",&intcon_reg, &pie2);

//  tmr0.set_cpu(this, m_gpio, 4, option_reg); FIX port steer
  tmr0.start(0);

  if(config_modes)
    config_modes->valid_bits = ConfigMode::CM_FOSC0 | ConfigMode::CM_FOSC1 | 
      ConfigMode::CM_FOSC1x | ConfigMode::CM_WDTE | ConfigMode::CM_PWRTE;

}

P12F1822::~P12F1822()
{
    delete e;
}
Processor * P12F1822::construct(const char *name)
{

  P12F1822 *p = new P12F1822(name);

  p->create(0x7f, 256);
  p->create_invalid_registers ();
  p->create_symbols();
  return p;

}

void P12F1822::create_symbols()
{
  pic_processor::create_symbols();
  addSymbol(W);

}
void P12F1822::create_sfr_map()
{

  pir_set_2_def.set_pir1(pir1);
  pir_set_2_def.set_pir2(pir2);


  //add_sfr_register(indf,    0x00);
  add_file_registers(0xa0, 0xbf, 0x00);
  add_sfr_register(m_porta, 0x0c);
  add_sfr_register(pir1,   0x11, RegisterValue(0,0),"pir1");
  add_sfr_register(pir2,   0x12, RegisterValue(0,0),"pir2");
  add_sfr_register(&tmr0,   0x15);

  add_sfr_register(&tmr1l,  0x16, RegisterValue(0,0),"tmr1l");
  add_sfr_register(&tmr1h,  0x17, RegisterValue(0,0),"tmr1h");
  add_sfr_register(&t1con,  0x18, RegisterValue(0,0));

  add_sfr_register(&tmr2,   0x1a, RegisterValue(0,0));
  add_sfr_register(&pr2,    0x1b, RegisterValue(0,0));
  add_sfr_register(&t2con,  0x1c, RegisterValue(0,0));


  add_sfr_register(m_trisa, 0x8c, RegisterValue(0x3f,0));

  add_sfr_register(option_reg,  0x95, RegisterValue(0xff,0));

  intcon = &intcon_reg;
  intcon_reg.set_pir_set(get_pir_set());


  tmr1l.tmrh = &tmr1h;
  tmr1l.t1con = &t1con;
  tmr1l.setInterruptSource(new InterruptSource(pir1, PIR1v1::TMR1IF));
  
  tmr1h.tmrl  = &tmr1l;
  t1con.tmrl  = &tmr1l;

  tmr1l.setIOpin(&(*m_porta)[5]);
  tmr1l.setGatepin(&(*m_porta)[3]);

  add_sfr_register(&pie1,   0x91, RegisterValue(0,0));
  add_sfr_register(&pie2,   0x92, RegisterValue(0,0));
  add_sfr_register(&osccal, 0x99, RegisterValue(0x80,0));
  add_sfr_register(&adresl, 0x9b);
  add_sfr_register(&adresh, 0x9c);
  add_sfr_register(&adcon0, 0x9d, RegisterValue(0x00,0));
  add_sfr_register(&adcon1, 0x9e, RegisterValue(0x00,0));



  add_sfr_register(m_lata, 0x10c);
  add_sfr_register(&ansela, 0x18c, RegisterValue(0x17,0));
  add_sfr_register(get_eeprom()->get_reg_eeadr(),   0x191);
  add_sfr_register(get_eeprom()->get_reg_eeadrh(),   0x192);
  add_sfr_register(get_eeprom()->get_reg_eedata(),  0x193);
  add_sfr_register(get_eeprom()->get_reg_eedatah(),  0x194);
  add_sfr_register(get_eeprom()->get_reg_eecon1(),  0x195, RegisterValue(0,0));
  add_sfr_register(get_eeprom()->get_reg_eecon2(),  0x196);
  add_sfr_register(m_wpua, 0x20c, RegisterValue(0x37,0),"wpua");
  add_sfr_register(&ccpr1l, 0x291, RegisterValue(0,0));
  add_sfr_register(&ccpr1h, 0x292, RegisterValue(0,0));
  add_sfr_register(&ccp1con, 0x293, RegisterValue(0,0));
  add_sfr_register(m_iocap, 0x391, RegisterValue(0,0),"iocap");
  add_sfr_register(m_iocan, 0x392, RegisterValue(0,0),"iocan");
  add_sfr_register(m_iocaf, 0x393, RegisterValue(0,0),"iocaf");


  if (pir1) {
    pir1->set_intcon(&intcon_reg);
    pir1->set_pie(&pie1);
  }
  pie1.setPir(pir1);
  pie2.setPir(pir2);
  t2con.tmr2 = &tmr2;
  tmr2.pir_set   = get_pir_set();
  tmr2.pr2    = &pr2;
  tmr2.t2con  = &t2con;
//  tmr2.add_ccp ( &ccp1con );
//  tmr2.add_ccp ( &ccp2con );
  pr2.tmr2    = &tmr2;

  ansela.setValidBits(0x17);
  ansela.setAdcon1(&adcon1);
  adcon0.setAdresLow(&adresl);
  adcon0.setAdres(&adresh);
  adcon0.setAdcon1(&adcon1);
  adcon0.setIntcon(&intcon_reg);
  adcon0.setA2DBits(10);
  adcon0.setPir(pir1);
  adcon0.setChannel_Mask(0x0f);
  adcon0.setChannel_shift(2);

  adcon1.setAdcon0(&adcon0); 
  adcon1.setNumberOfChannels(5);
  adcon1.setIOPin(0, &(*m_porta)[0]);
  adcon1.setIOPin(1, &(*m_porta)[1]);
  adcon1.setIOPin(2, &(*m_porta)[2]);
  adcon1.setIOPin(4, &(*m_porta)[4]);



  // Link the comparator and voltage ref to porta
  comparator.initialize(get_pir_set(), NULL, 
	&(*m_porta)[0], &(*m_porta)[1], 
	NULL, NULL,
	&(*m_porta)[2], NULL);

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
}

//-------------------------------------------------------------------
void P12F1822::set_out_of_range_pm(unsigned int address, unsigned int value)
{

  if( (address>= 0x2100) && (address < 0x2100 + get_eeprom()->get_rom_size()))
      get_eeprom()->change_rom(address - 0x2100, value);
}

void P12F1822::create_iopin_map()
{

  package = new Package(8);
  if(!package)
    return;

  // Now Create the package and place the I/O pins
  package->assign_pin(7, m_porta->addPin(new IO_bi_directional_pu("porta0"),0));
  package->assign_pin(6, m_porta->addPin(new IO_bi_directional_pu("porta1"),1));
  package->assign_pin(5, m_porta->addPin(new IO_bi_directional_pu("porta2"),2));
  package->assign_pin(4, m_porta->addPin(new IO_bi_directional_pu("porta3"),3));
  package->assign_pin(3, m_porta->addPin(new IO_bi_directional_pu("porta4"),4));
  package->assign_pin(2, m_porta->addPin(new IO_bi_directional_pu("porta5"),5));

  package->assign_pin( 1, 0);	// Vdd
  package->assign_pin( 8, 0);	// Vss


}




void  P12F1822::create(int ram_top, int eeprom_size)
{

  create_iopin_map();

  e = new EEPROM_EXTND(this, pir2);
  set_eeprom(e);

  pic_processor::create();


  e->initialize(eeprom_size, 16, 16, 0x8000);
  e->set_intcon(&intcon_reg);
  e->get_reg_eecon1()->set_valid_bits(0xff);

  add_file_registers(0x20, ram_top, 0x00);
  _14bit_e_processor::create_sfr_map();
  P12F1822::create_sfr_map();

}

//-------------------------------------------------------------------
void P12F1822::enter_sleep()
{
    tmr0.sleep();
    tmr1l.sleep();
    pic_processor::enter_sleep();
}

//-------------------------------------------------------------------
void P12F1822::exit_sleep()
{
    if (m_ActivityState == ePASleeping)
    {
	tmr0.wake();
	tmr1l.wake();
	pic_processor::exit_sleep();
    }
}

//-------------------------------------------------------------------
void P12F1822::option_new_bits_6_7(unsigned int bits)
{
	Dprintf(("P12F1822::option_new_bits_6_7 bits=%x\n", bits));
    m_porta->setIntEdge ( (bits & OPTION_REG::BIT6) == OPTION_REG::BIT6); 
    m_wpua->set_wpu_pu ( (bits & OPTION_REG::BIT7) != OPTION_REG::BIT7); 
}

void P12F1822::oscillator_select(unsigned int mode, bool clkout)
{
    unsigned int mask;
    switch(mode)
    {
    case 0:	//LP oscillator: low power crystal
    case 1:	//XT oscillator: Crystal/resonator 
    case 2:	//HS oscillator: High-speed crystal/resonator
        (m_porta->getPin(4))->newGUIname("OSC2");
        (m_porta->getPin(5))->newGUIname("OSC1");
	mask = 0x0f;

	break;

    case 3:	//EXTRC oscillator External RC circuit connected to CLKIN pin
        (m_porta->getPin(5))->newGUIname("CLKIN");
	mask = 0x1f;
	if(clkout) 
	{
	    (m_porta->getPin(4))->newGUIname("CLKOUT");
	    mask = 0x0f;
        }
	break;

    case 4:	//INTOSC oscillator: I/O function on CLKIN pin
	mask = 0x3f;
	if(clkout) 
	{
	    (m_porta->getPin(4))->newGUIname("CLKOUT");
	    mask = 0x2f;
        }
        (m_porta->getPin(5))->newGUIname((m_porta->getPin(5))->name().c_str());
	break;

    case 5:	//ECL: External Clock, Low-Power mode (0-0.5 MHz): on CLKIN pin
	mask = 0x1f;
	if(clkout) 
	{
	    (m_porta->getPin(4))->newGUIname("CLKOUT");
	    mask = 0x0f;
        }
        (m_porta->getPin(5))->newGUIname("CLKIN");
	break;

    case 6:	//ECM: External Clock, Medium-Power mode (0.5-4 MHz): on CLKIN pin
	mask = 0x1f;
	if(clkout) 
	{
	    (m_porta->getPin(4))->newGUIname("CLKOUT");
	    mask = 0x0f;
        }
        (m_porta->getPin(5))->newGUIname("CLKIN");
	break;

    case 7:	//ECH: External Clock, High-Power mode (4-32 MHz): on CLKIN pin
	mask = 0x1f;
	if(clkout) 
	{
	    (m_porta->getPin(4))->newGUIname("CLKOUT");
	    mask = 0x0f;
        }
        (m_porta->getPin(5))->newGUIname("CLKIN");
	break;
    };
    ansela.setValidBits(0x17 & mask);
    m_porta->setEnableMask(mask);
}
void P12F1822::program_memory_wp(unsigned int mode)
{
	switch(mode)
	{
	case 3:	// no write protect
	    get_eeprom()->set_prog_wp(0x0);
	    break;

	case 2: // write protect 0000-01ff
	    get_eeprom()->set_prog_wp(0x0200);
	    break;

	case 1: // write protect 0000-03ff
	    get_eeprom()->set_prog_wp(0x0400);
	    break;

	case 0: // write protect 0000-07ff
	    get_eeprom()->set_prog_wp(0x0800);
	    break;

	default:
	    printf("%s unexpected mode %d\n", __FUNCTION__, mode);
	    break;
	}

}
