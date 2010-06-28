/*
   Copyright (C) 1998,1999 T. Scott Dattalo

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


/*
  pic-packages.h

Here's where all of the pic packages are defined

 */

#ifndef __PACKAGES_H__
#define __PACKAGES_H__
#include <string>
using namespace std;

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
// The PinGeometry holds information the gui can query to build 
// a graphical representation of the pin.
struct PinGeometry
{
  PinGeometry() : pin_position(0.0), bNew(false), m_bShowPinname(false) {}
  void convertToNew(); // transition from old to new style

  float pin_position;

  // Newer style
  bool  bNew;
  float m_x;
  float m_y;
  int   m_orientation;
  bool  m_bShowPinname;
};


class Package
{
public:

  unsigned int number_of_pins;


  Package(void);
  Package(unsigned int number_of_pins);
  virtual ~Package();

  void assign_pin(unsigned int pin_number, IOPIN *pin, bool warn=true);
  void destroy_pin(unsigned int pin_number, IOPIN *pin=0);
  void create_pkg(unsigned int _number_of_pins);

  unsigned int isa(void){return _NO_PACKAGE_;};
  virtual void create_iopin_map(void);

  virtual int get_pin_count(void) {return number_of_pins;};
  virtual string &get_pin_name(unsigned int pin_number);
  virtual int get_pin_state(unsigned int pin_number);
  virtual float get_pin_position(unsigned int pin_number);
  virtual void set_pin_position(unsigned int pin_number, float position);
  int pin_existance(unsigned int pin_number);
  IOPIN *get_pin(unsigned int pin_number);

  void setPinGeometry(unsigned int pin_number, float x, float y, int orientation, bool bShowName);
  PinGeometry *getPinGeometry(unsigned int pin_number);

  // Debug
  void showPins();

protected:
  inline bool bIsValidPinNumber(unsigned int pin_number)
  {
    return (pin_number > 0) && (pin_number<=number_of_pins);
  }

  IOPIN **pins;  /* An array containing all of the package's pins. The index
		  * into the array is the package's pin #. If pins[i] is NULL
		  * then there's gpsim does not provide any resources for 
		  * simulating the pin.
		  */

  // pin_position is used by the breadboard to position the pin
  // Its value can be in the range from 0.0000 to 3.9999.
  // 0.0 is upmost left position. 0.9999 is lowest left.
  // 1.0 is leftmost bottom position. 1.99 is rightmost bottom.. A.S.O
  // float *pin_position;
  PinGeometry *m_pinGeometry;
};

#endif // __PACKAGES_H__
