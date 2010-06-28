/*
   Copyright (C) 1998-2003 Scott Dattalo
                 2004 Rob Pearce
                 2006   Roy R Rankin

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
#include "stimuli.h"
#include "i2c-ee.h"
#include "registers.h"

//#define DEBUG
#if defined(DEBUG)
#include "../config.h"
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

#define Vprintf(arg) { if (verbose) {printf("%s:%d ",__FILE__,__LINE__); printf arg;} }

// I2C EEPROM Peripheral
//
//  This object emulates the I2C EEPROM peripheral on the 12CE51x
//
//  It's main purpose is to provide a means by which the port pins
//  may communicate.
//


//--------------------------------------------------------------
//
//
class I2C_EE_PIN : public IO_open_collector
{
public:

  I2C_EE *eeprom;

  I2C_EE_PIN (I2C_EE *_eeprom, const char *_name)
    : IO_open_collector(_name)
  {

    eeprom = _eeprom;

    bDrivingState = true;
    bDrivenState = true;

    // Make the pin an output.
    update_direction(IO_bi_directional::DIR_OUTPUT,true);

  };

  //
  void setDrivingState(bool new_state) {

    bDrivingState = new_state;
    bDrivenState = new_state;

    if(snode)
      snode->update();

  }

};


class I2C_EE_SDA : public I2C_EE_PIN
{
public:

  I2C_EE_SDA (I2C_EE *_eeprom, const char *_name)
    : I2C_EE_PIN (_eeprom,_name)
  {
  }

  void setDrivenState(bool new_dstate) {

    bool diff = new_dstate ^ bDrivenState;

    Dprintf(("eeprom sda setDrivenState %d\n", new_dstate));
    if( eeprom && diff ) {
      bDrivenState = new_dstate;
      eeprom->new_sda_edge(new_dstate);
    }

  }

};
class I2C_EE_SCL : public I2C_EE_PIN
{
public:

  I2C_EE_SCL (I2C_EE *_eeprom, const char *_name)
    : I2C_EE_PIN (_eeprom,_name)
  {
  }

  void setDrivenState(bool new_state) {

    bool diff = new_state ^ bDrivenState;

    Dprintf(("eeprom scl setDrivenState %d\n", new_state));
    if( eeprom && diff ) {
      bDrivenState = new_state;
      eeprom->new_scl_edge(bDrivenState);
    }

  }

};

PromAddress::PromAddress(I2C_EE *eeprom, const char *_name, const char * desc)
    : Value(_name,desc)
{
     m_eeprom = eeprom;
}
void PromAddress::get(char *buffer, int buf_size)
{
        snprintf(buffer, buf_size, "%p", m_eeprom);
}

//----------------------------------------------------------
//
// I2C EE PROM
//
// There are many conditions that need to be verified against a real part:
//    1) what happens if
//       > the simulator
//    2) what happens if a RD is initiated while data is being written?
//       > the simulator ignores the read
//    3) what happens if
//       > the simulator

I2C_EE::I2C_EE(Processor *pCpu, unsigned int _rom_size, unsigned int _write_page_size,
        unsigned int _addr_bytes, unsigned int _CSmask,
        unsigned int _BSmask, unsigned int _BSshift)
  : rom(0),
    rom_size(_rom_size),                // size of eeprom in bytes
    rom_data_size(1),
    xfr_addr(0), xfr_data(0),
    write_page_off(0),
    write_page_size(_write_page_size),  // Page size for writes
    bit_count(0), m_command(0),
    m_chipselect(0),
    m_CSmask(_CSmask),                  // mask for chip select in command
    m_BSmask(_BSmask),                  // mask for bank select in command
    m_BSshift(_BSshift),                // right shift bank select to bit 0
    m_addr_bytes(_addr_bytes),          // number of address bytes
     m_write_protect(false),
    ee_busy(false),
    bus_state(IDLE)
{

  // Create the rom

  rom = (Register **) new char[sizeof (Register *) * rom_size];
  assert(rom != 0);

  // Initialize the rom

  char str[100];
  for (unsigned int i = 0; i < rom_size; i++) {
    snprintf (str,sizeof(str),"ee0x%02x", i);
    rom[i] = new Register(pCpu,str,"");
    rom[i]->address = i;
    rom[i]->value.put(0);
    rom[i]->alias_mask = 0;
  }

  if (pCpu)
  {
        m_UiAccessOfRom = new RegisterCollection(pCpu,
                                             "eeData",
                                             rom,
                                             rom_size);

  }
  else
        m_UiAccessOfRom = NULL;

  scl = new I2C_EE_SCL ( this, "SCL" );
  sda = new I2C_EE_SDA ( this, "SDA" );
}

I2C_EE::~I2C_EE()
{
  for (unsigned int i = 0; i < rom_size; i++)
        delete rom[i];
  delete [] rom;

  if (m_UiAccessOfRom)
        delete m_UiAccessOfRom;
}

// Bit 0 is write protect, 1-3 is A0 - A2
void I2C_EE::set_chipselect(unsigned int _cs)
{
    m_write_protect = (_cs & 1) == 1;
    m_chipselect = (_cs & m_CSmask);
}

void I2C_EE::debug()
{

  if (!scl || !sda || !rom)
    return;

  const char *cPBusState=0;
  switch (bus_state) {
  case IDLE:
    cPBusState = "IDLE";
    break;
  case START:
    cPBusState = "START";
    break;
  case RX_CMD:
    cPBusState = "RX_CMD";
    break;
  case ACK_CMD:
    cPBusState = "ACK_CMD";
    break;
  case RX_ADDR:
    cPBusState = "RX_ADDR";
    break;
  case RX_ADDR2:
    cPBusState = "RX_ADDR2";
    break;
  case ACK_ADDR:
    cPBusState = "ACK_ADDR";
    break;
  case RX_DATA:
    cPBusState = "RX_DATA";
    break;
  case ACK_WR:
    cPBusState = "ACK_WR";
    break;
  case WRPEND:
    cPBusState = "WRPEND";
    break;
  case ACK_RD:
    cPBusState = "ACK_RD";
    break;
  case TX_DATA:
    cPBusState = "TX_DATA";
    break;
  }

  cout << "I2C EEPROM: current state="<<cPBusState<<endl;
  cout << " t=0x"<< hex <<get_cycles().get() << endl;
  cout << "  scl drivenState="  << scl->getDrivenState()
       << " drivingState=" << scl->getDrivingState()
       << " direction=" << ((scl->get_direction()==IOPIN::DIR_INPUT) ?"IN":"OUT")
       << endl;
  cout << "  sda drivenState="  << sda->getDrivenState()
       << " drivingState=" << sda->getDrivingState()
       << " direction=" << ((sda->get_direction()==IOPIN::DIR_INPUT) ?"IN":"OUT")
       << endl;

  cout << "  bit_count:"<<bit_count
       << " ee_busy:"<<ee_busy
       << " xfr_addr:0x"<<hex<<xfr_addr
       << " xfr_data:0x"<<hex<<xfr_data
       << endl;

}
Register * I2C_EE::get_register(unsigned int address)
{
  if ( address < rom_size )
    return rom[address];
  return 0;

}

void I2C_EE::change_rom(unsigned int offset, unsigned int val)
{
  assert(offset < rom_size);
  rom[offset]->value.put(val);
}

// write data to eeprom unles write protect is active
void I2C_EE::start_write()
{
    unsigned int addr = xfr_addr + write_page_off;
    if (! m_write_protect)
    {
        rom[addr]->put ( xfr_data );
    }
     else
        cout << "I2c_EE start_write- write protect\n";
}

// allow 5 msec after last write
void I2C_EE::write_busy()
{
    guint64 fc;


    if (! ee_busy && ! m_write_protect)
    {
        fc = (guint64)(get_cycles().instruction_cps() * 0.005);
        get_cycles().set_break(get_cycles().get() + fc, this);
        ee_busy = true;
    }


}


void I2C_EE::write_is_complete()
{
}



void I2C_EE::callback()
{

  ee_busy = false;
  Vprintf(("I2C_EE::callback() - write cycle is complete\n"));
}

void I2C_EE::callback_print()
{
  cout << "Internal I2C-EEPROM\n";
}

bool I2C_EE::shift_read_bit ( bool x )
{
    xfr_data = ( xfr_data << 1 ) | ( x != 0 );
    bit_count++;
    if ( bit_count == 8 )
        return true;
    else
        return false;
}


bool I2C_EE::shift_write_bit ()
{
    bool bit;

    bit_count--;
    bit = ( xfr_data >> bit_count ) & 1;
    Dprintf(("I2C_EE : send bit %d = %d\n",bit_count, bit));
    return bit;
}

bool I2C_EE::processCommand(unsigned int command)
{
  if ((command & 0xf0) == 0xa0 && ((command & m_CSmask) == m_chipselect)) {
    m_command = command;
    return true;
  }
  return false;
}

void I2C_EE::new_scl_edge ( bool direction )
{
  int curBusState = bus_state;
  if (verbose) {
    Vprintf(("I2C_EE::new_scl_edge: %d\n",direction));
    debug();
  }
    if ( direction )
    {
        // Rising edge
        nxtbit = sda->getDrivenState();
        Dprintf(("I2C_EE SCL : Rising edge, data=%d\n",nxtbit));
    }
    else
    {
        // Falling edge
        //cout << "I2C_EE SCL : Falling edge\n";
        switch ( bus_state )
        {
            case IDLE :
                sda->setDrivingState ( true );
                break;

            case START :
                sda->setDrivingState ( true );
                bus_state = RX_CMD;
                break;

            case RX_CMD :
                if ( shift_read_bit ( sda->getDrivenState() ) )
                {
                    Vprintf(("I2C_EE : got command:0x%x\n", xfr_data));
                    //if ( ( xfr_data & 0xf0 ) == 0xA0 )
                    if (processCommand(xfr_data))
                    {
                        bus_state = ACK_CMD;
                        Vprintf((" - OK\n"));
                        // Acknowledge the command by pulling SDA low.
                        sda->setDrivingState ( false );
                    }
                    else
                    {
                        // not for us
                        bus_state = IDLE;
                        Vprintf((" - not for us\n"));
                    }
                }
                break;

            case ACK_CMD :
                sda->setDrivingState ( true );
                // Check the R/W bit of the command
                if ( m_command & 0x01 )
                {
                    // it's a read command
                    bus_state = TX_DATA;
                    bit_count = 8;
                    xfr_addr += write_page_off;
                    write_page_off = 0;
                    xfr_data = rom[xfr_addr]->get();
                    sda->setDrivingState ( shift_write_bit() );
                }
                else
                {
                    // it's a write command
                    bus_state = RX_ADDR;
                    bit_count = 0;
                    xfr_data = 0;
                    xfr_addr = (m_command & m_BSmask) >> m_BSshift;
                    m_addr_cnt = m_addr_bytes;
                }
                break;

            case RX_ADDR :
                if ( shift_read_bit ( sda->getDrivenState() ) )
                {
                    sda->setDrivingState ( false );
                    // convert xfr_data to base and page offset to allow
                    // sequencel writes to wrap around page
                    xfr_addr = ((xfr_addr << 8) | xfr_data ) % rom_size;
                    bus_state = ACK_ADDR;
                    Vprintf(("I2C_EE : address set to 0x%x data:0x%x\n", xfr_addr,xfr_data));
                }
                break;

            case ACK_ADDR :
                sda->setDrivingState ( true );
                if (--m_addr_cnt)
                    bus_state = RX_ADDR;
                else
                {
                    write_page_off = xfr_addr % write_page_size;
                    xfr_addr -= write_page_off;
                    Vprintf(("I2C_EE : address set to 0x%x page offset 0x%x data:0x%x\n",
                        xfr_addr, write_page_off ,xfr_data));
                    bus_state = RX_DATA;
                }
                bit_count = 0;
                xfr_data = 0;
                break;

            case RX_DATA :
                if ( shift_read_bit ( sda->getDrivenState() ) )
                {
                    start_write();
                    Vprintf(("I2C_EE : data set to 0x%x\n", xfr_data));
                    sda->setDrivingState ( false );
                    bus_state = ACK_WR;
                    write_page_off = ++write_page_off % write_page_size;
                }
                break;

            case ACK_WR :
                sda->setDrivingState ( true );
                bus_state = WRPEND;
                break;

            case WRPEND :
                // We were about to do the write but got more data instead
                // of the expected stop bit
                xfr_data = sda->getDrivenState();
                bit_count = 1;
                bus_state = RX_DATA;
                Vprintf(("I2C_EE : write postponed by extra data\n"));
                break;

            case TX_DATA :
                if ( bit_count == 0 )
                {
                    sda->setDrivingState ( true );     // Release the bus
                    xfr_addr++;
                    xfr_addr %= rom_size;
                    bus_state = ACK_RD;
                }
                else
                {
                    sda->setDrivingState ( shift_write_bit() );
                }
                break;

            case ACK_RD :
                if ( sda->getDrivenState() == false )
                {
                    // The master has asserted ACK, so we send another byte
                    bus_state = TX_DATA;
                    bit_count = 8;
                    xfr_data = rom[xfr_addr]->get();
                    sda->setDrivingState ( shift_write_bit() );
                }
                else
                {
                    bus_state = IDLE;   // Actually a limbo state
                }
                break;

            default :
                sda->setDrivingState ( true );     // Release the bus
                break;
        }
    }
    if ((bool)verbose && bus_state != curBusState) {
      Vprintf(("I2C_EE::new_scl_edge() new bus state = %d\n",bus_state));
      debug();
    }
}


void I2C_EE::new_sda_edge ( bool direction )
{
  if (verbose) {
    Vprintf(("I2C_EE::new_sda_edge: direction:%d\n",direction));
    debug();
  }

    if ( scl->getDrivenState() )
    {
        int curBusState = bus_state;
        if ( direction )
        {
            // stop bit
            Vprintf(("I2C_EE SDA : Rising edge in SCL high => stop bit\n"));
            if ( bus_state == WRPEND )
            {
                Vprintf(("I2C_EE : write is pending - commence...\n"));
                write_busy();
                bus_state = IDLE;   // Should be busy
            }
            else
                bus_state = IDLE;
        }
        else
        {
            // start bit
            Vprintf(("I2C_EE SDA : Falling edge in SCL high => start bit\n"));
            if ( ee_busy )
            {
              Vprintf(("             Device is busy - ignoring start bit\n"));
            }
            else
            {
                bus_state = START;
                bit_count = 0;
                xfr_data = 0;
            }
        }

        if ((bool)verbose && bus_state != curBusState) {
          Vprintf(("I2C_EE::new_sda_edge() new bus state = %d\n",bus_state));
          debug();
        }

    }
}


void I2C_EE::reset(RESET_TYPE by)
{

  switch(by)
    {
    case POR_RESET:
        bus_state = IDLE;
        ee_busy = false;
        break;
    default:
      break;
    }

}


void I2C_EE::attach ( Stimulus_Node *_scl, Stimulus_Node *_sda )
{
  _scl->attach_stimulus ( scl );
  _sda->attach_stimulus ( sda );
}


void I2C_EE::dump()
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
