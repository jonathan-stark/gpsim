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
#include "breakpoints.h"

class Stimulus;
class Stimulus_Node;

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
   */

  virtual void UpdateObject (gpointer xref,int new_value){};

  /*
   * remove_object - Invoked when gpsim has removed something.
   *
   * If an object, like a register, is deleted then this function 
   * will be called. There is one parameter:
   *  xref - this is a pointer to some structure in the client's data space.
   *
   */

  virtual void RemoveObject (gpointer xref) {};


  /*
   * simulation_has_stopped - invoked when gpsim has stopped simulating (e.g.
   *                          when a breakpoint is encountered).
   *
   * Some interfaces have more than one instance, so the 'object' parameter
   * allows the interface to uniquely identify the particular one.
   */

  virtual void SimulationHasStopped (gpointer object) {};


  /*
   * new_processor - Invoked when a new processor is added to gpsim
   */

  virtual void NewProcessor (Processor *new_cpu) {};


  /*
   * new_module - Invoked when a new module is instantiated.
   *
   */

  virtual void NewModule (Module *module) {};

  /*
   * node_configuration_changed - invoked when stimulus configuration has changed
   */

  virtual void NodeConfigurationChanged (Stimulus_Node *node) {};


  /*
   * new_program - Invoked when a new program is loaded into gpsim
   *
   */

  virtual void NewProgram  (Processor *new_cpu) { };

  /*
   * gui_update - Invoked when the gui should update
   */

  virtual void GuiUpdate  (gpointer object) {};

  /*
   * destructor - called when the interface is destroyed - this gives
   *  the interface object a chance to save state information.
   */
  virtual ~Interface() 
  {
  }

  unsigned int get_id(void) { return interface_id;};
  void set_id(unsigned int new_id) { interface_id = new_id;};
  Interface(gpointer new_object=NULL);
};


class gpsimInterface : public TriggerObject {
 public:

  GSList *interfaces;
  unsigned int interface_seq_number;

  guint64 gui_update_rate;
  guint64 future_cycle;


  gpsimInterface(void);

  // gpsim will call these functions to notify gui and/or modules
  // that something has changed. 

  void update_object (gpointer xref,int new_value);
  void remove_object (gpointer xref);
  void simulation_has_stopped (void);
  void update (void);
  void new_processor (Processor *);
  void new_module  (Module *module);
  void node_configuration_changed  (Stimulus_Node *node);
  void new_program  (Processor *);
  void set_update_rate(guint64 rate);

  unsigned int add_interface(Interface *new_interface);
  void remove_interface(unsigned int interface_id);


  virtual bool set_break(void) {return false;}
  virtual void callback(void);
  virtual void callback_print(void);
  virtual void print(void);
  virtual void clear(void);
  virtual char const * bpName() { return "gpsim interface"; }

};

#ifdef IN_MODULE
// we are in a module: don't access gi object directly!
gpsimInterface &get_interface(void);
#else
// we are in gpsim: use of get_interface() is recommended,
// even if gi object can be accessed directly.
extern gpsimInterface gi;

inline gpsimInterface &get_interface(void)
{
  return gi;
}
#endif

//------------------------------------------------------------------------

class Module;

class ModuleInterface
{

 public:
  Module *module;  // The module we're interfacing with.


  ModuleInterface(Module *new_module);


};

//------------------------------------------------------------------------

class Processor;

class ProcessorInterface : public ModuleInterface
{
 public:

  ProcessorInterface(Processor *cpu);

};

#endif // __GPSIM_INTERFACE_H__
