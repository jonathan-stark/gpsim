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
#include "14bit-registers.h"

#include "symbol.h"
#include "pic-instructions.h"
#include "12bit-instructions.h"
#include "16bit-instructions.h"
#include "16bit-processors.h"
#include "16bit-registers.h"

//--------------------------------------------------
void Branching::decode(pic_processor *new_cpu, unsigned int new_opcode)
{
  opcode = new_opcode;

  cpu = new_cpu;

  switch(cpu->isa()) {
    case  _P18Cxx2_:
    case  _P18C2x2_:
    case  _P18C242_:
    case  _P18C252_:
    case  _P18C442_:
    case  _P18C452_:
    case  _P18F442_:
    case  _P18F452_:
      destination = (new_opcode & 0xff)+1;
      absolute_destination = (cpu16->current_disasm_address + destination) & 0xfffff;
 
      if(new_opcode & 0x80)
        {
          absolute_destination -= 0x100;
          destination = 0x100 - destination;
        }
      break;

    case _P17C7xx_:
    case _P17C75x_:
    case _P17C756_:
    case _P17C756A_:
    case _P17C762_:
    case _P17C766_:
      cout << "Which instructions go here?\n";
      break;

    default:
      cout << "ERROR: (Branching) the processor is not defined\n";
      break;
  }
}

char *Branching::name(char *return_str)
{

  sprintf(return_str,"%s\t$%c0x%x\t;(0x%x)",
	  name_str,
	  (opcode & 0x80) ? '-' : '+', 
	  destination & 0x7f,
	  absolute_destination);


  return(return_str);
}

//--------------------------------------------------
void multi_word_branch::runtime_initialize(void)
{
  if(cpu->program_memory[address+1] != &bad_instruction)
    {
      word2_opcode = cpu->program_memory[address+1]->get_opcode();

      if((word2_opcode & 0xf000) != 0xf000) 
	{
	  cout << "16bit-instructions.cc multiword instruction error\n";
	  return;
	}

      cpu->program_memory[address+1]->update_line_number( file_id,  src_line, lst_line, 0, 0);
      destination = ( (word2_opcode & 0xfff)<<8) | (opcode & 0xff);
      initialized = 1;
    }
}

char * multi_word_branch::name(char *return_str)
{
  if(!initialized)
    runtime_initialize();

  sprintf(return_str,"%s\t0x%05x",name_str,destination);

  return(return_str);
}

//--------------------------------------------------
void ADDLW16::execute(void)
{
  unsigned int old_value,new_value;

  trace.instruction(opcode);

  new_value = (old_value = cpu->W->value) + L;

  cpu->W->put(new_value & 0xff);
  cpu->status->put_Z_C_DC_OV_N(new_value, old_value, L);

  cpu->pc->increment();

}

//--------------------------------------------------
void ADDWF16::execute(void)
{
  unsigned int new_value,src_value,w_value;

  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = (src_value = source->get()) + (w_value = cpu->W->value);

  // Store the result

  if(destination)
    {
      source->put(new_value & 0xff);      // Result goes to source
      cpu->status->put_Z_C_DC_OV_N(new_value, src_value, w_value);
    }
  else
    {
      cpu->W->put(new_value & 0xff);
      cpu->status->put_Z_C_DC_OV_N(new_value, w_value, src_value);
    }

  cpu->pc->increment();

}

//--------------------------------------------------

ADDWFC::ADDWFC (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);
  sprintf(name_str,"addwfc");

}

void ADDWFC::execute(void)
{
  unsigned int new_value,src_value,w_value;

  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = (src_value = source->get()) + 
    (w_value = cpu->W->value) +
    ((cpu->status->value & STATUS_C) ? 1 : 0);

  // Store the result

  if(destination)
    source->put(new_value & 0xff);      // Result goes to source
  else
    cpu->W->put(new_value & 0xff);

  cpu->status->put_Z_C_DC_OV_N(new_value, src_value, w_value);

  cpu->pc->increment();

}

//--------------------------------------------------

void ANDLW16::execute(void)
{
  unsigned int new_value;

  trace.instruction(opcode);

  new_value = cpu->W->value & L;

  cpu->W->put(new_value);
  cpu->status->put_N_Z(new_value);

  cpu->pc->increment();

}

//--------------------------------------------------

void ANDWF16::execute(void)
{
  unsigned int new_value;

  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = source->get() & cpu->W->value;

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu->W->put(new_value);

  cpu->status->put_N_Z(new_value);

  cpu->pc->increment();

}

