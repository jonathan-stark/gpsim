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


#ifndef __PACKAGES_H__
#define __PACKAGES_H__

class IOPIN;  // in stimuli.h
#include "pic-processor.h"
#include "stimuli.h"

enum PACKAGE_TYPE
{
  _NO_PACKAGE_,
  DIP_8PIN,
  DIP_14PIN,
  DIP_18PIN,
  DIP_28PIN,
  DIP_40PIN

};

enum PACKAGE_PIN_ERRORS
{
  E_NO_PIN,
  E_NO_PACKAGE,
  E_PIN_OUT_OF_RANGE,
  E_PIN_EXISTS
};
  
class Package
{
public:

  int number_of_pins;


  IOPIN **pins;  /* An array containing all of the package's pins. The index
		  * into the array is the package's pin #. If pins[i] is NULL
		  * then there's gpsim does not provide any resources for 
		  * simulating the pin.
		  */


  Package(void);
  Package(unsigned int number_of_pins);

  void assign_pin(unsigned int pin_number, IOPIN *pin);
  void create_pkg(unsigned int _number_of_pins);

  // void map_pins(IOPIN_map *iopin_map_ptr, int num_of_iopins)
  unsigned int isa(void){return _NO_PACKAGE_;};
  virtual void create_iopin_map(void);

  virtual int get_pin_count(void) {return number_of_pins;};
  virtual char *get_pin_name(unsigned int pin_number);
  virtual int get_pin_state(unsigned int pin_number);
  int pin_existance(unsigned int pin_number);

};


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

  /*
  IOPORT       portd;
  IOPORT_TRIS  trisd;
  */

  virtual void create_iopin_map(void); //{return;};

};

class _40pins  : public _28pins
{
public:

  /*
  IOPORT       portd;
  IOPORT_TRIS  trisd;
  */

  virtual void create_iopin_map(void){return;};

};

class _14bit_18pins  : public _18pins
{
public:

  INTCON       intcon_reg;

  virtual void create_iopin_map(void);

};

#endif // __PACKAGES_H__
