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

#include<stdio.h>
#include <iostream>
#include <iomanip>

#include "../config.h"
#include "14bit-processors.h"
#include "14bit-instructions.h"


//--------------------------------------------------

ADDLW::ADDLW (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);
  new_name("addlw");

}

void ADDLW::execute(void)
{
  unsigned int old_value,new_value;

  trace.instruction(opcode);

  new_value = (old_value = cpu14->W->value.get()) + L;

  cpu14->W->put(new_value & 0xff);
  cpu14->status->put_Z_C_DC(new_value, old_value, L);

  cpu14->pc->increment();

}



//--------------------------------------------------

RETFIE::RETFIE (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("retfie");

}

void RETFIE::execute(void)
{

  trace.instruction(opcode);

  cpu14->pc->new_address(cpu14->stack->pop());

  cpu14->intcon->set_gie();
}

//--------------------------------------------------

RETURN::RETURN (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("return");

}

void RETURN::execute(void)
{

  trace.instruction(opcode);

  cpu14->pc->new_address(cpu14->stack->pop());

}

//--------------------------------------------------

SUBLW::SUBLW (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("sublw");

}

void SUBLW::execute(void)
{
  unsigned int old_value,new_value;

  trace.instruction(opcode);

  new_value = L - (old_value = cpu14->W->value.get());

  cpu14->W->put(new_value & 0xff);

  cpu14->status->put_Z_C_DC_for_sub(new_value, old_value, L);

  cpu14->pc->increment();

}

