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
#include "pic-registers.h"
#include "pie.h"

class INTCON;

//---------------------------------------------------------
// PIR Peripheral Interrupt register base class for PIR1 & PIR2

class PIR : public sfr_register
{
public:
  INTCON  *intcon;
  PIE     *pie;
  int valid_bits;
 
  virtual void set_ccpif(void){}

  virtual bool interrupt_status(void)
    {
      if( value & valid_bits & pie->value)
	return true;

      return false;
    }

  virtual void put(unsigned int new_value);

};

//---------------------------------------------------------
// PIR1 Peripheral Interrupt register # 1
//
// This is version 1 of the PIR1 register - as seen on the 16f62x

class PIR1v1 : public PIR
{
public:

enum
{
    TMR1IF  = 1<<0,
    TMR2IF  = 1<<1,
    CCP1IF  = 1<<2,
    SSPIF   = 1<<3,
    TXIF    = 1<<4,
    RCIF    = 1<<5,
    CMIF    = 1<<6,     // 16f62x
    EEIF    = 1<<7      // 16f62x
};

  inline void set_tmr1if(void)
    {
      put(get() | TMR1IF);
    }

  inline void set_tmr2if(void)
    {
      put(get() | TMR2IF);
    }

  void set_ccpif(void)
    {
      put(get() | CCP1IF);
    }

  inline void set_sspif(void)
    {
      put(get() | SSPIF);
    }

  inline void set_txif(void)
    {
      put(get() | TXIF);
    }

  inline void set_rcif(void)
    {
      put(get() | RCIF);
    }

  inline void set_cmif(void)
    {
      put(get() | CMIF);
    }

  inline void set_eeif(void)
    {
      put(get() | EEIF);
    }

  unsigned int get_sspif(void)
  {
	return value & SSPIF;
  }
  void clear_sspif(void);

 unsigned int get_txif(void)
   {
     return value & TXIF;
   }
 void clear_txif(void);

 unsigned int get_rcif(void)
   {
     return value & RCIF;
   }
 void clear_rcif(void);
 

 PIR1v1(void)
   {
     // Even though TXIF is a valid bit, it can't be written by the PIC
     // source code.  Its state reflects whether the usart txreg is full
     // or not.
     valid_bits = TMR1IF | TMR2IF | CCP1IF | SSPIF | RCIF | CMIF | EEIF;
   }
};


//---------------------------------------------------------
// PIR1 Peripheral Interrupt register # 1
//
// This is version 2 of the PIR1 register - as seen on the 18xxxx devices
// and devices like the 16c63a.

class PIR1v2 : public PIR
{
public:

  enum
    {
      TMR1IF  = 1<<0,
      TMR2IF  = 1<<1,
      CCP1IF  = 1<<2,
      SSPIF   = 1<<3,
      TXIF    = 1<<4,
      RCIF    = 1<<5,
      ADIF    = 1<<6,     // 18cxxx
      PSPIF   = 1<<7
    };
 
  inline void set_tmr1if(void)
    {
      put(get() | TMR1IF);
    }

  inline void set_tmr2if(void)
    {
      put(get() | TMR2IF);
    }

  void set_ccpif(void)
    {
      put(get() | CCP1IF);
    }

  inline void set_sspif(void)
    {
      put(get() | SSPIF);
    }

  unsigned int get_sspif(void)
    {
      return value & SSPIF;
    }
  void clear_sspif(void);

  inline void set_txif(void)
    {
      put(get() | TXIF);
    }

  inline void set_rcif(void)
    {
      put(get() | RCIF);
    }

  inline void set_adif(void)
    {
      put(get() | ADIF);
    }

  inline void set_pspif(void)
    {
      put(get() | PSPIF);
    }

  unsigned int get_txif(void)
    {
      return value & TXIF;
    }
  void clear_txif(void);
  unsigned int get_rcif(void)
    {
      return value & TXIF;
    }
  void clear_rcif(void);
 
  PIR1v2(void)
    {
      // Even though TXIF is a valid bit, it can't be written by the PIC
      // source code.  Its state reflects whether the usart txreg is full
      // or not.
      valid_bits = TMR1IF | TMR2IF | CCP1IF | SSPIF | RCIF | ADIF | PSPIF;
    }
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

  void set_ccpif(void)
    {
      put(get() | CCP2IF);
    }

  PIR2v1(void)
    {
      valid_bits = CCP2IF;
    }
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

  void set_eccp1if(void)
    {
      put(get() | ECCP1IF);
    }

  void set_tmr3if(void)
    {
      put(get() | TMR3IF);
    }

  void set_lvdif(void)
    {
      put(get() | LVDIF);
    }

  void set_bclif(void)
    {
      put(get() | BCLIF);
    }

  void set_eeif(void)
    {
      put(get() | EEIF);
    }

  void set_cmif(void)
    {
      put(get() | CMIF);
    }

  PIR2v2(void)
    {
      valid_bits = ECCP1IF | TMR3IF | LVDIF | BCLIF | EEIF | CMIF;
    }
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

  void set_rxb0if(void)
    {
      put(get() | RXB0IF);
    }

  void set_rxb1if(void)
    {
      put(get() | RXB1IF);
    }

  void set_txb0if(void)
    {
      put(get() | TXB0IF);
    }

  void set_txb1if(void)
    {
      put(get() | TXB1IF);
    }

  void set_txb2if(void)
    {
      put(get() | TXB2IF);
    }

  void set_errif(void)
    {
      put(get() | ERRIF);
    }

  void set_wakif(void)
    {
      put(get() | WAKIF);
    }

  void set_irxif(void)
    {
      put(get() | IRXIF);
    }

