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

#include "../config.h"
#include "pic-processor.h"

extern unsigned int config_word;

unsigned char checksum;

int 
getachar (FILE * file)
{
  int c;

  do
    c = fgetc (file);
  while (c == '\r');		/* strip LF out of MSDOS files */

  return c;
}

unsigned char 
getbyte (FILE * file)
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
getword(FILE *file)
{
  unsigned char lo = getbyte(file);
  return ((getbyte(file) << 8) | lo);
}

int
readihex16 (pic_processor *cpu, FILE * file)
{
  int address;
  int linetype = 0;
  int wordsthisline;
  int i;
  int lineCount = 1;
  int csby;
  unsigned char hi, lo;

  while (1)
    {
      if (getachar (file) != ':')
	{
	  printf ("Need a colon as first character in each line\n");
	  printf ("Colon missing in line %d\n", lineCount);
	  //exit (1);
	  return 1;
	}

      checksum = 0;

      wordsthisline = getbyte (file) / 2;
      hi = getbyte (file);
      lo = getbyte (file);
      address = (hi << 8 | lo) / 2;

      /* wierdness of INHX16! address different */
      /* endian-ness and 2x too big */

      linetype = getbyte (file);	/* 0 for data, 1 for end  */

      if (linetype == 1)	/* lets get out of here hit the end */
	break;

      if (0 == linetype) {	// data record
	  for (i = 0; i < wordsthisline; i++)
	      cpu->init_program_memory(address++, getword(file));
      } else if (4 == linetype) {	// Extended linear address
	  unsigned char b1, b2;
	  b1 = getbyte (file);		// not sure what these mean
	  b2 = getbyte (file);

	  if ((0 != address) || (0 != b1) || (0 != b2)) {
	      printf ("Error! Unhandled Extended linear address! %x %.2x%.2x\n",
		      address, b1, b2);
	      break;
	  }
	  // Should do something with all this info
	  // BUG: must fix this for pic18 support
      } else {
	  printf ("Error! Unknown record type! %d\n", linetype);
	  break;
      }

      csby = getbyte (file);	/* get the checksum byte */
      /* this should make the checksum zero */
      /* due to side effect of getbyte */

      if (checksum)
	{
	  printf ("Checksum error in input file.\n");
	  printf ("Got 0x%02x want 0x%02x at line %d\n", csby, (0 - checksum) & 0xff, lineCount);
	  exit (1);
	}

      (void) getachar (file);	/* lose <return> */

      lineCount++;
    }

  return 0;
}

/* ... The End ... */
