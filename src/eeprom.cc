/*
   Copyright (C) 1998-2003 Scott Dattalo
                 2003 Mike Durian

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

#include <assert.h>

#include <iostream>
#include <iomanip>
using namespace std;

#include <glib.h>

#include "trace.h"
#include "pic-processor.h"
#include "eeprom.h"
#include "pir.h"
#include "intcon.h"


// EEPROM - Peripheral
//
//  This object emulates the 14-bit core's EEPROM/FLASH peripheral
//  (such as the 16c84).
//
//  It's main purpose is to provide a means by which the control
//  registers may communicate.
// 

//------------------------------------------------------------------------
//
// EEPROM related registers


void EECON1::put(unsigned int new_value)
{


  new_value &= valid_bits;
  
  //cout << "EECON1::put new_value " << new_value << "  valid_bits " << valid_bits << '\n';
  if(new_value & WREN)
    {
      if(eeprom->get_reg_eecon2()->is_unarmed())
	{
          eeprom->get_reg_eecon2()->unready();
	  value |= WREN;

	}

      // WREN is true and EECON2 is armed (which means that we've passed through here
      // once before with WREN true). Initiate an eeprom write only if WR is true and
      // RD is false AND EECON2 is ready

      else if( (new_value & WR) && !(new_value & RD) &&
        (eeprom->get_reg_eecon2()->is_ready_for_write()))
	{
	  value |= WR;
	  eeprom->start_write();
	}

      //    else cout << "EECON1: write ignored " << new_value << "  (WREN is probably already set)\n";

    }
  else
    {
      // WREN is low so inhibit eeprom writes:

      eeprom->get_reg_eecon2()->unarm();
      //cout << "EECON1: write is disabled\n";

    }

  value = (value & (RD | WR)) | new_value;

  if ( (value & RD) && !( value & WR) )
    {
      if(value & EEPGD) {
        eeprom->get_reg_eecon2()->read();
	eeprom->start_program_memory_read();
	//cout << "eestate " << eeprom->eecon2->eestate << '\n';
	// read program memory
      } else {
	//eeprom->eedata->value = eeprom->rom[eeprom->eeadr->value]->get();
        eeprom->get_reg_eecon2()->read();
	eeprom->callback();
	value &= ~RD;
      }
    }
  

  trace.register_write(address,value);

}

unsigned int EECON1::get(void)
{

  trace.register_read(address,value);

  return(value);
}

EECON1::EECON1(void)
{
  new_name("eecon1");
  valid_bits = EECON1_VALID_BITS;
}

void EECON2::put(unsigned int new_value)
{


  if( (eestate == EENOT_READY) && (0x55 == new_value))
    {
      eestate = EEHAVE_0x55;
    }
  else if ( (eestate == EEHAVE_0x55) && (0xaa == new_value))
    {
      eestate = EEREADY_FOR_WRITE;
    }
  else if ((eestate == EEHAVE_0x55) || (eestate == EEREADY_FOR_WRITE))
    {
      eestate = EENOT_READY;
    }

  trace.register_write(address,new_value);

}

unsigned int EECON2::get(void)
{

  trace.register_read(address,0);

  return(0);
}

EECON2::EECON2(void)
{
  new_name("eecon2");
  break_point = 0;
}





unsigned int EEDATA::get(void)
{
  trace.register_read(address,value);

  return(value);
}

void EEDATA::put(unsigned int new_value)
{

  value = new_value;
  trace.register_write(address,value);

}

EEDATA::EEDATA(void)
{
  new_name("eedata");
  break_point = 0;
}




unsigned int EEADR::get(void)
{

  trace.register_read(address,value);

  return(value);
}

void EEADR::put(unsigned int new_value)
{

  value = new_value;
  trace.register_write(address,value);

}


EEADR::EEADR(void)
{
  new_name("eeadr");
  break_point = 0;
}



//------------------------------------------------------------------------
// Set the EEIF and clear the WR bits. 

void EEPROM_PIR::write_is_complete(void) 
{

  assert(pir_set != 0);

  eecon1.value = (eecon1.value  & (~eecon1.WR));

  pir_set->set_eeif();
}


void EEPROM_PIR::set_pir_set(PIR_SET *p)
{ 
  pir_set = p;
}



//----------------------------------------------------------
//
// EE PROM
//
// There are many conditions that need to be verified against a real part:
//    1) what happens if RD and WR are set at the same time?
//       > the simulator ignores both the read and the write.
//    2) what happens if a RD is initiated while data is being written?
//       > the simulator ignores the read
//    3) what happens if EEADR or EEDATA are changed while data is being written?
//       > the simulator will update these registers with the new values that
//         are written to them, however the write operation will be unaffected.
//    4) if WRERR is set, will this prevent a valid write sequence from being initiated?
//       > the simulator views WRERR as a status bit
//    5) if a RD is attempted after the eeprom has been prepared for a write
//       will this affect the RD or write?
//       > The simulator will proceed with the read and leave the write-enable state alone.
//    6) what happens if WREN goes low while a write is happening?
//       > The simulator will complete the write and WREN will be cleared.

EEPROM::EEPROM(void)
{

  rom_size = 0;
  name_str = 0;
  cpu = 0;
  intcon = 0;
}

Register *EEPROM::get_register(unsigned int address)
{

  if(address<rom_size)
    return rom[address];
  return NULL;

}


void EEPROM::start_write(void)
{

  cycles.set_break(cycles.value + EPROM_WRITE_TIME, this);

  wr_adr = eeadr.value;
  wr_data = eedata.value;

}

// Set the EEIF and clear the WR bits. 

void EEPROM::write_is_complete(void) 
{

  assert(intcon != 0);

  eecon1.value = (eecon1.value  & (~eecon1.WR)) | eecon1.EEIF;

  intcon->peripheral_interrupt();


}

void EEPROM::start_program_memory_read(void)
{

  cout << "ERROR: program memory flash should not be accessible\n";

  bp.halt();

}


void EEPROM::callback(void)
{

  switch(eecon2.get_eestate()) {
  case EECON2::EEREAD:
    //cout << "eeread\n";

    eecon2.unarm();
    eedata.value = rom[eeadr.value]->get();
    eecon1.value &= (~EECON1::RD);
    break;
  case EECON2::EEREADY_FOR_WRITE:
    //cout << "eewrite\n";

    if(wr_adr < rom_size)
      rom[wr_adr]->value = wr_data;
    else
      cout << "EEPROM wr_adr is out of range " << wr_adr << '\n';

    write_is_complete();

    if (eecon1.value & eecon1.WREN)
      eecon2.unready();
    else
      eecon2.unarm();
    break;

    eecon1.value &= (~EECON1::WR);
  default:
    cout << "EEPROM::callback() bad eeprom state " <<
      eecon2.get_eestate() << '\n';
  }
}


void EEPROM::reset(RESET_TYPE by)
{

  switch(by)
    {
    case POR_RESET:
      eecon1.value = 0;          // eedata & eeadr are undefined at power up
      eecon2.unarm();
    }

}

void EEPROM::initialize(unsigned int new_rom_size)
{

  rom_size = new_rom_size;

  // Let the control registers have a pointer to the peripheral in
  // which they belong.

  eecon1.set_eeprom(this);
  eecon2.set_eeprom(this);
  eedata.set_eeprom(this);
  eeadr.set_eeprom(this);

  // Create the rom

  rom = (Register **) new char[sizeof (Register *) * rom_size];
  assert(rom != 0);
  
  // Initialize the rom

  char str[100];
  for (int i = 0; i < rom_size; i++)
    {

      rom[i] = new Register;
      rom[i]->address = i;
      rom[i]->break_point = 0;
      rom[i]->value = 0;
      rom[i]->alias_mask = 0;

      sprintf (str, "eeprom reg 0x%02x", i);
      rom[i]->new_name(str);

    }

  //??? FIXME:
  reset(POR_RESET);


  assert(cpu != 0);
  cpu->ema.set_cpu(cpu);
  cpu->ema.set_Registers(rom, rom_size);
}


void EEPROM::set_intcon(INTCON *ic)
{ 
  intcon = ic; 
}

void EEPROM::dump(void)
{
  unsigned int i, j, reg_num,v;

  cout << "     " << hex;

  // Column labels
  for (i = 0; i < 16; i++)
    cout << setw(2) << setfill('0') <<  i << ' ';

  cout << '\n';

  for (i = 0; i < rom_size/16; i++)
    {
      cout << setw(2) << setfill('0') <<  i << ":  ";

      for (j = 0; j < 16; j++)
	{
	  reg_num = i * 16 + j;
	  if(reg_num < rom_size)
	    {
	      v = rom[reg_num]->get_value();
	      cout << setw(2) << setfill('0') <<  v << ' ';
	    }
	  else
	    cout << "-- ";
	}
      cout << "   ";

      for (j = 0; j < 16; j++)
	{
	  reg_num = i * 16 + j;
	  if(reg_num < rom_size)
	    {
	      v = rom[reg_num]->get_value();
	      if( (v >= ' ') && (v <= 'z'))
		cout.put(v);
	      else
		cout.put('.');
	    }
	}

      cout << '\n';

    }
}




//------------------------------------------------------------------------

void EEPROM_WIDE::start_write(void)
{

  cycles.set_break(cycles.value + EPROM_WRITE_TIME, this);

  wr_adr = eeadr.value + (eeadrh.value << 8);
  wr_data = eedata.value + (eedatah.value << 8);

}

void EEPROM_WIDE::start_program_memory_read(void)
{

  rd_adr = eeadr.value | (eeadrh.value << 8);

  cycles.set_break(cycles.value + 2, this);

}

void EEPROM_WIDE::callback(void)
{
  //cout << "eeprom call back\n";
  

  switch(eecon2.get_eestate()) {
  case EECON2::EEREAD:
    //cout << "eeread\n";

    eecon2.unarm();
    if(eecon1.value & EECON1::EEPGD) {
      // read program memory
      
      int opcode = cpu->pma.get_opcode(eeadr.value | (eeadrh.value << 8));
      //cout << "read " << i << " from program memory\n";
      eedata.value = opcode & 0xff;
      eedatah.value = (opcode>>8) & 0xff;

    } else {
      eedata.value = rom[eeadr.value]->get();
    }

    eecon1.value &= (~EECON1::RD);
    break;
  case EECON2::EEREADY_FOR_WRITE:
    //cout << "eewrite\n";

    if(wr_adr < rom_size)
      rom[wr_adr]->value = wr_data;
    else
      cout << "EEPROM wr_adr is out of range " << wr_adr << '\n';

    write_is_complete();

    if (eecon1.value & eecon1.WREN)
      eecon2.unready();
    else
      eecon2.unarm();
    break;

    eecon1.value &= (~EECON1::WR);
  default:
    cout << "EEPROM::callback() bad eeprom state " << eecon2.get_eestate() << '\n';
  }
}

void EEPROM_WIDE::initialize(unsigned int new_rom_size)
{

  eedatah.set_eeprom(this);
  eeadrh.set_eeprom(this);



  EEPROM::initialize(new_rom_size);


}
