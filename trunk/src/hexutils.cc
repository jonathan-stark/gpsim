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

// T. Scott Dattalo 

// Portions of this file were obtained from:

/* intel16.c - read an intel hex file */
/* Copyright (c) 1994 Ian King */

#include <stdio.h>
#include <ctype.h>
#include "picdis.h"

#include "exports.h"
#include "pic-processor.h"
#include "hexutils.h"
#include "program_files.h"


//------------------------------------------------------------------------
// IntelHexProgramFileType -- constructor
//
// When a IntelHexProgramFileType is instantiated, it will get placed
// on to a 'ProgramFileType' list. This list is then used (ultimately)
// by the 'load' command to load files of this type.

IntelHexProgramFileType::IntelHexProgramFileType()
{
  RegisterProgramFileType(this);
}


int 
IntelHexProgramFileType::getachar (FILE * file)
{
  int c;

  do
    c = fgetc (file);
  while (c == '\r');		/* strip LF out of MSDOS files */

  return c;
}

unsigned char 
IntelHexProgramFileType::getbyte (FILE * file)
{
  unsigned char byte;
  unsigned int data;

  fscanf (file, "%02x", &data);

  byte = data & 0xff;
  checksum += byte;		/* all the bytes are used in the checksum */
  /* so here is the best place to update it */
  return byte;
}

unsigned int
IntelHexProgramFileType::getword(FILE *file)
{
  unsigned char lo = getbyte(file);
  return ((getbyte(file) << 8) | lo);
}
#include "cmd_gpsim.h"

int IntelHexProgramFileType::LoadProgramFile(Processor **pProcessor,
                                           const char *pFilename,
                                           FILE *inputfile, const char *pProcessorName) 
{
  if(verbose)
    cout << "load hex\n";

  if(*pProcessor == NULL) {
    // Need to determine processor from file.
    // for now return error.
    return ERR_NEED_PROCESSOR_SPECIFIED;
  }
  // assume no configuration word is in the hex file.
  (*pProcessor)->set_config_word((*pProcessor)->config_word_address(),0xffff);
  int iReturn;
  if ((iReturn = readihex16 (pProcessor, inputfile)) != SUCCESS) {
    // No errors were found in the hex file.
    (*pProcessor)->set_frequency(10e6);
    (*pProcessor)->reset(POR_RESET);
    (*pProcessor)->simulation_mode = eSM_STOPPED;
    if(verbose)
      get_cycles().dump_breakpoints();
    return SUCCESS;
  }
  return iReturn;
}

int IntelHexProgramFileType::readihex16 (Processor **pProcessor, FILE * file)
{
  int extended_address = 0;
  int address;
  int linetype = 0;
  int wordsthisline, bytesthisline;
  int i;
  int lineCount = 1;
  int csby;
  unsigned char hi, lo;
  Processor *& cpu = *pProcessor;

  while (1) {
      if (getachar (file) != ':') {
        printf ("Need a colon as first character in each line\n");
        printf ("Colon missing in line %d\n", lineCount);
        //exit (1);
        return ERR_BAD_FILE;
      }

      checksum = 0;
      bytesthisline = getbyte (file);
      wordsthisline = bytesthisline  / 2;
      hi = getbyte (file);
      lo = getbyte (file);
      address = (hi << 8 | lo) / 2;

      /* wierdness of INHX16! address different */
      /* endian-ness and 2x too big */

      linetype = getbyte (file);	/* 0 for data, 1 for end  */

      switch (linetype ) {
      case 0:      // Data record
	{
	  unsigned char buff[256];
	  bytesthisline &= 0xff;
	  for (i = 0; i < bytesthisline; i++) 
	    buff[i] = getbyte(file);

	  cpu->init_program_memory_at_index(address|extended_address,
					    buff, bytesthisline);
	}
	break;
      case 1:      // End of hex file
	return SUCCESS;
      case 4:      // Extended address
	{
	  unsigned char b1, b2;
	  b1 = getbyte (file);		// not sure what these mean
	  b2 = getbyte (file);

	  extended_address = 	(((unsigned int)b1)<<23) | (((unsigned int)b2)<<15);
	  printf ("Extended linear address %x %x\n",
		  address, extended_address);

	}
	break;
      default:
	printf ("Error! Unknown record type! %d\n", linetype);
        return ERR_BAD_FILE;
      }

      csby = getbyte (file);	/* get the checksum byte */
      /* this should make the checksum zero */
      /* due to side effect of getbyte */

      if (checksum)	{
        printf ("Checksum error in input file.\n");
        printf ("Got 0x%02x want 0x%02x at line %d\n",
          csby, (0 - checksum) & 0xff, lineCount);
        return ERR_BAD_FILE;
      }

      (void) getachar (file);	/* lose <return> */

      lineCount++;
    }

  return SUCCESS;
}

/* ... The End ... */