//--------------------------------------------------

BC::BC (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"bc");

}

void BC::execute(void)
{
  trace.instruction(opcode);

  if(cpu->status->value & STATUS_C)
    cpu->pc->jump(absolute_destination);
  else
    cpu->pc->increment();

}

//--------------------------------------------------

BN::BN (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"bn");

}

void BN::execute(void)
{
  trace.instruction(opcode);

  if(cpu->status->value & STATUS_N)
    cpu->pc->jump(absolute_destination);
  else
    cpu->pc->increment();

}

//--------------------------------------------------

BNC::BNC (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"bnc");

}

void BNC::execute(void)
{
  trace.instruction(opcode);

  if(cpu->status->value & STATUS_C)
    cpu->pc->increment();
  else
    cpu->pc->jump(absolute_destination);

}

//--------------------------------------------------

BNN::BNN (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"bnn");

}

void BNN::execute(void)
{
  trace.instruction(opcode);

  if(cpu->status->value & STATUS_N)
    cpu->pc->increment();
  else
    cpu->pc->jump(absolute_destination);

}

//--------------------------------------------------

BNOV::BNOV (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"bnov");

}

void BNOV::execute(void)
{
  trace.instruction(opcode);

  if(cpu->status->value & STATUS_OV)
    cpu->pc->increment();
  else
    cpu->pc->jump(absolute_destination);

}

//--------------------------------------------------

BNZ::BNZ (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"bnz");

}

void BNZ::execute(void)
{
  trace.instruction(opcode);

  if(cpu->status->value & STATUS_Z)
    cpu->pc->increment();
  else
    cpu->pc->jump(absolute_destination);

}

//--------------------------------------------------

BOV::BOV (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"bov");

}

void BOV::execute(void)
{
  trace.instruction(opcode);

  if(cpu->status->value & STATUS_OV)
    cpu->pc->jump(absolute_destination);
  else
    cpu->pc->increment();

}

//--------------------------------------------------
BRA::BRA (pic_processor *new_cpu, unsigned int new_opcode)
{
  opcode = new_opcode;
  cpu = new_cpu;

  destination = (new_opcode & 0x7ff)+1;
  absolute_destination = (cpu16->current_disasm_address + destination) & 0xfffff;

  if(new_opcode & 0x400)
    {
      absolute_destination -= 0x800;
      destination = 0x800 - destination;
    }

  sprintf(name_str,"bra");
}

void BRA::execute(void)
{
  trace.instruction(opcode);

  cpu->pc->jump(absolute_destination);

}

char * BRA::name(char *return_str)
{


  sprintf(return_str,"%s\t$%c0x%x\t;(0x%05x)",
	  name_str,
	  (opcode & 0x400) ? '-' : '+', 
	  destination & 0x7ff,
	  absolute_destination);

  return(return_str);
}


//--------------------------------------------------

BTG::BTG (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"btg");

}

void BTG::execute(void)
{
  trace.instruction(opcode);

  if(!access)
    reg = cpu->registers[register_address];
  else
    reg = cpu->register_bank[register_address];

  reg->put(reg->get() ^ mask);

  cpu->pc->increment();

}
//--------------------------------------------------

BZ::BZ (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"bz");

}

void BZ::execute(void)
{
  trace.instruction(opcode);

  if(cpu->status->value & STATUS_Z)
    cpu->pc->jump(absolute_destination);
  else
    cpu->pc->increment();

}

//--------------------------------------------------
CALL16::CALL16 (pic_processor *new_cpu, unsigned int new_opcode)
{
  opcode = new_opcode;
  fast = new_opcode & 0x100;
  cpu = new_cpu;
  address = cpu16->current_disasm_address;
  initialized = 0;

  sprintf(name_str,"call");

}

void CALL16::execute(void)
{
  trace.instruction(opcode);

  if(!initialized)
    runtime_initialize();

  cpu->stack->push(cpu->pc->get_next());
  if(fast)
    cpu16->fast_stack.push();

  cpu->pc->jump(destination);

}

char *CALL16::name(char  *return_str)
{

  if(!initialized)
    runtime_initialize();

  sprintf(return_str,"call\t0x%05x%s",
	  destination,
	  ((fast) ? ",f" : " "));

  return(return_str);
}

//--------------------------------------------------
void COMF16::execute(void)
{
  unsigned int new_value,src_value,w_value;

  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = source->get() ^ 0xff;

  // Store the result

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu->W->put(new_value);

  cpu->status->put_N_Z(new_value);

  cpu->pc->increment();

}

