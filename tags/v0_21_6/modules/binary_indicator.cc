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

/*

  binary_indicator.c

  This is an example module illustrating how modules may be created 

*/

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include <errno.h>
#include <stdlib.h>
#include <string>


#include "binary_indicator.h"
/*
#include <gpsim/stimuli.h>
#include <gpsim/ioports.h>
#include <gpsim/symbol.h>
*/
#include "../src/stimuli.h"
#include "../src/ioports.h"
#include "../src/symbol.h"

//--------------------------------------------------------------
// Binary_Input
//   This class is a minor extension of a normal IO_input. I may
// remove it later, but for now it does serve a simple purpose.
// Specifically, this derivation will intercept when a stimulus
// is being changed. 

#if 0
void Binary_Input::put_node_state( int new_state)
{

  int current_state = state;

  //cout << "void Binary_Input::put_node_state( int new_state = " << new_state << ")\n";
  IO_input::put_node_state(new_state);

  if(current_state ^ state)
    cout << "Binary Input " << name() << " changed to new state: " <<
      state << '\n';

}
#endif

Binary_Indicator::Binary_Indicator(void)
{

  cout << "binary indicator constructor\n";
  name_str = strdup("Binary Indicator");
}

Binary_Indicator::~Binary_Indicator(void)
{

    cout << "binary indicator destructor\n";

    delete port;
}

void Binary_Indicator::test(void)
{

  cout << "test";

}


//--------------------------------------------------------------
// create_iopin_map 
//
//  This is where the information for the Module's package is defined.
// Specifically, the I/O pins of the module are created.

void Binary_Indicator::create_iopin_map(void)
{


  // Create an I/O port to which the I/O pins can interface
  //   The module I/O pins are treated in a similar manner to
  //   the pic I/O pins. Each pin has a unique pin number that
  //   describes it's position on the physical package. This
  //   pin can then be logically grouped with other pins to define
  //   an I/O port. 

  port = new IOPORT;
  port->new_name("bin_port");

  // Define the physical package.
  //   The Package class, which is a parent of all of the modules,
  //   is responsible for allocating memory for the I/O pins.
  //
  //   The Binary_Indicator has only two pins.

  create_pkg(2);


  // Define the I/O pins and assign them to the package.
  //   There are two things happening here. First, there is
  //   a new I/O pin that is being created. For the binary 
  //   indicator, both pins are inputs. The second thing is
  //   that the pins are "assigned" to the package. If we
  //   need to reference these newly created I/O pins (like
  //   below) then we can call the member function 'get_pin'.


  assign_pin(1, new Binary_Input(port, 0));
  assign_pin(2, new Binary_Input(port, 1));


  // Create an entry in the symbol table for the new I/O pins.
  // This is how the pins are accessed at the higher levels (like
  // in the CLI).

  get_symbol_table().add_stimulus(get_pin(1));
  get_symbol_table().add_stimulus(get_pin(2));

}

//--------------------------------------------------------------
// construct

Module * Binary_Indicator::construct(const char *_new_name)
{

  cout << " Binary Indicator \n";

  Binary_Indicator *biP = new Binary_Indicator;

  biP->new_name(_new_name);
  biP->create_iopin_map();


  return biP;

}
