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

#include <unistd.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <assert.h>
#include <gtkextra/gtksheet.h>
#include "../src/interface.h"
#include "../src/gpsim_def.h"
#include "../src/modules.h"
#include "../src/processor.h"
#include "../src/pic-processor.h"

#define SBAW_NRFILES 20 // Max number of source files
//#define MAX_BREAKPOINTS 32

//------------------------------------------------------------
//
// Create structures to generically access the pic-processor
//

//
// Here's a list of all the types of windows that are supported
//

enum window_types {
  WT_INVALID = 0,
  WT_opcode_source_window=1,
  WT_asm_source_window,
  WT_register_window,
  WT_eeprom_window,
  WT_watch_window,
  WT_symbol_window,
  WT_breadboard_window,
  WT_stack_window,
  WT_trace_window,
  WT_profile_window,
  WT_stopwatch_window,
  WT_scope_window,
  WT_status_bar,
  WT_sfr_window,
  WT_list_source_window
};

//
// Here's a list of all the categories of windows that are supported
//
enum window_category {
  WC_misc,
  WC_source,
  WC_data
};

//
// This structure will cross reference the data in the simulator
// to its gui representation. There are cases when the same data
// appears in more than one place (e.g. the status register is in
// both the status bar and register windows). gpsim accomodates this
// with a singly-linked list. In other words, for each data element
// that is presented graphically there's a pointer within the simulator
// to reference it. The simulator keeps a linked listed of pointers
// to all instances of these graphical representations

class CrossReferenceToGUI {
public:
  enum window_types parent_window_type;
  gpointer     parent_window;
  gpointer     data;
  virtual void Update(int new_value);
  virtual void Remove(void);
};


#include "gui_object.h"
#include "gui_processor.h"

//
// External references and function prototypes
//

extern GtkItemFactory *item_factory;
// gui_processor.c
extern GUI_Processor *gp;

void exit_gpsim(void);

void update_menu_item(GUI_Object *_this);


void SourceBrowser_update_line(struct cross_reference_to_gui *xref, int new_value);


// Configuration -- records window states.

int config_get_variable(char *module, char *entry, int *value);
int config_set_variable(char *module, char *entry, int value);
int config_get_string(char *module, char *entry, char **string);
int config_set_string(char *module, char *entry, const char *string);

gint gui_object_configure_event(GtkWidget *widget, GdkEventConfigure *e, GUI_Object *go);


void ProfileWindow_notify_start_callback(Profile_Window *pw);
void ProfileWindow_notify_stop_callback(Profile_Window *pw);
int gui_get_value(char *prompt);


#endif // __GUI_H__

#endif // HAVE_GUI
