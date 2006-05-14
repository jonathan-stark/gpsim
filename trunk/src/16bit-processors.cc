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

#include <stdio.h>
#include <iostream>
#include <iomanip>

//#include "14bit-registers.h"
//#include "14bit-instructions.h"
#include "../config.h"
#include "16bit-processors.h"

#include <string>
#include "stimuli.h"
#include "symbol.h"
#include "eeprom.h"
//-------------------------------------------------------------------
_16bit_processor::_16bit_processor()
  : pir1(0,0), pir2(0,0)
{

  package = 0;
  cout << "FIXME: 16bit processor is assuming that PLL is on - should check config bits\n";
  pll_factor = 2;

  pc = new Program_Counter16();
  pc->set_trace_command(trace.allocateTraceType(new PCTraceType(this,0,1)));

  m_porta = new PicPortRegister("porta",8,0x7f);
  m_trisa = new PicTrisRegister("trisa", m_porta);
  m_lata  = new PicLatchRegister("lata", m_porta);
  m_lata->setEnableMask(0x7f);

  m_portb = new PicPortRegister("portb",8,0xff);
  m_trisb = new PicTrisRegister("trisb", m_portb);
  m_latb  = new PicLatchRegister("latb", m_portb);

  m_portc = new PicPortRegister("portc",8,0xff);
  m_trisc = new PicTrisRegister("trisc", m_portc);
  m_latc  = new PicLatchRegister("latc", m_portc);

}
//-------------------------------------------------------------------
pic_processor *_16bit_processor::construct()
{
    cout << "creating 16bit processor construct\n";

  _16bit_processor *p = new _16bit_processor;

  if(verbose)
    cout << " 18c242 construct\n";

  p->create();
  p->create_invalid_registers();
  p->create_symbols();
  p->name_str = "generic 16bit processor";
  symbol_table.add_module(p,p->name_str.c_str());

  return p;


}

