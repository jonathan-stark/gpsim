/*
   Copyright (C) 1998 T. Scott Dattalo

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
// p16x7x
//
//  This file supports:
//    P16C71
//    P16C712
//    P16C716
//    P16C72
//    P16C73
//    P16C74


#include <stdio.h>
#include <iostream>
#include <string>

#include "../config.h"
#include "symbol.h"

#include "p16x7x.h"
#include "pic-ioports.h"
#include "stimuli.h"
#include "pm_rd.h"


//#define DEBUG_AD


//------------------------------------------------------
class P16C71::PIR_16C71 : public PIR_SET
{
public:
  PIR_16C71(ADCON0 *adcon0)
    : m_adcon0(adcon0)
  {
  }

  virtual int interrupt_status()
  {
    return m_adcon0->getADIF();
  }
private:
  ADCON0 *m_adcon0;
};

//------------------------------------------------------------------------
//
P16C71::P16C71(const char *_name, const char *desc)
  : P16C61(_name, desc),
    adcon0(this,"adcon0", "A2D Control 0"),
    adcon1(this,"adcon1", "A2D Control 1"),
    adres(this,"adres", "A2D Result")

{
  if(verbose)
    cout << "c71 constructor, type = " << isa() << '\n';

  m_pir = new PIR_16C71(&adcon0);
}


void P16C71::create_sfr_map()
{

  if(verbose)
    cout << "creating c71 registers \n";

  add_sfr_register(&adcon0, 0x08, RegisterValue(0,0));
  add_sfr_register(&adcon1, 0x88, RegisterValue(0,0));

  //add_sfr_register(&adres,  0x89, RegisterValue(0,0));
  add_sfr_register(&adres,  0x09, RegisterValue(0,0));
  alias_file_registers(0x70,0x7f,0x80);


  adcon1.setValidCfgBits(ADCON1::PCFG0 | ADCON1::PCFG1,0);
  adcon1.setNumberOfChannels(4);
  adcon1.setIOPin(0, &(*m_porta)[0]);
  adcon1.setIOPin(1, &(*m_porta)[1]);
  adcon1.setIOPin(2, &(*m_porta)[2]);
  adcon1.setIOPin(3, &(*m_porta)[3]);
  adcon1.setChannelConfiguration(0, 0x0f);
  adcon1.setChannelConfiguration(1, 0x0f);
  adcon1.setChannelConfiguration(2, 0x03);
  adcon1.setChannelConfiguration(3, 0x00);
  adcon1.setVrefHiConfiguration(1, 3);

  adcon0.setAdres(&adres);
  adcon0.setAdresLow(0);
  adcon0.setAdcon1(&adcon1);
  adcon0.setIntcon(&intcon_reg);
  adcon0.setA2DBits(8);

  intcon = &intcon_reg;

  intcon_reg.set_pir_set(m_pir);

}


void P16C71::create_symbols()
{
  pic_processor::create_symbols();
}

void P16C71::create()
{

  P16C61::create();

  create_sfr_map();

}

Processor * P16C71::construct(const char *name)
{

  P16C71 *p = new P16C71(name);

  if(verbose)
    cout << " c71 construct\n";

  p->create();
  p->create_invalid_registers ();
  p->create_symbols();

  globalSymbolTable().addModule(p);

  return p;

}



//--------------------------------------

void P16C712::create_sfr_map()
{

  if(verbose)
    cout << "creating c712/6 registers \n";

  /* Extra timers and Capture/Compare are like in 16x63 => 16X6X code */
  P16X6X_processor::create_sfr_map();

  /* The A/D section is similar to 16x71, but not equal */
  add_sfr_register(&adcon0, 0x1F, RegisterValue(0,0));
  add_sfr_register(&adcon1, 0x9F, RegisterValue(0,0));

  add_sfr_register(&adres,  0x1E, RegisterValue(0,0));

  //1adcon0.analog_port = porta;
  adcon0.setAdres(&adres);
  adcon0.setAdresLow(0);
  adcon0.setAdcon1(&adcon1);
  adcon0.setIntcon(&intcon_reg);
  adcon0.setChannel_Mask(3);
  adcon0.setA2DBits(8);
  intcon = &intcon_reg;

  adcon1.setValidCfgBits(ADCON1::PCFG0 | ADCON1::PCFG1| ADCON1::PCFG2,0);
  adcon1.setNumberOfChannels(4);
  adcon1.setIOPin(0, &(*m_porta)[0]);
  adcon1.setIOPin(1, &(*m_porta)[1]);
  adcon1.setIOPin(2, &(*m_porta)[2]);
  adcon1.setIOPin(3, &(*m_porta)[3]);
  adcon1.setChannelConfiguration(0, 0x0f);
  adcon1.setChannelConfiguration(1, 0x0f);
  adcon1.setChannelConfiguration(2, 0x0f);
  adcon1.setChannelConfiguration(3, 0x0f);
  adcon1.setChannelConfiguration(4, 0x0b);
  adcon1.setChannelConfiguration(5, 0x0b);
  adcon1.setChannelConfiguration(6, 0x00);
  adcon1.setChannelConfiguration(7, 0x00);
  adcon1.setVrefHiConfiguration(1, 3);
  adcon1.setVrefHiConfiguration(3, 3);
  adcon1.setVrefHiConfiguration(5, 3);

}



