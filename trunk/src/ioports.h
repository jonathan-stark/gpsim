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

#ifndef __IOPORTS_H__
#define __IOPORTS_H__

#include "gpsim_classes.h"

//---------------------------------------------------------
// IOPORT

class IOPORT : public sfr_register
{
public:

#define IOPINS 8

  IOPIN  *pins[IOPINS];

  // Specify the pin low to high and high to low thresholds.
  int l2h_threshold[IOPINS];
  int h2l_threshold[IOPINS];

  IOPORT_TRIS * tris;

  unsigned int 
    valid_iopins,   // A mask that for those ports that don't have all 8 io bits.
    stimulus_mask,  // A mask indicating which io bits have a stimulus.
    internal_latch; // 

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  virtual void setbit(unsigned int bit_number, bool new_value);
  virtual void setbit_value(unsigned int bit_number, bool new_value);
  int get_bit(unsigned int bit_number);
  int get_bit_voltage(unsigned int bit_number);
  unsigned int get(void);
  void attach_stimulus(stimulus *new_stim, unsigned int bit_position);
  int update_stimuli(void);
  void attach_iopin(IOPIN * new_pin, unsigned int bit_position);
  void attach_node(Stimulus_Node *new_node, unsigned int bit_position);
  void update_pin_directions(unsigned int new_tris);

  IOPORT(void);

};

//---------------------------------------------------------
// IOPORT

class PIC_IOPORT : public IOPORT
{
public:

};

class IOPORT_TRIS : public sfr_register
{
public:
  PIC_IOPORT * port;

  unsigned int 
    valid_iopins;   // A mask that for those ports that don't have all 8 io bits.

  void put(unsigned int new_value);
  virtual void setbit(unsigned int bit_number, bool new_value);
  unsigned int get(void);
  IOPORT_TRIS(void);

};

class PORTB : public PIC_IOPORT
{
public:
#define intedg_MASK (1<<6)
#define rbpu_MASK   (1<<7)

  unsigned int rbpu;
  unsigned int intedg;

  PORTB(void);

  void rbpu_intedg_update(unsigned int);
  unsigned int get(void);
  void setbit(unsigned int bit_number, bool new_value);
};

class PORTA : public PIC_IOPORT
{
public:

  PORTA(void);

  void setbit(unsigned int bit_number, bool new_value);

};


class CCPCON;
class PORTC : public PIC_IOPORT
{
public:

  /* Define the I/O pins that are common among the various PIC's
   * endowed with a port C.
   */

enum
{
    T1CKI = 1 << 0,
    CCP1  = 1 << 2,
    SCK   = 1 << 3,
    SCL   = 1 << 3,  /* SCL and SCK share the same pin */
    SDI   = 1 << 4,
    SDA   = 1 << 4,  /* SDA and SDI share the same pin */
    SDO   = 1 << 5
};

  /* Now define the I/O pins that are device dependent */
enum
{
    T1OSO = 1 << 0,
    T1OSI = 1 << 1,
    _T1OSO = 1 << 1,  /* For some bizarre reason, TMR1's OS interface */
    _T1OSI = 1 << 0,  /* varies from pic to pic... */
    CCP2  = 1 << 1,
    TX    = 1 << 6,  /* Not all pics with a port C have a usart, */
    CK    = 1 << 6,  /* but the ones that do share TX and CK */
    RX    = 1 << 7,  /* Same goes for RX and DT */
    DT    = 1 << 7
};

  /* A flag to sort out how the TMR1 OS interface is implemented.
   *   set: T1OSO == RC0 && T1OSI == RC1
   * clear: T1OSO == RC1 && T1OSI == RC0
   */
  bool t1oso_order;

  CCPCON *ccp1con;
  USART_MODULE *usart;

  PORTC(void);
  unsigned int get(void);
  void setbit(unsigned int bit_number, bool new_value);
};


class PORTD : public PIC_IOPORT
{
public:

  PORTD(void);

  void setbit(unsigned int bit_number, bool new_value);

};

class PORTE : public PIC_IOPORT
{
public:

  PORTE(void);

  void setbit(unsigned int bit_number, bool new_value);

};

#endif  // __IOPORTS_H__
