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
// p16x6x
//
//  This file supports:
//    P16C61


#include <stdio.h>
#include <iostream.h>
#include <string>

#include "symbol.h"

#include "p16x6x.h"


void P16C61::create_sfr_map(void)
{
  if(verbose)
    cout << "creating c61 registers\n";

  add_file_registers(0x0c, 0x2f, 0x80);

 
  add_sfr_register(&indf,  0x80);
  add_sfr_register(&indf,  0x00);

  add_sfr_register(&tmr0,  0x01);
  add_sfr_register(&option_reg,  0x81, 0xff);

  add_sfr_register(&pcl,    0x02, 0);
  add_sfr_register(&status, 0x03, 0x18);
  add_sfr_register(&fsr,    0x04);
  alias_file_registers(0x02,0x04,0x80);

  add_sfr_register(porta,   0x05);
  add_sfr_register(&trisa,  0x85, 0x3f);

  add_sfr_register(portb,   0x06);
  add_sfr_register(&trisb,  0x86, 0xff);

  add_sfr_register(&pclath, 0x8a, 0);
  add_sfr_register(&pclath, 0x0a, 0);

  add_sfr_register(&intcon_reg, 0x8b, 0);
  add_sfr_register(&intcon_reg, 0x0b, 0);

  sfr_map = NULL;
  num_of_sfrs = 0;

  intcon = &intcon_reg;

}


void P16C61::create_symbols(void)
{

  symbol_table.add_ioport(portb->cpu, portb);
  symbol_table.add_ioport(porta->cpu, porta);


}

void P16C61::create(void)
{

  create_iopin_map();

  _14bit_processor::create();

  P16C61::create_sfr_map();

}

pic_processor * P16C61::construct(void)
{

  P16C61 *p = new P16C61;

  if(verbose)
    cout << " c61 construct\n";

  p->create();
  p->create_invalid_registers ();
  p->pic_processor::create_symbols();
  p->name_str = "16c61";

  return p;

}

P16C61::P16C61(void)
{
  if(verbose)
    cout << "c61 constructor, type = " << isa() << '\n';

}


//
//------------------------------------------------------------------------
//
//

void _14bit_40pins::create_iopin_map(void)
{

  porta = new PORTA;
  portb = new PORTB;
  portc = new PORTC;
  portd = new PORTD;
  porte = new PORTE;

  // ---- Complete the initialization for the I/O Ports

  // Build the links between the I/O Ports and their tris registers.
  porta->tris = &trisa;
  trisa.port = porta;

  portb->tris = &trisb;
  trisb.port = portb;

  portc->tris = &trisc;
  trisc.port = portc;

  portd->tris = &trisd;
  trisd.port = portd;

  porte->tris = &trise;
  trise.port = porte;


  // And give them a more meaningful name.
  trisa.new_name("trisa");
  trisb.new_name("trisb");
  trisc.new_name("trisc");
  trisd.new_name("trisd");
  trise.new_name("trise");

  // Define the valid I/O pins.
  porta->valid_iopins = 0x1f;
  portb->valid_iopins = 0xff;
  portc->valid_iopins = 0xff;
  portd->valid_iopins = 0xff;
  porte->valid_iopins = 0x07;


  // Now Create the package and place the I/O pins

  create_pkg(40);

  assign_pin(1, NULL);

  assign_pin(2, new IO_bi_directional(porta, 0));
  assign_pin(3, new IO_bi_directional(porta, 1));
  assign_pin(4, new IO_bi_directional(porta, 2));
  assign_pin(5, new IO_bi_directional(porta, 3));
  assign_pin(6, new IO_open_collector(porta, 4));

  assign_pin(7, NULL);


  assign_pin(8, new IO_bi_directional(porte, 0));
  assign_pin(9, new IO_bi_directional(porte, 1));
  assign_pin(10, new IO_bi_directional(porte, 2));


  assign_pin(11, NULL);
  assign_pin(12, NULL);
  assign_pin(13, NULL);
  assign_pin(14, NULL);

  assign_pin(15, new IO_bi_directional(portc, 0));
  assign_pin(16, new IO_bi_directional(portc, 1));
  assign_pin(17, new IO_bi_directional(portc, 2));
  assign_pin(18, new IO_bi_directional(portc, 3));
  assign_pin(23, new IO_bi_directional(portc, 4));
  assign_pin(24, new IO_bi_directional(portc, 5));
  assign_pin(25, new IO_bi_directional(portc, 6));
  assign_pin(26, new IO_bi_directional(portc, 7));


  assign_pin(19, new IO_bi_directional(portd, 0));
  assign_pin(20, new IO_bi_directional(portd, 1));
  assign_pin(21, new IO_bi_directional(portd, 2));
  assign_pin(22, new IO_bi_directional(portd, 3));
  assign_pin(27, new IO_bi_directional(portd, 4));
  assign_pin(28, new IO_bi_directional(portd, 5));
  assign_pin(29, new IO_bi_directional(portd, 6));
  assign_pin(30, new IO_bi_directional(portd, 7));

  assign_pin(31, NULL);
  assign_pin(32, NULL);

  assign_pin(33, new IO_bi_directional_pu(portb, 0));
  assign_pin(34, new IO_bi_directional_pu(portb, 1));
  assign_pin(35, new IO_bi_directional_pu(portb, 2));
  assign_pin(36, new IO_bi_directional_pu(portb, 3));
  assign_pin(37, new IO_bi_directional_pu(portb, 4));
  assign_pin(38, new IO_bi_directional_pu(portb, 5));
  assign_pin(39, new IO_bi_directional_pu(portb, 6));
  assign_pin(40, new IO_bi_directional_pu(portb, 7));

}

