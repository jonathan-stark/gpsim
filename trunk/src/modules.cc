/*
   Copyright (C) 1998,1999,2000 T. Scott Dattalo

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
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string>
#include <list>
#include <vector>

#include "pic-processor.h"
#include "stimuli.h"
#include "stimulus_orb.h"
#include "symbol.h"
#include "xref.h"

#include "modules.h"



class Module
{
public:

  int number_of_pins;


  IOPIN **pins;  /* An array containing all of the module's pins. The index
		  * into the array is the module's pin #. If pins[i] is NULL
		  * then gpsim does not provide any resources for 
		  * simulating the pin.
		  */


  Module(void);
  Module(unsigned int number_of_pins);

  void assign_pin(unsigned int pin_number, IOPIN *pin);
  void create_pkg(unsigned int _number_of_pins);

  virtual void create_iopin_map(void);

  virtual int get_pin_count(void) {return number_of_pins;};
  virtual char *get_pin_name(unsigned int pin_number);
  virtual int get_pin_state(unsigned int pin_number);
  int pin_existance(unsigned int pin_number);
  IOPIN *get_pin(unsigned int pin_number);
};


Module::Module(void)
{

  pins = NULL;
  number_of_pins = 0;

}

Module::Module(unsigned int _number_of_pins)
{
  number_of_pins = 0;
  create_pkg(_number_of_pins);
}

void Module::create_pkg(unsigned int _number_of_pins)
{
  if(number_of_pins)
    {
      cout << "error: Module::create_pkg. Module appears to already exist.\n";
      return;
    }


  number_of_pins = _number_of_pins;

  pins = (IOPIN **) new char[sizeof( IOPIN *) * number_of_pins];

  for(int i=0; i<number_of_pins; i++)
    pins[i] = NULL;

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
int Module::pin_existance(unsigned int pin_number)
{

  if(!number_of_pins)
    {
      cout << "error: Module::assign_pin. No module.\n";
      return E_NO_PACKAGE;
    }


  if((pin_number > number_of_pins) || (pin_number == 0))
    {
      cout << "error: Module::assign_pin. Pin number is out of range.\n";
      cout << "Max pins " << number_of_pins << ". Trying to add " << pin_number <<".\n";
      return E_PIN_OUT_OF_RANGE;
    }

  if(pins[pin_number-1])
    return E_PIN_EXISTS;
  
  return E_NO_PIN;

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
IOPIN *Module::get_pin(unsigned int pin_number)
{

  if(E_PIN_EXISTS == pin_existance(pin_number))
    return pins[pin_number-1];
  else
    return NULL;

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void Module::assign_pin(unsigned int pin_number, IOPIN *pin)
{

  switch(pin_existance(pin_number)) {

  case E_PIN_EXISTS:
    if(pins[pin_number-1])
      cout << "warning: Module::assign_pin. Pin number " << pin_number << " already exists.\n";

  case E_NO_PIN:
    pins[pin_number-1] = pin;
    // Tell the I/O pin its number
    //pin->new_pin_number(pin_number);
    break;

  }


}
void Module::create_iopin_map(void)
{

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------

char *Module::get_pin_name(unsigned int pin_number)
{

  if(pin_existance(pin_number) == E_PIN_EXISTS)
    return pins[pin_number-1]->name();
  else
    return NULL;

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
int Module::get_pin_state(unsigned int pin_number)
{

  if(pin_existance(pin_number) == E_PIN_EXISTS)
    return pins[pin_number-1]->get_voltage(0);
  else
    return 0;

}


