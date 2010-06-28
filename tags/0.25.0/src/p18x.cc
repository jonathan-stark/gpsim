/*
   Copyright (C) 1998 T. Scott Dattalo
   Copyright (C) 2010 Roy R Rankin

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


#include <stdio.h>
#include <iostream>
#include <string>

#include "../config.h"
#include "p18x.h"
#include "pic-ioports.h"
#include "packages.h"
#include "stimuli.h"
#include "symbol.h"

#define MCLRE (1<<7)
#define LPT1OSC (1<<2)
#define PBADEN (1<<1)
#define CCP2MX (1<<0)

class Config3H_2x21 : public ConfigWord
{
public:
  Config3H_2x21(_16bit_processor *pCpu, unsigned int addr, unsigned int def_val)
    : ConfigWord("CONFIG3H", ~def_val & 0xfff, "Config Reg 3H", pCpu, addr)
  {
	set(def_val);
  }

  virtual void set(gint64 v)
  {
    gint64 i64;
    get(i64);
    int diff = (i64 ^ v) &0xfff;
    Integer::set(v);

    if (m_pCpu)
    {
	if (diff & MCLRE)
	    (v & MCLRE) ? m_pCpu->assignMCLRPin(1) : m_pCpu->unassignMCLRPin();
    }
  }

    virtual string toString()
  {
    gint64 i64;
    get(i64);
    int i = i64 &0xfff;

    char buff[256];
    snprintf(buff, sizeof(buff), "$%04x\n"
	" MCLRE=%d - %s\n"
	" LPT1OSC=%d - Timer1 configured for %s operation\n"
	" PBADEN=%d - PORTB<4:0> pins %s\n"
	" CCP2MX=%d - CCP2 I/O is muxed with %s\n",
	i,
	(i & MCLRE) ? 1:0, (i & MCLRE) ? "Pin is MCLRE" : "Pin is RE3",
	(i & LPT1OSC) ? 1:0, (i & LPT1OSC) ? "low-power" : "higher power",
	(i & PBADEN) ?1:0, 
		(i & PBADEN) ? "analog on Reset" : "digital I/O on reset",
	(i & CCP2MX) ? 1:0, (i & CCP2MX) ? "RC1" : "RB3"
    );
    return string(buff);
  }

};

//----------------------------------------------------------------------
// For only MCLRE in CONFIG3H and using pin 4 (RA5)
//
class Config3H_1x20 : public ConfigWord
{
public:
  Config3H_1x20(_16bit_processor *pCpu, unsigned int addr, unsigned int def_val)
    : ConfigWord("CONFIG3H", ~def_val & 0xfff, "Config Reg 3H", pCpu, addr)
  {
	set(def_val);
  }

  virtual void set(gint64 v)
  {
    gint64 i64;
    get(i64);
    int diff = (i64 ^ v) &0xfff;
    Integer::set(v);

    if (m_pCpu)
    {
	if (diff & MCLRE)
	    (v & MCLRE) ? m_pCpu->assignMCLRPin(4) : m_pCpu->unassignMCLRPin();
    }
  }

    virtual string toString()
  {
    gint64 i64;
    get(i64);
    int i = i64 &0xfff;

    char buff[256];
    snprintf(buff, sizeof(buff), "$%04x\n"
	" MCLRE=%d - %s\n",
	i,
	(i & MCLRE) ? 1:0, (i & MCLRE) ? "Pin is MCLRE" : "Pin is RA5"
    );
    return string(buff);
  }

};

//========================================================================
//
// Pic 18C2x2
//

void P18C2x2::create()
{
  if(verbose)
    cout << "P18C2x2::create\n";

  create_iopin_map();

  _16bit_compat_adc::create();

}

//------------------------------------------------------------------------
void P18C2x2::create_iopin_map()
{
  package = new Package(28);

  if(!package)
    return;

  // Build the links between the I/O Ports and their tris registers.

  //package->assign_pin(1, 0);  // /MCLR
  createMCLRPin(1);

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


  set_osc_pin_Number(0,9, NULL);
  set_osc_pin_Number(1,10, &(*m_porta)[6]);
  //1portc.usart = &usart16;

}
void P18C2x2::create_symbols()
{
  if(verbose)
    cout << "P18C2x2 create symbols\n";

  _16bit_processor::create_symbols();
}

P18C2x2::P18C2x2(const char *_name, const char *desc)
  : _16bit_compat_adc(_name,desc)
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

  _16bit_compat_adc::create();

}
//------------------------------------------------------------------------
void P18C4x2::create_iopin_map()
{

  package = new Package(40);

  if(!package)
    return;

  createMCLRPin(1);

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
  package->assign_pin(13, new IOPIN("OSC1"));
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
  : _16bit_compat_adc(_name,desc)
{

  if(verbose)
    cout << "18c4x2 constructor, type = " << isa() << '\n';

  m_portd = new PicPSP_PortRegister(this,"portd","",8,0xff);
  m_trisd = new PicTrisRegister(this,"trisd","", (PicPortRegister *)m_portd, false);
  m_latd  = new PicLatchRegister(this,"latd","",m_portd);

  m_porte = new PicPortRegister(this,"porte","",8,0x07);
  m_trise = new PicPSP_TrisRegister(this,"trise","", m_porte, false);
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
  adcon1->setNumberOfChannels(8);
  adcon1->setIOPin(5, &(*m_porte)[0]);
  adcon1->setIOPin(6, &(*m_porte)[1]);
  adcon1->setIOPin(7, &(*m_porte)[2]);

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

    cout << " 18c442 create \n";
  set_osc_pin_Number(0,13, NULL);
  set_osc_pin_Number(1,14, &(*m_porta)[6]);
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
// P18F2455	- 18 pin
// 

P18F2455::P18F2455(const char *_name, const char *desc)
  : P18F242(_name,desc),
    eccpas(this, "eccp1as", "ECCP Auto-Shutdown Control Register"),
    pwm1con(this, "eccp1del", "Enhanced PWM Control Register")

{

  cout << "\nP18F2455 does not support USB registers and functionality\n\n";
  if(verbose)
    cout << "18f2455 constructor, type = " << isa() << '\n';

  m_trisc = new PicTrisRegister(this,"trisc","", (PicPortRegister *)m_portc, true);
}

void P18F2455::create()
{
  P18F242::create();

  if(verbose)
    cout << " 18f2455 create \n";

  package->assign_pin(18, 0, false);          // Vusb

  /* The MSSP/I2CC pins are different on this chip. */
  ssp.initialize(&pir_set_def,         // PIR
                &(*m_portb)[1],        // SCK
                &(*m_porta)[5],        // SS
               &(*m_portc)[7],         // SDO
                &(*m_portb)[0],        // SDI
               m_trisb,                // i2c tris port
               SSP_TYPE_MSSP
       );

  m_configMemory->addConfigWord(CONFIG3H-CONFIG1L,new Config3H_2x21(this, CONFIG3H, 0x83));
  add_sfr_register(&pwm1con, 0xfb7, RegisterValue(0,0));
  add_sfr_register(&eccpas, 0xfb6, RegisterValue(0,0));
  ccp1con.setBitMask(0x3f);
  pwm1con.set_mask(0x80);
  eccpas.set_mask(0xfc);
  eccpas.setIOpin(0, 0, &(*m_portb)[0]);
  eccpas.link_registers(&pwm1con, &ccp1con);

  ccp1con.setIOpin(&((*m_portc)[2]), 0, 0, 0);
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
//
// P18F4455
// 