void P16C64::create_sfr_map(void)
{
   if(verbose)
    cout << "creating c64 registers\n";

  add_file_registers(0x20, 0x7f, 0);
  add_file_registers(0xa0, 0xbf, 0);

 
  add_sfr_register(&indf,  0x80);
  add_sfr_register(&indf,  0x00);

  add_sfr_register(&tmr0,  0x01);
  add_sfr_register(&option_reg,  0x81, 0xff);

  add_sfr_register(&pcl,    0x02, 0);
  add_sfr_register(&status, 0x03, 0x18);
  add_sfr_register(&fsr,    0x04);
  alias_file_registers(0x02,0x04,0x80);

  add_sfr_register(porta,   0x05);
  add_sfr_register(&trisa,  0x85, 0x3f);

  add_sfr_register(portb,   0x06);
  add_sfr_register(&trisb,  0x86, 0xff);

  add_sfr_register(portc,   0x07);
  add_sfr_register(&trisc,  0x87, 0xff);

  add_sfr_register(portd,   0x08);
  add_sfr_register(&trisd,  0x88, 0xff);

  add_sfr_register(porte,   0x09);
  add_sfr_register(&trise,  0x89, 0x07);

  add_sfr_register(&pclath, 0x8a, 0);
  add_sfr_register(&pclath, 0x0a, 0);

  add_sfr_register(&intcon_reg, 0x8b, 0);
  add_sfr_register(&intcon_reg, 0x0b, 0);

  add_sfr_register(&pir1,   0x0c, 0);
  add_sfr_register(&pie1,   0x8c, 0);

  add_sfr_register(&tmr1l,  0x0e, 0);
  add_sfr_register(&tmr1h,  0x0f, 0);

  //add_sfr_register(&pcon,   0x8e, 0);

  add_sfr_register(&t1con,  0x10, 0);
  add_sfr_register(&tmr2,   0x11, 0);
  add_sfr_register(&t2con,  0x12, 0);
  add_sfr_register(&pr2,    0x92, 0xff);

  //add_sfr_register(&sspbuf,  0x13, 0);
  //add_sfr_register(&sspcon,  0x14, 0);

  //add_sfr_register(&sspadd,  0x93, 0);
  //add_sfr_register(&sspstat, 0x94, 0);

  add_sfr_register(&ccpr1l,  0x15, 0);
  add_sfr_register(&ccpr1h,  0x16, 0);
  add_sfr_register(&ccp1con, 0x17, 0);

  sfr_map = NULL;
  num_of_sfrs = 0;

  intcon = &intcon_reg;
  intcon_reg.pir1 = &pir1;
  intcon_reg.pir2 = NULL;

  // Maybe there's a better place for this, but let's go ahead and link all
  // of the registers together (there's probably a better way too) :

  tmr1l.tmr1h = &tmr1h;
  tmr1l.t1con = &t1con;
  tmr1l.pir1  = &pir1;
  tmr1l.ccpcon = &ccp1con;

  tmr1h.tmr1l = &tmr1l;

  t1con.tmr1l = &tmr1l;

  t2con.tmr2  = &tmr2;
  tmr2.pir1   = &pir1;
  tmr2.pr2    = &pr2;
  tmr2.t2con  = &t2con;
  tmr2.ccp1con = &ccp1con;
  tmr2.ccp2con = &ccp2con;
  pr2.tmr2    = &tmr2;


  ccp1con.ccprl = &ccpr1l;
  ccp1con.pir   = &pir1;
  ccp1con.tmr2  = &tmr2;
  ccpr1l.ccprh  = &ccpr1h;
  ccpr1l.tmr1l  = &tmr1l;
  ccpr1h.ccprl  = &ccpr1l;

  portc->ccp1con = &ccp1con;

  ccpr1l.new_name("ccpr1l");
  ccpr1h.new_name("ccpr1h");
  ccp1con.new_name("ccp1con");

  pir1.intcon = &intcon_reg;
  pir1.pie    = &pie1;
  pie1.pir    = &pir1;
  pie1.new_name("pie1");


}

