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


#include <gtkextra/gtkcombobox.h>
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


GtkWidget *SourceBrowserExperiment=0;
GtkWidget *RegisterWindowExperiment=0;
GtkWidget *StatusBarExperiment=0;

typedef struct _note_book_item
{
  gchar        *name;
  GtkSignalFunc func;
} NotebookItem;

GtkItemFactory *item_factory=0;

extern GUI_Processor *gp;
extern guint64 gui_update_rate;

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
  if(gp && gp->cpu)
    gp->cpu->pma->run();
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

//========================================================================
//
class UpdateRateMenuItem {
public:

  char id;
  int menu_index;
  bool bRealTime;
  bool bWithGui;
  bool bAnimate;

  int update_rate;

  UpdateRateMenuItem(char, const char *, int update_rate=0, bool _bRealTime=false, bool _bWithGui=false);

  void Select();
  static GtkWidget *menu;
  static int seq_no;
};


static void
gui_update_cb(GtkWidget *widget, gpointer data)
{

  UpdateRateMenuItem *umi = static_cast<UpdateRateMenuItem *>(data);
  umi->Select();
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
  //{ "/Windows/Program _memory", 0, TOGGLE_WINDOW,WT_opcode_source_window,"<ToggleItem>" },
  //{ "/Windows/_Source",         0, TOGGLE_WINDOW,WT_asm_source_window,"<ToggleItem>" },
  { "/Windows/sep1",            0, (GtkItemFactoryCallback)gtk_ifactory_cb,0,"<Separator>"  },
  //{ "/Windows/_Ram",            0, TOGGLE_WINDOW,WT_register_window,"<ToggleItem>" },
  //{ "/Windows/_EEPROM",         0, TOGGLE_WINDOW,WT_eeprom_window,"<ToggleItem>" },
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


//========================================================================
//========================================================================
//
// UpdateRateMenuItem

GtkWidget * UpdateRateMenuItem::menu = 0;
int UpdateRateMenuItem::seq_no=0;

map<guint, UpdateRateMenuItem*> UpdateRateMenuItemMap;

UpdateRateMenuItem::UpdateRateMenuItem(char _id, 
				       const char *label,
				       int _update_rate,
				       bool _bRealTime,
				       bool _bWithGui)
  : id(_id), bRealTime(_bRealTime), bWithGui(_bWithGui), update_rate(_update_rate)
{

  if(update_rate <0) {
    bAnimate = true;
    update_rate = -update_rate;
  } else
    bAnimate = false;

  if(!menu)
    menu = gtk_menu_new();

  GtkWidget *item = gtk_menu_item_new_with_label(label);

  
  gtk_signal_connect(GTK_OBJECT(item),"activate",
		     (GtkSignalFunc) gui_update_cb,
		     (gpointer)this);
  gtk_widget_show(item);
  gtk_menu_append(GTK_MENU(menu),item);

  menu_index = seq_no;
  seq_no++;

  UpdateRateMenuItemMap[id] = this;
}

void UpdateRateMenuItem::Select()
{
  realtime_mode = bRealTime ? 1 : 0;
  realtime_mode_with_gui = bWithGui ? 1 : 0;

  if(bAnimate) {
    gui_animate_delay = update_rate;
    gi.set_update_rate(1);
  } else
    gi.set_update_rate(update_rate);

  if(gp && gp->cpu)
    gp->cpu->pma->stop();

  config_set_variable("dispatcher", "SimulationMode", id);

  cout << "Update gui refresh: " << hex << update_rate 
       << " ID:" << id
       << endl;
}

//========================================================================
//========================================================================
class TimeWidget;
class TimeFormatter
{
public:

  enum eMenuID {
    eCyclesHex=0,
    eCyclesDec,
    eMicroSeconds,
    eMilliSeconds,
    eSeconds,
    eHHMMSS
  } time_format;
  TimeFormatter(TimeWidget *_tw,GtkWidget *menu, const char*menu_text)
   : tw(_tw)
  {
    AddToMenu(menu,menu_text);
  }

  void ChangeFormat();
  void AddToMenu(GtkWidget *menu, const char*menu_text);
  virtual void Format(char *, int)=0;
  TimeWidget *tw;
};

class TimeWidget : public EntryWidget
{
public:
  TimeWidget();
  void Create(GtkWidget *);
  virtual void Update();
  void NewFormat(TimeFormatter *tf);
  TimeFormatter *current_format;

  GtkWidget *menu;
};


class TimeMicroSeconds : public TimeFormatter
{
public:
  TimeMicroSeconds(TimeWidget *tw, GtkWidget *menu) 
    : TimeFormatter(tw,menu,"MicroSeconds") {}
  void Format(char *buf, int size)
  {
    double time_db = gp->cpu->get_InstPeriod() * cycles.value * 1e6;
    snprintf(buf,size, "%19.2f us",time_db);
  }
};

class TimeMilliSeconds : public TimeFormatter
{
public:
  TimeMilliSeconds(TimeWidget *tw, GtkWidget *menu) 
    : TimeFormatter(tw,menu,"MilliSeconds") {}
  void Format(char *buf, int size)
  {
    double time_db = gp->cpu->get_InstPeriod() * cycles.value * 1e3;
    snprintf(buf,size, "%19.3f ms",time_db);
  }
};

class TimeSeconds : public TimeFormatter
{
public:
  TimeSeconds(TimeWidget *tw, GtkWidget *menu) 
    : TimeFormatter(tw,menu,"Seconds") {}
  void Format(char *buf, int size)
  {
    double time_db = gp->cpu->get_InstPeriod() * cycles.value;
    snprintf(buf,size, "%19.3f Sec",time_db);
  }
};

class TimeHHMMSS : public TimeFormatter
{
public:
  TimeHHMMSS(TimeWidget *tw, GtkWidget *menu) 
    : TimeFormatter(tw,menu,"HH:MM:SS.mmm") {}
  void Format(char *buf, int size)
  {
    double time_db = gp->cpu->get_InstPeriod() * cycles.value;
    double v=time_db;
    int hh=(int)(v/3600),mm,ss,cc;
    v-=hh*3600.0;
    mm=(int)(v/60);
    v-=mm*60.0;
    ss=(int)v;
    cc=(int)(v*100.0+0.5);
    snprintf(buf,size,"    %02d:%02d:%02d.%02d",hh,mm,ss,cc);
  }
};

class TimeCyclesHex : public TimeFormatter
{
public:
  TimeCyclesHex(TimeWidget *tw, GtkWidget *menu) 
    : TimeFormatter(tw,menu,"Cycles (Hex)") {}
  void Format(char *buf, int size)
  {
    snprintf(buf,size,"0x%016Lx",cycles.value);
  }
};

class TimeCyclesDec : public TimeFormatter
{
public:
  TimeCyclesDec(TimeWidget *tw, GtkWidget *menu) 
    : TimeFormatter(tw,menu,"Cycles (Dec)") {}
  void Format(char *buf, int size)
  {
    snprintf(buf,size,"%016Ld",cycles.value);
  }
};

//========================================================================
// called when user has selected a menu item from the time format menu
static void
cbTimeFormatActivated(GtkWidget *widget, gpointer data)
{
  if(!widget || !data)
    return;
    
  TimeFormatter *tf = static_cast<TimeFormatter *>(data);
  tf->ChangeFormat();
}
// button press handler
static gint
cbTimeFormatPopup(GtkWidget *widget, GdkEventButton *event, TimeWidget *tw)
{
  if(!widget || !event || !tw)
    return 0;
  
  if( (event->type == GDK_BUTTON_PRESS) ) {// &&  (event->button == 3) ) {

    gtk_menu_popup(GTK_MENU(tw->menu), 0, 0, 0, 0,
		   3, event->time);
    // It looks like we need it to avoid a selection in the entry.
    // For this we tell the entry to stop reporting this event.
    gtk_signal_emit_stop_by_name(GTK_OBJECT(tw->entry),"button_press_event");
  }
  return FALSE;
}


void TimeFormatter::ChangeFormat()
{
  if(tw)
    tw->NewFormat(this);
}

void TimeFormatter::AddToMenu(GtkWidget *menu, 
			      const char*menu_text)
{
  GtkWidget *item = gtk_menu_item_new_with_label(menu_text);
  gtk_signal_connect(GTK_OBJECT(item),"activate",
		     (GtkSignalFunc) cbTimeFormatActivated,
		     this);
  gtk_widget_show(item);
  gtk_menu_append(GTK_MENU(menu),item);
}

void TimeWidget::Create(GtkWidget *container)
{
  EntryWidget::Create(false);

  gtk_container_add(GTK_CONTAINER(container),entry);

  SetEntryWidth(18);

  menu = gtk_menu_new();
  GtkWidget *item = gtk_tearoff_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), item);
  gtk_widget_show (item);


  // Create an entry for each item in the formatter pop up window.

  new TimeMicroSeconds(this,menu);
  new TimeMilliSeconds(this,menu);
  new TimeSeconds(this,menu);
  new TimeHHMMSS(this,menu);
  NewFormat(new TimeCyclesHex(this,menu));
  new TimeCyclesDec(this,menu);

  // Associate a callback with the user button-click actions
  gtk_signal_connect(GTK_OBJECT(entry),
		     "button_press_event",
		     (GtkSignalFunc) cbTimeFormatPopup,
		     this);
}


