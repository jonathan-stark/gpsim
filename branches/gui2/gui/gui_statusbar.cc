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


#define DEBUG

#include "gui.h"
#include "gui_statusbar.h"
#include "../src/processor.h"

extern GtkWidget *StatusBarExperiment;

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

  if(!gp || !gp->cpu || !le || !le->entry)
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
    gtk_widget_set_usize (entry,
			  string_width * 
			  gdk_string_width (gtk_style_get_font(entry->style), "9") + 6,
			  -1);
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
					   bool isEditable=true) 
 : LabeledEntry()
{
  reg = new_reg;

  if(reg) {
    printf("RegisterLabeledEntry\n");
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

  if( !created)
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
  Dprintf((" %s",__FUNCTION__));

  /* --- Put up h-box --- */
  //gtk_box_pack_end (GTK_BOX (vbox_main), hbox, FALSE, FALSE, 0);
  gtk_widget_show (StatusBarExperiment);

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
  Dprintf((" %s",__FUNCTION__));

  list<Register *>::iterator iReg;

  for(iReg = ma->SpecialRegisters.begin();
      iReg != ma->SpecialRegisters.end();
      ++iReg) {

    entries.push_back(new RegisterLabeledEntry(StatusBarExperiment, *iReg));
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
  ma = 0;

  cpu_cycles = 0;
  time = 0;
  created = false;

  /* --- Create h-box for holding the status line --- */
  //hbox = gtk_hbox_new (FALSE, 0);

  hbox = StatusBarExperiment;

}

#endif // HAVE_GUI
