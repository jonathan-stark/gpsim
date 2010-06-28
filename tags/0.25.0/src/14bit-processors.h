/*
   Copyright (C) 1998 T. Scott Dattalo
   Copyright (C) 2009 Roy R. Rankin


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


#include "pic-processor.h"
#include "intcon.h"
#include "uart.h"
#include "ssp.h"

#ifndef __14_BIT_PROCESSORS_H__
#define __14_BIT_PROCESSORS_H__

     // forward references
class _14bit_processor;
class PicPortRegister;
class PicTrisRegister;

class PicPortBRegister;
class PicTrisRegister;
class PortBSink;
class IOPIN;
class IO_open_collector;
class PinMonitor;

extern instruction *disasm14 (_14bit_processor *cpu,unsigned int inst, unsigned int address);



class _14bit_processor : public pic_processor
{

public:

#define EEPROM_SIZE              0x40
#define INTERRUPT_VECTOR         4
#define WDTE                     4

  unsigned int eeprom_size;

  INTCON       *intcon;

  void interrupt();
  virtual void save_state();
  virtual void create();
  virtual PROCESSOR_TYPE isa(){return _14BIT_PROCESSOR_;};
  virtual PROCESSOR_TYPE base_isa(){return _14BIT_PROCESSOR_;};
  virtual instruction * disasm (unsigned int address, unsigned int inst)
  {
    return disasm14(this, address, inst);
  }

  // Declare a set of functions that will allow the base class to
  // get information about the derived classes. NOTE, the values returned here
  // will cause errors if they are used -- the derived classes must define their
  // parameters appropriately.
  virtual void create_sfr_map()=0;
  virtual void option_new_bits_6_7(unsigned int)=0;
  virtual void put_option_reg(unsigned int);
  virtual void create_symbols()=0;

  virtual bool set_config_word(unsigned int address, unsigned int cfg_word);
  virtual void create_config_memory();

  // Return the portion of pclath that is used during branching instructions
  virtual unsigned int get_pclath_branching_jump()
    {
      return ((pclath->value.get() & 0x18)<<8);
    }

  // Return the portion of pclath that is used during modify PCL instructions
  virtual unsigned int get_pclath_branching_modpcl()
    {
      return((pclath->value.get() & 0x1f)<<8);
    }

  virtual unsigned int map_fsr_indf ( void )
    {
      return ( this->fsr->value.get() );
    }


  virtual unsigned int eeprom_get_size() {return 0;};
  virtual unsigned int eeprom_get_value(unsigned int address) {return 0;};
  virtual void eeprom_put_value(unsigned int value,
				unsigned int address)
    {return;}

  virtual unsigned int program_memory_size() const = 0;
  virtual unsigned int get_program_memory_at_address(unsigned int address);
  virtual void enter_sleep();
  virtual void exit_sleep();

  _14bit_processor(const char *_name=0, const char *desc=0);
  virtual ~_14bit_processor();

protected:
  OPTION_REG   *option_reg;
};

#define cpu14 ( (_14bit_processor *)cpu)


/***************************************************************************
 *
 * Include file for:  P16C84, P16F84, P16F83, P16CR83, P16CR84
 *
 * The x84 processors have a 14-bit core, eeprom, and are in an 18-pin
 * package. The class taxonomy is:
 *
 *   pic_processor 
 *      |-> 14bit_processor
 *             |    
 *             |----------\ 
 *                         |
 *                         |- P16C8x
 *                              |->P16C84
 *                              |->P16F84
 *                              |->P16C83
 *                              |->P16CR83
 *                              |->P16CR84
 *
 ***************************************************************************/
class PortBSink;
class Pic14Bit : public  _14bit_processor
{
public:

  Pic14Bit(const char *_name=0, const char *desc=0);
  virtual ~Pic14Bit();


  INTCON_14_PIR    intcon_reg;

  PicPortRegister  *m_porta;
  PicTrisRegister  *m_trisa;

  PicPortBRegister *m_portb;
  PicTrisRegister  *m_trisb;

  virtual PROCESSOR_TYPE isa(){return _14BIT_PROCESSOR_;};
  virtual void create_symbols();
  virtual void create_sfr_map();
  virtual void option_new_bits_6_7(unsigned int bits);
  virtual bool hasSSP() {return false;}
};



#endif
