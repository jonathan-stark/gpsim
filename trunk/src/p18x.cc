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

/* Config Word defines */
#define MCLRE 	(1<<7)
#define P2BMX 	(1<<5)
#define T3CMX	(1<<4)
#define HFOFST	(1<<3)
#define LPT1OSC (1<<2)
#define CCP3MX 	(1<<2)
#define PBADEN 	(1<<1)
#define CCP2MX 	(1<<0)


/**
 *  A special variant of the Config3H class that includes all the bits that
 *  config register does on PIC18F2x21 and derivatives (including 4620). Note
 *  that the "set" method requires that the parent processor is an instance
 *  of the P18F2x21 class (or a derived variant thereof).
 */
class Config3H_2x21 : public ConfigWord
{
public:
  Config3H_2x21(_16bit_processor *pCpu, unsigned int addr, unsigned int def_val)
    : ConfigWord("CONFIG3H", ~def_val & 0xfff, "Config Reg 3H", pCpu, addr)
  {
    set(def_val);
	if (verbose)
	  cout << "Config3H_2x21\n";
  }

  virtual void set(gint64 v)
  {
    gint64 i64;
    get(i64);
    int diff = (i64 ^ v) &0xfff;
    Integer::set(v);

    if (m_pCpu)
    {
      P18F2x21 *pCpu21 = (P18F2x21*)m_pCpu;

      if (diff & MCLRE)
        (v & MCLRE) ? m_pCpu->assignMCLRPin(1) : m_pCpu->unassignMCLRPin();
      if ( pCpu21->adcon1 )
      {
        unsigned int pcfg = (v & PBADEN) ? 0 
                           : (ADCON1::PCFG0 | ADCON1::PCFG1 | ADCON1::PCFG2);
        pCpu21->adcon1->por_value=RegisterValue(pcfg,0);
      }
      if ( diff & CCP2MX )
      {
        if ( v & CCP2MX )
          pCpu21->ccp2con.setIOpin(&((*pCpu21->m_portc)[1]));
        else
          pCpu21->ccp2con.setIOpin(&((*pCpu21->m_portb)[3]));
      }
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
  add_sfr_register(&osccon, 0xfd3, RegisterValue(0x00,0), "osccon");
  init_pir2(pir2, PIR2v2::TMR3IF);

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
  add_sfr_register(&osccon, 0xfd3, RegisterValue(0x00,0), "osccon");

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
P18C4x2::~P18C4x2()
{
  delete_sfr_register(m_portd);
  delete_sfr_register(m_porte);

  delete_sfr_register(m_latd);
  delete_sfr_register(m_late);

  delete_sfr_register(m_trisd);
  delete_sfr_register(m_trise);
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
  init_pir2(pir2, PIR2v2::TMR3IF); 
  tmr3l.setIOpin(&(*m_portc)[0]);

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

  if(verbose)
    cout << " 18f242 create \n";

  tbl.initialize ( eeprom_memory_size(), 32, 4, CONFIG1L, false);
  tbl.set_intcon(&intcon);
  set_eeprom_pir(&tbl);
  tbl.set_pir(pir2);
  tbl.eecon1.set_valid_bits(0xbf);

  P18C242::create();
}


Processor * P18F242::construct(const char *name)
{

  P18F242 *p = new P18F242(name);

  if(verbose)
    cout << " 18F242 construct\n";

  p->create();
  p->create_invalid_registers();
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

  if(verbose)
    cout << " 18f442 create \n";

  tbl.initialize ( eeprom_memory_size(), 32, 4, CONFIG1L, false);
  tbl.set_intcon(&intcon);
  set_eeprom_pir(&tbl);
  tbl.set_pir(pir2);
  tbl.eecon1.set_valid_bits(0xbf);

  P18C442::create();
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
// P18F2455	- 28 pin
// 

P18F2455::P18F2455(const char *_name, const char *desc)
  : P18F2x21(_name,desc),
      ufrml(this, "ufrml", "USB Frame Number register Low"      ),
      ufrmh(this, "ufrmh", "USB Frame Number register High"     ),
      uir  (this, "uir"  , "USB Interrupt Status register"      ),
      uie  (this, "uie"  , "USB Interrupt Enable register"      ),
      ueir (this, "ueir" , "USB Error Interrupt Status register"),
      ueie (this, "ueie" , "USB Error Interrupt Enable register"),
      ustat(this, "ustat", "USB Transfer Status register"       ),
      ucon (this, "ucon" , "USB Control register"               ),
      uaddr(this, "uaddr", "USB Device Address register"        ),
      ucfg (this, "ucfg" , "USB Configuration register"         ),
      uep0 (this, "uep0" , "USB Endpoint 0 Enable register"     ),
      uep1 (this, "uep1" , "USB Endpoint 1 Enable register"     ),
      uep2 (this, "uep2" , "USB Endpoint 2 Enable register"     ),
      uep3 (this, "uep3" , "USB Endpoint 3 Enable register"     ),
      uep4 (this, "uep4" , "USB Endpoint 4 Enable register"     ),
      uep5 (this, "uep5" , "USB Endpoint 5 Enable register"     ),
      uep6 (this, "uep6" , "USB Endpoint 6 Enable register"     ),
      uep7 (this, "uep7" , "USB Endpoint 7 Enable register"     ),
      uep8 (this, "uep8" , "USB Endpoint 8 Enable register"     ),
      uep9 (this, "uep9" , "USB Endpoint 9 Enable register"     ),
      uep10(this, "uep10", "USB Endpoint 10 Enable register"    ),
      uep11(this, "uep11", "USB Endpoint 11 Enable register"    ),
      uep12(this, "uep12", "USB Endpoint 12 Enable register"    ),
      uep13(this, "uep13", "USB Endpoint 13 Enable register"    ),
      uep14(this, "uep14", "USB Endpoint 14 Enable register"    ),
      uep15(this, "uep15", "USB Endpoint 15 Enable register"    )
{

  cout << "\nP18F2455 does not support USB registers and functionality\n\n";
  if(verbose)
    cout << "18f2455 constructor, type = " << isa() << '\n';

}

P18F2455::~P18F2455()
{
  remove_sfr_register(&ufrml);
  remove_sfr_register(&ufrmh);
  remove_sfr_register(&uir  );
  remove_sfr_register(&uie  );
  remove_sfr_register(&ueir );
  remove_sfr_register(&ueie );
  remove_sfr_register(&ustat);
  remove_sfr_register(&ucon );
  remove_sfr_register(&uaddr);
  remove_sfr_register(&ucfg );
  remove_sfr_register(&uep0 );
  remove_sfr_register(&uep1 );
  remove_sfr_register(&uep2 );
  remove_sfr_register(&uep3 );
  remove_sfr_register(&uep4 );
  remove_sfr_register(&uep5 );
  remove_sfr_register(&uep6 );
  remove_sfr_register(&uep7 );
  remove_sfr_register(&uep8 );
  remove_sfr_register(&uep9 );
  remove_sfr_register(&uep10);
  remove_sfr_register(&uep11);
  remove_sfr_register(&uep12);
  remove_sfr_register(&uep13);
  remove_sfr_register(&uep14);
  remove_sfr_register(&uep15);
}
void P18F2455::create_sfr_map()
{

  if(verbose)
    cout << " 18f2455 create_sfr_map \n";


  P18F2x21::create_sfr_map();
  package->destroy_pin(14);
  package->assign_pin(14, 0, false);          // Vusb

  /* The MSSP/I2CC pins are different on this chip. */
  ssp.initialize(&pir_set_def,         // PIR
                 &(*m_portb)[1],       // SCK
                 &(*m_porta)[5],       // SS
                 &(*m_portc)[7],       // SDO
                 &(*m_portb)[0],       // SDI
                 m_trisb,              // i2c tris port
                 SSP_TYPE_MSSP
       );
  add_sfr_register(&ufrml,0x0F66, RegisterValue(0,0),"ufrm");
  add_sfr_register(&ufrmh,0X0F67, RegisterValue(0,0));
  add_sfr_register(&uir  ,0x0F68, RegisterValue(0,0));
  add_sfr_register(&uie  ,0x0F69, RegisterValue(0,0));
  add_sfr_register(&ueir ,0x0F6A, RegisterValue(0,0));
  add_sfr_register(&ueie ,0x0F6B, RegisterValue(0,0));
  add_sfr_register(&ustat,0X0F6C, RegisterValue(0,0));
  add_sfr_register(&ucon ,0x0F6D, RegisterValue(0,0));
  add_sfr_register(&uaddr,0X0F6E, RegisterValue(0,0));
  add_sfr_register(&ucfg ,0x0F6F, RegisterValue(0,0));
  add_sfr_register(&uep0 ,0x0F70, RegisterValue(0,0));
  add_sfr_register(&uep1 ,0x0F71, RegisterValue(0,0));
  add_sfr_register(&uep2 ,0x0F72, RegisterValue(0,0));
  add_sfr_register(&uep3 ,0x0F73, RegisterValue(0,0));
  add_sfr_register(&uep4 ,0x0F74, RegisterValue(0,0));
  add_sfr_register(&uep5 ,0x0F75, RegisterValue(0,0));
  add_sfr_register(&uep6 ,0x0F76, RegisterValue(0,0));
  add_sfr_register(&uep7 ,0x0F77, RegisterValue(0,0));
  add_sfr_register(&uep8 ,0x0F78, RegisterValue(0,0));
  add_sfr_register(&uep9 ,0x0F79, RegisterValue(0,0));
  add_sfr_register(&uep10,0x0F7A, RegisterValue(0,0));
  add_sfr_register(&uep11,0x0F7B, RegisterValue(0,0));
  add_sfr_register(&uep12,0x0F7C, RegisterValue(0,0));
  add_sfr_register(&uep13,0x0F7D, RegisterValue(0,0));
  add_sfr_register(&uep14,0x0F7E, RegisterValue(0,0));
  add_sfr_register(&uep15,0x0F7F, RegisterValue(0,0));
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
// P18F2550	- 28 pin
// 

P18F2550::P18F2550(const char *_name, const char *desc)
  : P18F2x21(_name,desc),
      ufrml(this, "ufrml", "USB Frame Number register Low"      ),
      ufrmh(this, "ufrmh", "USB Frame Number register High"     ),
      uir  (this, "uir"  , "USB Interrupt Status register"      ),
      uie  (this, "uie"  , "USB Interrupt Enable register"      ),
      ueir (this, "ueir" , "USB Error Interrupt Status register"),
      ueie (this, "ueie" , "USB Error Interrupt Enable register"),
      ustat(this, "ustat", "USB Transfer Status register"       ),
      ucon (this, "ucon" , "USB Control register"               ),
      uaddr(this, "uaddr", "USB Device Address register"        ),
      ucfg (this, "ucfg" , "USB Configuration register"         ),
      uep0 (this, "uep0" , "USB Endpoint 0 Enable register"     ),
      uep1 (this, "uep1" , "USB Endpoint 1 Enable register"     ),
      uep2 (this, "uep2" , "USB Endpoint 2 Enable register"     ),
      uep3 (this, "uep3" , "USB Endpoint 3 Enable register"     ),
      uep4 (this, "uep4" , "USB Endpoint 4 Enable register"     ),
      uep5 (this, "uep5" , "USB Endpoint 5 Enable register"     ),
      uep6 (this, "uep6" , "USB Endpoint 6 Enable register"     ),
      uep7 (this, "uep7" , "USB Endpoint 7 Enable register"     ),
      uep8 (this, "uep8" , "USB Endpoint 8 Enable register"     ),
      uep9 (this, "uep9" , "USB Endpoint 9 Enable register"     ),
      uep10(this, "uep10", "USB Endpoint 10 Enable register"    ),
      uep11(this, "uep11", "USB Endpoint 11 Enable register"    ),
      uep12(this, "uep12", "USB Endpoint 12 Enable register"    ),
      uep13(this, "uep13", "USB Endpoint 13 Enable register"    ),
      uep14(this, "uep14", "USB Endpoint 14 Enable register"    ),
      uep15(this, "uep15", "USB Endpoint 15 Enable register"    )
{

  cout << "\nP18F2550 does not support USB registers and functionality\n\n";
  if(verbose)
    cout << "18f2550 constructor, type = " << isa() << '\n';
}

P18F2550::~P18F2550()
{
  remove_sfr_register(&ufrml);
  remove_sfr_register(&ufrmh);
  remove_sfr_register(&uir  );
  remove_sfr_register(&uie  );
  remove_sfr_register(&ueir );
  remove_sfr_register(&ueie );
  remove_sfr_register(&ustat);
  remove_sfr_register(&ucon );
  remove_sfr_register(&uaddr);
  remove_sfr_register(&ucfg );
  remove_sfr_register(&uep0 );
  remove_sfr_register(&uep1 );
  remove_sfr_register(&uep2 );
  remove_sfr_register(&uep3 );
  remove_sfr_register(&uep4 );
  remove_sfr_register(&uep5 );
  remove_sfr_register(&uep6 );
  remove_sfr_register(&uep7 );
  remove_sfr_register(&uep8 );
  remove_sfr_register(&uep9 );
  remove_sfr_register(&uep10);
  remove_sfr_register(&uep11);
  remove_sfr_register(&uep12);
  remove_sfr_register(&uep13);
  remove_sfr_register(&uep14);
  remove_sfr_register(&uep15);
}
void P18F2550::create_sfr_map()
{

  if(verbose)
    cout << " 18f2550 create_sfr_map \n";


  P18F2x21::create_sfr_map();
  package->destroy_pin(14);
  package->assign_pin(14, 0, false);          // Vusb

  /* The MSSP/I2CC pins are different on this chip. */
  ssp.initialize(&pir_set_def,         // PIR
                 &(*m_portb)[1],       // SCK
                 &(*m_porta)[5],       // SS
                 &(*m_portc)[7],       // SDO
                 &(*m_portb)[0],       // SDI
                 m_trisb,              // i2c tris port
                 SSP_TYPE_MSSP
       );
  add_sfr_register(&ufrml,0x0F66, RegisterValue(0,0),"ufrm");
  add_sfr_register(&ufrmh,0X0F67, RegisterValue(0,0));
  add_sfr_register(&uir  ,0x0F68, RegisterValue(0,0));
  add_sfr_register(&uie  ,0x0F69, RegisterValue(0,0));
  add_sfr_register(&ueir ,0x0F6A, RegisterValue(0,0));
  add_sfr_register(&ueie ,0x0F6B, RegisterValue(0,0));
  add_sfr_register(&ustat,0X0F6C, RegisterValue(0,0));
  add_sfr_register(&ucon ,0x0F6D, RegisterValue(0,0));
  add_sfr_register(&uaddr,0X0F6E, RegisterValue(0,0));
  add_sfr_register(&ucfg ,0x0F6F, RegisterValue(0,0));
  add_sfr_register(&uep0 ,0x0F70, RegisterValue(0,0));
  add_sfr_register(&uep1 ,0x0F71, RegisterValue(0,0));
  add_sfr_register(&uep2 ,0x0F72, RegisterValue(0,0));
  add_sfr_register(&uep3 ,0x0F73, RegisterValue(0,0));
  add_sfr_register(&uep4 ,0x0F74, RegisterValue(0,0));
  add_sfr_register(&uep5 ,0x0F75, RegisterValue(0,0));
  add_sfr_register(&uep6 ,0x0F76, RegisterValue(0,0));
  add_sfr_register(&uep7 ,0x0F77, RegisterValue(0,0));
  add_sfr_register(&uep8 ,0x0F78, RegisterValue(0,0));
  add_sfr_register(&uep9 ,0x0F79, RegisterValue(0,0));
  add_sfr_register(&uep10,0x0F7A, RegisterValue(0,0));
  add_sfr_register(&uep11,0x0F7B, RegisterValue(0,0));
  add_sfr_register(&uep12,0x0F7C, RegisterValue(0,0));
  add_sfr_register(&uep13,0x0F7D, RegisterValue(0,0));
  add_sfr_register(&uep14,0x0F7E, RegisterValue(0,0));
  add_sfr_register(&uep15,0x0F7F, RegisterValue(0,0));

}

Processor * P18F2550::construct(const char *name)
{

  P18F2550 *p = new P18F2550(name);

  if(verbose)
    cout << " 18F2550 construct\n";

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
  : P18F4x21(_name,desc),
      ufrml(this, "ufrml", "USB Frame Number register Low"      ),
      ufrmh(this, "ufrmh", "USB Frame Number register High"     ),
      uir  (this, "uir"  , "USB Interrupt Status register"      ),
      uie  (this, "uie"  , "USB Interrupt Enable register"      ),
      ueir (this, "ueir" , "USB Error Interrupt Status register"),
      ueie (this, "ueie" , "USB Error Interrupt Enable register"),
      ustat(this, "ustat", "USB Transfer Status register"       ),
      ucon (this, "ucon" , "USB Control register"               ),
      uaddr(this, "uaddr", "USB Device Address register"        ),
      ucfg (this, "ucfg" , "USB Configuration register"         ),
      uep0 (this, "uep0" , "USB Endpoint 0 Enable register"     ),
      uep1 (this, "uep1" , "USB Endpoint 1 Enable register"     ),
      uep2 (this, "uep2" , "USB Endpoint 2 Enable register"     ),
      uep3 (this, "uep3" , "USB Endpoint 3 Enable register"     ),
      uep4 (this, "uep4" , "USB Endpoint 4 Enable register"     ),
      uep5 (this, "uep5" , "USB Endpoint 5 Enable register"     ),
      uep6 (this, "uep6" , "USB Endpoint 6 Enable register"     ),
      uep7 (this, "uep7" , "USB Endpoint 7 Enable register"     ),
      uep8 (this, "uep8" , "USB Endpoint 8 Enable register"     ),
      uep9 (this, "uep9" , "USB Endpoint 9 Enable register"     ),
      uep10(this, "uep10", "USB Endpoint 10 Enable register"    ),
      uep11(this, "uep11", "USB Endpoint 11 Enable register"    ),
      uep12(this, "uep12", "USB Endpoint 12 Enable register"    ),
      uep13(this, "uep13", "USB Endpoint 13 Enable register"    ),
      uep14(this, "uep14", "USB Endpoint 14 Enable register"    ),
      uep15(this, "uep15", "USB Endpoint 15 Enable register"    ),
      sppcon(this, "sppcon", "Streaming Parallel port control register"),
      sppcfg(this, "sppcfg", "Streaming Parallel port configuration register"),
      sppeps(this, "sppeps", "SPP ENDPOINT ADDRESS AND STATUS REGISTER"),
      sppdata(this, "sppdata", "Streaming Parallel port data register")

{
  cout << "\nP18F4455 does not support USB registers and functionality\n\n";
  if(verbose)
    cout << "18f4455 constructor, type = " << isa() << '\n';
}

P18F4455::~P18F4455()
{
  remove_sfr_register(&ufrml);
  remove_sfr_register(&ufrmh);
  remove_sfr_register(&uir  );
  remove_sfr_register(&uie  );
  remove_sfr_register(&ueir );
  remove_sfr_register(&ueie );
  remove_sfr_register(&ustat);
  remove_sfr_register(&ucon );
  remove_sfr_register(&uaddr);
  remove_sfr_register(&ucfg );
  remove_sfr_register(&uep0 );
  remove_sfr_register(&uep1 );
  remove_sfr_register(&uep2 );
  remove_sfr_register(&uep3 );
  remove_sfr_register(&uep4 );
  remove_sfr_register(&uep5 );
  remove_sfr_register(&uep6 );
  remove_sfr_register(&uep7 );
  remove_sfr_register(&uep8 );
  remove_sfr_register(&uep9 );
  remove_sfr_register(&uep10);
  remove_sfr_register(&uep11);
  remove_sfr_register(&uep12);
  remove_sfr_register(&uep13);
  remove_sfr_register(&uep14);
  remove_sfr_register(&uep15);
  remove_sfr_register(&sppcon);
  remove_sfr_register(&sppcfg);
  remove_sfr_register(&sppeps);
  remove_sfr_register(&sppdata);
}

void P18F4455::create()
{
  P18F4x21::create();

  if(verbose)
    cout << " 18f4455 create \n";

  package->destroy_pin(18);
  package->assign_pin(18, 0, false);          // Vusb

  /* The MSSP/I2CC pins are different on this chip. */
  ssp.initialize(&pir_set_def,         // PIR
                 &(*m_portb)[1],       // SCK
                 &(*m_porta)[5],       // SS
                 &(*m_portc)[7],       // SDO
                 &(*m_portb)[0],       // SDI
                 m_trisb,              // i2c tris port
                 SSP_TYPE_MSSP
       );

  // RP: RRR commented out comparator.cmcon.set_eccpas(&eccpas);  ??
  // Streaming Parallel port (SPP)
  spp.initialize(&pir_set_def,         // PIR
		m_portd,		//Parallel port
		m_trisd,		//Parallel port tristate register
		&sppcon,
		&sppcfg,
		&sppeps,
		&sppdata,
		&(*m_porte)[0],		// CLK1SPP
		&(*m_porte)[1],		// CLK2SPP
		&(*m_porte)[2],		// OESPP
		&(*m_portb)[4]		// CSSPP
	);

  add_sfr_register(&sppdata,0x0F62, RegisterValue(0,0));
  add_sfr_register(&sppcfg,0x0F63, RegisterValue(0,0));
  add_sfr_register(&sppeps,0x0F64, RegisterValue(0,0));
  add_sfr_register(&sppcon,0x0F65, RegisterValue(0,0));
  add_sfr_register(&ufrml,0x0F66, RegisterValue(0,0),"ufrm");
  add_sfr_register(&ufrmh,0X0F67, RegisterValue(0,0));
  add_sfr_register(&uir  ,0x0F68, RegisterValue(0,0));
  add_sfr_register(&uie  ,0x0F69, RegisterValue(0,0));
  add_sfr_register(&ueir ,0x0F6A, RegisterValue(0,0));
  add_sfr_register(&ueie ,0x0F6B, RegisterValue(0,0));
  add_sfr_register(&ustat,0X0F6C, RegisterValue(0,0));
  add_sfr_register(&ucon ,0x0F6D, RegisterValue(0,0));
  add_sfr_register(&uaddr,0X0F6E, RegisterValue(0,0));
  add_sfr_register(&ucfg ,0x0F6F, RegisterValue(0,0));
  add_sfr_register(&uep0 ,0x0F70, RegisterValue(0,0));
  add_sfr_register(&uep1 ,0x0F71, RegisterValue(0,0));
  add_sfr_register(&uep2 ,0x0F72, RegisterValue(0,0));
  add_sfr_register(&uep3 ,0x0F73, RegisterValue(0,0));
  add_sfr_register(&uep4 ,0x0F74, RegisterValue(0,0));
  add_sfr_register(&uep5 ,0x0F75, RegisterValue(0,0));
  add_sfr_register(&uep6 ,0x0F76, RegisterValue(0,0));
  add_sfr_register(&uep7 ,0x0F77, RegisterValue(0,0));
  add_sfr_register(&uep8 ,0x0F78, RegisterValue(0,0));
  add_sfr_register(&uep9 ,0x0F79, RegisterValue(0,0));
  add_sfr_register(&uep10,0x0F7A, RegisterValue(0,0));
  add_sfr_register(&uep11,0x0F7B, RegisterValue(0,0));
  add_sfr_register(&uep12,0x0F7C, RegisterValue(0,0));
  add_sfr_register(&uep13,0x0F7D, RegisterValue(0,0));
  add_sfr_register(&uep14,0x0F7E, RegisterValue(0,0));
  add_sfr_register(&uep15,0x0F7F, RegisterValue(0,0));

  // Initialize the register cross linkages
  init_pir2(pir2, PIR2v4::TMR3IF);

  //new InterruptSource(pir2, PIR2v4::USBIF);
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
//
// P18F4550
// 

P18F4550::P18F4550(const char *_name, const char *desc)
  : P18F4x21(_name,desc),
      ufrml(this, "ufrml", "USB Frame Number register Low"      ),
      ufrmh(this, "ufrmh", "USB Frame Number register High"     ),
      uir  (this, "uir"  , "USB Interrupt Status register"      ),
      uie  (this, "uie"  , "USB Interrupt Enable register"      ),
      ueir (this, "ueir" , "USB Error Interrupt Status register"),
      ueie (this, "ueie" , "USB Error Interrupt Enable register"),
      ustat(this, "ustat", "USB Transfer Status register"       ),
      ucon (this, "ucon" , "USB Control register"               ),
      uaddr(this, "uaddr", "USB Device Address register"        ),
      ucfg (this, "ucfg" , "USB Configuration register"         ),
      uep0 (this, "uep0" , "USB Endpoint 0 Enable register"     ),
      uep1 (this, "uep1" , "USB Endpoint 1 Enable register"     ),
      uep2 (this, "uep2" , "USB Endpoint 2 Enable register"     ),
      uep3 (this, "uep3" , "USB Endpoint 3 Enable register"     ),
      uep4 (this, "uep4" , "USB Endpoint 4 Enable register"     ),
      uep5 (this, "uep5" , "USB Endpoint 5 Enable register"     ),
      uep6 (this, "uep6" , "USB Endpoint 6 Enable register"     ),
      uep7 (this, "uep7" , "USB Endpoint 7 Enable register"     ),
      uep8 (this, "uep8" , "USB Endpoint 8 Enable register"     ),
      uep9 (this, "uep9" , "USB Endpoint 9 Enable register"     ),
      uep10(this, "uep10", "USB Endpoint 10 Enable register"    ),
      uep11(this, "uep11", "USB Endpoint 11 Enable register"    ),
      uep12(this, "uep12", "USB Endpoint 12 Enable register"    ),
      uep13(this, "uep13", "USB Endpoint 13 Enable register"    ),
      uep14(this, "uep14", "USB Endpoint 14 Enable register"    ),
      uep15(this, "uep15", "USB Endpoint 15 Enable register"    ),
      sppcon(this, "sppcon", "Streaming Parallel port control register"),
      sppcfg(this, "sppcfg", "Streaming Parallel port configuration register"),
      sppeps(this, "sppeps", "SPP ENDPOINT ADDRESS AND STATUS REGISTER"),
      sppdata(this, "sppdata", "Streaming Parallel port data register")

{
  cout << "\nP18F4550 does not support USB registers and functionality\n\n";
  if(verbose)
    cout << "18f4550 constructor, type = " << isa() << '\n';
}

P18F4550::~P18F4550()
{
  remove_sfr_register(&ufrml);
  remove_sfr_register(&ufrmh);
  remove_sfr_register(&uir  );
  remove_sfr_register(&uie  );
  remove_sfr_register(&ueir );
  remove_sfr_register(&ueie );
  remove_sfr_register(&ustat);
  remove_sfr_register(&ucon );
  remove_sfr_register(&uaddr);
  remove_sfr_register(&ucfg );
  remove_sfr_register(&uep0 );
  remove_sfr_register(&uep1 );
  remove_sfr_register(&uep2 );
  remove_sfr_register(&uep3 );
  remove_sfr_register(&uep4 );
  remove_sfr_register(&uep5 );
  remove_sfr_register(&uep6 );
  remove_sfr_register(&uep7 );
  remove_sfr_register(&uep8 );
  remove_sfr_register(&uep9 );
  remove_sfr_register(&uep10);
  remove_sfr_register(&uep11);
  remove_sfr_register(&uep12);
  remove_sfr_register(&uep13);
  remove_sfr_register(&uep14);
  remove_sfr_register(&uep15);
  remove_sfr_register(&sppcon);
  remove_sfr_register(&sppcfg);
  remove_sfr_register(&sppeps);
  remove_sfr_register(&sppdata);
}

void P18F4550::create()
{
  P18F4x21::create();

  if(verbose)
    cout << " 18f4550 create \n";

  package->destroy_pin(18);
  package->assign_pin(18, 0, false);          // Vusb

  /* The MSSP/I2CC pins are different on this chip. */
  ssp.initialize(&pir_set_def,         // PIR
                 &(*m_portb)[1],       // SCK
                 &(*m_porta)[5],       // SS
                 &(*m_portc)[7],       // SDO
                 &(*m_portb)[0],       // SDI
                 m_trisb,              // i2c tris port
                 SSP_TYPE_MSSP
       );

  // Streaming Parallel port (SPP)
  spp.initialize(&pir_set_def,         // PIR
		m_portd,		//Parallel port
		m_trisd,		//Parallel port tristate register
		&sppcon,
		&sppcfg,
		&sppeps,
		&sppdata,
		&(*m_porte)[0],		// CLK1SPP
		&(*m_porte)[1],		// CLK2SPP
		&(*m_porte)[2],		// OESPP
		&(*m_portb)[4]		// CSSPP
	);

  // RP: RRR commented out comparator.cmcon.set_eccpas(&eccpas);  ??
  add_sfr_register(&sppdata,0x0F62, RegisterValue(0,0));
  add_sfr_register(&sppcfg,0x0F63, RegisterValue(0,0));
  add_sfr_register(&sppeps,0x0F64, RegisterValue(0,0));
  add_sfr_register(&sppcon,0x0F65, RegisterValue(0,0));
  add_sfr_register(&ufrml,0x0F66, RegisterValue(0,0),"ufrm");
  add_sfr_register(&ufrmh,0X0F67, RegisterValue(0,0));
  add_sfr_register(&uir  ,0x0F68, RegisterValue(0,0));
  add_sfr_register(&uie  ,0x0F69, RegisterValue(0,0));
  add_sfr_register(&ueir ,0x0F6A, RegisterValue(0,0));
  add_sfr_register(&ueie ,0x0F6B, RegisterValue(0,0));
  add_sfr_register(&ustat,0X0F6C, RegisterValue(0,0));
  add_sfr_register(&ucon ,0x0F6D, RegisterValue(0,0));
  add_sfr_register(&uaddr,0X0F6E, RegisterValue(0,0));
  add_sfr_register(&ucfg ,0x0F6F, RegisterValue(0,0));
  add_sfr_register(&uep0 ,0x0F70, RegisterValue(0,0));
  add_sfr_register(&uep1 ,0x0F71, RegisterValue(0,0));
  add_sfr_register(&uep2 ,0x0F72, RegisterValue(0,0));
  add_sfr_register(&uep3 ,0x0F73, RegisterValue(0,0));
  add_sfr_register(&uep4 ,0x0F74, RegisterValue(0,0));
  add_sfr_register(&uep5 ,0x0F75, RegisterValue(0,0));
  add_sfr_register(&uep6 ,0x0F76, RegisterValue(0,0));
  add_sfr_register(&uep7 ,0x0F77, RegisterValue(0,0));
  add_sfr_register(&uep8 ,0x0F78, RegisterValue(0,0));
  add_sfr_register(&uep9 ,0x0F79, RegisterValue(0,0));
  add_sfr_register(&uep10,0x0F7A, RegisterValue(0,0));
  add_sfr_register(&uep11,0x0F7B, RegisterValue(0,0));
  add_sfr_register(&uep12,0x0F7C, RegisterValue(0,0));
  add_sfr_register(&uep13,0x0F7D, RegisterValue(0,0));
  add_sfr_register(&uep14,0x0F7E, RegisterValue(0,0));
  add_sfr_register(&uep15,0x0F7F, RegisterValue(0,0));

  // Initialize the register cross linkages

  //new InterruptSource(pir2, PIR2v4::USBIF);
}

Processor * P18F4550::construct(const char *name)
{

  P18F4550 *p = new P18F4550(name);

  if(verbose)
    cout << " 18F4550 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();
  return p;
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

  tbl.initialize ( eeprom_memory_size(), 32, 4, CONFIG1L, false);
  tbl.set_intcon(&intcon);
  set_eeprom_pir(&tbl);
  tbl.set_pir(pir2);
  tbl.eecon1.set_valid_bits(0xbf);


  create_iopin_map();


  _16bit_processor::create();
  _16bit_v2_adc::create(7);
  add_sfr_register(&osccon, 0xfd3, RegisterValue(0x00,0), "osccon");
  usart.txsta.setIOpin(&(*m_portb)[1]);
  usart.rcsta.setIOpin(&(*m_portb)[4]);
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

  add_sfr_register(&usart.spbrgh,   0xfb0,RegisterValue(0,0),"spbrgh");
  add_sfr_register(&usart.baudcon,  0xfaa,RegisterValue(0,0),"baudctl");
  usart.set_eusart(true);

  add_sfr_register(&pwm1con, 0xfb7, RegisterValue(0,0));
  add_sfr_register(&eccpas, 0xfb6, RegisterValue(0,0));
  ccp1con.setBitMask(0xff);
  ccp1con.setCrosslinks(&ccpr1l, &pir1, PIR1v2::CCP1IF, &tmr2, &eccpas);
  eccpas.setIOpin(&(*m_portb)[1], &(*m_portb)[2], &(*m_portb)[0]);
  eccpas.link_registers(&pwm1con, &ccp1con);
  ccp1con.pwm1con = &pwm1con;
  ccp1con.setIOpin(&((*m_portb)[3]), &((*m_portb)[2]), &((*m_portb)[6]), &((*m_portb)[7]));
  init_pir2(pir2, PIR2v2::TMR3IF);
  tmr3l.setIOpin(&(*m_portb)[6]);
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
  : _16bit_v2_adc(_name,desc),
    osctune(this, "osctune", "OSC Tune"),
    eccpas(this, "eccpas", "ECCP Auto-Shutdown Control Register"),
    pwm1con(this, "pwm1con", "Enhanced PWM Control Register")

{

  if(verbose)
    cout << "18F1220 constructor, type = " << isa() << '\n';


}

P18F1220::~P18F1220()
{
  remove_sfr_register(&usart.spbrgh);
  remove_sfr_register(&usart.baudcon);
  remove_sfr_register(&osctune);
  remove_sfr_register(&pwm1con);
  remove_sfr_register(&eccpas);
}

void P18F1220::osc_mode(unsigned int value)
{
  IOPIN *m_pin;
  unsigned int pin_Number =  get_osc_pin_Number(0);
  
  value &= (FOSC3 | FOSC2 | FOSC1 | FOSC0);
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

  delete pir2;
  pir2 = (PIR2v2 *)(new PIR2v4(this, "pir2" , "Peripheral Interrupt Register",0,0  ));

  tbl.initialize ( eeprom_memory_size(), 32, 4, CONFIG1L, false);
  tbl.set_intcon(&intcon);
  set_eeprom_pir(&tbl);
  tbl.set_pir(pir2);
  tbl.eecon1.set_valid_bits(0xbf);
  create_iopin_map();

  _16bit_processor::create();
  add_sfr_register(&osccon, 0xfd3, RegisterValue(0x40,0), "osccon");

  m_configMemory->addConfigWord(CONFIG3H-CONFIG1L,new Config3H_2x21(this, CONFIG3H, 0x83));
  m_configMemory->addConfigWord(CONFIG1H-CONFIG1L,new Config1H_4bits(this, CONFIG1H, 0x07));

  set_osc_pin_Number(0, 9, &(*m_porta)[7]);
  set_osc_pin_Number(1,10, &(*m_porta)[6]);



  /// @bug registers not present on 28 pin according to table 5-1 of the
  /// data sheet, but bit-restricted according to section 16.4.7
  add_sfr_register(&pwm1con, 0xfb7, RegisterValue(0,0));
  add_sfr_register(&eccpas, 0xfb6, RegisterValue(0,0));  

  eccpas.setBitMask(0xfc);
  eccpas.setIOpin(0, 0, &(*m_portb)[0]);
  eccpas.link_registers(&pwm1con, &ccp1con);
  comparator.cmcon.set_eccpas(&eccpas);
  ccp1con.setBitMask(0x3f);
  ccp1con.setCrosslinks(&ccpr1l, &pir1, PIR1v2::CCP1IF, &tmr2, &eccpas);
  ccp1con.pwm1con = &pwm1con;
  ccp1con.setIOpin(&((*m_portc)[2]), 0, 0, 0);
  pwm1con.setBitMask(0x80);
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
    cout << "18F2x21 constructor, type = " << isa() << '\n';

    m_porte = new PicPortRegister(this,"porte","",8,0x08);
    // No TRIS register for port E on 28-pin devices
}

P18F2x21::~P18F2x21()
{
    delete_sfr_register(m_porte);
    remove_sfr_register(&usart.spbrgh);
    remove_sfr_register(&usart.baudcon);
    remove_sfr_register(&osctune);
    remove_sfr_register(&comparator.cmcon);
    remove_sfr_register(&comparator.vrcon);
    remove_sfr_register(&pwm1con);
    remove_sfr_register(&eccpas);
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
	&(*m_porta)[1], &(*m_porta)[2], &(*m_porta)[3], &(*m_porta)[4],
	&(*m_porta)[5]);

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

  ccp2con.setCrosslinks(&ccpr2l, pir2, PIR2v2::CCP2IF, &tmr2);
//  ccp2con.setIOpin(&((*m_portc)[1]));     // handled by Config3H_2x21
  ccpr2l.ccprh  = &ccpr2h;
  ccpr2l.tmrl   = &tmr1l;
  ccpr2h.ccprl  = &ccpr2l;

  //1 usart16.initialize_16(this,&pir_set_def,&portc);
  add_sfr_register(&usart.spbrgh,   0xfb0,porv,"spbrgh");
  add_sfr_register(&usart.baudcon,  0xfb8,porv,"baudcon");
  usart.set_eusart(true);
  init_pir2(pir2, PIR2v4::TMR3IF); 
  tmr3l.setIOpin(&(*m_portc)[0]);
}

void P18F2x21::osc_mode(unsigned int value)
{
  IOPIN *m_pin;
  unsigned int pin_Number =  get_osc_pin_Number(0);
  
  value &= (FOSC3 | FOSC2 | FOSC1 | FOSC0);
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



//------------------------------------------------------------------------
//
// P18F2221
// 

P18F2221::P18F2221(const char *_name, const char *desc)
  : P18F2x21(_name,desc)
{

  if(verbose)
    cout << "18F2221 constructor, type = " << isa() << '\n';

}


Processor * P18F2221::construct(const char *name)
{

  P18F2221 *p = new P18F2221(name);

  if(verbose)
    cout << " 18F2221 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();

  if(verbose&2)
    cout << " 18F2221 construct completed\n";
  return p;
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



//------------------------------------------------------------------------
//
// P18F2420
// 

P18F2420::P18F2420(const char *_name, const char *desc)
  : P18F2x21(_name,desc)
{

  if(verbose)
    cout << "18F2420 constructor, type = " << isa() << '\n';

}


Processor * P18F2420::construct(const char *name)
{

  P18F2420 *p = new P18F2420(name);

  if(verbose)
    cout << " 18F2420 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();

  if(verbose&2)
    cout << " 18F2420 construct completed\n";
  return p;
}



//------------------------------------------------------------------------
//
// P18F2520
// 

P18F2520::P18F2520(const char *_name, const char *desc)
  : P18F2x21(_name,desc)
{

  if(verbose)
    cout << "18F2520 constructor, type = " << isa() << '\n';

}


Processor * P18F2520::construct(const char *name)
{

  P18F2520 *p = new P18F2520(name);

  if(verbose)
    cout << " 18F2520 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();

  if(verbose&2)
    cout << " 18F2520 construct completed\n";
  return p;
}


//------------------------------------------------------------------------
//
// P18F2620
// 

P18F2620::P18F2620(const char *_name, const char *desc)
  : P18F2x21(_name,desc)
{

  if(verbose)
    cout << "18F2620 constructor, type = " << isa() << '\n';

}


Processor * P18F2620::construct(const char *name)
{

  P18F2620 *p = new P18F2620(name);

  if(verbose)
    cout << " 18F2620 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();

  if(verbose&2)
    cout << " 18F2620 construct completed\n";
  return p;
}

//------------------------------------------------------------------------
//
// P18F26K22
// 

//------------------------------------------------------------------------
void P18F26K22::create_iopin_map()
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

P18F26K22::P18F26K22(const char *_name, const char *desc)
  : _16bit_processor(_name,desc),
    osccon(this, "osccon", "Oscillator Control Register"),
    adcon0(this, "adcon0", "A2D control register 0"),
    adcon1(this, "adcon1", "A2D control register 1"),
    adcon2(this, "adcon2", "A2D control register 2"),
    vrefcon0(this, "vrefcon0", "Fixed Voltage Reference Control Register"),
    vrefcon1(this, "vrefcon1", "Voltage Reference Control Register 0", 0xed),
    vrefcon2(this, "vrefcon2", "Voltage Reference Control Register 1",0x1f, &vrefcon1),
    eccp1as(this, "eccp1as", "ECCP 1 Auto-Shutdown Control Register"),
    eccp2as(this, "eccp2as", "ECCP 2 Auto-Shutdown Control Register"),
    eccp3as(this, "eccp3as", "ECCP 3 Auto-Shutdown Control Register"),
    pwm1con(this, "pwm1con", "Enhanced PWM 1 Control Register"),
    pwm2con(this, "pwm2con", "Enhanced PWM 2 Control Register"),
    pwm3con(this, "pwm3con", "Enhanced PWM 3 Control Register"),
    osctune(this, "osctune", "OSC Tune"),
    t1gcon(this, "t1gcon", "Timer 1 Gate Control Register"),
    t3gcon(this, "t3gcon", "Timer 3 Gate Control Register"), 
    tmr5l(this, "tmr5l", "TMR5 Low "),
    tmr5h(this, "tmr5h", "TMR5 High"),
    t5gcon(this, "t5gcon", "Timer 5 Gate Control Register"), 
    t4con(this, "t4con", "TMR4 Control"), 
    pr4(this, "pr4", "TMR4 Period Register"), 
    tmr4(this, "tmr4", "TMR4 Register"), 
    t6con(this, "t6con", "TMR6 Control"),
    pr6(this, "pr6", "TMR6 Period Register"),
    tmr6(this, "tmr6", "TMR6 Register"),
    pir3(this,"pir3","Peripheral Interrupt Register",0,0),
    pie3(this, "pie3", "Peripheral Interrupt Enable"),
    pir4(this,"pir4","Peripheral Interrupt Register 4",0,0),
    pie4(this, "pie4", "Peripheral Interrupt Enable 4"),
    pir5(this,"pir5","Peripheral Interrupt Register 5",0,0),
    pie5(this, "pie5", "Peripheral Interrupt Enable 5"),
    ipr3(this, "ipr3", "Interrupt Priorities 3"),
    ipr4(this, "ipr4", "Interrupt Priorities 4"),
    ipr5(this, "ipr5", "Interrupt Priorities 5"),
    ccp3con(this, "ccp3con", "Enhanced Capture Compare Control"),
    ccpr3l(this, "ccpr3l", "Capture Compare 3 Low"),
    ccpr3h(this, "ccpr3h", "Capture Compare 3 High"),
    ccp4con(this, "ccp4con", "Capture Compare Control"),
    ccpr4l(this, "ccpr4l", "Capture Compare 4 Low"),
    ccpr4h(this, "ccpr4h", "Capture Compare 4 High"),
    ccp5con(this, "ccp5con", "Capture Compare Control"),
    ccpr5l(this, "ccpr5l", "Capture Compare 5 Low"),
    ccpr5h(this, "ccpr5h", "Capture Compare 5 High"),
    usart2(this), 
    comparator(this),
    pmd0(this, "pmd0", "Peripheral Module Disable 0"),
    pmd1(this, "pmd1", "Peripheral Module Disable 1"),
    pmd2(this, "pmd2", "Peripheral Module Disable 2"),
    ansela(this, "ansela", "PortA Analog Select Register"),
    anselb(this, "anselb", "PortB Analog Select Register"),
    anselc(this, "anselc", "PortC Analog Select Register"),
    slrcon(this, "slrcon", "Slew Rate Control Register", 0x1f),
    ccptmrs(this),
    pstr1con(this, "pstr1con", "PWM Steering Control Register 1"),
    pstr2con(this, "pstr2con", "PWM Steering Control Register 2"),
    pstr3con(this, "pstr3con", "PWM Steering Control Register 3"),
    sr_module(this),
    ssp1(this),
    ssp2(this),
    ctmu(this),
    hlvdcon(this, "hlvdcon", "High/Low-Voltage Detect Register")
{

  if(verbose)
    cout << "18F26K22 constructor, type = " << isa() << '\n';

  delete pir2;
  pir2 = (PIR2v2 *)(new PIR2v4(this, "pir2" , "Peripheral Interrupt Register",0,0  ));
  wpub = new WPU(this, "wpub", "Weak Pull-Up Portb Register", m_portb, 0xff);
  iocb = new IOC(this, "iocb", "Interrupt-On-Change Portb Control Register", 0xf0);
  m_porte = new PicPortRegister(this,"porte","",8,0xFF);
  m_trise = new PicTrisRegister(this,"trise","", m_porte, false);
  m_late  = new PicLatchRegister(this,"late","",m_porte);
  delete t1con;
  t1con = new T5CON(this, "t1con", "Timer 1 Control Register");
  t3con2 = new T5CON(this, "t3con", "Timer 3 Control Register");
  t5con = new T5CON(this, "t5con", "Timer 5 Control Register"); 
  pir_set_def.set_pir3(&pir3);
  pir_set_def.set_pir4(&pir4);
  pir_set_def.set_pir5(&pir5);

  // By default TMR2 controls deals with all ccp units until
  // changed by ccptmrsx registers
  tmr2.add_ccp(&ccp3con);
  tmr2.add_ccp(&ccp4con);
  tmr2.add_ccp(&ccp5con);
  tmr2.m_txgcon = &t1gcon;
  t4con.tmr2 = &tmr4;
  tmr4.pr2 = &pr4;
  tmr4.t2con = &t4con;
  tmr4.setInterruptSource(new InterruptSource(&pir5, PIR5v1::TMR4IF));
  tmr4.m_txgcon = &t3gcon;
  pr4.tmr2 = &tmr4;
  t6con.tmr2 = &tmr6;
  tmr6.pr2 = &pr6;
  tmr6.t2con = &t6con;
  tmr6.setInterruptSource(new InterruptSource(&pir5, PIR5v1::TMR6IF));
  tmr6.m_txgcon = &t5gcon;
  pr6.tmr2 = &tmr6;
  ccptmrs.set_tmr246(&tmr2, &tmr4, &tmr6);
  ccptmrs.set_ccp(&ccp1con, &ccp2con, &ccp3con, &ccp4con, &ccp5con);
  comparator.cmxcon0[0] = new CMxCON0_V2(this, "cm1con0", 
	" Comparator C1 Control Register 0", 0, &comparator);
  comparator.cmxcon0[1] = new CMxCON0_V2(this, "cm2con0", 
	" Comparator C2 Control Register 0", 1, &comparator);
  comparator.cmxcon1[0] = new CM2CON1_V2(this, "cm2con1", 
	" Comparator Control Register 1",   &comparator);
  comparator.cmxcon1[1] = comparator.cmxcon1[0];

  ctmu.ctmuconh = new CTMUCONH(this, "ctmuconh", 
	"CTMU Control Register 0", &ctmu);
  ctmu.ctmuconl = new CTMUCONL(this, "ctmuconl", 
	"CTMU Control Register 1", &ctmu);
  ctmu.ctmuicon = new CTMUICON(this, "ctmuicon", 
	"CTMU Current Control Register", &ctmu);

  ctmu.ctmu_stim = new ctmu_stimulus(this,"ctmu_stim", 5.0, 1e12);
  adcon0.set_ctmu_stim(ctmu.ctmu_stim);
  ctmu.adcon1 = &adcon1;
  ctmu.cm2con1 = (CM2CON1_V2 *)comparator.cmxcon1[0];
  ctmu.set_IOpins(&(*m_portb)[2],&(*m_portb)[3], &(*m_portc)[2]);
  hlvdcon.setIntSrc(new InterruptSource(pir2, PIR2v2::HLVDIF));
  hlvdcon.set_hlvdin(&(*m_porta)[5]);
}

P18F26K22::~P18F26K22()
{
    delete ctmu.ctmu_stim;
    delete_sfr_register(m_porte);
    delete_sfr_register(m_late);
    delete_sfr_register(m_trise);
    delete_sfr_register(t3con2);
    delete_sfr_register(t5con);
    delete_sfr_register(usart2.txreg);
    delete_sfr_register(usart2.rcreg);
    remove_sfr_register(comparator.cmxcon0[0]);
    remove_sfr_register(comparator.cmxcon0[1]);
    remove_sfr_register(comparator.cmxcon1[0]);

    remove_sfr_register(&usart.spbrgh);
    remove_sfr_register(&usart.baudcon);
    remove_sfr_register(&osctune);
    remove_sfr_register(&tmr2);
    remove_sfr_register(&pr2);
    remove_sfr_register(&t2con);
    remove_sfr_register(&pwm1con);
    remove_sfr_register(&eccp1as);
    remove_sfr_register(&pwm2con);
    remove_sfr_register(&eccp2as);
    remove_sfr_register(&pwm3con);
    remove_sfr_register(&eccp3as);
    remove_sfr_register(&ccpr2h);
    remove_sfr_register(&ccpr2l);
    remove_sfr_register(&ccp2con);
    remove_sfr_register(&ccpr3h);
    remove_sfr_register(&ccpr3l);
    remove_sfr_register(&ccp3con);
    remove_sfr_register(&ccpr4h);
    remove_sfr_register(&ccpr4l);
    remove_sfr_register(&ccp4con);
    remove_sfr_register(&ccpr5h);
    remove_sfr_register(&ccpr5l);
    remove_sfr_register(&ccp5con);
    remove_sfr_register(&osccon);
    remove_sfr_register(&ipr3);
    remove_sfr_register(&pir3);
    remove_sfr_register(&pie3);
    remove_sfr_register(&pie4);
    remove_sfr_register(&pir4);
    remove_sfr_register(&ipr4);
    remove_sfr_register(&pie5);
    remove_sfr_register(&pir5);
    remove_sfr_register(&ipr5);
    remove_sfr_register(&tmr5h);
    remove_sfr_register(&tmr5l);
    remove_sfr_register(&t1gcon);
    remove_sfr_register(&t3gcon);
    remove_sfr_register(&t5gcon);
    remove_sfr_register(&pmd0);
    remove_sfr_register(&pmd1);
    remove_sfr_register(&pmd2);
    remove_sfr_register(&ansela);
    remove_sfr_register(&anselb);
    remove_sfr_register(&anselc);
    delete_sfr_register(wpub);
    delete_sfr_register(iocb);
    remove_sfr_register(&slrcon);
    remove_sfr_register(&ccptmrs.ccptmrs1);
    remove_sfr_register(&ccptmrs.ccptmrs0);
    remove_sfr_register(&tmr6);
    remove_sfr_register(&pr6);
    remove_sfr_register(&t6con);
    remove_sfr_register(&tmr4);
    remove_sfr_register(&pr4);
    remove_sfr_register(&t4con);
    remove_sfr_register(&adcon0);
    remove_sfr_register(&adcon1);
    remove_sfr_register(&adcon2);
    remove_sfr_register(&vrefcon0);
    remove_sfr_register(&vrefcon1);
    remove_sfr_register(&vrefcon2);
    remove_sfr_register(&sr_module.srcon0);
    remove_sfr_register(&sr_module.srcon1);
    remove_sfr_register(&pstr1con);
    remove_sfr_register(&pstr2con);
    remove_sfr_register(&pstr3con);
    remove_sfr_register(&usart2.rcsta);
    remove_sfr_register(&usart2.txsta);
    remove_sfr_register(&usart2.spbrg);
    remove_sfr_register(&usart2.spbrgh);
    remove_sfr_register(&usart2.baudcon);
    remove_sfr_register(&ssp1.sspbuf);
    remove_sfr_register(&ssp1.sspadd);
    remove_sfr_register(&ssp1.ssp1msk);
    remove_sfr_register(&ssp1.sspstat);
    remove_sfr_register(&ssp1.sspcon);
    remove_sfr_register(&ssp1.sspcon2);
    remove_sfr_register(&ssp1.ssp1con3);
    remove_sfr_register(&ssp2.sspbuf);
    remove_sfr_register(&ssp2.sspadd);
    remove_sfr_register(&ssp2.ssp1msk);
    remove_sfr_register(&ssp2.sspstat);
    remove_sfr_register(&ssp2.sspcon);
    remove_sfr_register(&ssp2.sspcon2);
    remove_sfr_register(&ssp2.ssp1con3);
    delete_sfr_register(ctmu.ctmuconh);
    delete_sfr_register(ctmu.ctmuconl);
    delete_sfr_register(ctmu.ctmuicon);
    remove_sfr_register(&hlvdcon);


    delete_file_registers(0xf3b, 0xf3c, false);
    delete_file_registers(0xf83, 0xf83, false);
    delete_file_registers(0xf85, 0xf88, false);
    delete_file_registers(0xf8c, 0xf91, false);
    delete_file_registers(0xf95, 0xf95, false);
    delete_file_registers(0xf97, 0xf9a, false);
    //delete_file_registers(0xf9d, 0xf9e, false);
    delete_file_registers(0xfb5, 0xfb5, false);
    delete_file_registers(0xfd4, 0xfd4, false);
}

void P18F26K22::create()
{
  RegisterValue porv(0,0);
  RegisterValue porvh(0xff,0);

  if(verbose)
    cout << "P18F26K22::create\n";

  tbl.initialize ( eeprom_memory_size(), 32, 4, CONFIG1L);
  tbl.set_intcon(&intcon);
  set_eeprom_pir(&tbl);
  tbl.set_pir(pir2);
  tbl.eecon1.set_valid_bits(0xbf);

  create_iopin_map();
  
  _16bit_processor::create();
  remove_sfr_register(&ssp.sspcon2);
  remove_sfr_register(&ssp.sspcon);
  remove_sfr_register(&ssp.sspstat);
  remove_sfr_register(&ssp.sspadd);
  remove_sfr_register(&ssp.sspbuf);

  set_osc_pin_Number(0, 9, &(*m_porta)[7]);
  set_osc_pin_Number(1,10, &(*m_porta)[6]);

  m_configMemory->addConfigWord(CONFIG1L-CONFIG1L,new ConfigWord("CONFIG1L", 0x00, "Configuration Register 1 low", this, CONFIG1L));
  m_configMemory->addConfigWord(CONFIG1H-CONFIG1L,new Config1H_4bits(this, CONFIG1H, 0x25));
  m_configMemory->addConfigWord(CONFIG3H-CONFIG1L,new Config3H(this, CONFIG3H, 0xbf));


  add_sfr_register(&osccon,     0xfd3, RegisterValue(0x30,0), "osccon");

  add_sfr_register(&t1gcon,     0xfcc, porv, "t1gcon");
  add_sfr_register(&ssp1.ssp1con3, 0xfcb, RegisterValue(0,0),"ssp1con3");
  add_sfr_register(&ssp1.ssp1msk, 0xfca, RegisterValue(0xff,0),"ssp1msk");
  add_sfr_register(&ssp1.sspbuf,  0xfc9, RegisterValue(0,0),"ssp1buf");
  add_sfr_register(&ssp1.sspadd,  0xfc8, RegisterValue(0,0),"ssp1add");
  add_sfr_register(&ssp1.sspstat, 0xfc7, RegisterValue(0,0),"ssp1stat");
  add_sfr_register(&ssp1.sspcon,  0xfc6, RegisterValue(0,0),"ssp1con");
  add_sfr_register(&ssp1.sspcon2, 0xfc5, RegisterValue(0,0),"ssp1con2");

  add_sfr_register(&adcon0,     0xfc2, porv, "adcon0");
  add_sfr_register(&adcon1,     0xfc1, porv, "adcon1");
  add_sfr_register(&adcon2,     0xfc0, porv, "adcon2");

  add_sfr_register(&tmr2,       0xfbc, porv, "tmr2");
  add_sfr_register(&pr2,        0xfbb, RegisterValue(0xff,0), "pr2");
  add_sfr_register(&t2con,      0xfba, porv, "t2con");
  add_sfr_register(&pstr1con,   0xfb9, RegisterValue(0x01,0));
  add_sfr_register(&pwm1con,    0xfb7, porv);
  add_sfr_register(&eccp1as,    0xfb6, porv);  
  add_sfr_register(&t3gcon,     0xfb4, porv);  

  add_sfr_register(&ipr3,       0xfa5, porv, "ipr3");
  add_sfr_register(&pir3,       0xfa4, porv, "pir3");
  add_sfr_register(&pie3,       0xfa3, porv, "pie3");

  add_sfr_register(&hlvdcon,    0xf9c, porv, "hlvdcon");

  add_sfr_register(&ipr5,       0xf7f, porv, "ipr5");
  add_sfr_register(&pir5,       0xf7e, porv, "pir5");
  add_sfr_register(&pie5,       0xf7d, porv, "pie5");
  add_sfr_register(&ipr4,       0xf7c, porv, "ipr4");
  add_sfr_register(&pir4,       0xf7b, porv, "pir4");
  add_sfr_register(&pie4,       0xf7a, porv, "pie4");
  add_sfr_register(comparator.cmxcon0[0],       0xf79, RegisterValue(0x08,0), "cm1con0");
  add_sfr_register(comparator.cmxcon0[1],       0xf78, RegisterValue(0x08,0), "cm2con0");
  add_sfr_register(comparator.cmxcon1[0],       0xf77, porv, "cm2con1");


  add_sfr_register(&ssp2.sspbuf,  0xf6f, RegisterValue(0,0),"ssp2buf");
  add_sfr_register(&ssp2.sspadd,  0xf6e, RegisterValue(0,0),"ssp2add");
  add_sfr_register(&ssp2.sspstat, 0xf6d, RegisterValue(0,0),"ssp2stat");
  add_sfr_register(&ssp2.sspcon,  0xf6c, RegisterValue(0,0),"ssp2con");
  add_sfr_register(&ssp2.sspcon2, 0xf6b, RegisterValue(0,0),"ssp2con2");
  add_sfr_register(&ssp2.ssp1msk, 0xf6a, RegisterValue(0xff,0),"ssp2msk");
  add_sfr_register(&ssp2.ssp1con3, 0xf69, RegisterValue(0,0),"ssp2con3");
  add_sfr_register(&ccpr2h,     0xf68, porv, "ccpr2h");
  add_sfr_register(&ccpr2l,     0xf67, porv, "ccpr2l");
  add_sfr_register(&ccp2con,    0xf66, porv, "ccp2con");
  add_sfr_register(&pwm2con,    0xf65, porv);
  add_sfr_register(&eccp2as,    0xf64, porv);  
  add_sfr_register(&pstr2con,   0xf63, RegisterValue(0x01,0));
  add_sfr_register(iocb,       0xf62, porvh);
  add_sfr_register(wpub,       0xf61, porvh);
  add_sfr_register(&slrcon,     0xf60, porvh);

  add_sfr_register(&ccpr3h,     0xf5f, porv);  
  add_sfr_register(&ccpr3l,     0xf5e, porv);  
  add_sfr_register(&ccp3con,    0xf5d, porv);  
  add_sfr_register(&pwm3con,    0xf5c, porv);
  add_sfr_register(&eccp3as,    0xf5b, porv);  
  add_sfr_register(&pstr3con,   0xf5a, RegisterValue(0x01,0));
  add_sfr_register(&ccpr4h,     0xf59, porv);  
  add_sfr_register(&ccpr4l,     0xf58, porv);  
  add_sfr_register(&ccp4con,    0xf57, porv);  
  add_sfr_register(&ccpr5h,     0xf56, porv);  
  add_sfr_register(&ccpr5l,     0xf55, porv);  
  add_sfr_register(&ccp5con,    0xf54, porv);  
  add_sfr_register(&tmr4,       0xf53, porv);
  add_sfr_register(&pr4,        0xf52, porvh);
  add_sfr_register(&t4con,      0xf51, porv);
  add_sfr_register(&tmr5h,      0xf50, porv, "tmr5h");  

  add_sfr_register(&tmr5l,      0xf4f, porv, "tmr5l");  
  add_sfr_register(t5con,       0xf4e, porv);  
  add_sfr_register(&t5gcon,     0xf4d, porv);  
  add_sfr_register(&tmr6,       0xf4c, porv);
  add_sfr_register(&pr6,        0xf4b, porvh);
  add_sfr_register(&t6con,      0xf4a, porv);
  add_sfr_register(&ccptmrs.ccptmrs0, 0xf49, porv);
  add_sfr_register(&ccptmrs.ccptmrs1, 0xf48, porv);
  add_sfr_register(&sr_module.srcon0, 0xf47, porv);
  add_sfr_register(&sr_module.srcon1, 0xf46, porv);
  add_sfr_register(ctmu.ctmuconh, 0xf45, porv, "ctmuconh");
  add_sfr_register(ctmu.ctmuconl, 0xf44, porv, "ctmuconl");
  add_sfr_register(ctmu.ctmuicon, 0xf43, porv, "ctmuicon");
  add_sfr_register(&vrefcon0,   0xf42, RegisterValue(0x10,0));
  add_sfr_register(&vrefcon1,   0xf41, porv);
  add_sfr_register(&vrefcon2,   0xf40, porv);

  add_sfr_register(&pmd0,       0xf3f, porv);  
  add_sfr_register(&pmd1,       0xf3e, porv);  
  add_sfr_register(&pmd2,       0xf3d, porv);  
  add_sfr_register(&anselc,     0xf3a, RegisterValue(0xfc,0));
  add_sfr_register(&anselb,     0xf39, RegisterValue(0x3f,0));
  add_sfr_register(&ansela,     0xf38, RegisterValue(0x2f,0));

  add_sfr_register(new RegZero(this, "ZeroFD4", "Always read as zero"),    0xFD4, porv);
  add_sfr_register(new RegZero(this, "ZeroFB5", "Always read as zero"),    0xFB5, porv);
  add_sfr_register(new RegZero(this, "ZeroF9A", "Always read as zero"),    0xF9A, porv);
  add_sfr_register(new RegZero(this, "ZeroF99", "Always read as zero"),    0xF99, porv);
  add_sfr_register(new RegZero(this, "ZeroF98", "Always read as zero"),    0xF98, porv);
  add_sfr_register(new RegZero(this, "ZeroF97", "Always read as zero"),    0xF97, porv);
  add_sfr_register(new RegZero(this, "trisd", "Always read as zero"),    0xF95, porv);
  add_sfr_register(new RegZero(this, "ZeroF91", "Always read as zero"),    0xF91, porv);
  add_sfr_register(new RegZero(this, "ZeroF90", "Always read as zero"),    0xF90, porv);
  add_sfr_register(new RegZero(this, "ZeroF8F", "Always read as zero"),    0xF8F, porv);
  add_sfr_register(new RegZero(this, "ZeroF8E", "Always read as zero"),    0xF8E, porv);
  add_sfr_register(new RegZero(this, "late", "Always read as zero"),    0xF8D, porv);
  add_sfr_register(new RegZero(this, "latd", "Always read as zero"),    0xF8C, porv);
  add_sfr_register(new RegZero(this, "ZeroF88", "Always read as zero"),    0xF88, porv);
  add_sfr_register(new RegZero(this, "ZeroF87", "Always read as zero"),    0xF87, porv);
  add_sfr_register(new RegZero(this, "ZeroF86", "Always read as zero"),    0xF86, porv);
  add_sfr_register(new RegZero(this, "ZeroF85", "Always read as zero"),    0xF85, porv);
  add_sfr_register(new RegZero(this, "portd", "Always read as zero"),    0xF83, porv);
  add_sfr_register(new RegZero(this, "ansele", "Always read as zero"),    0xF3C, porv);
  add_sfr_register(new RegZero(this, "anseld", "Always read as zero"),    0xF3B, porv);

  eccp1as.setBitMask(0xfc);
  eccp2as.setBitMask(0xfc);
  eccp3as.setBitMask(0xfc);
  // ECCP shutdown trigger
  eccp1as.setIOpin(0, 0, &(*m_portb)[0]);
  eccp2as.setIOpin(0, 0, &(*m_portb)[0]);
  eccp3as.setIOpin(0, 0, &(*m_portb)[0]);
  eccp1as.link_registers(&pwm1con, &ccp1con);
  eccp2as.link_registers(&pwm2con, &ccp2con);
  eccp3as.link_registers(&pwm3con, &ccp3con);
 //RRR comparator.cmcon.set_eccpas(&eccp1as);
  ccp1con.setBitMask(0xff);
  ccp2con.setBitMask(0xff);
  ccp3con.setBitMask(0xff);
  ccp4con.setBitMask(0x3f);
  ccp5con.setBitMask(0x3f);
  ccp1con.setCrosslinks(&ccpr1l, &pir1, PIR1v2::CCP1IF, &tmr2, &eccp1as);
  ccp2con.setCrosslinks(&ccpr2l, pir2, PIR2v2::CCP2IF, &tmr2, &eccp2as);
  ccp3con.setCrosslinks(&ccpr3l, &pir4, PIR4v1::CCP3IF, &tmr2, &eccp3as);
  ccp1con.pwm1con = &pwm1con;
  ccp2con.pwm1con = &pwm2con;
  ccp3con.pwm1con = &pwm3con;
  ccp1con.pstrcon = &pstr1con;
  ccp2con.pstrcon = &pstr2con;
  ccp3con.pstrcon = &pstr3con;
  ccp1con.setIOpin(&((*m_portc)[2]), &((*m_portb)[2]), &((*m_portb)[1]), &((*m_portb)[4]));
  pwm1con.setBitMask(0x80);
  //ccp3con.setCrosslinks(&ccpr3l, &pir3, PIR3v2::CCP3IF, &tmr6, &eccp3as);
  adcon0.setAdresLow(&adresl);
  adcon0.setAdres(&adresh);
  adcon0.setAdcon1(&adcon1);
  adcon0.setAdcon2(&adcon2);
  adcon0.setIntcon(&intcon);
  adcon0.setPir(&pir1);
  adcon0.setChannel_Mask(0x1f); // upto 32 channels
  adcon0.setA2DBits(10);
  adcon1.setNumberOfChannels(20); 
  adcon1.setVrefHiChannel(3);
  adcon1.setVrefLoChannel(2);
  adcon1.setAdcon0(&adcon0);	// VCFG0, VCFG1 in adcon0
  vrefcon0.set_adcon1(&adcon1);
  vrefcon1.set_adcon1(&adcon1);
  vrefcon0.set_daccon0(&vrefcon1);


  ansela.setIOPin(0,  &(*m_porta)[0], &adcon1);
  ansela.setIOPin(1,  &(*m_porta)[1], &adcon1);
  ansela.setIOPin(2,  &(*m_porta)[2], &adcon1);
  ansela.setIOPin(3,  &(*m_porta)[3], &adcon1);
  ansela.setIOPin(4,  &(*m_porta)[5], &adcon1);
  anselb.setIOPin(8,  &(*m_portb)[2], &adcon1);
  anselb.setIOPin(9,  &(*m_portb)[3], &adcon1);
  anselb.setIOPin(10, &(*m_portb)[1], &adcon1);
  anselb.setIOPin(11, &(*m_portb)[4], &adcon1);
  anselb.setIOPin(12, &(*m_portb)[0], &adcon1);
  anselb.setIOPin(13, &(*m_portb)[5], &adcon1);
  anselc.setIOPin(14, &(*m_portc)[2], &adcon1);
  anselc.setIOPin(15, &(*m_portc)[3], &adcon1);
  anselc.setIOPin(16, &(*m_portc)[4], &adcon1);
  anselc.setIOPin(17, &(*m_portc)[5], &adcon1);
  anselc.setIOPin(18, &(*m_portc)[6], &adcon1);
  anselc.setIOPin(19, &(*m_portc)[7], &adcon1);


}
void P18F26K22::set_config3h(gint64 value)
{
    PinModule *p2b;
    (value & MCLRE) ? assignMCLRPin(1) : unassignMCLRPin();
    if ( value & P2BMX)
	p2b = &((*m_portb)[5]);
    else
	p2b = &((*m_portc)[0]);

    if ( value & CCP3MX )
         ccp3con.setIOpin(&((*m_portb)[5]), &((*m_portc)[5]));
    else
         ccp3con.setIOpin(&((*m_portc)[6]), &((*m_portc)[5]));

    if ( value & CCP2MX )
         ccp2con.setIOpin(&((*m_portc)[1]), p2b);
    else
         ccp2con.setIOpin(&((*m_portb)[3]), p2b);

    if ( value & PBADEN )
	anselb.por_value=RegisterValue( 0x3f,0);
    else
	anselb.por_value=RegisterValue(0,0);

}

void P18F26K22::osc_mode(unsigned int value)
{
  unsigned int mode = value & (FOSC3 | FOSC2 | FOSC1 | FOSC0);
  unsigned int pin_Number0 =  get_osc_pin_Number(0);
  unsigned int pin_Number1 =  get_osc_pin_Number(1);

  
  if (mode == 0x8 || mode == 0x9)
      set_int_osc(true);
  else
      set_int_osc(false);

  switch(mode)
  {
  case 0xf:	// external RC CLKOUT RA6
  case 0xe:
  case 0xc:
  case 0xa:
  case 0x9:
  case 0x6:
  case 0x4:
    if (pin_Number1 < 253)  // CLKO = OSC2
    {
        cout << "CLKO not simulated\n";
        set_clk_pin(pin_Number1, get_osc_PinMonitor(1) , "CLKO", 
	    false, m_porta, m_trisa, m_lata);
    }
    break;
  }
  if (mode != 0x9 && mode != 0x8 && pin_Number0 < 253) // not internal OSC, set OSC1
  {
        set_clk_pin(pin_Number0, get_osc_PinMonitor(0) , "OSC1", 
	    true, m_porta, m_trisa, m_lata);
  }
  if (mode < 4 && pin_Number1 < 253)
  {
        set_clk_pin(pin_Number1, get_osc_PinMonitor(1) , "OSC2", 
	    true, m_porta, m_trisa, m_lata);
  }
  
}

void P18F26K22::update_vdd()
{
    hlvdcon.check_hlvd();
    Processor::update_vdd();
}

Processor * P18F26K22::construct(const char *name)
{

  P18F26K22 *p = new P18F26K22(name);

  if(verbose)
    cout << " 18F26K22 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();

  if(verbose&2)
    cout << " 18F26K22 construct completed\n";
  return p;
}


void P18F26K22::create_sfr_map()
{

  if(verbose)
    cout << "create_sfr_map P18F26K22\n";

  _16bit_processor::create_sfr_map();


  RegisterValue porv(0,0);

  add_sfr_register(m_porte,       0xf84,porv);
  add_sfr_register(m_trise,       0xf96,RegisterValue(0x07,0));

  remove_sfr_register(t3con);
  add_sfr_register(t3con2,      0xfb1,porv);
  add_sfr_register(&osctune,      0xf9b,porv);
  osccon.set_osctune(&osctune);
  osctune.set_osccon(&osccon);

  comparator.cmxcon1[0]->set_OUTpin(&(*m_porta)[4], &(*m_porta)[5]);
  comparator.cmxcon1[0]->set_INpinNeg(&(*m_porta)[0], &(*m_porta)[1], 
					&(*m_portb)[3],&(*m_portb)[2]);
  comparator.cmxcon1[0]->set_INpinPos(&(*m_porta)[3], &(*m_porta)[2]);
  comparator.cmxcon1[0]->setBitMask(0x3f);
  comparator.cmxcon0[0]->setBitMask(0xbf);
  comparator.cmxcon0[0]->setIntSrc(new InterruptSource(pir2, PIR2v2::C1IF));
  comparator.cmxcon0[1]->setBitMask(0xbf);
  comparator.cmxcon0[1]->setIntSrc(new InterruptSource(pir2, PIR2v2::C2IF));
  vrefcon0.set_cmModule(&comparator);
  comparator.assign_t1gcon(&t1gcon, &t3gcon, &t5gcon);
  comparator.assign_sr_module(&sr_module);
  comparator.assign_eccpsas(&eccp1as, &eccp2as, &eccp3as);
  sr_module.srcon1.set_ValidBits(0xff);
  sr_module.setPins(&(*m_portb)[0], &(*m_porta)[4], &(*m_porta)[5]);

  vrefcon1.set_cmModule(&comparator);
  vrefcon1.setDACOUT(&(*m_porta)[2]);



  ccp2con.setCrosslinks(&ccpr2l, pir2, PIR2v2::CCP2IF, &tmr2);
  ccp2con.setIOpin(&((*m_portc)[1]));     // May be altered by Config3H_2x21::set
  ccpr2l.ccprh  = &ccpr2h;
  ccpr2l.tmrl   = &tmr1l;
  ccpr2h.ccprl  = &ccpr2l;
  ccp3con.setCrosslinks(&ccpr3l, &pir3, PIR3v1::CCP3IF, &tmr2);
  ccpr3l.ccprh  = &ccpr3h;
  ccpr3l.tmrl   = &tmr1l;
  ccpr3h.ccprl  = &ccpr3l;
  ccp4con.setCrosslinks(&ccpr4l, &pir3, PIR3v1::CCP4IF, &tmr2);
  ccp4con.setIOpin(&((*m_portb)[0]));
  ccpr4l.ccprh  = &ccpr4h;
  ccpr4l.tmrl   = &tmr1l;
  ccpr4h.ccprl  = &ccpr4l;
  ccp5con.setIOpin(&((*m_porta)[4]));
  ccp5con.setCrosslinks(&ccpr5l, &pir3, PIR3v1::CCP5IF, &tmr2);
  ccpr5l.ccprh  = &ccpr5h;
  ccpr5l.tmrl   = &tmr1l;
  ccpr5h.ccprl  = &ccpr5l;

  //1 usart16.initialize_16(this,&pir_set_def,&portc);
  add_sfr_register(&usart.spbrgh,   0xfb0,porv,"spbrgh");
  add_sfr_register(&usart.baudcon,  0xfb8,porv,"baudcon");
  usart.set_eusart(true);
  init_pir2(pir2, PIR2v4::TMR3IF); 
  tmr3l.setIOpin(&(*m_portc)[0]);

  pir3.set_intcon(&intcon);
  pir3.set_pie(&pie3);
  pir3.set_ipr(&ipr3);
  pie3.setPir(&pir3);

  pir4.set_intcon(&intcon);
  pir4.set_pie(&pie4);
  pir4.set_ipr(&ipr4);
  pie4.setPir(&pir4);

  pir5.set_intcon(&intcon);
  pir5.set_pie(&pie5);
  pir5.set_ipr(&ipr5);
  pie5.setPir(&pir5);

  ((T5CON *)t1con)->t1gcon = &t1gcon;
  t1gcon.setInterruptSource(new InterruptSource(&pir3, PIR3v3::TMR1GIF));
  t3gcon.setInterruptSource(new InterruptSource(&pir3, PIR3v3::TMR3GIF));
  t5gcon.setInterruptSource(new InterruptSource(&pir3, PIR3v3::TMR5GIF));

  t1gcon.set_tmrl(&tmr1l);
  t3gcon.set_tmrl(&tmr3l);
  t5gcon.set_tmrl(&tmr5l);
  t1gcon.setGatepin(&(*m_portb)[5]);
  t3gcon.setGatepin(&(*m_portc)[0]);
  t5gcon.setGatepin(&(*m_portb)[4]);

  t3con2->tmrl  = &tmr3l;
  t5con->tmrl  = &tmr5l;
  ((T5CON *)t3con2)->t1gcon = &t3gcon;
  ((T5CON *)t5con)->t1gcon = &t5gcon;
  tmr3l.setInterruptSource(new InterruptSource(pir2, PIR2v2::TMR3IF));
  tmr5l.setInterruptSource(new InterruptSource(&pir5, PIR5v1::TMR5IF));
  tmr5l.tmrh = &tmr5h;
  tmr5h.tmrl  = &tmr5l;
  tmr3l.t1con  = t3con2;
  tmr5l.t1con  = t5con;

  //cout << "Create second USART\n";
  usart2.initialize(&pir3,&(*m_portb)[6], &(*m_portb)[7],
                    new _TXREG(this,"txreg2", "USART Transmit Register", &usart2), 
                    new _RCREG(this,"rcreg2", "USART Receiver Register", &usart2));

  add_sfr_register(&usart2.baudcon,  0xf70, porv, "baudcon2");
  add_sfr_register(&usart2.rcsta,    0xf71, porv, "rcsta2");
  add_sfr_register(&usart2.txsta,    0xf72, RegisterValue(0x02,0), "txsta2");
  add_sfr_register(usart2.txreg,     0xf73, porv, "txreg2");
  add_sfr_register(usart2.rcreg,     0xf74, porv, "rcreg2");
  add_sfr_register(&usart2.spbrg,    0xf75, porv, "spbrg2");
  add_sfr_register(&usart2.spbrgh,   0xf76, porv, "spbrgh2");

  tmr2.ssp_module[0] = &ssp1;
  tmr2.ssp_module[1] = &ssp2;

  ssp1.initialize(
        0,    // PIR
        &(*m_portc)[3],   // SCK
        &(*m_porta)[5],   // SS
        &(*m_portc)[5],   // SDO
        &(*m_portc)[4],    // SDI
          m_trisc,          // i2c tris port
        SSP_TYPE_MSSP1
  );
  ssp1.mk_ssp_int(&pir1, PIR1v1::SSPIF);	// SSP1IF
  ssp1.mk_bcl_int(pir2, PIR2v2::BCLIF);	// BCL1IF

  ssp2.initialize(
        0,    // PIR
        &(*m_portb)[1],   // SCK
        &(*m_portb)[0],   // SS
        &(*m_portb)[3],   // SDO
        &(*m_portb)[2],    // SDI
          m_trisb,          // i2c tris port
        SSP_TYPE_MSSP1
  );
  ssp2.mk_ssp_int(&pir3, PIR3v3::SSP2IF);	// SSP2IF
  ssp2.mk_bcl_int(&pir3, PIR3v3::BCL2IF);	// BCL2IF

}

//========================================================================
//
// Pic 18F4x21
//

void P18F4x21::create()
{

  if(verbose)
    cout << "P18F4x21::create\n";

  delete pir2;
  pir2 = (PIR2v2 *)(new PIR2v4(this, "pir2" , "Peripheral Interrupt Register",0,0  ));

  tbl.initialize ( eeprom_memory_size(), 32, 4, CONFIG1L, false);
  tbl.set_intcon(&intcon);
  set_eeprom_pir(&tbl);
  tbl.set_pir(pir2);
  tbl.eecon1.set_valid_bits(0xbf);

  create_iopin_map();

  _16bit_processor::create();

  m_configMemory->addConfigWord(CONFIG3H-CONFIG1L,new Config3H_2x21(this, CONFIG3H, 0x83));
  m_configMemory->addConfigWord(CONFIG1H-CONFIG1L,new Config1H_4bits(this, CONFIG1H, 0x07));

  set_osc_pin_Number(0, 13, &(*m_porta)[7]);
  set_osc_pin_Number(1,14, &(*m_porta)[6]);

  add_sfr_register(&pwm1con, 0xfb7, RegisterValue(0,0));
  add_sfr_register(&eccpas, 0xfb6, RegisterValue(0,0));
  add_sfr_register(&osccon, 0xfd3, RegisterValue(0x40,0), "osccon");
  eccpas.setIOpin(0, 0, &(*m_portb)[0]);
  eccpas.link_registers(&pwm1con, &ccp1con);
  comparator.cmcon.set_eccpas(&eccpas);
  ccp1con.setBitMask(0xff);
  ccp1con.setCrosslinks(&ccpr1l, &pir1, PIR1v2::CCP1IF, &tmr2, &eccpas);
  ccp1con.pwm1con = &pwm1con;
  ccp1con.setIOpin(&((*m_portc)[2]), &((*m_portd)[5]), &((*m_portd)[6]), &((*m_portd)[7]));

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
  m_trisd = new PicTrisRegister(this,"trisd","", (PicPortRegister *)m_portd, false);
  m_latd  = new PicLatchRegister(this,"latd","",m_portd);

//  m_porte = new PicPortRegister(this,"porte","",8,0x07);
  m_porte->setEnableMask(0x07);     // It's been created by the P18F2x21 constructor, but with the wrong enables
  m_trise = new PicPSP_TrisRegister(this,"trise","", m_porte, false);
  m_late  = new PicLatchRegister(this,"late","",m_porte);

}
P18F4x21::~P18F4x21()
{
  delete_sfr_register(m_portd);
  delete_sfr_register(m_trisd);
  delete_sfr_register(m_latd);
  delete_sfr_register(m_trise);
  delete_sfr_register(m_late);
  remove_sfr_register(&pwm1con);
  remove_sfr_register(&eccpas);
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
	&(*m_porta)[1], &(*m_porta)[2], &(*m_porta)[3], &(*m_porta)[4],
	&(*m_porta)[5]);

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

  ccp2con.setCrosslinks(&ccpr2l, pir2, PIR2v2::CCP2IF, &tmr2);
//  ccp2con.setIOpin(&((*m_portc)[1]));     // Handled by Config3H_2x21::set
  ccpr2l.ccprh  = &ccpr2h;
  ccpr2l.tmrl   = &tmr1l;
  ccpr2h.ccprl  = &ccpr2l;

  //1 usart16.initialize_16(this,&pir_set_def,&portc);
  add_sfr_register(&usart.spbrgh,   0xfb0,porv,"spbrgh");
  add_sfr_register(&usart.baudcon,  0xfb8,porv,"baudcon");
  usart.set_eusart(true);
  init_pir2(pir2, PIR2v4::TMR3IF); 
  tmr3l.setIOpin(&(*m_portc)[0]);
}


//------------------------------------------------------------------------
//
// P18F4221
// 

P18F4221::P18F4221(const char *_name, const char *desc)
  : P18F4x21(_name,desc)
{

  if(verbose)
    cout << "18F4221 constructor, type = " << isa() << '\n';

}

Processor * P18F4221::construct(const char *name)
{

  P18F4221 *p = new P18F4221(name);

  if(verbose)
    cout << " 18F4221 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();

  if(verbose&2)
    cout << " 18F4221 construct completed\n";
  return p;
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



//------------------------------------------------------------------------
//
// P18F4420
// 

P18F4420::P18F4420(const char *_name, const char *desc)
  : P18F4x21(_name,desc)
{

  if(verbose)
    cout << "18F4420 constructor, type = " << isa() << '\n';

}

Processor * P18F4420::construct(const char *name)
{

  P18F4420 *p = new P18F4420(name);

  if(verbose)
    cout << " 18F4420 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();

  if(verbose&2)
    cout << " 18F4420 construct completed\n";
  return p;
}


//------------------------------------------------------------------------
//
// P18F4520
// 

P18F4520::P18F4520(const char *_name, const char *desc)
  : P18F4x21(_name,desc)
{

  if(verbose)
    cout << "18F4520 constructor, type = " << isa() << '\n';

}

Processor * P18F4520::construct(const char *name)
{

  P18F4520 *p = new P18F4520(name);

  if(verbose)
    cout << " 18F4520 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();

  if(verbose&2)
    cout << " 18F4520 construct completed\n";
  return p;
}




//------------------------------------------------------------------------
//
// P18F4620
// 

P18F4620::P18F4620(const char *_name, const char *desc)
  : P18F4x21(_name,desc)
{

  if(verbose)
    cout << "18F4620 constructor, type = " << isa() << '\n';

}

Processor * P18F4620::construct(const char *name)
{

  P18F4620 *p = new P18F4620(name);

  if(verbose) 
    cout << " 18F4620 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();

  if(verbose&2)
    cout << " 18F4620 construct completed\n";
  return p;
}



//========================================================================
//
// Pic 18F6x20
//

void P18F6x20::create()
{

  if(verbose)
    cout << "P18F6x20::create\n";

  tbl.initialize ( eeprom_memory_size(), 32, 4, CONFIG1L, true);
  tbl.set_intcon(&intcon);
  set_eeprom_pir(&tbl);
  tbl.set_pir(pir2);
  tbl.eecon1.set_valid_bits(0xbf);


  create_iopin_map();

  _16bit_processor::create();
  add_sfr_register(&osccon, 0xfd3, RegisterValue(0x40,0), "osccon");

  m_configMemory->addConfigWord(CONFIG1H-CONFIG1L,new Config1H_4bits(this, CONFIG1H, 0x27));
  init_pir2(pir2, PIR2v2::TMR3IF);
  tmr3l.setIOpin(&(*m_portc)[0]);
}

//------------------------------------------------------------------------
void P18F6x20::create_iopin_map()
{
  package = new Package(64);

  if(!package)
    return;

  // Build the links between the I/O Ports and their tris registers.

  package->assign_pin( 1, m_porte->addPin(new IO_bi_directional("porte1"),1));
  package->assign_pin( 2, m_porte->addPin(new IO_bi_directional("porte0"),0));

  package->assign_pin( 3, m_portg->addPin(new IO_bi_directional("portg0"),0));
  package->assign_pin( 4, m_portg->addPin(new IO_bi_directional("portg1"),1));
  package->assign_pin( 5, m_portg->addPin(new IO_bi_directional("portg2"),2));
  package->assign_pin( 6, m_portg->addPin(new IO_bi_directional("portg3"),3));

  createMCLRPin(7);

  package->assign_pin( 8, m_portg->addPin(new IO_bi_directional("portg4"),4));

  package->assign_pin( 9, 0);  // Vss
  package->assign_pin(10, 0);  // Vdd

  package->assign_pin(11, m_portf->addPin(new IO_bi_directional("portf7"),7));
  package->assign_pin(12, m_portf->addPin(new IO_bi_directional("portf6"),6));
  package->assign_pin(13, m_portf->addPin(new IO_bi_directional("portf5"),5));
  package->assign_pin(14, m_portf->addPin(new IO_bi_directional("portf4"),4));
  package->assign_pin(15, m_portf->addPin(new IO_bi_directional("portf3"),3));
  package->assign_pin(16, m_portf->addPin(new IO_bi_directional("portf2"),2));
  package->assign_pin(17, m_portf->addPin(new IO_bi_directional("portf1"),1));
  package->assign_pin(18, m_portf->addPin(new IO_bi_directional("portf0"),0));

  package->assign_pin(19, 0);  // AVdd
  package->assign_pin(20, 0);  // AVss

  package->assign_pin(21, m_porta->addPin(new IO_bi_directional("porta3"),3));
  package->assign_pin(22, m_porta->addPin(new IO_bi_directional("porta2"),2));
  package->assign_pin(23, m_porta->addPin(new IO_bi_directional("porta1"),1));
  package->assign_pin(24, m_porta->addPin(new IO_bi_directional("porta0"),0));

  package->assign_pin(25, 0);  // Vss
  package->assign_pin(26, 0);  // Vdd

  package->assign_pin(27, m_porta->addPin(new IO_bi_directional("porta5"),5));
  package->assign_pin(28, m_porta->addPin(new IO_open_collector("porta4"),4));

  package->assign_pin(29, m_portc->addPin(new IO_bi_directional("portc1"),1));
  package->assign_pin(30, m_portc->addPin(new IO_bi_directional("portc0"),0));
  package->assign_pin(31, m_portc->addPin(new IO_bi_directional("portc6"),6));
  package->assign_pin(32, m_portc->addPin(new IO_bi_directional("portc7"),7));
  package->assign_pin(33, m_portc->addPin(new IO_bi_directional("portc2"),2));
  package->assign_pin(34, m_portc->addPin(new IO_bi_directional("portc3"),3));
  package->assign_pin(35, m_portc->addPin(new IO_bi_directional("portc4"),4));
  package->assign_pin(36, m_portc->addPin(new IO_bi_directional("portc5"),5));

  package->assign_pin(37, m_portb->addPin(new IO_bi_directional_pu("portb7"),7));

  package->assign_pin(38, 0);  // Vdd
  package->assign_pin(39, 0);  // OSC1/CLKI

  package->assign_pin(40, m_porta->addPin(new IO_bi_directional("porta6"),6));

  package->assign_pin(41, 0);  // Vss

  package->assign_pin(42, m_portb->addPin(new IO_bi_directional_pu("portb6"),6));
  package->assign_pin(43, m_portb->addPin(new IO_bi_directional_pu("portb5"),5));
  package->assign_pin(44, m_portb->addPin(new IO_bi_directional_pu("portb4"),4));
  package->assign_pin(45, m_portb->addPin(new IO_bi_directional_pu("portb3"),3));
  package->assign_pin(46, m_portb->addPin(new IO_bi_directional_pu("portb2"),2));
  package->assign_pin(47, m_portb->addPin(new IO_bi_directional_pu("portb1"),1));
  package->assign_pin(48, m_portb->addPin(new IO_bi_directional_pu("portb0"),0));

  package->assign_pin(49, m_portd->addPin(new IO_bi_directional("portd7"),7));
  package->assign_pin(50, m_portd->addPin(new IO_bi_directional("portd6"),6));
  package->assign_pin(51, m_portd->addPin(new IO_bi_directional("portd5"),5));
  package->assign_pin(52, m_portd->addPin(new IO_bi_directional("portd4"),4));
  package->assign_pin(53, m_portd->addPin(new IO_bi_directional("portd3"),3));
  package->assign_pin(54, m_portd->addPin(new IO_bi_directional("portd2"),2));
  package->assign_pin(55, m_portd->addPin(new IO_bi_directional("portd1"),1));

  package->assign_pin(56, 0);  // Vss
  package->assign_pin(57, 0);  // Vdd

  package->assign_pin(58, m_portd->addPin(new IO_bi_directional("portd0"),0));

  package->assign_pin(59, m_porte->addPin(new IO_bi_directional("porte7"),7));
  package->assign_pin(60, m_porte->addPin(new IO_bi_directional("porte6"),6));
  package->assign_pin(61, m_porte->addPin(new IO_bi_directional("porte5"),5));
  package->assign_pin(62, m_porte->addPin(new IO_bi_directional("porte4"),4));
  package->assign_pin(63, m_porte->addPin(new IO_bi_directional("porte3"),3));
  package->assign_pin(64, m_porte->addPin(new IO_bi_directional("porte2"),2));

  psp.initialize(&pir_set_def,    // PIR
                m_portd,           // Parallel port
                m_trisd,           // Parallel tris
                pspcon,           // Control register
                &(*m_porte)[0],    // NOT RD
                &(*m_porte)[1],    // NOT WR
                &(*m_porte)[2]);   // NOT CS

  tmr1l.setIOpin(&(*m_portc)[0]);
  ssp.initialize(&pir_set_def,    // PIR
                &(*m_portc)[3],   // SCK
                &(*m_portf)[7],   // SS
                &(*m_portc)[5],   // SDO
                &(*m_portc)[4],   // SDI
                m_trisc,         // i2c tris port
		SSP_TYPE_MSSP
       );


  set_osc_pin_Number(0,39, NULL);
  set_osc_pin_Number(1,40, &(*m_porta)[6]);
}


void P18F6x20::create_symbols()
{
  if(verbose)
    cout << "P18F6x20 create symbols\n";

  _16bit_processor::create_symbols();
}

P18F6x20::P18F6x20(const char *_name, const char *desc)
  : _16bit_v2_adc(_name,desc),
    t4con(this, "t4con", "TMR4 Control"),
    pr4(this, "pr4", "TMR4 Period Register"),
    tmr4(this, "tmr4", "TMR4 Register"),
    pir3(this,"pir3","Peripheral Interrupt Register",0,0),
    pie3(this, "pie3", "Peripheral Interrupt Enable"),
    ipr3(this, "ipr3", "Interrupt Priorities"),
    ccp3con(this, "ccp3con", "Capture Compare Control"),
    ccpr3l(this, "ccpr3l", "Capture Compare 3 Low"),
    ccpr3h(this, "ccpr3h", "Capture Compare 3 High"),
    ccp4con(this, "ccp4con", "Capture Compare Control"),
    ccpr4l(this, "ccpr4l", "Capture Compare 4 Low"),
    ccpr4h(this, "ccpr4h", "Capture Compare 4 High"),
    ccp5con(this, "ccp5con", "Capture Compare Control"),
    ccpr5l(this, "ccpr5l", "Capture Compare 5 Low"),
    ccpr5h(this, "ccpr5h", "Capture Compare 5 High"),
    usart2(this), comparator(this)
{

  if(verbose)
    cout << "18F6x20 constructor, type = " << isa() << '\n';

  m_portd = new PicPSP_PortRegister(this,"portd","",8,0xFF);
  m_trisd = new PicTrisRegister(this,"trisd","", (PicPortRegister *)m_portd, false);
  m_latd  = new PicLatchRegister(this,"latd","",m_portd);

  m_porte = new PicPortRegister(this,"porte","",8,0xFF);
  m_trise = new PicTrisRegister(this,"trise","", m_porte, false);
  m_late  = new PicLatchRegister(this,"late","",m_porte);

  m_portf = new PicPortRegister(this,"portf","",8,0xFF);
  m_trisf = new PicTrisRegister(this,"trisf","", m_portf, false);
  m_latf  = new PicLatchRegister(this,"latf","",m_portf);

  m_portg = new PicPortRegister(this,"portg","",8,0x1F);
  m_trisg = new PicTrisRegister(this,"trisg","", m_portg, false);
  m_latg  = new PicLatchRegister(this,"latg","",m_portg);

  pspcon = new PSPCON(this, "pspcon","");


}

P18F6x20::~P18F6x20()
{
  delete_sfr_register(m_portd);
  delete_sfr_register(m_porte);
  delete_sfr_register(m_portf);
  delete_sfr_register(m_portg);

  delete_sfr_register(m_latd);
  delete_sfr_register(m_late);
  delete_sfr_register(m_latf);
  delete_sfr_register(m_latg);

  delete_sfr_register(m_trisd);
  delete_sfr_register(m_trise);
  delete_sfr_register(m_trisf);
  delete_sfr_register(m_trisg);
  delete_sfr_register(pspcon);
  delete_sfr_register(usart2.txreg);
  delete_sfr_register(usart2.rcreg);

  remove_sfr_register(&pie3);
  remove_sfr_register(&pir3);
  remove_sfr_register(&ipr3);
  remove_sfr_register(&usart2.rcsta);
  remove_sfr_register(&usart2.txsta);
  remove_sfr_register(&usart2.spbrg);
  remove_sfr_register(&ccp4con);
  remove_sfr_register(&ccpr4l);
  remove_sfr_register(&ccpr4h);
  remove_sfr_register(&ccp5con);
  remove_sfr_register(&ccpr5l);
  remove_sfr_register(&ccpr5h);
  remove_sfr_register(&t4con);
  remove_sfr_register(&pr4);
  remove_sfr_register(&tmr4);
  remove_sfr_register(&ccp3con);
  remove_sfr_register(&ccpr3l);
  remove_sfr_register(&ccpr3h);
  remove_sfr_register(&comparator.cmcon);
  remove_sfr_register(&comparator.vrcon);

}

void P18F6x20::create_sfr_map()
{

  if(verbose)
    cout << "create_sfr_map P18F6x20\n";

  _16bit_processor::create_sfr_map();
  _16bit_v2_adc::create(12);


  RegisterValue porv(0,0);

  // cout << "Create extra ports\n";
  add_sfr_register(m_portd,       0xf83,porv);
  add_sfr_register(m_porte,       0xf84,porv);
  add_sfr_register(m_portf,       0xf85,porv);
  add_sfr_register(m_portg,       0xf86,porv);

  add_sfr_register(m_latd,        0xf8c,porv);
  add_sfr_register(m_late,        0xf8d,porv);
  add_sfr_register(m_latf,        0xf8e,porv);
  add_sfr_register(m_latg,        0xf8f,porv);

  add_sfr_register(m_trisd,       0xf95,RegisterValue(0xff,0));
  add_sfr_register(m_trise,       0xf96,RegisterValue(0xff,0));
  add_sfr_register(m_trisf,       0xf97,RegisterValue(0xff,0));
  add_sfr_register(m_trisg,       0xf98,RegisterValue(0x1f,0));

  add_sfr_register(&pie3,	  0xfa3,porv,"pie3");
  add_sfr_register(&pir3,	  0xfa4,porv,"pir3");
  add_sfr_register(&ipr3,	  0xfa5,porv,"ipr3");



  add_sfr_register(pspcon,       0xfb0,RegisterValue(0x00,0));

  // cout << "Assign ADC pins to " << adcon1 << "\n";
  adcon1->setIOPin(4, &(*m_porta)[5]);
  adcon1->setIOPin(5, &(*m_portf)[0]);
  adcon1->setIOPin(6, &(*m_portf)[1]);
  adcon1->setIOPin(7, &(*m_portf)[2]);
  adcon1->setIOPin(8, &(*m_portf)[3]);
  adcon1->setIOPin(9, &(*m_portf)[4]);
  adcon1->setIOPin(10, &(*m_portf)[5]);
  adcon1->setIOPin(11, &(*m_portf)[6]);
//  adcon1->setIOPin(12, &(*m_portb)[0]);
/*
  adcon1->setChanTable(0x1ff, 0x1fff, 0x1fff, 0x0fff,
	0x07ff, 0x03ff, 0x01ff, 0x00ff, 0x007f, 0x003f,
	0x001f, 0x000f, 0x0007, 0x0003, 0x0001, 0x0000);
  adcon1->setVrefHiChannel(3);
  adcon1->setVrefLoChannel(2);
*/


  // Link the comparator and voltage ref to portf
  comparator.initialize(&pir_set_def, &(*m_portf)[5], 
	0, 0, 0, 0,
	&(*m_portf)[2], &(*m_portf)[1]);

  // set anx for input pins
  comparator.cmcon.setINpin(0, &(*m_portf)[6], "an11");
  comparator.cmcon.setINpin(1, &(*m_portf)[5], "an10");
  comparator.cmcon.setINpin(2, &(*m_portf)[4], "an9");
  comparator.cmcon.setINpin(3, &(*m_portf)[3], "an8");


  comparator.cmcon.set_configuration(1, 0, AN0, AN1, AN0, AN1, ZERO);
  comparator.cmcon.set_configuration(2, 0, AN2, AN3, AN2, AN3, ZERO);
  comparator.cmcon.set_configuration(1, 1, AN0, AN1, AN0, AN1, OUT0);
  comparator.cmcon.set_configuration(2, 1, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);
  comparator.cmcon.set_configuration(1, 2, AN0, AN1, AN0, AN1, NO_OUT);
  comparator.cmcon.set_configuration(2, 2, AN2, AN3, AN2, AN3, NO_OUT);
  comparator.cmcon.set_configuration(1, 3, AN0, AN1, AN0, AN1, OUT0);
  comparator.cmcon.set_configuration(2, 3, AN2, AN3, AN2, AN3, OUT1);
  comparator.cmcon.set_configuration(1, 4, AN0, AN1, AN0, AN1, NO_OUT);
  comparator.cmcon.set_configuration(2, 4, AN2, AN1, AN2, AN1, NO_OUT);
  comparator.cmcon.set_configuration(1, 5, AN0, AN1, AN0, AN1, OUT0);
  comparator.cmcon.set_configuration(2, 5, AN2, AN1, AN2, AN1, OUT1);
  comparator.cmcon.set_configuration(1, 6, AN0, VREF, AN1, VREF, NO_OUT);
  comparator.cmcon.set_configuration(2, 6, AN2, VREF, AN3, VREF, NO_OUT);
  comparator.cmcon.set_configuration(1, 7, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);
  comparator.cmcon.set_configuration(2, 7, NO_IN, NO_IN, NO_IN, NO_IN, ZERO);

  add_sfr_register(&comparator.cmcon, 0xfb4, RegisterValue(7,0),"cmcon");
  add_sfr_register(&comparator.vrcon, 0xfb5, RegisterValue(0,0),"cvrcon");


  // cout << "Setting CCP cross-links\n";
  ccp2con.setCrosslinks(&ccpr2l, pir2, PIR2v2::CCP2IF, &tmr2);
  ccp2con.setIOpin(&((*m_portc)[1]));
  ccpr2l.ccprh  = &ccpr2h;
  ccpr2l.tmrl   = &tmr1l;
  ccpr2h.ccprl  = &ccpr2l;

  add_sfr_register(&ccp3con,	  0xfb7,porv,"ccp3con");
  add_sfr_register(&ccpr3l,	  0xfb8,porv,"ccpr3l");
  add_sfr_register(&ccpr3h,	  0xfb9,porv,"ccpr3h");
  add_sfr_register(&ccp4con,	  0xf73,porv,"ccp4con");
  add_sfr_register(&ccpr4l,	  0xf74,porv,"ccpr4l");
  add_sfr_register(&ccpr4h,	  0xf75,porv,"ccpr4h");
  add_sfr_register(&ccp5con,	  0xf70,porv,"ccp5con");
  add_sfr_register(&ccpr5l,	  0xf71,porv,"ccpr5l");
  add_sfr_register(&ccpr5h,	  0xf72,porv,"ccpr5h");

  add_sfr_register(&t4con,	  0xf76,porv,"t4con");
  add_sfr_register(&pr4,	  0xf77,RegisterValue(0xff,0),"pr4");
  add_sfr_register(&tmr4,	  0xf78,porv,"tmr4");

  ccp3con.setCrosslinks(&ccpr3l, &pir3, PIR3v1::CCP3IF, &tmr2);
  ccp3con.setIOpin(&((*m_portg)[0]));
  ccpr3l.ccprh  = &ccpr3h;
  ccpr3l.tmrl   = &tmr1l;
  ccpr3h.ccprl  = &ccpr3l;
  tmr2.add_ccp ( &ccp3con );

  ccp4con.setCrosslinks(&ccpr4l, &pir3, PIR3v1::CCP4IF, &tmr2);
  ccp4con.setIOpin(&((*m_portg)[3]));
  ccpr4l.ccprh  = &ccpr4h;
  ccpr4l.tmrl   = &tmr1l;
  ccpr4h.ccprl  = &ccpr4l;
  tmr2.add_ccp ( &ccp4con );

  ccp5con.setCrosslinks(&ccpr5l, &pir3, PIR3v1::CCP5IF, &tmr2);
  ccp5con.setIOpin(&((*m_portg)[4]));
  ccpr5l.ccprh  = &ccpr5h;
  ccpr5l.tmrl   = &tmr1l;
  ccpr5h.ccprl  = &ccpr5l;
  tmr2.add_ccp ( &ccp5con );



  //cout << "Create second USART\n";
  usart2.initialize(&pir3,&(*m_portg)[1], &(*m_portg)[2],
	            new _TXREG(this,"txreg2", "USART Transmit Register", &usart2), 
                    new _RCREG(this,"rcreg2", "USART Receiver Register", &usart2));

  add_sfr_register(&usart2.rcsta,    0xf6b,porv,"rcsta2");
  add_sfr_register(&usart2.txsta,    0xf6c,RegisterValue(0x02,0),"txsta2");
  add_sfr_register(usart2.txreg,     0xf6d,porv,"txreg2");
  add_sfr_register(usart2.rcreg,     0xf6e,porv,"rcreg2");
  add_sfr_register(&usart2.spbrg,    0xf6f,porv,"spbrg2");

  t4con.tmr2  = &tmr4;
  tmr4.pir_set = &pir_set_def; //get_pir_set();
  tmr4.pr2    = &pr4;
  tmr4.t2con  = &t4con;
  tmr4.add_ccp ( &ccp1con );
  tmr4.add_ccp ( &ccp2con );
  pr4.tmr2    = &tmr4;

  pir3.set_intcon(&intcon);
  pir3.set_pie(&pie3);
  pir3.set_ipr(&ipr3);
  pie3.setPir(&pir3);
  //pie3.new_name("pie3");

}



//------------------------------------------------------------------------
//
// P18F6520
// 

P18F6520::P18F6520(const char *_name, const char *desc)
  : P18F6x20(_name,desc)
{
  if(verbose)
    cout << "18F6520 constructor, type = " << isa() << '\n';
}

Processor * P18F6520::construct(const char *name)
{
  P18F6520 *p = new P18F6520(name);

  if(verbose)
    cout << " 18F6520 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();

  if(verbose&2)
    cout << " 18F6520 construct completed\n";
  return p;
}