void P16C64::create_symbols(void)
{

  if(verbose)
    cout << "creating c64 symbols\n";

  symbol_table.add_ioport(porta->cpu, porta);
  symbol_table.add_ioport(portb->cpu, portb);
  symbol_table.add_ioport(portc->cpu, portc);
  symbol_table.add_ioport(portd->cpu, portd);
  symbol_table.add_ioport(porte->cpu, porte);

}


void  P16C64::create(void)
{

  if(verbose)
    cout << " c64 create \n";

  create_iopin_map();
  
  sfr_map = NULL;
  num_of_sfrs = 0;
 
  _14bit_processor::create();

  P16C64::create_sfr_map();

  // Build the links between the I/O Pins and the internal peripherals
  ccp1con.iopin = portc->pins[2];

  trace.program_counter (pc.value);


}

pic_processor * P16C64::construct(void)
{

  P16C64 *p = new P16C64;

  cout << " c64 construct\n";

  p->create();
  p->create_invalid_registers ();
  p->pic_processor::create_symbols();

  p->name_str = "16c64";

  return p;

}

P16C64::P16C64(void)
{
  if(verbose)
    cout << "c64 constructor, type = " << isa() << '\n';
}

//------------------------------------------------------------------------
//
//

void P16C65::create_sfr_map(void)
{

  if(verbose)
    cout << "creating c65 registers\n";

  add_file_registers(0xc0, 0xff, 0);

  add_sfr_register(&pir2,   0x0d, 0);
  add_sfr_register(&pie2,   0x8d, 0);

  add_sfr_register(&ccpr2l, 0x1b, 0);
  add_sfr_register(&ccpr2h, 0x1c, 0);
  add_sfr_register(&ccp2con, 0x1d, 0);

  intcon_reg.pir2 = &pir2;

  ccp2con.ccprl = &ccpr2l;
  ccp2con.pir   = &pir2;
  ccp2con.tmr2  = &tmr2;
  ccpr2l.ccprh  = &ccpr2h;
  ccpr2l.tmr1l  = &tmr1l;
  ccpr2h.ccprl  = &ccpr2l;

  add_sfr_register(&usart.rcsta, 0x18, 0,"rcsta");
  add_sfr_register(&usart.txsta, 0x98, 2,"txsta");
  add_sfr_register(&usart.spbrg, 0x99, 0,"spbrg");
  add_sfr_register(&usart.txreg, 0x19, 0,"txreg");
  add_sfr_register(&usart.rcreg, 0x1a, 0,"rcreg");
  usart.initialize(this,&pir1,portc);

  ccpr2l.new_name("ccpr2l");
  ccpr2h.new_name("ccpr2h");
  ccp2con.new_name("ccp2con");

  pir2.intcon = &intcon_reg;
  pir2.pie    = &pie2;
  pie2.pir    = &pir2;
  pie2.new_name("pie2");


  portc->usart = &usart;
}

void P16C65::create_symbols(void)
{

  if(verbose)
    cout << "creating c65 symbols\n";

  // There's nothing to create...

}


//------------------------------------------------------------------------
//
// P16C65 constructor
// 
// Note: Since the 'C65 is derived from the 'C64. So before this constructor
// is called, the C64 constructor will be called. Most of the initialization
// is done within the 'C64 constructor.

P16C65::P16C65(void)
{

  if(verbose)
    cout << "c65 constructor, type = " << isa() << '\n';

}

void P16C65::create(void)
{
  if(verbose)
    cout << " c65 create \n";

  P16C64::create();

  P16C65::create_sfr_map();

  // Build the links between the I/O Pins and the internal peripherals
  // ccp1con.iopin = portc.pins[2];
  ccp2con.iopin = portc->pins[1];

}

pic_processor * P16C65::construct(void)
{

  P16C65 *p = new P16C65;

  if(verbose)
    cout << " c65 construct\n";

  p->create();
  p->create_invalid_registers ();

  p->pic_processor::create_symbols();

  p->name_str = "16c65";

  return p;

}

