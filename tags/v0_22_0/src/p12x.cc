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
// p12x
//
//  This file supports:
//    PIC12C508
//    PIC12C509
//

#include <stdio.h>
#include <iostream>
#include <string>

#include "packages.h"
#include "stimuli.h"
#include "i2c-ee.h"

#include "p12x.h"

#include "symbol.h"


//========================================================================
// The P12 devices with an EEPROM contain two die. One is the 12C core and
// the other is an I2C EEPROM (Actually, it is not know if there are two
// physical die. However, it is known that there are two functional layouts
// in the same package.) These two devices are connected internally.
class P12_I2C_EE : public I2C_EE
{
public:
  P12_I2C_EE(pic_processor *, unsigned int _rom_size);
  
protected:
  RegisterCollection *m_UiAccessOfRom; // User access to the rom.
};


P12_I2C_EE::P12_I2C_EE(pic_processor *pcpu, unsigned int _rom_size)
  : I2C_EE(_rom_size)
{

  if(pcpu) {
    pcpu->ema.set_cpu(pcpu);
    pcpu->ema.set_Registers(rom, rom_size);
    m_UiAccessOfRom = new RegisterCollection(pcpu,
					     "eeData",
					     rom,
					     rom_size);
  }

}

//========================================================================
void P12C508::create_iopin_map()
{

  package = new Package(8);
  if(!package)
    return;

  package->assign_pin(7, m_gpio->addPin(new IO_bi_directional_pu("gpio0"),0));
  package->assign_pin(6, m_gpio->addPin(new IO_bi_directional_pu("gpio1"),1));
  package->assign_pin(5, m_gpio->addPin(new IO_bi_directional("gpio2"),2));
  package->assign_pin(4, m_gpio->addPin(new IOPIN("gpio3"),3));
  package->assign_pin(3, m_gpio->addPin(new IO_bi_directional("gpio4"),4));
  package->assign_pin(2, m_gpio->addPin(new IO_bi_directional("gpio5"),5));
  package->assign_pin(1, 0);
  package->assign_pin(8, 0);


}

//--------------------------------------------------------
void P12C508::reset(RESET_TYPE r)
{

  m_tris->reset(r);

  switch (r) {
  case IO_RESET:
    // Set GPWUF flag
    status->put(status->get() | 0x80);

    // fall through...
   default:
    _12bit_processor::reset(r);
  }

}

//------------------------------------------------------------------------
#define STATUS_GPWUF  0x80

void P12C508::enter_sleep()
{
  pic_processor::enter_sleep();

  status->put( status->get() & ~STATUS_GPWUF);
  cout << "enter sleep status="<<hex <<status->get()<<endl;
}


void  P12C508::option_new_bits_6_7(unsigned int bits)
{
  if(verbose)
    cout << "p12c508 option_new_bits_6_7\n";
  m_gpio->setPullUp ( (bits & (1<<6)) == (1<<6) );
}

void P12C508::create_sfr_map()
{

  RegisterValue porVal(0,0);

  add_sfr_register(indf,   0, porVal);
  add_sfr_register(&tmr0,  1, porVal);
  add_sfr_register(pcl,    2, porVal);
  add_sfr_register(status, 3, porVal);
  add_sfr_register(fsr,    4, porVal);
  add_sfr_register(&osccal,5, porVal);
  add_sfr_register(m_gpio, 6, porVal);
  add_sfr_register(m_tris, 0xffffffff, RegisterValue(0x3f,0));
  add_sfr_register(W, 0xffffffff, porVal);
  add_sfr_register(&option_reg, 0xffffffff, RegisterValue(0xff,0));
  osccal.new_name("osccal");


}

void P12C508::create_symbols()
{
  _12bit_processor::create_symbols();

  //symbol_table.add_register(m_gpio);
  symbol_table.add_register(m_tris);
}


