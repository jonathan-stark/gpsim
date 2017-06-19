/*
   Copyright (C) 2006 T. Scott Dattalo

This file is part of the libgpsim_dspic library of gpsim

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

#ifndef __DSPIC_PROCESSOR_H__
#define __DSPIC_PROCESSOR_H__

#include "../processor.h"
#include "dspic-registers.h"
/*
namespace dspic_registers {
  class PCL;
  class dsPicRegister;
};
*/
namespace dspic {

  class dsPicProcessor : public Processor
  {
  public:

    dsPicProcessor(const char *_name=0, const char *desc=0);

    // create - build the dsPic
    virtual void create ();
    virtual void create_sfr_map();

    virtual unsigned int program_memory_size() const { return 0x1000; };
    virtual unsigned int register_memory_size () const { return 0x2800;};

    virtual int  map_pm_address2index(int address) {return address/2;}
    virtual int  map_pm_index2address(int index) {return index*2;}

    // Register details
    virtual unsigned int register_size () const { return 2;}
    virtual unsigned int register_mask () const { return 0xffff;}
    virtual unsigned int YDataRamEnd   () const { return 0x27ff;}
    virtual int  map_rm_address2index(int address) {return address/2;}
    virtual int  map_rm_index2address(int index) {return index*2;}
    void add_sfr_register(dspic_registers::dsPicRegister *reg, 
			  unsigned int addr, const char*pName=0,
			  RegisterValue *rv=0);

    // opcode_size - number of bytes for an opcode.
    // The opcode's are really only 3 bytes, however in
    // hex files they're encoded in 4 bytes.
    virtual int opcode_size() { return 4;}

    // Load a hex file:
    bool LoadProgramFile(const char *pFilename, FILE *pFile, const char *pProcessorName);
    virtual void init_program_memory_at_index(unsigned int address, 
					      const unsigned char *, int nBytes);

    // disasm -- turn an opcode into an instruction
    // (this function resides dspic-instructions.cc)
    virtual instruction * disasm ( unsigned int address,unsigned int inst);

    // Execution control
    virtual void step_one(bool refresh=true);
    virtual void step(unsigned int steps,bool refresh=true);
    virtual void step_cycle();
    virtual void interrupt();
    virtual void run(bool refresh=true);
    virtual void finish();

    // Configuration control
    virtual unsigned int get_config_word(unsigned int);

    // Reset control
    // por = Power On Reset
    virtual void reset(RESET_TYPE r);

    // Public Data members:
    dspic_registers::WRegister W[16];
    dspic_registers::Stack  m_stack;
    dspic_registers::Status m_status;

  protected:
    unsigned int m_current_disasm_address;  // Used only when .hex files are loaded

    dspic_registers::PCL   *pcl;
  };


  class dsPic30F6010 : public dsPicProcessor
  {
  public:
    dsPic30F6010(const char *_name=0, const char *desc=0);
    static Processor *construct(const char *name);
    virtual void create ();
    void create_iopin_map();

  };


}  // end of namespace dspic

#define cpu_dsPic ((dspic::dsPicProcessor *)cpu)

#endif // __DSPIC_PROCESSOR_H__
