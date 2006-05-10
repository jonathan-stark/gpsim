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

#ifndef PIR_H
#define PIR_H

#include "assert.h"

#include "pie.h"

class INTCON;

//---------------------------------------------------------
// PIR Peripheral Interrupt register base class for PIR1 & PIR2

class PIR : public sfr_register
{
protected:
  INTCON  *intcon;
  PIE     *pie;
public:
  int valid_bits;
  int writable_bits;
 
  PIR(INTCON *, PIE *, int _valid_bits);
  // The PIR base class supports no PIR bits directly
  virtual void clear_sspif(){}
  virtual void clear_rcif(){}
  virtual void clear_txif(){}
  virtual void set_adif(){}
  virtual void set_bclif(){}
  virtual void set_ccpif(){}
  virtual void set_cmif(){}
  virtual void set_eccp1if(){}
  virtual void set_eeif(){}
  virtual void set_errif(){}
  virtual void set_irxif(){}
  virtual void set_lvdif(){}
  virtual void set_pspif(){}
  virtual void set_rcif(){}
  virtual void set_rxb0if(){}
  virtual void set_rxb1if(){}
  virtual void set_sspif(){}
  virtual void set_tmr1if(){}
  virtual void set_tmr2if(){}
  virtual void set_tmr3if(){}
  virtual void set_txb0if(){}
  virtual void set_txb1if(){}
  virtual void set_txb2if(){}
  virtual void set_txif(){}
  virtual void set_wakif(){}

  virtual unsigned int get_txif() { return 0;}
  virtual unsigned int get_rcif() { return 0;}
  virtual unsigned int get_sspif() { return 0;}

  virtual bool interrupt_status();
  virtual void put(unsigned int new_value);

  virtual void setInterrupt(unsigned int bitMask);
  virtual void setPeripheralInterrupt();

  void set_intcon(INTCON *);
  void set_pie(PIE *);

};
//---------------------------------------------------------
// InterruptSource

class InterruptSource
{
public:
  InterruptSource(PIR *_pir, unsigned int bitMask);
  void Trigger();
private:
  PIR *m_pir;
  unsigned int m_bitMask;
};
//---------------------------------------------------------
// PIR1 Peripheral Interrupt register # 1
//
// This is version 1 of the PIR1 register - as seen on the 16f62x

class PIR1v1 : public PIR
{
public:

  enum {
    TMR1IF  = 1<<0,
    TMR2IF  = 1<<1,
    CCP1IF  = 1<<2,
    SSPIF   = 1<<3,
    TXIF    = 1<<4,
    RCIF    = 1<<5,
    CMIF    = 1<<6,     // 16f62x
    EEIF    = 1<<7      // 16f62x
  };

  virtual void set_tmr1if()
  {
    put(get() | TMR1IF);
  }

  virtual void set_tmr2if()
  {
    put(get() | TMR2IF);
  }

  virtual void set_ccpif()
  {
    put(get() | CCP1IF);
  }

  virtual void set_sspif()
  {
    put(get() | SSPIF);
  }

  virtual void set_txif();
  virtual void set_rcif()
  {
    put(get() | RCIF);
  }

  virtual void set_cmif();

  virtual void set_eeif()
  {
    put(get() | EEIF);
  }

  virtual unsigned int get_sspif()
  {
    return value.get() & SSPIF;
  }
  void clear_sspif();

  unsigned int get_txif()
  {
    return value.get() & TXIF;
  }
  void clear_txif();

  unsigned int get_rcif()
  {
    return value.get() & RCIF;
  }
  virtual void clear_rcif();
 

  PIR1v1(INTCON *, PIE *);
};


//---------------------------------------------------------
// PIR1 Peripheral Interrupt register # 1
//
// This is version 2 of the PIR1 register - as seen on the 18xxxx devices
// and devices like the 16c63a.

class PIR1v2 : public PIR
{
public:

  enum {
    TMR1IF  = 1<<0,
    TMR2IF  = 1<<1,
    CCP1IF  = 1<<2,
    SSPIF   = 1<<3,
    TXIF    = 1<<4,
    RCIF    = 1<<5,
    ADIF    = 1<<6,     // 18cxxx
    PSPIF   = 1<<7
  };
 
  virtual void set_tmr1if()
  {
    put(get() | TMR1IF);
  }

  virtual void set_tmr2if()
  {
    put(get() | TMR2IF);
  }

  virtual void set_ccpif()
  {
    put(get() | CCP1IF);
  }

  virtual void set_sspif()
  {
    put(get() | SSPIF);
  }

