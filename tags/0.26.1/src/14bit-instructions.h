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

class instruction;  // forward declaration for the include files that follow

#ifndef __14BIT_INSTRUCTIONS_H__
#define __14BIT_INSTRUCTIONS_H__

#define REG_IN_INSTRUCTION_MASK  0x7f
#define DESTINATION_MASK         0x80

#include "pic-instructions.h"
#include "12bit-instructions.h"
#include "14bit-registers.h"


//---------------------------------------------------------

class ADDLW : public Literal_op
{

public:
  ADDLW(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
    {return new ADDLW(new_cpu,new_opcode, address);}

};



//---------------------------------------------------------
class RETFIE : public instruction
{
public:

  RETFIE(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute(void);
  virtual bool isBase() { return true;}
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
    {return new RETFIE(new_cpu,new_opcode,address);}

};


//---------------------------------------------------------
class RETURN : public instruction
{
public:

  RETURN(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute(void);
  virtual bool isBase() { return true;}
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
    {return new RETURN(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------

class SUBLW : public Literal_op
{

public:

  SUBLW(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
    {return new SUBLW(new_cpu,new_opcode,address);}

};


#endif //  __14BIT_INSTRUCTIONS_H__
