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
#include <sys/errno.h>

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

static int delete_event(GtkWidget *widget,
			GdkEvent  *event,
                        StopWatch_Window *sww)
{
    ((GUI_Object *)sww)->change_view((GUI_Object*)sww,VIEW_HIDE);
    return TRUE;
}


static void update(StopWatch_Window *sww)
{
    unsigned int pic_id;

    long long cyclecounter;
    static long long cyclecounter_last;
    long frequency;
    long long offset;
    long long rollover;
    unsigned long long timevalue;

    char frequencystring[100];
    char cyclestring[100];
    char timestring[100];
    char offsetstring[100];
    char rolloverstring[100];

    if(!sww->has_processor)
	return;
    
    pic_id = ((GUI_Object*)sww)->gp->pic_id;

    if(sww->rollover<=0)
        sww->rollover=1;

    if(sww->offset>sww->rollover)
	sww->offset%=sww->rollover;

    rollover=sww->rollover;
    offset=sww->offset;
    frequency = gpsim_get_inst_clock(pic_id)*4;
    cyclecounter=sww->cyclecounter;

    ////////////////////////

    if(sww->count_dir<0)
	cyclecounter -= gpsim_get_cycles(pic_id)-cyclecounter_last;
    else
	cyclecounter += gpsim_get_cycles(pic_id)-cyclecounter_last;

    cyclecounter_last=gpsim_get_cycles(pic_id);


    while(cyclecounter<offset)
        cyclecounter+=rollover;

    sww->cyclecounter=cyclecounter;

    cyclecounter=(cyclecounter-offset)%rollover;
    ////////////////////////

    timevalue = (cyclecounter*4000000)/frequency;

    sprintf(frequencystring, "%Ld Hz", frequency);
    sprintf(cyclestring, "%Ld", cyclecounter);
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

    gtk_entry_set_text (GTK_ENTRY (sww->frequencyentry), frequencystring);
    gtk_entry_set_text (GTK_ENTRY (sww->cycleentry), cyclestring);
    gtk_entry_set_text (GTK_ENTRY (sww->timeentry), timestring);
    gtk_entry_set_text (GTK_ENTRY (sww->offsetentry), offsetstring);
    gtk_entry_set_text (GTK_ENTRY (sww->rolloverentry), rolloverstring);

}

static void zero_cb(GtkWidget *w, gpointer user_data)
{
    unsigned int pic_id;
    StopWatch_Window *sww=(StopWatch_Window *)user_data;

    pic_id = ((GUI_Object*)sww)->gp->pic_id;

    sww->offset = sww->cyclecounter;

    update(sww);
}

void StopWatchWindow_new_processor(StopWatch_Window *sww, GUI_Processor *gp)
{
    sww->has_processor=1;
}

void StopWatchWindow_update(StopWatch_Window *sww)
{
    if( !((GUI_Object*)sww)->enabled)
	return;

    update(sww);
}

static void
modepopup_activated(GtkWidget *widget, gpointer data)
{
    StopWatch_Window *sww;

    unsigned int pic_id;

    unsigned char dir = *(unsigned char*)data;

    sww = gtk_object_get_data(GTK_OBJECT(widget),"sww");

    pic_id = ((GUI_Object*)sww)->gp->pic_id;

    switch(dir)
    {
    case '+':
	sww->count_dir=1;
	config_set_variable(sww->gui_obj.name,"count_dir",sww->count_dir);
	break;
    case '-':
	sww->count_dir=-1;
	config_set_variable(sww->gui_obj.name,"count_dir",sww->count_dir);
        break;
    default:
	assert(0);
        break;
    }

    update(sww);
}

static void
cyclechanged(GtkWidget *widget, StopWatch_Window *sww)
{
    char *text;
    if(widget==NULL|| sww==NULL)
    {
	printf("Warning cyclechanged(%p,%p)\n",widget,sww);
	return;
    }
    if((text=gtk_entry_get_text (GTK_ENTRY(widget)))!=NULL)
    {
        long long v;
	v=atoll(text);
	if(v!=(sww->cyclecounter-sww->offset)%sww->rollover)
	{
            v=(v+sww->offset)%sww->rollover;

            sww->cyclecounter=v;
	    update(sww);
	}
    }
}

static void
offsetchanged(GtkWidget *widget, StopWatch_Window *sww)
{
    char *text;
    if(widget==NULL|| sww==NULL)
    {
	printf("Warning offsetchanged(%p,%p)\n",widget,sww);
	return;
    }
    if((text=gtk_entry_get_text (GTK_ENTRY(widget)))!=NULL)
    {
        long long v;
	v=atoll(text);
	if(v!=sww->offset)
	{
            sww->offset=v;
	    update(sww);
	}
    }
}
static void
rolloverchanged(GtkWidget *widget, StopWatch_Window *sww)
{
    char *text;
    if(widget==NULL|| sww==NULL)
    {
	printf("Warning rolloverchanged(%p,%p)\n",widget,sww);
	return;
    }
    if((text=gtk_entry_get_text (GTK_ENTRY(widget)))!=NULL)
    {
        long long v;
	v=atoll(text);
	if(v!=sww->rollover)
	{
            sww->rollover=v;
	    config_set_string(sww->gui_obj.name,"rollover",text);
	    update(sww);
	}
    }
}


