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

#include "../config.h"
#ifdef HAVE_GUI

#ifdef DOING_GNOME
#include <gnome.h>
#endif

#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <string.h>

#include <gtkextra/gtkbordercombo.h>
#include <gtkextra/gtkcolorcombo.h>
#include <gtkextra/gtksheet.h>


#include "gui.h"
#include "gui_callbacks.h"
#include "gui_breadboard.h"
#include "gui_processor.h"
#include "gui_profile.h"
#include "gui_register.h"
#include "gui_regwin.h"
#include "gui_scope.h"
#include "gui_src.h"
#include "gui_stack.h"
#include "gui_stopwatch.h"
#include "gui_symbols.h"
#include "gui_trace.h"
#include "gui_watch.h"

#include "../cli/input.h"  // for gpsim_open()
#include "../src/gpsim_interface.h"

typedef struct _note_book_item
{
  gchar        *name;
  GtkSignalFunc func;
} NotebookItem;

GtkItemFactory *item_factory=0;

extern GUI_Processor *gp;

static void 
do_quit_app(GtkWidget *widget) 
{
	exit_gpsim();
}


static void
show_message (char *title, char *message)
{
  GtkWidget *window;
  GtkWidget *label;
  GtkWidget *button;

  window = gtk_dialog_new ();

//  gtk_signal_connect_object (GTK_OBJECT (window), "destroy",
//			     GTK_SIGNAL_FUNC (gtk_widget_destroyed), GTK_OBJECT(window));

  gtk_window_set_title (GTK_WINDOW (window), title);
  gtk_container_set_border_width (GTK_CONTAINER (window), 0);


  button = gtk_button_new_with_label ("close");
  gtk_container_set_border_width (GTK_CONTAINER (button), 10);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     GTK_SIGNAL_FUNC(gtk_widget_destroy),
			     GTK_OBJECT (window));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);


  label = gtk_label_new (message);
  gtk_misc_set_padding (GTK_MISC (label), 10, 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), label, TRUE, TRUE, 0);

  gtk_widget_show (label);
  gtk_widget_show (window);

  gtk_grab_add (window);

}

static void 
about_cb (gpointer             callback_data,
	  guint                callback_action,
	  GtkWidget           *widget)
{

  show_message(  "The GNUPIC Simulator - " VERSION, "A simulator for Microchip PIC microcontrollers.\n"
		 "by T. Scott Dattalo - mailto:scott@dattalo.com\n"
		 "   Ralf Forsberg - mailto:rfg@home.se\n"
		 "   Borut Razem - mailto:borut.razem@siol.net\n\n"
		 "gpsim homepage: http://www.dattalo.com/gnupic/gpsim.html\n"
		 "gpsimWin32: http://gpsim.sourceforge.net/gpsimWin32/gpsimWin32.html\n");

}


//--------------------------------------------------------------
// new_processor_dialog
//
// This will display the 'new processor' dialog window.
//
//

//static GtkWidget *new_processor_name_entry;

