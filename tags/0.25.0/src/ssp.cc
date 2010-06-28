/*
   Copyright (C) 1998,1999,2000,2001,2002 Scott Dattalo
	         2006 Roy R Rankin

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

#include <stdio.h>
#include <iostream>

#include "../config.h"
#include "ssp.h"
#include "pic-ioports.h"
#include "stimuli.h"
#include "14bit-processors.h"
#include "14bit-tmrs.h"

#include "xref.h"

//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d-%s() ",__FILE__,__LINE__,__FUNCTION__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

//#warning only supports SPI mode.
//-----------------------------------------------------------
// SSPSTAT - Synchronous Serial Port Status register.


_SSPSTAT::_SSPSTAT(Processor *pCpu, SSP_MODULE *pSSP)
  : sfr_register(pCpu, "sspstat","Synchronous Serial Port Status"),
    m_sspmod(pSSP)
{
}

/*
	only CKE and SMP is writable
*/
void _SSPSTAT::put(unsigned int new_value)
{
  unsigned int old6 = value.get() & ~(CKE|SMP);

  // For BSSP register is read only otherwise
  // only CKE and SMP are writable

  if (!m_sspmod || m_sspmod->ssp_type() == SSP_TYPE_BSSP)
	return;

  put_value(old6 | (new_value & (CKE|SMP)));


}

void _SSPSTAT::put_value(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  value.put(new_value);

}
class SDI_SignalSink : public SignalSink
{
public:
  SDI_SignalSink(SSP_MODULE *_ssp_mod)
    : m_ssp_mod(_ssp_mod)
  {
    assert(_ssp_mod);
  }
  virtual ~SDI_SignalSink(){}
  virtual void release(){}

  void setSinkState(char new3State)
  {
    m_ssp_mod->SDI_SinkState(new3State);
  }
private:
  SSP_MODULE *m_ssp_mod;
};

class SCL_SignalSink : public SignalSink
{
public:
  SCL_SignalSink(SSP_MODULE *_ssp_mod)
    : m_ssp_mod(_ssp_mod)
  {
    assert(_ssp_mod);
  }
  virtual ~SCL_SignalSink(){}
  virtual void release(){}

  void setSinkState(char new3State)
  {
    m_ssp_mod->SCL_SinkState(new3State);
  }
private:
  SSP_MODULE *m_ssp_mod;
};
class SS_SignalSink : public SignalSink
{
public:
  SS_SignalSink(SSP_MODULE *_ssp_mod)
    : m_ssp_mod(_ssp_mod)
  {
    assert(_ssp_mod);
  }
  virtual ~SS_SignalSink(){}
  virtual void release(){}

  void setSinkState(char new3State)
  {
    m_ssp_mod->SS_SinkState(new3State);
  }
private:
  SSP_MODULE *m_ssp_mod;
};


//-----------------------------------------------------------
// SSPCON - Synchronous Serial Port Control register.
//-----------------------------------------------------------

_SSPCON::_SSPCON(Processor *pCpu, SSP_MODULE *pSSP)
  : sfr_register(pCpu, "sspcon","Synchronous Serial Port Control"),
    m_sspmod(pSSP)
{
}
bool _SSPCON::isSPIActive(unsigned int value)
{
    if (value & SSPEN)
    {
      switch(value & SSPM_mask)
      {
	case SSPM_SPImaster4:
	case SSPM_SPImaster16:
	case SSPM_SPImaster64:
	case SSPM_SPImasterTMR2:
	case SSPM_SPIslaveSS:
	case SSPM_SPIslave:
	    return(true);
      }
    }
    return(false);
}
/*
	process write to SSPCON
*/
void _SSPCON::put(unsigned int new_value)
{
  unsigned int old_value = value.get();

  put_value(new_value);

  if ((new_value & SSPEN) && ! (old_value & SSPEN)) // Turn on SSP
       m_sspmod->startSSP(new_value);
  else if (!(new_value & SSPEN) &&  (old_value & SSPEN)) // Turn off SSP
       m_sspmod->stopSSP(old_value);
  else if ((new_value & SSPEN) &&  (old_value & SSPEN)) // change while active
      m_sspmod->changeSSP(new_value, old_value);
}

/*
	update SSPCON without action
*/
void _SSPCON::put_value(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  value.put(new_value & 0xff);

}

/*
	Set WCOL bit of SSPCON
*/
void _SSPCON::setWCOL()
{
  if (value.get() & WCOL)
    return;
  put_value(value.get() | WCOL);

}
/*
	return true if a I2C mode is enabled in SSPCON
*/

bool _SSPCON::isI2CActive(unsigned int val) 
{
    if ( (val & SSPEN) != SSPEN)
	return(false);
    switch(val & SSPM_mask)
    {
      case SSPM_I2Cslave_7bitaddr:
      case SSPM_I2Cslave_10bitaddr:
      case SSPM_MSSPI2Cmaster:
      case SSPM_I2Cfirmwaremaster:
      case SSPM_I2Cslave_7bitaddr_ints:
      case SSPM_I2Cslave_10bitaddr_ints:
	return(true);
	break;
    }
    return(false);
}
bool _SSPCON::isI2CMaster(unsigned int val) 
{
    if ( (val & SSPEN) != SSPEN)
	return(false);
    switch(val & SSPM_mask)
    {
      case SSPM_MSSPI2Cmaster:
      case SSPM_I2Cfirmwaremaster:
	return(true);
	break;
    }
    return(false);
}
/*
	return true if an I2C slave mode is enabled in SSPCON
*/
bool _SSPCON::isI2CSlave(unsigned int val) 
{
    if ( (val & SSPEN) != SSPEN)
	return(false);
    switch(val & SSPM_mask)
    {
      case SSPM_I2Cslave_7bitaddr:
      case SSPM_I2Cslave_10bitaddr:
      case SSPM_I2Cslave_7bitaddr_ints:
      case SSPM_I2Cslave_10bitaddr_ints:
	return(true);
	break;
    }
    return(false);
}

//-----------------------------------------------------------
// SSPBUF - Synchronous Serial Port Control register.
//-----------------------------------------------------------

_SSPBUF::_SSPBUF(Processor *pCpu, SSP_MODULE *pSSP)
  : sfr_register(pCpu, "sspbuf","Synchronous Serial Port Buffer"),
    m_sspmod(pSSP), m_bIsFull(false)
{
}

/*
	process write to SSPBUF
*/
void _SSPBUF::put(unsigned int new_value)
{
  put_value(new_value);
  m_sspmod->newSSPBUF(value.get());
  m_bIsFull = false;
}

