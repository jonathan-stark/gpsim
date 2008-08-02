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

#include <assert.h>

#include "../src/interface.h"
#include "../src/trace.h"

#include "gui.h"
#include "gui_trace.h"

#define MAXTRACES  100
#define TRACE_COLUMNS    2

typedef enum {
    MENU_BREAK_CLEAR,
    MENU_BREAK_READ,
    MENU_BREAK_WRITE,
    MENU_BREAK_READ_VALUE,
    MENU_BREAK_WRITE_VALUE,
    MENU_ADD_WATCH,
} menu_id;

static gchar *trace_titles[TRACE_COLUMNS]={"Cycle", "Trace"};

// gui trace flags:
#define GTF_ENABLE_XREF_UPDATES    (1<<0)

guint64 row_to_cycle[MAXTRACES];


static GtkStyle *normal_style;

//struct TraceMapping trace_map[MAXTRACES];


//========================================================================

class TraceXREF : public CrossReferenceToGUI
{
public:

  /*****************************************************************
   * Update
   *
   * This is called by the simulator when it has been determined that
   * that the trace buffer has changed and needs to be updated
   */

  void Update(int new_value)
  {

#define TRACE_STRING 100

    GtkCList *clist;

    char cycle_string[TRACE_STRING];
    char trace_string[TRACE_STRING];
    char *entry[TRACE_COLUMNS]={cycle_string,trace_string};

    Trace_Window *tw  = (Trace_Window *) (parent_window);

    if(!tw  || !tw->enabled)
      return;

    if(!tw->gp || !tw->gp->cpu)
      {
        puts("Warning gp or gp->cpu == NULL in TraceWindow_update");
        return;
      }

    // If we're not allowing xref updates then exit
    if( !(tw->trace_flags & GTF_ENABLE_XREF_UPDATES))
      return;

    strncpy(trace_string,get_trace().string_buffer,TRACE_STRING);

    if(trace_string[0] && (get_trace().string_cycle>=tw->last_cycle)) {
      tw->last_cycle = get_trace().string_cycle;
      tw->trace_map[tw->trace_map_index].cycle = get_trace().string_cycle;
      tw->trace_map[tw->trace_map_index].simulation_trace_index = get_trace().string_index;

      // Advance the trace_map_index using rollover arithmetic
      if(++tw->trace_map_index >= MAXTRACES)
        tw->trace_map_index = 0;

      clist=GTK_CLIST(tw->trace_clist);

      sprintf(cycle_string,"0x%016" PRINTF_GINT64_MODIFIER "x", get_trace().string_cycle);

      gtk_clist_append  (clist, entry);

      if(clist->rows>MAXTRACES)
        gtk_clist_remove(clist,0);

    }

  }

};

//========================================================================

/*****************************************************************
 * TraceWindow_update
 *
 * The purpose of this routine is to refresh the trace window with
 * the latest trace information. The current pic simulation cycle (should
 * this be change to real time???) is examined and compared to what
 * is currently displayed in the trace window. If the info in the
 * trace window is really old, this the entire window is deleted and
 * the trace is redrawn with the latest. If the trace window is rather
 * recent then the older trace info is deleted and the new is appended
 * to the end.
 *
 * INPUTS: *tw a pointer to a Trace_Window structure.
 */

void Trace_Window::Update(void)
{

  //guint64 cycle;

  if(!enabled)
    return;

  if(!gp || !gp->cpu)
  {
      puts("Warning gp or gp->cpu == NULL in TraceWindow_update");
      return;
  }

  // Get a convenient pointer to the gtk_clist that the trace is in.
  trace_clist=GTK_CLIST(trace_clist);

  gtk_clist_freeze(trace_clist);

  trace_flags |= GTF_ENABLE_XREF_UPDATES;
  if(get_cycles().get()-last_cycle>=MAXTRACES)
    // redraw the whole thing
    get_trace().dump(MAXTRACES, 0);
  else
    get_trace().dump(get_cycles().get()-last_cycle, 0);


  trace_flags &= ~GTF_ENABLE_XREF_UPDATES;
  last_cycle = get_cycles().get();
  gtk_clist_thaw(trace_clist);

}


