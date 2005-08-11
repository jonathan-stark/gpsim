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

#include <stdio.h>


class InvalidRegister;   // Forward reference


#ifndef __SSP_H__
#define __SSP_H__

#include "pic-processor.h"
#include "14bit-registers.h"
#include "ioports.h"

class PinModule;

class PIR1;
class PIR_SET;
class  _14bit_processor;

class _SSPBUF;
class _SSPSTAT;

enum SSP_TYPE {
	SSP_TYPE_BSSP = 1,
	SSP_TYPE_SSP,
	SSP_TYPE_MSSP
};


class _SSPCON : public sfr_register, public TriggerObject, public SignalSink
{

protected:

  // Register bit definitions

  enum {
    SSPM0  = 1<<0,
    SSPM1  = 1<<1,
    SSPM2  = 1<<2,
    SSPM3  = 1<<3,
    CKP    = 1<<4,
    SSPEN  = 1<<5,
    SSPOV  = 1<<6,
    WCOL   = 1<<7
  };

  static const unsigned int SSPM_mask = (SSPM0|SSPM1|SSPM2|SSPM3);

  enum {
    SSPM_SPImaster4 = 0x0,      // SPI master mode, clock = FOSC/4
    SSPM_SPImaster16 = 0x1,     // SPI master mode, clock = FOSC/16
    SSPM_SPImaster64 = 0x2,     // SPI master mode, clock = FOSC/64
    SSPM_SPImasterTMR2 = 0x3,   // SPI master mode, clock = TMR2/2
    SSPM_SPIslaveSS = 0x4,      // SPI slave mode, clock = SCK, /SS controlled
    SSPM_SPIslave = 0x5,        // SPI slave mode, clock = SCK, not /SS controlled

    // I2C stuff. FIX: This is not implemented in the code!!!
    SSPM_I2Cslave_7bitaddr = 0x6,
    SSPM_I2Cslave_10bitaddr = 0x7,
    SSPM_MSSPI2Cmaster = 0x8,
	
    SSPM_I2Cfirmwaremaster = 0xb,
    SSPM_I2Cfirmwaremultimaster_7bitaddr_ints = 0xe,
    SSPM_I2Cfirmwaremaster_10bitaddr_ints = 0xf,

    /* These are listed in the BSSP section of the midrange ref. manual. I didn't test it but I suspect them of being lies.
       SSPM_I2Cslave_7bitaddr_ints = 0xe,
       SSPM_I2Cslave_10bitaddr_ints = 0xf,
    */
  };


public:

  _SSPCON();



  virtual void put(unsigned int);
  virtual void put_value(unsigned int);

  bool isIdle() { return m_state==eIDLE;}
  virtual void clock(const char);
  virtual void start_transfer();
  virtual void stop_transfer();
  virtual void enable();
  void set_halfclock_break( unsigned int clocks );
  virtual void assign_pir_set(PIR_SET *ps);
  virtual void callback();

  bool isSSPEnabled() { return (value.get() & SSPEN) == SSPEN; }
  bool isI2CEnabled() { return false; }
  bool isSPIMaster() {
    return isSSPEnabled() && ((value.get() & SSPM_mask) <= SSPM_SPImasterTMR2);
  }
  void setWCOL();
  void newSSPBUF(unsigned int);
  void setIOpins(PinModule *sck,PinModule *ss,PinModule *sdo, PinModule *sdi);
  void setSSPBUF(_SSPBUF *);
  void setSSPSTAT(_SSPSTAT *);
  virtual void setSinkState(const char);

private:
  PeripheralSignalSource *m_SckSource;
  PeripheralSignalSource *m_SsSource;
  PeripheralSignalSource *m_SdoSource;

  _SSPBUF   *m_sspbuf;
  _SSPSTAT  *m_sspstat;

  PIR_SET   *m_pirset;

  char m_cSDIState; 

  unsigned int m_SSPsr;  // internal Shift Register
  enum SSPStateMachine {
    eIDLE,
    eACTIVE,
    eWAITING_FOR_LAST_SMP
  } m_state;


  int bits_transfered;
  bool m_bUnread;
  

};

class _SSPSTAT : public sfr_register
{
 public:

  // Register bit definitions

  enum {
    BF  = 1<<0,  // Buffer Full
    UA  = 1<<1,  // Update Address
    RW  = 1<<2,  // Read/Write info
    S   = 1<<3,  // Start bit (I2C mode)
    P   = 1<<4,  // Stop bit (I2C mode)
    DA  = 1<<5,  // Data/Address bit (I2C mode)

	// below are SSP and MSSP only. This class will force them to
	// always be 0 if ssptype == SSP_TYPE_BSSP. This will give the
	// corrent behavior.
    CKE = 1<<6,  // SPI clock edge select
    SMP = 1<<7   // SPI data input sample phase
  };

  SSP_TYPE ssptype;

  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
};


class _SSPBUF : public sfr_register
{
public:
  
  SSP_TYPE ssptype;

  _SSPBUF();


  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  void putFromSSPSR(unsigned int new_value);
  virtual unsigned int get();
  virtual unsigned int get_value();

  void setSSPCON(_SSPCON *);

  bool isFull() { return m_bIsFull; }
  void setFullFlag(bool bNewFull) { m_bIsFull = bNewFull; }
private:
  _SSPCON   *m_sspcon;
  bool m_bIsFull;
};

class _SSPADD : public sfr_register
{
 public: 

  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
};

class SSP_MODULE // this is SSP, but not MSSP
{
 public:
  _SSPBUF   sspbuf;
  _SSPCON   sspcon;
  _SSPSTAT  sspstat;

  // set to NULL for BSSP (It doesn't have this register)
  _SSPADD   sspadd;

  SSP_MODULE();
  //void initialize(PIR_SET *ps,  IOPORT *ssp_port, int sck_pin, int sdi_pin, int sdo_pin, IOPORT *ss_port, int ss_pin, SSP_TYPE ssp_type);

  void initialize(PIR_SET *ps,
		  PinModule *_SckPin,
		  PinModule *_SdiPin,
		  PinModule *_SdoPin,
		  PinModule *_SsPin);

  //virtual void new_sck_edge(unsigned int);
  //virtual void new_ss_edge(unsigned int);
};

class SSP_MODULE14 : public SSP_MODULE
{
 public:
  _14bit_processor *cpu;

  SSP_MODULE14();

  //virtual void new_sck_edge(unsigned int);
  //virtual void new_ss_edge(unsigned int);
  //void initialize_14(_14bit_processor *new_cpu, PIR_SET *ps, IOPORT *ssp_port, int sck_pin, int sdi_pin, int sdo_pin, IOPORT *ss_port, int ss_pin, SSP_TYPE ssp_type);
};

#endif  // __SSP_H__
