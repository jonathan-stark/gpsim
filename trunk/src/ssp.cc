/*
   Copyright (C) 1998,1999,2000,2001,2002 Scott Dattalo

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
  unsigned int diff = value.get() ^ new_value;

  value.put(new_value & 0xff);

  // FIX: How can I protect the lower six bits from being changed? They are read only.
  /*
  if( ssptype == SSP_TYPE_SSP || ssptype == SSP_TYPE_MSSP )
	value = new_value & (CKE|SMP); // these are the only writable bits and they are only availiable on SSP and MSSP, and not on BSSP
	*/

  if( (ssptype == SSP_TYPE_BSSP) && ((diff & ~(CKE|SMP)) != 0) )
	cout << "Write to invalid bits in SSPSTAT!!" << endl;

}

void _SSPSTAT::put_value(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  put(new_value);

}



//-----------------------------------------------------------
// SSPCON - Synchronous Serial Port Control register.
//-----------------------------------------------------------

_SSPCON::_SSPCON()
  :  m_SckSource(0), m_SsSource(0),
     m_sspbuf(0)
{
}
void _SSPCON::setIOpins(PinModule *_sck,PinModule *_ss,
			PinModule *_sdo, PinModule *_sdi)
{
  m_SckSource = new PeripheralSignalSource(_sck);
  m_SsSource  = new PeripheralSignalSource(_ss);
  m_SdoSource = new PeripheralSignalSource(_sdo);
  _sdi->addSink(this);

}
void _SSPCON::setSSPBUF(_SSPBUF *sspbuf)
{
  m_sspbuf = sspbuf;
}
void _SSPCON::setSSPSTAT(_SSPSTAT *sspstat)
{
  m_sspstat = sspstat;
}

void _SSPCON::put(unsigned int new_value)
{
  unsigned int diff = value.get() ^ new_value;

  value.put(new_value & 0xff);

  if( (diff & CKP) && (value.get() & SSPEN) ) {
    if( m_state != eIDLE )
      cout << "SSP: You just changed CKP in the middle of a transfer." << endl;
	
    switch( value.get() & SSPM_mask ) {
    case SSPM_SPImaster4:
    case SSPM_SPImaster16:
    case SSPM_SPImaster64:
      //case SSPM_SPImasterTMP2:
      //sckpin->putState( (value.get() & CKP) ? true : false );
      m_SckSource->putState( (value.get() & CKP) ? '1' : '0' );
    }
	
    if(verbose)
      cout << "SSP: CKP changed" << endl;
  }

  if( (diff & SSPEN) && (value.get() & SSPEN) ) {
    enable();

    switch( value.get() & SSPM_mask ) {
    case SSPM_SPImaster4:
    case SSPM_SPImaster16:
    case SSPM_SPImaster64:
      //case SSPM_SPImasterTMP2:
      //sckpin->putState( (sspcon->value.get() & _SSPCON::CKP) ? true : false );
      m_SckSource->putState( (value.get() & CKP) ? '1' : '0' );
      break;
    }
  }
  
  if( (diff & SSPEN) && !(value.get() & SSPEN) ) {
    // if( sspbuf && sspbuf->state != _SSPBUF::IDLE )
    stop_transfer();
    cout << "SSP: Disabled" << endl;
  }

}

//------------------------------------------------------------
// setState
// Called whenever the SDI input changes states.
// 
void _SSPCON::setSinkState(const char new3State)
{
  m_cSDIState = new3State;
}

void _SSPCON::setWCOL()
{
  if (value.get() & WCOL)
    return;
  value.put(value.get() | WCOL);
  trace.raw(write_trace.get() | value.get());

}

void _SSPCON::put_value(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  put(new_value);

}

void _SSPCON::newSSPBUF(unsigned int newTxByte)
{
  if (isSSPEnabled()) {
    if (m_state == eIDLE) {
      if (isSPIMaster()) {
	m_SSPsr = newTxByte;
	start_transfer();
      } else
	cout << "The selected SSP mode is unimplemented." << endl;
    } else {
      // Collision
      setWCOL();
    }
  } else if (isI2CEnabled() ) {
    cout << " I2C is not implemented...\n";
  }
}

//-----------------------------------------------------------
// SSPBUF - Synchronous Serial Port Control register.
//-----------------------------------------------------------

_SSPBUF::_SSPBUF()
  :  m_sspcon(0), m_bIsFull(false)
{
}

