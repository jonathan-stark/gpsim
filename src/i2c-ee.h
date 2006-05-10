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

#ifndef I2C_EE_H
#define I2C_EE_H

#include "trigger.h"
#include "gpsim_classes.h"

class Register;
class pic_processor;
class I2C_EE;
class I2C_EE_SCL;
class I2C_EE_SDA;
class Stimulus_Node;

//------------------------------------------------------------------------
//------------------------------------------------------------------------

const int I2C_EE_WRITE_TIME = 4000;

class I2C_EE :  public TriggerObject
{
public:

  I2C_EE(unsigned int _rom_size);
  virtual ~I2C_EE();
  void reset(RESET_TYPE);
  void debug();

  virtual void callback();
  virtual void callback_print();
  virtual void start_write();
  virtual void write_is_complete();

  virtual Register *get_register(unsigned int address);

  virtual void new_scl_edge ( bool direction );
  virtual void new_sda_edge ( bool direction );
  virtual void attach ( Stimulus_Node *_scl, Stimulus_Node *_sda );

  void dump();

protected:
  bool shift_read_bit ( bool x );
  bool shift_write_bit ();
  virtual bool processCommand(unsigned int cmd);

  Register **rom;          //  The data area.
  unsigned int rom_size;
  unsigned int xfr_addr,xfr_data;  // latched adr and data from I2C.
  unsigned int bit_count;  // Current bit number for either Tx or Rx
  unsigned int m_command;  // Most recent command received from I2C host
  bool ee_busy;            // true if a write is in progress.
  bool nxtbit;

  I2C_EE_SCL * scl;        // I2C clock
  I2C_EE_SDA * sda;        // I2C data

  enum {
    IDLE=0,
    START,
    RX_CMD,
    ACK_CMD,
    RX_ADDR,
    ACK_ADDR,
    RX_DATA,
    ACK_WR,
    WRPEND,
    ACK_RD,
    TX_DATA
  } bus_state;

private:
  // Is this even used?
  virtual void change_rom(unsigned int offset, unsigned int val);
  inline virtual unsigned int get_rom_size() { return (rom_size); }
  // XXX might want to make get_rom a friend only to cli_dump
  inline virtual Register **get_rom() { return (rom); }


};




#endif /* I2C_EE_H */