P18F4455::P18F4455(const char *_name, const char *desc)
  : P18F442(_name,desc),
    eccpas(this, "eccp1as", "ECCP Auto-Shutdown Control Register"),
    pwm1con(this, "eccp1del", "Enhanced PWM Control Register")

{

  cout << "\nP18F4455 does not support USB registers and functionality\n\n";
  if(verbose)
    cout << "18f4455 constructor, type = " << isa() << '\n';

  m_trisc = new PicTrisRegister(this,"trisc","", (PicPortRegister *)m_portc, true);
}

void P18F4455::create()
{
  P18F442::create();

  if(verbose)
    cout << " 18f4455 create \n";

 package->assign_pin(18, 0, false);          // Vusb

  /* The MSSP/I2CC pins are different on this chip. */
  ssp.initialize(&pir_set_def,         // PIR
                &(*m_portb)[1],        // SCK
                &(*m_porta)[5],        // SS
               &(*m_portc)[7],         // SDO
                &(*m_portb)[0],        // SDI
               m_trisb,                // i2c tris port
               SSP_TYPE_MSSP
       );

  m_configMemory->addConfigWord(CONFIG3H-CONFIG1L,new Config3H_2x21(this, CONFIG3H, 0x83));
  add_sfr_register(&pwm1con, 0xfb7, RegisterValue(0,0));
  add_sfr_register(&eccpas, 0xfb6, RegisterValue(0,0));
  eccpas.setIOpin(0, 0, &(*m_portb)[0]);
  eccpas.link_registers(&pwm1con, &ccp1con);
//RRR  comparator.cmcon.set_eccpas(&eccpas);
  ccp1con.setBitMask(0xff);
  ccp1con.setCrosslinks(&ccpr1l, &pir1, &tmr2, &eccpas);
  ccp1con.pwm1con = &pwm1con;
  ccp1con.setIOpin(&((*m_portc)[2]), &((*m_portd)[5]), &((*m_portd)[6]), &((*m_portd)[7]));
}

