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

void PIR::put(unsigned int new_value)
{
  // Only the "valid bits" can be written with put.
  // The "invalid" ones (such as TXIF) are written
  // through the set_/clear_ member functions.

  value = new_value & valid_bits | value & ~valid_bits;
  trace.register_write(address,value);

  if( value & pie->value )
    intcon->peripheral_interrupt();

}


void PIR1v1::clear_sspif(void)
{
  value &= ~SSPIF;
  trace.register_write(address,value);
}

void PIR1v1::clear_txif(void)
{
  value &= ~TXIF;
  trace.register_write(address,value);
}

void PIR1v1::clear_rcif(void)
{
  value &= ~RCIF;
  trace.register_write(address,value);
}





void PIR1v2::clear_sspif(void)
{
  value &= ~SSPIF;
  trace.register_write(address,value);
}

void PIR1v2::clear_txif(void)
{
  value &= ~TXIF;
  trace.register_write(address,value);
}

void PIR1v2::clear_rcif(void)
{
  value &= ~RCIF;
  trace.register_write(address,value);
}