//--------------------------------------------------

CPFSEQ::CPFSEQ (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"cpfseq");

}

void CPFSEQ::execute(void)
{
  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  if(source->get() == cpu->W->value)
    cpu->pc->skip();                  // Skip next instruction

  cpu->pc->increment();

}

//--------------------------------------------------

CPFSGT::CPFSGT (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"cpfsgt");

}

void CPFSGT::execute(void)
{
  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  if(source->get() > cpu->W->value)
    cpu->pc->skip();                  // Skip next instruction

  cpu->pc->increment();

}

//--------------------------------------------------

CPFSLT::CPFSLT (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"cpfslt");

}

void CPFSLT::execute(void)
{
  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  if(source->get() < cpu->W->value)
    cpu->pc->skip();                  // Skip next instruction

  cpu->pc->increment();

}

//--------------------------------------------------

DAW::DAW (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"daw");

}

void DAW::execute(void)
{
  unsigned int new_value;

  trace.instruction(opcode);

  new_value = cpu->W->value;
  if((new_value > 0x9) || (cpu->status->value & STATUS_DC))
    new_value += 0x6;

  if((new_value > 0x90) || (cpu->status->value & STATUS_C))
    new_value += 0x60;

  cpu->W->put(new_value & 0xff);
  cpu->status->put_C(new_value>0xff);

  cpu->pc->increment();

}

//--------------------------------------------------

void DECF16::execute(void)
{
  unsigned int new_value;

  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = (source->get() - 1)&0xff;

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu->W->put(new_value);

  cpu->status->put_N_Z(new_value);

  cpu->pc->increment();

}

//--------------------------------------------------

void DECFSZ16::execute(void)
{
  unsigned int new_value;

  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = (source->get() - 1)&0xff;

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu->W->put(new_value);

  if(0==new_value)
    cpu->pc->skip();                  // Skip next instruction

  cpu->pc->increment();

}

//--------------------------------------------------

DCFSNZ::DCFSNZ (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"dcfsnz");

}

void DCFSNZ::execute(void)
{
  unsigned int new_value;

  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = (source->get() - 1)&0xff;

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu->W->put(new_value);

  if(0!=new_value)
    cpu->pc->skip();                  // Skip next instruction

  cpu->pc->increment();

}


//--------------------------------------------------
GOTO16::GOTO16 (pic_processor *new_cpu, unsigned int new_opcode)
{
  opcode = new_opcode;
  cpu = new_cpu;
  address = cpu16->current_disasm_address;
  initialized = 0;

  sprintf(name_str,"goto");
}

void GOTO16::execute(void)
{
  trace.instruction(opcode);

  if(!initialized)
    runtime_initialize();

  //  cpu->stack.push(cpu->pc->get_next());

  cpu->pc->jump(destination);

}
//--------------------------------------------------

void INCF16::execute(void)
{
  unsigned int new_value, src_value;

  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  src_value = source->get();
  new_value = (src_value + 1);

  if(destination)
    {
      source->put(new_value & 0xff);      // Result goes to source
      cpu->status->put_Z_C_DC_OV_N(new_value, src_value, 1);
    }
  else
    {
      cpu->W->put(new_value & 0xff);
      cpu->status->put_Z_C_DC_OV_N(new_value, 1, src_value);
    }

//   if(destination) {
//     source->put(new_value);      // Result goes to source
//   } else {
//     cpu->W->put(new_value);
//   }

//   cpu->status->put_N_Z(new_value);

  cpu->pc->increment();

}

//--------------------------------------------------

void INCFSZ16::execute(void)
{
  unsigned int new_value;

  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = (source->get() + 1)&0xff;

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu->W->put(new_value);

  if(0==new_value)
    cpu->pc->skip();                  // Skip next instruction

  cpu->pc->increment();

}

//--------------------------------------------------

INFSNZ::INFSNZ (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"infsnz");

}

void INFSNZ::execute(void)
{
  unsigned int new_value;

  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = (source->get() + 1)&0xff;

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu->W->put(new_value);

  if(0!=new_value)
    cpu->pc->skip();                  // Skip next instruction

  cpu->pc->increment();

}

//--------------------------------------------------

void IORLW16::execute(void)
{
  unsigned int new_value;

  trace.instruction(opcode);

  new_value = cpu->W->value | L;

  cpu->W->put(new_value);
  cpu->status->put_N_Z(new_value);

  cpu->pc->increment();

}

