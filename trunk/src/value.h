/*
   Copyright (C) 1998-2004 Scott Dattalo

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

#ifndef __VALUE_H__
#define __VALUE_H__

#include "gpsim_object.h"

class Processor;
#include "xref.h"


//------------------------------------------------------------------------
//
// Values
//
// Everything that can hold a value is derived from the Value class.
// The primary purpose of this is to provide external objects (like
// the gui) an abstract way of getting the value of diverse things
// like registers, program counters, cycle counters, etc.
//
//

class gpsimValue : public gpsimObject {
 public:

  gpsimValue(void);
  gpsimValue(Processor *);
  virtual ~gpsimValue();

  // Access functions
  virtual unsigned int get(void)
    {
      return get_value();
    }

  virtual void put(unsigned int new_value)
    {
      put_value(new_value);
    }

  
  // put_value is the same as put(), but some extra stuff like
  // interfacing to the gui is done. (It's more efficient than
  // burdening the run time performance with (unnecessary) gui
  // calls.)
  //

  virtual void put_value(unsigned int new_value) = 0;

  // A variation of get(). (for register values, get_value returns
  // just the value whereas get() returns the value and does other
  // register specific things like tracing.

  virtual unsigned int get_value(void) = 0;

  virtual void set_cpu(Processor *new_cpu)
    {
      cpu = new_cpu;
    }

  Processor *get_cpu(void)
    {
      return cpu;
    }

  // When the value changes, then update() is called 
  // to update all things that are watching this value.
  virtual void update(void);
  virtual void add_xref(void *xref);
  virtual void remove_xref(void *xref);

  XrefObject xref(void) { return _xref; }

 protected:
  // If we are linking with a gui, then here are a
  // few declarations that are used to send data to it.
  // This is essentially a singly-linked list of pointers
  // to structures. The structures provide information
  // such as where the data is located, the type of window
  // it's in, and also the way in which the data is presented
  
  XrefObject _xref;

  // A pointer to the processor that owns this value.
  // FIXME - should this be a Module pointer instead?
  Processor *cpu;

};

#endif // __VALUE_H__
