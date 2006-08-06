/*
   Copyright (C) 1998,1999,2000,2001,2002 Scott Dattalo
	         2006 Roy R Rankin

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
#include <iostream>

#include "../config.h"
#include "ssp.h"
#include "stimuli.h"
#include "14bit-processors.h"
#include "14bit-tmrs.h"

#include "xref.h"

//#warning only supports SPI mode.
//-----------------------------------------------------------
// SSPSTAT - Synchronous Serial Port Status register.


void _SSPSTAT::put(unsigned int new_value)
{
  unsigned int old6 = value.get() & ~(CKE|SMP);

  // For BSSP register is read only otherwise
  // only CKE and SMP are writable

  if (m_sspmod->ssp_type() == SSP_TYPE_BSSP)
	return;

  put_value(old6 | (new_value & (CKE|SMP)));


}

void _SSPSTAT::put_value(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  value.put(new_value);

}
void _SSPSTAT::setSSPMODULE(SSP_MODULE *sspmod)
{
  m_sspmod = sspmod;
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

_SSPCON::_SSPCON()
{
}
void _SSPCON::setSSPMODULE(SSP_MODULE *sspmod)
{
  m_sspmod = sspmod;
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
void _SSPCON::put(unsigned int new_value)
{
  unsigned int old_value = value.get();
  unsigned int diff = value.get() ^ new_value;

  put_value(new_value);

  if (isSPIActive(new_value))
  {
       if (!isSPIActive(old_value))	// SPI being turned on
	    m_sspmod->startSPI(new_value);
       else if (diff & CKP)		// CKP change
	    m_sspmod->ckpSPI(new_value);
  }
  else if (isSPIActive(old_value))
  {
	m_sspmod->stopSPI(new_value);
  }
}

void _SSPCON::put_value(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  value.put(new_value & 0xff);

}

void _SSPCON::setWCOL()
{
  if (value.get() & WCOL)
    return;
  value.put(value.get() | WCOL);
  trace.raw(write_trace.get() | value.get());

}
void _SSPCON::setSSPOV()
{
  if (value.get() & SSPOV)
    return;
  value.put(value.get() | SSPOV);
  trace.raw(write_trace.get() | value.get());

}





bool _SSPCON::isI2CEnabled() 
{
    unsigned int val = value.get();
    if ( (val & SSPEN) != SSPEN)
	return(false);
    switch(val & SSPM_mask)
    {
      case SSPM_I2Cslave_7bitaddr:
      case SSPM_I2Cslave_10bitaddr:
      case SSPM_MSSPI2Cmaster:
      case SSPM_I2Cfirmwaremaster:
      case SSPM_I2Cfirmwaremultimaster_7bitaddr_ints:
      case SSPM_I2Cfirmwaremaster_10bitaddr_ints:
	return(true);
	break;
    }
    return(false);
}

//-----------------------------------------------------------
// SSPBUF - Synchronous Serial Port Control register.
//-----------------------------------------------------------

_SSPBUF::_SSPBUF()
  :  m_sspmod(0), m_bIsFull(false)
{
}

void _SSPBUF::put(unsigned int new_value)
{

  value.put(new_value & 0xff);

  m_sspmod->newSSPBUF(value.get());
}

void _SSPBUF::put_value(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  put(new_value);
}

void _SSPBUF::putFromSSPSR(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  value.put(new_value & 0xff);
  m_bIsFull = true;
}

void _SSPBUF::setSSPMODULE( SSP_MODULE *_sspmod)
{
  m_sspmod = _sspmod;
}

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


void _SSPADD::put(unsigned int new_value)
{
  cout << "SSPADD in unimplemented, as is all of I2C." << endl;

  value.put(new_value & 0xff);

}

void _SSPADD::put_value(unsigned int new_value)
{
  cout << "SSPADD in unimplemented, as is all of I2C." << endl;

  trace.raw(write_trace.get() | value.get());
  put(new_value);

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
  if( !m_sspstat || ! m_sspcon)
    return;
  unsigned int sspstat_val = m_sspstat->value.get();
  unsigned int sspcon_val = m_sspcon->value.get();

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
      if (verbose)
      	cout << "SPI clock called start_transfer\n";
      start_transfer();
    }

  }
 
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
	set_halfclock_break();
      } 
      else 
      {
	stop_transfer();
      }
    }
  }
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
  
  get_cycles().set_break(get_cycles().value + clock_in_cycles, this);
}
void SPI::callback()
{
  
  switch( m_state ) {
  case eIDLE:
    break;
  case eACTIVE:
    m_sspmod->Sck_toggle();
    clock( m_sspmod->get_SCL_State() );
    set_halfclock_break();
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

SSP_MODULE::SSP_MODULE() : m_sink_set(false)
{
  sspbuf.setSSPMODULE(this);
  sspcon.setSSPMODULE(this);
  sspstat.setSSPMODULE(this);

}
SSP_MODULE::~SSP_MODULE(){}

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
    // Setup callbacks for clocks
    set_halfclock_break();
    break;
  case _SSPCON::SSPM_SPImasterTMR2: 
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
    cout << "The selected SPI mode is unimplemented. mode=" << hex
	<<(sspcon_val & _SSPCON::SSPM_mask) << endl;
  }
  
}
void SPI::stop_transfer()
{
  if (!m_sspcon)
	return;
  unsigned int sspstat_val = m_sspstat->value.get();

  if( m_state == eACTIVE ) {
    if( bits_transfered == 8 && !m_sspbuf->isFull() ) {
      if (verbose)
      	cout << "SPI: Stoping transfer. Normal finish. Setting sspif and BF" << endl;
      m_sspbuf->putFromSSPSR(m_SSPsr & 0xff);
      m_sspmod->set_sspif();

      m_sspstat->put_value(sspstat_val | _SSPSTAT::BF);

    } else if( bits_transfered == 8 && m_sspbuf->isFull() ) {
      cout << "SPI: Stopping transfer. SSPBUF Overflow set SSPOV." << endl;
	m_sspcon->setSSPOV();  
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

void SSP_MODULE::initialize(
		        PIR_SET *ps,
			PinModule *SckPin,
			PinModule *SsPin,
			PinModule *SdoPin,
			PinModule *SdiPin,
			SSP_TYPE _ssptype
		)
{
  m_pirset = ps;
  m_sck = SckPin;
  m_ss = SsPin;
  m_sdo = SdoPin;
  m_sdi = SdiPin;
  m_ssptype = _ssptype;
  m_SckSource = new PeripheralSignalSource(m_sck);
  m_SdoSource = new PeripheralSignalSource(m_sdo);
  if (! m_spi)
  {
    m_spi = new SPI(this, &sspcon, &sspstat, &sspbuf);
    m_SDI_Sink = new SDI_SignalSink(this);
    m_SCL_Sink = new SCL_SignalSink(this);
    m_SS_Sink = new SS_SignalSink(this);
  }


}
void SSP_MODULE::ckpSPI(unsigned int value)
{
    if( !m_spi->isIdle())
      cout << "SPI: You just changed CKP in the middle of a transfer." << endl;
	
    switch( value & _SSPCON::SSPM_mask ) {
    case _SSPCON::SSPM_SPImaster4:
    case _SSPCON::SSPM_SPImaster16:
    case _SSPCON::SSPM_SPImaster64:
      m_SckSource->putState( (value & _SSPCON::CKP) ? '1' : '0' );
      break;
    case _SSPCON::SSPM_SPImasterTMR2:
      break;
    }
}

void SSP_MODULE::stopSPI(unsigned int value)
{
    m_spi->stop_transfer();
    m_sck->setSource(0);
    m_sdo->setSource(0);
    if (verbose)
      cout << "SSP: SPI turned off" << endl;
}
void SSP_MODULE::startSPI(unsigned int value)
{
    if (verbose)
      cout << "SSP: SPI turned on" << endl;
    sspbuf.setFullFlag(false);
    if (! m_sink_set)
    {
	m_sdi->addSink(m_SDI_Sink);
	m_sck->addSink(m_SCL_Sink);
	m_ss->addSink(m_SS_Sink);
	m_sink_set = true;
    }
    switch( value & _SSPCON::SSPM_mask ) {
    case _SSPCON::SSPM_SPImasterTMR2:
    case _SSPCON::SSPM_SPImaster4:
    case _SSPCON::SSPM_SPImaster16:
    case _SSPCON::SSPM_SPImaster64:
  	m_sck->setSource(m_SckSource);
  	m_sdo->setSource(m_SdoSource);
        m_SckSource->putState( (value & _SSPCON::CKP) ? '1' : '0' );
	m_SdoSource->putState('0'); // BUG, required to put SDO in know state
	break;

    case _SSPCON::SSPM_SPIslave:
    case _SSPCON::SSPM_SPIslaveSS:
  	m_sdo->setSource(m_SdoSource);
	m_SdoSource->putState('0'); // BUG, required to put SDO in know state
	break;

    default:
	cout << "SSP: SPI start, unexpected SSPM select bits SSPCON="
		<< hex << value << endl;;
	break;
    }
}
//------------------------------------------------------------
// Called whenever the SDI/SDA input changes states.
// 
void SSP_MODULE::SDI_SinkState(char new3State)
{
  m_SDI_State = (new3State == '1' || new3State == 'W');
}

// Called when the SCK/SCL input changes state
void SSP_MODULE::SCL_SinkState(char new3State)
{

  m_SCL_State = (new3State == '1' || new3State == 'W');
  if (!sspcon.isSSPEnabled())
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
	m_spi->clock(m_SCL_State);
	break;
  }


}
void SSP_MODULE::newSSPBUF(unsigned int value)
{
   m_spi->newSSPBUF(value);
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
     m_spi->isIdle())
	return;

    Sck_toggle();
    m_spi->clock( get_SCL_State() );
}
//-----------------------------------------------------------
//-------------------------------------------------------------------

void  _SSPCON2::put(unsigned int new_value)
{

  cout << "SSPCON2 is not implemented\n";

  sfr_register::put(new_value);
}

_SSPCON2::_SSPCON2(void)
{
}
 