//-------------------------------------------------------------------
void _16bit_processor :: create_sfr_map()
{
  if(verbose)
    cout << "creating 18cxxx common registers\n";

  add_file_registers(0x0, 0xf7f, 0);

  RegisterValue porv(0,0);

  add_sfr_register(m_porta,       0xf80,porv);
  add_sfr_register(m_portb,       0xf81,porv);
  add_sfr_register(m_portc,       0xf82,porv);

  add_sfr_register(m_lata,        0xf89,porv);
  add_sfr_register(m_latb,        0xf8a,porv);
  add_sfr_register(m_latc,        0xf8b,porv);

  add_sfr_register(m_trisa,       0xf92,RegisterValue(0x7f,0));
  add_sfr_register(m_trisb,       0xf93,RegisterValue(0xff,0));
  add_sfr_register(m_trisc,       0xf94,RegisterValue(0xff,0));

  add_sfr_register(&pie1,	  0xf9d,porv,"pie1");
  add_sfr_register(&pir1,	  0xf9e,porv,"pir1");

  add_sfr_register(&ipr1,	  0xf9f,porv,"ipr1");
  add_sfr_register(&pie2,	  0xfa0,porv,"pie2");
  add_sfr_register(&pir2,	  0xfa1,porv,"pir2");
  add_sfr_register(&ipr2,	  0xfa2,porv,"ipr2");


  usart16.initialize(&pir_set_def,&(*m_portc)[6], &(*m_portc)[7],
		     new TXREG_16(), new RCREG_16());

  add_sfr_register(&usart16.rcsta,    0xfab,porv,"rcsta");
  add_sfr_register(&usart16.txsta,    0xfac,RegisterValue(0x02,0),"txsta");
  add_sfr_register(usart16.txreg,     0xfad,porv,"txreg");
  add_sfr_register(usart16.rcreg,     0xfae,porv,"rcreg");
  add_sfr_register(&usart16.spbrg,    0xfaf,porv,"spbrg");

  add_sfr_register(&t3con,	  0xfb1,porv,"t3con");
  add_sfr_register(&tmr3l,	  0xfb2,porv,"tmr3l");
  add_sfr_register(&tmr3h,	  0xfb3,porv,"tmr3h");

  add_sfr_register(&ccp2con,	  0xfba,porv,"ccp2con");
  add_sfr_register(&ccpr2l,	  0xfbb,porv,"ccpr2l");
  add_sfr_register(&ccpr2h,	  0xfbc,porv,"ccpr2h");
  add_sfr_register(&ccp1con,	  0xfbd,porv,"ccp1con");
  add_sfr_register(&ccpr1l,	  0xfbe,porv,"ccpr1l");
  add_sfr_register(&ccpr1h,	  0xfbf,porv,"ccpr1h");

  add_sfr_register(&adcon1,	  0xfc1,porv,"adcon1");
  add_sfr_register(&adcon0,	  0xfc2,porv,"adcon0");
  add_sfr_register(&adresl,	  0xfc3,porv,"adresl");
  add_sfr_register(&adresh,	  0xfc4,porv,"adresh");

  add_sfr_register(&ssp.sspcon1,  0xfc5,porv,"sspcon1");
  add_sfr_register(&ssp.sspcon2,  0xfc6,porv,"sspcon2");
  add_sfr_register(&ssp.sspstat,  0xfc7,porv,"sspstat");
  add_sfr_register(&ssp.sspadd,   0xfc8,porv,"sspadd");
  //add_sfr_register(&ssp.sspbuf,   0xfc9,porv,"sspbuf");

  add_sfr_register(&t2con,	  0xfca,porv,"t2con");
  add_sfr_register(&pr2,	  0xfcb,RegisterValue(0xff,0),"pr2");
  add_sfr_register(&tmr2,	  0xfcc,porv,"tmr2");

  add_sfr_register(&t1con,	  0xfcd,porv,"t1con");
  add_sfr_register(&tmr1l,	  0xfce,porv,"tmr1l");
  add_sfr_register(&tmr1h,	  0xfcf,porv,"tmr1h");

  add_sfr_register(&rcon,	  0xfd0,porv,"rcon");
  add_sfr_register(&wdtcon,	  0xfd1,porv,"wdtcon");
  add_sfr_register(&lvdcon,	  0xfd2,porv,"lvdcon");
  add_sfr_register(&osccon,	  0xfd3,porv,"osccon");
  // 0x4 is not defined
  add_sfr_register(&t0con,	  0xfd5,RegisterValue(0xff,0),"t0con");
  add_sfr_register(&tmr0l,	  0xfd6,porv,"tmr0l");
  add_sfr_register(&tmr0h,	  0xfd7,porv,"tmr0h");
  t0con.put(0xff);  /**FIXME - need a way to set this to 0xff at reset*/

  add_sfr_register(status,       0xfd8);

  add_sfr_register(&ind2.fsrl,	  0xfd9,porv,"fsr2l");
  add_sfr_register(&ind2.fsrh,    0xfda,porv,"fsr2h");
  add_sfr_register(&ind2.plusw,   0xfdb,porv,"plusw2");
  add_sfr_register(&ind2.preinc,  0xfdc,porv,"preinc2");
  add_sfr_register(&ind2.postdec, 0xfdd,porv,"postdec2");
  add_sfr_register(&ind2.postinc, 0xfde,porv,"postinc2");
  add_sfr_register(&ind2.indf,    0xfdf,porv,"indf2");

  add_sfr_register(&bsr,          0xfe0,porv,"bsr");

  add_sfr_register(&ind1.fsrl,	  0xfe1,porv,"fsr1l");
  add_sfr_register(&ind1.fsrh,    0xfe2,porv,"fsr1h");
  add_sfr_register(&ind1.plusw,   0xfe3,porv,"plusw1");
  add_sfr_register(&ind1.preinc,  0xfe4,porv,"preinc1");
  add_sfr_register(&ind1.postdec, 0xfe5,porv,"postdec1");
  add_sfr_register(&ind1.postinc, 0xfe6,porv,"postinc1");
  add_sfr_register(&ind1.indf,    0xfe7,porv,"indf1");

  add_sfr_register(W,            0xfe8);

  add_sfr_register(&ind0.fsrl,	  0xfe9,porv,"fsr0l");
  add_sfr_register(&ind0.fsrh,    0xfea,porv,"fsr0h");
  add_sfr_register(&ind0.plusw,   0xfeb,porv,"plusw0");
  add_sfr_register(&ind0.preinc,  0xfec,porv,"preinc0");
  add_sfr_register(&ind0.postdec, 0xfed,porv,"postdec0");
  add_sfr_register(&ind0.postinc, 0xfee,porv,"postinc0");
  add_sfr_register(&ind0.indf,    0xfef,porv,"indf0");

  add_sfr_register(&intcon3, 0xff0, porv,"intcon3");
  add_sfr_register(&intcon2, 0xff1, porv,"intcon2");
  add_sfr_register(&intcon,  0xff2, porv,"intcon");

  add_sfr_register(&prodl, 0xff3, porv,"prodl");
  add_sfr_register(&prodh, 0xff4, porv,"prodh");

  add_sfr_register(&tbl.tablat,  0xff5, porv,"tablat");
  add_sfr_register(&tbl.tabptrl, 0xff6, porv,"tabptrl");
  add_sfr_register(&tbl.tabptrh, 0xff7, porv,"tabptrh");
  add_sfr_register(&tbl.tabptru, 0xff8, porv,"tabptru");

  if(pcl)
    delete pcl;
  pcl = new PCL16();

  add_sfr_register(pcl,     0xff9);
  add_sfr_register(pclath,  0xffa);
  add_sfr_register(&pclatu, 0xffb, porv, "pclatu");

  stack = &stack16;
  add_sfr_register(&stack16.stkptr,  0xffc,porv,"stkptr");
  add_sfr_register(&stack16.tosl,    0xffd,porv,"tosl");
  add_sfr_register(&stack16.tosh,    0xffe,porv,"tosh");
  add_sfr_register(&stack16.tosu,    0xfff,porv,"tosu");


  EEPROM *e = get_eeprom();

  if (e) {
    add_sfr_register(e->get_reg_eedata(), 0xfa8);
    add_sfr_register(e->get_reg_eeadr(), 0xfa9);
    add_sfr_register(e->get_reg_eecon1(), 0xfa6, RegisterValue(0,0));
    add_sfr_register(e->get_reg_eecon2(), 0xfa7);
  }

  // Initialize all of the register cross linkages
  pir_set_def.set_pir1(&pir1);
  pir_set_def.set_pir2(&pir2);

  tmr1l.tmrh   = &tmr1h;
  tmr1l.t1con  = &t1con;
  tmr1l.setInterruptSource(new InterruptSource(&pir1, PIR1v1::TMR1IF));
  tmr1l.ccpcon = &ccp1con;

  tmr1h.tmrl  = &tmr1l;

  t1con.tmrl  = &tmr1l;

  t2con.tmr2  = &tmr2;
  tmr2.pir_set = &pir_set_def; //get_pir_set();
  tmr2.pr2    = &pr2;
  tmr2.t2con  = &t2con;
  tmr2.ccp1con = &ccp1con;
  tmr2.ccp2con = &ccp2con;
  pr2.tmr2    = &tmr2;


  tmr3l.tmrh  = &tmr3h;
  tmr3l.t1con = &t3con;
  tmr3l.setInterruptSource(new InterruptSource(&pir2, PIR2v2::TMR3IF));
  tmr3l.ccpcon = &ccp1con;

  tmr3h.tmrl  = &tmr3l;

  t3con.tmrl  = &tmr3l;
  t3con.tmr1l = &tmr1l;
  t3con.ccpr1l = &ccpr1l;
  t3con.ccpr2l = &ccpr2l;

  ccp1con.setCrosslinks(&ccpr1l, &pir_set_def, &tmr2);
  ccp1con.setIOpin(&((*m_portc)[2]));
  ccpr1l.ccprh  = &ccpr1h;
  ccpr1l.tmrl   = &tmr1l;
  ccpr1h.ccprl  = &ccpr1l;

  pir1.set_intcon(&intcon);
  pir1.set_pie(&pie1);
  pie1.pir = &pir1;
  pie1.new_name("pie1");

  pir2.set_intcon(&intcon);
  pir2.set_pie(&pie2);
  
  pie2.pir    = &pir2;
  pie2.new_name("pie2");

  // All of the status bits on the 16bit core are writable
  status->write_mask = 0xff;


  adcon0.setAdresLow(&adresl);
  adcon0.setAdres(&adresh);
  adcon0.setAdcon1(&adcon1);
  adcon0.setIntcon(&intcon);
  adcon0.setPir(&pir1);
  adcon0.setChannel_Mask(7); // Greater than 4 channels
  adcon0.setA2DBits(10);

  adcon1.setValidCfgBits(ADCON1::PCFG0 | ADCON1::PCFG1 | 
			 ADCON1::PCFG2 | ADCON1::PCFG3,0);

  adcon1.setNumberOfChannels(8);	// Allow 8 channel configuration
  adcon1.setChannelConfiguration(0, 0xff);
  adcon1.setChannelConfiguration(1, 0xff);
  adcon1.setChannelConfiguration(2, 0x1f);
  adcon1.setChannelConfiguration(3, 0x1f);
  adcon1.setChannelConfiguration(4, 0x0b);
  adcon1.setChannelConfiguration(5, 0x0b);
  adcon1.setChannelConfiguration(6, 0x00);
  adcon1.setChannelConfiguration(7, 0x00);
  adcon1.setChannelConfiguration(8, 0xff);
  adcon1.setChannelConfiguration(9, 0x3f);
  adcon1.setChannelConfiguration(10, 0x3f);
  adcon1.setChannelConfiguration(11, 0x3f);
  adcon1.setChannelConfiguration(12, 0x1f);
  adcon1.setChannelConfiguration(13, 0x0f);
  adcon1.setChannelConfiguration(14, 0x01);
  adcon1.setChannelConfiguration(15, 0x0d);

  adcon1.setVrefHiConfiguration(1, 3);
  adcon1.setVrefHiConfiguration(3, 3);
  adcon1.setVrefHiConfiguration(5, 3);
  adcon1.setVrefHiConfiguration(8, 3);
  adcon1.setVrefHiConfiguration(10, 3);
  adcon1.setVrefHiConfiguration(11, 3);
  adcon1.setVrefHiConfiguration(12, 3);
  adcon1.setVrefHiConfiguration(13, 3);
  adcon1.setVrefHiConfiguration(15, 3);

  adcon1.setVrefLoConfiguration(8, 2);
  adcon1.setVrefLoConfiguration(11, 2);
  adcon1.setVrefLoConfiguration(12, 2);
  adcon1.setVrefLoConfiguration(13, 2);
  adcon1.setVrefLoConfiguration(15, 2);

  adcon1.setNumberOfChannels(5);
  adcon1.setIOPin(0, &(*m_porta)[0]);
  adcon1.setIOPin(1, &(*m_porta)[1]);
  adcon1.setIOPin(2, &(*m_porta)[2]);
  adcon1.setIOPin(3, &(*m_porta)[3]);
  adcon1.setIOPin(4, &(*m_porta)[5]);
  // AN5,AN6 and AN7 exist only on devices with a PORTE.
}

