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


  gp = 0;

  wc = WC_misc;
  wt = WT_INVALID;
  bIsBuilt = false;
  window = 0;
  menu = 0;

  x=0; y=0;
  width = 100;
  height = 100;
}

GUI_Object::~GUI_Object(void)
{

}

int GUI_Object::Create(GUI_Processor *_gp)
{
  printf("GUI_Object::Create  !!! \n");

  return 0;
}

char *GUI_Object::name(void)
{
  static char p[128];
  size_t len = string::npos;
  if(len > (1+sizeof(p)))
    len = sizeof(p)-1;

  p[name_str.copy(p,len)] = 0;

  return p;
}

void GUI_Object::set_name(const char *new_name)
{
  if(new_name)
    name_str = string(new_name);
  else
    name_str = string("no_name");

}

void GUI_Object::set_name(string &new_name)
{
  name_str = new_name;
}

void GUI_Object::UpdateMenuItem(void)
{
  GtkWidget *menu_item;

  if(menu) {

    menu_item = gtk_item_factory_get_item (item_factory,menu);

    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),enabled);
  } else {
    //printf("GUI_Object::UpdateMenuItem(void) -- 0 menu\n");
    ;
  }


}
void GUI_Object::Update(void)
{
  printf("GUI_Object::Update - shouldn't be called\n");
}

void GUI_Object::ChangeView (int view_state)
{
  if( (view_state==VIEW_SHOW) || (window==0) ||
      ((view_state==VIEW_TOGGLE) &&
       !GTK_WIDGET_VISIBLE(GTK_WIDGET(window)) )
      ) {

    if(!bIsBuilt) {

      if(!get_config()) {
        printf("warning %s\n",__FUNCTION__);
        set_default_config();
      }

      enabled=1;

      Build();

    } else {
      // hmm, this call shouldn't be necessary, but I (Scott) found that
      // in GTK+ 2.2.1 under Linux that it is.
      gtk_widget_set_uposition(GTK_WIDGET(window),x,y);
      gtk_widget_show(window);

      enabled=1;

      // Update the config database
      set_config();

    }

  }
  else if (GTK_WIDGET_VISIBLE(GTK_WIDGET(window))) {

    enabled=0;

    // Update the config database
    set_config();

    gtk_widget_hide(window);
  }

  // Update menu item
  UpdateMenuItem();
}


void GUI_Object::Build(void)
{
  cout << " GUI_Object::Build() should not be called\n";
}

int GUI_Object::get_config(void)
{
  char *pName = name();

  if(!pName)
    return 0;
  /*
  if(!config_get_variable(pName, "x", &x))
    printf("get_config %s failed\n",pName);
  */

  if(!config_get_variable(pName, "enabled", &enabled))
    enabled=0;
  if(!config_get_variable(pName, "x", &x))
    x=10;
  if(!config_get_variable(pName, "y", &y))
    y=10;
  if(!config_get_variable(pName, "width", &width))
    width=300;
  if(!config_get_variable(pName, "height", &height))
    height=100;

  check();

  return 1;
}


void GUI_Object::check(void)
{
#define MAX_REASONABLE   2000

  if((x+width < 0 || x > MAX_REASONABLE) ||
     (y+height < 0 || y > MAX_REASONABLE) ||
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

  printf("set_default_config\n");
  return 1;
}


int GUI_Object::set_config(void)
{
  check();

  char *pName = name();

  if(!pName)
    return 0;

  if(window) {
    gdk_window_get_root_origin(window->window,&x,&y);
    gdk_window_get_size(window->window,&width,&height);
  }

  config_set_variable(pName, "enabled", ((enabled) ? 1 : 0) );
  config_set_variable(pName, "x", x);
  config_set_variable(pName, "y", y);
  config_set_variable(pName, "width", width);
  config_set_variable(pName, "height", height);
  return 1;
}


//------------------------------------------------------------------------
// Helper functions for setting and retrieving variables stored in
// gpsim configuration file.

int config_set_string(const char *module, const char *entry, const char *str)
{
  return settings->set(module, entry, str);
}

int config_set_variable(const char *module, const char *entry, int value)
{
  return settings->set(module, entry, value);
}

int config_get_variable(const char *module, const char *entry, int *value)
{
  return settings->get(module, entry, value);
}

int config_get_string(const char *module, const char *entry, char **str)
{
  return settings->get(module, entry, str);
}

int config_remove(const char *module, const char *entry)
{
  return settings->remove(module, entry);
}

#endif  // HAVE_GUI
