/*
   Copyright (C) 1998 T. Scott Dattalo

This file is part of the libgpsim library of gpsim

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see 
<http://www.gnu.org/licenses/lgpl-2.1.html>.
*/

#include<stdio.h>
#include <iostream>
#include <iomanip>
#include <string>

#include "../config.h"
#include "pic-processor.h"
#include "14bit-instructions.h"
#include "registers.h"
#include "symbol.h"
#include "xref.h"

//------------------------------------------------------------------------
AddressSymbol::AddressSymbol(Processor *pCpu, const char *_name, unsigned int _val)
  :  Integer(_val)

{
  new_name(_name);
  cpu = pCpu;
}

string AddressSymbol::toString()
{
  char buf[256];
  int i = (int)getVal();
  snprintf(buf,sizeof(buf), " at address %d = 0x%X",i,i);

  return string(buf);
}

Value* AddressSymbol::evaluate()
{
  return copy();
}
int AddressSymbol::set_break(ObjectBreakTypes bt, ObjectActionTypes at,
                             Expression *pExpr)
{
  if (bt == gpsimObject::eBreakExecute && cpu)
    return get_bp().set_execution_break((Processor*)cpu,getVal(),pExpr);

  return -1;
}

LineNumberSymbol::LineNumberSymbol(Processor *pCpu,
                                   const char *_name,
                                   unsigned int _val)
  :  AddressSymbol(pCpu,_name,_val)
{
  if(!_name) {
    char buf[64];
    snprintf(buf,sizeof(buf), "line_%04x",_val);
    new_name(buf);
  }

}




instruction::instruction(Processor *pProcessor,
                         unsigned int uOpCode,
                         unsigned int uAddrOfInstr)
  : Value("","",pProcessor),
    m_bIsModified(false),
    cycle_count(0),
    opcode(uOpCode),
    m_uAddrOfInstr(uAddrOfInstr),
    pLineSymbol(0),
    file_id(-1),
    hll_file_id(-1),
    src_line(-1),
    lst_line(-1),
    hll_src_line(-1)
{
  if (cpu) {
    pLineSymbol = new LineNumberSymbol((Processor*)cpu,0, m_uAddrOfInstr);
    if (!cpu->addSymbol(pLineSymbol)) {
      delete pLineSymbol;
      pLineSymbol = 0;
    }
  }
}
instruction::~instruction()
{
  // cout << __FUNCTION__<<endl;
  if (cpu)
    cpu->deleteSymbol(pLineSymbol);
}

void instruction::decode(Processor *new_cpu, unsigned int new_opcode)
{
  cpu = new_cpu;
  opcode = new_opcode;
}

int instruction::set_break(ObjectBreakTypes bt, ObjectActionTypes at, Expression *pExpr)
{
  if (bt == gpsimObject::eBreakExecute)
    return get_bp().set_execution_break(get_cpu(),m_uAddrOfInstr,pExpr);

  return -1;
}

void instruction::addLabel(string &rLabel)
{
  if (cpu) {
    addName(rLabel);
    cpu->addSymbol(this, &rLabel);
  }
}

//------------------------------------------------------------------------
invalid_instruction::invalid_instruction(Processor *new_cpu,unsigned int new_opcode, unsigned int address)
  : instruction(new_cpu,new_opcode,address)
{
  new_name("INVALID");
}

void invalid_instruction::execute()
{
  //cout << "*** INVALID INSTRUCTION ***\n";
#ifdef __DEBUG_VERBOSE__
  debug();
#endif

  /* Don't know what to do, so just plow through like nothing happened */
  if(cpu_pic)
    cpu_pic->pc->increment();

};

void invalid_instruction::addLabel(string &rLabel)
{
  cout << "*** WARNING: adding label '"<<rLabel << "' to an invalid instruction\n";
}


//------------------------------------------------------------------------
// update_line_number(int file, int sline, int lline, int hllfile, int hllsline)
//
// map the instruction to the source file that generated it.
// If a line number is less than 0, then this means that the
// existing mapping should remain unchanged. This allows this
// method to be called multiple times; once for each type of
// mapping.