void _SSPCON::enable()
{
  cout << "SSP: Make sure the TRIS bits are correct." << endl;   

  
  m_state = eIDLE;
  bits_transfered = 0;
  m_sspbuf->setFullFlag(false);
}

void _SSPBUF::put(unsigned int new_value)
{

  value.put(new_value & 0xff);

  m_sspcon->newSSPBUF(value.get());

  /*
  if( state == IDLE ) {
    if (m_sspcon->isSPIMaster())
      start_transfer();
    else
      cout << "The selected SSP mode is unimplemented." << endl;
  } else {
    // Collision
    m_sspcon->setWCOL();
  }
  */

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

void _SSPBUF::setSSPCON( _SSPCON *nSSPCON)
{
  m_sspcon = nSSPCON;
}
void _SSPCON::start_transfer()
{
  // load the shift register
  m_state = eACTIVE;
  bits_transfered = 0;

  cout << "SSP: Starting transfer." << endl;

  switch( value.get() & SSPM_mask ) {
  case _SSPCON::SSPM_SPImaster4:
  case _SSPCON::SSPM_SPImaster16:
  case _SSPCON::SSPM_SPImaster64:
    //case _SSPCON::SSPM_SPImasterTMP2: not implemented
    // Setup callbacks for clocks
    set_halfclock_break( 1 );
    break;
  case _SSPCON::SSPM_SPIslaveSS:
    // The SS pin was pulled low
    if( m_sspstat->value.get() & _SSPSTAT::CKE )
      m_SdoSource->putState((m_SSPsr &(1<<7)) ? '1' : '0');
    break;
  case _SSPCON::SSPM_SPIslave:
    // I don't do any thing until first clock edge
    break;
  default:
    cout << "The selected SSP mode is unimplemented." << endl;
  }
  
}

void _SSPCON::stop_transfer()
{

  if( m_state == eACTIVE ) {
    if( bits_transfered == 8 && !m_bUnread ) {
      cout << "SSP: Stoping transfer. Normal finish." << endl;
	  
      m_sspbuf->putFromSSPSR(m_SSPsr & 0xff);

      if( m_pirset )
	m_pirset->set_sspif();
      if( m_sspstat ) {
	cout << "SSP: Setting SSPSTAT BF." << endl;
	m_sspstat->value.put(m_sspstat->value.get() | _SSPSTAT::BF);
      }
    } else if( bits_transfered == 8 && m_sspbuf->isFull() ) {
      cout << "SSP: Stopping transfer. Overflow finish." << endl;
	  
      value.put(value.get() | SSPOV);	  
    } else {
      cout << "SSP: Stopping transfer. Cancel finish." << endl;
      // The transfer was canceled in some way
    }
  } else {
    cout << "SSP: Stopping transfer. State != ACTIVE." << endl;
  }

  //sckpin->putState( (sspcon->value.get() & _SSPCON::CKP) ? true : false );
  m_SckSource->putState((value.get() & CKP) ? '1' : '0');
  m_state = eIDLE;
  
}

void _SSPCON::set_halfclock_break( unsigned int clocks )
{
  int clock_in_cycles = 1;

  if( !m_sspstat )
    return;

  switch( value.get() & SSPM_mask ) {
  case SSPM_SPImaster4:
    clock_in_cycles = 1;
    cout << "SPI Master Mode at Fosc/4, cannot be implemented at full speed because of an internal design choice! It will run at Fosc/8." << endl;
    break;
  case SSPM_SPImaster16:
    clock_in_cycles = 2;
    break;
  case SSPM_SPImaster64:
    clock_in_cycles = 8;
    break;
    //case _SSPCON::SSPM_SPImasterTMP2:
    //break;
  }
  
  get_cycles().set_break(get_cycles().value + clocks*clock_in_cycles, this);
}

void _SSPCON::clock( const char newClockState )
{
  // A clock has happened. Either we sent one or we recieved one.
  bool onbeat;

  if( !m_sspstat)
    return;

  bool bRisingEdge = (newClockState =='1') || (newClockState =='W');
  if( bRisingEdge ) {
    // rising edge
    if( ( (value.get() & CKP) && !(m_sspstat->value.get() & _SSPSTAT::CKE) )
	|| ( !(value.get() & CKP) && (m_sspstat->value.get() & _SSPSTAT::CKE) ) )
      onbeat = true;
    else
      onbeat = false;
  } else {
    // falling edge
    if( ( !(value.get() & CKP) && !(m_sspstat->value.get() & _SSPSTAT::CKE) )
	|| ( (value.get() & CKP) && (m_sspstat->value.get() & _SSPSTAT::CKE)))
      onbeat = true;
    else
      onbeat = false;
  }

  if( m_state == eIDLE ){
    if( m_sspstat->value.get() & _SSPSTAT::CKE ) {
      // FIX: I have NOT verified that PICs actually behave like this.
      cout << "SSP: I can't handle a non-started transfer with CKE = 1." << endl;
      return;
    } else if( onbeat ) {
      // FIX: I have NOT verified that PICs actually behave like this.
      cout << "SSP: Ignoring clock transition to neutral in state IDLE." << endl;
      return;
    } else {
      start_transfer();
    }
  }
 
  if( onbeat ) {
    // on beat: data is read in if SMP = 0
    if( !(m_sspstat->value.get() & _SSPSTAT::SMP) ) {
      m_SSPsr <<= 1;
      m_SSPsr |= ((m_cSDIState == '1') || (m_cSDIState == 'W'));
      cout << "SSP: Received bit = " << (m_SSPsr & 1) << ". (SMP=0)" << endl;
    }
  } else {
    // off beat: data is shifted out, data is read in if SMP = 1

    if( m_sspstat->value.get() & _SSPSTAT::SMP ) {
      m_SSPsr <<= 1;
      m_SSPsr |= ((m_cSDIState == '1') || (m_cSDIState == 'W'));

      cout << "SSP: Received bit = " << (m_SSPsr & 1) << ". (SMP=1)" << endl;
    }
	
    //sdopin->putState((sspsr & (1<<7))?true:false); // MSb of shift reg. out
    char nextSDO = (m_SSPsr&(1<<7)) ? '1' : '0';
    m_SdoSource->putState(nextSDO);
    cout << "SSP: Sent bit = " << nextSDO << "." << endl;
  }

  bool bSSPCONValue = (value.get() & CKP) ? true : false;
  if(bSSPCONValue == bRisingEdge) {
    bits_transfered++;
    if( bits_transfered == 8 ) {
      if( (m_sspstat->value.get() & _SSPSTAT::SMP) && !(m_sspstat->value.get() & _SSPSTAT::CKE) ) {
	m_state = eWAITING_FOR_LAST_SMP;
	set_halfclock_break( 1 );
      } else {
	stop_transfer();
      }
    }
  }

  
}

void _SSPCON::assign_pir_set(PIR_SET *ps)
{

  m_pirset = ps;

}

void _SSPCON::callback()
{
  
  switch( m_state ) {
  case eIDLE:
    break;
  case eACTIVE:
    m_SckSource->toggle();
    //sckpin->toggle();
    //clock( sckpin->get_state() );
    set_halfclock_break( 1 );
    break;
  case eWAITING_FOR_LAST_SMP:
    if( m_sspstat && m_sspstat->value.get() & _SSPSTAT::SMP ) {
      m_SSPsr <<= 1;
      m_SSPsr |= ((m_cSDIState=='1') || (m_cSDIState=='W'));

      cout << "SSP: Received bit = " << (m_SSPsr & 1) << ". (SMP=1)" << endl;
    }
	
    m_state = eACTIVE;
    stop_transfer();
    break;
  }
	  
}

unsigned int _SSPBUF::get()
{
  /*
  // FIX: When does BF get turn off?
  if( sspstat )
	sspstat->value &= ~_SSPSTAT::BF; // FIX: I don't know if this happens

  */
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

//-----------------------------------------------------------
SSP_MODULE::SSP_MODULE()
{
  sspbuf.setSSPCON(&sspcon);
  sspcon.setSSPBUF(&sspbuf);
  sspcon.setSSPSTAT(&sspstat);

}


void SSP_MODULE::initialize(PIR_SET *ps,
			    PinModule *SckPin,
			    PinModule *SsPin,
			    PinModule *SdoPin,
			    PinModule *SdiPin)
{
  sspcon.assign_pir_set(ps);
  sspcon.setIOpins(SckPin,SsPin,SdoPin,SdiPin);

}

//-----------------------------------------------------------
SSP_MODULE14::SSP_MODULE14()
  : cpu(0)
{
}
 
