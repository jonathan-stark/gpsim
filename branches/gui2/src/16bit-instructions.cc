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
#include <string>

#include "../config.h"
#include "pic-processor.h"
#include "14bit-registers.h"

//#include "symbol.h"
#include "pic-instructions.h"
#include "12bit-instructions.h"
#include "16bit-instructions.h"
#include "16bit-processors.h"
#include "16bit-registers.h"

//--------------------------------------------------
void Branching::decode(Processor *new_cpu, unsigned int new_opcode)
{
  opcode = new_opcode;

  cpu = new_cpu;

  switch(cpu16->isa()) {
    case  _P18Cxx2_:
    case  _P18C2x2_:
    case  _P18C242_:
    case  _P18C252_:
    case  _P18C442_:
    case  _P18C452_:
    case  _P18F242_:
    case  _P18F252_:
    case  _P18F442_:
    case  _P18F248_:
    case  _P18F452_:
    case  _P18F1220_:
    case  _P18F1320_:
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

char *Branching::name(char *return_str, int len)
{

  snprintf(return_str, len,"%s\t$%c0x%x\t;(0x%x)",
	   gpsimValue::name().c_str(),
	   (opcode & 0x80) ? '-' : '+', 
	   (destination & 0x7f)<<1,
	   absolute_destination<<1);


  return(return_str);
}

//--------------------------------------------------
void multi_word_branch::runtime_initialize(void)
{
  if(cpu16->program_memory[address+1] != &bad_instruction)
    {
      word2_opcode = cpu16->program_memory[address+1]->get_opcode();

      if((word2_opcode & 0xf000) != 0xf000) 
	{
	  cout << "16bit-instructions.cc multiword instruction error\n";
	  return;
	}

      cpu16->program_memory[address+1]->update_line_number( file_id,  src_line, lst_line, 0, 0);
      destination = ( (word2_opcode & 0xfff)<<8) | (opcode & 0xff);
      initialized = true;
    }
}

char * multi_word_branch::name(char *return_str,int len)
{
  if(!initialized)
    runtime_initialize();

  snprintf(return_str,len,"%s\t0x%05x",
	   gpsimValue::name().c_str(),
	   destination<<1);

  return(return_str);
}

//--------------------------------------------------
void ADDLW16::execute(void)
{
  unsigned int old_value,new_value;

  // trace.instruction(opcode);

  new_value = (old_value = cpu16->W->value.get()) + L;

  cpu16->W->put(new_value & 0xff);
  cpu16->status->put_Z_C_DC_OV_N(new_value, old_value, L);

  cpu16->pc->increment();

}

//--------------------------------------------------
void ADDWF16::execute(void)
{
  unsigned int new_value,src_value,w_value;

  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = (src_value = source->get()) + (w_value = cpu16->W->value.get());

  // Store the result

  if(destination)
    {
      source->put(new_value & 0xff);      // Result goes to source
      cpu16->status->put_Z_C_DC_OV_N(new_value, src_value, w_value);
    }
  else
    {
      cpu16->W->put(new_value & 0xff);
      cpu16->status->put_Z_C_DC_OV_N(new_value, w_value, src_value);
    }

  cpu16->pc->increment();

}

//--------------------------------------------------

ADDWFC::ADDWFC (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);
  new_name("addwfc");

}

void ADDWFC::execute(void)
{
  unsigned int new_value,src_value,w_value;

  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = (src_value = source->get()) + 
    (w_value = cpu16->W->value.get()) +
    ((cpu16->status->value.get() & STATUS_C) ? 1 : 0);

  // Store the result

  if(destination)
    source->put(new_value & 0xff);      // Result goes to source
  else
    cpu16->W->put(new_value & 0xff);

  cpu16->status->put_Z_C_DC_OV_N(new_value, src_value, w_value);

  cpu16->pc->increment();

}

//--------------------------------------------------

void ANDLW16::execute(void)
{
  unsigned int new_value;

  // trace.instruction(opcode);

  new_value = cpu16->W->value.get() & L;

  cpu16->W->put(new_value);
  cpu16->status->put_N_Z(new_value);

  cpu16->pc->increment();

}

//--------------------------------------------------

void ANDWF16::execute(void)
{
  unsigned int new_value;

  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = source->get() & cpu16->W->value.get();

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu16->W->put(new_value);

  cpu16->status->put_N_Z(new_value);

  cpu16->pc->increment();

}

//--------------------------------------------------

BC::BC (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("bc");

}

void BC::execute(void)
{
  // trace.instruction(opcode);

  if(cpu16->status->value.get() & STATUS_C)
    cpu16->pc->jump(absolute_destination);
  else
    cpu16->pc->increment();

}

//--------------------------------------------------

BN::BN (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("bn");

}

void BN::execute(void)
{
  // trace.instruction(opcode);

  if(cpu16->status->value.get() & STATUS_N)
    cpu16->pc->jump(absolute_destination);
  else
    cpu16->pc->increment();

}

//--------------------------------------------------

BNC::BNC (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("bnc");

}

void BNC::execute(void)
{
  // trace.instruction(opcode);

  if(cpu16->status->value.get() & STATUS_C)
    cpu16->pc->increment();
  else
    cpu16->pc->jump(absolute_destination);

}

//--------------------------------------------------

BNN::BNN (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("bnn");

}

void BNN::execute(void)
{
  // trace.instruction(opcode);

  if(cpu16->status->value.get() & STATUS_N)
    cpu16->pc->increment();
  else
    cpu16->pc->jump(absolute_destination);

}

//--------------------------------------------------

BNOV::BNOV (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("bnov");

}

void BNOV::execute(void)
{
  // trace.instruction(opcode);

  if(cpu16->status->value.get() & STATUS_OV)
    cpu16->pc->increment();
  else
    cpu16->pc->jump(absolute_destination);

}

//--------------------------------------------------

BNZ::BNZ (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("bnz");

}

void BNZ::execute(void)
{
  // trace.instruction(opcode);

  if(cpu16->status->value.get() & STATUS_Z)
    cpu16->pc->increment();
  else
    cpu16->pc->jump(absolute_destination);

}

//--------------------------------------------------

BOV::BOV (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("bov");

}

void BOV::execute(void)
{
  // trace.instruction(opcode);

  if(cpu16->status->value.get() & STATUS_OV)
    cpu16->pc->jump(absolute_destination);
  else
    cpu16->pc->increment();

}

//--------------------------------------------------
BRA::BRA (Processor *new_cpu, unsigned int new_opcode)
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

  new_name("bra");
}

