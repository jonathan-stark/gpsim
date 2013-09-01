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


//#define DEBUG

#include "gui.h"
#include "gui_statusbar.h"
#include "../src/processor.h"


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


//------------------------------------------------------------------------
static void LabeledEntry_callback(GtkWidget *entry, LabeledEntry *le)
{
  const char *text;
  unsigned int value;
  char *bad_position;

  if(!gpGuiProcessor || !gpGuiProcessor->cpu || !le || !le->entry)
    return;

  text=gtk_entry_get_text (GTK_ENTRY (le->entry));
    
  value = strtoul(text, &bad_position, 16);
  if( strlen(bad_position) )
    return;  /* string contains an invalid number */

  le->put_value(value);

  if(le->parent)
    le->parent->Update();

  return;
}

//========================================================================
EntryWidget::EntryWidget()
{
  entry = 0;
  parent = 0;
}

void EntryWidget::SetEntryWidth(int string_width)
{
  if(entry)
    gtk_entry_set_width_chars(GTK_ENTRY (entry), string_width);
}

void EntryWidget::AssignParent(GUI_Object *new_parent)
{
  parent = new_parent;
}


void EntryWidget::Create(bool isEditable)
{
  entry = gtk_entry_new ();
  if(!isEditable)
    gtk_entry_set_editable(GTK_ENTRY(entry),0);
  gtk_widget_show (entry);
}

//========================================================================


LabeledEntry::LabeledEntry(void)
{
  label = 0;
}
#if GTK_MAJOR_VERSION < 2
#define gtk_style_get_font(X) X->font
#endif

void LabeledEntry::Create(GtkWidget *box,
			  char *clabel, 
			  int string_width,
			  bool isEditable=true)
{

  label = (GtkWidget *)gtk_label_new (clabel);
    
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_widget_set_usize (label, 0, 15);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  EntryWidget::Create(isEditable);

  gtk_entry_set_text (GTK_ENTRY (entry), "----");

  SetEntryWidth(string_width);

  gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 0);



}

void LabeledEntry::Update(void)
{
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
RegisterLabeledEntry::RegisterLabeledEntry(GtkWidget *box,
					   Register *new_reg,
					   bool isEditable=true) 
 : LabeledEntry()
{
  reg = new_reg;

  if(reg) {
    pCellFormat = new char[10];
    sprintf(pCellFormat,"0x%%0%dx",reg->register_size()*2);

    label = (GtkWidget *)gtk_label_new ((char *) reg->name().c_str());
    
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    entry = gtk_entry_new ();
    SetEntryWidth(2 + reg->register_size()*2);
    Update();

    gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 0);

    gtk_widget_show (entry);

    if(!isEditable)
      gtk_entry_set_editable(GTK_ENTRY(entry),0);

    gtk_signal_connect(GTK_OBJECT(entry), "activate",
		       GTK_SIGNAL_FUNC(LabeledEntry_callback),
		       this);


  } else
    pCellFormat=0;

}

void RegisterLabeledEntry::put_value(unsigned int new_value)
{
  if(reg)
    reg->put_value(new_value);
}

void RegisterLabeledEntry::Update(void)
{
  char buffer[32];

  if(reg && pCellFormat) {

    unsigned int value = reg->get_value();

    sprintf(buffer,pCellFormat,value);

    gtk_entry_set_text (GTK_ENTRY (entry), buffer);

  }
}
void RegisterLabeledEntry::AssignRegister(Register *new_reg)
{
  reg = new_reg;

  if(pCellFormat)
    delete pCellFormat;

  if(reg) {
    pCellFormat = new char[10];
    sprintf(pCellFormat,"0x%%0%dx",reg->register_size()*2);

    NewLabel((char *) reg->name().c_str());
    SetEntryWidth(2 + reg->register_size()*2);
  }
}