void TimeWidget::NewFormat(TimeFormatter *tf)
{
  if(tf && tf != current_format) {
    current_format = tf;
    Update();
  }
}

void TimeWidget::Update()
{
  if(!current_format)
    return;

  char buffer[32];

  current_format->Format(buffer, sizeof(buffer));
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);

}

TimeWidget::TimeWidget()
{
  menu = 0;
  current_format =0;
}

//========================================================================
//========================================================================

class MainWindow 
{
public:
  //CyclesWidget *cyclesW;
  TimeWidget   *timeW;

  void Update();
  void Create();

  MainWindow();
};

MainWindow TheWindow;

MainWindow::MainWindow()
{
  //cyclesW=0;
  timeW=0;
}

void MainWindow::Update()
{
  if(timeW)
   timeW->Update();
}


//========================================================================
//========================================================================

void dispatch_Update()
{
  if(!dispatcher_window)
    return;

  if(gp && gp->cpu) {

    TheWindow.Update();

  }
}

//========================================================================
//========================================================================
// experimental register window replacement

//========================================================================
class RegCell
{
public:
  RegCell(int _address, const char *initialText,int _cell_width=2);
  GtkWidget *getWidget() { return entry; }
  gint ButtonPressEvent(GtkWidget *widget, GdkEventButton *event);
private:
  int address;
  int cell_width;
  GtkWidget *entry;

};


