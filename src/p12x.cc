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
//    PIC12C508 PIC12C509
//    PIC12CE518 PIC12CE519
//    PIC10F200 PIC10F202 PIC10F204
//    PIC10F220 PIC10F222
//

#include <stdio.h>
#include <iostream>
#include <string>

#include "../config.h"

#include "packages.h"
#include "stimuli.h"
#include "i2c-ee.h"

#include "p12x.h"

#include "symbol.h"


//========================================================================
// Generic Configuration word for the midrange family.

class Generic12bitConfigWord : public ConfigWord
{
public:
  Generic12bitConfigWord(_12bit_processor *pCpu)
    : ConfigWord("CONFIG", 0xfff, "Configuration Word", pCpu, 0xfff)
  {
    assert(pCpu);
    pCpu->wdt.initialize(true);
  }

  enum {
    FOSC0  = 1<<0,
    FOSC1  = 1<<1,
    WDTEN  = 1<<2,
    CP     = 1<<3,
    MCLRE  = 1<<4
  };

  virtual void set(gint64 v)
  {
    gint64 oldV = getVal();

    Integer::set(v);
    if (m_pCpu) {

      gint64 diff = oldV ^ v;

      if (diff & WDTEN)
        m_pCpu->wdt.initialize((v & WDTEN) == WDTEN);

      if (diff & MCLRE)
        m_pCpu->config_modes->set_mclre((v&MCLRE)==MCLRE);

    }

  }

  virtual string toString()
  {
    gint64 i64;
    get(i64);
    int i = i64 &0xfff;

    char buff[256];

    snprintf(buff,sizeof(buff),
             "$%3x\n"
             " FOSC=%d - Clk source = %s\n"
             " WDTEN=%d - WDT is %s\n"
             " CP=%d - Code protect is %s\n"
             " MCLRE=%d - /MCLR is %s",
             i,
             i&(FOSC0|FOSC1), (i&FOSC0 ? (i&FOSC1 ? "EXTRC":"XT"):(i&FOSC1 ? "INTRC":"LP")),
             (i&WDTEN?1:0), ((i&WDTEN) ? "enabled" : "disabled"),
             (i&CP?1:0), ((i&CP) ? "enabled" : "disabled"),
             (i&MCLRE?1:0), ((i&MCLRE) ? "enabled" : "disabled"));
    return string(buff);
  }

};

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
  : I2C_EE(pcpu,_rom_size)
{

  if(pcpu) {
    pcpu->ema.set_Registers(rom, rom_size);
    m_UiAccessOfRom = new RegisterCollection(pcpu,
                                             "eeData",
                                             rom,
                                             rom_size);
  }

}
//-------------------------------------------------------------------

P12bitBase::P12bitBase(const char *_name, const char *desc)
  : _12bit_processor(_name,desc),
    m_gpio(0),
    m_tris(0),
    osccal(this,"osccal","Oscillator Calibration")
{
  if(config_modes)
    config_modes->valid_bits = config_modes->CM_FOSC0 | config_modes->CM_FOSC1 |
      config_modes->CM_FOSC1x | config_modes->CM_WDTE | config_modes->CM_MCLRE;
}

P12bitBase::~P12bitBase()
{

  delete_sfr_register(m_gpio);
  delete_sfr_register(m_tris);
  delete_file_registers(0x7, 0x1f);
  /*
  removeSymbol(*m_configMemory);
  delete *m_configMemory;
  delete [] m_configMemory;
  */
}


void P12bitBase::create_config_memory()
{
  m_configMemory = new ConfigMemory(this,1);
  m_configMemory->addConfigWord(0,new Generic12bitConfigWord(this));

  /*
  m_configMemory = new ConfigMemory *[1];
  *m_configMemory = new Generic12bitConfigWord(this);
  */
}


//========================================================================
void P12bitBase::create_iopin_map()
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
void P12bitBase::reset(RESET_TYPE r)
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

