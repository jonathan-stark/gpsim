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

#include <assert.h>

static gint
key_press(GtkWidget *widget,
	  GdkEventKey *key, 
	  gpointer data)
{
    int low_level_step=0;

  SourceBrowser_Window *sbw = (SourceBrowser_Window *) data;

  if(!sbw) return(FALSE);
  if(!sbw->gp) return(FALSE);
  if(!sbw->gp->pic_id) return(FALSE);

  // fix this
  if(sbw->wt == WT_opcode_source_window)
  {
      SourceBrowserOpcode_Window *sbow = (SourceBrowserOpcode_Window*)sbw;

      if(gtk_notebook_get_current_page(GTK_NOTEBOOK(sbow->notebook)))
	  return FALSE;

      low_level_step=1;
  }
      
  switch(key->keyval) {

  case 's':  // Single Step
  case 'S':
  case GDK_F7:
      //sbw->gui_obj.gp->p->step(1);
      if(gpsim_get_hll_mode(sbw->gp->pic_id)
	&&!low_level_step)
      	gpsim_hll_step(sbw->gp->pic_id);
      else
      	gpsim_step(sbw->gp->pic_id, 1);
      break;

  case 'o':  // Step Over Next instruction, or hll statement
  case 'O':
  case 'n':
  case GDK_F8:
      if(gpsim_get_hll_mode(sbw->gp->pic_id)
	&&!low_level_step)
      	gpsim_hll_step_over(sbw->gp->pic_id);
      else
      	gpsim_step_over(sbw->gp->pic_id);
      break;
  case 'r':
  case 'R':
  case GDK_F9:
      gpsim_run(sbw->gp->pic_id);
      break;
  case GDK_Escape:
      gpsim_stop(sbw->gp->pic_id);
      break;
  case 'f':
  case 'F':
      gpsim_finish(sbw->gp->pic_id);
      break;
      
// Exit is Ctrl-Q; the dispatcher menu shortcut
//  case 'q':
//  case 'Q':
//      exit_gpsim();
  }

  return TRUE;
}

static int delete_event(GtkWidget *widget,
			GdkEvent  *event,
                        SourceBrowser_Window *sbw)
{
    SourceBrowser_change_view((GUI_Object *)sbw,VIEW_HIDE);
    return TRUE;
}

void SourceBrowser_update_line(struct cross_reference_to_gui *xref, int new_value)
{
  GUI_Processor *gp;
  int address;

  gp = (GUI_Processor *)xref->parent_window;
  if(!gp) { printf("gp == null\n"); return;}

  assert(xref && xref->data);

  address = *(int *)xref->data;

  if(gp->source_browser)
      SourceBrowserAsm_update_line( (SourceBrowserAsm_Window*)gp->source_browser,  address);

  if(gp->program_memory)
      SourceBrowserOpcode_update_line( (SourceBrowserOpcode_Window*)gp->program_memory,  address, address);

}

void SourceBrowser_Window::SetPC(int address)
{
  printf("%s shouldn't be called \n",__FUNCTION__);

}
void SourceBrowser_Window::Update(void)
{

  SetPC(gpsim_get_pc_value(gp->pic_id));
}


void SourceBrowser_Window::Create(void)
{


  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_window_set_default_size(GTK_WINDOW(window), width,height);
  gtk_widget_set_uposition(GTK_WIDGET(window),x,y);
  gtk_window_set_wmclass(GTK_WINDOW(window),name,"Gpsim");

  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC(delete_event),
		      (gpointer) this);

  /* Add a signal handler for key press events. This will capture
   * key commands for single stepping, running, etc.
   */
  gtk_signal_connect(GTK_OBJECT(window),"key_press_event",
		     (GtkSignalFunc) key_press,
		     (gpointer) this);


  
  gtk_container_set_border_width (GTK_CONTAINER (window), 0);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_widget_show(vbox);
  gtk_container_add (GTK_CONTAINER (window), vbox);

}

void SourceBrowser_Window::NewProcessor(GUI_Processor *gp)
{
  printf("%s shouldn't be called \n",__FUNCTION__);
}
void SourceBrowser_Window::SelectAddress(int address)
{
  printf("%s shouldn't be called \n",__FUNCTION__);
}

