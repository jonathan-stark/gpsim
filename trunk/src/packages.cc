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
#include "pic-packages.h"


Package::Package(void)
{

  pins = NULL;
  number_of_pins = 0;
  pin_position = NULL;

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

  pins = (IOPIN **) new char[sizeof( IOPIN *) * number_of_pins];

  pin_position = (float *) new float[number_of_pins];

  for(int i=0; i<number_of_pins; i++)
  {
    int pins_per_side;
    pins[i] = NULL;

    pins_per_side = number_of_pins/2;
    if(number_of_pins&1) // If odd number of pins
        pins_per_side++;

    // Positions for DIL package
    if(i<pins_per_side)
      pin_position[i]=(i)/((float)(pins_per_side-0.9999));
    else
      pin_position[i]=(i-pins_per_side)/((float)(pins_per_side-0.9999))+2.0;
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
    return NULL;

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

char *Package::get_pin_name(unsigned int pin_number)
{

  if(pin_existance(pin_number) == E_PIN_EXISTS)
    return pins[pin_number-1]->name();
  else
    return NULL;

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
int Package::get_pin_state(unsigned int pin_number)
{

  if(pin_existance(pin_number) == E_PIN_EXISTS)
    return pins[pin_number-1]->get_voltage(0);
  else
    return 0;

}


//-------------------------------------------------------------------
//-------------------------------------------------------------------
void _28pins::create_iopin_map(void)
{
  // ---- This is probably going to be moved:
  porta = new PORTA;
  portb = new PORTB;
  portc = new PORTC;


  if(verbose)
    cout << "Create i/o pin map\n";
  // Build the links between the I/O Ports and their tris registers.
  porta->tris = &trisa;
  trisa.port = porta;

  portb->tris = &trisb;
  trisb.port = portb;

  portc->tris = &trisc;
  trisc.port = portc;

  // And give them a more meaningful name.
  trisa.new_name("trisa");
  trisb.new_name("trisb");
  trisc.new_name("trisc");

  // Define the valid I/O pins.
  porta->valid_iopins = 0x3f;
  portb->valid_iopins = 0xff;
  portc->valid_iopins = 0xff;


  // Now Create the package and place the I/O pins

  create_pkg(28);

  assign_pin(1, NULL);

  assign_pin(2, new IO_bi_directional(porta, 0));
  assign_pin(3, new IO_bi_directional(porta, 1));
  assign_pin(4, new IO_bi_directional(porta, 2));
  assign_pin(5, new IO_bi_directional(porta, 3));
  assign_pin(6, new IO_open_collector(porta, 4));
  assign_pin(7, new IO_open_collector(porta, 5));

  assign_pin(8, NULL);
  assign_pin(9, NULL);
  assign_pin(10, NULL);

  assign_pin(11, new IO_bi_directional(portc, 0));
  assign_pin(12, new IO_bi_directional(portc, 1));
  assign_pin(13, new IO_bi_directional(portc, 2));
  assign_pin(14, new IO_bi_directional(portc, 3));
  assign_pin(15, new IO_bi_directional(portc, 4));
  assign_pin(16, new IO_bi_directional(portc, 5));
  assign_pin(17, new IO_bi_directional(portc, 6));
  assign_pin(18, new IO_bi_directional(portc, 7));

  assign_pin(19, NULL);
  assign_pin(20, NULL);

  assign_pin(21, new IO_bi_directional_pu(portb, 0));
  assign_pin(22, new IO_bi_directional_pu(portb, 1));
  assign_pin(23, new IO_bi_directional_pu(portb, 2));
  assign_pin(24, new IO_bi_directional_pu(portb, 3));
  assign_pin(25, new IO_bi_directional_pu(portb, 4));
  assign_pin(26, new IO_bi_directional_pu(portb, 5));
  assign_pin(27, new IO_bi_directional_pu(portb, 6));
  assign_pin(28, new IO_bi_directional_pu(portb, 7));

}
