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
// that hides the details of the simulator. In addition, the interface
// is written with 'C' callable functions. Currently only the gui
// interfaces to gpsim through this layer. However, since the simulator
// 'engine' is built as a library, it's possible for other code to
// interface through here as well.
//
//-------------------------------------------------------------------

#include <stdio.h>

#include "../config.h"
#define GPSIM_VERSION VERSION 
#include "gpsim_def.h"

#include "pic-processor.h"
#include "p16x8x.h"
#include "xref.h"
#include "interface.h"
#include "trace.h"
#include "eeprom.h"
#include "icd.h"


// Flag to tell us when all of the init stuff is done.
unsigned int gpsim_is_initialized = 0;

/**************************************************************************
 *
 *  Here's the gpsim interface class instantiation. It's through this class
 * that gpsim will notify the gui and/or modules of internal gpsim changes.
 *
 **************************************************************************/

gpsimInterface gi;



//--------------------------------------------------------------------------
// 
// hack...
pic_processor *get_pic_processor(unsigned int processor_id)
{
  return (pic_processor *)get_processor(processor_id);
}

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

extern void process_command_file(const char * file_name);
extern int open_cod_file(pic_processor **, char *);
#include "../cli/command.h"

int gpsim_open(unsigned int processor_id, const char *file)
{
    char *str;
    pic_processor *pic = get_pic_processor(processor_id);

    str = strrchr(file,'.');
    if(str==0)
    {
//	puts("found no dot in file!");
	return 0;
    }
    str++;
    if(!strcmp(str,"hex"))
    {

	if(!pic)
	{
	    puts("No pic selected!");
	    return 0;
	}
	pic->load_hex(file);
    
    }
    else if(!strcmp(str,"cod"))
    {

	int i;
	i=load_symbol_file(&pic, file);

	if(i)
	{
	    cout << "found a fatal error in the symbol file " << file <<'\n';
	    return 0;
	}

	// FIXME: questionable
	command_list[0]->cpu=pic;
	trace.switch_cpus(pic);
      
    }
    else if(!strcmp(str,"stc"))
    {

	process_command_file(file);
    }
    else
    {
      cout << "Unknown file extension \"" << str <<"\" \n";
	return 0;
    }

    return 1;
}







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

  interface_id = 0;
  objectPTR = new_object;

}

Interface *get_interface(unsigned int interface_id)
{

  GSList *interface_list = gi.interfaces;

  while(interface_list) {

    if(interface_list->data) {
      Interface *an_interface = (struct Interface *)(interface_list->data);


      if(an_interface->get_id() == interface_id)
	return an_interface;

    }

    interface_list = interface_list->next;
  }

  return 0;

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


static void gui_update_callback  (gpointer p)
{
  gpsimInterface *gpsiminterface = (gpsimInterface *)p;
  GSList *interface_list = gpsiminterface->interfaces;

  while(interface_list) {

    if(interface_list->data) {
      Interface *an_interface = (struct Interface *)(interface_list->data);

      an_interface->GuiUpdate(an_interface->objectPTR);
    }

    interface_list = interface_list->next;
  }
}

void update_gui(void)
{
    gui_update_callback(&gi);
}

gpsimInterface::gpsimInterface (void )
{

  interfaces = 0;
  gui_update_cbp  = 0;
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
      Interface *an_interface = (struct Interface *)(interface_list->data);

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
      Interface *an_interface = (struct Interface *)(interface_list->data);

      an_interface->RemoveObject(xref);
    }

    interface_list = interface_list->next;
  }


}
void gpsimInterface::simulation_has_stopped (void)
{

  GSList *interface_list = interfaces;

  profile_keeper.catchup();

  while(interface_list) {

    if(interface_list->data) {
      Interface *an_interface = (struct Interface *)(interface_list->data);

      an_interface->SimulationHasStopped(an_interface->objectPTR);
    }

    interface_list = interface_list->next;
  }

}

void gpsimInterface::new_processor (unsigned int processor_id)
{
    set_update_rate  (gui_update_rate);

  GSList *interface_list = interfaces;

  while(interface_list) {

    if(interface_list->data) {
      Interface *an_interface = (struct Interface *)(interface_list->data);

      an_interface->NewProcessor(processor_id);
    }

    interface_list = interface_list->next;
  }

}

void gpsimInterface::new_module (Module *module)
{

  GSList *interface_list = interfaces;

  while(interface_list) {

    if(interface_list->data) {
      Interface *an_interface = (struct Interface *)(interface_list->data);

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
      Interface *an_interface = (struct Interface *)(interface_list->data);

      an_interface->NodeConfigurationChanged(node);
    }

    interface_list = interface_list->next;
  }

}

