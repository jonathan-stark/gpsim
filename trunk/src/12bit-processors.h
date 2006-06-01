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


#include "pic-processor.h"


#ifndef __12_BIT_PROCESSORS_H__
#define __12_BIT_PROCESSORS_H__

     // forward references
class _12bit_processor;
class IOPIN;

extern instruction *disasm12 (pic_processor *cpu,unsigned int inst);


class _12bit_processor : public pic_processor
{

public:

#define WDTE                     4

enum _12BIT_DEFINITIONS
{
  PA0 = 1<<5,     /* Program page preselect bits (in status) */
  PA1 = 1<<6,
  PA2 = 1<<7,

  RP0 = 1<<5,     /* Register page select bits (in fsr) */
  RP1 = 1<<6

};

  unsigned int pa_bits;   /* a CPU dependent bit-mask defining which of the program
                           * page bits in the status register are significant. */

  virtual void reset(RESET_TYPE r);

  virtual void create_symbols(void);
  virtual void por(void);
  #define FILE_REGISTERS  0x100
  virtual unsigned int register_memory_size () const { return FILE_REGISTERS;};
  virtual void dump_registers(void);
  virtual void tris_instruction(unsigned int tris_register){return;};
  virtual void create(void);
  virtual PROCESSOR_TYPE isa(void){return _12BIT_PROCESSOR_;};
  virtual PROCESSOR_TYPE base_isa(void){return _12BIT_PROCESSOR_;};
  virtual instruction * disasm (unsigned int address, unsigned int inst)
    {
      return disasm12(this, inst);
    }
  void interrupt(void) { return; };

  // Declare a set of functions that will allow the base class to
  // get information about the derived classes. NOTE, the values returned here
  // will cause errors if they are used -- the derived classes must define their
  // parameters appropriately.
  virtual unsigned int program_memory_size(void){ return 3; }; // A bogus value that will cause errors if used
  // The size of a program memory bank is 2^11 bytes for the 12-bit core
  virtual void create_sfr_map(void) { return;};

  // Return the portion of pclath that is used during branching instructions
  // Actually, the 12bit core has no pclath. However, the program counter class doesn't need
  // to know that. Thus this virtual function really just returns the code page for the
  // 12bit cores.

  virtual unsigned int get_pclath_branching_jump(void)
    {
      return ((status->value.get() & pa_bits) << 4);
    }

  // The modify pcl type instructions execute exactly like call instructions

  virtual unsigned int get_pclath_branching_modpcl(void)
    {
      return ((status->value.get() & pa_bits) << 4);
    }

  // The valid bits in the FSR register vary across the various 12-bit devices

  virtual unsigned int fsr_valid_bits(void)
    {
      return 0x1f;  // Assume only 32 register addresses 
    }

  virtual unsigned int fsr_register_page_bits(void)
    {
      return 0;     // Assume only one register page.
    }

  virtual void option_new_bits_6_7(unsigned int);

  virtual unsigned int config_word_address(void) const {return 0xfff;};
  virtual bool set_config_word(unsigned int address, unsigned int cfg_word);

  _12bit_processor(void);
};

#define cpu12 ( (_12bit_processor *)cpu)


#endif