  PIR3v2(void)
    {
      valid_bits = RXB0IF | RXB1IF | TXB0IF | TXB1IF | TXB2IF | ERRIF |
                   WAKIF | IRXIF;
    }
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
  virtual bool interrupt_status(void) = 0;

  // uart stuff
  virtual bool get_txif(void) = 0;
  virtual void set_txif(void) = 0;
  virtual void clear_txif(void) = 0;
  virtual bool get_rcif(void) = 0;
  virtual void set_rcif(void) = 0;
  virtual void clear_rcif(void) = 0;
	
  // ssp stuff
  virtual bool get_sspif(void) = 0;
  virtual void set_sspif(void) = 0;
  virtual void clear_sspif(void) = 0;

  // eeprom stuff
  virtual void set_eeif(void) = 0;

  // CCP stuff
  virtual void set_ccpif(void) = 0;

  // Timer stuff
  virtual void set_tmr1if(void) = 0;
  virtual void set_tmr2if(void) = 0;
};


//----------------------------------------

class PIR_SET_1 : public PIR_SET
{
 public:
  PIR_SET_1() { pir1 = 0; pir2 = 0; }
  void set_pir1(PIR1v1 *p1) { pir1 = p1; }
  void set_pir2(PIR2v1 *p2) { pir2 = p2; }

  virtual bool interrupt_status(void) {
    assert(pir1 != 0);
    if (pir2 != 0)
      return (pir1->interrupt_status() ||
	      pir2->interrupt_status());
    else
      return (pir1->interrupt_status());
  }

  // uart stuff
  virtual bool get_txif(void) {
    assert(pir1 != 0);
    return (pir1->get_txif() != 0);
  }
  virtual void set_txif(void) {
    assert(pir1 != 0);
    pir1->set_txif();
  }
  virtual void clear_txif(void) {
    assert(pir1 != 0);
    pir1->clear_txif();
  }
  virtual bool get_rcif(void) {
    assert(pir1 != 0);
    return (pir1->get_rcif() != 0);
  }
  virtual void set_rcif(void) {
    assert(pir1 != 0);
    pir1->set_rcif();
  }
  virtual void clear_rcif(void) {
    assert(pir1 != 0);
    pir1->clear_rcif();
  }
	
  // ssp stuff
  virtual bool get_sspif(void) {
    assert(pir1 != 0);
    return (pir1->get_sspif() != 0);
  }
  virtual void set_sspif(void) {
    assert(pir1 != 0);
    pir1->set_sspif();
  }
  virtual void clear_sspif(void) {
    assert(pir1 != 0);
    pir1->clear_sspif();
  }

  // eeprom stuff
  virtual void set_eeif(void) {
    assert(pir1 != 0);
    pir1->set_eeif();
  }

  // CCP stuff
  virtual void set_ccpif(void) {
    assert(pir1 != 0);
    pir1->set_ccpif();
  }

  // Timer stuff
  virtual void set_tmr1if(void) {
    assert(pir1 != 0);
    pir1->set_tmr1if();
  }
  virtual void set_tmr2if(void) {
    assert(pir1 != 0);
    pir1->set_tmr2if();
  }

  // private:
  PIR1v1	*pir1;
  PIR2v1	*pir2;
};



class PIR_SET_2 : public PIR_SET
{
 public:
  PIR_SET_2() { pir1 = 0; pir2 = 0; pir3 = 0; }

  void set_pir1(PIR1v2 *p1) { pir1 = p1; }
  void set_pir2(PIR2v2 *p2) { pir2 = p2; }
  void set_pir3(PIR3v2 *p3) { pir3 = p3; }

  virtual bool interrupt_status(void) {
    assert(pir1 != 0);
    assert(pir2 != 0);
    return (pir1->interrupt_status() || pir2->interrupt_status());
  }

  // uart stuff
  virtual bool get_txif(void) {
    assert(pir1 != 0);
    return (pir1->get_txif() != 0);
  }
  virtual void set_txif(void) {
    assert(pir1 != 0);
    pir1->set_txif();
  }
  virtual void clear_txif(void) {
    assert(pir1 != 0);
    pir1->clear_txif();
  }
  virtual bool get_rcif(void) {
    assert(pir1 != 0);
    return (pir1->get_rcif() != 0);
  }
  virtual void set_rcif(void) {
    assert(pir1 != 0);
    pir1->set_rcif();
  }
  virtual void clear_rcif(void) {
    assert(pir1 != 0);
    pir1->clear_rcif();
  }
	
  // ssp stuff
  virtual bool get_sspif(void) {
    assert(pir1 != 0);
    return (pir1->get_sspif() != 0);
  }
  virtual void set_sspif(void) {
    assert(pir1 != 0);
    pir1->set_sspif();
  }
  virtual void clear_sspif(void) {
    assert(pir1 != 0);
    pir1->clear_sspif();
  }


  // eeprom stuff
  virtual void set_eeif(void) {
    assert(pir2 != 0);
    pir2->set_eeif();
  }

  // CCP stuff
  virtual void set_ccpif(void) {
    assert(pir1 != 0);
    pir1->set_ccpif();
  }

  // Timer stuff
  virtual void set_tmr1if(void) {
    assert(pir1 != 0);
    pir1->set_tmr1if();
  }
  virtual void set_tmr2if(void) {
    assert(pir1 != 0);
    pir1->set_tmr2if();
  }

  // A/D stuff - not part of base PIR_SET class
  virtual void set_adif(void) {
    assert(pir1 != 0);
    pir1->set_adif();
  }

  // private:
  PIR1v2	*pir1;
  PIR2v2	*pir2;
  PIR3v2	*pir3;
};


#endif /* PIR_H */
