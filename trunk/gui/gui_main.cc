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
#include "../src/gpsim_interface.h"

#include "gui.h"
#include "gui_interface.h"

#undef TRUE
#undef FALSE
// #include <eXdbm.h>

extern "C" {
#include "../eXdbm/eXdbm.h"
}

extern int gui_animate_delay; // in milliseconds

/*
 * --- Function prototypes
 */
void redisplay_prompt(void);

void init_link_to_gpsim(GUI_Processor *gp);
void link_src_to_gpsim(GUI_Processor *gp);

/* 
 * --- Global variables
 */

GUI_Processor *gp=0;
GSList *gui_processors=0;
unsigned int interface_id=0;

DB_ID dbid=-1;

extern GtkWidget *dispatcher_window;

//------------------------------------------------------------------------
void CrossReferenceToGUI::Update(int new_value)
{
  printf("CrossReferenceToGUI::Update shouldn't be called!\n");
}

void CrossReferenceToGUI::Remove(void)
{
  printf("CrossReferenceToGUI::Remove shouldn't be called!\n");
}

//------------------------------------------------------------------------
//

class GUI_Interface : public Interface
{
private:
  GUI_Processor *gp;

public:

  virtual void UpdateObject (gpointer xref,int new_value);
  virtual void RemoveObject (gpointer xref);
  virtual void SimulationHasStopped (gpointer object);
  virtual void NewProcessor (unsigned int processor_id);
  virtual void NewModule (Module *module);
  virtual void NodeConfigurationChanged (Stimulus_Node *node);
  virtual void NewProgram  (unsigned int processor_id);
  virtual void GuiUpdate  (gpointer object);


  GUI_Interface(GUI_Processor *_gp);
};


GUI_Interface::GUI_Interface(GUI_Processor *_gp) : Interface ( (gpointer *) _gp)
{

  gp = _gp;

}


/*------------------------------------------------------------------
 * UpdateObject
 *
 * Each 'thing' that the gui displays about a simulated pic has an
 * associated cross reference structure. Sometimes these 'things' 
 * displayed in more than one place (like the status register).
 * Each graphical instance has its own structure. All of the structures
 * pertaining to the same pic object (again, like the status register)
 * are stored in a singly-linked list. This routine scans through
 * this list and updates each instance of the object.
 */

void GUI_Interface::UpdateObject(gpointer gui_xref,int new_value)
{

  CrossReferenceToGUI  *xref = (CrossReferenceToGUI *)gui_xref;
  xref->Update(new_value);

}

/*------------------------------------------------------------------
 * RemoveObject
 *
 */

void GUI_Interface::RemoveObject(gpointer gui_xref)
{

  CrossReferenceToGUI  *xref = (CrossReferenceToGUI *)gui_xref;
  xref->Remove();

}

/*------------------------------------------------------------------
 * SimulationHasStopped
 *
 */

void GUI_Interface::SimulationHasStopped(gpointer callback_data)
{
  while(gtk_events_pending())
    gtk_main_iteration();

  if(callback_data) {
    
    GUI_Processor *gp = (GUI_Processor *) callback_data;

    gp->regwin_ram->Update();
    gp->regwin_eeprom->Update();
    gp->status_bar->Update();
    gp->program_memory->Update();
    gp->source_browser->Update();
    gp->watch_window->Update();
    gp->stack_window->Update();
    gp->breadboard_window->Update();
    gp->trace_window->Update();
    gp->profile_window->Update();
    gp->stopwatch_window->Update();
    gp->scope_window->Update();

  }

  if(gui_animate_delay!=0)
      usleep(1000*gui_animate_delay);

  while(gtk_events_pending())
      gtk_main_iteration();
}


/*------------------------------------------------------------------
 * NewProcessor - Add a new processor 
 *
 * This routine adds another processor to the list of currently
 * simulated processors (as of 0.0.14 though, you're still limited
 * to a list of one). It then notifies each child window. Finally
 * a communication link between the gui and the simulator is established.
 */

void GUI_Interface::NewProcessor (unsigned int pic_id)
{

  // Create a gui representation of the new processor

  if(gp) {
    gp->pic_id = pic_id;
    gp->cpu = dynamic_cast<Processor *>(get_processor(pic_id));

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
    gp->scope_window->NewProcessor(gp);


  }

}
/*------------------------------------------------------------------
 *
 */
void GUI_Interface::NewModule (Module *module)
{

  // FIX ME - need to search for *p in the gp list...
  if(gp)
    gp->breadboard_window->NewModule(module);
}

/*------------------------------------------------------------------
 *
 */
void GUI_Interface::NodeConfigurationChanged (Stimulus_Node *node)
{

  // FIX ME - need to search for *p in the gp list...
  if(gp)
    gp->breadboard_window->NodeConfigurationChanged(node);
}

/*------------------------------------------------------------------
 *
 */
void GUI_Interface::NewProgram (unsigned int pic_id)
{

  // FIX ME - need to search for *p in the gp list...
  if(gp) {

    // this is here because the eeprom is not set to values in cod
    // when gui_new_processor is run. eeprom is with program memory data
    gp->regwin_eeprom->NewProcessor(gp);
      
    gp->source_browser->CloseSource();
    gp->source_browser->NewSource(gp);
    gp->symbol_window->NewSymbols();
    gp->program_memory->NewSource(gp);
    gp->profile_window->NewProgram(gp);
    link_src_to_gpsim( gp);

  }
}


void GUI_Interface::GuiUpdate(gpointer object)
{
  SimulationHasStopped(object);
}









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
      gp->cpu = get_processor(pic_id);

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

#if 0
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
#endif //  0


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
  if(homedir==0)
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

  gi.add_interface(new GUI_Interface(gp));
  return(0);
}

void gui_main(void)
{
  redisplay_prompt();
  gtk_main();

}

#endif //HAVE_GUI