//------------------------------------------------------------------------
void StatusBar_Window::Update(void)
{

  if (!created)
      return;

  //update the displayed values

  if(!gp || !gp->cpu)
    return;


  list<RegisterLabeledEntry *>::iterator iRLE;

  for(iRLE = entries.begin();
      iRLE != entries.end();
      ++iRLE)
    (*iRLE)->Update();
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

  if(created)
    return;
  Dprintf((" %s\n",__FUNCTION__));

  /* --- Put up h-box --- */
  gtk_box_pack_end (GTK_BOX (vbox_main), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  created=1;


}

/*  NewProcessor
 *
 */

void StatusBar_Window::NewProcessor(GUI_Processor *_gp, MemoryAccess *_ma)
{


  if(!_gp  || !_gp->cpu || !_ma)
    return;

  if(ma)
    return;

  gp = _gp;
  ma = _ma;
  Dprintf((" %s\n",__FUNCTION__));

  list<Register *>::iterator iReg;

  for(iReg = ma->SpecialRegisters.begin();
      iReg != ma->SpecialRegisters.end();
      ++iReg)
    entries.push_back(new RegisterLabeledEntry(hbox, *iReg));


  /* Now create a cross-reference link that the simulator can use to
   * send information back to the gui
   */

  Program_Counter * pPC;
  ProgramMemoryAccess* pPMA;
  pPMA = dynamic_cast<ProgramMemoryAccess*>(ma);

  if(gp->cpu && gp->cpu->pc) {
    pPC = pPMA == NULL ? gp->cpu->pc : pPMA->GetProgramCounter();
    StatusBarXREF *cross_reference;

    cross_reference = new StatusBarXREF();
    cross_reference->parent_window_type =   WT_status_bar;
    cross_reference->parent_window = (gpointer) this;
    cross_reference->data = (gpointer) this;
  
    pPC->add_xref((gpointer) cross_reference);

  }

  Update();

}

StatusBar_Window::StatusBar_Window(void)
{
  gp = 0;
  ma = 0;

  cpu_cycles = 0;
  time = 0;
  created = false;

  /* --- Create h-box for holding the status line --- */
  hbox = gtk_hbox_new (FALSE, 0);

}












#if 0 
class CyclesLabeledEntry : public LabeledEntry {
public:

  CyclesLabeledEntry();
  virtual void Update(void);
};


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

class TimeLabeledEntry : public LabeledEntry 
{
public:
  TimeLabeledEntry();
  virtual void Update(void);
  GtkWidget *build_menu();

  void set_time_format(menu_id id)
  {
    time_format = id;
  }

  GtkWidget *menu;
  menu_id time_format;

};

struct popup_data {
  TimeLabeledEntry *tle;
  menu_id id;
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

  /**
    * Remove()
    * Override to circumvent the default behavior
    */
  void Remove(void) {}
};


//------------------------------------------------------------------------
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

  if(le->sbw)
    le->sbw->Update();

  return;
}

//========================================================================


LabeledEntry::LabeledEntry(void)
{
  label = 0;
  entry = 0;
  sbw = 0;
}

LabeledEntry::~LabeledEntry() {
  gtk_widget_destroy(entry);
  gtk_widget_destroy(label);
}

#if GTK_MAJOR_VERSION < 2
#define gtk_style_get_font(X) X->font
#endif

