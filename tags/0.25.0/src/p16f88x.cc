/*
 *
   Copyright (C) 2010 Roy R. Rankin

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
// p16f88x
//
//  This file supports:
//    PIC16F882
//    PIC16F883
//    PIC16F884
//    PIC16F885
//    PIC16F886
//    PIC16F887
//

#include <stdio.h>
#include <iostream>
#include <string>

//#include "../config.h"

#include "stimuli.h"

#include "p16f88x.h"
#include "pic-ioports.h"
#include "packages.h"
#include "symbol.h"

//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d-%s() ",__FILE__,__LINE__, __FUNCTION__); printf arg; }
#else
#define Dprintf(arg) {}
#endif


//========================================================================
//
// Configuration Memory for the 16F8X devices.

class Config188x : public ConfigWord
{
public:
  Config188x(P16F88x *pCpu)
    : ConfigWord("CONFIG188x", 0x3fff, "Configuration Word", pCpu, 0x2007)
  {
    if (m_pCpu) 
    {
	m_pCpu->wdt.initialize(true); // default WDT enabled
        m_pCpu->wdt.set_timeout(0.000035);
	m_pCpu->set_config_word(0x2007, 0x3fff);
    }
  }

  enum {
    FOSC0  = 1<<0,
    FOSC1  = 1<<1,
    FOSC2  = 1<<2,
    WDTEN  = 1<<3,
    PWRTEN = 1<<4,
    MCLRE  = 1<<5,

    BOREN  = 1<<8,
    BOREN1  = 1<<9,
    LVP    = 1<<12,

    CPD    = 1<<8,
    WRT0   = 1<<9,
    WRT1   = 1<<10,
    NOT_DEBUG  = 1<<11,

  };

  virtual void set(gint64 v)
  {

    Integer::set(v);
    Dprintf(("Config188x set %x\n", (int)v));
    if (m_pCpu) 
    {
	m_pCpu->wdt.initialize((v & WDTEN) == WDTEN);
    }

  }

};


//========================================================================

P16F88x::P16F88x(const char *_name, const char *desc)
  : _14bit_processor(_name,desc),
    intcon_reg(this,"intcon","Interrupt Control"),
    t1con(this, "t1con", "TMR1 Control"),
    pie1(this,"PIE1", "Peripheral Interrupt Enable"),
    pie2(this,"PIE2", "Peripheral Interrupt Enable"),
    t2con(this, "t2con", "TMR2 Control"),
    pr2(this, "pr2", "TMR2 Period Register"),
    tmr2(this, "tmr2", "TMR2 Register"),
    tmr1l(this, "tmr1l", "TMR1 Low"),
    tmr1h(this, "tmr1h", "TMR1 High"),
    ccp1con(this, "ccp1con", "Capture Compare Control"),
    ccpr1l(this, "ccpr1l", "Capture Compare 1 Low"),
    ccpr1h(this, "ccpr1h", "Capture Compare 1 High"),
    ccp2con(this, "ccp2con", "Capture Compare Control"),
    ccpr2l(this, "ccpr2l", "Capture Compare 2 Low"),
    ccpr2h(this, "ccpr2h", "Capture Compare 2 High"),
    pcon(this, "pcon", "pcon"),
    ssp(this),
    osccon(this, "osccon", "OSC Control"),
    osctune(this, "osctune", "OSC Tune"),
    wdtcon(this, "wdtcon", "WDT Control", 1),
    usart(this),
    cm1con0(this, "cm1con0", "Comparator 1 Control Register"),
    cm2con0(this, "cm2con0", "Comparator 2 Control Register"),
    cm2con1(this, "cm2con1", "Comparator 2 Control Register"),
    vrcon(this, "vrcon", "Voltage Reference Control Register"),
    srcon(this, "srcon", "SR Latch Control Resgister"),
    ansel(this,"ansel", "Analog Select"),
    anselh(this,"anselh", "Analog Select high"),
    adcon0(this,"adcon0", "A2D Control 0"),
    adcon1(this,"adcon1", "A2D Control 1"),
    eccpas(this, "eccpas", "ECCP Auto-Shutdown Control Register"),
    pwm1con(this, "pwm1con", "Enhanced PWM Control Register"),
    pstrcon(this, "pstrcon", "Pulse Sterring Control Register"),
    adresh(this,"adresh", "A2D Result High"),
    adresl(this,"adresl", "A2D Result Low")
{

  m_porta = new PicPortRegister(this,"porta","", 8,0x1f);
  m_trisa = new PicTrisRegister(this,"trisa","", m_porta, false);
  m_ioc = new IOC(this, "iocb", "Interrupt-On-Change B Register");
  m_portb = new PicPortGRegister(this,"portb","", &intcon_reg, m_ioc,8,0xff);
  m_trisb = new PicTrisRegister(this,"trisb","", m_portb, false);
  m_portc = new PicPortRegister(this,"portc","",8,0xff);
  m_trisc = new PicTrisRegister(this,"trisc","",m_portc, false);
  m_porte = new PicPortRegister(this,"porte","",8,0x0f);
  m_trise =  new PicPSP_TrisRegister(this,"trise","",m_porte, false);

  pir1_2_reg = new PIR1v2(this,"pir1","Peripheral Interrupt Register",&intcon_reg,&pie1);
  pir2_2_reg = new PIR2v3(this,"pir2","Peripheral Interrupt Register",&intcon_reg,&pie2);
  pir1 = pir1_2_reg;
  pir2 = pir2_2_reg;
  m_wpu = new WPU(this, "wpub", "Weak Pull-up Register", m_portb, 0xff);

  tmr0.set_cpu(this, m_porta, 4, option_reg);
  tmr0.start(0);

}

P16F88x::~P16F88x()
{
  delete_file_registers(0xa0, 0xbf);
}

void P16F88x::create_iopin_map()
{
  fprintf(stderr, "%s should be defined at a higer level\n", __FUNCTION__);
}

void P16F88x::create_sfr_map()
{
  add_sfr_register(indf,    0x00);
  alias_file_registers(0x00,0x00,0x80);

  add_sfr_register(&tmr0,   0x01);
  add_sfr_register(option_reg,  0x81, RegisterValue(0xff,0));

  add_sfr_register(pcl,     0x02, RegisterValue(0,0));
  add_sfr_register(status,  0x03, RegisterValue(0x18,0));
  add_sfr_register(fsr,     0x04);
  alias_file_registers(0x02,0x04,0x80);

  add_sfr_register(m_porta, 0x05);
  add_sfr_register(m_trisa, 0x85, RegisterValue(0x3f,0));

  add_sfr_register(m_portb, 0x06);
  add_sfr_register(m_trisb, 0x86, RegisterValue(0xff,0));

  add_sfr_register(pclath,  0x0a, RegisterValue(0,0));

  add_sfr_register(&intcon_reg, 0x0b, RegisterValue(0,0));
  alias_file_registers(0x0a,0x0b,0x80);

  intcon = &intcon_reg;


  pir_set_2_def.set_pir1(pir1);
  pir_set_2_def.set_pir2(pir2);
 

  add_sfr_register(m_porte, 0x09);
  add_sfr_register(m_trise, 0x89, RegisterValue(0xff,0));
  add_sfr_register(m_portc, 0x07);
  add_sfr_register(m_trisc, 0x87, RegisterValue(0xff,0));

  add_file_registers(0x20, 0x7f, 0);
  add_file_registers(0xa0, 0xbf, 0);

  alias_file_registers(0x70,0x7f,0x80);
  alias_file_registers(0x70,0x7f,0x100);
  alias_file_registers(0x70,0x7f,0x180);

  add_sfr_register(get_pir2(),   0x0d, RegisterValue(0,0),"pir2");
  add_sfr_register(&pie2,   0x8d, RegisterValue(0,0));
                                                                                
  pir_set_2_def.set_pir2(pir2);
                                                                                
  pie2.setPir(get_pir2());
  alias_file_registers(0x00,0x04,0x100);
  alias_file_registers(0x80,0x84,0x100);

  add_sfr_register(m_porta, 0x05);
  add_sfr_register(m_trisa, 0x85, RegisterValue(0xff,0));

  add_sfr_register(m_portb, 0x06);
  alias_file_registers(0x06,0x06,0x100);
  add_sfr_register(m_trisb, 0x86, RegisterValue(0xff,0));
  alias_file_registers(0x86,0x86,0x100);

  add_sfr_register(pir1,   0x0c, RegisterValue(0,0),"pir1");
  add_sfr_register(&pie1,   0x8c, RegisterValue(0,0));

  add_sfr_register(&tmr1l,  0x0e, RegisterValue(0,0),"tmr1l");
  add_sfr_register(&tmr1h,  0x0f, RegisterValue(0,0),"tmr1h");

  add_sfr_register(&pcon,   0x8e, RegisterValue(0,0),"pcon");

  add_sfr_register(&t1con,  0x10, RegisterValue(0,0));
  add_sfr_register(&tmr2,   0x11, RegisterValue(0,0));
  add_sfr_register(&t2con,  0x12, RegisterValue(0,0));
  add_sfr_register(&pr2,    0x92, RegisterValue(0xff,0));


  add_sfr_register(get_eeprom()->get_reg_eedata(),  0x10c);
  add_sfr_register(get_eeprom()->get_reg_eeadr(),   0x10d);
  add_sfr_register(get_eeprom()->get_reg_eedatah(),  0x10e);
  add_sfr_register(get_eeprom()->get_reg_eeadrh(),   0x10f);
  add_sfr_register(get_eeprom()->get_reg_eecon1(),  0x18c, RegisterValue(0,0));
  add_sfr_register(get_eeprom()->get_reg_eecon2(),  0x18d);

  add_sfr_register(&intcon_reg, 0x00b, RegisterValue(0,0));

  alias_file_registers(0x0a,0x0b,0x080);
  alias_file_registers(0x0a,0x0b,0x100);
  alias_file_registers(0x0a,0x0b,0x180);



  intcon = &intcon_reg;
  intcon_reg.set_pir_set(get_pir_set());


  add_sfr_register(&osccon, 0x8f, RegisterValue(0,0),"osccon");
  add_sfr_register(&osctune, 0x90, RegisterValue(0,0),"osctune");

  osccon.set_osctune(&osctune);
  osctune.set_osccon(&osccon);

  usart.initialize(get_pir_set(),&(*m_portc)[6], &(*m_portc)[7],
		   new _TXREG(this,"txreg", "USART Transmit Register", &usart), 
                   new _RCREG(this,"rcreg", "USART Receiver Register", &usart));

  add_sfr_register(&usart.rcsta, 0x18, RegisterValue(0,0),"rcsta");
  add_sfr_register(&usart.txsta, 0x98, RegisterValue(2,0),"txsta");
  add_sfr_register(&usart.spbrg, 0x99, RegisterValue(0,0),"spbrg");
  add_sfr_register(&usart.spbrgh, 0x9a, RegisterValue(0,0),"spbrgh");
  add_sfr_register(&usart.baudcon,  0x187,RegisterValue(0x40,0),"baudctl");
  add_sfr_register(usart.txreg,  0x19, RegisterValue(0,0),"txreg");
  add_sfr_register(usart.rcreg,  0x1a, RegisterValue(0,0),"rcreg");
  usart.set_eusart(true);
  cm1con0.setpins( &(*m_porta)[0], &(*m_porta)[1], &(*m_portb)[3],
	&(*m_portb)[1], &(*m_porta)[3], &(*m_porta)[4]);
  cm2con0.setpins( &(*m_porta)[0], &(*m_porta)[1], &(*m_portb)[3],
	&(*m_portb)[1], &(*m_porta)[2], &(*m_porta)[5]);
  cm1con0.link_registers(get_pir_set(), &cm2con1, &vrcon, &srcon, &eccpas);
  cm2con0.link_registers(get_pir_set(), &cm2con1, &vrcon, &srcon, &eccpas);
  cm2con0.set_tmrl(&tmr1l);
  cm2con1.link_cm12con0(&cm1con0, &cm2con0);
  add_sfr_register(&cm1con0, 0x107, RegisterValue(0,0),"cm1con0");
  add_sfr_register(&cm2con0, 0x108, RegisterValue(0,0),"cm2con0");
  add_sfr_register(&cm2con1, 0x109, RegisterValue(0,0),"cm2con1");
  add_sfr_register(&vrcon, 0x97, RegisterValue(0,0),"vrcon");
  add_sfr_register(&srcon, 0x185, RegisterValue(0,0),"srcon");

  add_sfr_register(&wdtcon, 0x105, RegisterValue(0x08,0),"wdtcon");

  add_sfr_register(&ccpr2l, 0x1b, RegisterValue(0,0));
  add_sfr_register(&ccpr2h, 0x1c, RegisterValue(0,0));
  add_sfr_register(&ccp2con, 0x1d, RegisterValue(0,0));

  add_sfr_register(&adresl,  0x9e, RegisterValue(0,0));
  add_sfr_register(&adresh,  0x1e, RegisterValue(0,0));
                                                                                
  add_sfr_register(&ansel, 0x188, RegisterValue(0xff,0));
  add_sfr_register(&anselh, 0x189, RegisterValue(0xff,0));
  add_sfr_register(&adcon0, 0x1f, RegisterValue(0,0));
  add_sfr_register(&adcon1, 0x9f, RegisterValue(0,0));
  add_sfr_register(m_wpu, 0x95, RegisterValue(0xff,0));
  add_sfr_register(m_ioc, 0x96, RegisterValue(0,0));
                                                                                
  ansel.setAdcon1(&adcon1);
  ansel.setAnselh(&anselh);
  anselh.setAdcon1(&adcon1);
  anselh.setAnsel(&ansel);
  adcon0.setAdresLow(&adresl);
  adcon0.setAdres(&adresh);
  adcon0.setAdcon1(&adcon1);
  adcon0.setIntcon(&intcon_reg);
  adcon0.setA2DBits(10);
  adcon0.setPir(pir1);
  adcon0.setChannel_Mask(0xf);
  adcon0.setChannel_shift(2);
  adcon0.setGo(1);
                               
  adcon1.setValidBits(0xb0);
  adcon1.setNumberOfChannels(14);
  adcon1.setValidCfgBits(ADCON1::VCFG0 | ADCON1::VCFG1 , 4);
  adcon1.setIOPin(0, &(*m_porta)[0]);
  adcon1.setIOPin(1, &(*m_porta)[1]);
  adcon1.setIOPin(2, &(*m_porta)[2]);
  adcon1.setIOPin(3, &(*m_porta)[3]);
  adcon1.setIOPin(4, &(*m_porta)[4]);

  adcon1.setIOPin(8, &(*m_portb)[2]);
  adcon1.setIOPin(9, &(*m_portb)[3]);
  adcon1.setIOPin(10, &(*m_portb)[1]);
  adcon1.setIOPin(11, &(*m_portb)[4]);
  adcon1.setIOPin(12, &(*m_portb)[0]);
  adcon1.setIOPin(13, &(*m_portb)[5]);


  // set a2d modes where an3 is Vref+ 
  adcon1.setVrefHiConfiguration(1, 3);
  adcon1.setVrefHiConfiguration(3, 3);

  // set a2d modes where an2 is Vref-
  adcon1.setVrefLoConfiguration(2, 2);
  adcon1.setVrefLoConfiguration(3, 2);

  vrcon.setValidBits(0xff); // All bits settable

  add_sfr_register(&ccpr1l, 0x15, RegisterValue(0,0));
  add_sfr_register(&ccpr1h, 0x16, RegisterValue(0,0));
  add_sfr_register(&ccp1con, 0x17, RegisterValue(0,0));
  add_sfr_register(&ccpr2l, 0x1b, RegisterValue(0,0));
  add_sfr_register(&ccpr2h, 0x1c, RegisterValue(0,0));
  add_sfr_register(&ccp2con, 0x1d, RegisterValue(0,0));
  add_sfr_register(&pwm1con, 0x9b, RegisterValue(0,0));
  add_sfr_register(&pstrcon, 0x9d, RegisterValue(1,0));
  add_sfr_register(&eccpas, 0x9c, RegisterValue(0,0));
  eccpas.setIOpin(0, 0, &(*m_portb)[0]);
  eccpas.link_registers(&pwm1con, &ccp1con);
  add_sfr_register(&ssp.sspcon2,  0x91, RegisterValue(0,0) ,"sspcon2");
  if (hasSSP()) {
    add_sfr_register(&ssp.sspbuf,  0x13, RegisterValue(0,0),"sspbuf");
    add_sfr_register(&ssp.sspcon,  0x14, RegisterValue(0,0),"sspcon");
    add_sfr_register(&ssp.sspadd,  0x93, RegisterValue(0,0),"sspadd");
    add_sfr_register(&ssp.sspstat, 0x94, RegisterValue(0,0),"sspstat");
    tmr2.ssp_module = &ssp;

    ssp.initialize(
		get_pir_set(),    // PIR
                &(*m_portc)[3],   // SCK
                &(*m_porta)[5],   // SS
                &(*m_portc)[5],   // SDO
                &(*m_portc)[4],    // SDI
                m_trisc,          // i2c tris port
		SSP_TYPE_SSP
	);
  }
  tmr1l.tmrh = &tmr1h;
  tmr1l.t1con = &t1con;
  tmr1h.tmrl  = &tmr1l;

  t1con.tmrl  = &tmr1l;

  t2con.tmr2  = &tmr2;
  tmr2.pir_set   = get_pir_set();
  tmr2.pr2    = &pr2;
  tmr2.t2con  = &t2con;
  tmr2.ccp1con = &ccp1con;
  tmr2.ccp2con = &ccp2con;
  pr2.tmr2    = &tmr2;

  tmr1l.setIOpin(&(*m_portc)[0]);
  ccp1con.setBitMask(0xff);
  ccp1con.pstrcon = &pstrcon;
  ccp1con.pwm1con = &pwm1con;
  ccp1con.setCrosslinks(&ccpr1l, pir1, &tmr2, &eccpas);
  ccpr1l.ccprh  = &ccpr1h;
  ccpr1l.tmrl   = &tmr1l;
  ccpr1h.ccprl  = &ccpr1l;

  ccp2con.setIOpin(&(*m_portc)[1]);
  ccp2con.setCrosslinks(&ccpr2l, pir2, &tmr2);
  ccpr2l.ccprh  = &ccpr2h;
  ccpr2l.tmrl   = &tmr1l;
  ccpr2h.ccprl  = &ccpr2l;

  if (pir1) {
    pir1->set_intcon(&intcon_reg);
    pir1->set_pie(&pie1);
  }
  pie1.setPir(pir1);

}

//-------------------------------------------------------------------
void P16F88x::option_new_bits_6_7(unsigned int bits)
{
        Dprintf(("P18F88x::option_new_bits_6_7 bits=%x\n", bits));
        m_portb->setIntEdge ( (bits & OPTION_REG::BIT6) != OPTION_REG::BIT6);
        m_wpu->set_wpu_pu ( (bits & OPTION_REG::BIT7) != OPTION_REG::BIT7);
}

void P16F88x::create_symbols()
{
  if(verbose)
    cout << "88x create symbols\n";

  pic_processor::create_symbols();
  addSymbol(W);
}

void P16F88x::set_out_of_range_pm(unsigned int address, unsigned int value)
{

  if( (address>= 0x2100) && (address < 0x2100 + get_eeprom()->get_rom_size()))
    {
      get_eeprom()->change_rom(address - 0x2100, value);
    }
}

//========================================================================
bool P16F88x::set_config_word(unsigned int address, unsigned int cfg_word)
{
  enum {
    CFG_FOSC0 = 1<<0,
    CFG_FOSC1 = 1<<1,
    CFG_FOSC2 = 1<<4,
    CFG_MCLRE = 1<<5,
    CFG_CCPMX = 1<<12
  };

  // Let the base class do most of the work:

  if (address == 0x2007)
  {
    pic_processor::set_config_word(address, cfg_word);

    if (verbose)
        cout << "p16f88 0x" << hex << address << " setting config word 0x" << cfg_word << '\n';


    unsigned int valid_pins = m_porta->getEnableMask();

    set_int_osc(false);
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
        (m_porta->getPin(6))->newGUIname("porta6");
        (m_porta->getPin(7))->newGUIname("CLKIN");
        valid_pins =  (valid_pins & 0x7f)|0x40;
        break;

    case 0x10:  // INTRC: Internal Oscillator, RA6 and RA7 are I/O's
        set_int_osc(true);
        (m_porta->getPin(6))->newGUIname("porta6");
        (m_porta->getPin(7))->newGUIname("porta7");
        valid_pins |= 0xc0;
        break;

    case 0x11:  // INTRC: Internal Oscillator, RA7 is an I/O, RA6 is CLKOUT
        set_int_osc(true);
	(m_porta->getPin(6))->newGUIname("CLKOUT");
        (m_porta->getPin(7))->newGUIname("porta7");
        valid_pins = (valid_pins & 0xbf)|0x80;
        break;

    }

    // If the /MCLRE bit is set then RE3 is the MCLR pin, otherwise it's 
    // a general purpose I/O pin.

    if ((cfg_word & CFG_MCLRE)) 
    {
	assignMCLRPin(1);
    }
    else
    {
	unassignMCLRPin();
    }

	
    if (valid_pins != m_porta->getEnableMask()) // enable new pins for IO
    {
        m_porta->setEnableMask(valid_pins);
        m_porta->setTris(m_trisa);
    }
    return true;
  }
  else if (address == 0x2008 )
  {
    //cout << "p16f88x 0x" << hex << address << " config word2 0x" << cfg_word << '\n';
  }

  return false;
}

//========================================================================

void P16F88x::create_config_memory()
{
  m_configMemory = new ConfigMemory(this,2);
  m_configMemory->addConfigWord(0,new Config188x(this));
  m_configMemory->addConfigWord(1,new ConfigWord("CONFIG2", 0,"Configuration Word",this,0x2008));

}


//========================================================================
void  P16F88x::create(int eesize)
{

  create_iopin_map();

  _14bit_processor::create();

  EEPROM_WIDE *e;
  e = new EEPROM_WIDE(this,pir2);
  e->initialize(eesize);
  e->set_intcon(&intcon_reg);
  set_eeprom_wide(e);

  status->rp_mask = 0x60;  // rp0 and rp1 are valid.
  indf->base_address_mask1 = 0x80; // used for indirect accesses above 0x100
  indf->base_address_mask2 = 0x1ff; // used for indirect accesses above 0x100

  P16F88x::create_sfr_map();

}
//========================================================================
//
// Pic 16F882 
//

Processor * P16F882::construct(const char *name)
{

  P16F882 *p = new P16F882(name);

  p->P16F88x::create(128);
  p->P16F882::create_sfr_map();
  p->create_invalid_registers ();
  p->create_symbols();
  return p;

}

P16F882::P16F882(const char *_name, const char *desc)
  : P16F88x(_name,desc)
{

  if(verbose)
    cout << "f882 constructor, type = " << isa() << '\n';

  m_porta->setEnableMask(0xff);



}

//------------------------------------------------------------------------
//
//

void P16F882::create_iopin_map(void)
{

  package = new Package(28);
  if(!package)
    return;

  // Now Create the package and place the I/O pins

  package->assign_pin(1, m_porte->addPin(new IO_bi_directional("porte3"),3));
  package->assign_pin( 2, m_porta->addPin(new IO_bi_directional("porta0"),0));
  package->assign_pin( 3, m_porta->addPin(new IO_bi_directional("porta1"),1));
  package->assign_pin( 4, m_porta->addPin(new IO_bi_directional("porta2"),2));
  package->assign_pin( 5, m_porta->addPin(new IO_bi_directional("porta3"),3));
  package->assign_pin( 6, m_porta->addPin(new IO_open_collector("porta4"),4));
  package->assign_pin( 7, m_porta->addPin(new IO_bi_directional("porta5"),5));

  package->assign_pin(8, 0);
  package->assign_pin( 9, m_porta->addPin(new IO_bi_directional("porta7"),7));
  package->assign_pin( 10, m_porta->addPin(new IO_bi_directional("porta6"),6));

  package->assign_pin(11, m_portc->addPin(new IO_bi_directional("portc0"),0));
  package->assign_pin(12, m_portc->addPin(new IO_bi_directional("portc1"),1));
  package->assign_pin(13, m_portc->addPin(new IO_bi_directional("portc2"),2));
  package->assign_pin(14, m_portc->addPin(new IO_bi_directional("portc3"),3));
  package->assign_pin(15, m_portc->addPin(new IO_bi_directional("portc4"),4));
  package->assign_pin(16, m_portc->addPin(new IO_bi_directional("portc5"),5));
  package->assign_pin(17, m_portc->addPin(new IO_bi_directional("portc6"),6));
  package->assign_pin(18, m_portc->addPin(new IO_bi_directional("portc7"),7));

  package->assign_pin(19, 0);
  package->assign_pin(20, 0);

  package->assign_pin(21, m_portb->addPin(new IO_bi_directional_pu("portb0"),0));
  package->assign_pin(22, m_portb->addPin(new IO_bi_directional_pu("portb1"),1));
  package->assign_pin(23, m_portb->addPin(new IO_bi_directional_pu("portb2"),2));
  package->assign_pin(24, m_portb->addPin(new IO_bi_directional_pu("portb3"),3));
  package->assign_pin(25, m_portb->addPin(new IO_bi_directional_pu("portb4"),4));
  package->assign_pin(26, m_portb->addPin(new IO_bi_directional_pu("portb5"),5));
  package->assign_pin(27, m_portb->addPin(new IO_bi_directional_pu("portb6"),6));
  package->assign_pin(28, m_portb->addPin(new IO_bi_directional_pu("portb7"),7));

}
void P16F882::create_symbols(void)
{

  if(verbose)
    cout << "creating f882 symbols\n";

  P16F88x::create_symbols();

  addSymbol(m_porte);
  addSymbol(m_trise);

}

void P16F882::create_sfr_map()
{

  ccp1con.setIOpin(&(*m_portc)[2], &(*m_portb)[2], &(*m_portb)[1], &(*m_portb)[4]);

}
//========================================================================
//
// Pic 16F883 
//

Processor * P16F883::construct(const char *name)
{

  P16F883 *p = new P16F883(name);

  p->P16F88x::create(256);
  p->P16F883::create_sfr_map();
  p->create_invalid_registers ();
  p->create_symbols();
  return p;

}

P16F883::P16F883(const char *_name, const char *desc)
  : P16F882(_name,desc)
{

  if(verbose)
    cout << "f883 constructor, type = " << isa() << '\n';

  m_porta->setEnableMask(0xff);



}

void P16F883::create_symbols(void)
{

  if(verbose)
    cout << "creating f883 symbols\n";

  P16F88x::create_symbols();

  addSymbol(m_porte);
  addSymbol(m_trise);

}

void P16F883::create_sfr_map()
{

  add_file_registers(0xc0,0xef,0);
  add_file_registers(0x120,0x16f,0);
  ccp1con.setIOpin(&(*m_portc)[2], &(*m_portb)[2], &(*m_portb)[1], &(*m_portb)[4]);


}
//========================================================================
//
// Pic 16F886 
//

Processor * P16F886::construct(const char *name)
{

  P16F886 *p = new P16F886(name);

  p->P16F88x::create(256);
  p->P16F886::create_sfr_map();
  p->create_invalid_registers ();
  p->create_symbols();
  return p;

}

P16F886::P16F886(const char *_name, const char *desc)
  : P16F882(_name,desc)
{

  if(verbose)
    cout << "f886 constructor, type = " << isa() << '\n';

  m_porta->setEnableMask(0xff);



}

void P16F886::create_symbols(void)
{

  if(verbose)
    cout << "creating f886 symbols\n";

  P16F88x::create_symbols();

  addSymbol(m_porte);
  addSymbol(m_trise);

}

void P16F886::create_sfr_map()
{

  add_file_registers(0xc0,0xef,0);
  add_file_registers(0x120,0x16f,0);
  add_file_registers(0x190,0x1ef,0);
  ccp1con.setIOpin(&(*m_portc)[2], &(*m_portb)[2], &(*m_portb)[1], &(*m_portb)[4]);


}
//========================================================================
//
// Pic 16F887 
//

Processor * P16F887::construct(const char *name)
{

  P16F887 *p = new P16F887(name);

  p->P16F88x::create(256);
  p->P16F887::create_sfr_map();
  p->create_invalid_registers ();
  p->create_symbols();
  return p;

}

P16F887::P16F887(const char *_name, const char *desc)
  : P16F884(_name,desc)
{

  if(verbose)
    cout << "f887 constructor, type = " << isa() << '\n';

}
void P16F887::create_symbols(void)
{

  if(verbose)
    cout << "creating f887 symbols\n";

  P16F88x::create_symbols();

  addSymbol(m_portd);
  addSymbol(m_trisd);

}
void P16F887::create_sfr_map()
{

  add_file_registers(0xb0,0xef,0);
  add_file_registers(0x110,0x16f,0);
  add_file_registers(0x190,0x1ef,0);


  add_sfr_register(m_portd, 0x08);
  add_sfr_register(m_trisd, 0x88, RegisterValue(0xff,0));

  ccp1con.setIOpin(&(*m_portc)[2], &(*m_portd)[5], &(*m_portd)[6], &(*m_portd)[7]);
  adcon1.setIOPin(5, &(*m_porte)[0]);
  adcon1.setIOPin(6, &(*m_porte)[1]);
  adcon1.setIOPin(7, &(*m_porte)[2]);


}
//========================================================================
//
// Pic 16F884 
//

Processor * P16F884::construct(const char *name)
{

  P16F884 *p = new P16F884(name);

  p->P16F88x::create(256);
  p->P16F884::create_sfr_map();
  p->create_invalid_registers ();
  p->create_symbols();
  return p;

}

P16F884::P16F884(const char *_name, const char *desc)
  : P16F88x(_name,desc)
{

  if(verbose)
    cout << "f884 constructor, type = " << isa() << '\n';

  m_porta->setEnableMask(0xff);

  // trisa5 is an input only pin
  m_trisa->setEnableMask(0xdf);

  m_portd = new PicPSP_PortRegister(this,"portd","",8,0xff);
  m_trisd = new PicTrisRegister(this,"trisd","",(PicPortRegister *)m_portd, false);

}

//------------------------------------------------------------------------
//
//

void P16F884::create_iopin_map(void)
{

  package = new Package(40);
  if(!package)
    return;

  // Now Create the package and place the I/O pins

  package->assign_pin(1, m_porte->addPin(new IO_bi_directional("porte3"),3));
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
  package->assign_pin( 13, m_porta->addPin(new IO_bi_directional("porta7"),7));
  package->assign_pin( 14, m_porta->addPin(new IO_bi_directional("porta6"),6));

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

}
void P16F884::create_symbols(void)
{

  if(verbose)
    cout << "creating f884 symbols\n";

  P16F88x::create_symbols();

  addSymbol(m_portd);
  addSymbol(m_trisd);

}

void P16F884::create_sfr_map()
{

  add_file_registers(0xc0,0xef,0);
  add_file_registers(0x120,0x16f,0);

  add_sfr_register(m_portd, 0x08);
  add_sfr_register(m_trisd, 0x88, RegisterValue(0xff,0));

  ccp1con.setIOpin(&(*m_portc)[2], &(*m_portd)[5], &(*m_portd)[6], &(*m_portd)[7]);
  adcon1.setIOPin(5, &(*m_porte)[0]);
  adcon1.setIOPin(6, &(*m_porte)[1]);
  adcon1.setIOPin(7, &(*m_porte)[2]);



}
