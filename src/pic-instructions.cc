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
#include <iostream.h>
#include <iomanip.h>
#include <string>

#include "../config.h"
#include "pic-processor.h"
#include "symbol.h"
#include "xref.h"


instruction::instruction(void)
{
#ifdef HAVE_GUI
    xref = new XrefObject;
#endif
    is_modified=0;
    cycle_count=0;
}

void instruction::add_line_number_symbol(int address)
{

  symbol_table.add_line_number(cpu, address);

}

void instruction::update_line_number(int file, int sline, int lline)
{

  file_id = file;
  src_line = sline;
  lst_line = lline;


}

char * Literal_op::name(char *return_str)
{

  sprintf(return_str,"%s\t0x%02x",name_str,L);

  return(return_str);
}

void Bit_op::decode(pic_processor *new_cpu, unsigned int new_opcode)
{
  opcode = new_opcode;

  cpu = new_cpu;
  switch(cpu->base_isa())
    {
    case _16BIT_PROCESSOR_:
      mask = 1 << ((opcode >> 9) & 7);
      register_address = opcode & REG_MASK_16BIT;
      access = opcode & ACCESS_MASK_16BIT;
      register_address = opcode & REG_MASK_16BIT;
      if((!access) && (opcode & 0x80))
	register_address |= 0xf00;

      break;

    case _14BIT_PROCESSOR_:
      mask = 1 << ((opcode >> 7) & 7);
      register_address = opcode & REG_MASK_14BIT;
      access = 1;
      break;

    case _12BIT_PROCESSOR_:
      mask = 1 << ((opcode >> 5) & 7);
      register_address = opcode & REG_MASK_12BIT;
      access = 1;
      break;
    default:
      cout << "ERROR: (Bit_op) the processor has a bad base type\n";
    }


}

char * Bit_op::name(char *return_str)
{
  //  %%% FIX ME %%% Actually just a slight dilemma - the source register will always be in
  //                 the lower bank of memory...

  reg = cpu->registers[register_address];

  unsigned int bit;

  switch(cpu->base_isa())
    {
    case _16BIT_PROCESSOR_:
      bit = ((opcode >> 9) & 7);
      sprintf(return_str,"%s\t%s,%d,%c",
	      name_str,
	      reg->name(), 
	      bit,
	      access ? '1' : '0');

      return(return_str);
      break;

    case _14BIT_PROCESSOR_:
      bit = ((opcode >> 7) & 7);
      break;

    case _12BIT_PROCESSOR_:
      bit = ((opcode >> 5) & 7);
      break;
    default:
      bit = 0;
    }


  sprintf(return_str,"%s\t%s,%d",name_str,reg->name(), bit);

  return(return_str);
}


//----------------------------------------------------------------
//
// Register_op::name
//

char * Register_op::name(char *return_str)
{
  //  %%% FIX ME %%% Actually just a slight dilemma - the source register will always be in
  //                 the lower bank of memory (for the 12 and 14 bit cores).

  source = cpu->registers[register_address];

  if(cpu->base_isa() != _16BIT_PROCESSOR_)
    sprintf(return_str,"%s\t%s,%c",name_str,source->name(), destination ? 'f' : 'w');
  else
    sprintf(return_str,"%s\t%s,%c,%c",
	    name_str,
	    source->name(), 
	    destination ? 'f' : 'w',
	    access ? '1' : '0');

  return(return_str);
}

//----------------------------------------------------------------
//
// Register_op::decode
//
// Base class to decode all 'register' type instructions. The main thing
// it does is obtains the register's address from the opcode. Note that this
// is processor dependent: in the 12-bit core processors, the register address
// is the lower 5 bits while in the 14-bit core it's the lower 7.

