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


#include "dspic-processors.h"
#include "dspic-registers.h"

#include <stdio.h>
#include "../symbol.h"
#include "../program_files.h"
#include "../packages.h"
#include "../trace.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//  Globals
//


using namespace dspic;
using namespace dspic_registers;

ProcessorConstructor pdsPic30F6010(dsPic30F6010::construct ,
				   "__30f6010", "dspic30f6010",  "30f6010", "30f610");


namespace dspic {

  Trace *gTrace=0;              // Points to gpsim's global trace object.
  Cycle_Counter *gCycles=0;  	// Points to gpsim's global cycle counter.

  //-------------------------------------------------------------------
  //
  // dsPicProcessor -- constructor.

  dsPicProcessor::dsPicProcessor(const char *_name, const char *desc)
    : Processor(_name, desc)
  {
    gTrace = &get_trace();
    gCycles = &get_cycles();

    pcl = new PCL();

    pc = new dsPicProgramCounter(this,pcl);

    m_stack.init(this);
  }

  //-------------------------------------------------------------------
  //
  // create
  // 
  // Build the basic dsPicProcessor elements.

  void dsPicProcessor::create()
  {
    init_program_memory (program_memory_size());
    pc->memory_size_mask = program_memory_size()-1;
    init_register_memory (register_memory_size()/2);

    create_sfr_map();
    create_invalid_registers();

  }


  //-------------------------------------------------------------------
  //
  void dsPicProcessor::add_sfr_register(dspic_registers::dsPicRegister *pReg, 
					unsigned int addr, const char *pName,
					RegisterValue *rv
					)
  {
    if (!pReg)
      return;

    printf("adding sfr %s\n",pReg->name().c_str());
    pReg->set_cpu(this);
    if (addr < register_memory_size()) {

      registers[map_rm_address2index(addr)] = pReg;
      if (pName)
	pReg->new_name(pName);
      pReg->address = addr;
      pReg->alias_mask = 0;
      get_symbol_table().add_register(pReg);
      if (rv) {
	pReg->value = *rv;
	pReg->por_value = *rv;
      }
      pReg->set_write_trace(getWriteTT(addr));
      pReg->set_read_trace(getReadTT(addr));
    }

  }

  //-------------------------------------------------------------------
  //
  // load_hex
  //

  bool dsPicProcessor::LoadProgramFile(const char *pFilename, FILE *pFile, const char *pProcessorName)
  {
    Processor * pProcessor = this;

    ProgramFileType *pPFT = ProgramFileTypeList::GetList()[0];  // IntelHexProgramFileType

    if (pPFT)
      return pPFT->LoadProgramFile(&pProcessor, pFilename, pFile, pProcessorName);

    return false;
  }

  //------------------------------------------------------------------------
  void dsPicProcessor::create_sfr_map()
  {
    unsigned int j;

    // Initialize the General Purpose Registers:

    //add_file_registers(0xf80, 0xf7f, 0);
    unsigned int start_address = 0x0800/2;
    unsigned int end_address   = 0x27ff/2;
    char str[100];
    for  (j = start_address; j <= end_address; j++) {

      registers[j] = new dsPicRegister;
      registers[j]->alias_mask = 0;
      registers[j]->address = j;
      registers[j]->set_write_trace(getWriteTT(j));
      registers[j]->set_read_trace(getReadTT(j));

      //The default register name is simply its address
      sprintf (str, "R%03X", j);
      registers[j]->new_name(str);
      registers[j]->set_cpu(this);
    }

    RegisterValue porv(0,0);

    for (j=0; j<16; j++) {
      char buff[16];
      snprintf(buff, 16, "W%d",j);
      add_sfr_register(&W[j], j*2, buff,&porv);
    }

    add_sfr_register(pcl,   0x02e);

  }

  //------------------------------------------------------------------------
  void dsPicProcessor::init_program_memory_at_index(unsigned int uIndex,
						    const unsigned char *bytes,
						    int nBytes)
  {
    unsigned int unBytes = nBytes;
    for (unsigned int i =0; i<unBytes; i+=4)
      Processor::init_program_memory_at_index(uIndex/2 + (i/4), 
					      (((unsigned int)bytes[i+0])<<0)  |
					      (((unsigned int)bytes[i+1])<<8)  | 
					      (((unsigned int)bytes[i+2])<<16));


  }

  //------------------------------------------------------------------------
  void dsPicProcessor::step_one(bool refresh)
  {
    program_memory[pc->value]->execute();
  }
  void dsPicProcessor::interrupt()
  {
  }
  unsigned int dsPicProcessor::get_config_word(unsigned int)
  {
    return 0xffffffff;
  }
  void dsPicProcessor::por()
  {
  }




  //------------------------------------------------------------------------
  // dsPIC30F6010
  //------------------------------------------------------------------------
  dsPic30F6010::dsPic30F6010(const char *_name, const char *desc)
  {
  }

  Processor * dsPic30F6010::construct(const char *name)
  {
    dsPic30F6010 *p = new dsPic30F6010(name);

    printf ("Constructing a dspic 6010\n");

    get_symbol_table().add_module(p,p->name().c_str());

    p->create();
    return p;
  }

  //------------------------------------------------------------
  //
  void dsPic30F6010::create()
  {
    create_iopin_map();

    dsPicProcessor::create();

  }

  //------------------------------------------------------------
  //
  void dsPic30F6010::create_iopin_map()
  {
    package = new Package(80);

    if(!package)
      return;
  }

};
