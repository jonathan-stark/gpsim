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
#include <errno.h>

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


#include "gui.h"
#include "gui_statusbar.h"
#include "gui_regwin.h"

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

//========================================================================
//
// A LabeledEntry is an object consisting of gtk entry
// widget that is labeled (with a gtk lable widget)
//

class LabeledEntry {
public:
  GtkWidget *label;
  GtkWidget *entry;
  StatusBar_Window *sbw;

  union {
    gint32    i32;
    guint64   ui64;
    double    db;
  } value;           // value displayed


  LabeledEntry(void);
  void Create(GtkWidget *box,char *clabel, int string_width);
  void NewLabel(char *clabel);
  virtual void Update(void);
  void AssignParent(StatusBar_Window *);
  virtual void put_value(unsigned int);

};

class RegisterLabeledEntry : public LabeledEntry {
public:

  Register *reg;

  RegisterLabeledEntry(Register *);

  virtual void put_value(unsigned int);
  void AssignRegister(Register *new_reg) {reg = new_reg;}
};

//========================================================================

class StatusBarXREF : public CrossReferenceToGUI
{
public:

  void Update(int new_value)
  {

    StatusBar_Window *sbw;

    sbw  = (StatusBar_Window *) (parent_window);
    sbw->Update();

  }
};

//========================================================================


LabeledEntry::LabeledEntry(void)
{
  label = 0;
  entry = 0;
}

void LabeledEntry::Create(GtkWidget *box,char *clabel, int string_width)
{

  label = (GtkWidget *)gtk_label_new (clabel);
    
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_widget_set_usize (label, 0, 15);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  entry = gtk_entry_new ();

  gtk_entry_set_text (GTK_ENTRY (entry), "----");

  value.i32 = 0;


#if GTK_MAJOR_VERSION >= 2
  gtk_widget_set_usize (entry,
			string_width * gdk_string_width (gtk_style_get_font(entry->style), "9") + 6,
			-1);
#else
  gtk_widget_set_usize (entry,
			string_width * gdk_string_width (entry->style->font, "9") + 6,
			-1);
#endif

  gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);
}

void LabeledEntry::AssignParent(StatusBar_Window *new_sbw)
{
  sbw = new_sbw;
}

void LabeledEntry::Update(void)
{
  if(sbw)
    sbw->Update();
}

void LabeledEntry::put_value(unsigned int new_value)
{

}

void LabeledEntry::NewLabel(char *clabel)
{
  if(label)
    gtk_label_set_text(GTK_LABEL(label),clabel);

}
//------------------------------------------------------------------------
RegisterLabeledEntry::RegisterLabeledEntry(Register *new_reg) 
 : LabeledEntry()
{
  reg = new_reg;
}

void RegisterLabeledEntry::put_value(unsigned int new_value)
{
  if(reg)
    reg->put_value(new_value);
}

//------------------------------------------------------------------------
// called when user has selected a menu item
static void
popup_activated(GtkWidget *widget, gpointer data)
{
    menu_item *item;

    if(widget==0 || data==0)
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
  unsigned int i;

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

  if(!gp || !gp->cpu)
    return;

  pic_processor *pic = dynamic_cast<pic_processor *>(gp->cpu);
  if(!pic)
    return;

  status->value.i32 = pic->status->get_value();
  sprintf(buffer,"0x%02x",status->value.i32);
  gtk_entry_set_text (GTK_ENTRY (status->entry), buffer);


  W->value.i32 = pic->W->get_value();
  sprintf(buffer,"0x%02x",W->value.i32);
  gtk_entry_set_text (GTK_ENTRY (W->entry), buffer);


  pc->value.i32 = gp->cpu->pc->get_value();
  sprintf(buffer,"0x%04x",pc->value.i32);
  gtk_entry_set_text (GTK_ENTRY (pc->entry), buffer);

  cpu_cycles->value.ui64 = cycles.value;
  sprintf(buffer,"0x%016Lx",cpu_cycles->value.ui64);
  gtk_entry_set_text (GTK_ENTRY (cpu_cycles->entry), buffer);

  time->value.db = 4.0 * cycles.value / (double)pic->time_to_cycles(1.0);

  if(time_format==MENU_TIME_USECONDS) {
    time->value.db *= 1e6;
    sprintf(buffer,"%19.2f us",time->value.db);
  }
  else if(time_format==MENU_TIME_MSECONDS) {
    time->value.db *= 1e3;
    sprintf(buffer,"%19.3f ms",time->value.db);
  }
  else if(time_format==MENU_TIME_HHMMSS) {
    double v=time->value.db;
    int hh=(int)(v/3600),mm,ss,cc;
    v-=hh*3600.0;
    mm=(int)(v/60);
    v-=mm*60.0;
    ss=(int)v;
    cc=(int)(v*100.0+0.5);
    sprintf(buffer,"    %02d:%02d:%02d.%02d",hh,mm,ss,cc);
  }
  else {
    sprintf(buffer,"%19.3f s",time->value.db);
  }
  gtk_entry_set_text (GTK_ENTRY (time->entry), buffer);

}

