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
#include <sys/errno.h>

#include "../config.h"
#ifdef HAVE_GUI

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
//#include <gtkextra/gtksheetentry.h>

#include "gui.h"

typedef enum {
    MENU_TIME_USECONDS,
    MENU_TIME_MSECONDS,
    MENU_TIME_SECONDS,
    MENU_TIME_HHMMSS
} menu_id;

typedef struct _menu_item {
    char *name;
    menu_id id;
} menu_item;

static menu_item menu_items[] = {
    {"Micro seconds", MENU_TIME_USECONDS},
    {"Mili seconds", MENU_TIME_MSECONDS},
    {"Seconds", MENU_TIME_SECONDS},
    {"HH:MM:SS.CC", MENU_TIME_HHMMSS}
};

static menu_id time_format=MENU_TIME_USECONDS;

// Used only in popup menus
static StatusBar_Window *popup_sbw;

// called when user has selected a menu item
static void
popup_activated(GtkWidget *widget, gpointer data)
{
    menu_item *item;

    if(widget==NULL || data==NULL)
    {
	printf("Warning popup_activated(%x,%x)\n",(unsigned int)widget,(unsigned int)data);
	return;
    }
    
    item = (menu_item *)data;
    time_format = (menu_id)item->id;
    popup_sbw->Update(); //StatusBar_update(popup_sbw);
}

static GtkWidget *
build_menu(void)
{
  GtkWidget *menu;
  GtkWidget *item;
  int i;

  menu=gtk_menu_new();

  item = gtk_tearoff_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), item);
  gtk_widget_show (item);
  
  
  for (i=0; i < (sizeof(menu_items)/sizeof(menu_items[0])) ; i++){
      item=gtk_menu_item_new_with_label(menu_items[i].name);

      gtk_signal_connect(GTK_OBJECT(item),"activate",
			 (GtkSignalFunc) popup_activated,
			 &menu_items[i]);

      gtk_widget_show(item);
      gtk_menu_append(GTK_MENU(menu),item);
  }
  
  return menu;
}


void StatusBar_Window::Update(void)
{
  char buffer[32];

  if( !created)
      return;
  
  //update the displayed values

  status->value.i32 = gpsim_get_status(gp->pic_id);
  sprintf(buffer,"0x%02x",status->value.i32);
  gtk_entry_set_text (GTK_ENTRY (status->entry), buffer);


  W->value.i32 = gpsim_get_w(gp->pic_id);
  sprintf(buffer,"0x%02x",W->value.i32);
  gtk_entry_set_text (GTK_ENTRY (W->entry), buffer);

  if(!gp || !gp->cpu)
    return;

  pc->value.i32 = gp->cpu->pc->get_value();
  sprintf(buffer,"0x%04x",pc->value.i32);
  gtk_entry_set_text (GTK_ENTRY (pc->entry), buffer);

  cpu_cycles->value.ui64 = gpsim_get_cycles(gp->pic_id);
  sprintf(buffer,"0x%016Lx",cpu_cycles->value.ui64);
  gtk_entry_set_text (GTK_ENTRY (cpu_cycles->entry), buffer);

  if(time_format==MENU_TIME_USECONDS) {
    time->value.db = gpsim_get_cycles(gp->pic_id)*1e6/(double)gpsim_get_inst_clock(gp->pic_id);
    sprintf(buffer,"%19.2f us",time->value.db);
  }
  else if(time_format==MENU_TIME_MSECONDS) {
    time->value.db = gpsim_get_cycles(gp->pic_id)*1e3/(double)gpsim_get_inst_clock(gp->pic_id);
    sprintf(buffer,"%19.3f ms",time->value.db);
  }
  else if(time_format==MENU_TIME_HHMMSS) {
    double v=time->value.db = gpsim_get_cycles(gp->pic_id)/(double)gpsim_get_inst_clock(gp->pic_id);
    int hh=(int)(v/3600),mm,ss,cc;
    v-=hh*3600.0;
    mm=(int)(v/60);
    v-=mm*60.0;
    ss=(int)v;
    cc=(int)(v*100.0+0.5);
    sprintf(buffer,"    %02d:%02d:%02d.%02d",hh,mm,ss,cc);
  }
  else {
    time->value.db = gpsim_get_cycles(gp->pic_id)/(double)gpsim_get_inst_clock(gp->pic_id);
    sprintf(buffer,"%19.3f s",time->value.db);
  }
  gtk_entry_set_text (GTK_ENTRY (time->entry), buffer);

}

static void w_callback(GtkWidget *entry, StatusBar_Window *sbw)
{
    char *text;
    unsigned int value;
    unsigned int pic_id;
    char *bad_position;

    pic_id = sbw->gp->pic_id;
    text=gtk_entry_get_text (GTK_ENTRY (sbw->W->entry));
    
    value = strtoul(text, &bad_position, 16);
    if( strlen(bad_position) )
	return;  /* string contains an invalid number */

    gpsim_put_w(pic_id, value);

    sbw->Update();

    return;
}

static void status_callback(GtkWidget *entry, StatusBar_Window *sbw)
{
    char *text;
    unsigned int value;
    unsigned int pic_id;
    char *bad_position;

    pic_id = sbw->gp->pic_id;
    text=gtk_entry_get_text (GTK_ENTRY (sbw->status->entry));
    
    value = strtoul(text, &bad_position, 16);
    if( strlen(bad_position) )
	return;  /* string contains an invalid number */
    
    gpsim_put_status(pic_id, value);
    
    sbw->Update();

    return;
}

