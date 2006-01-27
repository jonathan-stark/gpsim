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

#if !defined(__DSPIC_INSTRUCTIONS_H__)
#define __DSPIC_INSTRUCTIONS_H__

#include "../pic-instructions.h"

namespace dspic_instructions
{

  //----------------------------------------------------------------------
  class NOP : public instruction
  {
  public:
    NOP(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new NOP(new_cpu,new_opcode);}
  };

};

#endif // __DSPIC_INSTRUCTIONS_H__
