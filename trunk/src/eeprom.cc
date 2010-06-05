/*
   Copyright (C) 1998-2003 Scott Dattalo
                 2003 Mike Durian
                 2006 Roy Rankin

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

#include <assert.h>

#include <iostream>
#include <iomanip>
using namespace std;

#include <glib.h>

#include "../config.h"

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


  trace.raw(write_trace.get() | value.get());

  new_value &= valid_bits;

  //cout << "EECON1::put new_value " << new_value << "  valid_bits " << valid_bits << '\n';
  if(new_value & WREN)
    {
      if(eeprom->get_reg_eecon2()->is_unarmed())
        {
          eeprom->get_reg_eecon2()->unready();
          value.put(value.get() | WREN);

        }

      // WREN is true and EECON2 is armed (which means that we've passed through here
      // once before with WREN true). Initiate an eeprom write only if WR is true and
      // RD is false AND EECON2 is ready

      else if( (new_value & WR) && !(new_value & RD) &&
        (eeprom->get_reg_eecon2()->is_ready_for_write()))
        {
          value.put(value.get() | WR);
          eeprom->start_write();
        }
      else if( (new_value & WR) && (new_value & RD))
	{
         cout << "\n*** EECON1: write ignored " <<hex << new_value << " both WR & RD set\n\n";
	}

      //    else cout << "EECON1: write ignored " << new_value << "  (WREN is probably already set)\n";

    }
  else
    {
      // WREN is low so inhibit further eeprom writes:

      if ( ! eeprom->get_reg_eecon2()->is_writing() )
        {
          eeprom->get_reg_eecon2()->unarm();
        }
      //cout << "EECON1: write is disabled\n";

    }

  value.put((value.get() & (RD | WR)) | new_value);

  if ( (value.get() & RD) && !( value.get() & WR) )
    {
      if(value.get() & EEPGD) {
        eeprom->get_reg_eecon2()->read();
        eeprom->start_program_memory_read();
        //cout << "eestate " << eeprom->eecon2->eestate << '\n';
        // read program memory
      } else {
        //eeprom->eedata->value = eeprom->rom[eeprom->eeadr->value]->get();
        eeprom->get_reg_eecon2()->read();
        eeprom->callback();
        value.put(value.get() & ~RD);
      }
    }

}

unsigned int EECON1::get()
{

  trace.raw(read_trace.get() | value.get());
  //trace.register_read(address,value.get());

  return(value.get());
}

EECON1::EECON1(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu,pName,pDesc)
{
  valid_bits = EECON1_VALID_BITS;
}

void EECON2::put(unsigned int new_value)
{


  trace.raw(write_trace.get() | value.get());
  value.put(new_value);

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


}

unsigned int EECON2::get()
{

  trace.raw(read_trace.get() | value.get());

  return(0);
}

EECON2::EECON2(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu,pName,pDesc)
{
  ee_reset();
}

unsigned int EEDATA::get()
{
  trace.raw(read_trace.get() | value.get());
  return(value.get());
}

void EEDATA::put(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  value.put(new_value);

}

EEDATA::EEDATA(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu,pName,pDesc)
{
}

unsigned int EEADR::get()
{

  trace.raw(read_trace.get() | value.get());
  //trace.register_read(address,value.get());

  return(value.get());
}

void EEADR::put(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  value.put(new_value);

}


EEADR::EEADR(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu,pName,pDesc)
{
}


//------------------------------------------------------------------------
EEPROM_PIR::EEPROM_PIR(Processor *pCpu, PIR *pPir)
  : EEPROM(pCpu),m_pir(pPir)
{
}

EEPROM_PIR::~EEPROM_PIR()
{
}

//------------------------------------------------------------------------
// Set the EEIF and clear the WR bits.

void EEPROM_PIR::write_is_complete()
{

  assert(m_pir != 0);

  eecon1.value.put( eecon1.value.get()  & (~eecon1.WR));

  m_pir->set_eeif();
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

EEPROM::EEPROM(Processor *pCpu)
  : name_str(0),
    cpu(pCpu),
    intcon(0),
    eecon1(pCpu,"eecon1","EE Control 1"),
    eecon2(pCpu,"eecon2","EE Control 2"),
    eedata(pCpu,"eedata","EE Data"),
    eeadr(pCpu,"eeadr", "EE Address"),
    rom(0),
    m_UiAccessOfRom(0),
    rom_data_size(1),
    rom_size(0)
{
}
EEPROM::~EEPROM()
{
  pic_processor *pCpu = dynamic_cast<pic_processor *>(cpu);
  if (pCpu) {
    pCpu->remove_sfr_register(&eedata);
    pCpu->remove_sfr_register(&eeadr);
    pCpu->remove_sfr_register(&eecon1);
    pCpu->remove_sfr_register(&eecon2);
  }

  for (unsigned int i = 0; i < rom_size; i++)
    delete rom[i];

  delete [] rom;

  delete m_UiAccessOfRom;
}

Register *EEPROM::get_register(unsigned int address)
{

  if(address<rom_size)
    return rom[address];
  return 0;

}


void EEPROM::start_write()
{

  get_cycles().set_break(get_cycles().get() + EPROM_WRITE_TIME, this);

  wr_adr = eeadr.value.get();
  wr_data = eedata.value.get();

  eecon2.start_write();
}

// Set the EEIF and clear the WR bits.

void EEPROM::write_is_complete()
{

  assert(intcon != 0);

  eecon1.value.put((eecon1.value.get()  & (~eecon1.WR)) | eecon1.EEIF);

  intcon->peripheral_interrupt();


}

void EEPROM::start_program_memory_read()
{

  cout << "ERROR: program memory flash should not be accessible\n";

  bp.halt();

}


void EEPROM::callback()
{

  switch(eecon2.get_eestate()) {
  case EECON2::EEREAD:
    //cout << "eeread\n";

    eecon2.unarm();
    if (eeadr.value.get() < rom_size)
        eedata.value.put(rom[eeadr.value.get()]->get());
    else
    {
	cout << "EEPROM read address is out of range " << hex << eeadr.value.get() << endl;
	bp.halt();
    }
    eecon1.value.put(eecon1.value.get() & (~EECON1::RD));
    break;
  case EECON2::EEWRITE_IN_PROGRESS:
    //cout << "eewrite\n";

    if(wr_adr < rom_size)
      rom[wr_adr]->value.put(wr_data);
    else
    {
      cout << "EEPROM write address is out of range " << hex << wr_adr << '\n';
      bp.halt();
    }

    write_is_complete();

    if (eecon1.value.get() & eecon1.WREN)
      eecon2.unready();
    else
      eecon2.unarm();
    break;

    eecon1.value.put(eecon1.value.get() & (~EECON1::WR));
  default:
    cout << "EEPROM::callback() bad eeprom state " <<
      eecon2.get_eestate() << '\n';
    bp.halt();
  }
}


void EEPROM::reset(RESET_TYPE by)
{

  switch(by)
    {
    case POR_RESET:
      eecon1.value.put(0);          // eedata & eeadr are undefined at power up
      eecon2.unarm();
      break;
    default:
      break;
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
  for (unsigned int i = 0; i < rom_size; i++) {

    snprintf (str, sizeof(str), "eereg 0x%02x", i);
    rom[i] = new Register(cpu,str);
    rom[i]->address = i;
    rom[i]->value.put(0);
    rom[i]->alias_mask = 0;
  }

  if(cpu) {
    //cpu->ema.set_cpu(cpu);
    cpu->ema.set_Registers(rom, rom_size);
    m_UiAccessOfRom = new RegisterCollection(cpu,
                                             "eeData",
                                             rom,
                                             rom_size);
  }

}

//----------------------------------------
// Save the current state of the eeprom. This is used to reconstitute
// the trace buffer.

void EEPROM::save_state()
{

  if(!rom || !rom_size)
    return;

  for (unsigned int i = 0; i < rom_size; i++)
    if(rom[i])
      rom[i]->put_trace_state(rom[i]->value);

}

void EEPROM::set_intcon(INTCON *ic)
{
  intcon = ic;
}

void EEPROM::dump()
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
EEPROM_WIDE::EEPROM_WIDE(Processor *pCpu, PIR *pPir)
  : EEPROM_PIR(pCpu,pPir),
    eedatah(pCpu,"eedatah", "EE Data High byte"),
    eeadrh(pCpu, "eeadr", "EE Address High byte")
{
}

EEPROM_WIDE::~EEPROM_WIDE()
{

}

void EEPROM_WIDE::start_write()
{

  get_cycles().set_break(get_cycles().get() + EPROM_WRITE_TIME, this);

  wr_adr = eeadr.value.get() + (eeadrh.value.get() << 8);
  wr_data = eedata.value.get() + (eedatah.value.get() << 8);

  eecon2.start_write();
}

void EEPROM_WIDE::start_program_memory_read()
{

  rd_adr = eeadr.value.get() | (eeadrh.value.get() << 8);

  get_cycles().set_break(get_cycles().get() + 2, this);

}

void EEPROM_WIDE::callback()
{
  //cout << "eeprom call back\n";


  switch(eecon2.get_eestate()) {
  case EECON2::EEREAD:
    //cout << "eeread\n";

    eecon2.unarm();
    if(eecon1.value.get() & EECON1::EEPGD) {
      // read program memory

      int opcode = cpu->pma->get_opcode(rd_adr);
      eedata.value.put(opcode & 0xff);
      eedatah.value.put((opcode>>8) & 0xff);

    } else {
      if (eeadr.value.get() < rom_size)
      	eedata.value.put(rom[eeadr.value.get()]->get());
      else
      {
        cout << "WIDE_EEPROM read adrress is out of range " << hex << eeadr.value.get() << '\n';
        bp.halt();
      }
    }

    eecon1.value.put(eecon1.value.get() & (~EECON1::RD));
    break;

  case EECON2::EEWRITE_IN_PROGRESS:
    //cout << "eewrite\n";

    if(eecon1.value.get() & EECON1::EEPGD) // write program memory
    {
        cpu->init_program_memory_at_index(wr_adr, wr_data);
    }
    else                                  // read eeprom memory
    {
        if(wr_adr < rom_size)
        {
           rom[wr_adr]->value.put(wr_data);
        }
        else
        {
           cout << "WIDE_EEPROM write address is out of range " << hex << wr_adr << '\n';
	   bp.halt();
        }
    }

    write_is_complete();

    if (eecon1.value.get() & eecon1.WREN)
      eecon2.unready();
    else
      eecon2.unarm();
    break;

  default:
    cout << "EEPROM_WIDE::callback() bad eeprom state " << eecon2.get_eestate() << '\n';
    bp.halt();
  }
}

void EEPROM_WIDE::initialize(unsigned int new_rom_size)
{

  eedatah.set_eeprom(this);
  eeadrh.set_eeprom(this);

  EEPROM::initialize(new_rom_size);
}