void P16C712::create()
{

  if(verbose)
    cout << " c712/6 create \n";
  create_iopin_map(); /* 14 bits 18 pins connections */
  _14bit_processor::create();
  create_sfr_map();
  //1ccp1con.iopin = portb->pins[2];

}

Processor * P16C712::construct(const char *name)
{

  P16C712 *p = new P16C712(name);

  if(verbose)
    cout << " c712 construct\n";

  p->create();
  p->create_invalid_registers ();
  p->create_symbols();

  globalSymbolTable().addModule(p);

  return p;

}


P16C712::P16C712(const char *_name, const char *desc)
  : P16C62(_name, desc),
    adcon0(this,"adcon0", "A2D Control 0"),
    adcon1(this,"adcon1", "A2D Control 1"),
    adres(this,"adres", "A2D Result")
{
  if(verbose)
    cout << "c712 constructor, type = " << isa() << '\n';
}


//--------------------------------------

Processor * P16C716::construct(const char *name)
{

  P16C716 *p = new P16C716(name);

  if(verbose)
    cout << " c716 construct\n";

  p->create();
  p->create_invalid_registers ();
  p->create_symbols();

  globalSymbolTable().addModule(p);

  return p;

}


P16C716::P16C716(const char *_name, const char *desc)
  : P16C712(_name, desc)
{
  if(verbose)
    cout << "c716 constructor, type = " << isa() << '\n';

}


//--------------------------------------

void P16C72::create_sfr_map()
{

  if(verbose)
    cout << "creating c72 registers \n";

  // Parent classes just set PIR version 1

  pir_set_2_def.set_pir1(pir1_2_reg);
  pir_set_2_def.set_pir2(pir2_2_reg);

  add_sfr_register(&adcon0, 0x1f, RegisterValue(0,0));
  add_sfr_register(&adcon1, 0x9f, RegisterValue(0,0));

  add_sfr_register(&adres,  0x1e, RegisterValue(0,0));

  adcon0.setAdres(&adres);
  adcon0.setAdresLow(0);
  adcon0.setAdcon1(&adcon1);
  adcon0.setIntcon(&intcon_reg);
  adcon0.setPir(pir1_2_reg);
  adcon0.setChannel_Mask(7); // even though there are only 5 inputs...
  adcon0.setA2DBits(8);

  intcon = &intcon_reg;

  adcon1.setValidCfgBits(ADCON1::PCFG0 | ADCON1::PCFG1| ADCON1::PCFG2, 0);
  adcon1.setNumberOfChannels(5);
  adcon1.setIOPin(0, &(*m_porta)[0]);
  adcon1.setIOPin(1, &(*m_porta)[1]);
  adcon1.setIOPin(2, &(*m_porta)[2]);
  adcon1.setIOPin(3, &(*m_porta)[3]);
  adcon1.setIOPin(4, &(*m_porta)[5]);
  adcon1.setChannelConfiguration(0, 0x1f);
  adcon1.setChannelConfiguration(1, 0x1f);
  adcon1.setChannelConfiguration(2, 0x1f);
  adcon1.setChannelConfiguration(3, 0x1f);
  adcon1.setChannelConfiguration(4, 0x0b);
  adcon1.setChannelConfiguration(5, 0x0b);
  adcon1.setChannelConfiguration(6, 0x00);
  adcon1.setChannelConfiguration(7, 0x00);
  adcon1.setVrefHiConfiguration(1, 3);
  adcon1.setVrefHiConfiguration(3, 3);
  adcon1.setVrefHiConfiguration(5, 3);

  // Link the A/D converter to the Capture Compare Module
  ccp2con.setADCON(&adcon0);
}