void  Register_op::decode(pic_processor *new_cpu, unsigned int new_opcode)
{ 
  opcode = new_opcode;
  cpu = new_cpu;

  switch(cpu->base_isa())
    {
    case _16BIT_PROCESSOR_:
      destination = opcode & DESTINATION_MASK_16BIT;
      access = opcode & ACCESS_MASK_16BIT;
      register_address = opcode & REG_MASK_16BIT;
      if((!access) && (opcode & 0x80))
	register_address |= 0xf00;

      break;

    case _14BIT_PROCESSOR_:
      register_address = opcode & REG_MASK_14BIT;
      destination = opcode & DESTINATION_MASK_14BIT;
      access = 1;
      break;

    case _12BIT_PROCESSOR_:
      register_address = opcode & REG_MASK_12BIT;
      destination = opcode & DESTINATION_MASK_12BIT;
      access = 1;
      break;

    default:
      cout << "ERROR: (Register_op) the processor has a bad base type\n";
    }

}



// Instantiate an invalid instruction
invalid_instruction bad_instruction;

file_register * Register_op::source = NULL;

//--------------------------------------------------

ADDWF::ADDWF (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);
  sprintf(name_str,"%s","addwf");

}

void ADDWF::execute(void)
{
  unsigned int new_value,src_value,w_value;

  trace.instruction(opcode);

  source = cpu->register_bank[register_address];

  new_value = (src_value = source->get()) + (w_value = cpu->W.value);

  // Store the result

  if(destination)
    source->put(new_value & 0xff);      // Result goes to source
  else
    cpu->W.put(new_value & 0xff);

  cpu->status.put_Z_C_DC(new_value, src_value, w_value);

  cpu->pc.increment();

}

//--------------------------------------------------

ANDLW::ANDLW (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);
  sprintf(name_str,"%s","andlw");

}

void ANDLW::execute(void)
{
  unsigned int new_value;

  trace.instruction(opcode);

  new_value = cpu->W.value & L;

  cpu->W.put(new_value);
  cpu->status.put_Z(0==new_value);

  cpu->pc.increment();

}

//--------------------------------------------------

ANDWF::ANDWF (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);
  sprintf(name_str,"%s","andwf");

}

void ANDWF::execute(void)
{
  unsigned int new_value;

  trace.instruction(opcode);

  source = cpu->register_bank[register_address];
  new_value = source->get() & cpu->W.value;

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu->W.put(new_value);

  cpu->status.put_Z(0==new_value);

  cpu->pc.increment();

}


//--------------------------------------------------

BCF::BCF (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  mask ^= 0xff;        // decode initializes the mask to 1<<bit

  sprintf(name_str,"%s","bcf");

}

void BCF::execute(void)
{

  trace.instruction(opcode);

  if(!access)
    reg = cpu->registers[register_address];
  else
    reg = cpu->register_bank[register_address];

  reg->put(reg->get() & mask);

  cpu->pc.increment();

}

//--------------------------------------------------

BSF::BSF (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"%s","bsf");

}

void BSF::execute(void)
{

  trace.instruction(opcode);

  if(!access)
    reg = cpu->registers[register_address];
  else
    reg = cpu->register_bank[register_address];

  reg->put(reg->get() | mask);


  cpu->pc.increment();

}

//--------------------------------------------------

BTFSC::BTFSC (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"%s","btfsc");

}

void BTFSC::execute(void)
{

  trace.instruction(opcode);

  if(!access)
    reg = cpu->registers[register_address];
  else
    reg = cpu->register_bank[register_address];

  if( 0 == (mask & reg->get()) )
    {
      cpu->pc.skip();                  // Skip next instruction
    }

  cpu->pc.increment();

}

//--------------------------------------------------

BTFSS::BTFSS (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"%s","btfss");

}

void BTFSS::execute(void)
{

  trace.instruction(opcode);

  if(!access)
    reg = cpu->registers[register_address];
  else
    reg = cpu->register_bank[register_address];


  if( 0 != (mask & reg->get()) )
    {
      cpu->pc.skip();                  // Skip next instruction
    }

  cpu->pc.increment();

}