/*****************************************************************
 * TraceWindow_new_processor
 *
 *
 */

void Trace_Window::NewProcessor(GUI_Processor *_gp)
{

#define NAME_SIZE 32

  TraceXREF *cross_reference;

  if(!gp)
    return;

  if(!enabled)
    return;

  cross_reference = new TraceXREF();
  cross_reference->parent_window_type =  WT_trace_window;
  cross_reference->parent_window = (gpointer) this;
  cross_reference->data = 0;
  if(get_trace().xref)
    get_trace().xref->_add((gpointer) cross_reference);

}

static int delete_event(GtkWidget *widget,
                        GdkEvent  *event,
                        Trace_Window *rw)
{
  rw->ChangeView(VIEW_HIDE);
  return TRUE;
}

void Trace_Window::Build(void)
{
  if(bIsBuilt)
    return;
  GtkWidget *main_vbox;
  GtkWidget *scrolled_window;

  gint i;
  gint column_width,char_width;

  window=gtk_window_new(GTK_WINDOW_TOPLEVEL);

  main_vbox=gtk_vbox_new(FALSE,1);
  gtk_container_set_border_width(GTK_CONTAINER(main_vbox),0);
  gtk_container_add(GTK_CONTAINER(window), main_vbox);
  gtk_widget_show(main_vbox);

  gtk_window_set_title(GTK_WINDOW(window), "trace viewer");

  // Trace clist
  trace_clist=GTK_CLIST(gtk_clist_new_with_titles(TRACE_COLUMNS,trace_titles));
  gtk_clist_set_column_auto_resize(trace_clist,0,TRUE);

  GTK_WIDGET_UNSET_FLAGS(trace_clist,GTK_CAN_DEFAULT);

  gtk_window_set_default_size(GTK_WINDOW(window), width,height);
  gtk_widget_set_uposition(GTK_WIDGET(window),x,y);
  gtk_window_set_wmclass(GTK_WINDOW(window),name(),"Gpsim");


  gtk_signal_connect(GTK_OBJECT (window), "delete_event",
                     GTK_SIGNAL_FUNC(delete_event), this);

  scrolled_window=gtk_scrolled_window_new(0, 0);

  gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(trace_clist));

  gtk_widget_show(GTK_WIDGET(trace_clist));
  gtk_widget_show(scrolled_window);

  gtk_box_pack_start(GTK_BOX(main_vbox), scrolled_window, TRUE, TRUE, 0);
  ///////////////////////////////////////////////////
  ///////////////////////////////////////////////////



  normal_style = gtk_style_new ();
#if GTK_MAJOR_VERSION >= 2
  char_width = gdk_string_width(gtk_style_get_font(normal_style), "9");
#else
  char_width = gdk_string_width(normal_style->font, "9");
#endif
  column_width = 3 * char_width + 6;

  gtk_signal_connect_after(GTK_OBJECT(window), "configure_event",
                           GTK_SIGNAL_FUNC(gui_object_configure_event),this);



  gtk_widget_show (window);

  if(!trace_map) {
    trace_map = (struct TraceMapping *)malloc(MAXTRACES * sizeof(struct TraceMapping));

    for(i=0; i<MAXTRACES; i++) {
      trace_map[i].cycle = 0;
      trace_map[i].simulation_trace_index = 0;
    }
    trace_map_index = 0;
  }

  enabled=1;
  bIsBuilt = true;
  last_cycle = 0;

  NewProcessor(gp);

  Update();
  UpdateMenuItem();

}

//------------------------------------------------------------------------
//
//
//


Trace_Window::Trace_Window(GUI_Processor *_gp)
{

  menu = "<main>/Windows/Trace";

  gp = _gp;
  set_name("trace");
  window = 0;
  wc = WC_data;
  wt = WT_trace_window;
  trace_map = 0;

  trace_flags = 0;

  get_config();

  if(enabled)
      Build();
}

#endif // HAVE_GUI
