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

// T. Scott Dattalo 14bit core routines

// Portions of this file are from:
//
/* pic14.c  - pic 14bit core routines   */
/* version 0.1                          */
/* (c) I.King 1994                      */

#include <iostream.h>

#include "../config.h"
#include "14bit-processors.h"
struct instruction_constructor op_16cxx[] = {

  { 0x3f00,  0x3e00,  ADDLW::construct }, 
  { 0x3f00,  0x3f00,  ADDLW::construct }, // Accomdate don't care bit
  { 0x3f00,  0x0700,  ADDWF::construct },
  { 0x3f00,  0x3900,  ANDLW::construct },
  { 0x3f00,  0x0500,  ANDWF::construct },
  { 0x3c00,  0x1000,  BCF::construct },
  { 0x3c00,  0x1400,  BSF::construct },
  { 0x3c00,  0x1800,  BTFSC::construct },
  { 0x3c00,  0x1c00,  BTFSS::construct },
  { 0x3800,  0x2000,  CALL::construct },
  { 0x3f80,  0x0180,  CLRF::construct },
  { 0x3fff,  0x0103,  CLRW::construct },
  { 0x3fff,  0x0064,  CLRWDT::construct },
  { 0x3f00,  0x0900,  COMF::construct },
  { 0x3f00,  0x0300,  DECF::construct },
  { 0x3f00,  0x0b00,  DECFSZ::construct },
  { 0x3800,  0x2800,  GOTO::construct },
  { 0x3f00,  0x0a00,  INCF::construct },
  { 0x3f00,  0x0f00,  INCFSZ::construct },
  { 0x3f00,  0x3800,  IORLW::construct },
  { 0x3f00,  0x0400,  IORWF::construct },
  { 0x3f00,  0x0800,  MOVF::construct },
  { 0x3f00,  0x3000,  MOVLW::construct },
  { 0x3f00,  0x3100,  MOVLW::construct },
  { 0x3f00,  0x3200,  MOVLW::construct },
  { 0x3f00,  0x3300,  MOVLW::construct },
  { 0x3f80,  0x0080,  MOVWF::construct },
  { 0x3fff,  0x0000,  NOP::construct },
  { 0x3fff,  0x0020,  NOP::construct },
  { 0x3fff,  0x0040,  NOP::construct },
  { 0x3fff,  0x0060,  NOP::construct },
  { 0x3fff,  0x0062,  OPTION::construct },
  { 0x3fff,  0x0009,  RETFIE::construct },
  { 0x3f00,  0x3400,  RETLW::construct },
  { 0x3f00,  0x3500,  RETLW::construct },
  { 0x3f00,  0x3600,  RETLW::construct },
  { 0x3f00,  0x3700,  RETLW::construct },
  { 0x3fff,  0x0008,  RETURN::construct },
  { 0x3f00,  0x0d00,  RLF::construct },
  { 0x3f00,  0x0c00,  RRF::construct },
  { 0x3fff,  0x0063,  SLEEP::construct },
  { 0x3f00,  0x3c00,  SUBLW::construct },
  { 0x3f00,  0x3d00,  SUBLW::construct },
  { 0x3f00,  0x0200,  SUBWF::construct },
  { 0x3fff,  0x0065,  TRIS::construct },
  { 0x3fff,  0x0066,  TRIS::construct },
  { 0x3fff,  0x0067,  TRIS::construct },
  { 0x3f00,  0x0e00,  SWAPF::construct },
  { 0x3f00,  0x3a00,  XORLW::construct },
  { 0x3f00,  0x0600,  XORWF::construct },

};


const int NUM_OP_16CXX	= sizeof(op_16cxx) / sizeof(op_16cxx[0]);


//instruction * disasm14 (pic_processor *cpu, unsigned int address, unsigned int inst)
instruction * disasm14 (_14bit_processor *cpu, unsigned int inst)
{
  instruction *pi;
  char buf[50];

  pi = NULL;
  for(int i =0; i<NUM_OP_16CXX; i++)
    if((op_16cxx[i].inst_mask & inst) == op_16cxx[i].opcode)
      pi = op_16cxx[i].inst_constructor(cpu, inst);

  if(pi == NULL)
    pi = invalid_instruction::construct(cpu, inst);

  return (pi);
}


