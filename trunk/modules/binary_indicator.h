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


#ifndef __BINARY_INDICATOR_H__
#define __BINARY_INDICATOR_H__

#include <glib.h>
#include "../src/modules.h"
#include "../src/packages.h"
#include "../src/stimuli.h"


// Create a class derived from the IO_input class that
// will allow us to intercept when the I/O input is being
// driven. (This isn't done for PIC I/O pins because the
// logic for handling I/O pin changes resides in the IOPORT
// class.)

class Binary_Input : public IO_input
{
public:

  virtual void put_node_state( int new_state);
  Binary_Input (IOPORT *i, unsigned int b) : IO_input(i,b) { };
};

class Binary_Indicator : public ExternalModule //public Module, public Package
{
public:

  IOPORT  *port;

  Binary_Indicator(void);

  // Inheritances from the Package class
  virtual void create_iopin_map(void);

  static ExternalModule *construct(char *new_name=NULL);
  void test(void) ;
};

#endif //  __BINARY_INDICATOR_H__
