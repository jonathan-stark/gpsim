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

  put(new_value);

  trace.register_write(address,value.get());

}

//-----------------------------------------------------------
// SSPCON - Synchronous Serial Port Control register.


void _SSPCON::put(unsigned int new_value)
{
  unsigned int diff = value.get() ^ new_value;

  value.put(new_value & 0xff);

  if( (diff & CKP) && (value.get() & SSPEN) ) {
	if( sspbuf && sspbuf->state != _SSPBUF::IDLE )
	  cout << "SSP: You just changed CKP in the middle of a transfer." << endl;
	
	switch( value.get() & SSPM_mask ) {
	case SSPM_SPImaster4:
	case SSPM_SPImaster16:
	case SSPM_SPImaster64:
	  //case SSPM_SPImasterTMP2:
	  sckpin->put_digital_state( (value.get() & CKP) ? true : false );
	}
	
	if(verbose)
	  cout << "SSP: CKP changed" << endl;
  }

  if( (diff & SSPEN) && (value.get() & SSPEN) ) {
	sspbuf->enable();
  }
  
  if( (diff & SSPEN) && !(value.get() & SSPEN) ) {
	if( sspbuf && sspbuf->state != _SSPBUF::IDLE )
	  sspbuf->stop_transfer();
	cout << "SSP: Disabled" << endl;
  }

}

void _SSPCON::put_value(unsigned int new_value)
{

  put(new_value);

  trace.register_write(address,value.get());

}



//-----------------------------------------------------------
// SSPBUF - Synchronous Serial Port Control register.

_SSPBUF::_SSPBUF()
{
  state = IDLE;
  bits_transfered = 0;
  unread = false;

  pirset = 0;

  sckpin = 0;
  sdipin = 0;
  sdopin = 0;
  sspin = 0;

  sspcon = 0;
  sspstat = 0;
}

void _SSPBUF::enable()
{
  cout << "SSP: Make sure the TRIS bits are correct." << endl;   

  
  switch( sspcon->value.get() & _SSPCON::SSPM_mask ) {
  case _SSPCON::SSPM_SPImaster4:
  case _SSPCON::SSPM_SPImaster16:
  case _SSPCON::SSPM_SPImaster64:
	//case _SSPCON::SSPM_SPImasterTMP2:
	sckpin->put_digital_state( (sspcon->value.get() & _SSPCON::CKP) ? true : false );
	break;
  }
  
  state = IDLE;
  bits_transfered = 0;
  unread = false;
  
  cout << "SSP: Enabled" << endl;
}

void _SSPBUF::put(unsigned int new_value)
{

  value.put(new_value & 0xff);

  if( sspcon->value.get() & _SSPCON::SSPEN ) {
	if( state == IDLE ) {
	  switch( sspcon->value.get() & _SSPCON::SSPM_mask ) {
	  case _SSPCON::SSPM_SPImaster4:
	  case _SSPCON::SSPM_SPImaster16:
	  case _SSPCON::SSPM_SPImaster64:
		  //case _SSPCON::SSPM_SPImasterTMP2:
		start_transfer();
		break;
	  case _SSPCON::SSPM_SPIslaveSS:
	  case _SSPCON::SSPM_SPIslave:
		break;
	  default:
		cout << "The selected SSP mode is unimplemented." << endl;
	  }
	} else {
	  // Collision
	  sspcon->value.put(sspcon->value.get() & _SSPCON::WCOL);
	}
  }

}

void _SSPBUF::put_value(unsigned int new_value)
{

  put(new_value);

  trace.register_write(address,value.get());

}

void _SSPBUF::start_transfer()
{
  // load the shift register
  sspsr = value.get();
  state = ACTIVE;
  bits_transfered = 0;

  if( !sspcon ) {
	cout << "SSP: sspbuf is not properly initialized\n" << endl;
	return;
  }	

  cout << "SSP: Starting transfer." << endl;

  switch( sspcon->value.get() & _SSPCON::SSPM_mask ) {
  case _SSPCON::SSPM_SPImaster4:
  case _SSPCON::SSPM_SPImaster16:
  case _SSPCON::SSPM_SPImaster64:
	//case _SSPCON::SSPM_SPImasterTMP2: not implemented
	// Setup callbacks for clocks
	set_halfclock_break( 1 );
	break;
  case _SSPCON::SSPM_SPIslaveSS:
	// The SS pin was pulled low
	if( sspstat->value.get() & _SSPSTAT::CKE ) {
	  sdopin->put_digital_state((sspsr & (1<<7))?true:false); // MSb of shift reg. out
	  cout << "SSP: Sent bit = " << ((sspsr & (1<<7))>>7) << ". (SS)" << endl;
	}
	break;
  case _SSPCON::SSPM_SPIslave:
	// I don't do any thing until first clock edge
	break;
  default:
	cout << "The selected SSP mode is unimplemented." << endl;
  }
  
}

