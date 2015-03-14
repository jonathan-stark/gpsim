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

#include <cstdio>
#include <cstdlib>
#include <cerrno>

#include "../config.h"
#ifdef HAVE_GUI

#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <cstring>

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
  unsigned long value;
  char *bad_position;

  if(!gpGuiProcessor || !gpGuiProcessor->cpu || !le || !le->entry)
    return;

  text=gtk_entry_get_text (GTK_ENTRY (le->entry));
    
  value = strtoul(text, &bad_position, 16);
  if (*bad_position != '\0')
    return;  /* string contains an invalid number */

  le->put_value(value);
}

//========================================================================
EntryWidget::EntryWidget()
  : entry(0)
{
}

void EntryWidget::SetEntryWidth(int string_width)
{
  if(entry)
    gtk_entry_set_width_chars(GTK_ENTRY (entry), string_width);
}

void EntryWidget::Create(bool isEditable)
{
  entry = gtk_entry_new();
  if (!isEditable)
    gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
  gtk_widget_show(entry);
}

//========================================================================


LabeledEntry::LabeledEntry()
  : label(0)
{
}

void LabeledEntry::Create(GtkWidget *box,
			  const char *clabel, 
			  int string_width,
			  bool isEditable=true)
{
  label = gtk_label_new (clabel);
    
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  gtk_widget_set_size_request (label, 0, 15);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  EntryWidget::Create(isEditable);

  gtk_entry_set_text (GTK_ENTRY (entry), "----");

  SetEntryWidth(string_width);

  gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 0);
}

void LabeledEntry::Update()
{
}

void LabeledEntry::put_value(unsigned int new_value)
{

}

void LabeledEntry::NewLabel(const char *clabel)
{
  if (label)
    gtk_label_set_text(GTK_LABEL(label), clabel);
}
//------------------------------------------------------------------------
RegisterLabeledEntry::RegisterLabeledEntry(GtkWidget *box,
					   Register *new_reg,
					   bool isEditable=true) 
 : LabeledEntry()
{
  reg = new_reg;

  if(reg) {
    g_snprintf(pCellFormat, sizeof(pCellFormat), "0x%%0%dx",reg->register_size()*2);

    label = (GtkWidget *)gtk_label_new (reg->name().c_str());
    
    gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
    gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
    gtk_widget_show (label);

    entry = gtk_entry_new ();
    SetEntryWidth(2 + reg->register_size()*2);
    Update();

    gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 0);

    gtk_widget_show (entry);

    if (!isEditable)
      gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);

    g_signal_connect(entry, "activate",
		       G_CALLBACK(LabeledEntry_callback),
		       this);
  }
}

void RegisterLabeledEntry::put_value(unsigned int new_value)
{
  if(reg)
    reg->put_value(new_value);
}

void RegisterLabeledEntry::Update()
{
  char buffer[32];

  if (reg) {

    unsigned int value = reg->get_value();

    g_snprintf(buffer, sizeof(buffer), pCellFormat, value);

    gtk_entry_set_text (GTK_ENTRY (entry), buffer);

  }
}
void RegisterLabeledEntry::AssignRegister(Register *new_reg)
{
  reg = new_reg;

  if(reg) {
    g_snprintf(pCellFormat, sizeof(pCellFormat), "0x%%0%dx", reg->register_size()*2);

    NewLabel(reg->name().c_str());
    SetEntryWidth(2 + reg->register_size()*2);
  }
}

//------------------------------------------------------------------------
void StatusBar_Window::Update()
{

  if (!created)
      return;

  //update the displayed values

  if(!gp || !gp->cpu)
    return;


  std::vector<RegisterLabeledEntry *>::iterator iRLE;

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

  created = true;


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
    cross_reference->parent_window = (gpointer) this;
    cross_reference->data = (gpointer) this;
  
    pPC->add_xref((gpointer) cross_reference);

  }

  Update();

}

StatusBar_Window::StatusBar_Window()
  : gp(0), cpu_cycles(0), time(0), ma(0), created(false)
{
  /* --- Create h-box for holding the status line --- */
  hbox = gtk_hbox_new (FALSE, 0);
}

#endif // HAVE_GUI
