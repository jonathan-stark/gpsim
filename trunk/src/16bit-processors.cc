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


//-------------------------------------------------------------------
_16bit_processor::_16bit_processor(void)
{

  package = NULL;
  cout << "FIXME: 16bit processor is assuming that PLL is on - should check config bits\n";
  pll_factor = 2;

  pc = new Program_Counter16();

}
//-------------------------------------------------------------------
pic_processor *_16bit_processor::construct(void)
{
    cout << "creating 16bit processor construct\n";

  _16bit_processor *p = new _16bit_processor;

  if(verbose)
    cout << " 18c242 construct\n";

  p->create();
  p->create_invalid_registers();
  p->pic_processor::create_symbols();
  p->name_str = "generic 16bit processor";
  symbol_table.add_module(p,p->name_str);

  return p;


}

//-------------------------------------------------------------------
void _16bit_processor :: create_sfr_map(void)
{
  //if(verbose)
    cout << "creating 18cxxx common registers\n";

  add_file_registers(0x0, 0xf7f, 0);


  add_sfr_register(&pie1,	  0xf9d,0,"pie1");
  //cout << get_pir1()->name() << '\n';
  add_sfr_register(get_pir1(),	  0xf9e,0,"pir1");

  add_sfr_register(&ipr1,	  0xf9f,0,"ipr1");
  add_sfr_register(&pie2,	  0xfa0,0,"pie2");
  add_sfr_register(get_pir2(),	  0xfa1,0,"pir2");
  add_sfr_register(&ipr2,	  0xfa2,0,"ipr2");


  add_sfr_register(&(usart16.rcsta),  0xfab,0,"rcsta");
  add_sfr_register(&(usart16.txsta),  0xfac,0x02,"txsta");
  add_sfr_register(&(usart16.txreg),  0xfad,0,"txreg");
  add_sfr_register(&(usart16.rcreg),  0xfae,0,"rcreg");
  add_sfr_register(&(usart16.spbrg),  0xfaf,0,"spbrg");

  add_sfr_register(&t3con,	  0xfb1,0,"t3con");
  add_sfr_register(&tmr3l,	  0xfb2,0,"tmr3l");
  add_sfr_register(&tmr3h,	  0xfb3,0,"tmr3h");

  add_sfr_register(&ccp2con,	  0xfba,0,"ccp2con");
  add_sfr_register(&ccpr2l,	  0xfbb,0,"ccpr2l");
  add_sfr_register(&ccpr2h,	  0xfbc,0,"ccpr2h");
  add_sfr_register(&ccp1con,	  0xfbd,0,"ccp1con");
  add_sfr_register(&ccpr1l,	  0xfbe,0,"ccpr1l");
  add_sfr_register(&ccpr1h,	  0xfbf,0,"ccpr1h");

//   add_sfr_register(&adcon1,	  0xfc1,0,"adcon1");
//   add_sfr_register(&adcon2,	  0xfc2,0,"adcon2");
//   add_sfr_register(&adresl,	  0xfc3,0,"adresl");
//   add_sfr_register(&adresh,	  0xfc4,0,"adres4");

  add_sfr_register(&ssp.sspcon1,  0xfc5,0,"sspcon1");
  add_sfr_register(&ssp.sspcon2,  0xfc6,0,"sspcon2");
  add_sfr_register(&ssp.sspstat,  0xfc7,0,"sspstat");
  add_sfr_register(&ssp.sspadd,   0xfc8,0,"sspadd");
  //add_sfr_register(&ssp.sspbuf,   0xfc9,0,"sspbuf");

  add_sfr_register(&t2con,	  0xfca,0,"t2con");
  add_sfr_register(&pr2,	  0xfcb,0xff,"pr2");
  add_sfr_register(&tmr2,	  0xfcc,0,"tmr2");

  add_sfr_register(&t1con,	  0xfcd,0,"t1con");
  add_sfr_register(&tmr1l,	  0xfce,0,"tmr1l");
  add_sfr_register(&tmr1h,	  0xfcf,0,"tmr1h");

  add_sfr_register(&rcon,	  0xfd0,0,"rcon");
  add_sfr_register(&wdtcon,	  0xfd1,0,"wdtcon");
  add_sfr_register(&lvdcon,	  0xfd2,0,"lvdcon");
  add_sfr_register(&osccon,	  0xfd3,0,"osccon");
  // 0x4 is not defined
  add_sfr_register(&t0con,	  0xfd5,0x00,"t0con");
  add_sfr_register(&tmr0l,	  0xfd6,0,"tmr0l");
  add_sfr_register(&tmr0h,	  0xfd7,0,"tmr0h");
  t0con.put(0xff);  /**FIXME - need a way to set this to 0xff at reset*/

  add_sfr_register(status,       0xfd8);

  add_sfr_register(&ind2.fsrl,	  0xfd9,0,"fsr2l");
  add_sfr_register(&ind2.fsrh,    0xfda,0,"fsr2h");
  add_sfr_register(&ind2.plusw,   0xfdb,0,"plusw2");
  add_sfr_register(&ind2.preinc,  0xfdc,0,"preinc2");
  add_sfr_register(&ind2.postdec, 0xfdd,0,"postdec2");
  add_sfr_register(&ind2.postinc, 0xfde,0,"postinc2");
  add_sfr_register(&ind2.indf,    0xfdf,0,"indf2");

  add_sfr_register(&bsr,          0xfe0,0,"bsr");

  add_sfr_register(&ind1.fsrl,	  0xfe1,0,"fsr1l");
  add_sfr_register(&ind1.fsrh,    0xfe2,0,"fsr1h");
  add_sfr_register(&ind1.plusw,   0xfe3,0,"plusw1");
  add_sfr_register(&ind1.preinc,  0xfe4,0,"preinc1");
  add_sfr_register(&ind1.postdec, 0xfe5,0,"postdec1");
  add_sfr_register(&ind1.postinc, 0xfe6,0,"postinc1");
  add_sfr_register(&ind1.indf,    0xfe7,0,"indf1");

  add_sfr_register(W,            0xfe8);

  add_sfr_register(&ind0.fsrl,	  0xfe9,0,"fsr0l");
  add_sfr_register(&ind0.fsrh,    0xfea,0,"fsr0h");
  add_sfr_register(&ind0.plusw,   0xfeb,0,"plusw0");
  add_sfr_register(&ind0.preinc,  0xfec,0,"preinc0");
  add_sfr_register(&ind0.postdec, 0xfed,0,"postdec0");
  add_sfr_register(&ind0.postinc, 0xfee,0,"postinc0");
  add_sfr_register(&ind0.indf,    0xfef,0,"indf0");

  add_sfr_register(&intcon3, 0xff0, 0,"intcon3");
  add_sfr_register(&intcon2, 0xff1, 0,"intcon2");
  add_sfr_register(&intcon,  0xff2, 0,"intcon");

  add_sfr_register(&prodl, 0xff3, 0,"prodl");
  add_sfr_register(&prodh, 0xff4, 0,"prodh");

  add_sfr_register(&tbl.tablat,  0xff5, 0,"tablat");
  add_sfr_register(&tbl.tabptrl, 0xff6, 0,"tabptrl");
  add_sfr_register(&tbl.tabptrh, 0xff7, 0,"tabptrh");
  add_sfr_register(&tbl.tabptru, 0xff8, 0,"tabptru");

  if(pcl)
    delete pcl;
  pcl = new PCL16();

  add_sfr_register(pcl,    0xff9);
  add_sfr_register(pclath, 0xffa);
  //add_sfr_register(&pclatu, 0xffb);

  add_sfr_register(&((Stack16 *)stack)->stkptr,  0xffc,0,"stkptr");
  add_sfr_register(&((Stack16 *)stack)->tosl,    0xffd,0,"tosl");
  add_sfr_register(&((Stack16 *)stack)->tosh,    0xffe,0,"tosh");
  add_sfr_register(&((Stack16 *)stack)->tosu,    0xfff,0,"tosu");


  // Initialize all of the register cross linkages
  get_pir_set()->set_pir1(get_pir1());
  get_pir_set()->set_pir2(get_pir2());

  tmr1l.tmrh   = &tmr1h;
  tmr1l.t1con  = &t1con;
  tmr1l.pir_set   = get_pir_set();
  tmr1l.ccpcon = &ccp1con;

  tmr1h.tmrl  = &tmr1l;

  t1con.tmrl  = &tmr1l;

  t2con.tmr2  = &tmr2;
  tmr2.pir_set   = get_pir_set();
  tmr2.pr2    = &pr2;
  tmr2.t2con  = &t2con;
  tmr2.ccp1con = &ccp1con;
  tmr2.ccp2con = &ccp2con;
  pr2.tmr2    = &tmr2;


  tmr3l.tmrh  = &tmr3h;
  tmr3l.t1con = &t3con;
  tmr3l.pir_set  = get_pir_set();
  tmr3l.ccpcon = &ccp1con;

  tmr3h.tmrl  = &tmr3l;

  t3con.tmrl  = &tmr3l;
  t3con.tmr1l = &tmr1l;
  t3con.ccpr1l = &ccpr1l;
  t3con.ccpr2l = &ccpr2l;

  ccp1con.ccprl = &ccpr1l;
  ccp1con.pir_set   = get_pir_set();
  ccp1con.tmr2  = &tmr2;
  ccpr1l.ccprh  = &ccpr1h;
  ccpr1l.tmrl   = &tmr1l;
  ccpr1h.ccprl  = &ccpr1l;

  get_pir1()->intcon = &intcon;
  get_pir1()->pie    = &pie1;
  pie1.pir    = get_pir1();
  pie1.new_name("pie1");

  get_pir2()->intcon = &intcon;
  get_pir2()->pie    = &pie2;
  pie2.pir    = get_pir2();
  pie2.new_name("pie2");

  // All of the status bits on the 16bit core are writable
  status->write_mask = 0xff;


  //cout << "end  txsta assignment name: " << usart.txsta->name() << "\n";
}

