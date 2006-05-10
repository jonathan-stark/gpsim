/*
   Copyright (C) 1998-2003 Scott Dattalo


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

#include <glib.h>

#include "trace.h"
#include "pir.h"
#include "intcon.h"

PIR::PIR(INTCON *_intcon, PIE *_pie, int _valid_bits)
  : intcon(_intcon),pie(_pie),valid_bits(_valid_bits)  ,writable_bits(0)
{
}


void PIR::put(unsigned int new_value)
{
  // Only the "writable bits" can be written with put.
  // The "read-only" ones (such as TXIF) are written
  // through the set_/clear_ member functions.

  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());
  value.put(new_value & writable_bits | value.get() & ~writable_bits);

  if( value.get() & pie->value.get() )
    intcon->peripheral_interrupt();

}

void PIR::set_intcon(INTCON *_intcon)
{
  intcon = _intcon;
}


void PIR::set_pie(PIE *_pie)
{
  pie = _pie;
}

void PIR::setInterrupt(unsigned int bitMask)
{
  put(value.get() | bitMask);
}


void PIR::setPeripheralInterrupt()
{
  if (intcon)
    intcon->peripheral_interrupt();
}
bool PIR::interrupt_status()
{
  assert(pie);
  if( value.get() & valid_bits & pie->value.get())
    return true;

  return false;
}


InterruptSource::InterruptSource(PIR *_pir, unsigned int bitMask)
  : m_pir(_pir), m_bitMask(bitMask)
{
  assert(m_pir);
  // Only one bit in the bit mask should be set.
  assert(m_bitMask && ((m_bitMask & (m_bitMask-1)) == 0));
}

void InterruptSource::Trigger()
{
  m_pir->setInterrupt(m_bitMask);
}






//------------------------------------------------------------------------

PIR1v1::PIR1v1(INTCON *_intcon, PIE *_pie)
  : PIR(_intcon, _pie,0)
{
  // Even though TXIF is a valid bit, it can't be written by the PIC
  // source code.  Its state reflects whether the usart txreg is full
  // or not. Similarly for RCIF
  valid_bits = TMR1IF | TMR2IF | CCP1IF | SSPIF | TXIF | RCIF | CMIF | EEIF;
  writable_bits = TMR1IF | TMR2IF | CCP1IF | SSPIF | CMIF | EEIF;
}


void PIR1v1::clear_sspif(void)
{
  trace.raw(write_trace.get() | value.get());
  value.put(value.get() & ~SSPIF);
}

void PIR1v1::set_txif(void)
{
  trace.raw(write_trace.get() | value.get());
  value.put(value.get() | TXIF);
  if( value.get() & pie->value.get() )
    intcon->peripheral_interrupt();
}

void PIR1v1::clear_txif(void)
{
  trace.raw(write_trace.get() | value.get());
  value.put(value.get() & ~TXIF);
}

void PIR1v1::clear_rcif(void)
{
  trace.raw(write_trace.get() | value.get());
  value.put(value.get() & ~RCIF);
}

void PIR1v1::set_cmif(void)
{
  trace.raw(write_trace.get() | value.get());
  value.put(value.get() | CMIF);
  if( value.get() & pie->value.get() )
    intcon->peripheral_interrupt();
}
//------------------------------------------------------------------------
//
PIR1v2::PIR1v2(INTCON *_intcon, PIE *_pie)
  : PIR(_intcon, _pie,0)
{
  // Even though TXIF is a valid bit, it can't be written by the PIC
  // source code.  Its state reflects whether the usart txreg is full
  // or not.
  valid_bits = TMR1IF | TMR2IF | CCP1IF | SSPIF | TXIF | RCIF | ADIF | PSPIF;
  writable_bits = TMR1IF | TMR2IF | CCP1IF | SSPIF | RCIF | ADIF | PSPIF;
}

void PIR1v2::clear_sspif(void)
{
  trace.raw(write_trace.get() | value.get());
  value.put(value.get() & ~SSPIF);
}

void PIR1v2::set_txif(void)
{
  trace.raw(write_trace.get() | value.get());
  value.put(value.get() | TXIF);
  if( value.get() & pie->value.get() )
    intcon->peripheral_interrupt();
}

void PIR1v2::clear_txif(void)
{
  trace.raw(write_trace.get() | value.get());
  value.put(value.get() & ~TXIF);
}

void PIR1v2::clear_rcif(void)
{
  trace.raw(write_trace.get() | value.get());
  value.put(value.get() & ~RCIF);
}

//------------------------------------------------------------------------
PIR2v1::PIR2v1(INTCON *_intcon, PIE *_pie)
  : PIR(_intcon, _pie,0)
{
  valid_bits = CCP2IF;
  writable_bits = valid_bits;
}
//------------------------------------------------------------------------
PIR2v2::PIR2v2(INTCON *_intcon, PIE *_pie)
  : PIR(_intcon, _pie,0)
{
  valid_bits = ECCP1IF | TMR3IF | LVDIF | BCLIF | EEIF | CMIF;
  writable_bits = valid_bits;
}

//------------------------------------------------------------------------
PIR3v2::PIR3v2(INTCON *_intcon, PIE *_pie)
  : PIR(_intcon, _pie,0)
{
  valid_bits = RXB0IF | RXB1IF | TXB0IF | TXB1IF | TXB2IF | ERRIF |
    WAKIF | IRXIF;
  writable_bits = valid_bits;
}