Processor * P18F4455::construct(const char *name)
{

  P18F4455 *p = new P18F4455(name);

  if(verbose)
    cout << " 18F4455 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();
  return p;
}


//------------------------------------------------------------------------

P18Fxx20::P18Fxx20(const char *_name, const char *desc)
  : _16bit_v2_adc(_name,desc)
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
  EEPROM_PIR *e;

  if(verbose)
    cout << "P18F1220::create\n";

  e = new EEPROM_PIR(this,&pir2);

  // We might want to pass this value in for larger eeproms
  e->initialize(256);
  //e->set_pir_set(&pir_set_def);
  e->set_intcon(&intcon);

  // assign this eeprom to the processor
  set_eeprom_pir(e);

  create_iopin_map();

  //call his before _16bit_processor::create() because 1st one wins
  //with regards to the rx/tx ports
  usart.initialize(&pir_set_def,&(*m_portb)[1], &(*m_portb)[4],
		   new _TXREG(this,"txreg", "USART Transmit Register", &usart), 
                   new _RCREG(this,"rcreg", "USART Receiver Register", &usart));

  _16bit_processor::create();
  _16bit_v2_adc::create(7);
  adcon1->setIOPin(4, &(*m_portb)[0]);
  adcon1->setIOPin(5, &(*m_portb)[1]);
  adcon1->setIOPin(6, &(*m_portb)[4]);
  adcon1->setValidCfgBits(0x7f, 0);
  adcon0->setChannel_Mask(0x7);
  adcon1->setAdcon0(adcon0);	// VCFG0, VCFG1 in adcon0
  remove_sfr_register(&ssp.sspcon2);
  remove_sfr_register(&ssp.sspcon);
  remove_sfr_register(&ssp.sspstat);
  remove_sfr_register(&ssp.sspadd);
  remove_sfr_register(&ssp.sspbuf);

  add_sfr_register(&osctune,      0xf9b,RegisterValue(0,0));
  osccon.set_osctune(&osctune);
  osctune.set_osccon(&osccon);


  set_osc_pin_Number(0,16, &(*m_porta)[7]);
  set_osc_pin_Number(1,15, &(*m_porta)[6]);
  m_configMemory->addConfigWord(CONFIG1H-CONFIG1L,new Config1H_4bits(this, CONFIG1H, 0xcf));
  m_configMemory->addConfigWord(CONFIG3H-CONFIG1L,new Config3H_1x20(this, CONFIG3H, 0x80));

  add_sfr_register(&usart.baudcon,  0xfaa,RegisterValue(0,0),"baudcon");
  usart.set_eusart(true);

  add_sfr_register(&pwm1con, 0xfb7, RegisterValue(0,0));
  add_sfr_register(&eccpas, 0xfb6, RegisterValue(0,0));
  ccp1con.setBitMask(0xff);
  ccp1con.setCrosslinks(&ccpr1l, &pir1, &tmr2, &eccpas);
  eccpas.setIOpin(&(*m_portb)[1], &(*m_portb)[2], &(*m_portb)[0]);
  eccpas.link_registers(&pwm1con, &ccp1con);
  ccp1con.pwm1con = &pwm1con;
  ccp1con.setIOpin(&((*m_portb)[3]), &((*m_portb)[2]), &((*m_portb)[6]), &((*m_portb)[7]));
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
  : P18Fxx20(_name,desc),
    osctune(this, "osctune", "OSC Tune"),
    eccpas(this, "eccpas", "ECCP Auto-Shutdown Control Register"),
    pwm1con(this, "pwm1con", "Enhanced PWM Control Register")

