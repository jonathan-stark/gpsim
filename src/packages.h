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
  pic-packages.h

Here's where all of the pic packages are defined

 */

#ifndef __PACKAGES_H__
#define __PACKAGES_H__

#include "gpsim_classes.h"

class IOPIN; // forward reference

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

  // pin_position is used by the breadboard to position the pin
  // Its value can be in the range from 0.0000 to 3.9999.
  // 0.0 is upmost left position. 0.9999 is lowest left.
  // 1.0 is leftmost bottom position. 1.99 is rightmost bottom.. A.S.O
  float *pin_position;

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
  virtual float get_pin_position(unsigned int pin_number);
  virtual void set_pin_position(unsigned int pin_number, float position);
  int pin_existance(unsigned int pin_number);
  IOPIN *get_pin(unsigned int pin_number);
};

#endif // __PACKAGES_H__