//--------------------------------------------------

void IORWF16::execute(void)
{
  unsigned int new_value;

  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = source->get() | cpu->W->value;

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu->W->put(new_value);

  cpu->status->put_N_Z(new_value);

  cpu->pc->increment();

}

//--------------------------------------------------
LCALL16::LCALL16 (pic_processor *new_cpu, unsigned int new_opcode)
{
//    opcode = new_opcode;
//    fast = new_opcode & 0x100;
//    cpu = new_cpu;
//    address = cpu16->current_disasm_address;
//    initialized = 0;

  sprintf(name_str,"lcall");

}

void LCALL16::execute(void)
{
//    trace.instruction(opcode);

//    if(!initialized)
//      runtime_initialize();

//    cpu->stack->push(cpu->pc->get_next());
//    if(fast)
//      cpu16->fast_stack.push();

//    cpu->pc->jump(destination);

}

char *LCALL16::name(char  *return_str)
{

//    if(!initialized)
//      runtime_initialize();

  sprintf(return_str,"lcall\t0x%05x%s",
	  destination,
	  ((fast) ? ",f" : " "));

  return(return_str);
}

//--------------------------------------------------

LFSR::LFSR (pic_processor *new_cpu, unsigned int new_opcode)
{
  
  opcode = new_opcode;
  cpu = new_cpu;
  address = cpu16->current_disasm_address;
  initialized = 0;

  fsr = (opcode & 0x30)>>4;
  switch(fsr)
    {
    case 0:
      ia = &cpu16->ind0;
      break;

    case 1:
      ia = &cpu16->ind1;
      break;

    case 2:
      ia = &cpu16->ind2;
      break;

    case 3:
      cout << "LFSR decode error, fsr is 3 and should only be 0,1, or 2\n";
      ia = &cpu16->ind0;
    }

  sprintf(name_str,"lfsr");
}

void LFSR::runtime_initialize(void)
{
  if(cpu->program_memory[address+1])
    {
      word2_opcode = cpu->program_memory[address+1]->get_opcode();

      if((word2_opcode & 0xff00) != 0xf000) 
	{
	  cout << "16bit-instructions.cc LFSR error\n";
	  return;
	}

      cpu->program_memory[address+1]->update_line_number( file_id,  src_line, lst_line, 0, 0);
      k = ( (opcode & 0xf)<<8) | (word2_opcode & 0xff);
      initialized = 1;
    }

}

char *LFSR::name(char *return_str)
{

  if(!initialized)
    runtime_initialize();

  sprintf(return_str,"%s\t%d,0x%x",
	  name_str,
	  fsr,
	  k);


  return(return_str);
}


void LFSR::execute(void)
{
  trace.instruction(opcode);

  if(!initialized)
    runtime_initialize();

  //cpu16->registers[fsr].put(k);
  //cout << "LFSR: not implemented\n";

  ia->put_fsr(k);

  cpu->pc->skip();
  cpu->pc->increment();

}
//--------------------------------------------------

void MOVF16::execute(void)
{
  unsigned int source_value;

  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  source_value = source->get();

  // Store the result

  if(destination)
    source->put(source_value);
  else
    cpu->W->put(source_value);


  cpu->status->put_N_Z(source_value);

  cpu->pc->increment();

}

//--------------------------------------------------

MOVFF::MOVFF (pic_processor *new_cpu, unsigned int new_opcode)
{
  opcode = new_opcode;
  cpu = new_cpu;
  address = cpu16->current_disasm_address;
  initialized = 0;
  destination = 0;
  source = opcode & 0xfff;

  sprintf(name_str,"movff");
}

void MOVFF::runtime_initialize(void)
{
  if(cpu->program_memory[address+1])
    {
      word2_opcode = cpu->program_memory[address+1]->get_opcode();

      if((word2_opcode & 0xf000) != 0xf000) 
	{
	  cout << "16bit-instructions.cc MOVFF error\n";
	  return;
	}

      cpu->program_memory[address+1]->update_line_number( file_id,  src_line, lst_line, 0, 0);
      destination = word2_opcode & 0xfff;
      initialized = 1;
    }

}

char *MOVFF::name(char *return_str)
{

  if(!initialized)
    runtime_initialize();

  sprintf(return_str,"%s\t%s,%s",
	  name_str,
	  cpu->registers[source]->name(),
	  cpu->registers[destination]->name());


  return(return_str);
}


