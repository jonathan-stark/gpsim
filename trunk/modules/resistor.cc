/*
   Copyright (C) 1998,1999,2000,2001 T. Scott Dattalo

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

  resistor.cc

  This is gpsim's resistor component.

  resistor
  pullup
  pulldown

*/


#include <errno.h>
#include <stdlib.h>
#include <string>


#include "resistor.h"
#include "../src/stimuli.h"
#include "../src/ioports.h"
#include "../src/symbol.h"


//--------------------------------------------------------------
// Resistor_IO
//   This class is a minor extension of a normal IO_input. I may
// remove it later, but for now it does serve a simple purpose.
// Specifically, this derivation will intercept when a stimulus
// is being changed. 

void Resistor_IO::put_node_state( int new_state)
{

  int current_state = state;

  //cout << "void Resistor_IO::put_node_state( int new_state = " << new_state << ")\n";
  IO_input::put_node_state(new_state);

  if(current_state ^ state)
    cout << "Resistor " << name() << " changed to new state: " <<
      state << '\n';

}

int Resistor_IO::get_voltage(guint64 current_time)
{

  return drive;
}

//--------------------------------------------------------------
void Resistor_IOPORT::trace_register_write(void)
{

  trace.module1(value);
}

//--------------------------------------------------------------
Resistor::Resistor(void)
{

  cout << "resistor constructor\n";
  name_str = "Resistor";
}

//--------------------------------------------------------------
// create_iopin_map 
//
//  This is where the information for the Module's package is defined.
// Specifically, the I/O pins of the module are created.

void Resistor::create_iopin_map(void)
{


  // Create an I/O port to which the I/O pins can interface
  //   The module I/O pins are treated in a similar manner to
  //   the pic I/O pins. Each pin has a unique pin number that
  //   describes it's position on the physical package. This
  //   pin can then be logically grouped with other pins to define
  //   an I/O port. 

  port = new Resistor_IOPORT;
  port->new_name("res_port");

  // Define the physical package.
  //   The Package class, which is a parent of all of the modules,
  //   is responsible for allocating memory for the I/O pins.
  //
  //   The Resistor has only two pins.

  create_pkg(2);


  // Define the I/O pins and assign them to the package.
  //   There are two things happening here. First, there is
  //   a new I/O pin that is being created.The second thing is
  //   that the pins are "assigned" to the package. If we
  //   need to reference these newly created I/O pins (like
  //   below) then we can call the member function 'get_pin'.


  assign_pin(1, new Resistor_IO(port, 0));
  assign_pin(2, new Resistor_IO(port, 1));


  // Create an entry in the symbol table for the new I/O pins.
  // This is how the pins are accessed at the higher levels (like
  // in the CLI).

  symbol_table.add_stimulus(Package::get_pin(1));
  symbol_table.add_stimulus(Package::get_pin(2));

}

//--------------------------------------------------------------
// construct

ExternalModule * Resistor::construct(char *new_name = NULL)
{

  cout << " Resistor construct \n";

  Resistor *resP = new Resistor;

  resP->new_name(new_name);
  resP->create_iopin_map();


  return resP;

}

//--------------------------------------------------------------
PUResistor_IO::PUResistor_IO(void)
{

  res = NULL;  // Assigned later.
}

//--------------------------------------------------------------
void PUResistor_IO::put_node_state(int new_state)
{

  //IOPIN::put_node_state(new_state);

  cout << "PUResistor_IO::put_node_state " << name() << '\n';

}

//--------------------------------------------------------------
int PUResistor_IO::get_voltage(guint64 current_time)
{

  if (res) {
    cout << "PUResistor_IO::get_voltage " << res->drive << '\n';
    return res->drive;
  }
  return 0;

}

//--------------------------------------------------------------
// PullupResistor::create_iopin_map 
//

void PullupResistor::create_iopin_map(void)
{

  PUResistor_IO *pur;

  cout << "PullupResistor::create_iopin_map\n";

  //   The PullupResistor has only one pin.

  create_pkg(1);


  // Define the I/O pins and assign them to the package.
  //   There are two things happening here. First, there is
  //   a new I/O pin that is being created.The second thing is
  //   that the pins are "assigned" to the package. If we
  //   need to reference these newly created I/O pins (like
  //   below) then we can call the member function 'get_pin'.

  pur = new PUResistor_IO();
  pur->res = res;
  pur->put_name(name_str);
  assign_pin(1, pur);


  // Create an entry in the symbol table for the new I/O pins.
  // This is how the pins are accessed at the higher levels (like
  // in the CLI).
  IOPIN *iop = Package::get_pin(1);
  if(iop) {
    cout << "pullup resistor pin name: "<<iop->name() << '\n';
    cout << "voltage " << iop->get_voltage(0) << '\n';
  }
  symbol_table.add_stimulus(Package::get_pin(1));

}



//--------------------------------------------------------------

ExternalModule * PullupResistor::pu_construct(char *new_name = NULL)
{

  cout << "Pullup Resistor construct \n";

  PullupResistor *pur = new PullupResistor;

  if(new_name) {
    pur->new_name(new_name);
    pur->res->put_name(new_name);
  }

  pur->create_iopin_map();

  cout << "Resistance " << pur->res->drive << '\n';

  return pur;

}

//--------------------------------------------------------------
ExternalModule * PullupResistor::pd_construct(char *new_name = NULL)
{

  cout << "Pulldown Resistor construct \n";

  PullupResistor *pur = new PullupResistor;

  if(new_name) {
    pur->new_name(new_name);
    pur->res->put_name(new_name);
  }
  pur->create_iopin_map();

  pur->res->drive = -pur->res->drive;

  cout << "Resistance " << pur->res->drive << '\n';

  return pur;

}

//--------------------------------------------------------------
PullupResistor::PullupResistor(void)
{

  cout << "Pull up resistor constructor\n";

  // Create the resistor:

  res = new resistor;
  res->drive = MAX_DRIVE/2;

  name_str = "pur";
}
