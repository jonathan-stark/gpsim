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

#undef TRUE
#undef FALSE

extern "C" {
#include "../eXdbm/eXdbm.h"
}
extern DB_ID dbid;


GUI_Object::GUI_Object(void)
{


  gp = 0;

  window = 0;
  name = 0;
  menu = 0;

  x=0; y=0;
  width = 100;
  height = 100;
}

int GUI_Object::Create(GUI_Processor *_gp)
{
  printf("GUI_Object::Create  !!! \n");

  return 0;
}
void GUI_Object::UpdateMenuItem(void)
{
  GtkWidget *menu_item;

  if(menu) {

    menu_item = gtk_item_factory_get_item (item_factory,menu);

    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),enabled);
  } else {
    printf("GUI_Object::UpdateMenuItem(void) -- 0 menu\n");
  }


}

void GUI_Object::ChangeView (int view_state)
{

  if( (view_state==VIEW_SHOW) || (window==0) ||
      ((view_state==VIEW_TOGGLE) &&
       !GTK_WIDGET_VISIBLE(GTK_WIDGET(window)) )
      ) {
    
    if(!is_built) {
	
      if(!get_config()) {
	printf("warning %s\n",__FUNCTION__);
	set_default_config();
      }

      Build();
    } else {
     
      enabled=1;
      gtk_widget_show(window);
    }

  }
  else if (GTK_WIDGET_VISIBLE(GTK_WIDGET(window)))
    {
      enabled=0;
      gtk_widget_hide(window);
    }



  // Update the config database
  set_config();

  // Update menu item
  UpdateMenuItem();
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

  //printf("get_config: enabled:%d x:%d y:%d w:%d h:%d\n",enabled,x,y,width,height);
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



//------------------------------------------------------------------------
// Helper functions for setting and retrieving variables stored in
// gpsim configuration file.

int config_set_string(char *module, char *entry, const char *string)
{
    int ret;
    DB_LIST list;

    list = eXdbmGetList(dbid, 0, module);
    if(list==0)
    {
	ret = eXdbmCreateList(dbid, 0, module, 0);
	if(ret==-1)
	{
	    puts(eXdbmGetErrorString(eXdbmGetLastError()));
	    return 0;
	}
	
	list = eXdbmGetList(dbid, 0, module);
	if(list==0)
	{
	    puts(eXdbmGetErrorString(eXdbmGetLastError()));
	    return 0;
	}
    }

    // We have the list
    
    ret = eXdbmChangeVarString(dbid, list, entry, (char *)string);
    if(ret == -1)
    {
	ret = eXdbmCreateVarString(dbid, list, entry, 0, (char *)string);
	if(ret==-1)
	{
	    puts("\n\n\n\ndidn't work");
	    puts(eXdbmGetErrorString(eXdbmGetLastError()));
	    puts("\n\n\n\n");
	    return 0;
	}
    }
    ret=eXdbmUpdateDatabase(dbid);
    if(ret==-1)
    {
	puts(eXdbmGetErrorString(eXdbmGetLastError()));
	return 0;
    }
    return 1;
}

int config_set_variable(char *module, char *entry, int value)
{
    int ret;
    DB_LIST list;

    list = eXdbmGetList(dbid, 0, module);
    if(list==0)
    {
	ret = eXdbmCreateList(dbid, 0, module, 0);
	if(ret==-1)
	{
	    puts(eXdbmGetErrorString(eXdbmGetLastError()));
	    return 0;
	}
	
	list = eXdbmGetList(dbid, 0, module);
	if(list==0)
	{
	    puts(eXdbmGetErrorString(eXdbmGetLastError()));
	    return 0;
	}
    }

    // We have the list
    
    ret = eXdbmChangeVarInt(dbid, list, entry, value);
    if(ret == -1)
    {
	ret = eXdbmCreateVarInt(dbid, list, entry, 0, value);
	if(ret==-1)
	{
	    puts("\n\n\n\ndidn't work");
	    puts(eXdbmGetErrorString(eXdbmGetLastError()));
	    puts("\n\n\n\n");
	    return 0;
	}
    }
    ret=eXdbmUpdateDatabase(dbid);
    if(ret==-1)
    {
	puts(eXdbmGetErrorString(eXdbmGetLastError()));
	return 0;
    }
    return 1;
}

int config_get_variable(char *module, char *entry, int *value)
{
    int ret;
    DB_LIST list;

    list = eXdbmGetList(dbid, 0, module);
    if(list==0)
	return 0;

    // We have the list
    
    ret = eXdbmGetVarInt(dbid, list, entry, value);
    if(ret == -1)
	return 0;
    
    return 1;
}

int config_get_string(char *module, char *entry, char **string)
{
    int ret;
    DB_LIST list;

    list = eXdbmGetList(dbid, 0, module);
    if(list==0)
	return 0;

    // We have the list
    
    ret = eXdbmGetVarString(dbid, list, entry, string);
    if(ret == -1)
	return 0;
    
    return 1;
}



#endif  // HAVE_GUI
