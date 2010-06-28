/*
   Copyright (C) 1998 T. Scott Dattalo
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

#include <stdio.h>
#include <iostream>
#include <iomanip>

#include "../config.h"
#include "14bit-processors.h"
#include "pic-ioports.h"
#include "pic-registers.h"
#include <string>
#include "stimuli.h"
#include "packages.h"

//========================================================================
// Generic Configuration word for the midrange family.

class Generic14bitConfigWord : public ConfigWord
{
public:
  Generic14bitConfigWord(_14bit_processor *pCpu)
    : ConfigWord("CONFIG", 0x3fff, "Configuration Word", pCpu, 0x2007)
  {
    assert(pCpu);
    pCpu->wdt.initialize(true);
  }

  enum {
    FOSC0  = 1<<0,
    FOSC1  = 1<<1,
    WDTEN  = 1<<2,
    PWRTEN = 1<<3
  };

  virtual void set(gint64 v)
  {
    gint64 oldV = getVal();

    Integer::set(v);
    if (m_pCpu) {

      gint64 diff = oldV ^ v;

      if (diff & WDTEN)
        m_pCpu->wdt.initialize((v & WDTEN) == WDTEN);

      m_pCpu->config_modes->set_fosc01(v & (FOSC0 | FOSC1));
      m_pCpu->config_modes->set_wdte((v&WDTEN)==WDTEN);
      m_pCpu->config_modes->set_pwrte((v&PWRTEN)==PWRTEN);

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
             " PWRTEN=%d - Power up timer is %s\n",
             i,
             i&(FOSC0|FOSC1), (i&FOSC0 ? (i&FOSC1 ? "EXTRC":"XT"):(i&FOSC1 ? "INTRC":"LP")),
             (i&WDTEN?1:0), ((i&WDTEN) ? "enabled" : "disabled"),
             (i&PWRTEN?1:0), ((i&PWRTEN) ? "disabled" : "enabled"));

    return string(buff);
  }

};


//-------------------------------------------------------------------
_14bit_processor::_14bit_processor(const char *_name, const char *_desc)
  : pic_processor(_name,_desc), intcon(0)
{
  pc = new Program_Counter("pc", "Program Counter", this);
  pc->set_trace_command(); //trace.allocateTraceType(new PCTraceType(this,1)));
  option_reg = new OPTION_REG(this,"option_reg");
  stack = new Stack();
}

_14bit_processor::~_14bit_processor()
{
  delete_sfr_register(fsr);
  delete_sfr_register(option_reg);
  delete pc; pc=0;
}

//-------------------------------------------------------------------
//
//
//    create
//
//  The purpose of this member function is to 'create' those things
// that are unique to the 14-bit core processors.

void _14bit_processor :: create ()
{

  if(verbose)
    cout << "_14bit_processor create, type = " << isa() << '\n';

  pic_processor::create();
  fsr = new FSR(this, "fsr", "File Select Register for indirect addressing");
}



//-------------------------------------------------------------------
void _14bit_processor::interrupt ()
{

  bp.clear_interrupt();

  if (bp.have_sleep()) {
      bp.clear_sleep();
      stack->push(pc->value+1);
  } else {
      stack->push(pc->value);
  }
  intcon->clear_gie();

  pc->interrupt(INTERRUPT_VECTOR);

}

//-------------------------------------------------------------------
void _14bit_processor::save_state()
{
  pic_processor::save_state();

  option_reg->put_trace_state(option_reg->value);
}

//-------------------------------------------------------------------
void _14bit_processor::option_new_bits_6_7(unsigned int bits)
{
  cout << "14bit, option bits 6 and/or 7 changed\n";
}
//-------------------------------------------------------------------
void _14bit_processor::put_option_reg(unsigned int val)
{
  option_reg->put(val);
}


//------------------------------------------------------------------
// Fetch the rom contents at a particular address.
unsigned int _14bit_processor::get_program_memory_at_address(unsigned int address)
{
  unsigned int uIndex = map_pm_address2index(address);


  if (uIndex < program_memory_size())
    return  program_memory[uIndex] ? program_memory[uIndex]->get_opcode() : 0xffffffff;

  return get_config_word(address);
}

//-------------------------------------------------------------------
void _14bit_processor::create_config_memory()
{
  m_configMemory = new ConfigMemory(this,1);
  m_configMemory->addConfigWord(0,new Generic14bitConfigWord(this));
}

//-------------------------------------------------------------------

bool _14bit_processor::set_config_word(unsigned int address,unsigned int cfg_word)
{

  if((address == config_word_address()) && config_modes) {

    config_word = cfg_word;

    if (m_configMemory && m_configMemory->getConfigWord(0))
      m_configMemory->getConfigWord(0)->set((int)cfg_word);

    return true;
  }

  return false;

}

//-------------------------------------------------------------------
void _14bit_processor::enter_sleep()
{
    tmr0.sleep();
    pic_processor::enter_sleep();
}

 //-------------------------------------------------------------------
void _14bit_processor::exit_sleep()
{
  if (m_ActivityState == ePASleeping)
  {
    tmr0.wake();
    pic_processor::exit_sleep();
  }

}

//-------------------------------------------------------------------
Pic14Bit::Pic14Bit(const char *_name, const char *_desc)
  : _14bit_processor(_name,_desc),
    intcon_reg(this,"intcon","Interrupt Control")
{
  m_porta = new PicPortRegister(this,"porta","", 8,0x1f);
  m_trisa = new PicTrisRegister(this,"trisa","", m_porta, false);

  tmr0.set_cpu(this, m_porta, 4, option_reg);
  tmr0.start(0);

  m_portb = new PicPortBRegister(this,"portb","",&intcon_reg,8,0xff);
  m_trisb = new PicTrisRegister(this,"trisb","", m_portb, false);
}

//-------------------------------------------------------------------
Pic14Bit::~Pic14Bit()
{
  delete_sfr_register(m_portb);
  delete_sfr_register(m_trisb);

  delete_sfr_register(m_porta);
  delete_sfr_register(m_trisa);
}
//-------------------------------------------------------------------
void Pic14Bit::create_symbols()
{
  pic_processor::create_symbols();

  addSymbol(W);

}

//-------------------------------------------------------------------
void Pic14Bit::create_sfr_map()
{

  add_sfr_register(indf,    0x00);
  alias_file_registers(0x00,0x00,0x80);
  //add_sfr_register(indf,    0x00);

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
  //add_sfr_register(pclath,  0x8a, RegisterValue(0,0));

  add_sfr_register(&intcon_reg, 0x0b, RegisterValue(0,0));
  //add_sfr_register(&intcon_reg, 0x8b, RegisterValue(0,0));
  alias_file_registers(0x0a,0x0b,0x80);

  intcon = &intcon_reg;


}
//-------------------------------------------------------------------
void Pic14Bit::option_new_bits_6_7(unsigned int bits)
{
  //1 ((PORTB *)portb)->rbpu_intedg_update(bits);
  m_portb->setRBPU( (bits & (1<<7)) == (1<<7));
  m_portb->setIntEdge((bits & (1<<6)) == (1<<6));
}

