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
  sfr_register *ipr;
public:
  int valid_bits;
  int writable_bits;
 
  PIR(Processor *pCpu, const char *pName, const char *pDesc,
      INTCON *, PIE *, int _valid_bits);
  // The PIR base class supports no PIR bits directly
  virtual void clear_sspif(){}
  virtual void clear_rcif(){}
  virtual void clear_txif(){}
  virtual void set_adif(){}
  virtual void set_bclif(){}
  virtual void set_ccpif(){}
  virtual void set_cmif(){}
  virtual void set_c1if(){}
  virtual void set_c2if(){}
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

  /// Obtain interrupt request state, as a mask of priorities if relevant
  virtual int interrupt_status();

  virtual void put(unsigned int new_value);

  virtual void setInterrupt(unsigned int bitMask);
  virtual void setPeripheralInterrupt();

  void set_intcon(INTCON *);
  void set_pie(PIE *);
  void set_ipr(sfr_register *);

};
//---------------------------------------------------------
// InterruptSource

class InterruptSource
{
public:
  InterruptSource(PIR *_pir, unsigned int bitMask);
  void Trigger();
  void release(); // called when source is no longer needed.
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
  virtual void set_rcif();

  virtual void set_cmif();

  virtual void set_eeif();

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
 

  PIR1v1(Processor *pCpu, const char *pName, const char *pDesc,
         INTCON *, PIE *);
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

  virtual void set_sspif();

  unsigned int get_sspif()
  {
    return value.get() & SSPIF;
  }
  virtual void clear_sspif();

  virtual void set_txif();
  virtual void set_rcif();

  virtual void set_adif()
  {
    put(get() | ADIF);
  }

  virtual void set_pspif();

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
 
  PIR1v2(Processor *pCpu, const char *pName, const char *pDesc,
         INTCON *, PIE *);
};

//---------------------------------------------------------
// PIR1 Peripheral Interrupt register # 1
//
// This is version 3 of the PIR1 register - as seen on the p16f630 devices

class PIR1v3 : public PIR
{
public:

  enum {
    TMR1IF  = 1<<0,
    CMIF    = 1<<3,
    ADIF    = 1<<6,
    EEIF    = 1<<7
  };
 
  virtual void set_tmr1if() { put(get() | TMR1IF); }

  virtual void set_cmif();

  virtual void set_adif();

  virtual void set_eeif();

 
  PIR1v3(Processor *pCpu, const char *pName, const char *pDesc,
         INTCON *, PIE *);
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

  PIR2v1(Processor *pCpu, const char *pName, const char *pDesc,
         INTCON *, PIE *);
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
    CCP2IF  = 1<<0,
    ECCP1IF = 1<<0,             /* only on the PIC18F4xx devices */
    TMR3IF  = 1<<1,
    LVDIF   = 1<<2,
    BCLIF   = 1<<3,
    EEIF    = 1<<4,
    CMIF    = 1<<6              /* PIC16F87xA, PIC18F4xx devices */
};

  virtual void set_eccp1if()
    {
      put(get() | ECCP1IF);
    }

  virtual void set_ccpif()    /* RP - needs to define set_ccpif too! */
    {
      put(get() | CCP2IF);
    }

  virtual void set_tmr3if()
    {
      put(get() | TMR3IF);
    }

  virtual void set_lvdif()
    {
      put(get() | LVDIF);
    }

  virtual void set_bclif();
  virtual void set_eeif();
  virtual void set_cmif();

  PIR2v2(Processor *pCpu, const char *pName, const char *pDesc,
         INTCON *, PIE *);
};

//---------------------------------------------------------
// PIR2 Peripheral Interrupt register # 3
//
// This is version 2 of the PIR2 register - as seen on the 16f88x devices

class PIR2v3 : public PIR
{
public:

enum
{
    CCP2IF  = 1<<0,
    ULPWUIF = 1<<2,
    BCLIF   = 1<<3,
    EEIF    = 1<<4,
    C1IF    = 1<<5,
    C2IF    = 1<<6,
    OSFIF   = 1<<7
};