void BRA::execute(void)
{
  // trace.instruction(opcode);

  cpu16->pc->jump(absolute_destination);

}

char * BRA::name(char *return_str,int len)
{


  sprintf(return_str,"%s\t$%c0x%x\t;(0x%05x)",
	  gpsimValue::name().c_str(),
	  (opcode & 0x400) ? '-' : '+', 
	  (destination & 0x7ff)<<1,
	  absolute_destination<<1);

  return(return_str);
}


//--------------------------------------------------

BTG::BTG (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("btg");

}

void BTG::execute(void)
{
  // trace.instruction(opcode);

  if(!access)
    reg = cpu->registers[register_address];
  else
    reg = cpu->register_bank[register_address];

  reg->put(reg->get() ^ mask);

  cpu16->pc->increment();

}
//--------------------------------------------------

BZ::BZ (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("bz");

}

void BZ::execute(void)
{
  // trace.instruction(opcode);

  if(cpu16->status->value.get() & STATUS_Z)
    cpu16->pc->jump(absolute_destination);
  else
    cpu16->pc->increment();

}

//--------------------------------------------------
CALL16::CALL16 (Processor *new_cpu, unsigned int new_opcode)
{
  opcode = new_opcode;
  fast = (new_opcode & 0x100) ? true : false;
  cpu = new_cpu;
  address = cpu16->current_disasm_address;
  initialized = false;

  new_name("call");

}

void CALL16::execute(void)
{
  // trace.instruction(opcode);

  if(!initialized)
    runtime_initialize();

  cpu16->stack->push(cpu16->pc->get_next());
  if(fast)
    cpu16->fast_stack.push();

  cpu16->pc->jump(destination);

}