//--------------------------------------------------
CALL::CALL (pic_processor *new_cpu, unsigned int new_opcode)
{
  opcode = new_opcode;
  cpu = new_cpu;

  switch(cpu->base_isa())
    {
    case _14BIT_PROCESSOR_:
      destination = opcode&0x7ff;
      break;

    case _12BIT_PROCESSOR_:
      destination = opcode&0xff;
      break;
    default:
      cout << "ERROR: (Bit_op) the processor has a bad base type\n";
    }

  sprintf(name_str,"%s","call");
}

void CALL::execute(void)
{
  trace.instruction(opcode);

  cpu->stack->push(cpu->pc.get_next());

  cpu->pc.jump(destination);

}

char * CALL::name(char *return_str)
{

  sprintf(return_str,"%s\t0x%04x",name_str,destination);

  return(return_str);
}


//--------------------------------------------------
CLRF::CLRF (pic_processor *new_cpu, unsigned int new_opcode)
{
  decode(new_cpu, new_opcode);

  sprintf(name_str,"%s","clrf");
}

void CLRF::execute(void)
{
  trace.instruction(opcode);

  if(!access)
    cpu->registers[register_address]->put(0);
  else
    cpu->register_bank[register_address]->put(0);

  cpu->status.put_Z(1);

  cpu->pc.increment();
}

char * CLRF::name(char *return_str)
{

  sprintf(return_str,"%s\t%s",name_str,cpu->registers[register_address]->name());

  return(return_str);
}


//--------------------------------------------------

CLRW::CLRW (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"%s","clrw");

}

void CLRW::execute(void)
{

  trace.instruction(opcode);
  cpu->W.put(0);

  cpu->status.put_Z(1);
  cpu->pc.increment();

}

//--------------------------------------------------

CLRWDT::CLRWDT (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"%s","clrwdt");

}

void CLRWDT::execute(void)
{

  trace.instruction(opcode);
  cpu->wdt.clear();

  if(cpu->base_isa() != _16BIT_PROCESSOR_)
    {
      cpu->status.put_TO(1);
      cpu->status.put_PD(1);
    }
  else
    cout << "FIXME: CLRWDT for 16 bit processors\n";

  cpu->pc.increment();

}

//--------------------------------------------------

COMF::COMF (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"%s","comf");

}

void COMF::execute(void)
{
  unsigned int new_value;

  trace.instruction(opcode);


  source = cpu->register_bank[register_address];
  new_value = source->get() ^ 0xff;

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu->W.put(new_value);

  cpu->status.put_Z(0==new_value);

  cpu->pc.increment();

}

//--------------------------------------------------

DECF::DECF (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"%s","decf");

}

void DECF::execute(void)
{
  unsigned int new_value;

  trace.instruction(opcode);


  source = cpu->register_bank[register_address];
  new_value = (source->get() - 1)&0xff;

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu->W.put(new_value);

  cpu->status.put_Z(0==new_value);

  cpu->pc.increment();

}

//--------------------------------------------------

DECFSZ::DECFSZ (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"%s","decfsz");

}

void DECFSZ::execute(void)
{
  unsigned int new_value;

  trace.instruction(opcode);


  source = cpu->register_bank[register_address];
  new_value = (source->get() - 1)&0xff;

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu->W.put(new_value);

  if(0==new_value)
    cpu->pc.skip();                  // Skip next instruction

  cpu->pc.increment();

}

//--------------------------------------------------
GOTO::GOTO (pic_processor *new_cpu, unsigned int new_opcode)
{
  opcode = new_opcode;
  cpu = new_cpu;

  switch(cpu->base_isa())
    {
    case _14BIT_PROCESSOR_:
      destination = opcode&0x7ff;
      break;

    case _12BIT_PROCESSOR_:
      destination = opcode&0x1ff;
      break;
    default:
      cout << "ERROR: (Bit_op) the processor has a bad base type\n";
    }

  sprintf(name_str,"%s","goto");
}

void GOTO::execute(void)
{
  trace.instruction(opcode);

  cpu->pc.jump(destination);

}

char * GOTO::name(char *return_str)
{

  sprintf(return_str,"%s\t0x%04x",name_str,destination);

  return(return_str);
}


