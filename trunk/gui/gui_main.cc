/*
   Copyright (C) 1998,1999,2000,2001
   T. Scott Dattalo and Ralf Forsberg

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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../config.h"
#ifdef HAVE_GUI

#include <unistd.h>
#include <gtk/gtk.h>

#include <gdk/gdktypes.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#include "../src/gpsim_def.h"

//extern pic_processor *get_processor(unsigned int processor_id);

#include "gui.h"
#include "gui_interface.h"

#undef TRUE
#undef FALSE
// #include <eXdbm.h>

extern "C" {
#include "../eXdbm/eXdbm.h"
}


/*
 * --- Function prototypes
 */
void redisplay_prompt(void);

void init_link_to_gpsim(GUI_Processor *gp);
void link_src_to_gpsim(GUI_Processor *gp);

/* 
 * --- Global variables
 */

GUI_Processor *gp=NULL;
GSList *gui_processors=NULL;
unsigned int interface_id=0;

DB_ID dbid=-1;

extern GtkWidget *dispatcher_window;

/*------------------------------------------------------------------
 * quit_gui
 *
 */
void quit_gui(void)
{
    int x,y,width,height;

    gdk_window_get_root_origin(dispatcher_window->window,&x,&y);
    gdk_window_get_size(dispatcher_window->window,&width,&height);

    config_set_variable("dispatcher", "enable", 1);
    config_set_variable("dispatcher", "x", x);
    config_set_variable("dispatcher", "y", y);
    config_set_variable("dispatcher", "width", width);
    config_set_variable("dispatcher", "height", height);

    gtk_main_quit();
}

/*------------------------------------------------------------------
 * gui_new_processor - Add a new processor 
 *
 * This routine adds another processor to the list of currently
 * simulated processors (as of 0.0.14 though, you're still limited
 * to a list of one). It then notifies each child window. Finally
 * a communication link between the gui and the simulator is established.
 */

void gui_new_processor (unsigned int pic_id)
{

  // Create an gui representation of the new processor

  if(gp)
    {
      gp->pic_id = pic_id;
      gui_processors = g_slist_append(gui_processors,gp);

      gp->regwin_ram->NewProcessor(gp);
      gp->status_bar->NewProcessor(gp);
      gp->program_memory->NewProcessor(gp);
      gp->source_browser->CloseSource();
      gp->symbol_window->NewSymbols();
      gp->watch_window->ClearWatches();
      gp->breadboard_window->NewProcessor(gp);
      gp->stack_window->NewProcessor(gp);
      gp->trace_window->NewProcessor(gp);
      gp->profile_window->NewProcessor(gp);
      gp->stopwatch_window->NewProcessor(gp);


    }

}

/*------------------------------------------------------------------
 *
 */
void gui_new_module (Module *module)
{

  // FIX ME - need to search for *p in the gp list...
  if(gp)
    gp->breadboard_window->NewModule(module);
}

/*------------------------------------------------------------------
 *
 */
void gui_node_configuration_changed (Stimulus_Node *node)
{

  // FIX ME - need to search for *p in the gp list...
  if(gp)
    gp->breadboard_window->NodeConfigurationChanged(node);
}

/*------------------------------------------------------------------
 *
 */
void gui_new_program (unsigned int pic_id)
{

  // FIX ME - need to search for *p in the gp list...
  if(gp)
  {

      // this is here because the eeprom is not set to values in cod
      // when gui_new_processor is run. eeprom is with program memory data
      gp->regwin_eeprom->NewProcessor(gp);
      
      gp->source_browser->CloseSource();
      gp->symbol_window->NewSymbols();
      gp->program_memory->NewSource(gp);
      gp->profile_window->NewProgram(gp);
      link_src_to_gpsim( gp);
      //      redisplay_prompt();
    }
}

/*------------------------------------------------------------------
 *
 */
void gui_new_source (unsigned int pic_id)
{

  // FIX ME - need to search for *p in the gp list...
  if(gp) {
	
    gp->program_memory->NewSource(gp);
    gp->source_browser->NewSource(gp);
    gp->symbol_window->NewSymbols();
    link_src_to_gpsim( gp);

  }
}

/*------------------------------------------------------------------
 * update_program_memory
 *
 */
void update_program_memory(GUI_Processor *gp, unsigned int reg_number)
{

    //  printf("program memory needs to be updated\n");

}
/*------------------------------------------------------------------
 * gui_update_object
 *
 * Each 'thing' that the gui displays about a simulated pic has an
 * associated cross reference structure. Sometimes these 'things' 
 * displayed in more than one place (like the status register).
 * Each graphical instance has its own structure. All of the structures
 * pertaining to the same pic object (again, like the status register)
 * are stored in a singly-linked list. This routine scans through
 * this list and updates each instance of the object.
 */

void gui_update_object(gpointer gui_xref,int new_value)
{

  struct cross_reference_to_gui *xref;

  xref = (struct cross_reference_to_gui *)gui_xref;
  xref->update(xref,new_value);

}

/*------------------------------------------------------------------
 * gui_remove_object
 *
 */

void gui_remove_object(gpointer gui_xref)
{

  struct cross_reference_to_gui *xref;

  xref = (struct cross_reference_to_gui *)gui_xref;
  if(xref->remove)
    xref->remove(xref);

}

