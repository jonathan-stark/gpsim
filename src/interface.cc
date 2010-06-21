/*
   Copyright (C) 1998 T. Scott Dattalo

This file is part of the libgpsim library of gpsim

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see 
<http://www.gnu.org/licenses/lgpl-2.1.html>.
*/

//-------------------------------------------------------------------
//                     interface.cc
//
// interface.cc provides a layer of code on top of the simulator 
// portion of gpsim. It's purpose is to provide an abstract interface
// that hides the details of the simulator. Currently only the gui
// interfaces to gpsim through this layer. However, since the simulator
// 'engine' is built as a library, it's possible for other code to
// interface through here as well.
//
//-------------------------------------------------------------------

#include <stdio.h>

#include "../config.h"
#define GPSIM_VERSION VERSION 
#include "gpsim_def.h"

#include "sim_context.h"
#include "processor.h"
#include "xref.h"
#include "interface.h"
#include "trace.h"
#include "eeprom.h"
#include "icd.h"

#include "cmd_manager.h"
#include "clock_phase.h"

extern Integer *verbosity;  // in ../src/init.cc

// Flag to tell us when all of the init stuff is done.
unsigned int gpsim_is_initialized = 0;

/**************************************************************************
 *
 *  Here's the gpsim interface class instantiation. It's through this class
 * that gpsim will notify the gui and/or modules of internal gpsim changes.
 *
 **************************************************************************/

gpsimInterface gi;

// create an instance of inline get_interface() method by taking its address
gpsimInterface &(*dummy_gi)(void) = get_interface;

//------------------------------------------------------------------------
// Temporary -- provide a flag to inihibit multithreaded support.
bool gUsingThreads()
{
  return false;
}
//---------------------------------------------------------------------------
//   void gpsim_set_bulk_mode(int flag)
//---------------------------------------------------------------------------
void gpsim_set_bulk_mode(int flag)
{
  if(get_use_icd())
  {
    icd_set_bulk(flag);
  }
}


void  initialization_is_complete(void)
{
  gpsim_is_initialized = 1;
}


//========================================================================
//========================================================================





//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/*
  *            Module Interface
  *
  *
*/
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