//--------------------------------------------------

INCF::INCF (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"%s","incf");

}

void INCF::execute(void)
{
  unsigned int new_value;

  trace.instruction(opcode);

  source = cpu->register_bank[register_address];

  new_value = (source->get() + 1)&0xff;

  // Store the result

  if(destination)
    source->put(new_value);
  else
    cpu->W.put(new_value);


  cpu->status.put_Z(0==new_value);

  cpu->pc.increment();

}

//--------------------------------------------------

INCFSZ::INCFSZ (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"%s","incfsz");

}

void INCFSZ::execute(void)
{
  unsigned int new_value;

  trace.instruction(opcode);


  source = cpu->register_bank[register_address];

  new_value = (source->get() + 1)&0xff;

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu->W.put(new_value);

  if(0==new_value)
    cpu->pc.skip();                  // Skip next instruction

  cpu->pc.increment();

}

//--------------------------------------------------

IORLW::IORLW (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"%s","iorlw");

}

void IORLW::execute(void)
{
  unsigned int new_value;

  trace.instruction(opcode);

  new_value = cpu->W.value | L;

  cpu->W.put(new_value);
  cpu->status.put_Z(0==new_value);

  cpu->pc.increment();

}

//--------------------------------------------------

IORWF::IORWF (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"%s","iorwf");

}

void IORWF::execute(void)
{
  unsigned int new_value;

  trace.instruction(opcode);


  source = cpu->register_bank[register_address];
  new_value = source->get() | cpu->W.value;

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu->W.put(new_value);

  cpu->status.put_Z(0==new_value);

  cpu->pc.increment();

}

//--------------------------------------------------

MOVLW::MOVLW (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"%s","movlw");

}

void MOVLW::execute(void)
{
  unsigned int source_value;

  trace.instruction(opcode);

  cpu->W.put(L);

  cpu->pc.increment();

}

//--------------------------------------------------

MOVF::MOVF (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"%s","movf");

}

void MOVF::execute(void)
{
  unsigned int source_value;

  trace.instruction(opcode);

  source = cpu->register_bank[register_address];

  source_value = source->get();

  // Store the result

  if(destination)
    source->put(source_value);
  else
    cpu->W.put(source_value);


  cpu->status.put_Z(0==source_value);

  cpu->pc.increment();

}


void MOVF::debug(void)
{

   cout << "MOVF:  ";

}

//--------------------------------------------------
MOVWF::MOVWF (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"%s","movwf");
}

void MOVWF::execute(void)
{
  trace.instruction(opcode);

  cpu->register_bank[register_address]->put(cpu->W.get());

  cpu->pc.increment();
}

char * MOVWF::name(char *return_str)
{

  sprintf(return_str,"%s\t%s",name_str,cpu->register_bank[register_address]->name());

  return(return_str);
}


//--------------------------------------------------

NOP::NOP (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu,new_opcode);
  sprintf(name_str,"%s","nop");

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

  trace.instruction(opcode);

  cpu->pc.increment();

}

//--------------------------------------------------

OPTION::OPTION (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"%s","option");

}

void OPTION::execute(void)
{

  trace.instruction(opcode);

  cpu->option_reg.put(cpu->W.get());

  cpu->pc.increment();

}

//--------------------------------------------------

RETLW::RETLW (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"%s","retlw");

}

void RETLW::execute(void)
{

  trace.instruction(opcode);

  cpu->W.put(L);

  cpu->pc.new_address(cpu->stack->pop());

}

//--------------------------------------------------

RLF::RLF (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"%s","rlf");

}

void RLF::execute(void)
{
  unsigned int new_value;

  trace.instruction(opcode);


  source = cpu->register_bank[register_address];
  new_value = (source->get() << 1) | cpu->status.get_C();

  if(destination)
    source->put(new_value&0xff);      // Result goes to source
  else
    cpu->W.put(new_value&0xff);

  cpu->status.put_C(new_value>0xff);

  cpu->pc.increment();

}