void gpsimInterface::new_program  (unsigned int processor_id)
{

  GSList *interface_list = interfaces;

  while(interface_list) {

    if(interface_list->data) {
      Interface *an_interface = (struct Interface *)(interface_list->data);

      an_interface->NewProgram(processor_id);
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

void  gpsimInterface::remove_interface  (unsigned int interface_id)
{
  GSList *interface_list = interfaces;

  while(interface_list) {

    if(interface_list->data) {
      Interface *an_interface = (struct Interface *)(interface_list->data);

      if(an_interface->get_id()==interface_id)
      {
	  gi.interfaces = g_slist_remove(gi.interfaces, an_interface);
          return;
      }
    }

    interface_list = interface_list->next;
  }
  return;
}

void  gpsimInterface::set_update_rate  (guint64 update_rate)
{
    gui_update_rate = update_rate;
    if(active_cpu!=0)
    {
	if(gui_update_cbp==0)
	{
	    gui_update_cbp = new CyclicBreakPoint();
	    ((CyclicBreakPoint*)gui_update_cbp)->pic = active_cpu;
	    ((CyclicBreakPoint*)gui_update_cbp)->callback_function = gui_update_callback;
	    ((CyclicBreakPoint*)gui_update_cbp)->callback_data = this;
	    ((CyclicBreakPoint*)gui_update_cbp)->delta_cycles = DEFAULT_GUI_UPDATE_RATE;
	    ((CyclicBreakPoint*)gui_update_cbp)->set_break();
	}
	((CyclicBreakPoint*)gui_update_cbp)->set_delta(update_rate);
    }
}






//--------------------------------------------------------------------------
//
// valid_register - local function
//  Given a pointer to a pic processor, the type of register (file or eeprom 
//  for now), and the register number, this routine will return true if the
//  register is a valid one. 'Valid' means that the register is accessable
//  by the pic software that runs on the simulated processor.
//
unsigned int valid_register(pic_processor *pic, REGISTER_TYPE type, unsigned int register_number)
{

  if(!pic) 
    return 0;

  if(type == REGISTER_EEPROM)
  {
    if(pic->eeprom && (register_number < pic->eeprom->get_rom_size()))
      return TRUE;

    return FALSE;
  }
  
  if(register_number < pic->register_memory_size())
    {
      Register *reg = pic->registers[register_number];

      
      while(reg->isa() == Register::BP_REGISTER)
	{
	  reg = ((Notify_Register *)reg)->replaced;
	}


      if(reg->isa() == Register::INVALID_REGISTER)
	return 0;
      else
	return 1;
    }
  else
    cout << "Warning: Request for register 0x " << hex << register_number
	 << " in processor " << pic->name()
	 << " is out of range\n";


  return 0;
}

//--------------------------------------------------------------------------
// Register *gpsim_get_register(
//         unsigned int processor_id, 
//         REGISTER_TYPE type, 
//         unsigned int register_number)
//
// given a processor id, a register type, and the register number, this routine
// will return a pointer to the referenced file register. If the register doesn't
// exist, then NULL is returned.
//

Register *gpsim_get_register(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number)
{
  pic_processor *pic = (pic_processor *)get_processor(processor_id);

  if(!pic)
    return 0;

  if(!valid_register(pic,type,register_number))
    return 0;
  
  switch(type) {
  case REGISTER_RAM:
    return pic->registers[register_number];
    break;
  case REGISTER_EEPROM:
    if(pic->eeprom)
      return pic->eeprom->get_register(register_number);
    // fall through to return 0
  default:
    return 0;
  }

}


//--------------------------------------------------------------------------

char *gpsim_get_register_name(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number)
{
  static char buffer[128];
  char *name = 0;
  Register *fr = gpsim_get_register( processor_id, type,  register_number);

  if(fr)
    name = fr->name();
  if(!name)
    return 0;
  
  if(gpsim_register_is_alias(processor_id, type, register_number))
      sprintf(buffer,"alias (%s)", name);
  else
      strcpy(buffer,name);

  return buffer;
}


//--------------------------------------------------------------------------

gboolean gpsim_register_is_alias(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number)
{
  Register *fr = gpsim_get_register( processor_id, type,  register_number);

  if(fr) 
    if(fr->address!=register_number)
      return TRUE;

  return FALSE;

}



