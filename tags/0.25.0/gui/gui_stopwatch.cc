/*
   Copyright (C) 2000,2001
    Ralf Forsberg

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
#include <errno.h>

#include "../config.h"
#ifdef HAVE_GUI

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <string.h>

#include <assert.h>

#include "../src/interface.h"

#include "gui.h"
#include "gui_stopwatch.h"

static int delete_event(GtkWidget *widget,
			GdkEvent  *event,
                        StopWatch_Window *sww)
{
    sww->ChangeView(VIEW_HIDE);
    return TRUE;
}


void StopWatch_Window::Update(void)
{
  long long _cyclecounter;
  static long long cyclecounter_last=0;
  double timevalue;

  char frequencystring[100];
  char cyclestring[100];
  char timestring[100];
  char offsetstring[100];
  char rolloverstring[100];

  if(!gp || !gp->cpu)
    return;
  if(!enabled)
    return;

  if(!bIsBuilt)
    Build();

  if(rollover<=0)
    rollover=1;

  if(offset>rollover)
    offset%=rollover;

  double frequency = gp->cpu->get_frequency();
  unsigned int cycle_per_inst = gp->cpu->get_ClockCycles_per_Instruction();

  _cyclecounter=cyclecounter;

  ////////////////////////

  if(count_dir<0)
    _cyclecounter -= get_cycles().get() - cyclecounter_last;
  else
    _cyclecounter += get_cycles().get() - cyclecounter_last;

  cyclecounter_last = get_cycles().get();


  // %%% FIXME %%% - This surely must be wrong, given that we're working with
  // the local copy ('_cyclecounter')
  while(cyclecounter<offset)
    cyclecounter+=rollover;

  cyclecounter=_cyclecounter;   // See! We overwrite what we just did

  _cyclecounter=(_cyclecounter-offset)%rollover;
  ////////////////////////

  timevalue = (_cyclecounter*1000000*cycle_per_inst)/frequency;

  sprintf(frequencystring, "%.0f Hz", frequency);
  sprintf(cyclestring, "%Ld", _cyclecounter);
  if(timevalue<1000)
    sprintf(timestring, "%.2f us", timevalue/1.0);
  else if(timevalue<1000000)
    sprintf(timestring, "%.3f ms", timevalue/1000.0);
  else if(timevalue<1000000000)
    sprintf(timestring, "%.3f s", timevalue/1000000.0);
  else
    {
      double v= timevalue/1000000.0;
      int hh=(int)(v/3600),mm,ss;
      v-=hh*3600.0;
      mm=(int)(v/60);
      v-=mm*60.0;
      ss=(int)v;
      sprintf(timestring,"    %02dh %02dm %02ds",hh,mm,ss);
    }
  sprintf(offsetstring, "%Ld", offset);
  sprintf(rolloverstring, "%Ld", rollover);

  EnterUpdate();
  gtk_entry_set_text (GTK_ENTRY (frequencyentry), frequencystring);
  gtk_entry_set_text (GTK_ENTRY (cycleentry), cyclestring);
  gtk_entry_set_text (GTK_ENTRY (timeentry), timestring);
  gtk_entry_set_text (GTK_ENTRY (offsetentry), offsetstring);
  gtk_entry_set_text (GTK_ENTRY (rolloverentry), rolloverstring);
  ExitUpdate();
}

static void zero_cb(GtkWidget *w, gpointer user_data)
{
  StopWatch_Window *sww=(StopWatch_Window *)user_data;

  sww->offset = sww->cyclecounter;

  sww->Update();
}

static void
modepopup_activated(GtkWidget *widget, gpointer data)
{
  StopWatch_Window *sww;

  unsigned char dir = *(unsigned char*)data;

  sww = (StopWatch_Window *)gtk_object_get_data(GTK_OBJECT(widget),"sww");


  switch(dir)
  {
  case '+':
    sww->count_dir=1;
    config_set_variable(sww->name(),"count_dir",sww->count_dir);
    break;
  case '-':
    sww->count_dir=-1;
    config_set_variable(sww->name(),"count_dir",sww->count_dir);
    break;
  default:
    assert(0);
    break;
  }

  sww->Update();
}

static void
cyclechanged(GtkWidget *widget, StopWatch_Window *sww)
{
    const char *text;
    if(widget==0|| sww==0)
    {
        printf("Warning cyclechanged(%p,%p)\n",widget,sww);
        return;
    }
    if(!sww->IsUpdate() && (text=gtk_entry_get_text (GTK_ENTRY(widget)))!=0)
    {
        long long v = strtoll(text,0,10);
	if(v!=(sww->cyclecounter-sww->offset)%sww->rollover)
	{
            v=(v+sww->offset)%sww->rollover;

            sww->cyclecounter=v;
	    sww->Update();
	}
    }
}

static void
offsetchanged(GtkWidget *widget, StopWatch_Window *sww)
{
    const char *text;
    if(widget==0|| sww==0)
    {
	printf("Warning offsetchanged(%p,%p)\n",widget,sww);
	return;
    }
    if(!sww->IsUpdate() && (text=gtk_entry_get_text (GTK_ENTRY(widget)))!=0)
    {
        long long v = strtoll(text,0,10);

	if(v!=sww->offset)
	{
            sww->offset=v;
	    sww->Update();
	}
    }
}
static void
rolloverchanged(GtkWidget *widget, StopWatch_Window *sww)
{
    const char *text;
    if(widget==0|| sww==0)
    {
	printf("Warning rolloverchanged(%p,%p)\n",widget,sww);
	return;
    }
    if(!sww->IsUpdate() && (text=gtk_entry_get_text (GTK_ENTRY(widget)))!=0)
    {
        long long v = strtoll(text,0,10);

	if(v!=sww->rollover)
	{
            sww->rollover=v;
	    config_set_string(sww->name(),"rollover",text);
	    sww->Update();
	}
    }
}


void StopWatch_Window::Build(void)
{
  if(bIsBuilt)
    return;

  GtkWidget *vbox, *button, *label, *entry;
  GtkWidget *menuitem, *optionmenu, *table, *optionmenu_menu;

  window=gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title(GTK_WINDOW(window), "StopWatch");

  gtk_window_set_default_size(GTK_WINDOW(window), width,height);
  gtk_widget_set_uposition(GTK_WIDGET(window),x,y);
  gtk_window_set_wmclass(GTK_WINDOW(window),name(),"Gpsim");

  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC(delete_event), (gpointer)this);
  gtk_signal_connect_after(GTK_OBJECT(window), "configure_event",
			   GTK_SIGNAL_FUNC(gui_object_configure_event),this);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  table = gtk_table_new (6, 2, FALSE);
  gtk_widget_show (table);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);

  label = gtk_label_new ("Cycles");
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label = gtk_label_new ("Time");
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label = gtk_label_new ("Processor frequency");
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  cycleentry = entry = gtk_entry_new ();
  gtk_widget_show (entry);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc)cyclechanged,this);

  timeentry = entry = gtk_entry_new ();
  gtk_widget_show (entry);
  gtk_entry_set_editable(GTK_ENTRY(entry),0);
  GTK_WIDGET_UNSET_FLAGS (entry, GTK_SENSITIVE | GTK_CAN_FOCUS);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  frequencyentry = entry = gtk_entry_new ();
  gtk_widget_show (entry);
  gtk_entry_set_editable(GTK_ENTRY(entry),0);
  GTK_WIDGET_UNSET_FLAGS (entry, GTK_SENSITIVE | GTK_CAN_FOCUS);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  label = gtk_label_new ("Count direction");
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  optionmenu = gtk_option_menu_new ();
  gtk_widget_show (optionmenu);
  gtk_table_attach (GTK_TABLE (table), optionmenu, 1, 2, 4, 5,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  optionmenu_menu = gtk_menu_new ();
  menuitem = gtk_menu_item_new_with_label ("Up");
  gtk_widget_show (menuitem);
  gtk_menu_append (GTK_MENU (optionmenu_menu), menuitem);
  gtk_object_set_data(GTK_OBJECT(menuitem), "sww", this);
  gtk_signal_connect(GTK_OBJECT(menuitem),"activate",
		     (GtkSignalFunc) modepopup_activated,
		     (gpointer)"+");
  menuitem = gtk_menu_item_new_with_label ("Down");
  gtk_widget_show (menuitem);
  gtk_menu_append (GTK_MENU (optionmenu_menu), menuitem);
  gtk_object_set_data(GTK_OBJECT(menuitem), "sww", this);
  gtk_signal_connect(GTK_OBJECT(menuitem),"activate",
		     (GtkSignalFunc) modepopup_activated,
		     (gpointer)"-");
  gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), optionmenu_menu);

  gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu),count_dir>0?0:1);

  label = gtk_label_new ("Cycle offset");
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  offsetentry = entry = gtk_entry_new ();
  gtk_widget_show (entry);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc)offsetchanged,this);

  label = gtk_label_new ("Rollover");
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  rolloverentry = entry = gtk_entry_new ();
  gtk_widget_show (entry);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 5, 6,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc)rolloverchanged,this);

  button = gtk_button_new_with_label ("Zero");
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 4);
  gtk_signal_connect(GTK_OBJECT(button),"clicked",
		     GTK_SIGNAL_FUNC(zero_cb),this);



  gtk_widget_show (window);


  bIsBuilt=true;
  
  UpdateMenuItem();
  Update();   
}

//------------------------------------------------------------------------
// 
//
StopWatch_Window::StopWatch_Window(GUI_Processor *_gp)
{
  char *string;

  menu = "<main>/Windows/Stopwatch";

  gp = _gp;
  set_name("stopwatch_viewer");
  wc = WC_data;
  wt = WT_stopwatch_window;
  window = 0;

  count_dir=1;
  rollover=1000000;
  cyclecounter=0;
  offset=0;

  from_update = 0;

  get_config();
  if(config_get_string(name(),"rollover",&string))
    rollover = strtoll(string,0,10);
  config_get_variable(name(),"count_dir",&count_dir);
    
  if(enabled)
    Build();
}


#endif // HAVE_GUI
