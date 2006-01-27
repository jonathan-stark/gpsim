/*
   Copyright (C) 2006 T. Scott Dattalo

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
along with gpsim; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include "../pic-instructions.h"

#include "dspic-processors.h"
#include "dspic-instructions.h"

using namespace dspic;
using namespace dspic_instructions;



//namespace dspic_instructions {};


struct instruction_constructor op_dsPic[] = {

  { 0xff0000,  0x000000,  NOP::construct }

};

const int NUM_OP_DSPIC = sizeof(op_dsPic) / sizeof(op_dsPic[0]);


instruction * dsPicProcessor::disasm (unsigned int address, unsigned int inst)
{

  instruction *pi;

  setCurrentDisasmAddress(address);

  pi = 0;
  for(int i =0; i<NUM_OP_DSPIC; i++)
    if((op_dsPic[i].inst_mask & inst) == op_dsPic[i].opcode)
      pi = op_dsPic[i].inst_constructor(this, inst);

  if(pi == 0)
    pi = invalid_instruction::construct(this, inst);

  return (pi);

}



namespace dspic_instructions
{
  //--------------------------------------------------

  NOP::NOP (Processor *new_cpu, unsigned int new_opcode)
  {

    decode(new_cpu,new_opcode);
    new_name("nop");

    // For the 18cxxx family, this 'nop' may in fact be the
    // 2nd word in a 2-word opcode. So just to be safe, let's
    // initialize the cross references to the source file.
    // (Subsequent initialization code will overwrite this,
    // but there is a chance that this info will be accessed
    // before that occurs).

    file_id = 0;
    src_line = 0;
    lst_line = 0;
    
  }

  void NOP::execute(void)
  {
    //cpu_pic->pc->increment();
  }


};
