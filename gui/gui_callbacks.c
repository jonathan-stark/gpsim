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