  unsigned int get_sspif()
  {
    return value.get() & SSPIF;
  }
  virtual void clear_sspif();

  virtual void set_txif();
  virtual void set_rcif()
  {
    put(get() | RCIF);
  }

  virtual void set_adif()
  {
    put(get() | ADIF);
  }

  virtual void set_pspif()
  {
    put(get() | PSPIF);
  }

  virtual unsigned int get_txif()
  {
    return value.get() & TXIF;
  }
  virtual void clear_txif();
  unsigned int get_rcif()
  {
    return value.get() & RCIF;
  }
  virtual void clear_rcif();
 
  PIR1v2(INTCON *, PIE *);
};



//---------------------------------------------------------
// PIR2 Peripheral Interrupt register # 2
//
// This is version 1 of the PIR1 register - as seen on the 16f62x

class PIR2v1 : public PIR
{
public:

enum
{
    CCP2IF  = 1<<0
};

  virtual void set_ccpif()
    {
      put(get() | CCP2IF);
    }

  PIR2v1(INTCON *, PIE *);
};

//---------------------------------------------------------
// PIR2 Peripheral Interrupt register # 2
//
// This is version 2 of the PIR2 register - as seen on the 18xxxx devices
// and devices like the 16c63a.

class PIR2v2 : public PIR
{
public:

enum
{
    ECCP1IF = 1<<0,		/* only on the PIC18F4xx devices */
    TMR3IF  = 1<<1,
    LVDIF   = 1<<2,
    BCLIF   = 1<<3,
    EEIF    = 1<<4,
    CMIF    = 1<<6		/* only on the PIC18F4xx devices */
};

  virtual void set_eccp1if()
    {
      put(get() | ECCP1IF);
    }

  virtual void set_tmr3if()
    {
      put(get() | TMR3IF);
    }

  virtual void set_lvdif()
    {
      put(get() | LVDIF);
    }

  virtual void set_bclif()
    {
      put(get() | BCLIF);
    }

  virtual void set_eeif()
    {
      put(get() | EEIF);
    }

  PIR2v2(INTCON *, PIE *);
};


//---------------------------------------------------------
// PIR2 Peripheral Interrupt register # 3
//
// This is version 2 of the PIR3 register - as seen on the 18F248 devices
// Perhaps other devices too - it contains bits for the CAN device

class PIR3v2 : public PIR
{
public:

enum
{
    RXB0IF  = 1<<0,
    RXB1IF  = 1<<1,
    TXB0IF  = 1<<2,
    TXB1IF  = 1<<3,
    TXB2IF  = 1<<4,
    ERRIF   = 1<<5,
    WAKIF   = 1<<6,
    IRXIF   = 1<<7
};

  virtual void set_rxb0if()
    {
      put(get() | RXB0IF);
    }

  virtual void set_rxb1if()
    {
      put(get() | RXB1IF);
    }

  virtual void set_txb0if()
    {
      put(get() | TXB0IF);
    }

  virtual void set_txb1if()
    {
      put(get() | TXB1IF);
    }

  virtual void set_txb2if()
    {
      put(get() | TXB2IF);
    }

  virtual void set_errif()
    {
      put(get() | ERRIF);
    }

  virtual void set_wakif()
    {
      put(get() | WAKIF);
    }

  virtual void set_irxif()
    {
      put(get() | IRXIF);
    }

  PIR3v2(INTCON *, PIE *);
};


/*
 * PIR_SET defines and interface to some common interrupt capabilities.
 * PIR_SET is a pure virtual class - you must instantiate a more specific
 * version of PIR_SET.
 *
 * The idea behind PIR_SET is to hide the location of the interrupt bits.
 * in some cases, a bit might be in PIR1, in others it might be in PIR2.
 * Instead of accessing the register directly, you go through PIR_SET
 * and it will find the proper PIR register.
 */

class PIR_SET
{
public:
  virtual bool interrupt_status()
  {
    return false;
  }

  // uart stuff
  virtual bool get_txif()
  {
    return false;
  }
  virtual void set_txif()
  {}
  virtual void clear_txif()
  {}
  virtual bool get_rcif()
  {
    return false;
  }
  virtual void set_rcif()
  {}
  virtual void clear_rcif()
  {}
	
  // ssp stuff
  virtual bool get_sspif()
  {
    return false;
  }
  virtual void set_sspif()
  {}
  virtual void clear_sspif()
  {}

  // eeprom stuff
  virtual void set_eeif()
  {}

  // CCP stuff
  virtual void set_ccpif()
  {}

  // Timer stuff
  virtual void set_tmr1if()
  {}
  virtual void set_tmr2if()
  {}
};


