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

#include "../config.h"
//#include "14bit-registers.h"
//#include "14bit-instructions.h"
#include "14bit-processors.h"

#include <string>
#include "stimuli.h"


//-------------------------------------------------------------------
//
pic_processor * _14bit_processor::construct(void)
{

  cout << " Can't create a generic 14bit processor\n";

  return 0;

}
//-------------------------------------------------------------------
_14bit_processor::_14bit_processor(void)
{
  pc = new Program_Counter();

}
//-------------------------------------------------------------------
//
// 
//    create
//
//  The purpose of this member function is to 'create' those things
// that are unique to the 14-bit core processors.

void _14bit_processor :: create (void)
{

  if(verbose)
    cout << "_14bit_processor create, type = " << isa() << '\n';

  pic_processor::create();
  fsr = new FSR;
  fsr->new_name("fsr");

  tmr0.set_cpu(this);
  tmr0.start(0);

}

//
// create_symbols
//
//  Create symbols for a generic 14-bit core. This allows symbolic
// access to the pic. (e.g. It makes it possible to access the 
// status register by name instead of by its address.)
//

void _14bit_processor::create_symbols (void)
{

  cout << "14bit create symbols\n";

}


//-------------------------------------------------------------------
void _14bit_processor::
interrupt (void)
{
  
  bp.clear_interrupt();

  stack->push(pc->value);
  intcon->clear_gie();

  trace.cycle_increment();

  pc->interrupt(INTERRUPT_VECTOR);

}

//-------------------------------------------------------------------
void _14bit_processor::por(void)
{
  pic_processor::por();
}

//-------------------------------------------------------------------
void _14bit_processor::option_new_bits_6_7(unsigned int bits)
{

  //portb.rbpu_intedg_update(bits);
  cout << "14bit, option bits 6 and/or 7 changed\n";

}

