/*
   Copyright (C) 2000,2001 T. Scott Dattalo, Daniel Schudel, Robert Pearce

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


//
// p16x5x
//
//  This file supports:
//    P16C54
//    P16C55

#ifndef __P16X5X_H__
#define __P16X5X_H__

#include "packages.h"
#include "stimuli.h"
#include "12bit-processors.h"



class P16C54 : public  _12bit_processor
{
public:
  PIC_IOPORT   *porta;
  IOPORT_TRIS  trisa;

  PIC_IOPORT   *portb;
  IOPORT_TRIS  trisb;

  PIC_IOPORT   *portc;
  IOPORT_TRIS  trisc;

  virtual PROCESSOR_TYPE isa(void){return _P16C54_;};
  virtual void create_symbols(void);

  virtual unsigned int program_memory_size(void) const { return 0x200; };
  virtual unsigned int register_memory_size(void) const { return 0x20; };
  virtual unsigned int config_word_address(void) const {return 0xFFF;};

  virtual void create_sfr_map(void);

  virtual void option_new_bits_6_7(unsigned int bits)
    {
      // ((PORTB *)portb)->rbpu_intedg_update(bits);
    }

  P16C54(void);
  void create(void);
  virtual void create_iopin_map(void);

  static Processor *construct(void);
  virtual void tris_instruction(unsigned int tris_register);

  virtual unsigned int fsr_valid_bits(void)
    {
      return 0x1f;  // Only 32 register addresses 
    }

  virtual unsigned int fsr_register_page_bits(void)
    {
      return 0;     // Only one register page.
    }


};

class P16C55 : public  _12bit_processor
{
public:

  PIC_IOPORT   *porta;
  IOPORT_TRIS  trisa;

  PIC_IOPORT   *portb;
  IOPORT_TRIS  trisb;

  PIC_IOPORT   *portc;
  IOPORT_TRIS  trisc;


  virtual PROCESSOR_TYPE isa(void){return _P16C55_;};
  virtual void create_symbols(void);

  virtual unsigned int program_memory_size(void) const { return 0x200; };
  virtual unsigned int register_memory_size(void) const { return 0x20; };
  virtual unsigned int config_word_address(void) const {return 0xFFF;};

  virtual void create_sfr_map(void);

  virtual void option_new_bits_6_7(unsigned int bits)
    {
      // ((PORTB *)portb)->rbpu_intedg_update(bits);
    }

  P16C55(void);
  void create(void);
  virtual void create_iopin_map(void);

  static Processor *construct(void);
  virtual void tris_instruction(unsigned int tris_register);

};

#endif
