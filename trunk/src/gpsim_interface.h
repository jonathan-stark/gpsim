/*
   Copyright (C) 1998 T. Scott Dattalo

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

#ifndef __GPSIM_INTERFACE_H__
#define __GPSIM_INTERFACE_H__

#include "interface.h"

//---------------------------------------------------------------------------
//
//  struct Interface
//
//  Here's a structure containing all of the information for gpsim to
// interface to the gui and to dynamically loaded modules. Primarily,
// there are pointers to functions that gpsim will invoke upon certain
// conditions. 

class Interface {
 public:

  unsigned int interface_id;
  gpointer objectPTR;

  /*
   * update_object - pointer to the function that is invoked when an object changes
   *
   * If an object, like the contents of a register, changes then this function
   * will be called (if it's non-null). There are two parameters:
   *  xref - this is a pointer to some structure in the client's data space.
   *  new_value - this is the new value to which the object has changed.
   *
   *
   */

  void (*update_object) (gpointer xref,int new_value);

  /*
   * remove_object - pointer to the function that is invoked when an object is 
   *                 removed. Its purpose is to notify the client when gpsim 
   *                 has removed something.
   *
   * If an object, like a register, is deleted then this function 
   * will be called (if it's non-null). There is one parameter:
   *  xref - this is a pointer to some structure in the client's data space.
   *
   *
   */

  void (*remove_object) (gpointer xref);


  /*
   * simulation_has_stopped - pointer to the function that is invoked when gpsim has
   *                          stopped simulating. Some interfaces have more than one
   *                          instance, so the 'object' parameter allows the interface
   *                          to uniquely identify the particular one.
   */

  void (*simulation_has_stopped) (gpointer object);


  /*
   * new_processor - pointer to the function that is invoked when a new processor is
   *                 added to gpsim
   */

  void (*new_processor) (unsigned int processor_id);

  /*
   * new_program - pointer to the function that is invoked when a new program is
   *               loaded into gpsim
   */

  /* extern void (*new_program)  (pic_processor *p);*/

  void (*new_program)  (unsigned int processor_id);

  unsigned int get_id(void) { return interface_id;};
  unsigned int set_id(unsigned int new_id) { interface_id = new_id;};
  Interface(gpointer new_object=NULL);
  Interface(void) { Interface(NULL); };
};


class gpsimInterface {
 public:

  GSList *interfaces;
  unsigned int interface_seq_number;

  gpsimInterface(void);

  // gpsim will call these functions to notify gui and/or modules
  // that something has changed. 

  void update_object (gpointer xref,int new_value);
  void remove_object (gpointer xref);
  void simulation_has_stopped (void);
  void new_processor (unsigned int processor_id);
  void new_program  (unsigned int processor_id);

  unsigned int add_interface(Interface *new_interface);

};

extern gpsimInterface gi;

#endif // __GPSIM_INTERFACE_H__