{

  if(verbose)
    cout << "18F1220 constructor, type = " << isa() << '\n';


}

void P18F1220::osc_mode(unsigned int value)
{
  IOPIN *m_pin;
  unsigned int pin_Number =  get_osc_pin_Number(0);
  
  set_int_osc(false);
  if (pin_Number < 253)
  {
	m_pin = package->get_pin(pin_Number);
	if (value == 8 || value == 9)
	{
	    clr_clk_pin(pin_Number, get_osc_PinMonitor(0), 
		m_porta, m_trisa, m_lata);
	    set_int_osc(true);
	}
	else
	{
	    set_clk_pin(pin_Number, get_osc_PinMonitor(0), "OSC1", true,
		 m_porta, m_trisa, m_lata);
	}
  }
  if ( (pin_Number =  get_osc_pin_Number(1)) < 253 &&
	(m_pin = package->get_pin(pin_Number)))
  {
	pll_factor = 0;
	switch(value)
	{
	case 6:
	    pll_factor = 2;
	case 0:
	case 1:
	case 2:
	    set_clk_pin(pin_Number, get_osc_PinMonitor(1), "OSC2", true,
		m_porta, m_trisa, m_lata);
	    break;

	case 4:
	case 9:
	case 12:
	case 13:
	case 14:
	case 15:
	    cout << "CLKO not simulated\n";
	    set_clk_pin(pin_Number, get_osc_PinMonitor(1) , "CLKO", false,
		m_porta, m_trisa, m_lata);
	    break;

	default:
	    clr_clk_pin(pin_Number, get_osc_PinMonitor(1),
		m_porta, m_trisa, m_lata);
	    break;
	}
  }
  
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
  m_configMemory->addConfigWord(CONFIG3H-CONFIG1L,new Config3H_2x21(this, CONFIG3H, 0x83));


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
  package->assign_pin( 6, m_porta->addPin(new IO_open_collector("porta4"),4));  // %%%FIXME - is this O/C ?
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



void P18F2x21::create_symbols()
{
  if(verbose)
    cout << "P18F2x21 create symbols\n";

  _16bit_processor::create_symbols();
}

P18F2x21::P18F2x21(const char *_name, const char *desc)
  : _16bit_v2_adc(_name,desc),
    eccpas(this, "eccp1as", "ECCP Auto-Shutdown Control Register"),
    pwm1con(this, "eccp1del", "Enhanced PWM Control Register"),
    osctune(this, "osctune", "OSC Tune"),
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
  _16bit_v2_adc::create(13);

  RegisterValue porv(0,0);


  add_sfr_register(m_porte,       0xf84,porv);

  adcon1->setIOPin(4, &(*m_porta)[5]);
/*  Not on 28 pin processors
  adcon1->setIOPin(5, &(*m_porte)[0]);
  adcon1->setIOPin(6, &(*m_porte)[1]);
  adcon1->setIOPin(7, &(*m_porte)[2]);
*/
  adcon1->setIOPin(8, &(*m_portb)[2]);
  adcon1->setIOPin(9, &(*m_portb)[3]);
  adcon1->setIOPin(10, &(*m_portb)[1]);
  adcon1->setIOPin(11, &(*m_portb)[4]);
  adcon1->setIOPin(12, &(*m_portb)[0]);

  add_sfr_register(&osctune,      0xf9b,porv);
  osccon.set_osctune(&osctune);
  osctune.set_osccon(&osccon);

  // rest of configuration in parent class

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

  ccp2con.setCrosslinks(&ccpr2l, &pir2, &tmr2);
  ccp2con.setIOpin(&((*m_portc)[1]));
  ccpr2l.ccprh  = &ccpr2h;
  ccpr2l.tmrl   = &tmr1l;
  ccpr2h.ccprl  = &ccpr2l;

  //1 usart16.initialize_16(this,&pir_set_def,&portc);
  add_sfr_register(&usart.spbrgh,   0xfb0,porv,"spbrgh");
  add_sfr_register(&usart.baudcon,  0xfb8,porv,"baudcon");
  usart.set_eusart(true);
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

  set_osc_pin_Number(0, 9, &(*m_porta)[7]);
  set_osc_pin_Number(1,10, &(*m_porta)[6]);
  m_configMemory->addConfigWord(CONFIG1H-CONFIG1L,new Config1H_4bits(this, CONFIG1H, 0x07));
  add_sfr_register(&pwm1con, 0xfb7, RegisterValue(0,0));
  add_sfr_register(&eccpas, 0xfb6, RegisterValue(0,0));
  eccpas.set_mask(0xfc);
  eccpas.setIOpin(0, 0, &(*m_portb)[0]);
  eccpas.link_registers(&pwm1con, &ccp1con);
  comparator.cmcon.set_eccpas(&eccpas);
  ccp1con.setBitMask(0x3f);
  ccp1con.setCrosslinks(&ccpr1l, &pir1, &tmr2, &eccpas);
  ccp1con.pwm1con = &pwm1con;
  ccp1con.setIOpin(&((*m_portc)[2]), 0, 0, 0);
  pwm1con.set_mask(0x80);
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

void P18F2321::osc_mode(unsigned int value)
{
  IOPIN *m_pin;
  unsigned int pin_Number =  get_osc_pin_Number(0);
  
  set_int_osc(false);
  if (pin_Number < 253)
  {
	m_pin = package->get_pin(pin_Number);
	if (value == 8 || value == 9)	// internal RC clock
	{
	    clr_clk_pin(pin_Number, get_osc_PinMonitor(0),
		m_porta, m_trisa, m_lata);
	    set_int_osc(true);
	}
	else
        {
	    set_clk_pin(pin_Number, get_osc_PinMonitor(0), "OSC1", true,
		m_porta, m_trisa, m_lata);
	    set_int_osc(false);
	}
  }
  if ( (pin_Number =  get_osc_pin_Number(1)) < 253 &&
	(m_pin = package->get_pin(pin_Number)))
  {
	pll_factor = 0;
	switch(value)
	{
	case 6:
	    pll_factor = 2;
	case 0:
	case 1:
	case 2:
	    set_clk_pin(pin_Number, get_osc_PinMonitor(1), "OSC2", true,
		m_porta, m_trisa, m_lata);
	    break;

	case 3:
	case 4:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	    cout << "CLKO not simulated\n";
	    set_clk_pin(pin_Number, get_osc_PinMonitor(1) , "CLKO", false,
		m_porta, m_trisa, m_lata);
	    break;

	default:
	    clr_clk_pin(pin_Number, get_osc_PinMonitor(1),
		m_porta, m_trisa, m_lata);
	    break;
	}
  }
  
}


//========================================================================
//
// Pic 18F4x21
//

void P18F4x21::create()
{
  if(verbose)
    cout << "P18F4x21::create\n";

  create_iopin_map();

  _16bit_processor::create();

}

//------------------------------------------------------------------------
void P18F4x21::create_iopin_map()
{
  package = new Package(40);

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

  package->assign_pin( 8, m_porte->addPin(new IO_bi_directional("porte0"),0));
  package->assign_pin( 9, m_porte->addPin(new IO_bi_directional("porte1"),1));
  package->assign_pin(10, m_porte->addPin(new IO_bi_directional("porte2"),2));


  package->assign_pin(11, 0);   // Vdd
  package->assign_pin(12, 0);   // Vss
  package->assign_pin(13, m_porta->addPin(new IO_bi_directional("porta7"),7));
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

  package->assign_pin(31, 0);   // Vss
  package->assign_pin(32, 0);   // Vdd

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

void P18F4x21::create_symbols()
{
  if(verbose)
    cout << "P18F4x21 create symbols\n";

  _16bit_processor::create_symbols();
}

P18F4x21::P18F4x21(const char *_name, const char *desc)
  : P18F2x21(_name,desc)
{

  if(verbose)
    cout << "18f4x21 constructor, type = " << isa() << '\n';

  m_portd = new PicPSP_PortRegister(this,"portd","",8,0xFF);
  m_trisd = new PicTrisRegister(this,"trisd","", (PicPortRegister *)m_portd, true);
  m_latd  = new PicLatchRegister(this,"latd","",m_portd);

//  m_porte = new PicPortRegister(this,"porte","",8,0x07);
  m_porte->setEnableMask(0x07);     // It's been created by the P18F2x21 constructor, but with the wrong enables
  m_trise = new PicPSP_TrisRegister(this,"trise","", m_porte, true);
  m_late  = new PicLatchRegister(this,"late","",m_porte);

}

void P18F4x21::create_sfr_map()
{

  if(verbose)
    cout << "create_sfr_map P18F4x21\n";

  _16bit_processor::create_sfr_map();
  _16bit_v2_adc::create(13);


  RegisterValue porv(0,0);

  add_sfr_register(m_portd,       0xf83,porv);
  add_sfr_register(m_porte,       0xf84,porv);

  add_sfr_register(m_latd,        0xf8c,porv);
  add_sfr_register(m_late,        0xf8d,porv);

  add_sfr_register(m_trisd,       0xf95,RegisterValue(0xff,0));
  add_sfr_register(m_trise,       0xf96,RegisterValue(0x07,0));


  add_sfr_register(&osctune,      0xf9b,porv);
  osccon.set_osctune(&osctune);
  osctune.set_osccon(&osccon);

  adcon1->setIOPin(4, &(*m_porta)[5]);
  adcon1->setIOPin(5, &(*m_porte)[0]);
  adcon1->setIOPin(6, &(*m_porte)[1]);
  adcon1->setIOPin(7, &(*m_porte)[2]);
  adcon1->setIOPin(8, &(*m_portb)[2]);
  adcon1->setIOPin(9, &(*m_portb)[3]);
  adcon1->setIOPin(10, &(*m_portb)[1]);
  adcon1->setIOPin(11, &(*m_portb)[4]);
  adcon1->setIOPin(12, &(*m_portb)[0]);
/*
  adcon1->setChanTable(0x1ff, 0x1fff, 0x1fff, 0x0fff,
	0x07ff, 0x03ff, 0x01ff, 0x00ff, 0x007f, 0x003f,
	0x001f, 0x000f, 0x0007, 0x0003, 0x0001, 0x0000);
  adcon1->setVrefHiChannel(3);
  adcon1->setVrefLoChannel(2);
*/





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

  ccp2con.setCrosslinks(&ccpr2l, &pir2, &tmr2);
  ccp2con.setIOpin(&((*m_portc)[1]));
  ccpr2l.ccprh  = &ccpr2h;
  ccpr2l.tmrl   = &tmr1l;
  ccpr2h.ccprl  = &ccpr2l;

  //1 usart16.initialize_16(this,&pir_set_def,&portc);
  add_sfr_register(&usart.spbrgh,   0xfb0,porv,"spbrgh");
  add_sfr_register(&usart.baudcon,  0xfb8,porv,"baudcon");
  usart.set_eusart(true);
}



//------------------------------------------------------------------------
//
// P18F4321
// 

P18F4321::P18F4321(const char *_name, const char *desc)
  : P18F4x21(_name,desc)
{

  if(verbose)
    cout << "18F4321 constructor, type = " << isa() << '\n';

}

void P18F4321::create()
{
  EEPROM_PIR *e;

  if(verbose)
    cout << " 18F4321 create \n";

  e = new EEPROM_PIR(this,&pir2);

  // We might want to pass this value in for larger eeproms
  e->initialize(256);
  //e->set_pir_set(&pir_set_def);
  e->set_intcon(&intcon);

  // assign this eeprom to the processor
  set_eeprom_pir(e);

  P18F2x21::create();
  set_osc_pin_Number(0, 13, &(*m_porta)[7]);
  set_osc_pin_Number(1,14, &(*m_porta)[6]);
  m_configMemory->addConfigWord(CONFIG1H-CONFIG1L,new Config1H_4bits(this, CONFIG1H, 0x07));
  add_sfr_register(&pwm1con, 0xfb7, RegisterValue(0,0));
  add_sfr_register(&eccpas, 0xfb6, RegisterValue(0,0));
  eccpas.setIOpin(0, 0, &(*m_portb)[0]);
  eccpas.link_registers(&pwm1con, &ccp1con);
  comparator.cmcon.set_eccpas(&eccpas);
  ccp1con.setBitMask(0xff);
  ccp1con.setCrosslinks(&ccpr1l, &pir1, &tmr2, &eccpas);
  ccp1con.pwm1con = &pwm1con;
  ccp1con.setIOpin(&((*m_portc)[2]), &((*m_portd)[5]), &((*m_portd)[6]), &((*m_portd)[7]));

}

Processor * P18F4321::construct(const char *name)
{

  P18F4321 *p = new P18F4321(name);

  if(verbose)
    cout << " 18F4321 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();

  if(verbose&2)
    cout << " 18F4321 construct completed\n";
  return p;
}


void P18F4321::osc_mode(unsigned int value)
{
  IOPIN *m_pin;
  unsigned int pin_Number =  get_osc_pin_Number(0);
  
  
  set_int_osc(false);
  if (pin_Number < 253)
  {
	m_pin = package->get_pin(pin_Number);
	if (value == 8 || value == 9)
	{
	    set_int_osc(true);
	    clr_clk_pin(pin_Number, get_osc_PinMonitor(0),
		m_porta, m_trisa, m_lata);
	}
	else
	{
	    set_clk_pin(pin_Number, get_osc_PinMonitor(0), "OSC1", true,
		m_porta, m_trisa, m_lata);
	}
  }
  if ( (pin_Number =  get_osc_pin_Number(1)) < 253 &&
	(m_pin = package->get_pin(pin_Number)))
  {
	pll_factor = 0;
	switch(value)
	{
	case 6:
	   pll_factor = 2;
	case 0:
	case 1:
	case 2:
	    set_clk_pin(pin_Number, get_osc_PinMonitor(1), "OSC2", true,
		m_porta, m_trisa, m_lata);
	    break;

	case 3:
	case 4:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	    cout << "CLKO not simulated\n";
	    set_clk_pin(pin_Number, get_osc_PinMonitor(1) , "CLKO", false,
		m_porta, m_trisa, m_lata);
	    break;

	default:
	    clr_clk_pin(pin_Number, get_osc_PinMonitor(1),
		m_porta, m_trisa, m_lata);
	    break;
	}
  }
  
}