ModuleInterface::ModuleInterface(Module *new_module)
{
  module = new_module;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/*
  *            Processor Interface
  *
  *
*/
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

ProcessorInterface::ProcessorInterface(Processor *new_cpu) : ModuleInterface(new_cpu)
{


}



//--------------------------------------------------------------------------
//
// callback registration functions
//
//
//--------------------------------------------------------------------------
Interface::Interface(gpointer new_object)
{

  objectPTR = new_object;

}


//--------------------------------------------------------------------------
//
// gpsimInterface
//
// Here are where the member functions for the gpsimInterface class are
// defined.
//
// The gpsimInterface class contains a singly-linked-list of Interface objects.
// Interface objects are structures that primarily contain pointers to a whole
// bunch of functions. The purpose is to have some external entity, like the 
// gui code, define where these functions point. gpsim will then use these
// functions as a means of notifying the gui when something has changed.
// In addition to the gui, this class also provides the support for interfacing
// to modules. When a module is loaded from a module library, a new Interface
// object is created for it. The module will be given the opportunity to
// register functions (e.g. provide pointers to functions) that gpsim can
// then call.
//
//--------------------------------------------------------------------------


void gpsimInterface::update  (void)
{

  GSList *external_interfaces = interfaces;

  while(external_interfaces) {

    if(external_interfaces->data) {
      Interface *an_interface = (Interface *)(external_interfaces->data);

      an_interface->Update(an_interface->objectPTR);
    }

    external_interfaces = external_interfaces->next;
  }
}

void gpsimInterface::callback(void)
{
  if(update_rate) {
    future_cycle = get_cycles().get() + update_rate;
    get_cycles().set_break(future_cycle, this);
  }

  update();
}
void gpsimInterface::clear(void)
{

}

void gpsimInterface::print(void)
{
  cout << "Interface update rate " << update_rate << endl;
}

void gpsimInterface::callback_print(void)
{
  cout << "gpsim Interface callback\n";
}

void update_gui(void)
{
  gi.update();
}

gpsimInterface::gpsimInterface (void )
{
  interfaces = 0;
  future_cycle = 0;
  interface_seq_number = 0;
  socket_interface = 0;
  mbSimulating = false;
  mbUseGUI = false;
}

ISimConsole & gpsimInterface::GetConsole()
{
  // The static ISimConsole object is currently in the
  // CCommandManger class because it initially was used
  // to enable external modules to write to the console.
  // We may want to put it somewhere else someday.
  return CCommandManager::m_CommandManger.GetConsole();
}

//--------------------------------------------------------------------------
//
// A xref, or cross reference, object is an arbitrary thing that gpsim
// will pass back to the gui or module. The gui (or module) will then
// interpret the contents of the xref and possibly update some state
// with 'new_value'. An example is when one of the pic registers changes;
// if there's a xref object associated with the register gpsim will
// then notify the gui (or module) through the xref.

void gpsimInterface::update_object (gpointer xref,int new_value)
{

  GSList *interface_list = interfaces;

  while(interface_list) {

    if(interface_list->data) {
      Interface *an_interface = (Interface *)(interface_list->data);

      an_interface->UpdateObject(xref, new_value);
    }

    interface_list = interface_list->next;
  }

}

void gpsimInterface::remove_object (gpointer xref)
{


  GSList *interface_list = interfaces;

  while(interface_list) {

    if(interface_list->data) {
      Interface *an_interface = (Interface *)(interface_list->data);

      an_interface->RemoveObject(xref);
    }

    interface_list = interface_list->next;
  }


}

void gpsimInterface::simulation_has_stopped (void)
{


  GSList *interface_list = interfaces;

  profile_keeper.catchup();     // FIXME: remove this!

  while(interface_list) {

    if(interface_list->data) {
      Interface *an_interface = (Interface *)(interface_list->data);

      an_interface->SimulationHasStopped(an_interface->objectPTR);
    }

    interface_list = interface_list->next;
  }

}

#if GLIB_MAJOR_VERSION >= 2

GMutex *muRunMutex;
GCond  *cvRunCondition;

static Processor *tcpu=0;

static void *run_thread( void *ptr )
{
  printf("run thread\n");
  while(1) {

    g_mutex_lock(muRunMutex);
    printf("running waiting for condition\n");

    g_cond_wait(cvRunCondition, muRunMutex);
    if(tcpu) {
      printf("running\n");
      tcpu->run();
      printf("stopped running\n");
    }

    g_mutex_unlock(muRunMutex);

  }

}


void start_run_thread(void)
{
  std::cout << "starting run thread....\n";

  muRunMutex     = g_mutex_new ();
  cvRunCondition = g_cond_new ();
  g_mutex_lock (muRunMutex);

  GError    *err = NULL ;

  if( g_thread_create((GThreadFunc)run_thread, 
		      (void *)0, 
		      TRUE, 
		      &err) == NULL)
  {
    printf("Thread create failed: %s!!\n", err->message );
    g_error_free ( err ) ;
  }

  g_mutex_unlock (muRunMutex);

  std::cout << " started thread\n";
}
#endif


//========================================================================
//
void gpsimInterface::start_simulation (double duration)
{


  Processor *cpu = get_active_cpu();
  if (cpu) {
    mbSimulating = true;
    cout << "running...\n";
    cpu->run(true);
    mbSimulating = false;
    trace.dump_last_instruction(); 
    simulation_has_stopped();
  }

}

void gpsimInterface::step_simulation (int nSteps)
{
  Processor *cpu = get_active_cpu();

  if (cpu)
    cpu->step(nSteps);

}

void gpsimInterface::advance_simulation(eAdvancementModes nAdvancement)
{
  switch (nAdvancement)
  {
  case eAdvanceNextInstruction:
    {
      Processor *cpu = get_active_cpu();

      if (cpu)
        cpu->step_over();

    }
    break;
  case eAdvanceNextCycle:
  case eAdvanceNextCall:
  case eAdvanceNextReturn:
    break;

  }
}
void gpsimInterface::reset (RESET_TYPE resetType)
{
  CSimulationContext::GetContext()->Reset(resetType);
}

bool gpsimInterface::bSimulating()
{
  return mbSimulating;
}

bool gpsimInterface::bUsingGUI()
{
  return mbUseGUI;
}
void gpsimInterface::setGUImode(bool bnewGUImode)
{
  // We can only turn the gui on we can't turn it off.
  mbUseGUI |= bnewGUImode;
}
void gpsimInterface::new_processor (Processor *new_cpu)
{
  GSList *interface_list = interfaces;

  while(interface_list) {

    if(interface_list->data) {
      Interface *an_interface = (Interface *)(interface_list->data);

      an_interface->NewProcessor(new_cpu);
    }

    interface_list = interface_list->next;
  }

}

void gpsimInterface::new_module (Module *module)
{

  GSList *interface_list = interfaces;

  while(interface_list) {

    if(interface_list->data) {
      Interface *an_interface = (Interface *)(interface_list->data);

      an_interface->NewModule(module);
    }

    interface_list = interface_list->next;
  }

}

void gpsimInterface::node_configuration_changed (Stimulus_Node *node)
{

  GSList *interface_list = interfaces;

  while(interface_list) {

    if(interface_list->data) {
      Interface *an_interface = (Interface *)(interface_list->data);

      an_interface->NodeConfigurationChanged(node);
    }

    interface_list = interface_list->next;
  }

}

void gpsimInterface::new_program  (Processor *cpu)
{

  GSList *interface_list = interfaces;

  while(interface_list) {

    if(interface_list->data) {
      Interface *an_interface = (Interface *)(interface_list->data);

      an_interface->NewProgram(cpu);
    }

    interface_list = interface_list->next;
  }

}

unsigned int  gpsimInterface::add_interface  (Interface *new_interface)
{

  interface_seq_number++;

  new_interface->set_id(interface_seq_number);

  gi.interfaces = g_slist_append(gi.interfaces, new_interface);


  return interface_seq_number;
}

unsigned int  gpsimInterface::add_socket_interface  (Interface *new_interface)
{
  if(!socket_interface)
    return add_interface(new_interface);
  
  return 0;
}

void  gpsimInterface::remove_interface  (unsigned int interface_id)
{
  GSList *interface_list = interfaces;

  while(interface_list) {

    if(interface_list->data) {
      Interface *an_interface = (Interface *)(interface_list->data);

      if(an_interface->get_id()==interface_id)
      {

	gi.interfaces = g_slist_remove(gi.interfaces, an_interface);
	if(an_interface == socket_interface)
	  socket_interface = 0;

	delete an_interface;
	return;
      }
    }

    interface_list = interface_list->next;
  }
  return;
}

void  gpsimInterface::set_update_rate  (guint64 _update_rate)
{
  update_rate = _update_rate;

  if(update_rate)
  {
    guint64 fc = get_cycles().get() + _update_rate;

    if(fc) {
      if(future_cycle)
        get_cycles().reassign_break(future_cycle, fc, this);
      else
        get_cycles().set_break(fc, this);
  
      future_cycle = fc;
    }
  }
}


guint64 gpsimInterface::get_update_rate()
{
  return update_rate;
}



const char *get_dir_delim(const char *path)
{
#ifdef _WIN32
  const char *p = path + strlen(path);

  do
  {
    if (--p < path)
      return 0;
  }
  while (*p != '/' && *p != '\\');

  return p;
#else
  return strrchr(path, '/');
#endif
}

// For libgpsim.dll
// The console now owns the verbose flags. At some point as set
// of functions called TraceDisplayXXX() the conditionally
// display message depending on the verbose flags.
// I'll leave this it as it for now because I'm in the middle
// of making the src project its own DLL on windows and I have 
// enough changes for now.
// Replaced the int verbose = 0; with GlobalVerbosityAccessor verbose.
// GlobalVerbosityAccessor that has overridden operators for 'if(verbose)'
// and 'if(verbose&4)' to still work as desired.
// The purpose was to decouple verbose from cli and gui. Now these
// modules (acutally gpsim.exe) will allocate there own
// GlobalVerbosityAccessor verbose object to gain access to the
// verbose flags and for the overridden operators.
GlobalVerbosityAccessor verbose;