//========================================================================
//
class RegWindow
{
public:
  RegWindow();

  GtkWidget *Build();
  gint ButtonPressEvent(GtkWidget *widget, GdkEventButton *event);

private:

  GtkWidget *parent;  /// Container that holds the register window.
  int nRows;
  int nCols;

};

//------------------------------------------------------------------------
static gint
RegWindow_ButtonPressEvent(GtkWidget *widget, GdkEventButton *event, RegWindow *rw)
{
  printf("regwindow event\n");
  if(widget && event && rw) {
    return rw->ButtonPressEvent(widget,event);
  }
  return FALSE;
}

static gint
RegCell_ButtonPressEvent(GtkWidget *widget, GdkEventButton *event, RegCell *rc)
{

  if(widget && event && rc) {
    return rc->ButtonPressEvent(widget,event);
  }
  return FALSE;
}


//------------------------------------------------------------------------

RegWindow::RegWindow()
{
  nCols = 18;
  nRows = 5;
}

GtkWidget *RegWindow::Build()
{
  GtkWidget *frame = gtk_frame_new("Table");

  GtkAdjustment *hadj = (GtkAdjustment*)gtk_adjustment_new(0.0,0.0,0.0,0.0,0.0,0.0);
  GtkAdjustment *vadj = (GtkAdjustment*)gtk_adjustment_new(0.0,0.0,0.0,0.0,0.0,0.0);
  GtkWidget *scrolled_window = gtk_scrolled_window_new(hadj,vadj);
  gtk_widget_show(scrolled_window);
  gtk_container_add(GTK_CONTAINER(frame),scrolled_window);
  GtkWidget *table = gtk_table_new (64,18,TRUE);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),table);

  gtk_table_set_row_spacings (GTK_TABLE(table),0);

  char buf[32];
  int row,col;
  GtkWidget *label;
  for(col=0; col<nCols;col++) {
    snprintf(buf,sizeof(buf),"%02X",col);
    label = gtk_label_new(buf);
    gtk_table_attach_defaults(GTK_TABLE(table),label, col+1, col+2, 0, 1);
  }
  label = gtk_label_new("ASCII");
  gtk_table_attach_defaults(GTK_TABLE(table),label, nCols+1, nCols+5, 0, 1);

  for(row=0; row<nRows;row++) {
    snprintf(buf,sizeof(buf),"%02X",row);
    label = gtk_label_new(buf);
    gtk_table_attach_defaults(GTK_TABLE(table),label, 0, 1, row+1, row+2);
  }

  for(row=0; row<nRows; row++) {
    for(col=0; col<nCols;col++) {

      snprintf(buf,sizeof(buf),"%02X%02X",row,col);

      RegCell *rc = new RegCell(row * nCols + col, buf);

      gtk_table_attach_defaults(GTK_TABLE(table),rc->getWidget(), col+1, col+2, row+1, row+2);

    }
  
    snprintf(buf,sizeof(buf),"%02X23456789ABCDEF",row);

    RegCell *rc = new RegCell(row * nCols + nCols+1, buf, nCols);

    gtk_table_attach_defaults(GTK_TABLE(table),rc->getWidget(), nCols+1, nCols+5, row+1, row+2);
  }
  gtk_signal_connect(GTK_OBJECT(table),
		     "button_press_event",
		     (GtkSignalFunc) RegWindow_ButtonPressEvent, 
		     this);

  gtk_table_resize(GTK_TABLE(table), nRows +1 , nCols+1);
  return frame;

}