void P12C508::dump_registers ()
{


  _12bit_processor::dump_registers();

  cout << "tris = 0x" << hex << m_tris->value.get() << '\n';
  cout << "osccal = 0x" << osccal.value.get()  << '\n';

}


void P12C508::tris_instruction(unsigned int tris_register)
{
  m_tris->put(W->value.get());
  trace.write_TRIS(m_tris->value.get());
}
  
void P12C508::create()
{

  create_iopin_map();

  _12bit_processor::create();

  add_file_registers(0x07, 0x1f, 0);
  P12C508::create_sfr_map();
  create_invalid_registers ();

  tmr0.set_cpu(this,m_gpio,2);
  tmr0.start(0);

  pc->reset();
}


Processor * P12C508::construct(const char *name)
{

  P12C508 *p = new P12C508(name);

  p->pc->set_reset_address(0x1ff);

  p->create();
  p->create_symbols();

  symbol_table.add_module(p,p->name_str.c_str());

  return p;

}


P12C508::P12C508(const char *_name, const char *desc)
  : _12bit_processor(_name,desc)
{
  if(verbose)
    cout << "12c508 constructor, type = " << isa() << '\n';

  m_gpio = new GPIO("gpio",8,0x3f);
  m_tris = new PicTrisRegister("tris",m_gpio);
  m_tris->wdtr_value=RegisterValue(0x3f,0);

  if(config_modes)
    config_modes->valid_bits = config_modes->CM_FOSC0 | config_modes->CM_FOSC1 | 
      config_modes->CM_FOSC1x | config_modes->CM_WDTE | config_modes->CM_MCLRE;
}

P12C508::~P12C508()
{
}


//--------------------------------------------------------

void P12C509::create_sfr_map()
{

}

Processor * P12C509::construct(const char *name)
{

  P12C509 *p = new P12C509(name);

  if (verbose)
    cout << " 12c508 construct\n";

  p->pc->set_reset_address(0x3ff);

  p->create();
  p->create_symbols();

  symbol_table.add_module(p,p->name_str.c_str());

  return p;

}


void P12C509::create()
{

  if ( verbose )
    cout << " 12c509 create \n";

  P12C508::create();

  alias_file_registers(0x00,0x0f,0x20);
  add_file_registers(0x30, 0x3f, 0);

  pa_bits = PA0;                 // the 509 has two code pages (i.e. PAO in status is used)
  indf->base_address_mask2 = 0x3F;  // RP - need this or INDF won't work right

}

P12C509::P12C509(const char *_name, const char *desc)
  : P12C508(_name,desc)
{
  if(verbose)
    cout << "12c509 constructor, type = " << isa() << '\n';
}


//--------------------------------------------------------

// construct function is identical to 12C508 version ??
Processor * P12CE518::construct(const char *name)
{

  P12CE518 *p = new P12CE518(name);

  if(verbose)
    cout << " 12ce518 construct\n";

  p->pc->set_reset_address(0x1ff);

  p->create();

  if(verbose)
    cout << " ... create symbols\n";
  p->create_symbols();

  symbol_table.add_module(p,p->name_str.c_str());

  return p;

}


void P12CE518::create_iopin_map()
{
  P12C508::create_iopin_map();

  // Define the valid I/O pins.

  //gpio.valid_iopins = 0xff;
}

