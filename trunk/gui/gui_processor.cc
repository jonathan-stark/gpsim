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
//#include <gtkextra/gtksheetentry.h>

#include "gui.h"
#include "gui_callbacks.h"
/*

void gp_add_window_to_list(GUI_Processor *gp, GUI_Object *go)
{

  if(!gp) return;

  switch(go->wc)
    {
    case WC_misc:
      gp->misc_windows = g_list_append(gp->misc_windows, (gpointer)go);
      break;

    case WC_source:
      gp->source_windows = g_list_append(gp->source_windows, (gpointer)go);
      break;

    case WC_data:
      gp->data_windows = g_list_append(gp->data_windows, (gpointer)go);
      break;

    default:
      g_warning("bad window type in gp_add_window_to_list");
    }


}
  */

GUI_Processor *new_GUI_Processor(void)
{

  GUI_Processor *gp;

  gp = (GUI_Processor *)malloc(sizeof(GUI_Processor));
  memset(gp, 0, sizeof(GUI_Processor));
  gp->source_windows = g_list_alloc();
  gp->data_windows = g_list_alloc();
  gp->misc_windows = g_list_alloc();
  gp->pic_id = 0;

  return gp;
}

GUI_Processor::GUI_Processor(void)
{

  source_windows = g_list_alloc();
  data_windows = g_list_alloc();
  misc_windows = g_list_alloc();

  regwin_ram = NULL;
  regwin_eeprom = NULL;
  status_bar = NULL;
  program_memory = NULL;
  source_browser = NULL;
  symbol_window = NULL;
  watch_window = NULL;
  stack_window = NULL;
  breadboard_window = NULL;
  trace_window = NULL;
  profile_window = NULL;
  stopwatch_window = NULL;

  pic_id = 0;

}



void GUI_Processor::add_window_to_list(GUI_Object *go)
{


  switch(go->wc)
    {
    case WC_misc:
      if(misc_windows)
	misc_windows = g_list_append(misc_windows, (gpointer)go);
      break;

    case WC_source:
      if(source_windows)
	source_windows = g_list_append(source_windows, (gpointer)go);
      break;

    case WC_data:
      if(data_windows)
	data_windows = g_list_append(data_windows, (gpointer)go);
      break;

    default:
      g_warning("bad window type in gp_add_window_to_list");
    }


}

#endif // HAVE_GUI