/*
	update SSPBUF without processing data
*/
void _SSPBUF::put_value(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  value.put(new_value & 0xff);
}
/*
void _SSPBUF::setSSPMODULE( SSP_MODULE *_sspmod)
{
  m_sspmod = _sspmod;
}
*/
unsigned int _SSPBUF::get()
{
  if( m_sspmod )
	m_sspmod->rdSSPBUF();

  trace.raw(read_trace.get() | value.get());
  m_bIsFull = false;

  return value.get();
}
  
unsigned int _SSPBUF::get_value()
{
  return value.get();
}


//-----------------------------------------------------------
// SSPADD - Synchronous Serial Port Address (for I2C)
//-----------------------------------------------------------
_SSPADD::_SSPADD(Processor *pCpu, SSP_MODULE *pSSP)
  : sfr_register(pCpu, "sspadd","Synchronous Serial Port Address (I2C)"),
    m_sspmod(pSSP)
{
}

void _SSPADD::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  put_value(new_value);

  if( m_sspmod )
	m_sspmod->newSSPADD(new_value);

}

void _SSPADD::put_value(unsigned int new_value)
{
  value.put(new_value & 0xff);
}
SPI::SPI(SSP_MODULE *_ssp_mod, _SSPCON *_sspcon, _SSPSTAT *_sspstat, _SSPBUF *_sspbuf)
{
    m_sspmod = _ssp_mod;
    m_sspcon = _sspcon;
    m_sspstat = _sspstat;
    m_sspbuf = _sspbuf;
}

void SPI::clock( bool ClockState )
{
  // A clock has happened. Either we sent one or we recieved one.
  bool onbeat;
  bool allDone = false;
  if( !m_sspstat || ! m_sspcon)
    return;
  unsigned int sspstat_val = m_sspstat->value.get();
  unsigned int sspcon_val = m_sspcon->value.get();

  //cout << "SPi clock " << ClockState << " m_state=" << m_state << endl;

  if( ClockState ) // rising edge
  {
    if( ( (sspcon_val & _SSPCON::CKP) && !(sspstat_val & _SSPSTAT::CKE) )
	|| ( !(sspcon_val & _SSPCON::CKP) && (sspstat_val & _SSPSTAT::CKE) ) )
      onbeat = true;
    else
      onbeat = false;
  } 
  else // falling edge
  {
    if( ( !(sspcon_val & _SSPCON::CKP) && !(sspstat_val & _SSPSTAT::CKE) )
	|| ( (sspcon_val & _SSPCON::CKP) && (sspstat_val & _SSPSTAT::CKE)))
      onbeat = true;
    else
      onbeat = false;
  }

  if( m_state == eIDLE ){
    if( sspstat_val & _SSPSTAT::CKE ) 
    {
      // FIX: I have NOT verified that PICs actually behave like this.
      cout << "SSP: I can't handle a non-started transfer with CKE = 1." << endl;
      return;
    } 
    else if( onbeat ) 
    {
      // FIX: I have NOT verified that PICs actually behave like this.
      cout << "SSP: Ignoring clock transition to neutral in state IDLE." << endl;
      return;
    }
    else 
    {
        // RP: This is only relevant in slave mode? I think clock is never called
        // while idle in master mode.
      if (verbose)
      	cout << "SPI clock called start_transfer\n";
      start_transfer();
    }

  }
 
  if (!m_sspmod)
    return;

  if( onbeat ) {
    // on beat: data is read in if SMP = 0
    if( !(sspstat_val & _SSPSTAT::SMP) ) {
      m_SSPsr <<= 1;
      if (m_sspmod->get_SDI_State())
	m_SSPsr |= 1;
      if (verbose)
      	cout << "SSP: SPI Received bit = " << (m_SSPsr & 1) << ". (SMP=0)" << endl;
    }
  } else {
    // off beat: data is shifted out, data is read in if SMP = 1

    if( sspstat_val & _SSPSTAT::SMP ) {
      m_SSPsr <<= 1;
      if (m_sspmod->get_SDI_State())
	m_SSPsr |= 1;
      if (verbose)
      	cout << "SSP: SPI Received bit = " << (m_SSPsr & 1) << ". (SMP=1)" << endl;
    }
	
    char nextSDO = (m_SSPsr&(1<<7)) ? '1' : '0';
    m_sspmod->putStateSDO(nextSDO);
    if (verbose)
      cout << "SSP: SPI Sent bit = " << nextSDO << "."  << endl;
  }

  bool bSSPCONValue = (sspcon_val & _SSPCON::CKP) ? true : false;
  if(bSSPCONValue == ClockState) {
    bits_transfered++;
    if( bits_transfered == 8 ) 
    {
      if( (sspstat_val & _SSPSTAT::SMP) && !(sspstat_val & _SSPSTAT::CKE) ) 
      {
	m_state = eWAITING_FOR_LAST_SMP;
      } 
      else 
      {
	stop_transfer();
        allDone = true;
      }
    }
  }
  if ( !allDone && m_sspcon->isSPIMaster() )
    set_halfclock_break();
}

void SPI::set_halfclock_break()
{
  int clock_in_cycles = 1;

  if( !m_sspstat || ! m_sspcon)
    return;

  unsigned int sspcon_val = m_sspcon->value.get();

  switch( sspcon_val & _SSPCON::SSPM_mask ) {
  // Simulation requires Fosc/4 to be run at Fosc/8
  case _SSPCON::SSPM_SPImaster4: 
    clock_in_cycles = 1;
    break;
  case _SSPCON::SSPM_SPImaster16:
    clock_in_cycles = 2;
    break;
  case _SSPCON::SSPM_SPImaster64:
    clock_in_cycles = 8;
    break;
  case _SSPCON::SSPM_SPImasterTMR2:
    break;
  }
  
  get_cycles().set_break(get_cycles().get() + clock_in_cycles, this);
}
void SPI::callback()
{
  if (!m_sspmod)
    return;

  if (verbose)
    cout << "SPI callback m_state=" << m_state << endl;

  switch( m_state ) {
  case eIDLE:
    break;
  case eACTIVE:
    m_sspmod->Sck_toggle();
    clock( m_sspmod->get_SCL_State() );
    break;
  case eWAITING_FOR_LAST_SMP:
    if( m_sspstat && m_sspstat->value.get() & _SSPSTAT::SMP ) {
      m_SSPsr <<= 1;
      if (m_sspmod->get_SDI_State())
	m_SSPsr |= 1;

      if (verbose)
      	cout << "SSP: Received bit = " << (m_SSPsr & 1) << ". (SMP=1)" << endl;
    }
	
    m_state = eACTIVE;
    stop_transfer();
    break;
  }
	  
}
//-----------------------------------------------------------