char *CALL16::name(char  *return_str,int len)
{

  if(!initialized)
    runtime_initialize();

  snprintf(return_str,len,"call\t0x%05x%s",
	  destination<<1,
	  ((fast) ? ",f" : " "));

  return(return_str);
}

//--------------------------------------------------
void COMF16::execute(void)
{
  unsigned int new_value;

  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = source->get() ^ 0xff;

  // Store the result

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu16->W->put(new_value);

  cpu16->status->put_N_Z(new_value);

  cpu16->pc->increment();

}

//--------------------------------------------------

CPFSEQ::CPFSEQ (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("cpfseq");

}

void CPFSEQ::execute(void)
{
  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  if(source->get() == cpu16->W->value.get())
    cpu16->pc->skip();                  // Skip next instruction

  cpu16->pc->increment();

}

//--------------------------------------------------

CPFSGT::CPFSGT (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("cpfsgt");

}

void CPFSGT::execute(void)
{
  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  if(source->get() > cpu16->W->value.get())
    cpu16->pc->skip();                  // Skip next instruction

  cpu16->pc->increment();

}

//--------------------------------------------------

CPFSLT::CPFSLT (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("cpfslt");

}

void CPFSLT::execute(void)
{
  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  if(source->get() < cpu16->W->value.get())
    cpu16->pc->skip();                  // Skip next instruction

  cpu16->pc->increment();

}

//--------------------------------------------------

DAW::DAW (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("daw");

}

void DAW::execute(void)
{
  unsigned int new_value;

  // trace.instruction(opcode);

  new_value = cpu16->W->value.get();
  if(((new_value & 0x0f) > 0x9) || (cpu16->status->value.get() & STATUS_DC))
    new_value += 0x6;

  if(((new_value & 0xf0) > 0x90) || (cpu16->status->value.get() & STATUS_C))
    new_value += 0x60;

  cpu16->W->put(new_value & 0xff);
  cpu16->status->put_C(new_value>0xff);

  cpu16->pc->increment();

}

//--------------------------------------------------

void DECF16::execute(void)
{
  unsigned int new_value;

  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = (source->get() - 1)&0xff;

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu16->W->put(new_value);

  cpu16->status->put_N_Z(new_value);

  cpu16->pc->increment();

}

//--------------------------------------------------

void DECFSZ16::execute(void)
{
  unsigned int new_value;

  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = (source->get() - 1)&0xff;

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu16->W->put(new_value);

  if(0==new_value)
    cpu16->pc->skip();                  // Skip next instruction

  cpu16->pc->increment();

}

//--------------------------------------------------

DCFSNZ::DCFSNZ (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("dcfsnz");

}

void DCFSNZ::execute(void)
{
  unsigned int new_value;

  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = (source->get() - 1)&0xff;

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu16->W->put(new_value);

  if(0!=new_value)
    cpu16->pc->skip();                  // Skip next instruction

  cpu16->pc->increment();

}


//--------------------------------------------------
GOTO16::GOTO16 (Processor *new_cpu, unsigned int new_opcode)
{
  opcode = new_opcode;
  cpu = new_cpu;
  address = cpu16->current_disasm_address;
  initialized = false;

  new_name("goto");
}

void GOTO16::execute(void)
{
  // trace.instruction(opcode);

  if(!initialized)
    runtime_initialize();

  //  cpu16->stack.push(cpu16->pc->get_next());

  cpu16->pc->jump(destination);

}
//--------------------------------------------------

void INCF16::execute(void)
{
  unsigned int new_value, src_value;

  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  src_value = source->get();
  new_value = (src_value + 1);

  if(destination)
    {
      source->put(new_value & 0xff);      // Result goes to source
      cpu16->status->put_Z_C_DC_OV_N(new_value, src_value, 1);
    }
  else
    {
      cpu16->W->put(new_value & 0xff);
      cpu16->status->put_Z_C_DC_OV_N(new_value, 1, src_value);
    }

//   if(destination) {
//     source->put(new_value);      // Result goes to source
//   } else {
//     cpu->W->put(new_value);
//   }

//   cpu->status->put_N_Z(new_value);

  cpu16->pc->increment();

}