int config_set_string(char *module, char *entry, char *string)
{
    int ret;
    DB_LIST list;

    list = eXdbmGetList(dbid, NULL, module);
    if(list==NULL)
    {
	ret = eXdbmCreateList(dbid, NULL, module, NULL);
	if(ret==-1)
	{
	    puts(eXdbmGetErrorString(eXdbmGetLastError()));
	    return 0;
	}
	
	list = eXdbmGetList(dbid, NULL, module);
	if(list==NULL)
	{
	    puts(eXdbmGetErrorString(eXdbmGetLastError()));
	    return 0;
	}
    }

    // We have the list
    
    ret = eXdbmChangeVarString(dbid, list, entry, string);
    if(ret == -1)
    {
	ret = eXdbmCreateVarString(dbid, list, entry, NULL, string);
	if(ret==-1)
	{
	    puts("\n\n\n\ndidn't work");
	    puts(eXdbmGetErrorString(eXdbmGetLastError()));
	    puts("\n\n\n\n");
	    return 0;
	}
    }
    ret=eXdbmUpdateDatabase(dbid);
    if(ret==-1)
    {
	puts(eXdbmGetErrorString(eXdbmGetLastError()));
	return 0;
    }
    return 1;
}

int config_set_variable(char *module, char *entry, int value)
{
    int ret;
    DB_LIST list;

    list = eXdbmGetList(dbid, NULL, module);
    if(list==NULL)
    {
	ret = eXdbmCreateList(dbid, NULL, module, NULL);
	if(ret==-1)
	{
	    puts(eXdbmGetErrorString(eXdbmGetLastError()));
	    return 0;
	}
	
	list = eXdbmGetList(dbid, NULL, module);
	if(list==NULL)
	{
	    puts(eXdbmGetErrorString(eXdbmGetLastError()));
	    return 0;
	}
    }

    // We have the list
    
    ret = eXdbmChangeVarInt(dbid, list, entry, value);
    if(ret == -1)
    {
	ret = eXdbmCreateVarInt(dbid, list, entry, NULL, value);
	if(ret==-1)
	{
	    puts("\n\n\n\ndidn't work");
	    puts(eXdbmGetErrorString(eXdbmGetLastError()));
	    puts("\n\n\n\n");
	    return 0;
	}
    }
    ret=eXdbmUpdateDatabase(dbid);
    if(ret==-1)
    {
	puts(eXdbmGetErrorString(eXdbmGetLastError()));
	return 0;
    }
    return 1;
}

int config_get_variable(char *module, char *entry, int *value)
{
    int ret;
    DB_LIST list;

    list = eXdbmGetList(dbid, NULL, module);
    if(list==NULL)
	return 0;

    // We have the list
    
    ret = eXdbmGetVarInt(dbid, list, entry, value);
    if(ret == -1)
	return 0;
    
    return 1;
}

int config_get_string(char *module, char *entry, char **string)
{
    int ret;
    DB_LIST list;

    list = eXdbmGetList(dbid, NULL, module);
    if(list==NULL)
	return 0;

    // We have the list
    
    ret = eXdbmGetVarString(dbid, list, entry, string);
    if(ret == -1)
	return 0;
    
    return 1;
}

/*------------------------------------------------------------------
 * gui_init
 *
 */

int gui_init (int argc, char **argv)
{
    int ret;

    char path[256], *homedir;
    
  ret = eXdbmInit();
  if(ret==-1)
  {
      puts(eXdbmGetErrorString(eXdbmGetLastError()));
  }

  homedir=getenv("HOME");
  if(homedir==NULL)
      homedir=".";

  sprintf(path,"%s/.gpsim",homedir);
  
  ret = eXdbmOpenDatabase(path,&dbid);
  if(ret==-1)
  {
      int error=eXdbmGetLastError();
      if(error==DBM_OPEN_FILE)
      {
	  ret=eXdbmNewDatabase(path,&dbid);
	  if(ret==-1)
	      puts(eXdbmGetErrorString(eXdbmGetLastError()));
	  else
	  {
	      ret=eXdbmUpdateDatabase(dbid);
	      if(ret==-1)
		  puts(eXdbmGetErrorString(eXdbmGetLastError()));
	  }
      }
      else
	  puts(eXdbmGetErrorString(eXdbmGetLastError()));
  }


  gtk_init (&argc, &argv);

    
  gp = new GUI_Processor();

  interface_id = gpsim_register_interface((gpointer) gp);
  gpsim_register_update_object(interface_id,gui_update_object);
  gpsim_register_remove_object(interface_id, gui_remove_object);
  gpsim_register_new_processor(interface_id, gui_new_processor);
  gpsim_register_simulation_has_stopped(interface_id, gui_simulation_has_stopped);
  gpsim_register_new_program(interface_id, gui_new_program);
  gpsim_register_new_module(interface_id, gui_new_module);
  gpsim_register_node_configuration_changed(interface_id, gui_node_configuration_changed);
  gpsim_register_gui_update(interface_id, gui_simulation_has_stopped);

  return(0);
}

void gui_main(void)
{
  redisplay_prompt();
  gtk_main();

}

#endif //HAVE_GUI