gint RegWindow::ButtonPressEvent(GtkWidget *widget, GdkEventButton *event)
{
  if(!event || !widget)
    return FALSE;

  printf("ButtonPressEvent for RegWindow\n");
  if( (event->type == GDK_BUTTON_PRESS) &&  (event->button == 3) ) {
    printf("button 3 pressed\n");
    return TRUE;
  }
  return FALSE;
}

//------------------------------------------------------------------------
// RegCell
RegCell::RegCell(int _address, const char *initialText, int _cell_width)
  : address(_address), cell_width(_cell_width)
{
  entry = gtk_entry_new ();

  PangoContext *pc = gtk_widget_get_pango_context(entry);
  PangoFontDescription *pfd = pango_context_get_font_description(pc);
  pango_font_description_set_family_static(pfd, "Monospace");
  gtk_widget_modify_font(entry, pfd);

  gtk_entry_set_width_chars(GTK_ENTRY (entry), cell_width);
  gtk_entry_set_text (GTK_ENTRY (entry), initialText);

  gtk_signal_connect(GTK_OBJECT(entry),
		     "button_press_event",
		     (GtkSignalFunc) RegCell_ButtonPressEvent, 
		     this);
}

static void debugFont(PangoContext *pc)
{
  if(!pc)
    return;

  PangoFontDescription *pfd = pango_context_get_font_description(pc);
  G_CONST_RETURN char *fontDesc = pango_font_description_get_family(pfd);
  gint fsize = pango_font_description_get_size(pfd);
  printf("Font: %s size %d\n",fontDesc,fsize);

  char *fullDesc = pango_font_description_to_string(pfd);
  if(fullDesc) {
    printf("full description: %s\n",fullDesc);
    g_free(fullDesc);
  }

  int i,n_families=0;
  PangoFontFamily **families;

  pango_context_list_families(pc, &families, &n_families);

  printf("Number of families for the context:%d\n",n_families);
  for(i=0; i<n_families; i++) {
    fullDesc = (char *)pango_font_family_get_name(families[i]);
    if(fullDesc)
      printf(" %s\n",fullDesc);
  }
  g_free(families);

}
gint RegCell::ButtonPressEvent(GtkWidget *widget, GdkEventButton *event)
{
  if(!event || !widget)
    return FALSE;

  printf("ButtonPressEvent for RegCell, address = 0x%x\n",address);
  if( (event->type == GDK_BUTTON_PRESS) &&  (event->button == 3) ) {
    printf("button 3 pressed\n");

    //PangoLayout *pl = gtk_entry_get_layout(GTK_ENTRY(entry));
    //PangoContext *pc = pango_layout_get_context(pl);
    debugFont(gtk_widget_get_pango_context(entry));
    return TRUE;
  }
  return FALSE;
}

//========================================================================
//========================================================================