void P12bitBase::enter_sleep()
{
  pic_processor::enter_sleep();

  status->put( status->get() & ~STATUS_GPWUF);
  cout << "enter sleep status="<<hex <<status->get()<<endl;
}


void  P12bitBase::option_new_bits_6_7(unsigned int bits)
{
  if(verbose)
    cout << "p12c508 option_new_bits_6_7\n";
  m_gpio->setPullUp ( (bits & (1<<6)) == (1<<6) );
}

void P12bitBase::create_sfr_map()
{

  RegisterValue porVal(0,0);

  add_sfr_register(indf,   0, porVal);
  add_sfr_register(&tmr0,  1, porVal);
  add_sfr_register(pcl,    2, RegisterValue(0xff,0));
  add_sfr_register(status, 3, porVal);
  add_sfr_register(fsr,    4, porVal);
  add_sfr_register(&osccal,5, RegisterValue(0xfe,0));
  add_sfr_register(m_gpio, 6, porVal);
  add_sfr_register(m_tris, 0xffffffff, RegisterValue(0x3f,0));
  add_sfr_register(W, 0xffffffff, porVal);
  //add_sfr_register(option_reg, 0xffffffff, RegisterValue(0x00,0));
  option_reg->set_cpu(this);
  osccal.new_name("osccal");


}

void P12bitBase::create_symbols()
{
  _12bit_processor::create_symbols();

  //symbol_table.add_register(m_gpio);
  addSymbol(m_tris);
}


void P12bitBase::dump_registers ()
{


  _12bit_processor::dump_registers();

  cout << "tris = 0x" << hex << m_tris->value.get() << '\n';
  cout << "osccal = 0x" << osccal.value.get()  << '\n';

}


void P12bitBase::tris_instruction(unsigned int tris_register)
{
  m_tris->put(W->value.get());


  //trace.write_TRIS(m_tris->value.get());
}

void P12C508::create()
{

  create_iopin_map();

  _12bit_processor::create();

  add_file_registers(0x07, 0x1f, 0);
  P12bitBase::create_sfr_map();
  create_invalid_registers ();

  tmr0.set_cpu(this,m_gpio,2,option_reg);
  tmr0.start(0);

  pc->reset();
}


Processor * P12C508::construct(const char *name)
{

  P12C508 *p = new P12C508(name);

  p->pc->set_reset_address(0x1ff);

  p->create();
  p->create_symbols();
  return p;

}

P12C508::P12C508(const char *_name, const char *desc)
  : P12bitBase(_name,desc)
{
  m_gpio = new GPIO(this,"gpio","I/O port",8,0x3f);
  m_tris = new PicTrisRegister(this,"tris","Port Direction Control", m_gpio, false);
  m_tris->wdtr_value=RegisterValue(0x3f,0);
}

P12C508::~P12C508()
{
}

P12F508::P12F508(const char *_name, const char *desc)
  : P12C508(_name,desc)
{
}
P12F508::~P12F508()
{
}
Processor * P12F508::construct(const char *name)
{

  P12F508 *p = new P12F508(name);
  p->pc->set_reset_address(0x1ff);
  p->create();
  p->create_symbols();
  return p;

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
}


