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
#include "gui_breadboard.h"
#include "gui_interface.h"
#include "gui_processor.h"
#include "gui_profile.h"
#include "gui_register.h"
#include "gui_regwin.h"
#include "gui_scope.h"
#include "gui_src.h"
#include "gui_stack.h"
#include "gui_statusbar.h"
#include "gui_stopwatch.h"
#include "gui_symbols.h"
#include "gui_trace.h"
#include "gui_watch.h"

#ifndef _WIN32
#undef TRUE
#undef FALSE
#include "settings_exdbm.h"
#else
#include "settings_reg.h"
#endif

extern int gui_animate_delay; // in milliseconds

/*
 * --- Function prototypes
 */
void init_link_to_gpsim(GUI_Processor *gp);
void link_src_to_gpsim(GUI_Processor *gp);

/* 
 * --- Global variables
 */

GUI_Processor *gp=0;
GSList *gui_processors=0;
unsigned int interface_id=0;

Settings *settings;

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
  virtual void NewProcessor (Processor *);
  virtual void NewModule (Module *module);
  virtual void NodeConfigurationChanged (Stimulus_Node *node);
  virtual void NewProgram  (Processor *);
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
    gp->program_memory->Update();
    gp->source_browser->Update();
    gp->watch_window->Update();
    gp->stack_window->Update();
    gp->breadboard_window->Update();
    gp->trace_window->Update();
    gp->profile_window->Update();
    gp->stopwatch_window->Update();
    //gp->scope_window->Update();

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

void GUI_Interface::NewProcessor (Processor *new_cpu)
{

  // Create a gui representation of the new processor

  if(gp) {

    gp->cpu = new_cpu;

    gui_processors = g_slist_append(gui_processors,gp);

    gp->regwin_ram->NewProcessor(gp);
    gp->program_memory->NewProcessor(gp);
    gp->source_browser->CloseSource();
    gp->source_browser->NewProcessor(gp);
    gp->symbol_window->NewSymbols();
    gp->watch_window->ClearWatches();
    gp->breadboard_window->NewProcessor(gp);
    gp->stack_window->NewProcessor(gp);
    gp->trace_window->NewProcessor(gp);
    gp->profile_window->NewProcessor(gp);
    gp->stopwatch_window->NewProcessor(gp);
    //gp->scope_window->NewProcessor(gp);


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
void GUI_Interface::NewProgram (Processor *new_cpu)
{

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
 * gui_init
 *
 */

int gui_init (int argc, char **argv)
{
#ifndef _WIN32
  settings = new SettingsEXdbm("gpsim");
#else
  settings = new SettingsReg("gpsim");
#endif

  // gtk is thread aware but not thread safe.
  // Under certain circumstances, it is possible for
  // gpsim to send commands to the gui from multiple 
  // threads (e.g. when communicating to an external
  // module or communicating across a socket).
  //#if GTK_MAJOR_VERSION >= 2
  //  gdk_threads_init();
  //#endif
  //  g_thread_init (NULL);

  if (!gtk_init_check (&argc, &argv))
  {
      return -1;
  }

    
  gp = new GUI_Processor();

  get_interface().add_interface(new GUI_Interface(gp));
  return(0);
}

void gui_main(void)
{
  //  GDK_THREADS_ENTER ();
  gtk_main ();
  //  GDK_THREADS_LEAVE ();
}

#endif //HAVE_GUI