//--------------------------------------------------

void INCFSZ16::execute(void)
{
  unsigned int new_value;

  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = (source->get() + 1)&0xff;

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu16->W->put(new_value);

  if(0==new_value)
    cpu16->pc->skip();                  // Skip next instruction

  cpu16->pc->increment();

}

//--------------------------------------------------

INFSNZ::INFSNZ (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("infsnz");

}

void INFSNZ::execute(void)
{
  unsigned int new_value;

  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = (source->get() + 1)&0xff;

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu16->W->put(new_value);

  if(0!=new_value)
    cpu16->pc->skip();                  // Skip next instruction

  cpu16->pc->increment();

}

//--------------------------------------------------

void IORLW16::execute(void)
{
  unsigned int new_value;

  // trace.instruction(opcode);

  new_value = cpu16->W->value.get() | L;

  cpu16->W->put(new_value);
  cpu16->status->put_N_Z(new_value);

  cpu16->pc->increment();

}

//--------------------------------------------------

void IORWF16::execute(void)
{
  unsigned int new_value;

  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = source->get() | cpu16->W->value.get();

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu16->W->put(new_value);

  cpu16->status->put_N_Z(new_value);

  cpu16->pc->increment();

}

//--------------------------------------------------
LCALL16::LCALL16 (Processor *new_cpu, unsigned int new_opcode)
{
//    opcode = new_opcode;
//    fast = new_opcode & 0x100;
//    cpu = new_cpu;
//    address = cpu16->current_disasm_address;
//    initialized = 0;

  new_name("lcall");

}

void LCALL16::execute(void)
{
//    // trace.instruction(opcode);

//    if(!initialized)
//      runtime_initialize();

//    cpu16->stack->push(cpu16->pc->get_next());
//    if(fast)
//      cpu16->fast_stack.push();

//    cpu16->pc->jump(destination);

}

char *LCALL16::name(char  *return_str,int len)
{

//    if(!initialized)
//      runtime_initialize();

  snprintf(return_str,len,"lcall\t0x%05x%s",
	  destination,
	  ((fast) ? ",f" : " "));

  return(return_str);
}

//--------------------------------------------------

LFSR::LFSR (Processor *new_cpu, unsigned int new_opcode)
{
  
  opcode = new_opcode;
  cpu = new_cpu;
  address = cpu16->current_disasm_address;
  initialized = false;

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

  new_name("lfsr");
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
      initialized = true;
    }

}

char *LFSR::name(char *return_str,int len)
{

  if(!initialized)
    runtime_initialize();

  snprintf(return_str,len,"%s\t%d,0x%x",
	   gpsimValue::name().c_str(),
	   fsr,
	   k);


  return(return_str);
}


void LFSR::execute(void)
{
  // trace.instruction(opcode);

  if(!initialized)
    runtime_initialize();

  //cpu16->registers[fsr].put(k);
  //cout << "LFSR: not implemented\n";

  ia->put_fsr(k);

  cpu16->pc->skip();
  cpu16->pc->increment();

}
//--------------------------------------------------

void MOVF16::execute(void)
{
  unsigned int source_value;

  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  source_value = source->get();

  // Store the result

  if(destination)
    source->put(source_value);
  else
    cpu16->W->put(source_value);


  cpu16->status->put_N_Z(source_value);

  cpu16->pc->increment();

}

//--------------------------------------------------

MOVFF::MOVFF (Processor *new_cpu, unsigned int new_opcode)
{
  opcode = new_opcode;
  cpu = new_cpu;
  address = cpu16->current_disasm_address;
  initialized = false;
  destination = 0;
  source = opcode & 0xfff;

  new_name("movff");
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
      initialized = true;
    }

}

char *MOVFF::name(char *return_str,int len)
{

  if(!initialized)
    runtime_initialize();

  snprintf(return_str,len,"%s\t%s,%s",
	   gpsimValue::name().c_str(),
	   cpu->registers[source]->name().c_str(),
	   cpu->registers[destination]->name().c_str());


  return(return_str);
}


