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

  usart.cc

  This is gpsim's universal synchronous/asynchronous receiver/transceiver.

  Features:

    8 or 9 bit receiver and transmitter
    0 or 1 start bits
    0 or 1 stop bits
    0 or 1 parity bits and even/odd selectable
    variable sized transmit and receive buffers

*/


#include <errno.h>
#include <stdlib.h>
#include <string>
#include "../config.h"

#ifdef HAVE_GUI
#include <gtk/gtk.h>
#endif



#include "../src/modules.h"
#include "../src/packages.h"
#include "../src/stimuli.h"
#include "../src/ioports.h"
#include "../src/symbol.h"
#include "../src/attribute.h"

#include "usart.h"


class USART_IO : public IOPIN
{
public:


  USART_IO(void) {
    cout << "USART_IO constructor - do nothing\n";
  }
  USART_IO (IOPORT *i, unsigned int b) : IOPIN(i,b) { };

  virtual void put_node_state(int new_state); // From attached node


  //virtual void put_state( int new_state);
  virtual int get_voltage(guint64 current_time);
};



class USART_IOPORT : public IOPORT
{
public:

  virtual void trace_register_write(void);

  USART_IOPORT (unsigned int _num_iopins=2);

};


//--------------------------------------------------------------
// create_iopin_map 
//
//  This is where the information for the Module's package is defined.
// Specifically, the I/O pins of the module are created.

void USARTModule::create_iopin_map(void)
{


  // Create an I/O port to which the I/O pins can interface
  //   The module I/O pins are treated in a similar manner to
  //   the pic I/O pins. Each pin has a unique pin number that
  //   describes it's position on the physical package. This
  //   pin can then be logically grouped with other pins to define
  //   an I/O port. 

  port = new USART_IOPORT;
  port->new_name("usart_port");

  // Define the physical package.
  //   The Package class, which is a parent of all of the modules,
  //   is responsible for allocating memory for the I/O pins.
  //
  //   

  create_pkg(2);


  // Define the I/O pins and assign them to the package.
  //   There are two things happening here. First, there is
  //   a new I/O pin that is being created.The second thing is
  //   that the pins are "assigned" to the package. If we
  //   need to reference these newly created I/O pins (like
  //   below) then we can call the member function 'get_pin'.


  assign_pin(1, new USART_IO(port, 0));
  assign_pin(2, new USART_IO(port, 1));


  // Create an entry in the symbol table for the new I/O pins.
  // This is how the pins are accessed at the higher levels (like
  // in the CLI).

  symbol_table.add_stimulus(Package::get_pin(1));
  symbol_table.add_stimulus(Package::get_pin(2));

}


//--------------------------------------------------------------

ExternalModule * USARTModule::USART_construct(char *new_name = NULL)
{

  cout << "USART construct \n";

  USARTModule *um = new USARTModule(new_name);

  if(new_name) {
    um->new_name(new_name);
    //um->res->put_name(new_name);
  }

  um->create_iopin_map();


  return um;

}