void LabeledEntry::Create(GtkWidget *box,
			  char *clabel, 
			  int string_width,
			  bool isEditable=true)
{
  // Entry to the right
  entry = gtk_entry_new ();
  gtk_entry_set_text (GTK_ENTRY (entry), "----");
  SetEntryWidth(string_width);
  gtk_box_pack_end (GTK_BOX (box), entry, FALSE, FALSE, 0);
  gtk_widget_show (entry);

  if(!isEditable)
    gtk_entry_set_editable(GTK_ENTRY(entry),0);
  // Lable to the left of the entry
  label = (GtkWidget *)gtk_label_new (clabel);
    
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_widget_set_usize (label, 0, 15);
  gtk_box_pack_end (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
}

void LabeledEntry::SetEntryWidth(int string_width)
{
  if(entry)
    gtk_widget_set_usize (entry,
			  string_width * 
			  gdk_string_width (gtk_style_get_font(entry->style), "9") + 6,
			  -1);
}

void LabeledEntry::AssignParent(StatusBar_Window *new_sbw)
{
  sbw = new_sbw;
}

void LabeledEntry::Update(void)
{
  //  if(sbw)
  //    sbw->Update();

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
RegisterLabeledEntry::RegisterLabeledEntry(GtkWidget *box,
					   Register *new_reg,
					   bool isEditable) 
 : LabeledEntry()
{
  reg = new_reg;

  if(reg) {

    pCellFormat = new char[10];
    sprintf(pCellFormat,"0x%%0%dx",reg->register_size()*2);

    label = (GtkWidget *)gtk_label_new ((char *) reg->name().c_str());
    
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    //    gtk_widget_set_usize (label, 0, 15);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    entry = gtk_entry_new ();

    // gtk_entry_set_text (GTK_ENTRY (entry), "----");

    SetEntryWidth(2 + reg->register_size()*2);
    Update();

    //SetEntryWidth(string_width);

    gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 0);

    gtk_widget_show (entry);

    if(!isEditable)
      gtk_entry_set_editable(GTK_ENTRY(entry),0);

    gtk_signal_connect(GTK_OBJECT(entry), "activate",
		       GTK_SIGNAL_FUNC(LabeledEntry_callback),
		       this);


  } else
    pCellFormat=0;

}

RegisterLabeledEntry::~RegisterLabeledEntry() {
  delete pCellFormat;
}

void RegisterLabeledEntry::put_value(unsigned int new_value)
{
  if(reg)
    reg->put_value(new_value);
}

void RegisterLabeledEntry::Update(void)
{
  char buffer[32];

  if(reg && pCellFormat) {

    unsigned int value = reg->get_value();

    sprintf(buffer,pCellFormat,value);

    gtk_entry_set_text (GTK_ENTRY (entry), buffer);

  }
}
void RegisterLabeledEntry::AssignRegister(Register *new_reg)
{
  reg = new_reg;

  if(pCellFormat)
    delete pCellFormat;

  if(reg) {
    pCellFormat = new char[10];
    sprintf(pCellFormat,"0x%%0%dx",reg->register_size()*2);

    NewLabel((char *) reg->name().c_str());
    SetEntryWidth(2 + reg->register_size()*2);
  }
}

//------------------------------------------------------------------------
CyclesLabeledEntry::CyclesLabeledEntry()
{
}

void CyclesLabeledEntry::Update(void)
{
  char buffer[32];
  sprintf(buffer,"0x%016" PRINTF_INT64_MODIFIER "x",get_cycles().value);
  gtk_entry_set_text (GTK_ENTRY (entry), buffer);
}

TimeLabeledEntry::TimeLabeledEntry()
{
  time_format = MENU_TIME_USECONDS;
  menu = 0;
}

void TimeLabeledEntry::Update()
{
  char buffer[32];

  double time_db = gp->cpu->get_InstPeriod() * get_cycles().value;


  switch(time_format) {
  case MENU_TIME_USECONDS:
    time_db *= 1e6;
    sprintf(buffer,"%19.2f us",time_db);
    break;
  
  case MENU_TIME_MSECONDS:
    time_db *= 1e3;
    sprintf(buffer,"%19.3f ms",time_db);
    break;

  case MENU_TIME_HHMMSS:
    {
      double v=time_db + 0.005;	// round msec
      int hh=(int)(v/3600),mm,ss,cc;
      v-=hh*3600.0;
      mm=(int)(v/60);
      v-=mm*60.0;
      ss=(int)v;
      cc=(int)((v-ss)*100.0);
      sprintf(buffer,"    %02d:%02d:%02d.%02d",hh,mm,ss,cc);
    }
    break;

  default:
    sprintf(buffer,"%19.3f s",time_db);
  }

  gtk_entry_set_text (GTK_ENTRY (entry), buffer);

}

//----------------------------------------
// called when user has selected a menu item
static void
popup_activated(GtkWidget *widget, gpointer data)
{
  if(!widget || !data)
    return;
    
  popup_data *pd = (popup_data *)data;
  if(pd->tle) {
    pd->tle->set_time_format(pd->id);
    pd->tle->Update();
  }
}

GtkWidget * TimeLabeledEntry::build_menu(void)
{
  static menu_item menu_items[] = {
    {"Micro seconds", MENU_TIME_USECONDS},
    {"Mili seconds", MENU_TIME_MSECONDS},
    {"Seconds", MENU_TIME_SECONDS},
    {"HH:MM:SS.CC", MENU_TIME_HHMMSS}
  };


  GtkWidget *item;
  unsigned int i;

  menu=gtk_menu_new();

  item = gtk_tearoff_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), item);
  gtk_widget_show (item);
  
  
  for (i=0; i < (sizeof(menu_items)/sizeof(menu_items[0])) ; i++){
    item=gtk_menu_item_new_with_label(menu_items[i].name);

    popup_data *pd = new popup_data;
    pd->tle = this;
    pd->id = menu_items[i].id;

    gtk_signal_connect(GTK_OBJECT(item),"activate",
		       (GtkSignalFunc) popup_activated,
		       pd);

    gtk_widget_show(item);
    gtk_menu_append(GTK_MENU(menu),item);
  }
  
  return menu;
}

