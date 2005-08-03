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


#ifndef INTCON_H
#define INTCON_H
#include <iostream>		// for cout used in breakpoints.h

#include "gpsim_classes.h"
#include "registers.h"
#include "breakpoints.h"

//---------------------------------------------------------
// INTCON - Interrupt control register

class INTCON : public sfr_register
{
public:
  unsigned int interrupt_trace;

enum
{
  RBIF = 1<<0,
  INTF = 1<<1,
  T0IF = 1<<2,
  RBIE = 1<<3,
  INTE = 1<<4,
  T0IE = 1<<5,
  XXIE = 1<<6,    // Processor dependent
  GIE  = 1<<7
};



  INTCON(void);

  virtual void set_cpu(Processor *new_cpu);

  void set_T0IF(void);

  /*
  // Bit 6 of intcon depends on the processor that's being simulated, 
  // This generic function will get called whenever interrupt flag upon
  // which bit 6 enables becomes true. (e.g. for the c84, this
  // routine is called when EEIF goes high.)
  */
  void peripheral_interrupt(void);

  inline void set_gie(void)
    {
      value.put(value.get() | GIE);
      put(value.get());
    }

  inline void set_rbif(bool b)
  {
    if (b)
      put(get() | RBIF);
    else
      put(get() & ~RBIF);
  }

  inline void set_intf(bool b)
  {
    if (b)
      put(get() | INTF);
    else
      put(get() & ~INTF);
 }

  inline void set_t0if(void)
    {
      put(get() | T0IF);
    }

  inline void set_rbie(void)
    {
      put(get() | RBIE);
    }

  inline void set_inte(void)
    {
      put(get() | INTE);
    }

  inline void set_t0ie(void)
    {
      put(get() | T0IE);
    }

  inline void clear_gie(void)
    {
      put(get() & ~GIE);
    }

  virtual bool check_peripheral_interrupt()=0;
  virtual void put(unsigned int new_value);

};


//---------------------------------------------------------
class INTCON2 :  public sfr_register
{
public:

enum
{
  RBIP    = 1<<0,
  TMR0IP  = 1<<2,
  INTEDG2 = 1<<4,
  INTEDG1 = 1<<5,
  INTEDG0 = 1<<6,
  RBPU    = 1<<7
};
};


class INTCON3 :  public sfr_register
{
public:

enum
{
  INT1IF  = 1<<0,
  INT2IF  = 1<<1,
  INT1IE  = 1<<3,
  INT2IE  = 1<<4,
  INT1IP  = 1<<6,
  INT2IP  = 1<<7
};

};


class PIR_SET;

// A 14-bit intcon with pir registers
class INTCON_14_PIR : public INTCON
{
public:

  INTCON_14_PIR()
    {
      pir_set = 0;
    }

  inline void set_pir_set(PIR_SET *p) { pir_set = p; }

  virtual bool check_peripheral_interrupt();

  //private:
  PIR_SET *pir_set;
};



//---------------------------------------------------------
// INTCON_16 - Interrupt control register for the 16-bit core
class RCON;

class INTCON_16 : public INTCON
{
public:

  enum {
    GIEH = GIE,
    GIEL = XXIE
  };
#define INTERRUPT_VECTOR_LO       (0x18 >> 1)
#define INTERRUPT_VECTOR_HI       (0x08 >> 1)

  INTCON_16();

  inline void set_rcon(RCON *r) { rcon = r; }
  inline void set_intcon2(INTCON2 *ic) { intcon2 = ic; }

  virtual void put(unsigned int new_value);

  void clear_gies(void);
  void set_gies(void);
  virtual bool check_peripheral_interrupt() {return false;} // Is this right?
  unsigned int get_interrupt_vector() 
  {
    return interrupt_vector;
  }
  bool haveHighPriorityInterrupt(void) 
  { 
    return  true;
  }
  void set_interrupt_vector(unsigned int new_int_vect)
  {
    interrupt_vector = new_int_vect;
  }

private:
  unsigned int interrupt_vector;        // Starting address of the interrupt
  RCON *rcon;
  INTCON2 *intcon2;
};


#endif /* INTCON_H */
