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

#include "processor.h"
#include "xref.h"
#include "interface.h"
#include "trace.h"
#include "eeprom.h"
#include "icd.h"


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
static gpsimInterface &(*dummy_gi)(void) = get_interface;

//---------------------------------------------------------------------------
//   void gpsim_set_bulk_mode(int flag)
//---------------------------------------------------------------------------
void gpsim_set_bulk_mode(int flag)
{
  if(use_icd)
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
    future_cycle = get_cycles().value + update_rate;
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


// pthread variables.
pthread_attr_t thAttribute;
pthread_mutex_t muRunMutex;
pthread_cond_t  cvRunCondition;
pthread_t thPrint;

static Processor *tcpu=0;

static void *run_thread( void *ptr )
{
  printf("run thread\n");
  while(1) {

    pthread_mutex_lock(&muRunMutex);
    printf("running waiting for condition\n");

    pthread_cond_wait(&cvRunCondition, &muRunMutex);
    if(tcpu) {
      printf("running\n");
      tcpu->run();
      printf("stopped running\n");
    }

    pthread_mutex_unlock(&muRunMutex);

  }

}


void start_run_thread(void)
{
  std::cout << "starting run thread....\n";

  pthread_mutex_init(&muRunMutex, NULL);
  pthread_cond_init (&cvRunCondition, NULL);

  pthread_mutex_lock(&muRunMutex);

  pthread_attr_init(&thAttribute);
  pthread_attr_setdetachstate(&thAttribute, PTHREAD_CREATE_JOINABLE);
  pthread_create(&thPrint, &thAttribute, run_thread, (void *)0);

  pthread_mutex_unlock(&muRunMutex);

  std::cout << " started thread\n";
}


void gpsimInterface::start_simulation (void)
{
  Processor *cpu = get_active_cpu();

  mbSimulating = true;

  if(cpu) {

    static bool thread_initialized=false;

    if(!thread_initialized) {
      start_run_thread();
      g_usleep(10000);
      thread_initialized = true;
    }

    pthread_mutex_lock(&muRunMutex);

    tcpu = cpu;
    /*
    if(verbosity && verbosity->getVal()) {
      cout << "running...\n";
      cpu->run(true);
    } else
      cpu->run(false);
    */
    printf("signalling run thread\n");
    pthread_cond_signal(&cvRunCondition);
    pthread_mutex_unlock(&muRunMutex);
    printf("leaving start_simulation\n");
  }

  mbSimulating = false;
}

bool gpsimInterface::bSimulating()
{
  return mbSimulating;
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

  guint64 fc = get_cycles().value + _update_rate;

  update_rate = _update_rate;

  if(fc) {
    if(future_cycle)
      get_cycles().reassign_break(future_cycle, fc, this);
    else
      get_cycles().set_break(fc, this);
  
    future_cycle = fc;
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