void _SSPBUF::stop_transfer()
{

  if( state == ACTIVE ) {
	if( bits_transfered == 8 && !unread ) {
	  cout << "SSP: Stoping transfer. Normal finish." << endl;
	  
	  value.put(sspsr & 0xff);

	  unread = true;
	  
	  if( pirset )
		pirset->set_sspif();
	  if( sspstat ) {
		cout << "SSP: Setting SSPSTAT BF." << endl;
		sspstat->value.put(sspstat->value.get() | _SSPSTAT::BF);
	  }
	} else if( bits_transfered == 8 && unread ) {
	  cout << "SSP: Stoping transfer. Overflow finish." << endl;
	  
	  sspcon->value.put(sspcon->value.get() | _SSPCON::SSPOV);	  
	} else {
	  cout << "SSP: Stoping transfer. Cancel finish." << endl;
	  // The transfer was canceled in some way
	}
  } else {
	cout << "SSP: Stoping transfer. State != ACTIVE." << endl;
  }

  sckpin->put_digital_state( (sspcon->value.get() & _SSPCON::CKP) ? true : false );
  
  state = IDLE;
  
}

void _SSPBUF::set_halfclock_break( unsigned int clocks )
{
  int clock_in_cycles = 1;

  if( !sspstat || !sspcon ) {
	cout << "SSP: I don't have important registers!!" << endl;
	return;
  }	

  switch( sspcon->value.get() & _SSPCON::SSPM_mask ) {
  case _SSPCON::SSPM_SPImaster4:
	clock_in_cycles = 1;
	cout << "SPI Master Mode at Fosc/4, cannot be implemented at full speed because of an internal design choice! It will run at Fosc/8." << endl;
	break;
  case _SSPCON::SSPM_SPImaster16:
	clock_in_cycles = 2;
	break;
  case _SSPCON::SSPM_SPImaster64:
	clock_in_cycles = 8;
	break;
	//case _SSPCON::SSPM_SPImasterTMP2:
	//break;
  }
  
  cycles.set_break(cycles.value + clocks*clock_in_cycles, this);
}

