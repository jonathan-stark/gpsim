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
//  Here's a structure containing all of the information for gpsim
// and an external interface like the GUI to communicate. There are
// two major functions. First, a GUI can provide pointers to callback
// functions that the simulator can invoke upon certain conditions.
// For example, if the contents of a register change, the simulator
// can notify the GUI and thus save the gui having to continuously
// poll. (This may occur when the command line interface changes
// something; such changes need to be propogated up to the gui.) 


class Interface {
 public:

  unsigned int interface_id;
  gpointer objectPTR;

  /*
   * UpdateObject - Invoked when an object changes
   *
   * If an object, like the contents of a register, changes then this function
   * will be called. There are two parameters:
   *  xref - this is a pointer to some structure in the client's data space.
   *  new_value - this is the new value to which the object has changed.
   *
   *
   */

  //void (*update_object) (gpointer xref,int new_value);
  virtual void UpdateObject (gpointer xref,int new_value){};

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

  //void (*remove_object) (gpointer xref);
  virtual void RemoveObject (gpointer xref) {};


  /*
   * simulation_has_stopped - pointer to the function that is invoked when gpsim has
   *                          stopped simulating. Some interfaces have more than one
   *                          instance, so the 'object' parameter allows the interface
   *                          to uniquely identify the particular one.
   */

  //void (*simulation_has_stopped) (gpointer object);
  virtual void SimulationHasStopped (gpointer object) {};


  /*
   * new_processor - pointer to the function that is invoked when a new processor is
   *                 added to gpsim
   */

  //void (*new_processor) (unsigned int processor_id);
  virtual void NewProcessor (unsigned int processor_id) {};


  /*
   * new_module - pointer to the function that is invoked when a new module is
   *              added into gpsim
   */

  //void (*new_module) (Module *module);
  virtual void NewModule (Module *module) {};

  /*
   * node_configuration_changed - pointer to the function that is invoked when stimulus
   * configuration are changed
   */

  //void (*node_configuration_changed) (Stimulus_Node *node);
  virtual void NodeConfigurationChanged (Stimulus_Node *node) {};

  /*
   * new_program - pointer to the function that is invoked when a new program is
   *               loaded into gpsim
   */

  //void (*new_program)  (unsigned int processor_id);
  virtual void NewProgram  (unsigned int processor_id) { };

  /*
   * gui_update - pointer to the function that is invoked when the gui should update
   */

  //void (*gui_update)  (gpointer object);
  virtual void GuiUpdate  (gpointer object) {};

  unsigned int get_id(void) { return interface_id;};
  void set_id(unsigned int new_id) { interface_id = new_id;};
  Interface(gpointer new_object=NULL);
};


class gpsimInterface {
 public:

  GSList *interfaces;
  unsigned int interface_seq_number;

  void *gui_update_cbp;
  guint64 gui_update_rate;


  gpsimInterface(void);

  // gpsim will call these functions to notify gui and/or modules
  // that something has changed. 

  void update_object (gpointer xref,int new_value);
  void remove_object (gpointer xref);
  void simulation_has_stopped (void);
  void new_processor (unsigned int processor_id);
  void new_module  (Module *module);
  void node_configuration_changed  (Stimulus_Node *node);
  void new_program  (unsigned int processor_id);
  void set_update_rate(guint64 rate);

  unsigned int add_interface(Interface *new_interface);
  void remove_interface(unsigned int interface_id);

};

extern gpsimInterface gi;

#endif // __GPSIM_INTERFACE_H__