void P16C72::create_symbols()
{

  if(verbose)
    cout << "c72 create symbols\n";
  pic_processor::create_symbols();
}


void P16C72::create()
{

  P16C62::create();

  P16C72::create_sfr_map();

}

Processor * P16C72::construct(const char *name)
{

  P16C72 *p = new P16C72(name);

  if(verbose)
    cout << " c72 construct\n";

  p->create();
  p->create_invalid_registers ();
  p->create_symbols();


  globalSymbolTable().addModule(p);

  return p;

}


P16C72::P16C72(const char *_name, const char *desc)
  : P16C62(_name, desc),
    adcon0(this,"adcon0", "A2D Control 0"),
    adcon1(this,"adcon1", "A2D Control 1"),
    adres(this,"adres", "A2D Result")
{
  if(verbose)
    cout << "c72 constructor, type = " << isa() << '\n';

  pir1_2_reg = new PIR1v2(this,"pir1","Peripheral Interrupt Register",&intcon_reg,&pie1);
  pir2_2_reg = new PIR2v2(this,"pir2","Peripheral Interrupt Register",&intcon_reg,&pie2);
  pir1 = pir1_2_reg;
  pir2 = pir2_2_reg;

}


//--------------------------------------

void P16C73::create_sfr_map()
{

  if(verbose)
    cout << "creating c73 registers \n";

  // Parent classes just set PIR version 1
  pir_set_2_def.set_pir1(pir1_2_reg);
  pir_set_2_def.set_pir2(pir2_2_reg);

  add_sfr_register(&adcon0, 0x1f, RegisterValue(0,0));
  add_sfr_register(&adcon1, 0x9f, RegisterValue(0,0));

  add_sfr_register(&adres,  0x1e, RegisterValue(0,0));

  adcon0.setAdres(&adres);
  adcon0.setAdresLow(0);
  adcon0.setAdcon1(&adcon1);
  adcon0.setIntcon(&intcon_reg);
  adcon0.setPir(pir1_2_reg);
  adcon0.setChannel_Mask(7); // even though there are only 5 inputs...
  adcon0.setA2DBits(8);

  intcon = &intcon_reg;

  //1adcon1.analog_port = porta;
  adcon1.setValidCfgBits(ADCON1::PCFG0 | ADCON1::PCFG1| ADCON1::PCFG2, 0);

  adcon1.setNumberOfChannels(5);
  adcon1.setIOPin(0, &(*m_porta)[0]);
  adcon1.setIOPin(1, &(*m_porta)[1]);
  adcon1.setIOPin(2, &(*m_porta)[2]);
  adcon1.setIOPin(3, &(*m_porta)[3]);
  adcon1.setIOPin(4, &(*m_porta)[5]);
  adcon1.setChannelConfiguration(0, 0x1f);
  adcon1.setChannelConfiguration(1, 0x1f);
  adcon1.setChannelConfiguration(2, 0x1f);
  adcon1.setChannelConfiguration(3, 0x1f);
  adcon1.setChannelConfiguration(4, 0x0b);
  adcon1.setChannelConfiguration(5, 0x0b);
  adcon1.setChannelConfiguration(6, 0x00);
  adcon1.setChannelConfiguration(7, 0x00);
  adcon1.setVrefHiConfiguration(1, 3);
  adcon1.setVrefHiConfiguration(3, 3);
  adcon1.setVrefHiConfiguration(5, 3);

  // Link the A/D converter to the Capture Compare Module
  ccp2con.setADCON(&adcon0);
}