P12F509::P12F509(const char *_name, const char *desc)
  : P12C509(_name,desc)
{
}
P12F509::~P12F509()
{
}
Processor * P12F509::construct(const char *name)
{

  P12F509 *p = new P12F509(name);
  p->pc->set_reset_address(0x3ff);
  p->create();
  p->create_symbols();
  return p;

}
//
P12F510::P12F510(const char *_name, const char *desc)
  : P12F509(_name,desc)
{
}
P12F510::~P12F510()
{
}
Processor * P12F510::construct(const char *name)
{

  P12F510 *p = new P12F510(name);
  p->pc->set_reset_address(0x3ff);
  p->create();
  p->create_symbols();
  return p;

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

  //  trace.write_TRIS(w_val);
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

GPIO::GPIO(Processor *pCpu, const char *pName, const char *pDesc,
           unsigned int numIopins,
           unsigned int enableMask)
  : PicPortRegister (pCpu,pName,pDesc, numIopins, enableMask)
{
}

void GPIO::setbit(unsigned int bit_number, char new_value)
{
  unsigned int lastDrivenValue = rvDrivenValue.data;

  PortRegister::setbit(bit_number, new_value);

  // If gpio bit 0,1 or 3 changed states AND
  // ~GPWU is low (wake up on change is enabled) AND
  // the processor is sleeping.
  //    Then wake

  unsigned int diff = lastDrivenValue ^ rvDrivenValue.data;
  if ((diff & (1<<3)) && cpu_pic->config_modes->get_mclre()) { // GP3 is the reset pin

    cpu->reset( (rvDrivenValue.data & (1<<3)) ? EXIT_RESET : MCLR_RESET);
    return;
  }

  if (diff & 0x0b) {
    // If /GPWU is 0 (i.e. enabled) and the processor is currently sleeping
    // then wake up the processor by resetting it.
    if( ((cpu12->option_reg->value.get() & 0x80) == 0) &&
        cpu12->getActivityState() == pic_processor::ePASleeping) {

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

  // In the following do not change pullup state of internal pins
  unsigned int mask = getEnableMask() & 0x3f;
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



void P10F200::create()
{

  create_iopin_map();

  _12bit_processor::create();

  add_file_registers(0x10, 0x1f, 0);    // 10F200 only has 16 bytes RAM
  P12bitBase::create_sfr_map();
  create_invalid_registers ();

  tmr0.set_cpu(this,m_gpio,2,option_reg);
  tmr0.start(0);

  pc->reset();
}


Processor * P10F200::construct(const char *name)
{

  P10F200 *p = new P10F200(name);

  p->pc->set_reset_address(0x0ff);

  p->create();
  p->create_symbols();
  return p;

}


P10F200::P10F200(const char *_name, const char *desc)
  : P12bitBase(_name,desc)
{
  if(verbose)
    cout << "10f200 constructor, type = " << isa() << '\n';

  m_gpio = new GPIO(this,"gpio","I/O port",8,0x0f);
  m_tris = new PicTrisRegister(this, "tris", "Port Direction Control",m_gpio, false);
  m_tris->wdtr_value=RegisterValue(0x3f,0);

  if(config_modes)
    config_modes->valid_bits = config_modes->CM_WDTE | config_modes->CM_MCLRE;
}

P10F200::~P10F200()
{

}
void P10F200::updateGP2Source()
{
  PinModule *pmGP2 = &(*m_gpio)[2];

  // revert to default control, i.e. let TRIS control the output
  pmGP2->setControl(0);
  pmGP2->setSource(0);

  cout << "tris is controlling the output\n";


}
//------------------------------------------------------------------------

void P10F202::create()
{

  create_iopin_map();

  _12bit_processor::create();

  add_file_registers(0x08, 0x1f, 0);    // 10F202 has 24 bytes RAM
  P12bitBase::create_sfr_map();
  create_invalid_registers ();

  tmr0.set_cpu(this,m_gpio,2,option_reg);
  tmr0.start(0);

  pc->reset();
}


Processor * P10F202::construct(const char *name)
{

  P10F202 *p = new P10F202(name);

  p->pc->set_reset_address(0x1ff);

  p->create();
  p->create_symbols();
  return p;

}


P10F202::P10F202(const char *_name, const char *desc)
  : P10F200(_name,desc)
{
  if(verbose)
    cout << "10f202 constructor, type = " << isa() << '\n';
}

//========================================================================
// Comparator module for the 10c204 and 10c206
//
class Comparator10C20x
{
public:
  Comparator10C20x();
  ~Comparator10C20x();

};

class CMCON0;
//========================================================================
// COUT_SignalSource
//
// The comparator output is driven on to the GPIO pin if the COUTEN bit in
// CMCON0 is cleared ( and if the FOSC/4 logic is not driving).
// This is implemented via COUT_SignalSource. When COUTEN bit is asserted,
// then COUT_SignalSource overides the default output driver control for
// the GPIO pin.

class COUT_SignalSource : public SignalControl
{
public:
  COUT_SignalSource(CMCON0 *pcmcon0);
  ~COUT_SignalSource()
  {
  }
  char getState();
  void release()
  {
    delete this;
  }
private:
  CMCON0 *m_cmcon0;
};

//========================================================================
// COUT_SignalControl -- controls GPIO2's direction when the comparator is
// enabled. When the comparator is enabled, GPIO2 is an output.

class COUT_SignalControl : public SignalControl
{
public:
  COUT_SignalControl(){}
  ~COUT_SignalControl(){}
  char getState() { return '0'; }
  void release()
  {
    delete this;
  }
};


class CIN_SignalSink;
class CMCON0 : public sfr_register
{
public:
  enum {
    CWU    = 1<<0,
    CPREF  = 1<<1,
    CNREF  = 1<<2,
    CMPON  = 1<<3,
    CMPTOCS = 1<<4,
    POL    = 1<<5,
    COUTEN = 1<<6,
    CMPOUT = 1<<7
  };

  CMCON0(P10F204 *pCpu, const char *pName, const char *pDesc,
         PinModule *CInP, PinModule *CInM, PinModule *COut);

  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  bool isEnabled()
  {
    return ((value.get() & COUTEN) == 0);
  }

  char getState()
  {
    char ret='Z';
    if ( (value.get() & (COUTEN | CMPON)) == CMPON)
      ret = (((value.get() & CMPOUT)==CMPOUT) ^ ((value.get() & POL)==POL)) ? '0' : '1';
    cout <<"CMCON0::getState-->"<<ret << endl;
    return ret;
  }

  void refresh();

  SignalControl *getSource()
  {
    return m_source;
  }
  SignalControl *getGPDirectionControl()
  {
    return m_control;
  }

  void setInputState(char newState, bool bInput);
private:
  P10F204 *p_F204;
  COUT_SignalControl *m_control;
  COUT_SignalSource *m_source;
  CIN_SignalSink    *m_PosInput;
  CIN_SignalSink    *m_NegInput;

  PinModule *m_CInP;
  PinModule *m_CInM;
  PinModule *m_COut;

  double m_pV, m_nV;

};


COUT_SignalSource::COUT_SignalSource(CMCON0 *pcmcon0)
  : m_cmcon0(pcmcon0)
{
}
char COUT_SignalSource::getState()
{
    return m_cmcon0->getState();
}

class CIN_SignalSink : public SignalSink
{
public:
  CIN_SignalSink(CMCON0 *pcmcon0, bool binput)
    : m_cmcon0(pcmcon0),
      m_binput(binput)  // true==+input
  {}
  void setSinkState(char new3State)
  {
    cout << "LINE:"<<__LINE__<< "  "<< (m_binput ? "POS ":"NEG ")
         <<"set sink:"<<new3State << endl;

    m_cmcon0->setInputState(new3State, m_binput);
  }
  void release()
  {
    delete this;
  }
private:
  CMCON0 *m_cmcon0;
  bool m_binput;
};

//-----------------------------------------------------------
CMCON0::CMCON0(P10F204 *pCpu, const char *pName, const char *pDesc,
               PinModule *CInP, PinModule *CInM, PinModule *COut)
  : sfr_register(pCpu, pName, pDesc),
    p_F204(pCpu),
    m_CInP(CInP),
    m_CInM(CInM),
    m_COut(COut)
{
  // assign the I/O pin associated with the
  // the comparator output.

  m_source = new COUT_SignalSource(this);
  m_control = new COUT_SignalControl();
  m_PosInput = new CIN_SignalSink(this,true);
  m_NegInput = new CIN_SignalSink(this,false);

  CInP->addSink(m_PosInput);
  CInM->addSink(m_NegInput);
  //COut->setSource(m_source);

  m_pV = m_nV = 0.0;
}

void CMCON0::put(unsigned int new_value)
{
  unsigned old_value = value.get();
  trace.raw(write_trace.get() | old_value);
  value.put((new_value & 0x7f ) | (old_value & CMPOUT) );

  // If any of the control bits that afffect CMPOUT have changed,
  // then refresh CMPOUT
  if ((old_value ^ new_value) & (CPREF | CNREF | CMPON | CMPTOCS | POL))
    refresh();

  // If the output enable changed states.
  if ((old_value ^ new_value) & COUTEN)
    p_F204->updateGP2Source();

  // If the comparator output state has changed or the polarity changed:
  if ((old_value ^ value.get()) & (CMPOUT | POL))
    m_COut->updatePinModule();
}

void CMCON0::refresh()
{
  if (value.get() & CMPON) {
    if (value.get() & CPREF)
      m_pV = m_CInP->getPin().get_nodeVoltage();
    else
      m_pV = m_CInM->getPin().get_nodeVoltage();

    if (value.get() & CNREF)
      m_nV = m_CInM->getPin().get_nodeVoltage();
    else
      m_nV = 0.6;

    value.put( (value.get() & 0x7f) | ((m_pV>m_nV)? CMPOUT : 0));
  }
}

void CMCON0::put_value(unsigned int new_value)
{
}

void CMCON0::setInputState(char newState, bool bInput)
{

  if (bInput) {
    if (value.get() & CPREF)
      m_pV = m_CInP->getPin().get_nodeVoltage();
  }
  else {
    if ((value.get() & CPREF) == 0)
      m_pV = m_CInM->getPin().get_nodeVoltage();
    if (value.get() & CNREF)
      m_nV = m_CInM->getPin().get_nodeVoltage();
    else
      m_nV = 0.6;
  }

  cout << " setInputState: pV="<<m_pV<<",nV="<<m_nV << endl;

  unsigned old_value = value.get();
  trace.raw(write_trace.get() | old_value);
  value.put( (old_value&0x7f) | ((m_pV>m_nV) ? CMPOUT : 0));

  m_COut->updatePinModule();

}

//========================================================================
P10F204::P10F204(const char *_name, const char *desc)
  : P10F200(_name,desc)
{
}

void P10F204::create()
{
  P10F200::create();

  m_cmcon0 = new CMCON0(this, "cmcon0", "Comparator Control",
                        &(*m_gpio)[0], &(*m_gpio)[1], &(*m_gpio)[2]);

  RegisterValue porVal = RegisterValue(0xff,0);
  add_sfr_register(m_cmcon0, 7, porVal);

}

void P10F204::updateGP2Source()
{
  //    m_gpio->getIOpins(2)->setSource(m_cmcon0->getSource());
  PinModule *pmGP2 = &(*m_gpio)[2];

  if (m_cmcon0->isEnabled()) {
    pmGP2->setControl(m_cmcon0->getGPDirectionControl());
    pmGP2->setSource(m_cmcon0->getSource());
    cout << "comparator is controlling the output\n";
  } else
    P10F200::updateGP2Source();
}

//========================================================================
Processor * P10F204::construct(const char *name)
{

  P10F204 *p = new P10F204(name);

  p->pc->set_reset_address(0x1ff);

  p->create();
  p->create_symbols();
  return p;

}
//========================================================================
P10F220::P10F220(const char *_name, const char *desc)
  : P10F200(_name,desc),
    adcon0(this,"adcon0", "A2D Control 0"),
    adcon1(this,"adcon1", "A2D Control 1"),
    adres(this,"adres", "A2D Result")

{
}

void P10F220::create()
{
  P10F200::create();
  add_sfr_register(&adcon0,  0x07, RegisterValue(0xcc,0));
  add_sfr_register(&adres,  0x08, RegisterValue(0,0));

  adcon1.setValidCfgBits(ADCON1::PCFG0 | ADCON1::PCFG1,0);
  adcon1.setNumberOfChannels(4);
  adcon1.setIOPin(0, &(*m_gpio)[0]);
  adcon1.setIOPin(1, &(*m_gpio)[1]);
  adcon1.setVoltRef(2, 0.6);
  adcon1.setVoltRef(3, 0.6);
  adcon1.setChannelConfiguration(0, 0x03);
  adcon1.setChannelConfiguration(1, 0x03);
  adcon1.setChannelConfiguration(2, 0x00);
  adcon1.setChannelConfiguration(3, 0x00);

  adcon0.setChannel_Mask(3);
  adcon0.setChannel_shift(2);
  adcon0.setAdres(&adres);
  adcon0.setAdresLow(0);
  adcon0.setAdcon1(&adcon1);
  adcon0.setA2DBits(8);




/*
  m_cmcon0 = new CMCON0(this, "cmcon0", "Comparator Control",
                        &(*m_gpio)[0], &(*m_gpio)[1], &(*m_gpio)[2]);
*/

  RegisterValue porVal = RegisterValue(0xff,0);
 // add_sfr_register(m_cmcon0, 7, porVal);

}

#ifdef RRR
void P10F220::updateGP2Source()
{
/*
  PinModule *pmGP2 = &(*m_gpio)[2];

  if (m_cmcon0->isEnabled()) {
    pmGP2->setControl(m_cmcon0->getGPDirectionControl());
    pmGP2->setSource(m_cmcon0->getSource());
    cout << "comparator is controlling the output\n";
  } else
*/
    P10F200::updateGP2Source();
}
#endif

//========================================================================
Processor * P10F220::construct(const char *name)
{

  P10F220 *p = new P10F220(name);

  p->pc->set_reset_address(0xff);

  p->create();
  p->create_symbols();
  // RRR
  //pma->init_program_memory(0xff, 0x0c00);
  //p->program_memory[map_pm_adress2index(0xff)] = 0x0c00;
  //p->program_memory[0xff] = disasm12(p->get_cpu(), 0xff, 0x0c00);
  return p;

}
void P10F220::enter_sleep()
{
  unsigned int val;

  pic_processor::enter_sleep();

  status->put( status->get() & ~STATUS_GPWUF);
  //cout << "RRR enter sleep status="<<hex <<status->get()<<endl;
  val = (adcon0.get() & ~(ADCON0_10::ADON|ADCON0_10::GO)) 
  	| ADCON0_10::CHS1 | ADCON0_10::CHS0;
  adcon0.put(val);
}
void P10F220::exit_sleep()
{
  pic_processor::exit_sleep();
  //cout << "RRR exit sleep status="<<hex <<status->get()<<endl;
  adcon0.put(adcon0.get() | ADCON0_10::ANS1 | ADCON0_10::ANS0);
}
//========================================================================
P10F222::P10F222(const char *_name, const char *desc)
  : P10F220(_name,desc)
{
}

void P10F222::create()
{
  P10F220::create();
  add_file_registers(0x09, 0x0f, 0);    // 10F222 has 23 bytes RAM

/*
  m_cmcon0 = new CMCON0(this, "cmcon0", "Comparator Control",
                        &(*m_gpio)[0], &(*m_gpio)[1], &(*m_gpio)[2]);
*/

  RegisterValue porVal = RegisterValue(0xff,0);
 // add_sfr_register(m_cmcon0, 7, porVal);

}

#ifdef RRR
void P10F222::updateGP2Source()
{
/*
  PinModule *pmGP2 = &(*m_gpio)[2];

  if (m_cmcon0->isEnabled()) {
    pmGP2->setControl(m_cmcon0->getGPDirectionControl());
    pmGP2->setSource(m_cmcon0->getSource());
    cout << "comparator is controlling the output\n";
  } else
*/
    P10F200::updateGP2Source();
}
#endif

//========================================================================
Processor * P10F222::construct(const char *name)
{

  P10F222 *p = new P10F222(name);

  p->pc->set_reset_address(0x1ff);

  p->create();
  p->create_symbols();
  return p;

}
