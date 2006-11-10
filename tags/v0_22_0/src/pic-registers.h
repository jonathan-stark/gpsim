
/*
   Copyright (C) 1998-2000 T. Scott Dattalo

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

#ifndef __PIC_REGISTERS_H__
#define __PIC_REGISTERS_H__


#include "gpsim_classes.h"
#include "registers.h"
#include "breakpoints.h"


//------------------------------------------------------------------------
//
// PCHelper
//
// The purpose of this class is to provide a register wrapper around the
// program counter. On the low and mid range pics, the program counter spans
// two registers. On the high end ones it spans 3. This class allows the
// gui to treat the program counter as though if it's a single register.

class PCHelper : public Register 
{
public:

  PCHelper(ProgramMemoryAccess *);
  virtual void put_value(unsigned int new_value);
  virtual unsigned int get_value(void);
  virtual unsigned int register_size () const
  { 
    return 2;
  }

  ProgramMemoryAccess *pma;
};

#endif