void SPI::startSPI()
{
    m_state = eIDLE;
    bits_transfered = 0;
}

SSP_MODULE::SSP_MODULE(Processor *pCpu)
  : sspbuf(pCpu,this),
    sspcon(pCpu,this),
    sspstat(pCpu,this),
    sspcon2(pCpu,this),
    sspadd(pCpu,this),
    m_pirset(0),
    m_spi(0),
    m_i2c(0),
    m_sck(0),
    m_ss(0),
    m_sdo(0),
    m_sdi(0),
    m_i2c_tris(0),
    m_SckSource(0),
    m_SdoSource(0),
    m_SdiSource(0),
    m_SDI_Sink(0),
    m_SCL_Sink(0),
    m_SS_Sink(0),
    m_sink_set(false)
{
}
SSP_MODULE::~SSP_MODULE()
{
}

void SPI::newSSPBUF(unsigned int newTxByte)
{
  if (m_sspcon->isSSPEnabled()) {
    if (m_state == eIDLE) {
	m_SSPsr = newTxByte;
	start_transfer();
    } else {
      // Collision
      m_sspcon->setWCOL();
    }
  } 
}
void SPI::start_transfer()
{
  if (!m_sspcon || !m_sspstat)
    return;

  // load the shift register
  m_state = eACTIVE;
  bits_transfered = 0;
  unsigned int sspcon_val = m_sspcon->value.get();
  unsigned int sspstat_val = m_sspstat->value.get();

  if (verbose)
    cout << "SSP: SPI Starting transfer. byte=0x" << hex << m_SSPsr << endl;

  switch( sspcon_val & _SSPCON::SSPM_mask ) {
  case _SSPCON::SSPM_SPImaster4:
  case _SSPCON::SSPM_SPImaster16:
  case _SSPCON::SSPM_SPImaster64:
    // In master mode, the SDO line is always set at the start of the transfer
    m_sspmod->putStateSDO((m_SSPsr &(1<<7)) ? '1' : '0');
    // Setup callbacks for clocks
    set_halfclock_break();
    break;
  case _SSPCON::SSPM_SPImasterTMR2: 
      m_sspmod->putStateSDO((m_SSPsr &(1<<7)) ? '1' : '0');
	break;
  case _SSPCON::SSPM_SPIslaveSS:
    // The SS pin was pulled low
    if( sspstat_val & _SSPSTAT::CKE )
      m_sspmod->putStateSDO((m_SSPsr &(1<<7)) ? '1' : '0');
    break;
  case _SSPCON::SSPM_SPIslave:
    // I don't do any thing until first clock edge
    break;
  default:
    cout << "start_transfer: The selected SPI mode is unimplemented. mode=" << hex
	<<(sspcon_val & _SSPCON::SSPM_mask) << endl;
  }
  
}
void SPI::stop_transfer()
{
  if (!m_sspcon  || !m_sspstat || !m_sspbuf || !m_sspmod)
	return;

  if( m_state == eACTIVE ) {
    if( bits_transfered == 8 && !m_sspbuf->isFull() )
    {
        if (verbose)
            cout << "SPI: Stoping transfer. Normal finish. Setting sspif and BF\n";
	m_sspbuf->put_value(m_SSPsr & 0xff);
	m_sspbuf->setFullFlag(true);
        m_sspmod->set_sspif();
	m_sspstat->put_value(m_sspstat->value.get() | _SSPSTAT::BF);
    } else if( bits_transfered == 8 && m_sspbuf->isFull() ) {
      if (verbose)
          cout << "SPI: Stopping transfer. SSPBUF Overflow setting SSPOV." << endl;
        m_sspcon->setSSPOV();
        m_sspmod->set_sspif();      // The real PIC sets sspif even with overflow
    } else {
      cout << "SPI: Stopping transfer. Cancel finish." << endl;
      // The transfer was canceled in some way
    }
  } else {
    if (verbose)
      cout << "SSP: Stopping transfer. State != ACTIVE." << endl;
  }

  m_state = eIDLE;
  
}

I2C::I2C(SSP_MODULE *_ssp_mod, _SSPCON *_sspcon, _SSPSTAT *_sspstat, 
	_SSPBUF *_sspbuf, _SSPCON2 *_sspcon2, _SSPADD *_sspadd)
{
    m_sspmod = _ssp_mod;
    m_sspcon = _sspcon;
    m_sspstat = _sspstat;
    m_sspbuf = _sspbuf;
    m_sspcon2 = _sspcon2;
    m_sspadd = _sspadd;
    future_cycle = 0;
}