gint gui_object_configure_event(GtkWidget *widget, GdkEventConfigure *e, GUI_Object *go)
{
//    struct gui_config_winattr winattr;

    if(widget->window==NULL)
	return 0;
    
    gdk_window_get_root_origin(widget->window,&go->x,&go->y);
    gdk_window_get_size(widget->window,&go->width,&go->height);
    
    go->set_config();
    
    return 0; // what should be returned?, FIXME
}

void update_menu_item(struct GUI_Object *_this)
{
    GtkWidget *menu_item;
    switch(_this->wt)
    {
    case WT_register_window:
      _this->UpdateMenuItem();
      break;
    case WT_symbol_window:
	menu_item = gtk_item_factory_get_item (item_factory,"<main>/Windows/Symbols");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),_this->enabled);
	break;
    case WT_asm_source_window:
      _this->UpdateMenuItem();

      //menu_item = gtk_item_factory_get_item (item_factory,"<main>/Windows/Source");
      //gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),_this->enabled);
      break;
    case WT_opcode_source_window:
      _this->UpdateMenuItem();
      //menu_item = gtk_item_factory_get_item (item_factory,"<main>/Windows/Program memory");
      //gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),_this->enabled);
	break;
    case WT_watch_window:
	menu_item = gtk_item_factory_get_item (item_factory,"<main>/Windows/Watch");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),_this->enabled);
	break;
    case WT_breadboard_window:
	menu_item = gtk_item_factory_get_item (item_factory,"<main>/Windows/Breadboard");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),_this->enabled);
	break;
    case WT_stack_window:
	menu_item = gtk_item_factory_get_item (item_factory,"<main>/Windows/Stack");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),_this->enabled);
	break;
    case WT_trace_window:
	menu_item = gtk_item_factory_get_item (item_factory,"<main>/Windows/Trace");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),_this->enabled);
	break;
    case WT_profile_window:
	menu_item = gtk_item_factory_get_item (item_factory,"<main>/Windows/Profile");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),_this->enabled);
	break;
    case WT_stopwatch_window:
	menu_item = gtk_item_factory_get_item (item_factory,"<main>/Windows/Stopwatch");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),_this->enabled);
	break;
    default:
	puts("update_menu_item(): unhandled case");
	break;
    }
}

void SourceBrowser_change_view (GUI_Object *_this, int view_state)
{
    
    if( (view_state==VIEW_SHOW) || (_this->window==NULL) ||
	((view_state==VIEW_TOGGLE) &&
	 !GTK_WIDGET_VISIBLE(GTK_WIDGET(_this->window)) )
      )
    {
      if(!_this->is_built)
      {
	if(!_this->get_config()) {
	  printf("warning %s\n",__FUNCTION__);
	  _this->set_default_config();
	}
	  switch(_this->wt)
	  {
	  case WT_register_window:
	      _this->Build();
	      break;
	  case WT_symbol_window:
	    _this->Build();
	    //BuildSymbolWindow((Symbol_Window*)_this);
	      break;
	  case WT_asm_source_window:
	      _this->Build();
	      break;
	  case WT_opcode_source_window:
	      _this->Build();
	      break;
	  case WT_watch_window:
	    //BuildWatchWindow((Watch_Window*)_this);
	    _this->Build();
	      break;
	  case WT_breadboard_window:
	      _this->enabled=1;
	      BuildBreadboardWindow((Breadboard_Window*)_this);
	      break;
	  case WT_stack_window:
	    _this->Build();
	    //BuildStackWindow((Stack_Window*)_this);
	      break;
	  case WT_trace_window:
	    _this->Build();
	    //BuildTraceWindow((Trace_Window*)_this);
	      break;
	  case WT_profile_window:
	      BuildProfileWindow((Profile_Window*)_this);
	      break;
	  case WT_stopwatch_window:
	    _this->Build();
	    //BuildStopWatchWindow((StopWatch_Window*)_this);
	      break;
	  default:
	      puts("SourceBrowser_change_view(): unhandled case");
	      break;
	  }
      }
      else
      {
	  _this->enabled=1;
	  gtk_widget_show(_this->window);
      }

    }
  else if (GTK_WIDGET_VISIBLE(GTK_WIDGET(_this->window)))
  {
      _this->enabled=0;
      gtk_widget_hide(_this->window);
  }

    // we update config database
    _this->set_config();

    // Update menu item
    update_menu_item(_this);
}
#endif // HAVE_GUI