#if 0

instruction * disasm14 (_14bit_processor *cpu, unsigned int inst)
{
  unsigned char hinibble;
  unsigned char topnibble;
  unsigned char midnibble;
  unsigned char lownibble;
  unsigned char bbyte;
  unsigned char bits6and7;
  unsigned char bits10and11;

  hinibble = (inst & 0x3000) >> 12;
  topnibble = (inst & 0x0f00) >> 8;
  midnibble = (inst & 0x00f0) >> 4;
  lownibble = (inst & 0x000f);
  bbyte = (inst & 0x00ff);
  bits6and7 = (unsigned char) ((int) (bbyte & 0xc0) >> 6);
  bits10and11 = (unsigned char) ((int) (inst & 0x0c00) >> 10);

  switch (hinibble)
    {
    case 0x00:
      switch (topnibble)
	{
	case 0x00:
	  if (bbyte & 0x80)
	    return(new MOVWF(cpu,inst));
	  else
	    switch (bbyte)
	      {
	      case 0x00:
	      case 0x20:
	      case 0x40:
	      case 0x60:
		return(new NOP(cpu,inst));
	      case 0x64:
		return(new CLRWDT(cpu,inst));
	      case 0x09:
		return(new RETFIE(cpu,inst));
	      case 0x08:
		return(new RETURN(cpu,inst));
	      case 0x63:
		return(new SLEEP(cpu,inst));
	      case 0x62:
		return(new OPTION(cpu,inst));
	      default:
		if ((bbyte & 0xf8) == 0x60)
		  return(new TRIS(cpu,inst));
		else
  		  break;
	      }
	  break;

	case 0x01:
	  if (bbyte & 0x80)
	    return(new CLRF(cpu,inst));
	  else
	    return(new CLRW(cpu,inst));
	case 0x02:
	  return(new SUBWF(cpu,inst));
	case 0x03:
	  return(new DECF(cpu,inst));
	case 0x04:
	  return(new IORWF(cpu,inst));
	case 0x05:
	  return(new ANDWF(cpu,inst));
	case 0x06:
	  return(new XORWF(cpu,inst));
	case 0x07:
	  return(new ADDWF(cpu,inst));
	case 0x08:
	  return(new MOVF(cpu,inst));
	case 0x09:
	  return(new COMF(cpu,inst));
	case 0x0a:
	  return(new INCF(cpu,inst));
	case 0x0b:
	  return(new DECFSZ(cpu,inst));
	case 0x0c:
	  return(new RRF(cpu,inst));
	case 0x0d:
	  return(new RLF(cpu,inst));
	case 0x0e:
	  return(new SWAPF(cpu,inst));
	case 0x0f:
	  return(new INCFSZ(cpu,inst));
	}
      break;

    case 0x01:
      switch (bits10and11)
	{
	case 0x00:
	  return(new BCF(cpu,inst));
	case 0x01:
	  return(new BSF(cpu,inst));
	case 0x02:
	  return(new BTFSC(cpu,inst));
	case 0x03:
	  return(new BTFSS(cpu,inst));
	}
      break;

    case 0x02:
      if (inst & 0x0800)
	return(new GOTO(cpu,inst));
      else
	return(new CALL(cpu,inst));

    case 0x03:
      switch (topnibble)
	{
	case 0x00:
	case 0x01:
	case 0x02:
	case 0x03:
	  return(new MOVLW(cpu,inst));
	case 0x04:
	case 0x05:
	case 0x06:
	case 0x07:
	  return(new RETLW(cpu,inst));
	case 0x08:
	  return(new IORLW(cpu,inst));
	case 0x09:
	  return(new ANDLW(cpu,inst));
	case 0x0a:
	  return(new XORLW(cpu,inst));
	case 0x0b:
	  break;
	case 0x0c:
	case 0x0d:
	  return(new SUBLW(cpu,inst));
	case 0x0e:
	case 0x0f:
	  return(new ADDLW(cpu,inst));
	}
      break;

    }

  cout << "*** Warning Illegal Instruction  " << hex << inst << '\n';

  return(NULL);
}

#endif
/* ... The End ... */