void I2C::set_idle()
{
    i2c_state = eIDLE;
}
bool I2C::isIdle()
{
    return(
	(m_sspstat->value.get() & _SSPSTAT::RW) == 0 &&
	(m_sspcon2->value.get() & 
	   (
	     _SSPCON2::ACKEN ||
	     _SSPCON2::RCEN ||
	     _SSPCON2::PEN ||
	     _SSPCON2::RSEN ||
	     _SSPCON2::SEN
	   )) == 0
	);
}
bool I2C::rx_byte()
{
    m_SSPsr = ( m_SSPsr << 1 ) |  (m_sspmod->get_SDI_State()?1:0);
    bits_transfered++;
    if (bits_transfered == 8)
    {
	m_sspcon2->put_value(m_sspcon2->value.get() & ~_SSPCON2::RCEN);
	if (verbose & 2)
	    cout << "CLK_RX_BYTE got byte=" << hex << m_SSPsr << endl;
	m_sspmod->SaveSSPsr(m_SSPsr & 0xff);
        m_sspmod->set_sspif();
	set_idle();
	return(true);
    }
    return(false);
}
void I2C::callback()
{


    if (verbose & 2)
        cout << "I2C::callback i2c_state " << i2c_state << " phase=" << phase <<endl;
    if (future_cycle != get_cycles().get())
    {
	cout << "I2C program error future_cycle=" << future_cycle << " now=" 
		<< get_cycles().get() << " i2c_state=" << i2c_state << endl;
    }

    future_cycle = 0;
    switch(i2c_state)
    {
    case CLK_ACKEN:
	if (phase == 1)
	{
	    m_sspmod->setSCL(true);
	}
	else if (phase == 2)
	{
	    m_sspmod->setSCL(false);
       	    m_sspcon2->value.put( m_sspcon2->value.get() & ~(_SSPCON2::ACKEN ));
            m_sspmod->set_sspif();
	}
	else
	{
	    cout << "CLK_ACKEN unexpected phase " << phase << endl;
	}
	break;

    case CLK_START:
	if (phase == 0)
	{
	    phase++;
	    m_sspmod->setSDA(false);
	    setBRG();
	}
	else
	{
	    m_sspcon2->value.put(m_sspcon2->value.get() & 
		~(_SSPCON2::SEN | _SSPCON2::RSEN));
	    m_sspmod->setSCL(false);
	    m_sspmod->set_sspif();
	    set_idle();
	}
	break;

    case CLK_RSTART:
	if (phase == 0)
  	{
	    m_sspmod->setSCL(true);
	}
	break;

    case CLK_STOP:
	if (phase == 0)
	{
	    phase++;
	    if (m_sspmod->get_SCL_State())
	    {
		setBRG();
	    }
	    m_sspmod->setSCL(true);
	}
	else if (phase == 1)
	{
	    phase++;
	    setBRG();
	    m_sspmod->setSDA(true);
	}
	else
	{
	    if (m_sspstat->value.get() &  _SSPSTAT::P)
	    {
		if (verbose & 2)
		    cout << "I2C::callback stop finish\n";
	    	m_sspmod->set_sspif();
	    }
	    else
	    {
		if (verbose & 2)
		    cout << "I2C::callback stop fail\n";
	    	m_sspmod->set_bclif();
	    }
	    set_idle();
	    m_sspcon2->value.put(m_sspcon2->value.get() & ~_SSPCON2::PEN );
	}
	break;


    case CLK_TX_BYTE:
	if(m_sspmod->get_SCL_State()) 
	{
	    bool n_ack = m_sspmod->get_SDI_State();
	    bits_transfered++;
	    if (bits_transfered < 8)
	    {
	        m_SSPsr <<= 1;
	        m_sspmod->setSCL(false);
	        m_sspmod->setSDA((m_SSPsr & 0x80) == 0x80);
	    }
	    else if(bits_transfered == 8) 
	    {
	        m_sspmod->setSCL(false);
	        m_sspmod->setSDA(true);
	    	m_sspstat->put_value(m_sspstat->value.get() & ~_SSPSTAT::BF);
		if (verbose & 2)
		    cout << "I2C::callback CLK_TX_BYTE sent\n";
            }
	    else
	    {
		if (verbose & 2)
		{
		    cout << "I2C::callback CLK_TX_BYTE _ACK=" << n_ack <<
		        " clock=" << get_cycles().get() << endl;
		}
		if (n_ack)
		    m_sspcon2->put_value(m_sspcon2->value.get() | _SSPCON2::ACKSTAT);
		else
		    m_sspcon2->put_value(m_sspcon2->value.get() & ~_SSPCON2::ACKSTAT);
		m_sspstat->put_value(m_sspstat->value.get() & ~_SSPSTAT::RW);
		m_sspmod->set_sspif();
		set_idle();
	        m_sspmod->setSCL(false);
	    }
	}
	else 
		m_sspmod->setSCL(true);
	break;

    case CLK_RX_BYTE:
	if(m_sspmod->get_SCL_State()) 
	{
	    rx_byte();
	    m_sspmod->setSCL(false);
	}
	else 
		m_sspmod->setSCL(true);
	break;


    default:
	cout << "I2C::callback unxpected i2c_state=" << dec << i2c_state << endl;
	break;
	
    }
}
void I2C::clock(bool clock_state)
{
  unsigned int sspcon_val = m_sspcon->value.get();
  unsigned int sspstat_val = m_sspstat->value.get();
    if (verbose & 2)
        cout << "I2C::clock  SCL=" << clock_state << " SDI=" << 
	    m_sspmod->get_SDI_State() << " i2c_state=" << i2c_state << 
	    " phase=" << phase << endl;
    if (clock_state)	// Do read on clock high transition
    {
	switch(i2c_state)
	{
	  case CLK_STOP:
	    if (phase == 1)
		setBRG();
	    break;

	  case CLK_ACKEN:
	    if (phase == 1)
	    {
		phase++;
		setBRG();
	    }
	    break;

	case CLK_RSTART:
	    if (phase == 0)
	    {
    		if (!m_sspmod->get_SDI_State())
    		{
		    if (verbose)
		        cout << "I2C::clock CLK_RSTART bus collision\n";
		    bus_collide();
	    	    m_sspmod->setSDA(true);
		}
		else
		{
		    clrBRG();
		    start_bit();
		}
	    }
	    else if (phase == 1)
	    {
		setBRG();
	    }
	    break;
	    

	  case RX_CMD:
	  case RX_CMD2:
	  case RX_DATA:
		if (bits_transfered < 8)
		{
	  	    m_SSPsr = (m_SSPsr << 1) | (m_sspmod->get_SDI_State()?1:0);
		    bits_transfered++;
		}
	    break;


	   case CLK_TX_BYTE:
	   case CLK_RX_BYTE:
	     setBRG();
	     break;

	   default:
	     break;
	}
    }
    else	// Do writes of clock low transition
    {
	switch(i2c_state)
	{
	case CLK_ACKEN:
	    clrBRG();
	    if (phase)
	    {
		m_sspmod->setSCL(false);
        	m_sspcon2->value.put(
			m_sspcon2->value.get() & ~(_SSPCON2::ACKEN ));
	    	m_sspmod->set_sspif();
	    	set_idle();
	    }
	    break;

	case CLK_START:
	    clrBRG();
	    if (phase == 0 )
	    {
		if (verbose)
		   cout << "I2C::clock CLK_START Bus collision\n";
		bus_collide();
	    }
	    else if (phase == 1)
	    {
        	m_sspcon2->value.put(m_sspcon2->value.get() & 
		    ~(_SSPCON2::SEN | _SSPCON2::RSEN));
	    }
	    break;

	case CLK_RSTART:
	    if (phase == 0)
        	m_sspmod->setSDA(true);
	    break;

	case RX_CMD:
	case RX_CMD2:
	    if (bits_transfered == 8)
	    {
    		if ( !( m_SSPsr == 0 && 
			(m_sspcon2->value.get() & _SSPCON2::GCEN)
		      )
		      &&
     		      (m_SSPsr & 0xfe) != m_sspadd->value.get() )
    		{
      		    cout << "READ_CMD address missmatch " << hex << m_SSPsr <<
			" != " << m_sspadd->value.get() << endl;
      		    set_idle();
		    return;
    		}
	    }
	    else if (bits_transfered == 9)
	    {
		if(end_ack())
		{
		    m_sspstat->put_value(sspstat_val & ~_SSPSTAT::DA);
		slave_command();
	        }
		return;
	    }
	    // Fall Through
	case RX_DATA:
	    if (bits_transfered == 8)
	    {
		if (verbose)
		    cout << "I2C::clock RX_DATA or CMD m_SSPsr=" << hex << (m_SSPsr & 0xff) << endl;
		if (m_sspmod->SaveSSPsr(m_SSPsr & 0xff) ) // ACK ?
		{
		    if (verbose)
	    	        cout << "I2C::clock RX_DATA or CMD  Send ACK\n";
	    	    m_sspmod->setSDA(false);
		}
		else
		{
		    if (verbose)
	    	        cout << "I2C::clock RX_DATA or CMD  Send NACK\n";
		    m_sspmod->setSDA(true);
		}
		bits_transfered++;
	    }
	    else if (bits_transfered == 9)
	    {
		end_ack();
		m_sspstat->put_value(sspstat_val | _SSPSTAT::DA);
	    }
	    break;


	case CLK_TX_BYTE:
	case CLK_RX_BYTE:
	    setBRG();
	    break;

	case TX_DATA:
	    bits_transfered++;
	    if (bits_transfered < 8)
	    {
	        m_SSPsr <<= 1;
	        m_sspmod->setSDA((m_SSPsr & 0x80) == 0x80);
	    }
	    else if(bits_transfered == 8) 
	    {
	        m_sspmod->setSDA(true);
	    	m_sspstat->put_value(sspstat_val & ~_SSPSTAT::BF);
		if (verbose)
		    cout << "I2C::clock TX_DATA  sent byte\n";
	    }
	    else if(bits_transfered == 9) 
	    {
	        m_sspmod->set_sspif();
	    	if (m_sspmod->get_SDI_State())	// NACK
	    	{
		    if (verbose)
		    	cout << "I2C::clock TX_DATA  got NACK\n";
		    m_sspstat->put_value(sspstat_val & _SSPSTAT::BF);
		    set_idle();
		    return;
	    	}
		m_sspstat->put_value(sspstat_val | _SSPSTAT::DA);
 	     	if (sspstat_val & _SSPSTAT::RW)
	    	{
		    sspcon_val &= ~ _SSPCON::CKP;
		    m_sspcon->put_value(sspcon_val);
		    if (verbose)
		        cout << "I2C::clock TX_DATA Strech clock sspcon=" << hex << sspcon_val << endl;
		    m_sspmod->setSCL(false);
	    	}
            }
	    break;

	default:
	    break;
	}
    }
}