void MOVFF::execute(void)
{
  trace.instruction(opcode);

  if(!initialized)
    runtime_initialize();

  unsigned int r =  cpu->registers[source]->get();
  cpu->pc->skip();

  cpu->registers[destination]->put(r);

  cpu->pc->increment();

}

//--------------------------------------------------

MOVFP::MOVFP (pic_processor *new_cpu, unsigned int new_opcode)
{
//    opcode = new_opcode;
//    cpu = new_cpu;
//    address = cpu16->current_disasm_address;
//    initialized = 0;
//    destination = 0;
//    source = opcode & 0xfff;

  sprintf(name_str,"movfp");
}

void MOVFP::runtime_initialize(void)
{
//    if(cpu->program_memory[address+1])
//      {
//        word2_opcode = cpu->program_memory[address+1]->get_opcode();

//        if((word2_opcode & 0xf000) != 0xf000) 
//  	{
//  	  cout << "16bit-instructions.cc MOVFP error\n";
//  	  return;
//  	}

//        cpu->program_memory[address+1]->update_line_number( file_id,  src_line, lst_line);
//        destination = word2_opcode & 0xfff;
//        initialized = 1;
//      }

}

char *MOVFP::name(char *return_str)
{

//    if(!initialized)
//      runtime_initialize();

  sprintf(return_str,"%s\t%s,%s",
	  name_str,
	  cpu->registers[source]->name(),
	  cpu->registers[destination]->name());


  return(return_str);
}


void MOVFP::execute(void)
{
//    trace.instruction(opcode);

//    if(!initialized)
//      runtime_initialize();

//    unsigned int r =  cpu->registers[source]->get();
//    cpu->pc->skip();

//    cpu->registers[destination]->put(r);

//    cpu->pc->increment();

}

//--------------------------------------------------

MOVLB::MOVLB (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"movlb");

}

void MOVLB::execute(void)
{
  unsigned int source_value;

  trace.instruction(opcode);

  cpu16->bsr.put(L);

  cpu->pc->increment();

}

//--------------------------------------------------

MOVLR::MOVLR (pic_processor *new_cpu, unsigned int new_opcode)
{

//    decode(new_cpu, new_opcode);

  sprintf(name_str,"movlb");

}

void MOVLR::execute(void)
{
//    unsigned int source_value;

//    trace.instruction(opcode);

//    cpu16->bsr.put(L);

//    cpu->pc->increment();

}

//--------------------------------------------------

MOVPF::MOVPF (pic_processor *new_cpu, unsigned int new_opcode)
{
//    opcode = new_opcode;
//    cpu = new_cpu;
//    address = cpu16->current_disasm_address;
//    initialized = 0;
//    destination = 0;
//    source = opcode & 0xfff;

  sprintf(name_str,"movpf");
}

void MOVPF::runtime_initialize(void)
{
//    if(cpu->program_memory[address+1])
//      {
//        word2_opcode = cpu->program_memory[address+1]->get_opcode();

//        if((word2_opcode & 0xf000) != 0xf000) 
//  	{
//  	  cout << "16bit-instructions.cc MOVFP error\n";
//  	  return;
//  	}

//        cpu->program_memory[address+1]->update_line_number( file_id,  src_line, lst_line);
//        destination = word2_opcode & 0xfff;
//        initialized = 1;
//      }

}

char *MOVPF::name(char *return_str)
{

//    if(!initialized)
//      runtime_initialize();

  sprintf(return_str,"%s\t%s,%s",
	  name_str,
	  cpu->registers[source]->name(),
	  cpu->registers[destination]->name());


  return(return_str);
}


void MOVPF::execute(void)
{
//    trace.instruction(opcode);

//    if(!initialized)
//      runtime_initialize();

//    unsigned int r =  cpu->registers[source]->get();
//    cpu->pc->skip();

//    cpu->registers[destination]->put(r);

//    cpu->pc->increment();

}


//--------------------------------------------------

MOVWF16::MOVWF16(pic_processor *new_cpu, unsigned int new_opcode) 
  : MOVWF(new_cpu,new_opcode)
{
  register_address = new_opcode & 0xff;
}

void MOVWF16::execute(void)
{
  trace.instruction(opcode);

//   source = ((!access) ?
// 	    cpu->registers[register_address] 
// 	    :
// 	    cpu->register_bank[register_address] );

  source = cpu->register_bank[register_address];

  source->put(cpu->W->get());

  cpu->pc->increment();
}