void _SSPBUF::clock( unsigned int new_value )
{
  // A clock has happened. Either we sent one or we recieved one.
  bool onbeat;

  if( !sspstat || !sspcon ) {
	cout << "SSP: I don't have important registers!!" << endl;
	return;
  }	
  
  if( new_value ) {
	// rising edge
	if( ( (sspcon->value.get() & _SSPCON::CKP) && !(sspstat->value.get() & _SSPSTAT::CKE) )
		|| ( !(sspcon->value.get() & _SSPCON::CKP) && (sspstat->value.get() & _SSPSTAT::CKE) ) )
	  onbeat = true;
	else
	  onbeat = false;
  } else {
	// falling edge
	if( ( !(sspcon->value.get() & _SSPCON::CKP) && !(sspstat->value.get() & _SSPSTAT::CKE) )
		|| ( (sspcon->value.get() & _SSPCON::CKP) && (sspstat->value.get() & _SSPSTAT::CKE)))
	  onbeat = true;
	else
	  onbeat = false;
  }

  if( state == IDLE ){
	if( sspstat->value.get() & _SSPSTAT::CKE ) {
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
	if( !(sspstat->value.get() & _SSPSTAT::SMP) ) {
	  sspsr <<= 1;
	  
	  if( ssp_port->get_bit(sdipin) )
		sspsr |= 1<<0;
	  else
		sspsr &= ~(1<<0);
	  
	  cout << "SSP: Received bit = " << (sspsr & 1) << ". (SMP=0)" << endl;
	}
  } else {
	// off beat: data is shifted out, data is read in if SMP = 1

	if( sspstat->value.get() & _SSPSTAT::SMP ) {
	  sspsr <<= 1;
	  sspsr &= 0xff;
	  
	  if( ssp_port->get_bit(sdipin) )
		sspsr |= 1<<0;
	  else
		sspsr &= ~(1<<0);

	  cout << "SSP: Received bit = " << (sspsr & 1) << ". (SMP=1)" << endl;
	}
	
	sdopin->put_digital_state((sspsr & (1<<7))?true:false); // MSb of shift reg. out
	cout << "SSP: Sent bit = " << ((sspsr & (1<<7))>>7) << "." << endl;
  }

  bool bNewValue = new_value ? true : false;
  bool bSSPCONValue = (sspcon->value.get() & _SSPCON::CKP) ? true : false;
  if(bSSPCONValue == bNewValue) {
	bits_transfered++;
	if( bits_transfered == 8 ) {
	  if( (sspstat->value.get() & _SSPSTAT::SMP) && !(sspstat->value.get() & _SSPSTAT::CKE) ) {
		state = WAITING_FOR_LAST_SMP;
		set_halfclock_break( 1 );
	  } else {
		stop_transfer();
	  }
	}
  }

  
}

void _SSPBUF::assign_pir_set(PIR_SET *ps)
{

  pirset = ps;

}

void _SSPBUF::callback(void)
{
  
  switch( state ) {
  case IDLE:
	// What am I doing here?
	break;
  case ACTIVE:
	sckpin->toggle();
	//clock( sckpin->get_state() );
	set_halfclock_break( 1 );
	break;
  case WAITING_FOR_LAST_SMP:
	if( sspstat && sspstat->value.get() & _SSPSTAT::SMP ) {
	  sspsr <<= 1;
	  
	  if( ssp_port->get_bit(sdipin) )
		sspsr |= 1<<0;
	  else
		sspsr &= ~(1<<0);

	  cout << "SSP: Received bit = " << (sspsr & 1) << ". (SMP=1)" << endl;
	}
	
	state = ACTIVE;
	stop_transfer();
	break;
  }
	  
}

unsigned int _SSPBUF::get(void)
{
  // FIX: How do I detect when the PIC code reads the register as different from the GUI or something reads it?
  // Currently the code cannot detect buffer overruns because of this.
  
  /*
  // FIX: When does BF get turn off?
  if( sspstat )
	sspstat->value &= ~_SSPSTAT::BF; // FIX: I don't know if this happens

  */
  
  unread = false;

  return value.get();
}
  
unsigned int _SSPBUF::get_value(void)
{
  trace.register_read(address, get());
  return value.get();
}


//-----------------------------------------------------------
// SSPBUF - Synchronous Serial Port Control register.


void _SSPADD::put(unsigned int new_value)
{
  cout << "SSPADD in unimplemented, as is all of I2C." << endl;

  value.put(new_value & 0xff);

}

void _SSPADD::put_value(unsigned int new_value)
{
  cout << "SSPADD in unimplemented, as is all of I2C." << endl;

  put(new_value);

  trace.register_write(address,value.get());

}

//-----------------------------------------------------------
SSP_MODULE::SSP_MODULE(void)
{
  sspbuf = new _SSPBUF;
  sspstat = new _SSPSTAT;
  sspcon = new _SSPCON;
  sspadd = new _SSPADD;
}

void SSP_MODULE::initialize(PIR_SET *ps, IOPORT *ssp_port, int sck_pin, int sdi_pin, int sdo_pin,
			    IOPORT *ss_port, int ss_pin, SSP_TYPE ssp_type)
{
  sspbuf->ssp_port = ssp_port;
  sspbuf->sdipin = sdi_pin;
  sspbuf->sckpin = ssp_port->pins[sck_pin];
  sspbuf->sdopin = ssp_port->pins[sdo_pin];
  sspbuf->sspin = ss_port->pins[ss_pin];
  sspbuf->ssptype = ssp_type;
  sspbuf->assign_pir_set(ps);

  sspbuf->sspcon = sspcon;
  sspbuf->sspstat = sspstat;

  sspcon->sckpin = ssp_port->pins[sck_pin];
  sspcon->sspbuf = sspbuf;
	
  sspstat->ssptype = ssp_type;
}

void SSP_MODULE::new_sck_edge(unsigned int)
{
}

void SSP_MODULE::new_ss_edge(unsigned int)
{
}


//-----------------------------------------------------------
SSP_MODULE14::SSP_MODULE14(void)
{
}
 

void SSP_MODULE14::initialize_14(_14bit_processor *new_cpu, PIR_SET *ps, IOPORT *ssp_port, int sck_pin, int sdi_pin, int sdo_pin,
								 IOPORT *ss_port, int ss_pin, SSP_TYPE ssp_type)
{
  cpu = new_cpu;
  
  SSP_MODULE::initialize(ps, ssp_port, sck_pin, sdi_pin, sdo_pin, ss_port, ss_pin, ssp_type);
}

void SSP_MODULE14::new_sck_edge(unsigned int value)
{

  if( sspcon && sspcon->value.get() & _SSPCON::SSPEN ) {
	sspbuf->clock( value );
  }

}

void SSP_MODULE14::new_ss_edge(unsigned int value)
{

  if( sspcon && sspcon->value.get() & _SSPCON::SSPEN ) {
	sspbuf->start_transfer();
  }

}