void I2C::slave_command()
{
    unsigned int sspcon_val = m_sspcon->value.get();
    unsigned int sspstat_val = m_sspstat->value.get();

    if (verbose)
	cout << "I2C::slave_command m_SSPsr=" << hex << m_SSPsr << endl;
    if ( m_SSPsr == 0 && (m_sspcon2->value.get() & _SSPCON2::GCEN))
    {
	i2c_state = RX_DATA;
    }
    else
    {
	if (verbose)
	    cout << "I2c::slave_command i2c_state=" << i2c_state << " sspcon=" << sspcon_val << endl;
    	switch( sspcon_val & _SSPCON::SSPM_mask ) 
	{
      	case _SSPCON::SSPM_I2Cslave_10bitaddr_ints:
      	case _SSPCON::SSPM_I2Cslave_10bitaddr:
		if (i2c_state == RX_CMD && (m_SSPsr & 1))
		{
		    sspstat_val |= _SSPSTAT::RW;
		    i2c_state = TX_DATA;
	    	    m_sspmod->setSCL(false);  // clock low
	    	    sspcon_val &= ~ _SSPCON::CKP;
	    	    m_sspcon->put_value(sspcon_val);
		}
		else
		{
	       	    sspstat_val |= _SSPSTAT::UA;
		    i2c_state = (i2c_state == RX_CMD2) ? 
			RX_DATA : RX_CMD2;
		}
		break;

      	case _SSPCON::SSPM_I2Cslave_7bitaddr:
      	case _SSPCON::SSPM_I2Cslave_7bitaddr_ints:
		if (i2c_state == RX_CMD && (m_SSPsr & 1))
		{
		    sspstat_val |= _SSPSTAT::RW;
	            sspstat_val &= ~_SSPSTAT::BF;
	    	    i2c_state = TX_DATA;
		    sspcon_val &= ~ _SSPCON::CKP;
		    m_sspcon->put_value(sspcon_val);
		    m_sspmod->setSCL(false);  // clock low
		}
		else
		{
		    i2c_state = RX_DATA;
		}
		break;
	}
    	m_sspstat->put_value(sspstat_val);
    }
}
bool I2C::end_ack()
{

	m_sspmod->set_sspif();
	bits_transfered = 0;
 	if (m_sspmod->get_SDI_State())      // NACK
        {
	    if (verbose & 2)
	      cout << "I2C::end_ack NACK\n";
            set_idle();
	    return(false);
        }
	else
	{
	    m_sspmod->setSDA(true);
	    if (verbose & 2)
	      cout << "I2C::end_ack ACK\n";
	    return(true);
	}
}
void I2C::bus_collide()
{
   m_sspcon2->value.put(m_sspcon2->value.get() &
       ~ (_SSPCON2::SEN | _SSPCON2::RSEN | _SSPCON2::PEN |
	  _SSPCON2::RCEN | _SSPCON2::ACKEN));
   m_sspmod->set_bclif();
   set_idle();

}
void I2C::newSSPADD(unsigned int newTxByte)
{
    unsigned int sspstat_val = m_sspstat->value.get();

    if (sspstat_val & _SSPSTAT::UA)
    {
	m_sspstat->put_value(sspstat_val & ~_SSPSTAT::UA);
	m_sspmod->setSCL(true);		// turn off clock stretch
    }
}
void I2C::setBRG()
{
    if (future_cycle)
	cout << "ERROR I2C::setBRG called with future_cycle=" << future_cycle << endl;
      future_cycle = get_cycles().get() + 
	  	((m_sspadd->value.get() &0x7f)/ 2) + 1;
      get_cycles().set_break(future_cycle, this);
}

