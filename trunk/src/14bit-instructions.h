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
along with gpsim; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

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
  ADDLW(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new ADDLW(new_cpu,new_opcode);}

};



//---------------------------------------------------------
class RETFIE : public instruction
{
public:

  RETFIE(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new RETFIE(new_cpu,new_opcode);}

};


//---------------------------------------------------------
class RETURN : public instruction
{
public:

  RETURN(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new RETURN(new_cpu,new_opcode);}

};

//---------------------------------------------------------

class SUBLW : public Literal_op
{

public:

  SUBLW(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new SUBLW(new_cpu,new_opcode);}

};


#endif //  __14BIT_INSTRUCTIONS_H__