//-------------------------------------------------------------------
//
// 
//    create
//
//  The purpose of this member function is to 'create' those things
// that are unique to the 16-bit core processors.

void _16bit_processor :: create ()
{

  if(verbose)
    cout << " _16bit_processor :: create\n" ;

  fast_stack.init(this);
  ind0.init(this);
  ind1.init(this);
  ind2.init(this);
  //usart16 = new USART_MODULE16();

  pic_processor::create();
  create_sfr_map();

  tmr0l.initialize();

  intcon.set_rcon(&rcon);
  intcon.set_intcon2(&intcon2);
  intcon.set_cpu(this);

  tbl.initialize(this);
  tmr0l.start(0);

  if(pma) {

    pma->SpecialRegisters.push_back(&bsr);
    rma.SpecialRegisters.push_back(&bsr);
  }
}

//
// create_symbols
//
//  Create symbols for a generic 16-bit core. This allows symbolic
// access to the pic. (e.g. It makes it possible to access the 
// status register by name instead of by its address.)
//

void _16bit_processor::create_symbols ()
{
  pic_processor::create_symbols();
}


//-------------------------------------------------------------------
// void _16bit_processor::interrupt ()
//
//  When the virtual function cpu->interrupt() is called during 
// pic_processor::run() AND the cpu gpsim is simulating is an 18cxxx
// device then we end up here. For an interrupt to have occured,
// the interrupt processing logic must have just ran. One of the
// responsibilities of that logic is to determine at what address
// the interrupt should begin executing. That address is placed
// in 'interrupt_vector'.
//
//-------------------------------------------------------------------
void _16bit_processor::
interrupt ()
{
  
  bp.clear_interrupt();

  stack->push(pc->value);

  // Save W,status, and BSR if this is a high priority interrupt.

  fast_stack.push();

  intcon.clear_gies();  // The appropriate gie bits get cleared (not just gieh)

  pc->jump(intcon.get_interrupt_vector());

}
//-------------------------------------------------------------------
void _16bit_processor::por()
{
  pic_processor::por();
}

