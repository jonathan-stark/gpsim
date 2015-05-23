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

#include "../config.h"
#ifdef HAVE_GUI

#include <gtk/gtk.h>
#include <glib.h>

#include "gui_object.h"
#include "settings.h"

extern GtkUIManager *ui;

//------------------------------------------------------------------------
// Helper functions for setting and retrieving variables stored in
// gpsim configuration file.

extern Settings *settings;

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

GUI_Object::GUI_Object()
  : gp(0), window(0), menu(0), x(0), y(0), width(100), height(100),
    enabled(0), bIsBuilt(false)
{
}

GUI_Object::~GUI_Object()
{

}

void GUI_Object::UpdateMenuItem()
{
  if (menu) {
    GtkAction *menu_item = gtk_ui_manager_get_action(ui, menu);
    gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(menu_item), enabled);
  }
}

void GUI_Object::ChangeView(gboolean view_state)
{
  if (view_state) {

    if(!bIsBuilt) {

      if(!get_config()) {
        g_print("warning %s\n",__FUNCTION__);
        set_default_config();
      }

      enabled = TRUE;

      Build();

    } else {
      // hmm, this call shouldn't be necessary, but I (Scott) found that
      // in GTK+ 2.2.1 under Linux that it is.
      gtk_window_move(GTK_WINDOW(window), x, y);
      gtk_widget_show(window);

      enabled = TRUE;

      // Update the config database
      set_config();

    }

  } else if (window && gtk_widget_get_visible(window)) {

    enabled = FALSE;

    // Update the config database
    set_config();

    gtk_widget_hide(window);
  }

  // Update menu item
  UpdateMenuItem();
}

int GUI_Object::get_config()
{
  const char *pName = name();

  if(!pName)
    return 0;

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


void GUI_Object::check()
{
#define MAX_REASONABLE   2000

  if((x+width < 0 || x > MAX_REASONABLE) ||
     (y+height < 0 || y > MAX_REASONABLE) ||
     (width < 0 || width > MAX_REASONABLE) ||
     (height < 0 || height > MAX_REASONABLE) )

    set_default_config();

}

int GUI_Object::set_default_config()
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


int GUI_Object::set_config()
{
  check();

  const char *pName = name();

  if(!pName)
    return 0;

  if(window) {
    gtk_window_get_position(GTK_WINDOW(window), &x, &y);
    gtk_window_get_size(GTK_WINDOW(window), &width, &height);
  }

  config_set_variable(pName, "enabled", ((enabled) ? 1 : 0) );
  config_set_variable(pName, "x", x);
  config_set_variable(pName, "y", y);
  config_set_variable(pName, "width", width);
  config_set_variable(pName, "height", height);
  return 1;
}

#endif  // HAVE_GUI