static void pc_callback(GtkWidget *entry, StatusBar_Window *sbw)
{
  char *text;
  unsigned int value;
  unsigned int pic_id;
  char *bad_position;

  if(!sbw || !sbw->gp || !sbw->gp->cpu)
    return;

  pic_id = sbw->gp->pic_id;
  text=gtk_entry_get_text (GTK_ENTRY (sbw->pc->entry));
    
  value = strtoul(text, &bad_position, 16);
  if( strlen(bad_position) )
    return;  /* string contains an invalid number */

  sbw->gp->cpu->pc->put_value(value);
    
  sbw->Update();
}



/*
 * create_labeled_entry
 */

labeled_entry *create_labeled_entry(GtkWidget *box,char *label, int string_width)
{

  labeled_entry *le;


  le = (labeled_entry *)malloc(sizeof(labeled_entry));

  le->label = gtk_label_new (label);

  gtk_misc_set_alignment (GTK_MISC (le->label), 1.0, 0.5);
  gtk_widget_set_usize (le->label, 0, 15);
  gtk_box_pack_start (GTK_BOX (box), le->label, FALSE, FALSE, 0);
  gtk_widget_show (le->label);

  le->entry = gtk_entry_new ();
//  gtk_signal_connect(GTK_OBJECT(le->entry), "activate",
//		     GTK_SIGNAL_FUNC(enter_callback),
//		     label);
  gtk_entry_set_text (GTK_ENTRY (le->entry), "----");

  le->value.i32 = 0;

  gtk_widget_set_usize (le->entry,
			string_width * gdk_string_width (le->entry->style->font, "9") + 6,
			-1);
  gtk_box_pack_start (GTK_BOX (box), le->entry, FALSE, FALSE, 0);
  gtk_widget_show (le->entry);

  return(le);
}


// button press handler
static gint
do_popup(GtkWidget *widget, GdkEventButton *event, StatusBar_Window *sbw)
{
    if(widget==NULL || event==NULL || sbw==NULL)
    {
        printf("Warning do_popup(%x,%x,%x)\n",(unsigned int)widget,(unsigned int)event,(unsigned int)sbw);
        return 0;
    }
  
    if( (event->type == GDK_BUTTON_PRESS) &&  (event->button == 3) )
    {
	popup_sbw = sbw;
  
	gtk_menu_popup(GTK_MENU(sbw->popup_menu), NULL, NULL, NULL, NULL,
			   3, event->time);
	// It looks like we need it to avoid a selection in the entry.
	// For this we tell the entry to stop reporting this event.
	gtk_signal_emit_stop_by_name(GTK_OBJECT(sbw->time->entry),"button_press_event");
    }
    return FALSE;
}

/*
 * CreateStatusBar
 *
 * Create the status bar at the bottom of the window
 * 
 *
 * vbox_main - The box to which we will append.
 *
 */ 

void StatusBar_Window::Create(GtkWidget *vbox_main)
{
  GtkWidget *hbox;

  /* --- Create h-box for holding the status line --- */
  hbox = gtk_hbox_new (FALSE, 0);

  /* --- Put up h-box --- */
  gtk_box_pack_end (GTK_BOX (vbox_main), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  status = create_labeled_entry(hbox,"Status:", 4);
  status->parent = this;
  gtk_signal_connect(GTK_OBJECT(status->entry), "activate",
		     GTK_SIGNAL_FUNC(status_callback),
		     this);

  W = create_labeled_entry(hbox,"W:", 4);
  W->parent = this;
  gtk_signal_connect(GTK_OBJECT(W->entry), "activate",
		     GTK_SIGNAL_FUNC(w_callback),
		     this);

  pc = create_labeled_entry(hbox,"PC:", 6);
  pc->parent = this;
  gtk_signal_connect(GTK_OBJECT(pc->entry), "activate",
		     GTK_SIGNAL_FUNC(pc_callback),
		     this);

  cpu_cycles = create_labeled_entry(hbox,"Cycles:", 18);
  cpu_cycles->parent = this;
  gtk_entry_set_editable(GTK_ENTRY(cpu_cycles->entry),0);

  time = create_labeled_entry(hbox,"Time:", 22);
  time->parent = this;
  gtk_entry_set_editable(GTK_ENTRY(time->entry),0);

  /* create popupmenu */
  popup_menu=build_menu();
  gtk_signal_connect(GTK_OBJECT(time->entry),
		     "button_press_event",
		     (GtkSignalFunc) do_popup,
		     this);

  created=1;
  
}

void StatusBar_update_xref(struct cross_reference_to_gui *xref, int new_value)
{
  StatusBar_Window *sbw;

  sbw  = (StatusBar_Window *) (xref->parent_window);
  sbw->Update();
}

void StatusBar_Window::NewProcessor(GUI_Processor *_gp)
{

  struct cross_reference_to_gui  *cross_reference;

  if(_gp == NULL)
    return;

  gp = _gp;

  gp->status_bar = this;


  /* Now create a cross-reference link that the simulator can use to
   * send information back to the gui
   */

  cross_reference = (struct cross_reference_to_gui *) malloc(sizeof(struct cross_reference_to_gui));
  cross_reference->parent_window_type =   WT_status_bar;
  cross_reference->parent_window = (gpointer) this;
  cross_reference->data = (gpointer) this;
  cross_reference->update = StatusBar_update_xref;
  cross_reference->remove = NULL;
  
  gpsim_assign_pc_xref(gp->pic_id, (gpointer) cross_reference);

  Update();

}

StatusBar_Window::StatusBar_Window(void)
{
  gp = NULL;

  popup_menu = NULL;
  
  status = NULL;
  W = NULL;
  pc = NULL;
  cpu_cycles = NULL;
  time = NULL;
  
}

#endif // HAVE_GUI
