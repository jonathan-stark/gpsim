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

#ifndef EEPROM_H
#define EEPROM_H

#include <assert.h>

#include "pic-registers.h"


class pic_processor;
class EEPROM;
class PIR_SET;
class INTCON;

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


  EECON1(void);

  void put(unsigned int new_value);
  unsigned int get(void);

  inline void set_eeprom(EEPROM *ee) { eeprom = ee; }
  inline void set_valid_bits(unsigned int vb) { valid_bits = valid_bits; }
  inline unsigned int get_valid_bits(void) { return (valid_bits); }
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
  EEUNARMED,
  EEREAD
};

  EECON2(void);

  void put(unsigned int new_value);
  unsigned int get(void);
  void ee_reset(void) { eestate = EENOT_READY;};

  inline virtual void set_eeprom(EEPROM *ee) { eeprom = ee; }
  inline enum EE_STATES get_eestate(void) { return (eestate); }
  inline void unarm(void) { eestate = EEUNARMED; }
  inline void unready(void) { eestate = EENOT_READY; }
  inline void read(void) { eestate = EEREAD; }

  inline bool is_unarmed(void) { return (eestate == EEUNARMED); }
  inline bool is_not_ready(void) { return (eestate == EENOT_READY); }
  inline bool is_ready_for_write(void) {
    return (eestate == EEREADY_FOR_WRITE);
  }

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

  EEDATA(void);

  void put(unsigned int new_value);
  unsigned int get(void);
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

  EEADR(void);

  void put(unsigned int new_value);
  unsigned int get(void);

  virtual void set_eeprom(EEPROM *ee) { eeprom = ee; }

  //private:
  EEPROM *eeprom;
};


//------------------------------------------------------------------------
//------------------------------------------------------------------------

const int EPROM_WRITE_TIME = 20;

class EEPROM :  public BreakCallBack
{
public:

  EEPROM(void);
  void reset(RESET_TYPE);
  virtual void set_cpu(pic_processor *p) { cpu = p; }
  virtual void set_intcon(INTCON *ic);

  virtual void callback(void);
  virtual void start_write(void);
  virtual void write_is_complete(void);
  virtual void start_program_memory_read(void);  
  virtual void initialize(unsigned int new_rom_size);
  virtual Register *get_register(unsigned int address);

  inline virtual void change_rom(unsigned int offset, unsigned int val) {
    assert(offset < rom_size);
    rom[offset]->value = val;
  }

  inline virtual unsigned int get_rom_size(void) { return (rom_size); }
  // XXX might want to make get_rom a friend only to cli_dump
  inline virtual Register **get_rom(void) { return (rom); }

  inline virtual EECON1 *get_reg_eecon1(void) { return (&eecon1); }
  inline virtual EECON2 *get_reg_eecon2(void) { return (&eecon2); }
  inline virtual EEDATA *get_reg_eedata(void) { return (&eedata); }
  inline virtual EEADR *get_reg_eeadr(void) { return (&eeadr); }

  void dump(void);

  //protected:
  char *name_str;
  pic_processor *cpu;
  INTCON *intcon;

  EECON1 eecon1;            // The EEPROM consists of 4 control registers
  EECON2 eecon2;            // on the F84 and 6 on the F877
  EEDATA eedata;
  EEADR  eeadr;

  Register **rom;          //  and the data area.
  unsigned int rom_size;
  unsigned int wr_adr,wr_data;  // latched adr and data for eewrites.
  unsigned int rd_adr;          // latched adr for eereads.
  unsigned int abp;             // break point number that's set during eewrites

};

class EEPROM_PIR : public EEPROM
{
public:

  virtual void set_pir_set(PIR_SET *p);

  // the 16f628 eeprom is identical to the 16f84 eeprom except
  // for the size and the location of EEIF. The size is taken
  // care of when the '628 is constructed, the EEIF is taken
  // care of here:

  virtual void write_is_complete(void);

  //protected:
  PIR_SET *pir_set;


};


class EEPROM_WIDE : public EEPROM
{
public:

  virtual void start_write(void);
  virtual void callback(void);
  virtual void start_program_memory_read(void);
  virtual void initialize(unsigned int new_rom_size);

  inline virtual EEDATA *get_reg_eedatah(void) { return (&eedatah); }
  inline virtual EEADR *get_reg_eeadrh(void) { return (&eeadrh); }

  //protected:
  EEDATA eedatah;
  EEADR  eeadrh;
};


#endif /* EEPROM_H */
