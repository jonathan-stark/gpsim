/*
   Copyright (C) 1999 T. Scott Dattalo

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


#include <iostream>
#include <iomanip>
#include <string>
#include <stdio.h>
#include <vector>

#include "command.h"
#include "cmd_dump.h"
#include "input.h"

#include "../src/pic-processor.h"
#include "../src/14bit-processors.h"
#include "../src/interface.h"
#include "../src/eeprom.h"

cmd_dump dump;

static cmd_options cmd_dump_options[] =
{
  {"e", cmd_dump::DUMP_EEPROM,    OPT_TT_BITFLAG},
  {"r", cmd_dump::DUMP_RAM,       OPT_TT_BITFLAG},
  {"s", cmd_dump::DUMP_SFRS,      OPT_TT_BITFLAG},
  {0,0,0}
};


cmd_dump::cmd_dump(void)
{ 
  name = "dump";
  abbreviation = "du";

  brief_doc = string("Display either the RAM or EEPROM");

  long_doc = string ("dump [r | e | s]\n"
    "\tdump r or dump with no options will display all of the file\n"
    "\t       registers and special function registers.\n"
    "\tdump e will display the contents of the eeprom (if the pic\n"
    "\t       being simulated contains any)\n"
    "\tdump s will display only the special function registers.\n");

  op = cmd_dump_options; 
}



#define REGISTERS_PER_ROW  16
#define SFR_COLUMNS         3
#define MAX_SFR_NAME       10

void cmd_dump::dump_sfrs(void)
{
  Processor * cpu = GetActiveCPU();
  unsigned int reg_size = cpu->register_size();
  unsigned int uToDisplayCount = 0;
  unsigned int uColumns = SFR_COLUMNS;
  vector<Register*> RegListToDisplay;
  unsigned int auTopRowIndex[SFR_COLUMNS];
  // Examine all registers this pic has to offer
  for (unsigned int i = 0; i < cpu->register_memory_size(); i++) {
    Register * pReg = cpu->registers[i];
    if(pReg->isa() == Register::SFR_REGISTER &&
      // Found an sfr. Display its contents only if not aliased
      // at some other address too.
      i == pReg->address) {
      uToDisplayCount++;
      RegListToDisplay.push_back(pReg);
    }
  }
  //
  //  All this is so we can have the reg number sequence go
  //  from top to bottom of each column instead of across columns.
  unsigned int uMod = uToDisplayCount % uColumns;
  unsigned int uRowsPerColumn = uToDisplayCount / uColumns;
  auTopRowIndex[0] = 0;
  for (unsigned int i = 1; i < sizeof(auTopRowIndex) / sizeof(unsigned int);
    i++) {
    auTopRowIndex[i] = auTopRowIndex[i-1] + uRowsPerColumn + (uMod > i ? 1 : 0);
  }
  uRowsPerColumn += (uMod == 0 ? 0 : 1);

  putchar('\n');
  unsigned int uRegCount = 0;
  for (unsigned int uRow = 0; uRow < uRowsPerColumn; uRow++) {
    for (unsigned int uColCurrent = 0; uColCurrent < uColumns; uColCurrent++) {
      unsigned int uIndex = auTopRowIndex[uColCurrent] + uRow;
      if(uRegCount > uToDisplayCount)
        break;
//      printf("%03d ", uIndex); // used for testings
      uRegCount++;
      Register *pReg = RegListToDisplay[uIndex];
      printf("%03x %-7s = %0*x   ", pReg->address,
        pReg->name().c_str(), reg_size * 2,
        pReg->get_value());
    }
    putchar('\n');
  }
}

void cmd_dump::dump(int mem_type)
{
  unsigned int i, j, reg_num,mem_size=0,reg_size=1,uRegPerRow = REGISTERS_PER_ROW;
  unsigned int v;
  bool 
    previous_row_is_invalid=false, 
    all_invalid=false;

  Register **fr=0;

  if(!have_cpu(1))
    return;

  switch(mem_type)
    {
    case DUMP_EEPROM:
      {
      pic_processor *pic = dynamic_cast<pic_processor *> (GetActiveCPU());
      if(pic && pic->eeprom) {
        fr = pic->eeprom->get_rom();
        mem_size = pic->eeprom->get_rom_size();
      } else
        return;
      }
      break;
    case DUMP_RAM:
      mem_size = GetActiveCPU()->register_memory_size();
      reg_size = GetActiveCPU()->register_size();
      uRegPerRow = reg_size == 1 ? REGISTERS_PER_ROW : 8;
      fr = GetActiveCPU()->registers;
      break;
    case DUMP_SFRS:
      dump_sfrs();
      putchar('\n');
      return;
    }

  if(mem_size == 0)
    return;

  gpsim_set_bulk_mode(1);
  

  if(reg_size == 1) {
    printf("     ");
    // Column labels
    for (i = 0; i < uRegPerRow; i++)
      printf(" %0*x", reg_size * 2, i);

    putchar('\n');
  }
  reg_num = 0;
  for (i = 0; i < mem_size; i+=uRegPerRow) {

    /* First, see if there are any valid registers on this row */
    all_invalid = true;
    for (j = 0; j < uRegPerRow; j++) {
      if(fr[i+j]->address) {
        all_invalid = false;
        break;
      }
    }
    if(!all_invalid) {
      previous_row_is_invalid = false;
    
      printf("%03x:  ",i);

      for (j = 0; j < uRegPerRow; j++)
      {
        reg_num = i  + j;

        if( (reg_num < mem_size) && ( reg_num || fr[reg_num]->address)) {

          v = fr[reg_num]->get_value();
          printf("%0*x ",reg_size * 2, v);
        } else {
          for (unsigned int i = 0; i < reg_size; i++)
            printf("--");
          putchar(' ');
        }
      }
      if(reg_size == 1) {
        // don't bother with ASCII for > 8 bit registers
        printf("   ");

        for (j = 0; j < uRegPerRow; j++)
        {
          reg_num = i + j;
          v = fr[reg_num]->get_value();
          if( (v >= ' ') && (v <= 'z'))
            putchar(v);
          else
            putchar('.');
        }
      }
      putchar('\n');
    } else {
      if(!previous_row_is_invalid)
        putchar('\n');
      previous_row_is_invalid = true;
      reg_num += REGISTERS_PER_ROW;
    }
  }

  // Now Dump the sfr's 

  if(mem_type == 2) {

    dump_sfrs();

    pic_processor *pic = dynamic_cast<pic_processor *>(GetActiveCPU());
    if(pic)
      printf("\n%s = %02x\n",pic->W->name().c_str(), pic->W->get_value());

    printf("pc = 0x%x\n",GetActiveCPU()->pc->value);
  }
  
  gpsim_set_bulk_mode(0);

}