  virtual void set_ccpif() 
    {
      put(get() | CCP2IF);
    }

  virtual void set_bclif();
  virtual void set_eeif();
  virtual void set_c1if();
  virtual void set_c2if();

  PIR2v3(Processor *pCpu, const char *pName, const char *pDesc,
         INTCON *, PIE *);
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

  PIR3v2(Processor *pCpu, const char *pName, const char *pDesc,
         INTCON *, PIE *);
};


/*
 * PIR_SET defines an interface to some common interrupt capabilities.
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
  virtual ~PIR_SET()
  {
  }

  virtual int interrupt_status()
  {
    return 0;
  }

  // uart stuff
  virtual bool get_txif()
  {
    return false;
  }
  virtual void set_txif() {}
  virtual void clear_txif() {}
  virtual bool get_rcif()
  {
    return false;
  }
  virtual void set_rcif() {}
  virtual void clear_rcif() {}
        
  // ssp stuff
  virtual bool get_sspif()
  {
    return false;
  }
  virtual void clear_sspif() {}
  virtual void set_sspif() {}
  virtual void set_bclif() {}
 

  virtual void set_pspif() {}
  virtual void set_cmif() {}
  virtual void set_c1if() {}
  virtual void set_c2if() {}

  // eeprom stuff
  virtual void set_eeif() {}

  // CCP stuff
  virtual void set_ccpif() {}

  // Timer stuff
  virtual void set_tmr1if() {}
  virtual void set_tmr2if() {}
  virtual void set_adif() {}
};


//----------------------------------------
// Supports 1 or 2 Pir version 1 registers

class PIR_SET_1 : public PIR_SET
{
 public:
  PIR_SET_1() { pir1 = 0; pir2 = 0; }

  virtual ~PIR_SET_1()
  {
  }

  void set_pir1(PIR *p1) { pir1 = p1; }
  void set_pir2(PIR *p2) { pir2 = p2; }

  virtual int interrupt_status() {
    assert(pir1 != 0);
    if (pir2 != 0)
      return (pir1->interrupt_status() |
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

  // A/D stuff - not part of base PIR_SET class
  virtual void set_adif() {
    assert(pir1 != 0);
    pir1->set_adif();
  }
  // Comparator
  virtual void set_cmif() {
    assert(pir1 != 0);
    pir1->set_cmif();
  }

private:
  PIR   *pir1;
  PIR   *pir2;
};


// Supports 1, 2 or 3 version 2 Pir registers

class PIR_SET_2 : public PIR_SET
{
 public:
  PIR_SET_2() { pir1 = 0; pir2 = 0; pir3 = 0; }

  virtual ~PIR_SET_2()
  {
  }

  void set_pir1(PIR *p1) { pir1 = p1; }
  void set_pir2(PIR *p2) { pir2 = p2; }
  void set_pir3(PIR *p3) { pir3 = p3; }

  virtual int interrupt_status() {
    assert(pir1 != 0);
    int result = pir1->interrupt_status();
    if ( pir2 != 0 )
        result |= pir2->interrupt_status();
    if ( pir3 != 0 )
        result |= pir3->interrupt_status();
    return result; 
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
  // Comparator
  virtual void set_cmif() {
    assert(pir2 != 0);
    pir2->set_cmif();
  }

  virtual void set_c1if() {
    assert(pir2 != 0);
    pir2->set_c1if();
  }
  virtual void set_c2if() {
    assert(pir2 != 0);
    pir2->set_c2if();
  }

  // I2C master
  virtual void set_bclif() {
    assert(pir2 != 0);
    pir2->set_bclif();
  }

  // Parallel Slave Port
  virtual void set_pspif() {
    assert(pir1 != 0);
    pir1->set_pspif();
  }

private:
  PIR   *pir1;
  PIR   *pir2;
  PIR   *pir3;
};


#endif /* PIR_H */