//--------------------------------------------------
MOVWF16a::MOVWF16a(pic_processor *new_cpu, unsigned int new_opcode) : MOVWF(new_cpu,new_opcode)
{
  register_address = ((new_opcode & 0x80) ? 0xf80 : 0 ) | (new_opcode & 0x7f);
}

void MOVWF16a::execute(void)
{
  trace.instruction(opcode);

//   source = ((!access) ?
// 	    cpu->registers[register_address] 
// 	    :
// 	    cpu->register_bank[register_address] );

  source = cpu->registers[register_address];
  source->put(cpu->W->get());

  cpu->pc->increment();
}

//--------------------------------------------------

MULLW::MULLW (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"mullw");

}

void MULLW::execute(void)
{
  unsigned int value;

  trace.instruction(opcode);

  value = (0xff & cpu->W->get()) * L;

  cpu16->prodl.put(value &0xff);
  cpu16->prodh.put((value>>8) &0xff);


  cpu->pc->increment();

}

//--------------------------------------------------

MULWF::MULWF (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"mulwf");

}

void MULWF::execute(void)
{
  unsigned int value;

  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  //It's not necessary to '&' the get()'s with 0xff, but it doesn't
  //hurt either. 
  value = (0xff & cpu->W->get()) * (0xff & source->get());

  cpu16->prodl.put(value &0xff);
  cpu16->prodh.put((value>>8) &0xff);

  cpu->pc->increment();

}

//--------------------------------------------------

NEGF::NEGF (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"negf");

}

void NEGF::execute(void)
{
  unsigned int new_value,src_value;

  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = -(src_value = source->get());


  if(destination)
    source->put(new_value&0xff);      // Result goes to source
  else
    cpu->W->put(new_value&0xff);

  cpu->status->put_Z_C_DC_OV_N_for_sub(new_value,0,src_value);

  cpu->pc->increment();

}


//--------------------------------------------------

NEGW::NEGW (pic_processor *new_cpu, unsigned int new_opcode)
{

//    decode(new_cpu, new_opcode);

  sprintf(name_str,"negw");

}

void NEGW::execute(void)
{
  unsigned int new_value,src_value;

//    trace.instruction(opcode);

//    source = ((!access) ?
//  	    cpu->registers[register_address] 
//  	    :
//  	    cpu->register_bank[register_address] );

//    new_value = -(src_value = source->get());


//    if(destination)
//      source->put(new_value&0xff);      // Result goes to source
//    else
//      cpu->W->put(new_value&0xff);

//    cpu->status->put_Z_C_DC_OV_N_for_sub(new_value,0,src_value);

//    cpu->pc->increment();

}

//--------------------------------------------------

POP::POP (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"pop");

}

void POP::execute(void)
{

  trace.instruction(opcode);

  cpu->stack->pop();  // discard TOS

  cpu->pc->increment();

}

//--------------------------------------------------

PUSH::PUSH (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"push");

}

void PUSH::execute(void)
{

  trace.instruction(opcode);

  cpu->stack->push(cpu->pc->get_next());

  cpu->pc->increment();

}

//--------------------------------------------------
RCALL::RCALL (pic_processor *new_cpu, unsigned int new_opcode)
{
  opcode = new_opcode;
  cpu = new_cpu;

  destination = (new_opcode & 0x7ff)+1;
  if(new_opcode & 0x400)
    destination -= 0x800;

  absolute_destination = (cpu16->current_disasm_address + destination) & 0xfffff;

  sprintf(name_str,"rcall");
}

void RCALL::execute(void)
{
  trace.instruction(opcode);

  cpu->stack->push(cpu->pc->get_next());

  cpu->pc->jump(absolute_destination);

}

char * RCALL::name(char *return_str)
{


  sprintf(return_str,"%s\t$%c0x%x\t;(0x%05x)",
	  name_str,
	  (destination < 0) ? '-' : '+', 
	  destination & 0x7ff,
	  absolute_destination);

  return(return_str);
}


//--------------------------------------------------

RESET::RESET (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"reset");

}

void RESET::execute(void)
{

  trace.instruction(opcode);

  cpu->reset(SOFT_RESET);

}

//--------------------------------------------------
void RETFIE16::execute(void)
{

  trace.instruction(opcode);

  cpu->pc->new_address(cpu->stack->pop());
  if(fast)
    cpu16->fast_stack.pop();
  //cout << "retfie: need to enable interrupts\n";

  cpu16->intcon.set_gies();  // re-enable the appropriate interrupt

}

