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
#include <iostream.h>
#include <iomanip.h>

//#include "14bit-registers.h"
//#include "14bit-instructions.h"
#include "16bit-processors.h"

#include <string>
#include "stimuli.h"


//-------------------------------------------------------------------
void _16bit_processor :: create_sfr_map(void)
{
  if(verbose)
    cout << "creating 18cxxx common registers\n";

  add_file_registers(0x0, 0xf7f, 0);


  //add_sfr_register(&porta,	  0xf80,0,"porta");
  //add_sfr_register(&portb,	  0xf81,0,"portb");
  //add_sfr_register(&portc,	  0xf82,0,"portc");
  //add_sfr_register(&portd,	  0xf83,0,"portd");
  //add_sfr_register(&porte,	  0xf84,0,"porte");

  //add_sfr_register(&lata,	  0xf89,0,"lata");
  //add_sfr_register(&latb,	  0xf8a,0,"latb");
  //add_sfr_register(&latc,	  0xf8b,0,"latc");
  //add_sfr_register(&latd,	  0xf8c,0,"latd");
  //add_sfr_register(&late,	  0xf8d,0,"late");

  //add_sfr_register(&trisa,	  0xf92,0,"trisa");
  //add_sfr_register(&trisb,	  0xf93,0,"trisb");
  //add_sfr_register(&trisc,	  0xf94,0,"trisc");
  //add_sfr_register(&trisd,	  0xf95,0,"trisd");
  //add_sfr_register(&trise,	  0xf96,0,"trise");

  //add_sfr_register(&pie1,	  0xf9d,0,"pie1");
  add_sfr_register(&pir1,	  0xf9e,0,"pir1");
  //add_sfr_register(&ipr1,	  0xf9f,0,"ipr1");
  //add_sfr_register(&pie2,	  0xfa0,0,"pie2");
  //add_sfr_register(&pir2,	  0xfa1,0,"pir2");
  //add_sfr_register(&ipr2,	  0xfa2,0,"ipr2");


  add_sfr_register(&usart.rcsta,	  0xfab,0,"rcsta");
  add_sfr_register(&usart.txsta,	  0xfac,0x02,"txsta");
  add_sfr_register(&usart.txreg,	  0xfad,0,"txreg");
  add_sfr_register(&usart.rcreg,	  0xfae,0,"rcreg");
  add_sfr_register(&usart.spbrg,	  0xfaf,0,"spbrg");

  //add_sfr_register(&t3con,	  0xfb1,0xff,"t3con");
  //add_sfr_register(&tmr3l,	  0xfb2,0,"tmr3l");
  //add_sfr_register(&tmr3h,	  0xfb3,0,"tmr3h");

  add_sfr_register(&rcon,	  0xfd0,0,"rcon");
  add_sfr_register(&t0con,	  0xfd5,0xff,"t0con");
  add_sfr_register(&tmr0l,	  0xfd6,0,"tmr0l");
  add_sfr_register(&tmr0h,	  0xfd7,0,"tmr0h");

  add_sfr_register(&status,       0xfd8);

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

  add_sfr_register(&W,            0xfe8);

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

  add_sfr_register(&pcl,    0xff9);
  add_sfr_register(&pclath, 0xffa);
  //add_sfr_register(&pclatu, 0xffb);

  add_sfr_register(&((Stack16 *)stack)->stkptr,  0xffc,0,"stkptr");
  add_sfr_register(&((Stack16 *)stack)->tosl,    0xffd,0,"tosl");
  add_sfr_register(&((Stack16 *)stack)->tosh,    0xffe,0,"tosh");
  add_sfr_register(&((Stack16 *)stack)->tosu,    0xfff,0,"tosu");

  pic_processor::create_symbols();

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

  //create_iopin_map(&iopin_map, &num_of_iopins);
  //create_iopins(iopin_map, num_of_iopins);

  sfr_map = NULL;
  num_of_sfrs = 0;
 

  pic_processor::create();

  _16bit_processor::create_sfr_map();

  fast_stack.init(this);
  ind0.init(this);
  ind1.init(this);
  ind2.init(this);
  trace.program_counter (pc.value);

  tmr0l.initialize();
  intcon.initialize();

  usart.initialize(this);
  tbl.initialize(this);

  tmr0l.start();

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

  stack->push(pc.value);

  intcon.clear_gies();  // The appropriate gie bits get cleared (not just gieh)

  pc.jump(interrupt_vector);

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

  switch (address) {
  case CONFIG2:
    if(value & WDTEN) {
      if(verbose)
	cout << "Enabling WDT\n";
      config_modes |= CM_WDTE;
    } else
      config_modes &= ~CM_WDTE;

    if((value & (FOSC0 | FOSC1 | FOSC2)) != (FOSC0 | FOSC1 | FOSC2))
      cout << "FOSC bits in CONFIG2H are not supported\n";

    break;

  case CONFIG1:
  case CONFIG3:
  case CONFIG4:
    cout << "18cxxx config address " << (address<<1) << " is not supported\n";
    break;

  case DEVID:

    cout << "18cxxx device id address " << (address<<1) << " is not supported\n";
    break;

  default:
    cout << "WARNING: 18cxxx is ignoring code at address " << (address<<1) << '\n';
  }

}