void I2C::clrBRG()
{
	    if (future_cycle)
	    {
	    	get_cycles().clear_break(this);
		future_cycle = 0;
	    }
}
void I2C::newSSPBUF(unsigned int newTxByte)
{

    if (!m_sspstat || !m_sspcon)
	return;
    unsigned int sspstat_val = m_sspstat->value.get();
    unsigned int sspcon_val = m_sspcon->value.get();

    if (m_sspcon2 && (sspcon_val &  _SSPCON::SSPM_mask) ==  _SSPCON::SSPM_MSSPI2Cmaster)
    {
	if (isIdle())
	{
	   if (verbose)
		cout << "I2C::newSSPBUF send " << hex << newTxByte << endl;
	    m_sspmod->setSCL(false);
	    m_sspstat->put_value(sspstat_val | _SSPSTAT::BF | _SSPSTAT::RW);
	    m_SSPsr = newTxByte;
	    m_sspmod->setSDA((m_SSPsr & 0x80) == 0x80);
	    bits_transfered = 0;
	    i2c_state = CLK_TX_BYTE;
	    setBRG();
	}
	else
	{
	    cout << "I2C::newSSPBUF I2C not idle on write data=" << hex << 
		newTxByte << endl;
      	    // Collision
      	    m_sspcon->setWCOL();
	}
    }
    else
    {
	if (sspstat_val & _SSPSTAT::RW)
	{
      	    if (!(sspstat_val & _SSPSTAT::BF))
	    {    
		if (verbose)
		    cout << "I2C::newSSPBUF send " << hex << newTxByte << endl;
	        m_SSPsr = newTxByte;
	        m_sspstat->put_value(sspstat_val | _SSPSTAT::BF);
	        m_sspmod->setSDA((m_SSPsr & 0x80) == 0x80);
	        bits_transfered = 0;
    	    } 
	    else // Collision
	    {
	        cout << "I2C::newSSPBUF I2C not idle on write data=" << hex << 
	 	    newTxByte << endl;
      	        m_sspcon->setWCOL();
	    }
	} 
	else
	   cout << "I2C::newSSPBUF write SSPSTAT::RW not set\n";
    }
}
void I2C::sda(bool data_val)
{
    if (m_sspmod->get_SCL_State())	// Clock is high
    {
    	unsigned int stat_val = m_sspstat->value.get();
	unsigned int sspm = (m_sspcon->value.get() & _SSPCON::SSPM_mask);
	if (data_val)	// Data going high - STOP
	{
    	    stat_val = (stat_val & _SSPSTAT::BF) | _SSPSTAT::P;
	    if (! future_cycle)
	    	set_idle();

	    if (verbose)
		cout << "I2C::sda got STOP future_cycle=" << future_cycle <<  endl;
 	}
	else		// Data going low - START
	{
	    switch (i2c_state)
	    {
	    case CLK_STOP:
		break;

	    case CLK_START:
		if (phase == 0)
		{	
		    guint64 fc = get_cycles().get() + 
			((m_sspadd->value.get() &0x7f)/ 2) + 1;

		    if (future_cycle)
		    {
			phase++;
			if (verbose)
			  cout << "I2C::sda BUS_CHECK fc=" << fc << " future_cycle=" << future_cycle << endl;
		    	get_cycles().reassign_break(future_cycle, fc, this);
		    	future_cycle = fc;
		    }
		    else
		    {
		    	get_cycles().set_break(fc, this);
		    	future_cycle = fc;
		    }
		}
		break;

	    default:
		i2c_state = RX_CMD;
		break;
	    }
    	    stat_val = (stat_val & _SSPSTAT::BF) | _SSPSTAT::S;
	    bits_transfered = 0;
	    m_SSPsr = 0;
	    if (verbose)
		cout << "I2C::sda got START ";
	}
	m_sspstat->put_value(stat_val);

	// interrupt ? 
	if (sspm == _SSPCON::SSPM_I2Cslave_7bitaddr_ints ||
	    sspm == _SSPCON::SSPM_I2Cslave_10bitaddr_ints)
	{
	    m_sspmod->set_sspif();
	}
    }
    else	// clock low
    {
	if (i2c_state == CLK_STOP)
	{
	    if (verbose)
	        cout << "I2C::sda CLK_STOP SDA low CLOCK low\n";
//	    setBRG();
	}
    }
}
/*
	master mode, begin reading a byte
*/
void I2C::master_rx()
{
    if (verbose)
        cout << "I2C::master_rx SCL=" << m_sspmod->get_SCL_State() << " SDI=" << m_sspmod->get_SDI_State() << endl;
    m_sspmod->setSCL(false);
    m_sspmod->setSDA(true);
    bits_transfered = 0;
    m_SSPsr = 0;
    i2c_state = CLK_RX_BYTE;
    setBRG();
}
/*
	master, begin start sequence
		SCL and SDA must be high, then force SDA low
*/
void I2C::start_bit()
{
    if (m_sspmod->get_SCL_State() && m_sspmod->get_SDI_State())
    {

	i2c_state = CLK_START;
	phase = 0;
	setBRG();

    }
    else
    {
	if (verbose & 2)
	    cout << "I2C::start_bit bus collision " <<
	        " SCL=" << m_sspmod->get_SCL_State() <<
	        " SDI=" << m_sspmod->get_SDI_State() << endl;
	bus_collide();
    }
}
/*
	Master mode, begin rstart sequence
		bring SDA and SCL high, then SDA low with SCL high (start)
*/ 
void I2C::rstart_bit()
{
    if (verbose)
        cout << "I2C::rstart_bit SCL=" << m_sspmod->get_SCL_State() << 
	    " SDI=" << m_sspmod->get_SDI_State() << endl;

    i2c_state = CLK_RSTART;
    phase = 0;
    m_sspmod->setSCL(false);

    if (!m_sspmod->get_SCL_State())
    {
	setBRG();
        m_sspmod->setSDA(true);
    }
    else
	bus_collide();

    
}
/*
	master, begin stop sequence
		drop SDA (mught cause start if SCL high)
		when SCL high, raise SDA (stop condition)
		
*/
void I2C::stop_bit()
{
    i2c_state = CLK_STOP;
    phase = 0;
    m_sspmod->setSDA(false);

    if (!m_sspmod->get_SDI_State())
    {
	setBRG();
    }
    else
	bus_collide();

}