int BuildStopWatchWindow(StopWatch_Window *sww)
{
    GtkWidget *window;
    GtkWidget *vbox, *button, *label, *entry;
    GtkWidget *menuitem, *optionmenu, *table, *optionmenu_menu;

    int x,y,width,height;

    window=sww->gui_obj.window=gtk_window_new(GTK_WINDOW_TOPLEVEL);
    sww->gui_obj.window = window;

    gtk_window_set_title(GTK_WINDOW(sww->gui_obj.window), "StopWatch");

    width=((GUI_Object*)sww)->width;
    height=((GUI_Object*)sww)->height;
    x=((GUI_Object*)sww)->x;
    y=((GUI_Object*)sww)->y;
    gtk_window_set_default_size(GTK_WINDOW(sww->gui_obj.window), width,height);
    gtk_widget_set_uposition(GTK_WIDGET(sww->gui_obj.window),x,y);
    gtk_window_set_wmclass(GTK_WINDOW(sww->gui_obj.window),sww->gui_obj.name,"Gpsim");

    gtk_signal_connect (GTK_OBJECT (window), "destroy",
			GTK_SIGNAL_FUNC (gtk_widget_destroyed), &window);
    gtk_signal_connect (GTK_OBJECT (sww->gui_obj.window), "delete_event",
			GTK_SIGNAL_FUNC(delete_event), (gpointer)sww);
    gtk_signal_connect_after(GTK_OBJECT(sww->gui_obj.window), "configure_event",
			     GTK_SIGNAL_FUNC(gui_object_configure_event),sww);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox);
  gtk_container_add (GTK_CONTAINER (window), vbox);

  table = gtk_table_new (6, 2, FALSE);
  gtk_widget_show (table);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);

  label = gtk_label_new ("Delta cycles");
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

  sww->cycleentry = entry = gtk_entry_new ();
  gtk_widget_show (entry);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc)cyclechanged,sww);

  sww->timeentry = entry = gtk_entry_new ();
  gtk_widget_show (entry);
  gtk_entry_set_editable(GTK_ENTRY(entry),0);
  GTK_WIDGET_UNSET_FLAGS (entry, GTK_SENSITIVE | GTK_CAN_FOCUS);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  sww->frequencyentry = entry = gtk_entry_new ();
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
  gtk_object_set_data(GTK_OBJECT(menuitem), "sww", sww);
  gtk_signal_connect(GTK_OBJECT(menuitem),"activate",
		     (GtkSignalFunc) modepopup_activated,
		     "+");
  menuitem = gtk_menu_item_new_with_label ("Down");
  gtk_widget_show (menuitem);
  gtk_menu_append (GTK_MENU (optionmenu_menu), menuitem);
  gtk_object_set_data(GTK_OBJECT(menuitem), "sww", sww);
  gtk_signal_connect(GTK_OBJECT(menuitem),"activate",
		     (GtkSignalFunc) modepopup_activated,
		     "-");
  gtk_option_menu_set_menu (GTK_OPTION_MENU (optionmenu), optionmenu_menu);

  gtk_option_menu_set_history (GTK_OPTION_MENU (optionmenu),sww->count_dir>0?0:1);

  label = gtk_label_new ("Cycle offset");
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 3, 4,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  sww->offsetentry = entry = gtk_entry_new ();
  gtk_widget_show (entry);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 3, 4,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc)offsetchanged,sww);

  label = gtk_label_new ("Rollover");
  gtk_widget_show (label);
  gtk_table_attach (GTK_TABLE (table), label, 0, 1, 5, 6,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  sww->rolloverentry = entry = gtk_entry_new ();
  gtk_widget_show (entry);
  gtk_table_attach (GTK_TABLE (table), entry, 1, 2, 5, 6,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_signal_connect(GTK_OBJECT(entry), "changed",
		     (GtkSignalFunc)rolloverchanged,sww);

  button = gtk_button_new_with_label ("Zero");
  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 4);
  gtk_signal_connect(GTK_OBJECT(button),"clicked",
		     GTK_SIGNAL_FUNC(zero_cb),sww);



    gtk_widget_show (window);


    sww->gui_obj.enabled=1;

    sww->gui_obj.is_built=1;
    
    update_menu_item((GUI_Object*)sww);

    update(sww);
    
    return 0;
}

int CreateStopWatchWindow(GUI_Processor *gp)
{
    StopWatch_Window *stopwatch_window;
    char *string;

    stopwatch_window = malloc(sizeof(StopWatch_Window));

    stopwatch_window->gui_obj.gp = gp;
    stopwatch_window->gui_obj.name = "stopwatch_viewer";
    stopwatch_window->gui_obj.wc = WC_data;
    stopwatch_window->gui_obj.wt = WT_stopwatch_window;
    stopwatch_window->gui_obj.change_view = SourceBrowser_change_view;
    stopwatch_window->gui_obj.window = NULL;
    stopwatch_window->gui_obj.is_built=0;
    gp->stopwatch_window = stopwatch_window;
    stopwatch_window->count_dir=1;
    stopwatch_window->rollover=1000000;
    stopwatch_window->cyclecounter=0;
    stopwatch_window->frequency=0;
    stopwatch_window->offset=0;

    stopwatch_window->has_processor=1;


    gp_add_window_to_list(gp, (GUI_Object *)stopwatch_window);


    gui_object_get_config((GUI_Object*)stopwatch_window);
    if(config_get_string(stopwatch_window->gui_obj.name,"rollover",&string))
	stopwatch_window->rollover=atoll(string);
    config_get_variable(stopwatch_window->gui_obj.name,"count_dir",&stopwatch_window->count_dir);
    
    if(stopwatch_window->gui_obj.enabled)
	BuildStopWatchWindow(stopwatch_window);

    return 1;
}

#endif // HAVE_GUI
