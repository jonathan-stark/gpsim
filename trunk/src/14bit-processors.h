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


#include "pic-processor.h"
#include "packages.h"

#ifndef __14_BIT_PROCESSORS_H__
#define __14_BIT_PROCESSORS_H__

     // forward references
class _14bit_processor;
class IOPIN;

extern instruction *disasm14 (_14bit_processor *cpu,unsigned int inst);



class _14bit_processor : public pic_processor
{

public:

#define EEPROM_SIZE              0x40
#define INTERRUPT_VECTOR         4
#define WDTE                     4

  unsigned int eeprom_size;

  INTCON       *intcon;

  EECON1       eecon1;
  EECON2       eecon2;
  EEDATA       eedata;
  EEADR        eeadr;
  EEDATA       eedatah;
  EEADR        eeadrh;

  EEPROM       eeprom;


  virtual void create_symbols(void);
//  virtual void load_hex(char *hex_file);
  void interrupt(void);
  virtual void por(void);
  virtual void create(void);// {return;};
  virtual PROCESSOR_TYPE isa(void){return _14BIT_PROCESSOR_;};
  virtual PROCESSOR_TYPE base_isa(void){return _14BIT_PROCESSOR_;};
  virtual instruction * disasm (unsigned int address, unsigned int inst)
    {
      return disasm14(this, inst);
    }

  // Declare a set of functions that will allow the base class to
  // get information about the derived classes. NOTE, the values returned here
  // will cause errors if they are used -- the derived classes must define their
  // parameters appropriately.
  virtual void create_sfr_map(void) { return;};

  // Return the portion of pclath that is used during branching instructions
  virtual unsigned int get_pclath_branching_jump(void)
    {
      return ((pclath.value & 0x18)<<8);
    }

  // Return the portion of pclath that is used during modify PCL instructions
  virtual unsigned int get_pclath_branching_modpcl(void)
    {
      return((pclath.value & 0x1f)<<8);
    }

  virtual void option_new_bits_6_7(unsigned int);

  virtual unsigned int eeprom_get_size(void) {return 0;};
  virtual unsigned int eeprom_get_value(unsigned int address) {return 0;};
  virtual void eeprom_put_value(unsigned int value,
				unsigned int address)
    {return;};

  virtual int get_pin_count(void){return 0;};
  virtual char *get_pin_name(unsigned int pin_number) {return NULL;};
  virtual int get_pin_state(unsigned int pin_number) {return 0;};
  virtual IOPIN *get_pin(unsigned int pin_number) {return NULL;};

  virtual unsigned int program_memory_size(void) const {return 0;};
  static pic_processor *construct(void);
};

#define cpu14 ( (_14bit_processor *)cpu)


#endif
