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


/*
  packages.h

Here's where all of the pic packages are defined

 */


#ifndef __PIC_PACKAGES_H__
#define __PIC_PACKAGES_H__

#include "gpsim_classes.h"
#include "pic-processor.h"
#include "stimuli.h"
#include "packages.h"



class _18pins  : public Package
{
public:

  PORTA        *porta;
  IOPORT_TRIS  trisa;

  PORTB        *portb;
  IOPORT_TRIS  trisb;

  virtual void create_iopin_map(void){return;};

};


class _28pins  : public _18pins
{
public:

  PORTC        *portc;
  IOPORT_TRIS  trisc;

  virtual void create_iopin_map(void);

};

class _40pins  : public _28pins
{
public:


  PIC_IOPORT   *portd;
  IOPORT_TRIS  trisd;
  PIC_IOPORT   *porte;
  IOPORT_TRIS  trise;


  virtual void create_iopin_map(void){return;};

};

class _14bit_18pins  : public _18pins
{
public:

  virtual void create_iopin_map(void);

};

class _68pins : public Package 
{
public:

  PORTA        *porta;
  IOPORT_TRIS  ddra;

  PORTB        *portb;
  IOPORT_TRIS  ddrb;

  PORTC        *portc;
  IOPORT_TRIS  ddrc;

  PIC_IOPORT   *portd;
  IOPORT_TRIS  ddrd;

  PIC_IOPORT   *porte;
  IOPORT_TRIS  ddre;

  PIC_IOPORT   *portf;
  IOPORT_TRIS  ddrf;

  PIC_IOPORT   *portg;
  IOPORT_TRIS  ddrg;

  virtual void create_iopin_map(void);

};

class _16bit_68pins  : public _68pins
{
public:

  virtual void create_iopin_map(void) {return;};

};

#endif // __PIC_PACKAGES_H__