/*
static void
new_processor_dialog (void)
{
  static GtkWidget *window = 0;

  GtkWidget *box1;
  GtkWidget *box2;
  GtkWidget *cb;
  GtkWidget *button;
  GtkWidget *separator;
  GList *cbitems = 0;

  if (!window)
    {
      //      for(i=3; i<number_of_available_processors; i++)
      //	cbitems = g_list_append(cbitems, available_processors[i].names[2]);


      window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

      gtk_signal_connect (GTK_OBJECT (window), "destroy",
			  GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			  &window);

      gtk_window_set_title (GTK_WINDOW (window), "New Processor (not supported)");
      gtk_container_set_border_width (GTK_CONTAINER (window), 0);


      box1 = gtk_vbox_new (FALSE, 0);
      gtk_container_add (GTK_CONTAINER (window), box1);
      gtk_widget_show (box1);


      box2 = gtk_vbox_new (FALSE, 10);
      gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
      gtk_box_pack_start (GTK_BOX (box1), box2, TRUE, TRUE, 0);
      gtk_widget_show (box2);

      cb = gtk_combo_new ();
      gtk_combo_set_popdown_strings (GTK_COMBO (cb), cbitems);
      cbitems = g_list_first(cbitems);
      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO(cb)->entry), (gchar *)cbitems->data);
      gtk_editable_select_region (GTK_EDITABLE (GTK_COMBO(cb)->entry),
				  0, -1);
      gtk_box_pack_start (GTK_BOX (box2), cb, TRUE, TRUE, 0);
      gtk_widget_show (cb);

      new_processor_name_entry = gtk_entry_new ();
      gtk_entry_set_text (GTK_ENTRY (new_processor_name_entry), "no_name");
      gtk_editable_select_region (GTK_EDITABLE (new_processor_name_entry), 0, 5);
      gtk_box_pack_start (GTK_BOX (box2), new_processor_name_entry, TRUE, TRUE, 0);
      gtk_widget_show (new_processor_name_entry);


      separator = gtk_hseparator_new ();
      gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);
      gtk_widget_show (separator);


      box2 = gtk_hbox_new (FALSE, 10);
      gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
      gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, TRUE, 0);
      gtk_widget_show (box2);


      button = gtk_button_new_with_label ("NOT");
      gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
				 GTK_SIGNAL_FUNC(gtk_widget_destroy),
				 GTK_OBJECT (window));
      gtk_box_pack_start (GTK_BOX (box2), button, TRUE, TRUE, 0);
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_widget_grab_default (button);
      gtk_widget_show (button);

      button = gtk_button_new_with_label ("YET");
      gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
				 GTK_SIGNAL_FUNC(gtk_widget_destroy),
				 GTK_OBJECT (window));
      gtk_box_pack_start (GTK_BOX (box2), button, TRUE, TRUE, 0);
      gtk_widget_show (button);
    }

  if (!GTK_WIDGET_VISIBLE (window))
    gtk_widget_show (window);
  else
    gtk_widget_destroy (window);
}
*/

void
file_selection_hide_fileops (GtkWidget *widget,
			     GtkFileSelection *fs)
{
  gtk_file_selection_hide_fileop_buttons (fs);
}

extern int gui_message(char *message);

void
file_selection_ok (GtkWidget        *w,
		   GtkFileSelection *fs)
{

  const char *file;
  char msg[200];

  if(gp)
  {
    file=gtk_file_selection_get_filename (fs);
    if(!gpsim_open(gp->cpu, file))
    {
      sprintf(msg, "Open failedCould not open \"%s\"", (char *)file);
      gui_message(msg);
    }
  }
  gtk_widget_hide (GTK_WIDGET (fs));
}

extern int gui_question(char *question, char *a, char *b);

static GtkItemFactoryCallback 
fileopen_dialog(gpointer             callback_data,
	      guint                callback_action,
	      GtkWidget           *widget)
{
  static GtkWidget *window = 0;
  GtkWidget *button;

  if (!window)
  {

    window = gtk_file_selection_new ("file selection dialog");

    gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (window));

    gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);

    //      gtk_signal_connect_object (GTK_OBJECT (window), "destroy",
    //				 GTK_SIGNAL_FUNC(gtk_widget_destroyed),
    //				 GTK_OBJECT(window));

    gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (window)->ok_button),
			"clicked", GTK_SIGNAL_FUNC(file_selection_ok),
			window);
    gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (window)->cancel_button),
			       "clicked", GTK_SIGNAL_FUNC(gtk_widget_hide),
			       GTK_OBJECT (window));
      
    button = gtk_button_new_with_label ("Hide Fileops");
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			(GtkSignalFunc) file_selection_hide_fileops, 
			(gpointer) window);
    gtk_box_pack_start (GTK_BOX (GTK_FILE_SELECTION (window)->action_area), 
			button, FALSE, FALSE, 0);
    gtk_widget_show (button);

    button = gtk_button_new_with_label ("Show Fileops");
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			(GtkSignalFunc) gtk_file_selection_show_fileop_buttons,
			(gpointer) window);
    gtk_box_pack_start (GTK_BOX (GTK_FILE_SELECTION (window)->action_area), 
			button, FALSE, FALSE, 0);
    gtk_widget_show (button);
  }
    gtk_widget_show (window);
    return 0;
}




