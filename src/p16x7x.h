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
along with gpsim; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#ifndef __P16X7X_H__
#define __P16X7X_H__


#include "p16x6x.h"    /* The '7x stuff is like '6x stuff with a/d converters */

class IOPORT;

//---------------------------------------------------------
// ADRES
//

class ADRES : public sfr_register
{
public:

  //  unsigned int get(void) {return value;};
  void put(int new_value);
  //  void put(unsigned int new_value);

};


//---------------------------------------------------------
// ADCON1
//

class ADCON1 : public sfr_register
{
public:

  IOPORT *analog_port;

  unsigned int valid_bits;

enum PCFG_bits
{
  PCFG0 = 1<<0,
  PCFG1 = 1<<1,
  PCFG2 = 1<<2,
  PCFG3 = 1<<3,   // 16f87x
  ADFM  = 1<<7    // 16f87x
};

  /* Vrefhi/lo_position - this is an array that tells which
   * channel the A/D converter's  voltage reference(s) are located. 
   * The index into the array is formed from the PCFG bits. 
   * The encoding is as follows:
   *   0-7:  analog channel containing Vref(s)
   *     8:  The reference is internal (i.e. Vdd)
   *  0xff:  The analog inputs are configured as digital inputs
   */
  unsigned int Vrefhi_position[16];
  unsigned int Vreflo_position[16];

  /* configuration bits is an array of 8-bit masks definining whether or not
   * a given channel is analog or digital
   */
  unsigned int configuration_bits[16];

  //  unsigned int get(void) {return value;};
  int get_Vref(void);

};


//---------------------------------------------------------
// ADCON0
//

class ADCON0 : public sfr_register, public BreakCallBack
{
public:
  IOPORT *analog_port;
  IOPORT *analog_port2;
  ADRES *adres;
  ADRES *adresl;
  ADCON1 *adcon1;
  INTCON *intcon;

enum
{
  ADON = 1<<0,
  ADIF = 1<<1,
  GO   = 1<<2,
  CHS0 = 1<<3,
  CHS1 = 1<<4,
  ADCS0 = 1<<6,
  ADCS1 = 1<<7
};

enum AD_states
{
  AD_IDLE,
  AD_ACQUIRING,
  AD_CONVERTING
};

  unsigned int ad_state;
  unsigned int Tad_2;
  int acquired_value;
  int reference;
  unsigned int channel_mask;
  guint64 future_cycle;

  void start_conversion(void);
  void stop_conversion(void);
  virtual void set_interrupt(void);
  virtual void callback(void);
  void put(unsigned int new_value);
  void put_conversion(void);
  ADCON0(void)
  {
    ad_state = AD_IDLE;
  }

};

//---------------------------------------------------------
// ADCON0_withccp
//

class ADCON0_withccp : public ADCON0
{
public:

  PIR1   *pir;
  virtual void set_interrupt(void);

};


//---------------------------------------------------------

class P16C71 :  public P16C61
{
 public:

  ADCON0 adcon0;
  ADCON1 adcon1;
  ADRES  adres;

  virtual PROCESSOR_TYPE isa(void){return _P16C71_;};
  virtual void create_symbols(void);

  virtual unsigned int program_memory_size(void) const { return 0x400; };
  virtual void create_sfr_map(void);


  P16C71(void);
  void create(void);
  static pic_processor *construct(void);

};

class P16C72 : public P16C62 
{
 public:

  ADCON0_withccp adcon0;
  ADCON1 adcon1;
  ADRES  adres;

  virtual PROCESSOR_TYPE isa(void){return _P16C72_;};
  virtual void create_symbols(void);
  void create_sfr_map(void);

  //virtual unsigned int program_memory_size(void) const { return 0x800; };


  P16C72(void);
  void create(void);
  static pic_processor *construct(void);

};

class P16C73 : public P16C63
{
 public:

  ADCON0_withccp adcon0;
  ADCON1 adcon1;
  ADRES  adres;

  virtual PROCESSOR_TYPE isa(void){return _P16C73_;};
  virtual void create_symbols(void);
  void create_sfr_map(void);

  //virtual unsigned int program_memory_size(void) const { return 0x800; };


  P16C73(void);
  void create(void);
  static pic_processor *construct(void);

};

//---------------------------------------------------------

class P16C74 : public P16C65 // Not a typo, a 'c74 is more like a 'c65 then a 'c64!
{
 public:

  ADCON0_withccp adcon0;
  ADCON1 adcon1;
  ADRES  adres;

  //const int PROGRAM_MEMORY_SIZE = 0x800;

  virtual PROCESSOR_TYPE isa(void){return _P16C74_;};
  virtual void create_symbols(void);
  void create_sfr_map(void);

  virtual unsigned int program_memory_size(void) const { return 0x1000; };


  P16C74(void);
  void create(void);
  static pic_processor *construct(void);

};


#endif