//-------------------------------------------------------------------
void _16bit_processor::option_new_bits_6_7(unsigned int bits)
{

  //portb.rbpu_intedg_update(bits);
  //cout << "16bit, option bits 6 and/or 7 changed\n";

}

#define FOSC0   1<<0
#define FOSC1   1<<1
#define FOSC2   1<<2
#define OSCEN   1<<5

#define WDTEN   ((1<<0) << 8)
#define WDTPS0  ((1<<1) << 8)
#define WDTPS1  ((1<<2) << 8)
#define WDTPS2  ((1<<3) << 8)


#define PWRTEN  1<<0
#define BOREN   1<<1
#define BORV0   1<<2
#define BORV1   1<<2

#define CCP2MX  1<<0

#define STVREN  1<<0
//-------------------------------------------------------------------
void _16bit_processor::set_out_of_range_pm(unsigned int address, unsigned int value)
{
  cout << hex << "16bit proc setting config address 0x" <<(address<<1) << " to 0x"<<value<<'\n';
  switch (address) {
  case CONFIG2:
    if(config_modes) {
      if(value & WDTEN) {
	//if(verbose)
	  cout << "config Enabling WDT\n";

	config_modes->enable_wdt();
      } else {
	cout << "config disabling WDT\n";
	config_modes->disable_wdt();
      }
    }

    break;

  case CONFIG1:
    if(((value>>8) & (OSCEN | FOSC0 | FOSC1 | FOSC2)) != (OSCEN | FOSC0 | FOSC1 | FOSC2))
      cout << "FOSC bits in CONFIG1H are not supported\n";

    cout << "18cxxx config address 0x" << hex << (address<<1) << " Copy protection " <<
      (value&0xff) << '\n';

    break;

  case CONFIG3:
  case CONFIG4:
    cout << "18cxxx config address 0x" << hex << (address<<1) << " is not supported\n";
    break;

  case DEVID:

    cout << "18cxxx device id address 0x" << hex << (address<<1) << " is not supported\n";
    break;

  default:
    cout << "WARNING: 18cxxx is ignoring code at address 0x" << hex <<(address<<1) << '\n';
  }

}
//-------------------------------------------------------------------
//
// Package stuff
//
void _16bit_processor::create_iopin_map()
{
  cout << "_16bit_processor::create_iopin_map WARNING --- not creating package \n";
}

//------------------------------------------------------------------------
// Latch Register
// (Probably should put this somewhere else)
PicLatchRegister::PicLatchRegister(const char *_name, PortRegister *_port)
  : m_port(_port), m_EnableMask(0xff)
{
  new_name(_name);
}

void PicLatchRegister::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.data);
  value.data = new_value & m_EnableMask;
  m_port->put_value(value.data);
}
void PicLatchRegister::put_value(unsigned int new_value)
{
  value.data = new_value & m_EnableMask;
  m_port->put_value(value.data);
}
unsigned int PicLatchRegister::get()
{
  trace.raw(read_trace.get()  | value.data);
  trace.raw(read_trace.geti() | value.init);

  return value.data;
}
void PicLatchRegister::setbit(unsigned int bit_number, char new_value)
{
  printf("PicLatchRegister::setbit() -- shouldn't be called\n");
}
void PicLatchRegister::setEnableMask(unsigned int nEnableMask)
{
  m_EnableMask = nEnableMask;
}
