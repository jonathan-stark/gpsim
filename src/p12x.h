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

#ifndef __P12X_H__
#define __P12X_H__

#include "12bit-processors.h"

class GPIO : public PIC_IOPORT
{
public:

  unsigned int get(void);
  void setbit(unsigned int bit_number, bool new_value);
};

/*
class OSCCAL : public sfr_register
{

}

*/


class P12C508 : public  _12bit_processor
{
  public:

  GPIO         gpio;
  IOPORT_TRIS  tris;
  sfr_register osccal;  // %%% FIX ME %%% Nothing's done with this.

  //  const int PROGRAM_MEMORY_SIZE = 0x200;

  virtual PROCESSOR_TYPE isa(void){return _P12C508_;};
  virtual void create_symbols(void);

  virtual unsigned int program_memory_size(void) const { return 0x200; };
  virtual void create_sfr_map(void);
  virtual void dump_registers(void);
  virtual void tris_instruction(unsigned int tris_register);
  virtual void reset(RESET_TYPE r);

  P12C508(void);
  static Processor *construct(void);
  void create(void);
  virtual void create_iopin_map(void);

  virtual unsigned int fsr_valid_bits(void)
    {
      return 0x1f;  // Assume only 32 register addresses 
    }

  virtual unsigned int fsr_register_page_bits(void)
    {
      return 0;     // Assume only one register page.
    }


  virtual void option_new_bits_6_7(unsigned int);

};


// A 12c509 is like a 12c508
class P12C509 : public P12C508
{
  public:

  virtual PROCESSOR_TYPE isa(void){return _P12C509_;};

  virtual unsigned int program_memory_size(void) const { return 0x400; };

  virtual void create_sfr_map(void);

  virtual unsigned int fsr_valid_bits(void)
    {
      return 0x3f;  // 64 registers in all (some are actually aliased)
    }

  virtual unsigned int fsr_register_page_bits(void)
    {
      return 0x20;  // 509 has 2 register banks
    }

  P12C509(void);
  static Processor *construct(void);
  void create(void);


};

#endif //  __P12X_H__