// Menuhandler for Windows menu buttons
static GtkItemFactoryCallback 
toggle_window (gpointer             callback_data,
	      guint                callback_action,
	      GtkWidget           *widget)
{
  GtkWidget *menu_item = 0;

  //    g_message ("\"%s\" is not supported yet.", gtk_item_factory_path_from_widget (widget));

  menu_item = gtk_item_factory_get_item (item_factory,
					 gtk_item_factory_path_from_widget (widget));
  if(gp && menu_item) {
    
    int view_state =  GTK_CHECK_MENU_ITEM(menu_item)->active ? VIEW_SHOW : VIEW_HIDE;
			
    switch(callback_action) {
    case WT_opcode_source_window:
      gp->program_memory->ChangeView(view_state);
      break;
    case WT_asm_source_window:
      gp->source_browser->ChangeView(view_state);
      break;
    case WT_register_window:
      gp->regwin_ram->ChangeView(view_state);
      break;
    case WT_eeprom_window:
      gp->regwin_eeprom->ChangeView(view_state);
      break;
    case WT_watch_window:
      gp->watch_window->ChangeView(view_state);
      break;
    case WT_symbol_window:
      gp->symbol_window->ChangeView(view_state);
      break;
    case WT_breadboard_window:
      gp->breadboard_window->ChangeView(view_state);
      break;
    case WT_stack_window:
      gp->stack_window->ChangeView(view_state);
      break;
    case WT_trace_window:
      gp->trace_window->ChangeView(view_state);
      break;
    case WT_profile_window:
      gp->profile_window->ChangeView(view_state);
      break;
    case WT_stopwatch_window:
      gp->stopwatch_window->ChangeView(view_state);
      break;
    case WT_scope_window:
      //gp->scope_window->ChangeView(view_state);
      cout << " The Scope is disabled right now\n";
      break;
    default:
      puts("unknown menu action");
    }

  }
  return 0;
}

static void 
runbutton_cb(GtkWidget *widget)
{
  get_interface().start_simulation();
}

static void 
stopbutton_cb(GtkWidget *widget)
{
  if(gp && gp->cpu)
    gp->cpu->pma->stop();
}
    
static void 
stepbutton_cb(GtkWidget *widget)
{
  if(gp && gp->cpu) 
    gp->cpu->pma->step(1);
}
    
static void 
overbutton_cb(GtkWidget *widget)
{
  if(gp && gp->cpu) 
    gp->cpu->pma->step_over();

}
    
static void 
finishbutton_cb(GtkWidget *widget)
{
  if(gp && gp->cpu) 
    gp->cpu->pma->finish();
}

static void 
resetbutton_cb(GtkWidget *widget)
{
  if(gp && gp->cpu)
    gp->cpu->reset(POR_RESET);
}

int gui_animate_delay; // in milliseconds
extern int realtime_mode;
extern int realtime_mode_with_gui;

static void set_simulation_mode(char m)
{
    guint64 value=1;

    realtime_mode=0;
    realtime_mode_with_gui=0;
    gui_animate_delay=0;

    switch(m)
    {
    case 'r':
        value=0;
        gui_animate_delay=0;
	realtime_mode=1;
	break;
    case 'R':
        value=0;
        gui_animate_delay=0;
	realtime_mode=1;
        realtime_mode_with_gui=1;
        break;
    case 'b':
	value=1;
        gui_animate_delay=100;
        break;
    case 'c':
	value=1;
        gui_animate_delay=300;
        break;
    case 'd':
	value=1;
        gui_animate_delay=600;
        break;
    case '1':
        value=1;
        break;
    case '2':
        value=1000;
        break;
    case '3':
        value=100000;
        break;
    case '4':
        value=2000000;
        break;
    case '5':
        value=0;
        break;
    }
    gi.set_update_rate(value);

    if(gp && gp->cpu)
      gp->cpu->pma->stop();

    config_set_variable("dispatcher", "SimulationMode", m);
}

static void
gui_update_cb(GtkWidget *widget, gpointer data)
{
    char *s = (char*)data;

    set_simulation_mode(*s);

}


static int dispatcher_delete_event(GtkWidget *widget,
				   GdkEvent  *event,
				   gpointer data)
{
    do_quit_app(0);

    return 0;
}

static GtkItemFactoryCallback
gtk_ifactory_cb (gpointer             callback_data,
		 guint                callback_action,
		 GtkWidget           *widget)
{
  g_message ("\"%s\" is not supported yet.", gtk_item_factory_path_from_widget (widget));
    return 0;
}

#define TOGGLE_WINDOW (GtkItemFactoryCallback)toggle_window