//----------------------------------------

class PIR_SET_1 : public PIR_SET
{
 public:
  PIR_SET_1() { pir1 = 0; pir2 = 0; }
  void set_pir1(PIR *p1) { pir1 = p1; }
  void set_pir2(PIR *p2) { pir2 = p2; }

  virtual bool interrupt_status() {
    assert(pir1 != 0);
    if (pir2 != 0)
      return (pir1->interrupt_status() ||
	      pir2->interrupt_status());
    else
      return (pir1->interrupt_status());
  }

  // uart stuff
  virtual bool get_txif() {
    assert(pir1 != 0);
    return (pir1->get_txif() != 0);
  }
  virtual void set_txif() {
    assert(pir1 != 0);
    pir1->set_txif();
  }
  virtual void clear_txif() {
    assert(pir1 != 0);
    pir1->clear_txif();
  }
  virtual bool get_rcif() {
    assert(pir1 != 0);
    return (pir1->get_rcif() != 0);
  }
  virtual void set_rcif() {
    assert(pir1 != 0);
    pir1->set_rcif();
  }
  virtual void clear_rcif() {
    assert(pir1 != 0);
    pir1->clear_rcif();
  }
	
  // ssp stuff
  virtual bool get_sspif() {
    assert(pir1 != 0);
    return (pir1->get_sspif() != 0);
  }
  virtual void set_sspif() {
    assert(pir1 != 0);
    pir1->set_sspif();
  }
  virtual void clear_sspif() {
    assert(pir1 != 0);
    pir1->clear_sspif();
  }

  // eeprom stuff
  virtual void set_eeif() {
    assert(pir1 != 0);
    pir1->set_eeif();
  }

  // CCP stuff
  virtual void set_ccpif() {
    assert(pir1 != 0);
    pir1->set_ccpif();
  }

  // Timer stuff
  virtual void set_tmr1if() {
    assert(pir1 != 0);
    pir1->set_tmr1if();
  }
  virtual void set_tmr2if() {
    assert(pir1 != 0);
    pir1->set_tmr2if();
  }

  // Comparator
  virtual void set_cmif() {
    assert(pir1 != 0);
    pir1->set_cmif();
  }

private:
  PIR	*pir1;
  PIR	*pir2;
};



class PIR_SET_2 : public PIR_SET
{
 public:
  PIR_SET_2() { pir1 = 0; pir2 = 0; pir3 = 0; }

  void set_pir1(PIR *p1) { pir1 = p1; }
  void set_pir2(PIR *p2) { pir2 = p2; }
  void set_pir3(PIR *p3) { pir3 = p3; }

  virtual bool interrupt_status() {
    assert(pir1 != 0);
    assert(pir2 != 0);
    return (pir1->interrupt_status() || pir2->interrupt_status());
  }

  // uart stuff
  virtual bool get_txif() {
    assert(pir1 != 0);
    return (pir1->get_txif() != 0);
  }
  virtual void set_txif() {
    assert(pir1 != 0);
    pir1->set_txif();
  }
  virtual void clear_txif() {
    assert(pir1 != 0);
    pir1->clear_txif();
  }
  virtual bool get_rcif() {
    assert(pir1 != 0);
    return (pir1->get_rcif() != 0);
  }
  virtual void set_rcif() {
    assert(pir1 != 0);
    pir1->set_rcif();
  }
  virtual void clear_rcif() {
    assert(pir1 != 0);
    pir1->clear_rcif();
  }
	
  // ssp stuff
  virtual bool get_sspif() {
    assert(pir1 != 0);
    return (pir1->get_sspif() != 0);
  }
  virtual void set_sspif() {
    assert(pir1 != 0);
    pir1->set_sspif();
  }
  virtual void clear_sspif() {
    assert(pir1 != 0);
    pir1->clear_sspif();
  }


  // eeprom stuff
  virtual void set_eeif() {
    assert(pir2 != 0);
    pir2->set_eeif();
  }

  // CCP stuff
  virtual void set_ccpif() {
    assert(pir1 != 0);
    pir1->set_ccpif();
  }

  // Timer stuff
  virtual void set_tmr1if() {
    assert(pir1 != 0);
    pir1->set_tmr1if();
  }
  virtual void set_tmr2if() {
    assert(pir1 != 0);
    pir1->set_tmr2if();
  }

  // A/D stuff - not part of base PIR_SET class
  virtual void set_adif() {
    assert(pir1 != 0);
    pir1->set_adif();
  }

private:
  PIR	*pir1;
  PIR	*pir2;
  PIR	*pir3;
};


#endif /* PIR_H */
