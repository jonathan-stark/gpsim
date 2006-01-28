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

#if !defined(__DSPIC_REGISTERS_H__)
#define __DSPIC_REGISTERS_H__

#include "../registers.h"

class dsPicProgramCounter : public Program_Counter
{
public:
  //virtual void increment();
  //virtual void skip();
  //virtual void jump(unsigned int new_value);
  //virtual void interrupt(unsigned int new_value);
  virtual void computed_goto(unsigned int new_value);
  //virtual void new_address(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  virtual unsigned int get_value();
  //virtual unsigned int get_next();

  dsPicProgramCounter();

};
#endif // !defined(__DSPIC_REGISTERS_H__)
