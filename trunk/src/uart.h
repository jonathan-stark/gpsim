/*
   Copyright (C) 1998,1999 T. Scott Dattalo

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

#include <iostream.h>
#include <stdio.h>


class invalid_file_register;   // Forward reference


#ifndef __USART_H__
#define __USART_H__

#include "pic-processor.h"
#include "14bit-registers.h"


class _TXSTA;   // Forward references
class _SPBRG;
class _RCSTA;

class _TXREG : public sfr_register
{
 public:

  _TXSTA  *txsta;

  virtual void put(unsigned int);
  virtual void put_value(unsigned int);
  virtual bool  is_empty(void)=0;
  virtual void empty(void)=0;
  virtual void full(void)=0;

};

class _TXSTA : public sfr_register
{
public:
  _TXREG  *txreg;
  unsigned int tsr;
  unsigned int bit_count;

  enum {
    TX9D = 1<<0,
    TRMT = 1<<1,
    BRGH = 1<<2,
    SYNC = 1<<4,
    TXEN = 1<<5,
    TX9  = 1<<6,
    CSRC = 1<<7
  };

  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);

  virtual void transmit_a_bit(void);
  virtual void start_transmitting(void);

};

class _RCREG : public sfr_register
{
 public:

  _RCSTA  *rcsta;


  unsigned int oldest_value;  /* rcreg has a 2-deep fifo. The oldest received
			       * value is stored here, while the most recent
			       * is stored in sfr_register.value . */

  unsigned int fifo_sp;       /* fifo stack pointer */

  virtual unsigned int get(void);
  virtual unsigned int get_value(void);
  virtual void push(unsigned int);
  virtual void pop(void);


};

class _RCSTA : public sfr_register
{
 public:
  enum {
    RX9D = 1<<0,
    OERR = 1<<1,
    FERR = 1<<2,
    ADDEN = 1<<3,
    CREN = 1<<4,
    SREN = 1<<5,
    RX9  = 1<<6,
    SPEN = 1<<7
  };

  _RCREG  *rcreg;
  _SPBRG  *spbrg;
  _TXSTA  *txsta;

  unsigned int rsr;
  unsigned int bit_count;

  unsigned int new_bit;    //TEST!!!!


  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  void receive_a_bit(void);
  virtual void start_receiving(void);

};
class _SPBRG : public sfr_register, public BreakCallBack
{
 public:
  _TXSTA *txsta;
  _RCSTA *rcsta;

  guint64 
    last_cycle,    // The cycle when the spbrg clock last changed
    future_cycle;  // The next cycle spbrg is predicted to change

  virtual void callback(void);
  void start(void);
  void get_next_cycle_break(void);
};

//---------------------------------------------------------------
/*
class USART_MODULE
{
public:

  pic_processor *cpu;
  char * name_str;

  USART_MODULE(void);
  void initialize(void);

};
*/
#endif