/*
	master, begin ack sequence 
		clock SCL low, set SDA as per ACKDT,
		clock SCL high
*/
void I2C::ack_bit()
{
    if (verbose)
        cout << "I2C::ack_bit ACKDT="
	    << (m_sspcon2->value.get() & _SSPCON2::ACKDT) << endl;
    i2c_state = CLK_ACKEN;
    phase = 0;
    m_sspmod->setSCL(false);
    if (!m_sspmod->get_SCL_State())
    {
        phase++;
	setBRG();
        m_sspmod->setSDA((m_sspcon2->value.get() & _SSPCON2::ACKDT) ? true : false);
    }
    else
	bus_collide();

    
}
void SSP_MODULE::initialize(
		        PIR_SET *ps,
			PinModule *SckPin,
			PinModule *SsPin,
			PinModule *SdoPin,
			PinModule *SdiPin,
                 	PicTrisRegister *_i2ctris,
			SSP_TYPE _ssptype
		)
{
  m_pirset = ps;
  m_sck = SckPin;
  m_ss = SsPin;
  m_sdo = SdoPin;
  m_sdi = SdiPin;
  m_i2c_tris = _i2ctris;
  m_ssptype = _ssptype;
  m_SckSource = new PeripheralSignalSource(m_sck);
  m_SdoSource = new PeripheralSignalSource(m_sdo);
  m_SdiSource = new PeripheralSignalSource(m_sdi);
  if (! m_spi)
  {
    m_spi = new SPI(this, &sspcon, &sspstat, &sspbuf);
    m_i2c = new I2C(this, &sspcon, &sspstat, &sspbuf, &sspcon2, &sspadd);
    m_SDI_Sink = new SDI_SignalSink(this);
    m_SCL_Sink = new SCL_SignalSink(this);
    m_SS_Sink = new SS_SignalSink(this);
  }


}
void SSP_MODULE::ckpSPI(unsigned int value)
{
    if(m_spi && !m_spi->isIdle())
      cout << "SPI: You just changed CKP in the middle of a transfer." << endl;
	
    switch( value & _SSPCON::SSPM_mask ) {
    case _SSPCON::SSPM_SPImaster4:
    case _SSPCON::SSPM_SPImaster16:
    case _SSPCON::SSPM_SPImaster64:
      if (m_SckSource) m_SckSource->putState( (value & _SSPCON::CKP) ? '1' : '0' );
      break;
    case _SSPCON::SSPM_SPImasterTMR2:
      break;
    }
}

/*
	drive SCL by changing pin direction (with data low)
*/
void SSP_MODULE::setSCL(bool direction)
{
    if (!m_sck || !m_i2c_tris)
	return;

    unsigned int pin = m_sck->getPinNumber();
    unsigned int tris_val = m_i2c_tris->get();
    if (!direction)
	tris_val &= ~(1<<pin);
    else
	tris_val |= (1<<pin);

    m_i2c_tris->put_value(tris_val);
}
/*
	drive SDA by changing pin direction (with data low)
*/
void SSP_MODULE::setSDA(bool direction)
{
    unsigned int pin = m_sdi->getPinNumber();
    unsigned int tris_val = m_i2c_tris->get();
    if (!direction)
	tris_val &= ~(1<<pin);
    else
	tris_val |= (1<<pin);

    m_i2c_tris->put_value(tris_val);
}
/*
	deactivate SPI and I2C mode
*/
void SSP_MODULE::stopSSP(unsigned int old_value)
{
    if (sspcon.isSPIActive(old_value))
    {
        m_spi->stop_transfer();
        m_sck->setSource(0);
        m_sdo->setSource(0);

        if (verbose)
      	    cout << "SSP: SPI turned off" << endl;
    }
    else if (sspcon.isI2CActive(old_value))
    {
	m_i2c->set_idle();
        m_sck->setSource(0);
        m_sdi->setSource(0);
        if (verbose)
      	    cout << "SSP: I2C turned off" << endl;
    }
}
/*
	activate SPI module
*/
void SSP_MODULE::startSSP(unsigned int value)
{
    if (verbose)
      cout << "SSP: SPI turned on" << endl;
    sspbuf.setFullFlag(false);
    if (! m_sink_set)
    {
	if (m_sdi) m_sdi->addSink(m_SDI_Sink);
	if (m_sck) m_sck->addSink(m_SCL_Sink);
	if (m_ss) m_ss->addSink(m_SS_Sink);
	m_sink_set = true;
    }
    switch( value & _SSPCON::SSPM_mask ) {
    case _SSPCON::SSPM_SPImasterTMR2:
    case _SSPCON::SSPM_SPImaster4:
    case _SSPCON::SSPM_SPImaster16:
    case _SSPCON::SSPM_SPImaster64:
  	if (m_sck) m_sck->setSource(m_SckSource);
  	if (m_sdo) m_sdo->setSource(m_SdoSource);
        if (m_SckSource) m_SckSource->putState( (value & _SSPCON::CKP) ? '1' : '0' );
	if (m_SdoSource) m_SdoSource->putState('0'); // BUG, required to put SDO in know state
	break;

    case _SSPCON::SSPM_SPIslave:
    case _SSPCON::SSPM_SPIslaveSS:
  	if (m_sdo) m_sdo->setSource(m_SdoSource);
	if (m_SdoSource) m_SdoSource->putState('0'); // BUG, required to put SDO in know state
	break;

      case _SSPCON::SSPM_I2Cslave_7bitaddr:
      case _SSPCON::SSPM_I2Cslave_10bitaddr:
      case _SSPCON::SSPM_MSSPI2Cmaster:
      case _SSPCON::SSPM_I2Cfirmwaremaster:
      case _SSPCON::SSPM_I2Cslave_7bitaddr_ints:
      case _SSPCON::SSPM_I2Cslave_10bitaddr_ints:
	m_i2c->set_idle();
  	m_sck->setSource(m_SckSource);
  	m_sdi->setSource(m_SdiSource);
	m_sck->refreshPinOnUpdate(true);
	m_sdi->refreshPinOnUpdate(true);
	m_SdiSource->putState('0'); 
	m_SckSource->putState('0'); 
	m_sck->refreshPinOnUpdate(false);
	m_sdi->refreshPinOnUpdate(false);
	break;

    default:
	cout << "SSP: start, unexpected SSPM select bits SSPCON="
		<< hex << value << endl;;
	break;
    }
}
/*
	process mode change or clock edge due to write to SSPCON
*/
void SSP_MODULE::changeSSP(unsigned int new_value, unsigned int old_value)
{
    unsigned int diff = new_value ^ old_value;

    if (verbose)
	cout << "SSP_MODULE::changeSSP CKP new=" << hex << new_value << " old=" << old_value << endl;
    if (diff & _SSPCON::SSPM_mask)	// mode changed
    {
	stopSSP(old_value);
	startSSP(new_value);
    }
    else if (diff & _SSPCON::CKP)
    {
	if (sspcon.isSPIActive(new_value))
	    ckpSPI(new_value);
	else if (sspcon.isI2CActive(new_value) && new_value & _SSPCON::CKP)
		setSCL(true);
    }
}

//------------------------------------------------------------
// Called whenever the SDI/SDA input changes states.
// 
void SSP_MODULE::SDI_SinkState(char new3State)
{
  bool new_SDI_State = (new3State == '1' || new3State == 'W');
 
  if (new_SDI_State == m_SDI_State)
	return;

  m_SDI_State = new_SDI_State;

  if(sspcon.isI2CActive(sspcon.value.get()))
  {
      if(m_i2c) m_i2c->sda(m_SDI_State);
   }
}

