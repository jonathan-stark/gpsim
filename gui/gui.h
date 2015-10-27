/*
   Copyright (C) 1998,1999,2000,2001,2002,2003,2004
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

#ifndef __GUI_H__
#define __GUI_H__

#include "../config.h"

#ifdef HAVE_GUI

#include <cstdio>
#include <glib.h>
#include <gtk/gtk.h>
#include "../src/processor.h"
#include "settings.h"

#define SBAW_NRFILES 100 // Max number of source files

//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

#define GTKWAIT { while(gtk_events_pending()) gtk_main_iteration(); }

//------------------------------------------------------------
//
// Create structures to generically access the pic-processor
//

//
// This structure will cross reference the data in the simulator
// to its gui representation. There are cases when the same data
// appears in more than one place (e.g. the status register is in
// both the status bar and register windows). gpsim accomodates this
// with a singly-linked list. In other words, for each data element
// that is presented graphically there's a pointer within the simulator
// to reference it. The simulator keeps a linked listed of pointers
// to all instances of these graphical representations

class CrossReferenceToGUI : public XrefObject
{
public:
  gpointer     parent_window;

  CrossReferenceToGUI();
  ~CrossReferenceToGUI();

  virtual void Update(int new_value) = 0;
  virtual void Remove();
};

#include "gui_object.h"
#include "gui_processor.h"

//========================================================================
class EntryWidget
{
public:
  EntryWidget();
  virtual ~EntryWidget();

  virtual void Update()=0;
  virtual void set_editable(bool Editable = true);
  void SetEntryWidth(int string_width);

  GtkWidget *entry;
};

//========================================================================
//
// A LabeledEntry is an object consisting of gtk entry
// widget that is labeled (with a gtk lable widget)
//

class LabeledEntry : public EntryWidget {
public:
  LabeledEntry(GtkWidget *box, const char *clabel);

  virtual ~LabeledEntry()
  {
  }

  virtual void Update();
  virtual void put_value(unsigned int);

private:
  GtkWidget *label;
};

class RegisterLabeledEntry : public LabeledEntry {
public:
  RegisterLabeledEntry(GtkWidget *, Register *, bool);
  virtual ~RegisterLabeledEntry();

  virtual void put_value(unsigned int);
  virtual void Update();
private:
  Register *reg;
  char pCellFormat[10];
};

//------------------------------------------------------------------------
// The KeyEvent class is used for key mapping (e.g. mapping the 's'
// key to step)

class KeyEvent
{
public:
  virtual ~KeyEvent()
  {
  }

  virtual void press(gpointer data = 0) {};
  virtual void release(gpointer data = 0) {};
};

//
// External references and function prototypes
//

extern GtkUIManager *ui;
extern GUI_Processor *gpGuiProcessor;

void exit_gpsim(void);

void update_menu_item(GUI_Object *_this);


void SourceBrowser_update_line(struct cross_reference_to_gui *xref, int new_value);


// Configuration -- records window states.
extern Settings *settings;

int config_get_variable(const char *module, const char *entry, int *value);
int config_set_variable(const char *module, const char *entry, int value);
int config_get_string(const char *module, const char *entry, char **string);
int config_set_string(const char *module, const char *entry, const char *string);
int config_remove(const char *module, const char *entry);

gboolean gui_object_configure_event(GtkWidget *widget, GdkEventConfigure *e, GUI_Object *go);


void ProfileWindow_notify_start_callback(Profile_Window *pw);
void ProfileWindow_notify_stop_callback(Profile_Window *pw);
int gui_get_value(const char *prompt);

#endif // __GUI_H__

#endif // HAVE_GUI