//-------------------------------------------------------------------
//
// 
//    create
//
//  The purpose of this member function is to 'create' those things
// that are unique to the 16-bit core processors.

void _16bit_processor :: create (void)
{

  if(verbose)
    cout << " _16bit_processor :: create\n" ;

  //create_iopin_map(&iopin_map, &num_of_iopins);
  //create_iopins(iopin_map, num_of_iopins);

  sfr_map = NULL;
  num_of_sfrs = 0;
 

  fast_stack.init(this);
  ind0.init(this);
  ind1.init(this);
  ind2.init(this);
  //usart16 = new USART_MODULE16();

  cout << "16bit_processor1 create txreg => " << usart16.txreg.name() << "\n";
  pic_processor::create();
  cout << "16bit_processor2 create txreg => " << usart16.txreg.name() << "\n";
  _16bit_processor::create_sfr_map();
  cout << "16bit_processor3 create txreg => " << usart16.txreg.name() << "\n";

  trace.program_counter (pc->value);
  tmr0l.initialize();

  intcon.set_rcon(&rcon);
  intcon.set_intcon2(&intcon2);
  intcon.cpu = this;

  tbl.initialize(this);
  tmr0l.start(0);

}

//
// create_symbols
//
//  Create symbols for a generic 16-bit core. This allows symbolic
// access to the pic. (e.g. It makes it possible to access the 
// status register by name instead of by its address.)
//

void _16bit_processor::create_symbols (void)
{

  cout << "16bit create symbols\n";

}


//-------------------------------------------------------------------
// void _16bit_processor::interrupt (void)
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
interrupt (void)
{
  
  bp.clear_interrupt();

  stack->push(pc->value);

  // Save W,status, and BSR if this is a high priority interrupt.
  //if(interrupt_vector == INTERRUPT_VECTOR_HI)
  cout << "16bit interrupt, vector = 0x" <<hex<<interrupt_vector<<'\n';
  fast_stack.push();

  intcon.clear_gies();  // The appropriate gie bits get cleared (not just gieh)

  pc->jump(interrupt_vector);

}
//-------------------------------------------------------------------
void _16bit_processor::por(void)
{
  pic_processor::por();
}

//-------------------------------------------------------------------
void _16bit_processor::option_new_bits_6_7(unsigned int bits)
{

  //portb.rbpu_intedg_update(bits);
  cout << "16bit, option bits 6 and/or 7 changed\n";

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
void _16bit_processor::set_out_of_range_pm(int address, int value)
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
void _16bit_processor::create_iopin_map(void)
{
  cout << "_16bit_processor::create_iopin_map WARNING --- not createing package \n";
}
