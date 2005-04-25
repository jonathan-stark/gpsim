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


/* gui_callbacks.cc
 *
 * callback functions for the register viewer
 */
#include "../config.h"
#ifdef HAVE_GUI

#include <stdio.h>
#include <gtk/gtk.h>



/*---------------------------------------------------*
 *
 * gui_cb_quit - 'quit' callback
 * 
 *---------------------------------------------------*/

void gui_cb_quit (GtkWidget *widget, gpointer data)
{

  gtk_main_quit ();

}



/*---------------------------------------------------*
 *
 * gui_cb_break - 'break' callback
 * 
 *---------------------------------------------------*/

void gui_cb_break (GtkWidget *widget, gpointer data)
{

  printf("break\n");

}

/*---------------------------------------------------*
 *
 * gui_cb_break_r - 'break_r' callback for break on read
 * 
 *---------------------------------------------------*/

void gui_cb_break_r (GtkWidget *widget, gpointer data)
{

  printf("break on read\n");

}

/*---------------------------------------------------*
 *
 * gui_cb_break_w - 'break_w' callback for break on write
 * 
 *---------------------------------------------------*/

void gui_cb_break_w (GtkWidget *widget, gpointer data)
{

  printf("break on write\n");

}
#endif // HAVE_GUI
