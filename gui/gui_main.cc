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
// debug
static void gtl()
{
  printf("gdk_threads_leave\n");
  gdk_threads_leave ();
}
static void gte()
{
  printf("gdk_threads_enter\n");
  gdk_threads_enter ();
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
    //gp->scope_window->set_config();
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

  printf("%s\n",__FUNCTION__);
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

// pthread variables.
pthread_attr_t thSimStopAttr;
pthread_mutex_t muSimStopMutex;
pthread_cond_t  cvSimStopCondition;
pthread_t thSimStop;

static GUI_Processor *lgp=0;

static void *SimulationHasStopped( void *ptr )
{
  printf("sim stopped thread started\n");
  while(1) {

    pthread_mutex_lock(&muSimStopMutex);
    printf("waiting for sim stop condition\n");

    pthread_cond_wait(&cvSimStopCondition, &muSimStopMutex);
    printf("got a  sim stop condition\n");

    if(lgp) {
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
    }

    pthread_mutex_unlock(&muSimStopMutex);

  }

}


/*------------------------------------------------------------------
 * SimulationHasStopped
 *
 */

void GUI_Interface::SimulationHasStopped(gpointer callback_data)
{
  //while(gtk_events_pending())
  //  gtk_main_iteration();
  printf("%s\n",__FUNCTION__);

  if(callback_data) {
    
    lgp = (GUI_Processor *) callback_data;

    printf("signalling gui update thread\n");
    pthread_mutex_lock(&muSimStopMutex);

    pthread_cond_signal(&cvSimStopCondition);
    pthread_mutex_unlock(&muSimStopMutex);
    printf("leaving simulation has stopped\n");

    /*
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
    */
  }
  /*
  if(gui_animate_delay!=0)
      usleep(1000*gui_animate_delay);

  while(gtk_events_pending())
      gtk_main_iteration();
  */

  gtl ();
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
    link_src_to_gpsim( gp);
    gtl ();

  }
}


void GUI_Interface::Update(gpointer object)
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

    get_interface().remove_interface(interface_id);

    gtk_main_quit();
}


void *print_message_function( void *ptr )
{
  char *message;

  while(1) {
    g_usleep(1000000);
    gdk_threads_enter ();
    message = (char *) ptr;
    //printf("%s \n", message);
    gdk_threads_leave ();

  }

  return 0;
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

  if( !g_thread_supported() )
  {
    g_thread_init(NULL);
    gdk_threads_init();
    printf("g_thread supported\n");

    GThread          *Thread1, *Thread2;
    char *message1 = "Thread 1";
    char *message2 = "Thread 2";
    GError           *err1 = NULL ;
    GError           *err2 = NULL ;

    if( (Thread1 = g_thread_create((GThreadFunc)print_message_function, 
				   (void *)message1, 
				   TRUE, 
				   &err1)) == NULL)
    {
      printf("Thread create failed: %s!!\n", err1->message );
      g_error_free ( err1 ) ;
    }

    pthread_mutex_init(&muSimStopMutex, NULL);
    pthread_cond_init (&cvSimStopCondition, NULL);
    pthread_mutex_lock(&muSimStopMutex);

    if( (Thread2 = g_thread_create((GThreadFunc)SimulationHasStopped, 
				   (void *)message2, 
				   TRUE, 
				   &err2)) == NULL)
    {
      printf("Thread create failed: %s!!\n", err2->message );
      g_error_free ( err2 ) ;
    }
    pthread_mutex_unlock(&muSimStopMutex);


  }
  else
  {
     printf("g_thread NOT supported\n");
  }


  if (!gtk_init_check (&argc, &argv))
  {
      return -1;
  }


  gdk_threads_enter ();
  gp = new GUI_Processor();

  interface_id = get_interface().add_interface(new GUI_Interface(gp));
  gdk_threads_leave ();

  return(0);
}

void gui_main(void)
{
  gdk_threads_enter ();

  //gp = new GUI_Processor();

  //interface_id = get_interface().add_interface(new GUI_Interface(gp));

  gtk_main ();
  gdk_threads_leave ();
}

#endif //HAVE_GUI
