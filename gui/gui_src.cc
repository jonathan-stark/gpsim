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
#include <typeinfo>

#include "../config.h"
#ifdef HAVE_GUI

#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <string.h>

#include <gtkextra/gtkbordercombo.h>
#include <gtkextra/gtkcolorcombo.h>
#include <gtkextra/gtksheet.h>

#include "gui.h"
#include "gui_src.h"

#include <assert.h>
#include <map>


class StepEvent : public KeyEvent
{
public:
  void press(gpointer data)
  {
    SourceBrowser_Window *sbw = (SourceBrowser_Window *) data;
    if(sbw && sbw->pma)
        sbw->pma->step(1);
  }
  void release(gpointer data) {}
};

class StepOverEvent : public KeyEvent
{
public:
  void press(gpointer data)
  {
    SourceBrowser_Window *sbw = (SourceBrowser_Window *) data;
    if(sbw && sbw->pma)
      // Step Over Next instruction, or hll statement
      sbw->pma->step_over();
  }
  void release(gpointer data) {}
};

class RunEvent : public KeyEvent
{
public:
  void press(gpointer data)
  {
    /*
    SourceBrowser_Window *sbw = (SourceBrowser_Window *) data;
    if(sbw && sbw->pma)
      sbw->pma->run();
    */
    get_interface().start_simulation();
  }
  void release(gpointer data) {}
};

class StopEvent : public KeyEvent
{
public:
  void press(gpointer data)
  {
    SourceBrowser_Window *sbw = (SourceBrowser_Window *) data;
    if(sbw && sbw->pma)
      sbw->pma->stop();
  }
  void release(gpointer data) {}
};

class FinishEvent : public KeyEvent
{
public:
  void press(gpointer data)
  {
    SourceBrowser_Window *sbw = (SourceBrowser_Window *) data;
    if(sbw && sbw->pma)
      sbw->pma->finish();
  }
  void release(gpointer data) {}
};

static map<guint, KeyEvent *> KeyMap;


static gint
key_press(GtkWidget *widget,
          GdkEventKey *key,
          gpointer data)
{
  int low_level_step=0;

  SourceBrowser_Window *sbw = (SourceBrowser_Window *) data;

  if(!sbw) return(FALSE);
  if(!sbw->gp) return(FALSE);
  if(!sbw->gp->cpu) return(FALSE);

  // fix this
  if(sbw->wt == WT_opcode_source_window)
  {
      SourceBrowserOpcode_Window *sbow = (SourceBrowserOpcode_Window*)sbw;

      if(gtk_notebook_get_current_page(GTK_NOTEBOOK(sbow->notebook)))
          return FALSE;

      low_level_step=1;
  }

  KeyEvent *pKE = KeyMap[key->keyval];
  if(pKE)
    {
      pKE->press(data);
      return TRUE;
    }

  return FALSE;
}

static int delete_event(GtkWidget *widget,
                        GdkEvent  *event,
                        SourceBrowser_Window *sbw)
{
    sbw->ChangeView(VIEW_HIDE);
    return TRUE;
}


void SourceBrowser_Window::SetPC(int address)
{
  printf("%s shouldn't be called \n",__FUNCTION__);

}
void SourceBrowser_Window::UpdateLine(int address)
{
  printf("%s shouldn't be called \n",__FUNCTION__);

}
void SourceBrowser_Window::Update(void)
{
  if(!gp || !gp->cpu)
    return;
  if (gp->cpu->simulation_mode == eSM_RUNNING ||
    gp->cpu->simulation_mode == eSM_SINGLE_STEPPING)
    return;

  SetPC(gp->cpu->pma->get_PC());
}


void SourceBrowser_Window::Create(void)
{
  last_simulation_mode = eSM_INITIAL;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  gtk_window_set_default_size(GTK_WINDOW(window), width,height);
  gtk_widget_set_uposition(GTK_WIDGET(window),x,y);
  gtk_window_set_wmclass(GTK_WINDOW(window),name(),"Gpsim");

  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
                      GTK_SIGNAL_FUNC(delete_event),
                      (gpointer) this);

  // FIXME - populate the KeyMap map with source browser functions. This should
  // FIXME   probably go into some kind of configuration file.

  KeyMap['s'] = new StepEvent();
  KeyMap['S'] = KeyMap['s'];
  KeyMap[GDK_F7] = KeyMap['s'];

  KeyMap['o'] = new StepOverEvent();
  KeyMap['O'] = KeyMap['o'];
  KeyMap['n'] = KeyMap['o'];
  KeyMap[GDK_F8] = KeyMap['o'];

  KeyMap['r'] = new RunEvent();
  KeyMap['R'] = KeyMap['r'];
  KeyMap[GDK_F9] = KeyMap['r'];

  KeyMap[GDK_Escape] = new StopEvent();

  KeyMap['f'] = new FinishEvent();
  KeyMap['F'] = KeyMap['f'];

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

void SourceBrowser_Window::SetTitle() {
  char buffer[256];

  if (gp->cpu == NULL || pma == NULL) {
      return;
  }

  if (last_simulation_mode != eSM_INITIAL &&
    ((last_simulation_mode == eSM_RUNNING &&
    gp->cpu->simulation_mode == eSM_RUNNING) ||
    (last_simulation_mode != eSM_RUNNING &&
    gp->cpu->simulation_mode != eSM_RUNNING)) &&
    sLastPmaName == pma->name()) {
      return;
  }
  last_simulation_mode = gp->cpu->simulation_mode;
  const char * sStatus;
  if (gp->cpu->simulation_mode == eSM_RUNNING)
    sStatus = "Run";
  else // if (gp->cpu->simulation_mode == eSM_STOPPED)
    sStatus = "Stopped";
  sprintf(buffer,"Source Browser: [%s] %s", sStatus, pma != NULL ?
    pma->name().c_str() : "" );
  sLastPmaName = pma->name();
  gtk_window_set_title (GTK_WINDOW (window), buffer);
}

void SourceBrowser_Window::NewProcessor(GUI_Processor *gp)
{
  printf("%s shouldn't be called \n",__FUNCTION__);
}
void SourceBrowser_Window::SelectAddress(int address)
{
  printf("%s shouldn't be called \n",__FUNCTION__);
}
void SourceBrowser_Window::SelectAddress(Value *addrSym)
{

  if(typeid(*addrSym) == typeid(LineNumberSymbol) ||
     typeid(*addrSym) == typeid(AddressSymbol)) {

    int i;
    addrSym->get(i);
    SelectAddress(i);
  }

}


gint gui_object_configure_event(GtkWidget *widget, GdkEventConfigure *e, GUI_Object *go)
{
//    struct gui_config_winattr winattr;

    if(widget->window==0)
        return 0;

    gdk_window_get_root_origin(widget->window,&go->x,&go->y);
    gdk_window_get_size(widget->window,&go->width,&go->height);

    go->set_config();

    return 0; // what should be returned?, FIXME
}

#endif // HAVE_GUI
