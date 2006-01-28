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

using namespace dspic;

ProcessorConstructor pdsPic30F6010(dsPic30F6010::construct ,
				   "__30f6010", "dspic30f6010",  "30f6010", "30f610");


namespace dspic {

  //-------------------------------------------------------------------
  //
  // dsPicProcessor -- constructor.

  dsPicProcessor::dsPicProcessor()
  {
    pc = new dsPicProgramCounter();
  }
  //-------------------------------------------------------------------
  //
  // create
  // 
  // Build the basic dsPicProcessor elements.

  void dsPicProcessor::create()
  {
    init_program_memory (program_memory_size());

    init_register_memory (register_memory_size());

  }
  //-------------------------------------------------------------------
  //
  // load_hex
  //

  bool dsPicProcessor::LoadProgramFile(const char *pFilename, FILE *pFile)
  {
    Processor * pProcessor = this;

    ProgramFileType *pPFT = ProgramFileTypeList::GetList()[0];  // IntelHexProgramFileType

    if (pPFT)
      return pPFT->LoadProgramFile(&pProcessor, pFilename, pFile);

    return false;
  }

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

  }
  void dsPicProcessor::interrupt()
  {
  }
  unsigned int dsPicProcessor::get_config_word()
  {
  }
  void dsPicProcessor::por()
  {
  }

  //------------------------------------------------------------------------
  // dsPIC30F6010

  Processor * dsPic30F6010::construct()
  {
    dsPic30F6010 *p = new dsPic30F6010();

    printf ("Constructing a dspic 6010\n");

    p->new_name("dsPIC30F6010");
    get_symbol_table().add_module(p,p->name().c_str());

    p->create();
    return p;
  }

};