void instruction::update_line_number(int file, int sline, int lline, int hllfile, int hllsline)
{
  file_id      = file<0     ? file_id      : file;
  src_line     = sline<0    ? src_line     : sline;
  lst_line     = lline<0    ? lst_line     : lline;
  hll_src_line = hllsline<0 ? hll_src_line : hllsline;
  hll_file_id  = hllfile<0  ? hll_file_id  : hllfile;
}
char *instruction::ReadSrcLine(char *buf, int nBytes)
{
  if (buf && nBytes && cpu)
    return ((Processor *)cpu)->files.ReadLine(file_id,src_line,buf,nBytes);

  return buf;
}
char *instruction::ReadHLLLine(char *buf, int nBytes)
{
  if (buf && nBytes && cpu)
    return ((Processor *)cpu)->files.ReadLine(hll_file_id,hll_src_line,buf,nBytes);

  return buf;
}

char *instruction::ReadLstLine(char *buf, int nBytes)
{
  if (buf && nBytes && cpu)
    return ((Processor *)cpu)->files.ReadLine(((Processor *)cpu)->files.list_id(),lst_line,buf,nBytes);

  return buf;
}

//------------------------------------------------------------------------
AliasedInstruction::AliasedInstruction(instruction *_replaced)
  : instruction(0,0,0), m_replaced(_replaced)
{
}

AliasedInstruction::AliasedInstruction()
  : instruction(0,0,0), m_replaced(0)
{
}

AliasedInstruction::AliasedInstruction(Processor *pProcessor,
                     unsigned int uOpCode,
                     unsigned int uAddrOfInstr)
  : instruction(pProcessor, uOpCode, uAddrOfInstr),
    m_replaced(0)
{
}

AliasedInstruction::~AliasedInstruction()
{
}

void AliasedInstruction::setReplaced(instruction *_replaced)
{
  m_replaced = _replaced;
}

instruction * AliasedInstruction::getReplaced()
{
  return m_replaced ? m_replaced : &dynamic_cast<Processor *>(cpu)->bad_instruction;
}

void AliasedInstruction::execute()
{
  getReplaced()->execute();
}
void AliasedInstruction::debug()
{
  getReplaced()->debug();
}
int AliasedInstruction::instruction_size()
{
  return getReplaced()->instruction_size();
}
unsigned int AliasedInstruction::get_opcode()
{
  return getReplaced()->get_opcode();
}
unsigned int AliasedInstruction::get_value()
{
  return getReplaced()->get_value();
}
void AliasedInstruction::put_value(unsigned int new_value)
{
  getReplaced()->put_value(new_value);
}
int AliasedInstruction::get_src_line()
{
  return getReplaced()->get_src_line();
}
int AliasedInstruction::get_hll_src_line()
{
  return getReplaced()->get_hll_src_line();
}
void AliasedInstruction::update_line_number(int file, int sline, int lline, int hllfile, int hllsline)
{
  if (m_replaced)
    m_replaced->update_line_number(file, sline, lline, hllfile, hllsline);
}

int AliasedInstruction::get_lst_line()
{
  return getReplaced()->get_lst_line();
}
int AliasedInstruction::get_file_id()
{
  return getReplaced()->get_file_id();
}
int AliasedInstruction::get_hll_file_id()
{
  return getReplaced()->get_hll_file_id();
}
enum instruction::INSTRUCTION_TYPES AliasedInstruction::isa()
{
  return getReplaced()->isa();
}
void AliasedInstruction::initialize(bool init_state)
{
  getReplaced()->initialize(init_state);
}
char *AliasedInstruction::name(char *cPtr,int len)
{
  return getReplaced()->name(cPtr,len);
}
void AliasedInstruction::update()
{
  getReplaced()->update();
}
void AliasedInstruction::add_xref(void *an_xref)
{
  getReplaced()->add_xref(an_xref);
}
void AliasedInstruction::remove_xref(void *an_xref)
{
  getReplaced()->remove_xref(an_xref);
}
bool AliasedInstruction::isBase()
{
  return getReplaced()->isBase();
}