char *RETFIE16::name(char  *return_str)
{
  if(fast)
    sprintf(return_str,"retfie\tfast");
  else
    sprintf(return_str,"retfie");

  return(return_str);
}

//--------------------------------------------------
void RETURN16::execute(void)
{

  trace.instruction(opcode);

  cpu->pc->new_address(cpu->stack->pop());
  if(fast)
    cpu16->fast_stack.pop();
}

char *RETURN16::name(char  *return_str)
{
  if(fast)
    sprintf(return_str,"return\tfast");
  else
    sprintf(return_str,"return");

  return(return_str);
}

//--------------------------------------------------

RLCF::RLCF (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"rlcf");

}

void RLCF::execute(void)
{
  unsigned int new_value;

  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = (source->get() << 1) | cpu->status->get_C();


  if(destination)
    source->put(new_value&0xff);      // Result goes to source
  else
    cpu->W->put(new_value&0xff);

  cpu->status->put_Z_C_N(new_value);

  cpu->pc->increment();

}

//--------------------------------------------------

RLNCF::RLNCF (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"rlncf");

}

void RLNCF::execute(void)
{
  unsigned int new_value,src_value;

  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  src_value = source->get();
  new_value = (src_value << 1) | ( (src_value & 0x80) ? 1 : 0);


  if(destination)
    source->put(new_value&0xff);      // Result goes to source
  else
    cpu->W->put(new_value&0xff);

  cpu->status->put_N_Z(new_value);

  cpu->pc->increment();

}


//--------------------------------------------------

RRCF::RRCF (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"rrcf");

}

void RRCF::execute(void)
{
  unsigned int new_value,src_value;

  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  src_value = source->get() & 0xff;
  new_value = (src_value >> 1) | (cpu->status->get_C() ? 0x80 : 0);


  if(destination)
    source->put(new_value&0xff);      // Result goes to source
  else
    cpu->W->put(new_value&0xff);

  cpu->status->put_Z_C_N(new_value | ((src_value & 1) ? 0x100 : 0) );

  cpu->pc->increment();

}

//--------------------------------------------------

RRNCF::RRNCF (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"rrncf");

}

void RRNCF::execute(void)
{
  unsigned int new_value,src_value;

  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  src_value = source->get() & 0xff;
  new_value = (src_value >> 1) | ( (src_value & 1) ? 0x80 : 0);


  if(destination)
    source->put(new_value&0xff);      // Result goes to source
  else
    cpu->W->put(new_value&0xff);

  cpu->status->put_Z_C_N(new_value | ((src_value & 1) ? 0x100 : 0) );

  cpu->pc->increment();

}

//--------------------------------------------------

SETF::SETF (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"setf");

}

void SETF::execute(void)
{

  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );


  source->put(0xff);

  cpu->pc->increment();

}

//--------------------------------------------------

void SLEEP16::execute(void)
{

  trace.instruction(opcode);

  //cpu->status->put_TO(1);
  //cpu->status->put_PD(0);

  cout << "16BIT-SLEEP is not implemented\n";

  //  bp.set_sleep();

}

//--------------------------------------------------

void SUBLW16::execute(void)
{
  unsigned int new_value,old_value;

  trace.instruction(opcode);

  new_value = L - (old_value = cpu->W->value);

  cpu->W->put(new_value & 0xff);

  cpu->status->put_Z_C_DC_OV_N_for_sub(new_value, old_value, L);

  cpu->pc->increment();

}


//--------------------------------------------------

SUBFWB::SUBFWB (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"subfwb");

}

void SUBFWB::execute(void)
{
  unsigned int new_value,src_value,w_value;

  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = (w_value = cpu->W->value) - (src_value = source->get()) -
    (1 - cpu->status->get_C());

  if(destination)
    source->put(new_value & 0xff);
  else
    cpu->W->put(new_value & 0xff);

  cpu->status->put_Z_C_DC_OV_N_for_sub(new_value, w_value, src_value);

  cpu->pc->increment();

}


//--------------------------------------------------

void SUBWF16::execute(void)
{
  unsigned int new_value,src_value,w_value;

  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = (src_value = source->get()) - (w_value = cpu->W->value);

  if(destination)
    source->put(new_value & 0xff);
  else
    cpu->W->put(new_value & 0xff);

  cpu->status->put_Z_C_DC_OV_N_for_sub(new_value, src_value, w_value);

  cpu->pc->increment();

}

//--------------------------------------------------

SUBWFB::SUBWFB (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"subwfb");

}