void P16C73::create_symbols()
{

  if(verbose)
    cout << "c73 create symbols\n";
  pic_processor::create_symbols();
}


void P16C73::create()
{

  P16C63::create();

  P16C73::create_sfr_map();

}

Processor * P16C73::construct(const char *name)
{

  P16C73 *p = new P16C73(name);

  if(verbose)
    cout << " c73 construct\n";

  p->create();
  p->create_invalid_registers ();
  p->create_symbols();

  globalSymbolTable().addModule(p);

  return p;

}


P16C73::P16C73(const char *_name, const char *desc)
  : P16C63(_name, desc),
    adcon0(this,"adcon0", "A2D Control 0"),
    adcon1(this,"adcon1", "A2D Control 1"),
    adres(this,"adres", "A2D Result")
{
  if(verbose)
    cout << "c73 constructor, type = " << isa() << '\n';

  pir1_2_reg = new PIR1v2(this,"pir1","Peripheral Interrupt Register",&intcon_reg,&pie1);
  pir2_2_reg = new PIR2v2(this,"pir2","Peripheral Interrupt Register",&intcon_reg,&pie2);

  pir1 = pir1_2_reg;
  pir2 = pir2_2_reg;


}

//------------------------------------------------------------

void P16F73::create_sfr_map()
{

  if(verbose)
    cout << "creating f73 registers \n";

  add_sfr_register(pm_rd.get_reg_pmadr(),  0x10d);
  add_sfr_register(pm_rd.get_reg_pmadrh(), 0x10f);
  add_sfr_register(pm_rd.get_reg_pmdata(), 0x10c);
  add_sfr_register(pm_rd.get_reg_pmdath(), 0x10e);
  add_sfr_register(pm_rd.get_reg_pmcon1(), 0x18c);

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

}


void P16F73::create_symbols()
{

  if(verbose)
    cout << "f73 create symbols\n";
  pic_processor::create_symbols();
}


void P16F73::create()
{

  P16C73::create();

  status->rp_mask = 0x60;  // rp0 and rp1 are valid.
  indf->base_address_mask1 = 0x80; // used for indirect accesses above 0x100
  indf->base_address_mask2 = 0x1ff; // used for indirect accesses above 0x100

  P16F73::create_sfr_map();

}

Processor * P16F73::construct(const char *name)
{

  P16F73 *p = new P16F73(name);

  if(verbose)
    cout << " f73 construct\n";

  p->create();
  p->create_invalid_registers ();
  p->create_symbols();

  globalSymbolTable().addModule(p);

  return p;

}


P16F73::P16F73(const char *_name, const char *desc)
  : P16C73(_name, desc),
    pm_rd(this)
{
  if(verbose)
    cout << "f73 constructor, type = " << isa() << '\n';

}

//------------------------------------------------------------
//
//           16C74
//