static GtkItemFactoryEntry menu_items[] =
{
  { "/_File",            0,         0,                     0, "<Branch>" },
  { "/File/tearoff1",    0,         (GtkItemFactoryCallback)gtk_ifactory_cb,       0, "<Tearoff>" },
  //{ "/File/_New",        "<control>N", (GtkItemFactoryCallback)gtk_ifactory_cb,       0 },
  { "/File/_Open",       "<control>O", (GtkItemFactoryCallback)fileopen_dialog,       0 },
  //{ "/File/_Save",       "<control>S", (GtkItemFactoryCallback)gtk_ifactory_cb,       0 },
  //{ "/File/Save _As...", 0,         (GtkItemFactoryCallback)gtk_ifactory_cb,       0 },
  { "/File/sep1",        0,         (GtkItemFactoryCallback)gtk_ifactory_cb,       0, "<Separator>" },
  { "/File/_Quit",       "<control>Q", (GtkItemFactoryCallback)do_quit_app,         0 },

  //  { "/_Processor",     	 0,       0,               0, "<Branch>" },
  //  { "/_Processor/New",   0,       (GtkItemFactoryCallback)new_processor_dialog,       0 },
  //  { "/_Processor/Delete",0,       (GtkItemFactoryCallback)gtk_ifactory_cb,       0 },
  //  { "/_Processor/Switch",0,       (GtkItemFactoryCallback)gtk_ifactory_cb,       0 },


  //  { "/_Break",     	 0, 0,               0, "<Branch>" },
  //  { "/_Break/Set",       0, (GtkItemFactoryCallback)gtk_ifactory_cb,       0 },
  //  { "/_Break/Clear",     0, (GtkItemFactoryCallback)gtk_ifactory_cb,       0 },

  { "/_Windows",     0, 0,       0, "<Branch>" },
  { "/Windows/Program _memory", 0, TOGGLE_WINDOW,WT_opcode_source_window,"<ToggleItem>" },
  { "/Windows/_Source",         0, TOGGLE_WINDOW,WT_asm_source_window,"<ToggleItem>" },
  { "/Windows/sep1",            0, (GtkItemFactoryCallback)gtk_ifactory_cb,0,"<Separator>"  },
  { "/Windows/_Ram",            0, TOGGLE_WINDOW,WT_register_window,"<ToggleItem>" },
  { "/Windows/_EEPROM",         0, TOGGLE_WINDOW,WT_eeprom_window,"<ToggleItem>" },
  { "/Windows/_Watch",          0, TOGGLE_WINDOW,WT_watch_window,"<ToggleItem>" },
  { "/Windows/Sta_ck",          0, TOGGLE_WINDOW,WT_stack_window,"<ToggleItem>" },
  { "/Windows/sep2",            0, (GtkItemFactoryCallback)gtk_ifactory_cb,0,"<Separator>"  },
  { "/Windows/Symbo_ls",        0, TOGGLE_WINDOW,WT_symbol_window,"<ToggleItem>" },
  { "/Windows/_Breadboard",     0, TOGGLE_WINDOW,WT_breadboard_window,"<ToggleItem>" },
  { "/Windows/sep3",            0, (GtkItemFactoryCallback)gtk_ifactory_cb,0,"<Separator>"  },
  { "/Windows/_Trace",          0, TOGGLE_WINDOW,WT_trace_window,"<ToggleItem>" },
  { "/Windows/Pro_file",        0, TOGGLE_WINDOW,WT_profile_window,"<ToggleItem>" },
  { "/Windows/St_opwatch",      0, TOGGLE_WINDOW,WT_stopwatch_window,"<ToggleItem>" },
  { "/Windows/Sco_pe",          0, TOGGLE_WINDOW,WT_scope_window,"<ToggleItem>" },

//  { "/_Preferences",          0, 0,               0, "<Branch>" },
//  { "/_Preferences/Windows",  0, (GtkItemFactoryCallback)create_notebook,       0 },


  { "/_Help",            0,         0,                     0, "<LastBranch>" },
  { "/Help/_About",      0,         (GtkItemFactoryCallback)about_cb,       0 },

};

static int nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);


GtkWidget *dispatcher_window = 0;