//------------------------------------------------------------------------
Literal_op::Literal_op(Processor *pProcessor,
                       unsigned int uOpCode,
                       unsigned int uAddrOfInstr)
  : instruction(pProcessor, uOpCode, uAddrOfInstr),
    L(uOpCode&0xff)
{
}
char *Literal_op::name(char *return_str,int len)
{

  snprintf(return_str,len,"%s\t0x%02x",
           gpsimObject::name().c_str(),L);

  return(return_str);
}

void Literal_op::decode(Processor *new_cpu, unsigned int new_opcode)
{
  opcode = new_opcode;
  cpu = new_cpu;
  L = opcode & 0xff;
}

Bit_op::Bit_op(Processor *pProcessor,
                       unsigned int uOpCode,
                       unsigned int uAddrOfInstr)
  : instruction(pProcessor, uOpCode, uAddrOfInstr),
    mask(0),register_address(0),
    access(false),
    reg(0)
{
}

void Bit_op::decode(Processor *new_cpu, unsigned int new_opcode)
{
  opcode = new_opcode;

  cpu = new_cpu;
  switch(cpu_pic->base_isa())
    {
    case _PIC17_PROCESSOR_:
      mask = 1 << ((opcode >> 8) & 7);
      register_address = opcode & REG_MASK_16BIT;
      access = 0;
      break;

    case _PIC18_PROCESSOR_:
      mask = 1 << ((opcode >> 9) & 7);
      register_address = opcode & REG_MASK_16BIT;
      access = (opcode & ACCESS_MASK_16BIT) ? true : false;
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

char * Bit_op::name(char *return_str,int len)
{
  //  %%% FIX ME %%% Actually just a slight dilemma - the source register will always be in
  //                 the lower bank of memory...

  reg = get_cpu()->registers[register_address];

  unsigned int bit;

  switch(cpu_pic->base_isa())
    {
    case _PIC17_PROCESSOR_:
        cout << "Bit_op::name %%% FIX ME %%% treating 17x as 18x\n";
    case _PIC18_PROCESSOR_:
      bit = ((opcode >> 9) & 7);
      snprintf(return_str,len,"%s\t%s,%d,%c",
               gpsimObject::name().c_str(),
               reg->name().c_str(),
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


  snprintf(return_str,len,"%s\t%s,%d",
           gpsimObject::name().c_str(),
           reg->name().c_str(),
           bit);

  return(return_str);
}


//----------------------------------------------------------------
Register_op::Register_op(Processor *pProcessor,
                       unsigned int uOpCode,
                       unsigned int uAddrOfInstr)
  : instruction(pProcessor, uOpCode, uAddrOfInstr),
    register_address(0),
    destination(false), access(false)
{
}

//----------------------------------------------------------------
//
// Register_op::name
//

char * Register_op::name(char *return_str,int len)
{
  //  %%% FIX ME %%% Actually just a slight dilemma - the source register will always be in
  //                 the lower bank of memory (for the 12 and 14 bit cores).

  source = get_cpu()->registers[register_address];

  if ( cpu_pic->base_isa() != _PIC18_PROCESSOR_ )
    snprintf(return_str,len,"%s\t%s,%c",
             gpsimObject::name().c_str(),
             source->name().c_str(),
             destination ? 'f' : 'w');
  else
    snprintf(return_str,len,"%s\t%s,%c,%c",
             gpsimObject::name().c_str(),
             source->name().c_str(),
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

void  Register_op::decode(Processor *new_cpu, unsigned int new_opcode)
{
  opcode = new_opcode;
  cpu = new_cpu;

  switch(cpu_pic->base_isa())
    {
    case _PIC17_PROCESSOR_:
        cout << "Register_op::decode %%% FIXME %%% - PIC17 core is not the same as PIC18\n";
    case _PIC18_PROCESSOR_:
      destination = (opcode & DESTINATION_MASK_16BIT) ? true : false;
      access = (opcode & ACCESS_MASK_16BIT) ? true : false;
      register_address = opcode & REG_MASK_16BIT;
      if((!access) && (opcode & 0x80))
        register_address |= 0xf00;

      break;

    case _14BIT_PROCESSOR_:
      register_address = opcode & REG_MASK_14BIT;
      destination = (opcode & DESTINATION_MASK_14BIT) ? true : false;
      access = 1;
      break;

    case _12BIT_PROCESSOR_:
      register_address = opcode & REG_MASK_12BIT;
      destination = (opcode & DESTINATION_MASK_12BIT) ? true : false;
      access = 1;
      break;

    default:
      cout << "ERROR: (Register_op) the processor has a bad base type\n";
    }

}



Register * Register_op::source = 0;

//--------------------------------------------------

ADDWF::ADDWF (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Register_op(new_cpu, new_opcode, address)
{

  decode(new_cpu, new_opcode);
  new_name("addwf");

}

void ADDWF::execute()
{
  unsigned int new_value,src_value,w_value;

  if(!access)
    source = cpu_pic->registers[register_address];
  else
    source = cpu_pic->register_bank[register_address];

  new_value = (src_value = source->get()) + (w_value = cpu_pic->W->value.get());

  // Store the result

  if(destination)
    source->put(new_value & 0xff);      // Result goes to source
  else
    cpu_pic->W->put(new_value & 0xff);

  cpu_pic->status->put_Z_C_DC(new_value, src_value, w_value);

  cpu_pic->pc->increment();

}

//--------------------------------------------------

ANDLW::ANDLW (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Literal_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("andlw");
}

void ANDLW::execute()
{
  unsigned int new_value;

  new_value = cpu_pic->W->value.get() & L;

  cpu_pic->W->put(new_value);
  cpu_pic->status->put_Z(0==new_value);

  cpu_pic->pc->increment();

}

//--------------------------------------------------

ANDWF::ANDWF (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Register_op(new_cpu, new_opcode, address)
{

  decode(new_cpu, new_opcode);
  new_name("andwf");

}

void ANDWF::execute()
{
  unsigned int new_value;

  if(!access)
    source = cpu_pic->registers[register_address];
  else
    source = cpu_pic->register_bank[register_address];

  new_value = source->get() & cpu_pic->W->value.get();

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu_pic->W->put(new_value);

  cpu_pic->status->put_Z(0==new_value);

  cpu_pic->pc->increment();

}


//--------------------------------------------------

BCF::BCF (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Bit_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  mask ^= 0xff;        // decode initializes the mask to 1<<bit
  new_name("bcf");
}

void BCF::execute()
{

  if(!access)
    reg = cpu_pic->registers[register_address];
  else
    reg = cpu_pic->register_bank[register_address];

  reg->put(reg->get_value() & mask);    // Must not use reg->value.get() as it breaks indirects

  cpu_pic->pc->increment();

}

//--------------------------------------------------

BSF::BSF (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Bit_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("bsf");
}

void BSF::execute()
{

  if(!access)
    reg = cpu_pic->registers[register_address];
  else
    reg = cpu_pic->register_bank[register_address];

  reg->put(reg->get_value() | mask);    // Must not use reg->value.get() as it breaks indirects


  cpu_pic->pc->increment();

}

//--------------------------------------------------

BTFSC::BTFSC (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Bit_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("btfsc");
}

void BTFSC::execute()
{

  if(!access)
    reg = cpu_pic->registers[register_address];
  else
    reg = cpu_pic->register_bank[register_address];

  unsigned int result = mask & reg->get();

  if(!result)
    cpu_pic->pc->skip();                  // Skip next instruction
  else
    cpu_pic->pc->increment();

}

//--------------------------------------------------

BTFSS::BTFSS (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Bit_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("btfss");
}

void BTFSS::execute()
{

  if(!access)
    reg = cpu_pic->registers[register_address];
  else
    reg = cpu_pic->register_bank[register_address];

  unsigned int result = mask & reg->get();

  if(result)
    cpu_pic->pc->skip();                  // Skip next instruction
  else
    cpu_pic->pc->increment();

}

//--------------------------------------------------
CALL::CALL (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : instruction(new_cpu, new_opcode, address)
{

  switch(cpu_pic->base_isa())
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

  new_name("call");
}

void CALL::execute()
{

  cpu_pic->stack->push(cpu_pic->pc->get_next());

  cpu_pic->pc->jump(cpu_pic->get_pclath_branching_jump() | destination);

}

char * CALL::name(char *return_str,int len)
{

  snprintf(return_str,len,"%s\t0x%04x",
           gpsimObject::name().c_str(),
           destination);

  return(return_str);
}


//--------------------------------------------------
CLRF::CLRF (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Register_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);

  new_name("clrf");
}

void CLRF::execute()
{

  if(!access)
    cpu_pic->registers[register_address]->put(0);
  else
    cpu_pic->register_bank[register_address]->put(0);

  cpu_pic->status->put_Z(1);

  cpu_pic->pc->increment();
}

char * CLRF::name(char *return_str,int len)
{

  snprintf(return_str,len,"%s\t%s",
           gpsimObject::name().c_str(),
           get_cpu()->registers[register_address]->name().c_str());

  return(return_str);
}


//--------------------------------------------------

CLRW::CLRW (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : instruction(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("clrw");
}

void CLRW::execute()
{

  cpu_pic->W->put(0);

  cpu_pic->status->put_Z(1);
  cpu_pic->pc->increment();

}

//--------------------------------------------------

CLRWDT::CLRWDT (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : instruction(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("clrwdt");
}

void CLRWDT::execute()
{


  cpu_pic->wdt.clear();

  cpu_pic->status->put_TO(1);
  cpu_pic->status->put_PD(1);
  cpu_pic->pc->increment();

}

//--------------------------------------------------

COMF::COMF (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Register_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("comf");
}

void COMF::execute()
{
  unsigned int new_value;

  if(!access)
    source = cpu_pic->registers[register_address];
  else
    source = cpu_pic->register_bank[register_address];

  new_value = source->get() ^ 0xff;

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu_pic->W->put(new_value);

  cpu_pic->status->put_Z(0==new_value);

  cpu_pic->pc->increment();

}

//--------------------------------------------------

DECF::DECF (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Register_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("decf");
}

void DECF::execute()
{
  unsigned int new_value;

  if(!access)
    source = cpu_pic->registers[register_address];
  else
    source = cpu_pic->register_bank[register_address];

  new_value = (source->get() - 1)&0xff;

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu_pic->W->put(new_value);

  cpu_pic->status->put_Z(0==new_value);

  cpu_pic->pc->increment();

}

//--------------------------------------------------

DECFSZ::DECFSZ (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Register_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("decfsz");
}

void DECFSZ::execute()
{
  unsigned int new_value;

  if(!access)
    source = cpu_pic->registers[register_address];
  else
    source = cpu_pic->register_bank[register_address];

  new_value = (source->get() - 1)&0xff;

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu_pic->W->put(new_value);

  if(0==new_value)
    cpu_pic->pc->skip();                  // Skip next instruction
  else
    cpu_pic->pc->increment();

}

//--------------------------------------------------
GOTO::GOTO (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : instruction(new_cpu, new_opcode, address)
{

  switch(cpu_pic->base_isa())
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

  new_name("goto");
}

void GOTO::execute()
{

  cpu_pic->pc->jump(cpu_pic->get_pclath_branching_jump() | destination);

}

char * GOTO::name(char *return_str,int len)
{

  snprintf(return_str,len,"%s\t0x%04x",
          gpsimObject::name().c_str(),destination);

  return(return_str);
}


//--------------------------------------------------

INCF::INCF (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Register_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("incf");
}

void INCF::execute()
{
  unsigned int new_value;

  if(!access)
    source = cpu_pic->registers[register_address];
  else
    source = cpu_pic->register_bank[register_address];

  new_value = (source->get() + 1)&0xff;

  // Store the result

  if(destination)
    source->put(new_value);
  else
    cpu_pic->W->put(new_value);


  cpu_pic->status->put_Z(0==new_value);

  cpu_pic->pc->increment();

}

//--------------------------------------------------

INCFSZ::INCFSZ (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Register_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("incfsz");
}

void INCFSZ::execute()
{
  unsigned int new_value;

  if(!access)
    source = cpu_pic->registers[register_address];
  else
    source = cpu_pic->register_bank[register_address];

  new_value = (source->get() + 1)&0xff;

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu_pic->W->put(new_value);

  if(0==new_value)
    cpu_pic->pc->skip();                  // Skip next instruction
  else
    cpu_pic->pc->increment();

}

//--------------------------------------------------

IORLW::IORLW (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Literal_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("iorlw");
}

void IORLW::execute()
{
  unsigned int new_value;

  new_value = cpu_pic->W->value.get() | L;

  cpu_pic->W->put(new_value);
  cpu_pic->status->put_Z(0==new_value);

  cpu_pic->pc->increment();

}

//--------------------------------------------------

IORWF::IORWF (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Register_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("iorwf");
}

void IORWF::execute()
{
  unsigned int new_value;

  if(!access)
    source = cpu_pic->registers[register_address];
  else
    source = cpu_pic->register_bank[register_address];

  new_value = source->get() | cpu_pic->W->value.get();

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu_pic->W->put(new_value);

  cpu_pic->status->put_Z(0==new_value);

  cpu_pic->pc->increment();

}

//--------------------------------------------------

MOVLW::MOVLW (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Literal_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("movlw");
}

void MOVLW::execute()
{

  cpu_pic->W->put(L);

  cpu_pic->pc->increment();

}

//--------------------------------------------------

MOVF::MOVF (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Register_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("movf");
}

void MOVF::execute()
{
  unsigned int source_value;

  if(!access)
    source = cpu_pic->registers[register_address];
  else
    source = cpu_pic->register_bank[register_address];

  source_value = source->get();

  // Store the result

  if(destination)
    source->put(source_value);
  else
    cpu_pic->W->put(source_value);


  cpu_pic->status->put_Z(0==source_value);

  cpu_pic->pc->increment();

}


void MOVF::debug()
{

   cout << "MOVF:  ";

}

//--------------------------------------------------
MOVWF::MOVWF (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Register_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("movwf");
}

void MOVWF::execute()
{

  if(!access)
    cpu_pic->registers[register_address]->put(cpu_pic->W->get());
  else
    cpu_pic->register_bank[register_address]->put(cpu_pic->W->get());

  cpu_pic->pc->increment();
}

char * MOVWF::name(char *return_str, int len)
{

  snprintf(return_str,len,"%s\t%s",
           gpsimObject::name().c_str(),
           get_cpu()->registers[register_address]->name().c_str());

  return(return_str);
}


//--------------------------------------------------

NOP::NOP (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : instruction(new_cpu,new_opcode,address)
{

  decode(new_cpu,new_opcode);
  new_name("nop");

  // For the 18cxxx family, this 'nop' may in fact be the
  // 2nd word in a 2-word opcode. So just to be safe, let's
  // initialize the cross references to the source file.
  // (Subsequent initialization code will overwrite this,
  // but there is a chance that this info will be accessed
  // before that occurs).
  /*
  file_id = 0;
  src_line = 0;
  lst_line = 0;
  */
}

void NOP::execute()
{
  cpu_pic->pc->increment();
}

//--------------------------------------------------

OPTION::OPTION (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : instruction(new_cpu,new_opcode,address)
{
  decode(new_cpu, new_opcode);
  new_name("option");
}

void OPTION::execute()
{

  cpu_pic->put_option_reg(cpu_pic->W->get());

  cpu_pic->pc->increment();

}

//--------------------------------------------------

RETLW::RETLW (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Literal_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("retlw");
}

void RETLW::execute()
{

  cpu_pic->W->put(L);

  cpu_pic->pc->new_address(cpu_pic->stack->pop());

}

//--------------------------------------------------

RLF::RLF (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Register_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("rlf");
}

void RLF::execute()
{
  unsigned int new_value;

  if(!access)
    source = cpu_pic->registers[register_address];
  else
    source = cpu_pic->register_bank[register_address];

  new_value = (source->get() << 1) | cpu_pic->status->get_C();

  if(destination)
    source->put(new_value&0xff);      // Result goes to source
  else
    cpu_pic->W->put(new_value&0xff);

  cpu_pic->status->put_C(new_value>0xff);

  cpu_pic->pc->increment();

}

//--------------------------------------------------

RRF::RRF (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Register_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("rrf");
}

void RRF::execute()
{
  unsigned int new_value,old_value;

  if(!access)
    source = cpu_pic->registers[register_address];
  else
    source = cpu_pic->register_bank[register_address];

  old_value = source->get();
  new_value = (old_value >> 1) | (cpu_pic->status->get_C() ? 0x80 : 0);

  if(destination)
    source->put(new_value&0xff);      // Result goes to source
  else
    cpu_pic->W->put(new_value&0xff);

  cpu_pic->status->put_C(old_value&0x01);

  cpu_pic->pc->increment();

}

//--------------------------------------------------

SLEEP::SLEEP (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : instruction(new_cpu,new_opcode,address)
{
  decode(new_cpu, new_opcode);
  new_name("sleep");
}

void SLEEP::execute()
{
  cpu_pic->enter_sleep();
}

//--------------------------------------------------

SUBWF::SUBWF (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Register_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("subwf");
}

void SUBWF::execute()
{
  unsigned int new_value,src_value,w_value;

  if(!access)
    source = cpu_pic->registers[register_address];
  else
    source = cpu_pic->register_bank[register_address];

  new_value = (src_value = source->get()) - (w_value = cpu_pic->W->value.get());

  // Store the result

  if(destination)
    source->put(new_value & 0xff);      // Result goes to source
  else
    cpu_pic->W->put(new_value & 0xff);

  cpu_pic->status->put_Z_C_DC_for_sub(new_value, src_value, w_value);

  cpu_pic->pc->increment();

}


//--------------------------------------------------

SWAPF::SWAPF (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Register_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("swapf");
}

void SWAPF::execute()
{
  unsigned int src_value;

  if(!access)
    source = cpu_pic->registers[register_address];
  else
    source = cpu_pic->register_bank[register_address];

  src_value = source->get();

  if(destination)
    source->put( ((src_value >> 4) & 0x0f) | ( (src_value << 4) & 0xf0) );
  else
    cpu_pic->W->put( ((src_value >> 4) & 0x0f) | ( (src_value << 4) & 0xf0) );

  cpu_pic->pc->increment();

}


//--------------------------------------------------
TRIS::TRIS (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Register_op(new_cpu, new_opcode, address)
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
     if(cpu_pic->base_isa() == _14BIT_PROCESSOR_)
       register_address |= 0x80;  // The destination register is the TRIS
  }
  new_name("tris");
}

void TRIS::execute()
{
  if(register_address)
    {
      // Execute the instruction only if the register is valid.
      if(cpu_pic->base_isa() == _14BIT_PROCESSOR_)
        cpu_pic->registers[register_address]->put(cpu_pic->W->get());
      else
        cpu_pic->tris_instruction(register_address);
    }

  cpu_pic->pc->increment();
}

char * TRIS::name(char *return_str,int len)
{

  snprintf(return_str,len,"%s\t%s",
           gpsimObject::name().c_str(),
           cpu_pic->registers[register_address]->name().c_str());

  return(return_str);
}


//--------------------------------------------------

XORLW::XORLW (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Literal_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("xorlw");
}

void XORLW::execute()
{
  unsigned int new_value;

  new_value = cpu_pic->W->value.get() ^ L;

  cpu_pic->W->put(new_value);
  cpu_pic->status->put_Z(0==new_value);

  cpu_pic->pc->increment();

}

//--------------------------------------------------

XORWF::XORWF (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Register_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("xorwf");
}

void XORWF::execute()
{
  unsigned int new_value;

  if(!access)
    source = cpu_pic->registers[register_address];
  else
    source = cpu_pic->register_bank[register_address];

  new_value = source->get() ^ cpu_pic->W->value.get();

  if(destination)
    source->put(new_value);      // Result goes to source
  else
    cpu_pic->W->put(new_value);

  cpu_pic->status->put_Z(0==new_value);

  cpu_pic->pc->increment();

}