void P16C74::create_sfr_map()
{

  if(verbose)
    cout << "creating c74 registers \n";

  // Parent classes just set PIR version 1
  pir_set_2_def.set_pir1(pir1_2_reg);
  pir_set_2_def.set_pir2(pir2_2_reg);

  add_sfr_register(&adcon0, 0x1f, RegisterValue(0,0));
  add_sfr_register(&adcon1, 0x9f, RegisterValue(0,0));

  add_sfr_register(&adres,  0x1e, RegisterValue(0,0));

  //1adcon0.analog_port = porta;
  //1adcon0.analog_port2 = porte;

  adcon0.setAdres(&adres);
  adcon0.setAdresLow(0);
  adcon0.setAdcon1(&adcon1);
  adcon0.setIntcon(&intcon_reg);
  adcon0.setPir(pir1_2_reg);
  adcon0.setChannel_Mask(7);
  adcon0.setA2DBits(8);

  intcon = &intcon_reg;


  adcon1.setValidCfgBits(ADCON1::PCFG0 | ADCON1::PCFG1 | ADCON1::PCFG2, 0);
  adcon1.setNumberOfChannels(8);
  adcon1.setIOPin(0, &(*m_porta)[0]);
  adcon1.setIOPin(1, &(*m_porta)[1]);
  adcon1.setIOPin(2, &(*m_porta)[2]);
  adcon1.setIOPin(3, &(*m_porta)[3]);
  adcon1.setIOPin(4, &(*m_porta)[5]);
  adcon1.setIOPin(5, &(*m_porte)[0]);
  adcon1.setIOPin(6, &(*m_porte)[1]);
  adcon1.setIOPin(7, &(*m_porte)[2]);

  adcon1.setChannelConfiguration(0, 0xff);
  adcon1.setChannelConfiguration(1, 0xff);
  adcon1.setChannelConfiguration(2, 0x1f);
  adcon1.setChannelConfiguration(3, 0x1f);
  adcon1.setChannelConfiguration(4, 0x0b);
  adcon1.setChannelConfiguration(5, 0x0b);
  adcon1.setChannelConfiguration(6, 0x00);
  adcon1.setChannelConfiguration(7, 0x00);

  adcon1.setVrefHiConfiguration(1, 3);
  adcon1.setVrefHiConfiguration(3, 3);
  adcon1.setVrefHiConfiguration(5, 3);

  // Link the A/D converter to the Capture Compare Module
  ccp2con.setADCON(&adcon0);
}


void P16C74::create_symbols()
{

  if(verbose)
    cout << "c74 create symbols\n";
  Pic14Bit::create_symbols();
}


void P16C74::create()
{

  P16C65::create();

  P16C74::create_sfr_map();

}

Processor * P16C74::construct(const char *name)
{

  P16C74 *p = new P16C74(name);;

  if(verbose)
    cout << " c74 construct\n";

  p->create();
  p->create_invalid_registers ();
  p->create_symbols();

  globalSymbolTable().addModule(p);

  return p;

}


P16C74::P16C74(const char *_name, const char *desc)
  : P16C65(_name, desc) ,
    adcon0(this,"adcon0", "A2D Control 0"),
    adcon1(this,"adcon1", "A2D Control 1"),
    adres(this,"adres", "A2D Result")
{
  if(verbose)
    cout << "c74 constructor, type = " << isa() << '\n';

  pir1_2_reg = new PIR1v2(this,"pir1","Peripheral Interrupt Register",&intcon_reg,&pie1);
  pir2_2_reg = new PIR2v2(this,"pir2","Peripheral Interrupt Register",&intcon_reg,&pie2);

  pir1 = pir1_2_reg;
  pir2 = pir2_2_reg;

}

//------------------------------------------------------------

void P16F74::create_sfr_map()
{

  if(verbose)
    cout << "creating f74 registers \n";

  add_sfr_register(pm_rd.get_reg_pmadr(),  0x10d);
  add_sfr_register(pm_rd.get_reg_pmadrh(), 0x10f);
  add_sfr_register(pm_rd.get_reg_pmdata(), 0x10c);
  add_sfr_register(pm_rd.get_reg_pmdath(), 0x10e);
  add_sfr_register(pm_rd.get_reg_pmcon1(), 0x18c);

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

}


void P16F74::create_symbols()
{

  if(verbose)
    cout << "f74 create symbols\n";
  pic_processor::create_symbols();
}


void P16F74::create()
{

  P16C74::create();

  status->rp_mask = 0x60;  // rp0 and rp1 are valid.
  indf->base_address_mask1 = 0x80; // used for indirect accesses above 0x100
  indf->base_address_mask2 = 0x1ff; // used for indirect accesses above 0x100

  P16F74::create_sfr_map();

}

Processor * P16F74::construct(const char *name)
{

  P16F74 *p = new P16F74(name);

  if(verbose)
    cout << " f74 construct\n";

  p->create();
  p->create_invalid_registers ();
  p->create_symbols();

  globalSymbolTable().addModule(p);

  return p;

}


P16F74::P16F74(const char *_name, const char *desc)
  : P16C74(_name, desc),
    pm_rd(this)
{
  if(verbose)
    cout << "f74 constructor, type = " << isa() << '\n';
}