void P12CE518::create()
{
  Stimulus_Node *scl, *sda;

  if(verbose)
    cout << " 12ce518 create \n";

  P12C508::create();

  if(verbose)
    cout << "  adding serial EE\n";

  m_eeprom = new P12_I2C_EE(this, 0x10);
  m_eeprom->debug();

  // GPIO bits 6 and 7 are not bonded to physical pins, but are tied
  // to the internal I2C device.
  m_gpio->setEnableMask(0xc0 | m_gpio->getEnableMask());
  RegisterValue por_value(0xc0,0x00);
  m_gpio->value       = por_value;
  m_gpio->por_value   = por_value;
  m_gpio->wdtr_value  = por_value;
  m_gpio->put(0xc0);

  // Kludge to force top two bits to be outputs
  m_tris->put(0x3f);

  {
    scl = new Stimulus_Node ( "EE_SCL" );
    IO_bi_directional_pu *io_scl = new IO_bi_directional_pu("gpio7");
    io_scl->update_pullup('1',true);
    io_scl->setDrivingState(true);
    io_scl->setDriving(true);
    scl->attach_stimulus( m_gpio->addPin(io_scl,7));
    scl->update();
  }
  {
    sda = new Stimulus_Node ( "EE_SDA" );

    IO_open_collector *io_sda = new IO_open_collector("gpio6");
    // enable the pullup resistor.
    io_sda->update_pullup('1',true);
    io_sda->setDrivingState(true);
    io_sda->setDriving(true);
    m_gpio->addPin(io_sda,6);
    sda->attach_stimulus (io_sda);
    sda->update();
  }


  m_eeprom->attach ( scl, sda );
  /*
  ema.set_cpu(this);
  ema.set_Registers(m_eeprom->rom, m_eeprom->rom_size);
  */

}

P12CE518::P12CE518(const char *_name, const char *desc)
  : P12C508(_name,desc)
{
  if(verbose)
    cout << "12CE518 constructor, type = " << isa() << '\n';

  if(config_modes)
    config_modes->valid_bits = config_modes->CM_FOSC0 | config_modes->CM_FOSC1 | 
      config_modes->CM_FOSC1x | config_modes->CM_WDTE | config_modes->CM_MCLRE;
}

void P12CE518::tris_instruction(unsigned int tris_register)
{
    unsigned int w_val;

  w_val = W->value.get();
  m_tris->put ( w_val & 0x3F );     // top two bits always output
  trace.write_TRIS(w_val);
}


//--------------------------------------------------------

void P12CE519::create_sfr_map()
{

}

Processor * P12CE519::construct(const char *name)
{

  P12CE519 *p = new P12CE519(name);

  cout << " 12ce519 construct\n";

  p->pc->set_reset_address(0x3ff);

  p->create();
  p->create_symbols();

  symbol_table.add_module(p,p->name_str.c_str());

  return p;

}


void P12CE519::create()
{
  if ( verbose )
    cout << " 12ce519 create \n";

  P12CE518::create();

  alias_file_registers(0x00,0x0f,0x20);
  add_file_registers(0x30, 0x3f, 0);

  pa_bits = PA0;                 // the 519 has two code pages (i.e. PAO in status is used)
  indf->base_address_mask2 = 0x3F;  // RP - need this or INDF won't work right

}


P12CE519::P12CE519(const char *_name, const char *desc)
  : P12CE518(_name,desc)
{
  if(verbose)
    cout << "12ce519 constructor, type = " << isa() << '\n';
}



//--------------------------------------------------------
//
// GPIO Port

GPIO::GPIO(const char *port_name, unsigned int numIopins, 
	   unsigned int enableMask)
  : PicPortRegister (port_name, numIopins, enableMask)
{
}

void GPIO::setbit(unsigned int bit_number, bool new_value)
{
  unsigned int lastDrivenValue = rvDrivenValue.data;

  PortRegister::setbit(bit_number, new_value);

  // If gpio bit 0,1 or 3 changed states AND
  // ~GPWU is low (wake up on change is enabled) AND
  // the processor is sleeping.
  //    Then wake 

  if ((lastDrivenValue ^ rvDrivenValue.data) & 0x0b) {
    if( ((cpu12->option_reg.value.get() & 0x80) == 0) && bp.have_sleep()) {

      if(verbose)
	cout << "IO bit changed while the processor was sleeping,\n\
so the processor is waking up\n";

      cpu->reset(IO_RESET);

    }

  }
}