void MOVFF::execute(void)
{
  // trace.instruction(opcode);

  if(!initialized)
    runtime_initialize();

  unsigned int r =  cpu->registers[source]->get();
  cpu16->pc->skip();

  cpu->registers[destination]->put(r);

  cpu16->pc->increment();

}

//--------------------------------------------------

MOVFP::MOVFP (Processor *new_cpu, unsigned int new_opcode)
{
//    opcode = new_opcode;
//    cpu = new_cpu;
//    address = cpu16->current_disasm_address;
//    initialized = 0;
//    destination = 0;
//    source = opcode & 0xfff;

  new_name("movfp");
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

char *MOVFP::name(char *return_str, int len)
{

//    if(!initialized)
//      runtime_initialize();

  snprintf(return_str,len,"%s\t%s,%s",
	   gpsimValue::name().c_str(),
	   cpu->registers[source]->name().c_str(),
	   cpu->registers[destination]->name().c_str());


  return(return_str);
}


void MOVFP::execute(void)
{
//    // trace.instruction(opcode);

//    if(!initialized)
//      runtime_initialize();

//    unsigned int r =  cpu->registers[source]->get();
//    cpu->pc->skip();

//    cpu->registers[destination]->put(r);

//    cpu->pc->increment();

}

//--------------------------------------------------

MOVLB::MOVLB (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("movlb");

}

void MOVLB::execute(void)
{
  // trace.instruction(opcode);

  cpu16->bsr.put(L);

  cpu16->pc->increment();

}

//--------------------------------------------------

MOVLR::MOVLR (Processor *new_cpu, unsigned int new_opcode)
{

//    decode(new_cpu, new_opcode);

  new_name("movlr");

}

void MOVLR::execute(void)
{
//    unsigned int source_value;

//    // trace.instruction(opcode);

//    cpu16->bsr.put(L);

//    cpu->pc->increment();

}

//--------------------------------------------------

MOVPF::MOVPF (Processor *new_cpu, unsigned int new_opcode)
{
//    opcode = new_opcode;
//    cpu = new_cpu;
//    address = cpu16->current_disasm_address;
//    initialized = 0;
//    destination = 0;
//    source = opcode & 0xfff;

  new_name("movpf");
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

char *MOVPF::name(char *return_str,int len)
{

//    if(!initialized)
//      runtime_initialize();

  snprintf(return_str,len,"%s\t%s,%s",
	   gpsimValue::name().c_str(),
	   cpu->registers[source]->name().c_str(),
	   cpu->registers[destination]->name().c_str());


  return(return_str);
}


void MOVPF::execute(void)
{
//    // trace.instruction(opcode);

//    if(!initialized)
//      runtime_initialize();

//    unsigned int r =  cpu->registers[source]->get();
//    cpu->pc->skip();

//    cpu->registers[destination]->put(r);

//    cpu->pc->increment();

}


//--------------------------------------------------

MOVWF16::MOVWF16(Processor *new_cpu, unsigned int new_opcode) 
  : MOVWF(new_cpu,new_opcode)
{
  register_address = new_opcode & 0xff;
}

void MOVWF16::execute(void)
{
  // trace.instruction(opcode);

//   source = ((!access) ?
// 	    cpu->registers[register_address] 
// 	    :
// 	    cpu->register_bank[register_address] );

  source = cpu->register_bank[register_address];

  source->put(cpu16->W->get());

  cpu16->pc->increment();
}

//--------------------------------------------------
MOVWF16a::MOVWF16a(Processor *new_cpu, unsigned int new_opcode) : MOVWF(new_cpu,new_opcode)
{
  register_address = ((new_opcode & 0x80) ? 0xf80 : 0 ) | (new_opcode & 0x7f);
}

void MOVWF16a::execute(void)
{
  // trace.instruction(opcode);

//   source = ((!access) ?
// 	    cpu->registers[register_address] 
// 	    :
// 	    cpu->register_bank[register_address] );

  source = cpu->registers[register_address];
  source->put(cpu16->W->get());

  cpu16->pc->increment();
}

//--------------------------------------------------

MULLW::MULLW (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("mullw");

}

void MULLW::execute(void)
{
  unsigned int value;

  // trace.instruction(opcode);

  value = (0xff & cpu16->W->get()) * L;

  cpu16->prodl.put(value &0xff);
  cpu16->prodh.put((value>>8) &0xff);


  cpu16->pc->increment();

}

//--------------------------------------------------

MULWF::MULWF (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("mulwf");

}

void MULWF::execute(void)
{
  unsigned int value;

  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  //It's not necessary to '&' the get()'s with 0xff, but it doesn't
  //hurt either. 
  value = (0xff & cpu16->W->get()) * (0xff & source->get());

  cpu16->prodl.put(value &0xff);
  cpu16->prodh.put((value>>8) &0xff);

  cpu16->pc->increment();

}

//--------------------------------------------------

NEGF::NEGF (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("negf");

}

void NEGF::execute(void)
{
  unsigned int new_value,src_value;

  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  src_value = source->get();
  new_value = 1 + ~src_value;        // two's complement


  if(destination)
    source->put(new_value&0xff);      // Result goes to source
  else
    cpu16->W->put(new_value&0xff);

  cpu16->status->put_Z_C_DC_OV_N_for_sub(new_value,0,src_value);

  cpu16->pc->increment();

}


//--------------------------------------------------

NEGW::NEGW (Processor *new_cpu, unsigned int new_opcode)
{

//    decode(new_cpu, new_opcode);

  new_name("negw");

}

void NEGW::execute(void)
{
	cout << "negw is not implemented???";

}

//--------------------------------------------------

POP::POP (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("pop");

}

void POP::execute(void)
{

  // trace.instruction(opcode);

  cpu16->stack->pop();  // discard TOS

  cpu16->pc->increment();

}

//--------------------------------------------------

PUSH::PUSH (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("push");

}

void PUSH::execute(void)
{

  // trace.instruction(opcode);

  cpu16->stack->push(cpu16->pc->get_next());

  cpu16->pc->increment();

}

//--------------------------------------------------
RCALL::RCALL (Processor *new_cpu, unsigned int new_opcode)
{
  opcode = new_opcode;
  cpu = new_cpu;

  destination = (new_opcode & 0x7ff)+1;
  if(new_opcode & 0x400)
    destination -= 0x800;

  absolute_destination = (cpu16->current_disasm_address + destination) & 0xfffff;

  new_name("rcall");
}

void RCALL::execute(void)
{
  // trace.instruction(opcode);

  cpu16->stack->push(cpu16->pc->get_next());

  cpu16->pc->jump(absolute_destination);

}

char * RCALL::name(char *return_str,int len)
{


  snprintf(return_str,len,"%s\t$%c0x%x\t;(0x%05x)",
	   gpsimValue::name().c_str(),
	   (destination < 0) ? '-' : '+', 
	   (destination & 0x7ff)<<1,
	   absolute_destination<<1);

  return(return_str);
}


//--------------------------------------------------

RESET::RESET (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("reset");

}

void RESET::execute(void)
{

  // trace.instruction(opcode);

  cpu16->reset(SOFT_RESET);

}

//--------------------------------------------------
void RETFIE16::execute(void)
{

  // trace.instruction(opcode);

  cpu16->pc->new_address(cpu16->stack->pop());
  if(fast)
    cpu16->fast_stack.pop();
  //cout << "retfie: need to enable interrupts\n";

  cpu16->intcon.set_gies();  // re-enable the appropriate interrupt

}

char *RETFIE16::name(char  *return_str,int len)
{
  if(fast)
    snprintf(return_str,len,"retfie\tfast");
  else
    snprintf(return_str,len,"retfie");

  return(return_str);
}

//--------------------------------------------------
void RETURN16::execute(void)
{

  // trace.instruction(opcode);

  cpu16->pc->new_address(cpu16->stack->pop());
  if(fast)
    cpu16->fast_stack.pop();
}

char *RETURN16::name(char  *return_str,int len)
{
  if(fast)
    snprintf(return_str,len,"return\tfast");
  else
    snprintf(return_str,len,"return");

  return(return_str);
}

//--------------------------------------------------

RLCF::RLCF (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("rlcf");

}

void RLCF::execute(void)
{
  unsigned int new_value;

  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = (source->get() << 1) | cpu16->status->get_C();


  if(destination)
    source->put(new_value&0xff);      // Result goes to source
  else
    cpu16->W->put(new_value&0xff);

  cpu16->status->put_Z_C_N(new_value);

  cpu16->pc->increment();

}

//--------------------------------------------------

RLNCF::RLNCF (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("rlncf");

}

void RLNCF::execute(void)
{
  unsigned int new_value,src_value;

  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  src_value = source->get();
  new_value = (src_value << 1) | ( (src_value & 0x80) ? 1 : 0);


  if(destination)
    source->put(new_value&0xff);      // Result goes to source
  else
    cpu16->W->put(new_value&0xff);

  cpu16->status->put_N_Z(new_value);

  cpu16->pc->increment();

}


//--------------------------------------------------

RRCF::RRCF (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("rrcf");

}

void RRCF::execute(void)
{
  unsigned int new_value,src_value;

  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  src_value = source->get() & 0xff;
  new_value = (src_value >> 1) | (cpu16->status->get_C() ? 0x80 : 0);


  if(destination)
    source->put(new_value&0xff);      // Result goes to source
  else
    cpu16->W->put(new_value&0xff);

  cpu16->status->put_Z_C_N(new_value | ((src_value & 1) ? 0x100 : 0) );

  cpu16->pc->increment();

}

//--------------------------------------------------

RRNCF::RRNCF (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("rrncf");

}

void RRNCF::execute(void)
{
  unsigned int new_value,src_value;

  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  src_value = source->get() & 0xff;
  new_value = (src_value >> 1) | ( (src_value & 1) ? 0x80 : 0);


  if(destination)
    source->put(new_value&0xff);      // Result goes to source
  else
    cpu16->W->put(new_value&0xff);

  cpu16->status->put_N_Z(new_value | ((src_value & 1) ? 0x100 : 0) );

  cpu16->pc->increment();

}

//--------------------------------------------------

SETF::SETF (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("setf");

}

void SETF::execute(void)
{

  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );


  source->put(0xff);

  cpu16->pc->increment();

}

//--------------------------------------------------

void SLEEP16::execute(void)
{

  // trace.instruction(opcode);

  //cpu->status->put_TO(1);
  //cpu->status->put_PD(0);

  cout << "16BIT-SLEEP is not implemented\n";

  //  bp.set_sleep();

}

//--------------------------------------------------

void SUBLW16::execute(void)
{
  unsigned int new_value,old_value;

  // trace.instruction(opcode);

  new_value = L - (old_value = cpu16->W->value.get());

  cpu16->W->put(new_value & 0xff);

  cpu16->status->put_Z_C_DC_OV_N_for_sub(new_value, L, old_value);

  cpu16->pc->increment();

}


//--------------------------------------------------

SUBFWB::SUBFWB (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("subfwb");

}

void SUBFWB::execute(void)
{
  unsigned int new_value,src_value,w_value;

  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = (w_value = cpu16->W->value.get()) - (src_value = source->get()) -
    (1 - cpu16->status->get_C());

  if(destination)
    source->put(new_value & 0xff);
  else
    cpu16->W->put(new_value & 0xff);

  cpu16->status->put_Z_C_DC_OV_N_for_sub(new_value, w_value, src_value);

  cpu16->pc->increment();

}


//--------------------------------------------------

void SUBWF16::execute(void)
{
  unsigned int new_value,src_value,w_value;

  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = (src_value = source->get()) - (w_value = cpu16->W->value.get());

  if(destination)
    source->put(new_value & 0xff);
  else
    cpu16->W->put(new_value & 0xff);

  cpu16->status->put_Z_C_DC_OV_N_for_sub(new_value, src_value, w_value);

  cpu16->pc->increment();

}

//--------------------------------------------------

SUBWFB::SUBWFB (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("subwfb");

}

void SUBWFB::execute(void)
{
  unsigned int new_value,src_value,w_value;

  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = (src_value = source->get()) - (w_value = cpu16->W->value.get()) -
    (1 - cpu16->status->get_C());

  if(destination)
    source->put(new_value & 0xff);
  else
    cpu16->W->put(new_value & 0xff);

  cpu16->status->put_Z_C_DC_OV_N_for_sub(new_value, src_value, w_value);

  cpu16->pc->increment();

}


//--------------------------------------------------

TBLRD::TBLRD (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("tblrd");

}

char *TBLRD::name(char *return_str,int len)
{
  char *index_modes[4] = {"*","*+","*-","+*"};

  snprintf(return_str,len,"%s\t%s",
	   gpsimValue::name().c_str(),
	   index_modes[opcode&0x3]);


  return(return_str);
}

void TBLRD::execute(void)
{
  // trace.instruction(opcode);

  if((opcode & 3)==3)
    cpu16->tbl.increment();

  cpu16->tbl.read();

  if((opcode & 3)==1)
    cpu16->tbl.increment();
  else if((opcode & 3)==2)
    cpu16->tbl.decrement();

  cpu16->pc->increment();

}

//--------------------------------------------------

TBLWT::TBLWT (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("tblwt");

}

char *TBLWT::name(char *return_str,int len)
{
  char *index_modes[4] = {"*","*+","*-","+*"};

  snprintf(return_str,len,"%s\t%s",
	   gpsimValue::name().c_str(),
	   index_modes[opcode&0x3]);


  return(return_str);
}

void TBLWT::execute(void)
{
  // trace.instruction(opcode);

  if((opcode & 3)==3)
    cpu16->tbl.increment();

  cpu16->tbl.write();

  if((opcode & 3)==1)
    cpu16->tbl.increment();
  else if((opcode & 3)==2)
    cpu16->tbl.decrement();

  cpu16->pc->increment();

}


//--------------------------------------------------

TLRD::TLRD (Processor *new_cpu, unsigned int new_opcode)
{

//    decode(new_cpu, new_opcode);

  new_name("tlrd");

}

char *TLRD::name(char *return_str,int len)
{
  char *index_modes[4] = {"*","*+","*-","+*"};

  snprintf(return_str,len,"%s\t%s",
	  gpsimValue::name().c_str(),
	  index_modes[opcode&0x3]);


  return(return_str);
}

void TLRD::execute(void)
{
//    unsigned int pm_opcode;

//    // trace.instruction(opcode);

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

TLWT::TLWT (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("tlwt");

}

char *TLWT::name(char *return_str,int len)
{
  char *index_modes[4] = {"*","*+","*-","+*"};

  snprintf(return_str,len,"%s\t%s",
	   gpsimValue::name().c_str(),
	   index_modes[opcode&0x3]);


  return(return_str);
}

void TLWT::execute(void)
{
//    unsigned int pm_opcode;

//    // trace.instruction(opcode);

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

TSTFSZ::TSTFSZ (Processor *new_cpu, unsigned int new_opcode)
{

  decode(new_cpu, new_opcode);

  new_name("tstfsz");

}

void TSTFSZ::execute(void)
{

  // trace.instruction(opcode);

  if(!access)
    source = cpu->registers[register_address];
  else
    source = cpu->register_bank[register_address];

  if( 0 == (source->get() & 0xff) )
    {
      cpu16->pc->skip();                  // Skip next instruction
    }

  cpu16->pc->increment();

}
//--------------------------------------------------

void XORLW16::execute(void)
{
  unsigned int new_value;

  // trace.instruction(opcode);

  new_value = cpu16->W->value.get() ^ L;

  cpu16->W->put(new_value);
  cpu16->status->put_N_Z(new_value);

  cpu16->pc->increment();

}

//--------------------------------------------------

void XORWF16::execute(void)
{
  unsigned int new_value;

  // trace.instruction(opcode);

  source = ((!access) ?
	    cpu->registers[register_address] 
	    :
	    cpu->register_bank[register_address] );

  new_value = source->get() ^ cpu16->W->value.get();

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu16->W->put(new_value);

  cpu16->status->put_N_Z(new_value);

  cpu16->pc->increment();

}