//------------------------------------------------------------------------
void StatusBar_Window::Update(void)
{

  if( !created)
      return;

  //update the displayed values

  if(!gp || !gp->cpu)
    return;


  RegisterLabeledList::iterator iRLE;

  for(iRLE = entries.begin();
      iRLE != entries.end();
      ++iRLE)

    (*iRLE)->Update();

  cpu_cycles->Update();
  time->Update();
}


// button press handler
static gint
do_popup(GtkWidget *widget, GdkEventButton *event, TimeLabeledEntry *tle)
{
    if(!widget || !event || !tle)
      return 0;
  
    if( (event->type == GDK_BUTTON_PRESS) &&  (event->button == 3) )
    {
	gtk_menu_popup(GTK_MENU(tle->menu), 0, 0, 0, 0,
			   3, event->time);
	// It looks like we need it to avoid a selection in the entry.
	// For this we tell the entry to stop reporting this event.
	gtk_signal_emit_stop_by_name(GTK_OBJECT(tle->entry),"button_press_event");
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
  if(created)
    return;

  Dprintf((" %s",__FUNCTION__));

  /* --- Put up h-box --- */
  gtk_box_pack_end (GTK_BOX (vbox_main), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  created=1;
}

/*  NewProcessor
 *
 */

void StatusBar_Window::NewProcessor(GUI_Processor *_gp, MemoryAccess *_ma)
{


  if(!_gp  || !_gp->cpu || !_ma || !hbox)
    return;

  gp = _gp;
  ma = _ma;
  Dprintf((" %s",__FUNCTION__));

  if(cpu_cycles == NULL) {
    cpu_cycles = new CyclesLabeledEntry();
    cpu_cycles->Create(hbox,"Cycles:", 18,false);
  }
  if(time == NULL) {
    TimeLabeledEntry *tle = new TimeLabeledEntry();
    time = tle;
    time->Create(hbox,"Time:", 22,false);
    /* create popupmenu */
    tle->build_menu();
    gtk_signal_connect(GTK_OBJECT(time->entry),
		      "button_press_event",
		      (GtkSignalFunc) do_popup,
		      tle);

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
  }

  list<Register *>::iterator iReg;
  entries.clear();

  for(iReg = ma->SpecialRegisters.begin();
      iReg != ma->SpecialRegisters.end();
      ++iReg) {

    //cout << " Adding " << ((*iReg)->showType()) << " to status bar\n";
    entries.push_back(hbox, *iReg);
  }


  Update();

}

StatusBar_Window::StatusBar_Window(void)
{
  gp = 0;
  ma = 0;

  cpu_cycles = 0;
  time = 0;
  created = false;

  time = NULL;
  cpu_cycles = NULL;

  /* --- Create h-box for holding the status line --- */
  hbox = gtk_hbox_new (FALSE, 0);

}
#endif

#endif // HAVE_GUI