void SUBWFB::execute(void)
{
  unsigned int new_value,src_value,w_value;

  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = (src_value = source->get()) - (w_value = cpu->W->value) -
    (1 - cpu->status->get_C());

  if(destination)
    source->put(new_value & 0xff);
  else
    cpu->W->put(new_value & 0xff);

  cpu->status->put_Z_C_DC_OV_N_for_sub(new_value, src_value, w_value);

  cpu->pc->increment();

}


//--------------------------------------------------

TBLRD::TBLRD (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"tblrd");

}

char *TBLRD::name(char *return_str)
{
  char *index_modes[4] = {"*","*+","*-","+*"};

  sprintf(return_str,"%s\t%s",
	  name_str,
	  index_modes[opcode&0x3]);


  return(return_str);
}

void TBLRD::execute(void)
{
  unsigned int pm_opcode;

  trace.instruction(opcode);

  if((opcode & 3)==3)
    cpu16->tbl.increment();

  cpu16->tbl.read();

  if((opcode & 3)==1)
    cpu16->tbl.increment();
  else if((opcode & 3)==2)
    cpu16->tbl.decrement();

  cpu->pc->increment();

}

//--------------------------------------------------

TBLWT::TBLWT (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"tblwt");

}

char *TBLWT::name(char *return_str)
{
  char *index_modes[4] = {"*","*+","*-","+*"};

  sprintf(return_str,"%s\t%s",
	  name_str,
	  index_modes[opcode&0x3]);


  return(return_str);
}

void TBLWT::execute(void)
{
  unsigned int pm_opcode;

  trace.instruction(opcode);

  if((opcode & 3)==3)
    cpu16->tbl.increment();

  cpu16->tbl.write();

  if((opcode & 3)==1)
    cpu16->tbl.increment();
  else if((opcode & 3)==2)
    cpu16->tbl.decrement();

  cpu->pc->increment();

}


//--------------------------------------------------

TLRD::TLRD (pic_processor *new_cpu, unsigned int new_opcode)
{

//    decode(new_cpu, new_opcode);

  sprintf(name_str,"tlrd");

}

char *TLRD::name(char *return_str)
{
  char *index_modes[4] = {"*","*+","*-","+*"};

  sprintf(return_str,"%s\t%s",
	  name_str,
	  index_modes[opcode&0x3]);


  return(return_str);
}

void TLRD::execute(void)
{
//    unsigned int pm_opcode;

//    trace.instruction(opcode);

//    if((opcode & 3)==3)
//      cpu16->tbl.increment();

//    cpu16->tbl.read();

//    if((opcode & 3)==1)
//      cpu16->tbl.increment();
//    else if((opcode & 3)==2)
//      cpu16->tbl.decrement();

//    cpu->pc->increment();

}

//--------------------------------------------------

TLWT::TLWT (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"tlwt");

}

char *TLWT::name(char *return_str)
{
  char *index_modes[4] = {"*","*+","*-","+*"};

  sprintf(return_str,"%s\t%s",
	  name_str,
	  index_modes[opcode&0x3]);


  return(return_str);
}

void TLWT::execute(void)
{
//    unsigned int pm_opcode;

//    trace.instruction(opcode);

//    if((opcode & 3)==3)
//      cpu16->tbl.increment();

//    cpu16->tbl.write();

//    if((opcode & 3)==1)
//      cpu16->tbl.increment();
//    else if((opcode & 3)==2)
//      cpu16->tbl.decrement();

//    cpu->pc->increment();

}

//--------------------------------------------------

TSTFSZ::TSTFSZ (pic_processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  sprintf(name_str,"tstfsz");

}

void TSTFSZ::execute(void)
{

  trace.instruction(opcode);

  if(!access)
    source = cpu->registers[register_address];
  else
    source = cpu->register_bank[register_address];

  if( 0 == (source->get() & 0xff) )
    {
      cpu->pc->skip();                  // Skip next instruction
    }

  cpu->pc->increment();

}
//--------------------------------------------------

void XORLW16::execute(void)
{
  unsigned int new_value;

  trace.instruction(opcode);

  new_value = cpu->W->value ^ L;

  cpu->W->put(new_value);
  cpu->status->put_N_Z(new_value);

  cpu->pc->increment();

}

//--------------------------------------------------

void XORWF16::execute(void)
{
  unsigned int new_value;

  trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = source->get() ^ cpu->W->value;

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu->W->put(new_value);

  cpu->status->put_N_Z(new_value);

  cpu->pc->increment();

}