void GPIO::setPullUp ( bool bNewPU )
{
  m_bPU = !bNewPU;

  if ( verbose & 16 )
    printf("GPIO::setPullUp() =%d\n",(m_bPU?1:0));

  unsigned int mask = getEnableMask();
  for (unsigned int i=0, m=1; mask; i++, m<<= 1)
    if (mask & m)
    {
      mask ^= m;
      getPin(i)->update_pullup ( m_bPU ? '1' : '0', true );
    }
}



//--------------------------------------------------------
//------------------------------------------------------------------------


void P10F200::create_iopin_map()
{

  package = new Package(6);
  if(!package)
    return;

  package->assign_pin(1, m_gpio->addPin(new IO_bi_directional_pu("gpio0"),0));
  package->assign_pin(3, m_gpio->addPin(new IO_bi_directional_pu("gpio1"),1));
  package->assign_pin(4, m_gpio->addPin(new IO_bi_directional("gpio2"),2));
  package->assign_pin(6, m_gpio->addPin(new IOPIN("gpio3"),3));
  package->assign_pin(2, 0);
  package->assign_pin(5, 0);


}

/*
void P10F200::create_sfr_map()
{

  RegisterValue porVal(0,0);

  add_sfr_register(indf,   0, porVal);
  add_sfr_register(&tmr0,  1, porVal);
  add_sfr_register(pcl,    2, porVal);
  add_sfr_register(status, 3, porVal);
  add_sfr_register(fsr,    4, porVal);
  add_sfr_register(&osccal,5, porVal);
  add_sfr_register(m_gpio, 6, porVal);
  add_sfr_register(m_tris, 0xffffffff, RegisterValue(0x0f,0));
  add_sfr_register(W, 0xffffffff, porVal);
  add_sfr_register(&option_reg, 0xffffffff, RegisterValue(0xff,0));
  osccal.new_name("osccal");


}
*/

  
void P10F200::create()
{

  create_iopin_map();

  _12bit_processor::create();

  add_file_registers(0x10, 0x1f, 0);    // 10F200 only has 16 bytes RAM
  P12C508::create_sfr_map();
  create_invalid_registers ();

  tmr0.set_cpu(this,m_gpio,2);
  tmr0.start(0);

  pc->reset();
}


Processor * P10F200::construct(const char *name)
{

  P10F200 *p = new P10F200(name);

  p->pc->set_reset_address(0x0ff);

  p->create();
  p->create_symbols();

  symbol_table.add_module(p,p->name_str.c_str());

  return p;

}


P10F200::P10F200(const char *_name, const char *desc)
  : P12C508(_name,desc)
{
  if(verbose)
    cout << "10f200 constructor, type = " << isa() << '\n';

  m_gpio = new GPIO("gpio",8,0x0f);
  m_tris = new PicTrisRegister("tris",m_gpio);
  m_tris->wdtr_value=RegisterValue(0x3f,0);

  if(config_modes)
    config_modes->valid_bits = config_modes->CM_WDTE | config_modes->CM_MCLRE;
}

P10F200::~P10F200()
{

}
//------------------------------------------------------------------------

void P10F202::create()
{

  create_iopin_map();

  _12bit_processor::create();

  add_file_registers(0x08, 0x1f, 0);    // 10F202 has 24 bytes RAM
  P12C508::create_sfr_map();
  create_invalid_registers ();

  tmr0.set_cpu(this,m_gpio,2);
  tmr0.start(0);

  pc->reset();
}


Processor * P10F202::construct(const char *name)
{

  P10F202 *p = new P10F202(name);

  p->pc->set_reset_address(0x1ff);

  p->create();
  p->create_symbols();

  symbol_table.add_module(p,p->name_str.c_str());

  return p;

}


P10F202::P10F202(const char *_name, const char *desc)
  : P10F200(_name,desc)
{
  if(verbose)
    cout << "10f202 constructor, type = " << isa() << '\n';
}