static void LabeledEntry_callback(GtkWidget *entry, LabeledEntry *le)
{
  const char *text;
  unsigned int value;
  char *bad_position;

  if(!gp || !gp->cpu || !le || !le->entry)
    return;

  text=gtk_entry_get_text (GTK_ENTRY (le->entry));
    
  value = strtoul(text, &bad_position, 16);
  if( strlen(bad_position) )
    return;  /* string contains an invalid number */

  le->put_value(value);
  le->Update();

  return;
}

static void w_callback(GtkWidget *entry, StatusBar_Window *sbw)
{
  const char *text;
  unsigned int value;
  char *bad_position;

  if(!gp || !gp->cpu)
    return;

  pic_processor *pic = dynamic_cast<pic_processor *>(gp->cpu);
  if(!pic)
    return;

  text=gtk_entry_get_text (GTK_ENTRY (sbw->W->entry));
    
  value = strtoul(text, &bad_position, 16);
  if( strlen(bad_position) )
    return;  /* string contains an invalid number */

  pic->W->put_value(value);

  sbw->Update();

  return;
}

static void pc_callback(GtkWidget *entry, StatusBar_Window *sbw)
{
  const char *text;
  unsigned int value;
  char *bad_position;

  if(!sbw || !sbw->gp || !sbw->gp->cpu)
    return;

  text=gtk_entry_get_text (GTK_ENTRY (sbw->pc->entry));
    
  value = strtoul(text, &bad_position, 16);
  if( strlen(bad_position) )
    return;  /* string contains an invalid number */

  sbw->gp->cpu->pc->put_value(value);
    
  sbw->Update();
}



// button press handler
static gint
do_popup(GtkWidget *widget, GdkEventButton *event, StatusBar_Window *sbw)
{
    if(widget==0 || event==0 || sbw==0)
    {
        printf("Warning do_popup(%x,%x,%x)\n",
	       (unsigned int)widget,
	       (unsigned int)event,
	       (unsigned int)sbw);
        return 0;
    }
  
    if( (event->type == GDK_BUTTON_PRESS) &&  (event->button == 3) )
    {
	popup_sbw = sbw;
  
	gtk_menu_popup(GTK_MENU(sbw->popup_menu), 0, 0, 0, 0,
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

  status = new RegisterLabeledEntry(0);
  status->Create(hbox,"Status:", 4);

  gtk_signal_connect(GTK_OBJECT(status->entry), "activate",
		     GTK_SIGNAL_FUNC(LabeledEntry_callback),
		     status);

  W = new LabeledEntry();
  W->Create(hbox,"W:", 4);
  gtk_signal_connect(GTK_OBJECT(W->entry), "activate",
		     GTK_SIGNAL_FUNC(w_callback),
		     this);

  pc = new LabeledEntry();
  pc->Create(hbox,"PC:", 6);
  pc->AssignParent(this);

  gtk_signal_connect(GTK_OBJECT(pc->entry), "activate",
		     GTK_SIGNAL_FUNC(pc_callback),
		     this);

  cpu_cycles = new LabeledEntry();
  cpu_cycles->Create(hbox,"Cycles:", 18);

  gtk_entry_set_editable(GTK_ENTRY(cpu_cycles->entry),0);

  time = new LabeledEntry();
  time->Create(hbox,"Time:", 22);

  gtk_entry_set_editable(GTK_ENTRY(time->entry),0);

  /* create popupmenu */
  popup_menu=build_menu();
  gtk_signal_connect(GTK_OBJECT(time->entry),
		     "button_press_event",
		     (GtkSignalFunc) do_popup,
		     this);

  created=1;
  
}

void StatusBar_Window::NewProcessor(GUI_Processor *_gp)
{


  if(_gp == 0)
    return;

  gp = _gp;

  gp->status_bar = this;

  if(gp->cpu) {

    RegisterLabeledEntry *rle = dynamic_cast<RegisterLabeledEntry *>(status);
    pic_processor *pic = dynamic_cast<pic_processor *>(gp->cpu);
    if(pic && rle)
      rle->AssignRegister(pic->status);

    status->NewLabel((char *) rle->reg->name().c_str());

  }

  /* Now create a cross-reference link that the simulator can use to
   * send information back to the gui
   */

  if(gp->cpu && gp->cpu->pc) {
    StatusBarXREF *cross_reference;

    cross_reference = new StatusBarXREF();
    cross_reference->parent_window_type =   WT_status_bar;
    cross_reference->parent_window = (gpointer) this;
    cross_reference->data = (gpointer) this;
  
    gp->cpu->pc->add_xref((gpointer) cross_reference);

  }

  Update();

}

StatusBar_Window::StatusBar_Window(void)
{
  gp = 0;

  popup_menu = 0;
  
  status = 0;
  W = 0;
  pc = 0;
  cpu_cycles = 0;
  time = 0;
  
}

#endif // HAVE_GUI
