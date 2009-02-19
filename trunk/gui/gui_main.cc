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

GUI_Processor *gpGuiProcessor=0;
GSList *gui_processors=0;
unsigned int interface_id=0;

Settings *settings;

extern GtkWidget *dispatcher_window;
extern void dispatch_Update();

//------------------------------------------------------------------------
// 
extern bool gUsingThreads(); // in ../src/interface.cc

//------------------------------------------------------------------------
// debug
static void gtl()
{
#if GLIB_MAJOR_VERSION >= 2
  if(gUsingThreads())
    gdk_threads_leave ();
#endif
}
static void gte()
{
#if GLIB_MAJOR_VERSION >= 2
  if(gUsingThreads())
    gdk_threads_enter ();
#endif
}

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
  virtual void Update  (gpointer object);

  virtual ~GUI_Interface();

  GUI_Interface(GUI_Processor *_gp);

};


GUI_Interface::GUI_Interface(GUI_Processor *_gp) : Interface ( (gpointer *) _gp)
{

  gp = _gp;

}

GUI_Interface::~GUI_Interface()
{
  if(gp) {
    gp->regwin_ram->set_config();
    gp->regwin_eeprom->set_config();
    gp->program_memory->set_config();
    gp->source_browser->set_config();
    gp->watch_window->set_config();
    gp->stack_window->set_config();
    gp->breadboard_window->set_config();
    gp->trace_window->set_config();
    gp->profile_window->set_config();
    gp->stopwatch_window->set_config();
    gp->scope_window->set_config();
  }
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

  gte ();
  CrossReferenceToGUI  *xref = (CrossReferenceToGUI *)gui_xref;
  xref->Update(new_value);
  gtl ();

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

#if GLIB_MAJOR_VERSION >= 2
// thread variables.
static GMutex *muSimStopMutex=0;
static GCond  *cvSimStopCondition=0;
#endif

extern int gui_animate_delay; // in milliseconds

static GUI_Processor *lgp=0;

static void *SimulationHasStopped( void *ptr )
{

#if GLIB_MAJOR_VERSION >= 2
  while(1) {
    if(gUsingThreads()) {
      g_mutex_lock(muSimStopMutex);
      g_cond_wait(cvSimStopCondition, muSimStopMutex);
    }
#endif
    if(lgp) {
      GTKWAIT;

      lgp->regwin_ram->Update();
      lgp->regwin_eeprom->Update();
      lgp->program_memory->Update();
      lgp->source_browser->Update();
      lgp->watch_window->Update();
      lgp->stack_window->Update();
      lgp->breadboard_window->Update();
      lgp->trace_window->Update();
      lgp->profile_window->Update();
      lgp->stopwatch_window->Update();
      lgp->scope_window->Update();


      
    }
    
    if(gui_animate_delay!=0)
          usleep(1000*gui_animate_delay);

    dispatch_Update();

#if GLIB_MAJOR_VERSION >= 2
    if(gUsingThreads()) 
      g_mutex_unlock(muSimStopMutex);
    else
      return 0;
  }
#endif

}


/*------------------------------------------------------------------
 * SimulationHasStopped
 *
 */

void GUI_Interface::SimulationHasStopped(gpointer callback_data)
{
  if(callback_data) {
    
    lgp = (GUI_Processor *) callback_data;
#if GLIB_MAJOR_VERSION >= 2

    if(gUsingThreads()) {
      g_mutex_lock(muSimStopMutex);
      g_cond_signal(cvSimStopCondition);
      g_mutex_unlock(muSimStopMutex);
    } else
#endif
    ::SimulationHasStopped(0);

  }
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
    gte ();

    gp->SetCPU(new_cpu);

    gui_processors = g_slist_append(gui_processors,gp);

    gp->regwin_ram->NewProcessor(gp);
    gp->program_memory->NewProcessor(gp);
    gp->source_browser->CloseSource();
    gp->source_browser->NewProcessor(gp);
    gp->symbol_window->NewSymbols();
    //gp->watch_window->NewProcessor(gp);
    gp->breadboard_window->NewProcessor(gp);
    gp->stack_window->NewProcessor(gp);
    gp->trace_window->NewProcessor(gp);
    gp->profile_window->NewProcessor(gp);
    gp->stopwatch_window->NewProcessor(gp);
    //gp->scope_window->NewProcessor(gp);

    Dprintf((" New processor has been added"));
    gtl ();
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
    gte ();

    gp->regwin_eeprom->NewProcessor(gp);
      
    gp->source_browser->CloseSource();
    gp->source_browser->NewSource(gp);
    gp->symbol_window->NewSymbols();
    gp->program_memory->NewSource(gp);
    gp->profile_window->NewProgram(gp);
    gp->watch_window->NewProcessor(gp);
    link_src_to_gpsim( gp);
    gtl ();

  }
}


void GUI_Interface::Update(gpointer object)
{
  SimulationHasStopped(object);
}



/*------------------------------------------------------------------
 * 
 */
#if GTK_MAJOR_VERSION >= 2
int gStringWidth(PangoFontDescription *font, const char *str)
{
  return (font && str) ? gdk_string_width (gdk_font_from_description(font),str) : 0;
}
int gStringHeight(PangoFontDescription *font, const char *str)
{
  return (font && str) ? gdk_string_height (gdk_font_from_description(font),str) : 0;
}
GdkFont *gFontFromDescription(PangoFontDescription *font)
{
  return (font ? gdk_font_from_description(font) : 0);
}
#else
int gStringWidth(GdkFont *font, const char *str)
{
  return (font && str) ? gdk_string_width (font,str) : 0;
}
int gStringHeight(GdkFont *font, const char *str)
{
  return (font && str) ? gdk_string_height (font,str) : 0;
}
#endif


/*------------------------------------------------------------------
 * quit_gui
 *
 */
void quit_gui(void)
{
  if(!get_interface().bUsingGUI())
    return;

    int x,y,width,height;

    gdk_window_get_root_origin(dispatcher_window->window,&x,&y);
    gdk_window_get_size(dispatcher_window->window,&width,&height);

    config_set_variable("dispatcher", "enable", 1);
    config_set_variable("dispatcher", "x", x);
    config_set_variable("dispatcher", "y", y);
    config_set_variable("dispatcher", "width", width);
    config_set_variable("dispatcher", "height", height);

    get_interface().remove_interface(interface_id);

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

#if GLIB_MAJOR_VERSION >= 2
  if(gUsingThreads()) {

    GThread          *Thread1;
    GError           *err1 = NULL ;
  
    muSimStopMutex     = g_mutex_new ();
    cvSimStopCondition = g_cond_new ();
    g_mutex_lock(muSimStopMutex);

    if( (Thread1 = g_thread_create((GThreadFunc)SimulationHasStopped, 
				   (void *)0, 
				   TRUE, 
				   &err1)) == NULL)
      {
	printf("Thread create failed: %s!!\n", err1->message );
	g_error_free ( err1 ) ;
      }
    g_mutex_unlock(muSimStopMutex);
  }
#endif

  if (!gtk_init_check (&argc, &argv))
  {
      return -1;
  }

  // standardize IO format ref bug 1832702
  setlocale(LC_NUMERIC, "C");

  gte();
  gpGuiProcessor = new GUI_Processor();
  interface_id = get_interface().add_interface(new GUI_Interface(gpGuiProcessor));
  gtl();

  return(0);
}

void gui_main(void)
{
  gte ();
  gtk_main ();
  gtl ();
}

#endif //HAVE_GUI
