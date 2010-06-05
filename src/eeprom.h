/*
   Copyright (C) 1998-2003 Scott Dattalo
                 2003 Mike Durian

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

#ifndef EEPROM_H
#define EEPROM_H

#include <assert.h>

#include "gpsim_classes.h"
#include "registers.h"
#include "breakpoints.h"

class pic_processor;
class EEPROM;
class PIR_SET;
class INTCON;
class PIR;

//---------------------------------------------------------
// EECON1 - EE control register 1
//

class EECON1 : public sfr_register
{
public:
enum
{

 RD    = (1<<0),
 WR    = (1<<1),
 WREN  = (1<<2),
 WRERR = (1<<3),
 EEIF  = (1<<4),
 EEPGD = (1<<7)
};


  EECON1(Processor *pCpu, const char *pName, const char *pDesc);

  void put(unsigned int new_value);
  unsigned int get();

  inline void set_eeprom(EEPROM *ee) { eeprom = ee; }
  inline void set_valid_bits(unsigned int vb) { valid_bits = valid_bits; }
  inline unsigned int get_valid_bits() { return (valid_bits); }
  inline void set_bits(unsigned int b) { valid_bits |= b; }
  inline void clear_bits(unsigned int b) { valid_bits &= ~b; }


  //private:
  unsigned int valid_bits;
  EEPROM *eeprom;
};

const unsigned int EECON1_VALID_BITS = (EECON1::RD | EECON1::WR |
  EECON1::WREN | EECON1::EEIF);

//
// EECON2 - EE control register 2
//

class EECON2 : public sfr_register
{
public:

enum EE_STATES
{
  EENOT_READY,
  EEHAVE_0x55,
  EEREADY_FOR_WRITE,
  EEWRITE_IN_PROGRESS,
  EEUNARMED,
  EEREAD
};

  EECON2(Processor *pCpu, const char *pName, const char *pDesc);

  void put(unsigned int new_value);
  unsigned int get();
  void ee_reset() { eestate = EENOT_READY;};

  inline virtual void set_eeprom(EEPROM *ee) { eeprom = ee; }
  inline enum EE_STATES get_eestate() { return (eestate); }
  inline void unarm() { eestate = EEUNARMED; }
  inline void unready() { eestate = EENOT_READY; }
  inline void read() { eestate = EEREAD; }
  inline void start_write() { eestate = EEWRITE_IN_PROGRESS; }

  inline bool is_unarmed() { return (eestate == EEUNARMED); }
  inline bool is_not_ready() { return (eestate == EENOT_READY); }
  inline bool is_ready_for_write() {
    return (eestate == EEREADY_FOR_WRITE);
  }
  inline bool is_writing() { return (eestate == EEWRITE_IN_PROGRESS); }

  //private:
  EEPROM *eeprom;
  enum EE_STATES eestate;
};

//
// EEDATA - EE data register
//

class EEDATA : public sfr_register
{
public:

  EEDATA(Processor *pCpu, const char *pName, const char *pDesc);

  void put(unsigned int new_value);
  unsigned int get();
  virtual void set_eeprom(EEPROM *ee) { eeprom = ee; }

  //private:
  EEPROM *eeprom;

};

//
// EEADR - EE address register
//

class EEADR : public sfr_register
{
public:

  EEADR(Processor *pCpu, const char *pName, const char *pDesc);

  void put(unsigned int new_value);
  unsigned int get();

  virtual void set_eeprom(EEPROM *ee) { eeprom = ee; }

  //private:
  EEPROM *eeprom;
};


//------------------------------------------------------------------------
//------------------------------------------------------------------------

const int EPROM_WRITE_TIME = 20;

class EEPROM :  public TriggerObject
{
public:

  EEPROM(Processor *pCpu);
  ~EEPROM();
  void reset(RESET_TYPE);
  virtual void set_intcon(INTCON *ic);

  virtual void callback();
  virtual void callback_print(){ cout << " EEPROM\n";}
  virtual void start_write();
  virtual void write_is_complete();
  virtual void start_program_memory_read();  
  virtual void initialize(unsigned int new_rom_size);
  virtual Register *get_register(unsigned int address);
  virtual void save_state();

  inline virtual void change_rom(unsigned int offset, unsigned int val) {
    assert(offset < rom_size);
    rom[offset]->value.put(val);
  }

  inline int register_size() { return (rom_data_size); }
  inline void set_resister_size(int bytes) { rom_data_size = bytes; }
  inline virtual unsigned int get_rom_size() { return (rom_size); }
  // XXX might want to make get_rom a friend only to cli_dump
  inline virtual Register **get_rom() { return (rom); }

  inline virtual EECON1 *get_reg_eecon1() { return (&eecon1); }
  inline virtual EECON2 *get_reg_eecon2() { return (&eecon2); }
  inline virtual EEDATA *get_reg_eedata() { return (&eedata); }
  inline virtual EEADR *get_reg_eeadr() { return (&eeadr); }

  void dump();

  //protected:
  char *name_str;
  Processor *cpu;
  INTCON *intcon;

  EECON1 eecon1;            // The EEPROM consists of 4 control registers
  EECON2 eecon2;            // on the F84 and 6 on the F877
  EEDATA eedata;
  EEADR  eeadr;

  Register **rom;           //  and the data area.
  RegisterCollection *m_UiAccessOfRom; // User access to the rom.

  int rom_data_size;		// data width in bytes
  unsigned int rom_size;
  unsigned int wr_adr,wr_data;  // latched adr and data for eewrites.
  unsigned int rd_adr;          // latched adr for eereads.
  unsigned int abp;             // break point number that's set during eewrites

};

class EEPROM_PIR : public EEPROM
{
public:

  EEPROM_PIR(Processor *pCpu, PIR *);
  ~EEPROM_PIR();

  // the 16f628 eeprom is identical to the 16f84 eeprom except
  // for the size and the location of EEIF. The size is taken
  // care of when the '628 is constructed, the EEIF is taken
  // care of here:

  virtual void write_is_complete();

protected:
  PIR *m_pir;


};


class EEPROM_WIDE : public EEPROM_PIR
{
public:
  EEPROM_WIDE(Processor *pCpu, PIR *);
  ~EEPROM_WIDE();

  virtual void start_write();
  virtual void callback();
  virtual void callback_print(){ cout << " EEPROM_WIDE\n";}
  virtual void start_program_memory_read();
  virtual void initialize(unsigned int new_rom_size);

  inline virtual EEDATA *get_reg_eedatah() { return (&eedatah); }
  inline virtual EEADR *get_reg_eeadrh() { return (&eeadrh); }

  //protected:
  EEDATA eedatah;
  EEADR  eeadrh;
};


#endif /* EEPROM_H */
