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


#include "gui.h"
#include "gui_callbacks.h"

GUI_Object::GUI_Object(void)
{

  printf("GUI Object constructor\n");

  gp = NULL;
  window = NULL;
  name = NULL;
  menu = NULL;
  change_view = NULL;

  x=0; y=0;
  width = 100;
  height = 100;
}

int GUI_Object::Create(GUI_Processor *_gp)
{
  printf("GUI_Object::Create  !!! \n");

}
void GUI_Object::UpdateMenuItem(void)
{
  GtkWidget *menu_item;

  if(menu) {
  printf("%s getting menu item%s\n",__FUNCTION__,menu);

    menu_item = gtk_item_factory_get_item (item_factory,menu);

    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),enabled);
  } else {
    printf("GUI_Object::UpdateMenuItem(void) -- NULL menu\n");
  }


}

void GUI_Object::ChangeView (int view_state)
{

  printf("%s 1\n",__FUNCTION__);
  if(change_view) {
    change_view(this, view_state);
    return;
  }
    
  printf("%s 2\n",__FUNCTION__);

  if( (view_state==VIEW_SHOW) || (window==NULL) ||
      ((view_state==VIEW_TOGGLE) &&
       !GTK_WIDGET_VISIBLE(GTK_WIDGET(window)) )
      )
    {
      if(!is_built)
	{
	  if(!get_config()) {
	    printf("warning %s\n",__FUNCTION__);
	    set_default_config();
	  }
	  switch(wt)
	    {
	    case WT_register_window:
	      Build();
	      break;
	    case WT_symbol_window:
	      Build();
	      break;
	    case WT_asm_source_window:
	      Build();
	      break;
	    case WT_opcode_source_window:
	      Build();
	      break;
	    case WT_watch_window:
	      Build();
	      break;
	    case WT_breadboard_window:
	      enabled=1;
	      BuildBreadboardWindow((Breadboard_Window*)this);
	      break;
	    case WT_stack_window:
	      Build();
	      break;
	    case WT_trace_window:
	      Build();
	      break;
	    case WT_profile_window:
	      BuildProfileWindow((Profile_Window*)this);
	      break;
	    case WT_stopwatch_window:
	      Build();
	      break;
	    default:
	      puts("SourceBrowser_change_view(): unhandled case");
	      break;
	    }
	}
      else
	{
	  enabled=1;
	  gtk_widget_show(window);
	}

    }
  else if (GTK_WIDGET_VISIBLE(GTK_WIDGET(window)))
    {
  printf("%s 4\n",__FUNCTION__);
      enabled=0;
      gtk_widget_hide(window);
    }

  printf("%s 5\n",__FUNCTION__);

  // we update config database
  set_config();

  // Update menu item
  update_menu_item(this);
}


void GUI_Object::Build(void)
{

}

int GUI_Object::get_config(void)
{
  if(!name)
    return 0;

  if(!config_get_variable(name, "enabled", &enabled))
    enabled=0;
  if(!config_get_variable(name, "x", &x))
    x=10;
  if(!config_get_variable(name, "y", &y))
    y=10;
  if(!config_get_variable(name, "width", &width))
    width=300;
  if(!config_get_variable(name, "height", &height))
    height=100;

  printf("get_config: enabled:%d x:%d y:%d w:%d h:%d\n",enabled,x,y,width,height);
  check();

  return 1;
}


void GUI_Object::check(void)
{
#define MAX_REASONABLE   2000

  if((x < 0 || x > MAX_REASONABLE) ||
     (y < 0 || y > MAX_REASONABLE) ||
     (width < 0 || width > MAX_REASONABLE) ||
     (height < 0 || height > MAX_REASONABLE) )

    set_default_config();

}

int GUI_Object::set_default_config(void)
{
  static int defaultX = 100;
  static int defaultY = 100;

  enabled = 0;
  x = defaultX;
  y = defaultY;
  defaultX += 100;
  defaultY += 100;

  width = 100;
  height = 100;

  return 1;
}


int GUI_Object::set_config(void)
{
  check();

  config_set_variable(name, "enabled", ((enabled) ? 1 : 0) );
  config_set_variable(name, "x", x);
  config_set_variable(name, "y", y);
  config_set_variable(name, "width", width);
  config_set_variable(name, "height", height);
  return 1;
}



#endif  // HAVE_GUI