//--------------------------------------------------

RRF::RRF (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"%s","rrf");

}

void RRF::execute(void)
{
  unsigned int new_value,old_value;

  trace.instruction(opcode);


  source = cpu->register_bank[register_address];
  old_value = source->get();
  new_value = (old_value >> 1) | (cpu->status.get_C() ? 0x80 : 0);

  if(destination)
    source->put(new_value&0xff);      // Result goes to source
  else
    cpu->W.put(new_value&0xff);

  cpu->status.put_C(old_value&0x01);

  cpu->pc.increment();

}

//--------------------------------------------------

SLEEP::SLEEP (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"%s","sleep");

}

void SLEEP::execute(void)
{

  trace.instruction(opcode);

  cpu->status.put_TO(1);
  cpu->status.put_PD(0);

  bp.set_sleep();

}

//--------------------------------------------------

SUBWF::SUBWF (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"%s","subwf");

}

void SUBWF::execute(void)
{
  unsigned int new_value,src_value,w_value;

  trace.instruction(opcode);


  source = cpu->register_bank[register_address];
  new_value = (src_value = source->get()) - (w_value = cpu->W.value);

  // Store the result

  if(destination)
    source->put(new_value & 0xff);      // Result goes to source
  else
    cpu->W.put(new_value & 0xff);

  cpu->status.put_Z_C_DC_for_sub(new_value, src_value, w_value);

  cpu->pc.increment();

}


//--------------------------------------------------

SWAPF::SWAPF (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"%s","swapf");

}

void SWAPF::execute(void)
{
  unsigned int src_value;

  trace.instruction(opcode);


  if(!access)
    source = cpu->registers[register_address];
  else
    source = cpu->register_bank[register_address];

  src_value = source->get();

  if(destination)
    source->put( ((src_value >> 4) & 0x0f) | ( (src_value << 4) & 0xf0) );
  else
    cpu->W.put( ((src_value >> 4) & 0x0f) | ( (src_value << 4) & 0xf0) );

  cpu->pc.increment();

}


//--------------------------------------------------
TRIS::TRIS (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  // The TRIS instruction only uses the lower three bits to determine
  // the destination register

  register_address &= 7;

  // Furthermore, those three bits can only be 5,6, or 7

  if( (register_address > 7) || (register_address < 5))
    {
      cout << "Warning: TRIS address '" << register_address << "' is  out of range\n";

	// set the address to a 'bad value' that's
	// easy to detect at run time:
	register_address = 0;
      
    }
  else {
     if(cpu->base_isa() == _14BIT_PROCESSOR_)
       register_address |= 0x80;  // The destination register is the TRIS
  }
  sprintf(name_str,"%s","tris");
}

void TRIS::execute(void)
{
  trace.instruction(opcode);

  if(register_address)
    {
      // Execute the instruction only if the register is valid.
      if(cpu->base_isa() == _14BIT_PROCESSOR_)
	cpu->registers[register_address]->put(cpu->W.get());
      else
	cpu->tris_instruction(register_address);
    }

  cpu->pc.increment();
}

char * TRIS::name(char *return_str)
{

  sprintf(return_str,"%s\t%s",name_str,cpu->registers[register_address]->name());

  return(return_str);
}


//--------------------------------------------------

XORLW::XORLW (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"%s","xorlw");

}

void XORLW::execute(void)
{
  unsigned int new_value;

  trace.instruction(opcode);

  new_value = cpu->W.value ^ L;

  cpu->W.put(new_value);
  cpu->status.put_Z(0==new_value);

  cpu->pc.increment();

}

//--------------------------------------------------

XORWF::XORWF (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"%s","xorwf");

}

void XORWF::execute(void)
{
  unsigned int new_value;

  trace.instruction(opcode);

  source = cpu->register_bank[register_address];
  new_value = source->get() ^ cpu->W.value;

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu->W.put(new_value);

  cpu->status.put_Z(0==new_value);

  cpu->pc.increment();

}
