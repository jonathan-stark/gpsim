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
#include <iostream.h>

#include "../config.h"
#include "ssp.h"
#include "14bit-processors.h"
#include "14bit-tmrs.h"

#include "xref.h"

//-----------------------------------------------------------
// SSPSTAT - Synchronous Serial Port Status register.


void _SSPSTAT::put(unsigned int new_value)
{

  value = new_value & 0xff;

}

void _SSPSTAT::put_value(unsigned int new_value)
{

  put(new_value);

  trace.register_write(address,value);

}

//-----------------------------------------------------------
// SSPCON - Synchronous Serial Port Control register.


void _SSPCON::put(unsigned int new_value)
{

  value = new_value & 0xff;

}

void _SSPCON::put_value(unsigned int new_value)
{

  put(new_value);

  trace.register_write(address,value);

}



//-----------------------------------------------------------
// SSPBUF - Synchronous Serial Port Control register.


void _SSPBUF::put(unsigned int new_value)
{

  value = new_value & 0xff;

}

void _SSPBUF::put_value(unsigned int new_value)
{

  put(new_value);

  trace.register_write(address,value);

}


//-----------------------------------------------------------
// SSPBUF - Synchronous Serial Port Control register.


void _SSPADD::put(unsigned int new_value)
{

  value = new_value & 0xff;

}

void _SSPADD::put_value(unsigned int new_value)
{

  put(new_value);

  trace.register_write(address,value);

}

//-----------------------------------------------------------
SSP_MODULE::SSP_MODULE(void)
{
  sspbuf = new _SSPBUF;
  sspstat = new _SSPSTAT;
  sspcon = new _SSPCON;
  sspadd = new _SSPADD;
}

void SSP_MODULE::initialize(IOPORT *ssp_port)
{

}


//-----------------------------------------------------------
SSP_MODULE14::SSP_MODULE14(void)
{


}
 

void SSP_MODULE14::initialize(_14bit_processor *new_cpu)
{

  cpu = new_cpu;

}
