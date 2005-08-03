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


#include <stdio.h>

#include "../config.h"
#include "pic-processor.h"
#include "stimuli.h"
#include "packages.h"

Package::Package(void)
{

  pins = 0;
  number_of_pins = 0;
  pin_position = 0;

}

Package::Package(unsigned int _number_of_pins)
{
  number_of_pins = 0;
  create_pkg(_number_of_pins);
}

void Package::create_pkg(unsigned int _number_of_pins)
{
  if(number_of_pins)
    {
      cout << "error: Package::create_pkg. Package appears to already exist.\n";
      return;
    }


  number_of_pins = _number_of_pins;

  pins = new IOPIN *[number_of_pins];

  pin_position =  new float[number_of_pins];

  for(unsigned int i=0; i<number_of_pins; i++)
  {
    unsigned int pins_per_side;
    pins[i] = 0;

    pins_per_side = number_of_pins/2;
    if(number_of_pins&1) // If odd number of pins
        pins_per_side++;

    // Positions for DIL package
    if(i<pins_per_side)
      pin_position[i]=(i)/((float)(pins_per_side-0.9999));
    else
      pin_position[i]=(i-pins_per_side)/((float)(pins_per_side-0.9999))+(float)2.0;
  }


}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
int Package::pin_existance(unsigned int pin_number)
{

  if(!number_of_pins)
    {
      cout << "error: Package::assign_pin. No package.\n";
      return E_NO_PACKAGE;
    }


  if((pin_number > number_of_pins) || (pin_number == 0))
    {
      cout << "error: Package::assign_pin. Pin number is out of range.\n";
      cout << "Max pins " << number_of_pins << ". Trying to add " << pin_number <<".\n";
      return E_PIN_OUT_OF_RANGE;
    }

  if(pins[pin_number-1])
    return E_PIN_EXISTS;
  
  return E_NO_PIN;

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
IOPIN *Package::get_pin(unsigned int pin_number)
{
  if(E_PIN_EXISTS == pin_existance(pin_number))
    return pins[pin_number-1];
  else
    return 0;

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
float Package::get_pin_position(unsigned int pin_number)
{
    if(number_of_pins==0 ||
       pin_number<=0 ||
       pin_number>number_of_pins)
	return 0.0;

    return pin_position[pin_number-1];
}

void Package::set_pin_position(unsigned int pin_number, float position)
{
    if(number_of_pins==0 ||
       pin_number<=0 ||
       pin_number>number_of_pins)
	return;

    pin_position[pin_number-1]=position;
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void Package::assign_pin(unsigned int pin_number, IOPIN *pin)
{

  switch(pin_existance(pin_number)) {

  case E_PIN_EXISTS:
    if(pins[pin_number-1])
      cout << "warning: Package::assign_pin. Pin number " << pin_number << " already exists.\n";

  case E_NO_PIN:
    pins[pin_number-1] = pin;

    // Tell the I/O pin its number
    //pin->new_pin_number(pin_number);
    break;

  }


}
void Package::create_iopin_map(void)
{

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------

string &Package::get_pin_name(unsigned int pin_number)
{
  static string invalid("");
  if(pin_existance(pin_number) == E_PIN_EXISTS)
    return pins[pin_number-1]->name();
  else
    return invalid;  //FIXME

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
int Package::get_pin_state(unsigned int pin_number)
{

  if(pin_existance(pin_number) == E_PIN_EXISTS)
    return pins[pin_number-1]->getDrivingState();
  else
    return 0;

}


