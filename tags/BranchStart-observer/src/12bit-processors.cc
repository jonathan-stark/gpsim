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
#include "12bit-processors.h"

#include <string>
#include "stimuli.h"

extern unsigned int config_word;


void _12bit_processor::create_symbols(void)
{
  cout << "12bit create symbols\n";
}

void _12bit_processor::por(void)
{
  pic_processor::por();
}

void _12bit_processor::create(void)
{

  if(verbose)
    cout << "_12bit_processor create, type = " << isa() << '\n';

  pa_bits = 0;                 // Assume only one code page (page select bits in status)

  fsr.register_page_bits = 0;  // Assume only one register page (e.g. 12c508)
  pic_processor::create();

  stack->stack_mask = 1;        // The 12bit core only has 2 stack positions

  tmr0.cpu = this;
  tmr0.start();

}

//-------------------------------------------------------------------
void _12bit_processor::option_new_bits_6_7(unsigned int bits)
{

  //portb.rbpu_intedg_update(bits);

  cout << "12bit, option bits 6 and/or 7 changed\n";

}

//-------------------------------------------------------------------
void _12bit_processor::dump_registers (void)
{


  pic_processor::dump_registers();

  cout << "option = " << option_reg.value << '\n';

}