// Called when the SCK/SDI input changes state
void SSP_MODULE::SCL_SinkState(char new3State)
{
  bool new_SCL_State = (new3State == '1' || new3State == 'W');
 
  if (new_SCL_State == m_SCL_State)
	return;

  m_SCL_State = new_SCL_State;

  if (!sspcon.isSSPEnabled() )
     return;

  switch( sspcon.value.get() & _SSPCON::SSPM_mask ) 
  {
      case _SSPCON::SSPM_SPIslaveSS:
      /*
     	SS high during transfer for BSSP, suspends transfers which
	continues when SS goes low.

	None BSSP interfaces handled when SS goes high  
      */
	if (m_SS_State)
	    return;	// suspend transfer 
			// Fall through
      case _SSPCON::SSPM_SPIslave:
	if (m_spi) m_spi->clock(m_SCL_State);
	break;

      case _SSPCON::SSPM_I2Cslave_7bitaddr:
      case _SSPCON::SSPM_I2Cslave_10bitaddr:
      case _SSPCON::SSPM_MSSPI2Cmaster:
      case _SSPCON::SSPM_I2Cfirmwaremaster:
      case _SSPCON::SSPM_I2Cslave_7bitaddr_ints:
      case _SSPCON::SSPM_I2Cslave_10bitaddr_ints:
	m_i2c->clock(m_SCL_State);
  }


}
/*
	on write to SSPBUF, pass on to either SPI or I2C if active
*/
void SSP_MODULE::newSSPBUF(unsigned int value)
{
  if (!m_spi)
  {
	cout << "Warning bug, SPI initialization error " << __FILE__ << ":" << dec << __LINE__<<endl;
	return;
  }
  if (!m_i2c)
  {
	cout << "Warning bug, I2C initialization error " << __FILE__ << ":" << dec << __LINE__<<endl;
	return;
  }
  if(sspcon.isSPIActive(sspcon.value.get()))
       m_spi->newSSPBUF(value);
  else if(sspcon.isI2CActive(sspcon.value.get()))
       m_i2c->newSSPBUF(value);
}

/*
	on write to SSPADD, pass onto I2C if active
*/
void SSP_MODULE::newSSPADD(unsigned int value)
{
  if(sspcon.isI2CActive(sspcon.value.get()))
       m_i2c->newSSPADD(value);
}
// clear BF flag

void SSP_MODULE::rdSSPBUF()
{
    sspstat.put_value(sspstat.value.get() & ~_SSPSTAT::BF);
}

void SSP_MODULE::SS_SinkState(char new3State)
{
  m_SS_State = (new3State == '1' || new3State == 'W');

  // If SS goes high in the middle of an SPI transfer while in slave_SS mode,
  // transfer is aborted unless BSSP which streches the clocking

  if (!sspcon.isSSPEnabled() || 
	! m_SS_State ||
	(sspcon.value.get() & _SSPCON::SSPM_mask) != _SSPCON::SSPM_SPIslaveSS ||
	! m_spi->isIdle() ||
	ssp_type() == SSP_TYPE_BSSP)
     return;

  m_spi->stop_transfer();
}

void SSP_MODULE::tmr2_clock()
{
  unsigned int sspcon_val = sspcon.value.get();
  if (! (sspcon_val & _SSPCON::SSPEN) || 
     ((sspcon_val & _SSPCON::SSPM_mask) != _SSPCON::SSPM_SPImasterTMR2) ||
     (m_spi && m_spi->isIdle()))
	return;

    Sck_toggle();
    if (m_spi) m_spi->clock( get_SCL_State() );
}
/*
	on write to SSPCON2 select master operation to initiate
*/
void SSP_MODULE::newSSPCON2(unsigned int value)
{
    if (!m_i2c)
	return;

    if(value & _SSPCON2::SEN)
	m_i2c->start_bit();
    else if(value & _SSPCON2::RSEN)
	m_i2c->rstart_bit();
    else if (value & _SSPCON2::PEN)
        m_i2c->stop_bit();
    else if (value & _SSPCON2::RCEN)
        m_i2c->master_rx();
    else if (value & _SSPCON2::ACKEN)
        m_i2c->ack_bit();


}
/*
	Process a received data byte
	    if BF == 0 and SSPOV == 0 return true otherwise false
	    if BF == 0 transfer data to SSPBUF and set BF
	    if BF == 1 set SSPOV
	    set SSPIF
*/
bool SSP_MODULE::SaveSSPsr(unsigned int value)
{
	bool ret = false;
	unsigned int stat_val = sspstat.value.get();
	unsigned int con_val = sspcon.value.get();

	if ((stat_val & _SSPSTAT::BF) == 0)
	{
	    if (verbose)
		cout << "SSP receive transfer " << hex << (value & 0xff) <<
			" to SSPBUF\n";
	    sspbuf.put_value(value);
	    sspstat.put_value(stat_val | _SSPSTAT::BF);
	    if ((con_val & _SSPCON::SSPOV) == 0)
		ret = true;
	}
	else
	{
	    sspcon.put_value(con_val | _SSPCON::SSPOV);
	    cout << "SSP receive overflow\n";
	}

	return(ret);
}
//-----------------------------------------------------------
//-------------------------------------------------------------------
_SSPCON2::_SSPCON2(Processor *pCpu, SSP_MODULE *pSSP)
  : sfr_register(pCpu, "sspcon2","Synchronous Serial Port Control"),
    m_sspmod(pSSP)
{
}

/*
	write to SSPCON2 without processing data
*/
void  _SSPCON2::put_value(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  value.put(new_value);

}
/*
    If a command is currently active, 
	lower 5 bits of register cannot be changed
    if no command is currently active,
	activate command and write data
*/
void _SSPCON2::put(unsigned int new_value)
{
  unsigned int cmd_active = value.get() & (ACKEN|RCEN|PEN|RSEN|SEN);
  if (verbose & 2)
      cout << "_SSPCON2::put " << hex << new_value << endl;
  if (cmd_active)
  {
	put_value((new_value & ~(ACKEN|RCEN|PEN|RSEN|SEN)) | cmd_active);
  }
  else
  {
      	switch (new_value & (ACKEN|RCEN|PEN|RSEN|SEN))
      	{
	case ACKEN:
	case RCEN:
	case PEN:
	case RSEN:
	case SEN:
      	    put_value(new_value);
      	    m_sspmod->newSSPCON2(new_value);
	    break;

	case 0:	// just write value
      	    put_value(new_value);
	    break;

	default:
	    cout << "SSPCON2 cannot select more than one function at a time\n";
	    break;
 	}
  }
	
}