void MainWindow::Create (void)
{
  if (dispatcher_window)
    return;

  GtkWidget *box1;

  GtkWidget *buttonbox;
  GtkWidget *separator;
  GtkWidget *button;
  GtkWidget *frame;
  GtkAccelGroup *accel_group;
      
  int x,y,width,height;

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
  gtk_box_pack_start (GTK_BOX (box1), buttonbox, FALSE, FALSE, 0);

      
      
  // Buttons
  button = gtk_button_new_with_label ("step");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc) stepbutton_cb, 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), button, FALSE, FALSE, 0);
      
  button = gtk_button_new_with_label ("over");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc) overbutton_cb, 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), button, FALSE, FALSE, 0);

  button = gtk_button_new_with_label ("finish");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc) finishbutton_cb, 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), button, FALSE, FALSE, 0);

  button = gtk_button_new_with_label ("run");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc) runbutton_cb, 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), button, FALSE, FALSE, 0);

  button = gtk_button_new_with_label ("stop");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc) stopbutton_cb, 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), button, FALSE, FALSE, 0);

  button = gtk_button_new_with_label ("reset");
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc) resetbutton_cb, 0);
  gtk_box_pack_start (GTK_BOX (buttonbox), button, FALSE, FALSE, 0);

  //
  // Simulation Mode Frame
  //

  frame = gtk_frame_new("Simulation mode");
  if(!config_get_variable("dispatcher", "SimulationMode", &SimulationMode))
    {
      SimulationMode='4';
    }

  update_rate_menu = gtk_option_menu_new();
  gtk_widget_show(update_rate_menu);
  gtk_container_add(GTK_CONTAINER(frame),update_rate_menu);

  new UpdateRateMenuItem('5',"Without gui (fastest simulation)",0);
  new UpdateRateMenuItem('4',"2000000 cycles/gui update",2000000);
  new UpdateRateMenuItem('3',"100000 cycles/gui update",100000);
  new UpdateRateMenuItem('2',"1000 cycles/gui update",1000);
  new UpdateRateMenuItem('1',"Update gui every cycle",1);
  new UpdateRateMenuItem('b',"100ms animate",-100);
  new UpdateRateMenuItem('c',"300ms animate",-300);
  new UpdateRateMenuItem('d',"700ms animate",-700);
  new UpdateRateMenuItem('r',"Realtime without gui",0,true);
  new UpdateRateMenuItem('R',"Realtime with gui",0,true,true);

  gtk_option_menu_set_menu(GTK_OPTION_MENU(update_rate_menu), UpdateRateMenuItem::menu);

  UpdateRateMenuItem *umi = UpdateRateMenuItemMap[SimulationMode];

  if(!umi)
    cout << "error selecting update rate menu\n";
  umi->Select();
  gtk_option_menu_set_history(GTK_OPTION_MENU(update_rate_menu), umi->seq_no);

  gtk_box_pack_start (GTK_BOX (buttonbox), frame, FALSE, FALSE, 5);

  //
  // Simulation Time Frame
  //

  frame = gtk_frame_new("Simulation Time");
  gtk_box_pack_start (GTK_BOX (buttonbox), frame, FALSE, FALSE, 5);

  timeW = new TimeWidget();
  timeW->Create(frame);

  //
  // Source Browser and Register Windows
  //

  separator = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

  GtkWidget *vpane = gtk_vpaned_new ();
  gtk_box_pack_start (GTK_BOX (box1), vpane, TRUE, TRUE, 0);

  SourceBrowserExperiment = gtk_frame_new("SourceBrowser");
  RegWindow *rw = new RegWindow();
  frame = rw->Build();

  gtk_frame_set_shadow_type (GTK_FRAME (SourceBrowserExperiment), GTK_SHADOW_IN);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);

  gtk_paned_pack1 (GTK_PANED (vpane), SourceBrowserExperiment, TRUE, TRUE);
  gtk_widget_set_size_request (SourceBrowserExperiment, 50, -1);

  gtk_paned_pack2 (GTK_PANED (vpane), frame, TRUE, TRUE);
  gtk_widget_set_size_request (frame, 50, -1);



  // Status Bar

  separator = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);

  frame = gtk_frame_new("Status Bar");
  gtk_box_pack_start (GTK_BOX (box1), frame, FALSE, FALSE, 0);

  StatusBarExperiment = gtk_hbox_new (FALSE, 0);
  gtk_container_add(GTK_CONTAINER(frame),StatusBarExperiment);

  gtk_widget_show_all (dispatcher_window);
      
}

void create_dispatcher (void)
{
  TheWindow.Create();
}


#endif // HAVE_GUI