void create_dispatcher (void)
{
  if (!dispatcher_window)
    {
      GtkWidget *box1;

      GtkWidget *buttonbox;
      GtkWidget *separator;
      GtkWidget *button;
      GtkWidget *frame;
      GtkAccelGroup *accel_group;
      
      int x,y,width,height;

      GtkWidget *menu;
      GtkWidget *item;

      GtkWidget *update_rate_menu;

      int SimulationMode;
      
      dispatcher_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

      if(!config_get_variable("dispatcher", "x", &x))
	  x=10;
      if(!config_get_variable("dispatcher", "y", &y))
	  y=10;
      if(!config_get_variable("dispatcher", "width", &width))
	  width=1;
      if(!config_get_variable("dispatcher", "height", &height))
	  height=1;
      gtk_window_set_default_size(GTK_WINDOW(dispatcher_window), width,height);
      gtk_widget_set_uposition(GTK_WIDGET(dispatcher_window),x,y);
      
      
      gtk_signal_connect (GTK_OBJECT (dispatcher_window), "delete-event",
			  GTK_SIGNAL_FUNC (dispatcher_delete_event),
			  0);
      
#if GTK_MAJOR_VERSION >= 2
      accel_group = gtk_accel_group_new();
#else
      accel_group = gtk_accel_group_get_default ();
#endif
      item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group);
      gtk_object_set_data_full (GTK_OBJECT (dispatcher_window),
				"<main>",
				item_factory,
				(GtkDestroyNotify) gtk_object_unref);
//      gtk_accel_group_attach (accel_group, GTK_OBJECT (dispatcher_window));
      gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, 0);
      gtk_window_set_title (GTK_WINDOW (dispatcher_window), 
			    VERSION);
      gtk_container_set_border_width (GTK_CONTAINER (dispatcher_window), 0);
      
      box1 = gtk_vbox_new (FALSE, 0);
      gtk_container_add (GTK_CONTAINER (dispatcher_window), box1);
      
      gtk_box_pack_start (GTK_BOX (box1),
			  gtk_item_factory_get_widget (item_factory, "<main>"),
			  FALSE, FALSE, 0);


      
      buttonbox = gtk_hbox_new(FALSE,0);
      gtk_container_set_border_width (GTK_CONTAINER (buttonbox), 1);
      gtk_box_pack_start (GTK_BOX (box1), buttonbox, TRUE, TRUE, 0);

      
      
      // Buttons
      button = gtk_button_new_with_label ("step");
      gtk_signal_connect(GTK_OBJECT(button), "clicked",
			 (GtkSignalFunc) stepbutton_cb, 0);
      gtk_box_pack_start (GTK_BOX (buttonbox), button, TRUE, TRUE, 0);
      
      button = gtk_button_new_with_label ("over");
      gtk_signal_connect(GTK_OBJECT(button), "clicked",
			 (GtkSignalFunc) overbutton_cb, 0);
      gtk_box_pack_start (GTK_BOX (buttonbox), button, TRUE, TRUE, 0);

      button = gtk_button_new_with_label ("finish");
      gtk_signal_connect(GTK_OBJECT(button), "clicked",
			 (GtkSignalFunc) finishbutton_cb, 0);
      gtk_box_pack_start (GTK_BOX (buttonbox), button, TRUE, TRUE, 0);

      button = gtk_button_new_with_label ("run");
      gtk_signal_connect(GTK_OBJECT(button), "clicked",
			 (GtkSignalFunc) runbutton_cb, 0);
      gtk_box_pack_start (GTK_BOX (buttonbox), button, TRUE, TRUE, 0);

      button = gtk_button_new_with_label ("stop");
      gtk_signal_connect(GTK_OBJECT(button), "clicked",
			 (GtkSignalFunc) stopbutton_cb, 0);
      gtk_box_pack_start (GTK_BOX (buttonbox), button, TRUE, TRUE, 0);

      button = gtk_button_new_with_label ("reset");
      gtk_signal_connect(GTK_OBJECT(button), "clicked",
			 (GtkSignalFunc) resetbutton_cb, 0);
      gtk_box_pack_start (GTK_BOX (buttonbox), button, TRUE, TRUE, 0);


      frame = gtk_frame_new("Simulation mode");
      if(!config_get_variable("dispatcher", "SimulationMode", &SimulationMode))
      {
	  SimulationMode='4';
      }
      set_simulation_mode(SimulationMode);
	update_rate_menu = gtk_option_menu_new();
        gtk_widget_show(update_rate_menu);
	gtk_container_add(GTK_CONTAINER(frame),update_rate_menu);

	menu=gtk_menu_new();

	item=gtk_menu_item_new_with_label("Without gui (fastest simulation)");
	gtk_signal_connect(GTK_OBJECT(item),"activate",
			   (GtkSignalFunc) gui_update_cb,
			   (gpointer)"5");
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(menu),item);

	item=gtk_menu_item_new_with_label("2000000 cycles/gui update");
	gtk_signal_connect(GTK_OBJECT(item),"activate",
			   (GtkSignalFunc) gui_update_cb,
			   (gpointer)"4");
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(menu),item);

	item=gtk_menu_item_new_with_label("100000 cycles/gui update");
	gtk_signal_connect(GTK_OBJECT(item),"activate",
			   (GtkSignalFunc) gui_update_cb,
			   (gpointer)"3");
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(menu),item);

	item=gtk_menu_item_new_with_label("1000 cycles/gui update");
	gtk_signal_connect(GTK_OBJECT(item),"activate",
			   (GtkSignalFunc) gui_update_cb,
			   (gpointer)"2");
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(menu),item);

	item=gtk_menu_item_new_with_label("Update gui every cycle");
	gtk_signal_connect(GTK_OBJECT(item),"activate",
			   (GtkSignalFunc) gui_update_cb,
			   (gpointer)"1");
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(menu),item);

	item=gtk_menu_item_new_with_label("100ms animate");
	gtk_signal_connect(GTK_OBJECT(item),"activate",
			   (GtkSignalFunc) gui_update_cb,
			   (gpointer)"b");
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(menu),item);

	item=gtk_menu_item_new_with_label("300ms animate");
	gtk_signal_connect(GTK_OBJECT(item),"activate",
			   (GtkSignalFunc) gui_update_cb,
			   (gpointer)"c");
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(menu),item);

	item=gtk_menu_item_new_with_label("700ms animate");
	gtk_signal_connect(GTK_OBJECT(item),"activate",
			   (GtkSignalFunc) gui_update_cb,
			   (gpointer)"d");
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(menu),item);

	item=gtk_menu_item_new_with_label("Realtime without gui");
	gtk_signal_connect(GTK_OBJECT(item),"activate",
			   (GtkSignalFunc) gui_update_cb,
			   (gpointer)"r");
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(menu),item);

	item=gtk_menu_item_new_with_label("Realtime with gui");
	gtk_signal_connect(GTK_OBJECT(item),"activate",
			   (GtkSignalFunc) gui_update_cb,
			   (gpointer)"R");
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(menu),item);

	gtk_option_menu_set_menu(GTK_OPTION_MENU(update_rate_menu), menu);

	int gui_update_index=0;
	switch(SimulationMode) // ugh
	{
	case '5':
            gui_update_index=0;
            break;
	case '4':
            gui_update_index=1;
            break;
	case '3':
            gui_update_index=2;
            break;
	case '2':
            gui_update_index=3;
            break;
	case '1':
            gui_update_index=4;
            break;
	case 'b':
            gui_update_index=5;
            break;
	case 'c':
            gui_update_index=6;
            break;
	case 'd':
            gui_update_index=7;
	    break;
	case 'r':
            gui_update_index=8;
	    break;
	case 'R':
            gui_update_index=9;
            break;
	}

	gtk_option_menu_set_history(GTK_OPTION_MENU(update_rate_menu), gui_update_index);


      gtk_box_pack_start (GTK_BOX (buttonbox), frame, TRUE, TRUE, 5);


      
      separator = gtk_hseparator_new ();
      gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 5);
      button = gtk_button_new_with_label ("Quit gpsim");
      gtk_signal_connect(GTK_OBJECT(button), "clicked",
			 (GtkSignalFunc) do_quit_app, 0);

      gtk_box_pack_start (GTK_BOX (box1), button, FALSE, TRUE, 5);
      gtk_widget_show_all (dispatcher_window);
      
    }
  else
    gtk_widget_destroy (dispatcher_window);
}
/*
static GtkWidget *
build_option_menu (OptionMenuItem items[],
		   gint           num_items,
		   gint           history,
		   gpointer       data)
{
  GtkWidget *omenu;
  GtkWidget *menu;
  GtkWidget *menu_item;
  GSList *group;
  gint i;

  omenu = gtk_option_menu_new ();
      
  menu = gtk_menu_new ();
  group = 0;
  
  for (i = 0; i < num_items; i++)
    {
      menu_item = gtk_radio_menu_item_new_with_label (group, items[i].name);
      gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
			  (GtkSignalFunc) items[i].func, data);
      group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menu_item));
      gtk_menu_append (GTK_MENU (menu), menu_item);
      if (i == history)
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), TRUE);
      gtk_widget_show (menu_item);
    }

  gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (omenu), history);
  
  return omenu;
}
*/


#endif // HAVE_GUI
