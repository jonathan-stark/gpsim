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

void StatusBar_update(StatusBar_Window *sbw)
{
  char buffer[20];
  unsigned int pic_id;

  pic_id = sbw->gp->pic_id;

  if( !sbw->created)
      return;
  
  //update the displayed values

  sbw->status->value.i32 = gpsim_get_status(pic_id);
  sprintf(buffer,"0x%02x",sbw->status->value.i32);
  gtk_entry_set_text (GTK_ENTRY (sbw->status->entry), buffer);


  sbw->W->value.i32 = gpsim_get_w(pic_id);
  sprintf(buffer,"0x%02x",sbw->W->value.i32);
  gtk_entry_set_text (GTK_ENTRY (sbw->W->entry), buffer);

  sbw->pc->value.i32 = gpsim_get_pc_value(pic_id);
  sprintf(buffer,"0x%04x",sbw->pc->value.i32);
  gtk_entry_set_text (GTK_ENTRY (sbw->pc->entry), buffer);

  sbw->cycles->value.ui64 = gpsim_get_cycles(pic_id);
  sprintf(buffer,"0x%016Lx",sbw->cycles->value.ui64);
  gtk_entry_set_text (GTK_ENTRY (sbw->cycles->entry), buffer);

}

static void w_callback(GtkWidget *entry, StatusBar_Window *sbw)
{
    char *text;
    unsigned int value;
    unsigned int pic_id;
    char *bad_position;

    pic_id = sbw->gp->pic_id;
    text=gtk_entry_get_text (GTK_ENTRY (sbw->W->entry));
    
    value = strtoul(text, &bad_position, 16);
    if( strlen(bad_position) )
	return;  /* string contains an invalid number */

    gpsim_put_w(pic_id, value);

    StatusBar_update(sbw);

    return;
}

static void status_callback(GtkWidget *entry, StatusBar_Window *sbw)
{
    char *text;
    unsigned int value;
    unsigned int pic_id;
    char *bad_position;

    pic_id = sbw->gp->pic_id;
    text=gtk_entry_get_text (GTK_ENTRY (sbw->status->entry));
    
    value = strtoul(text, &bad_position, 16);
    if( strlen(bad_position) )
	return;  /* string contains an invalid number */
    
    gpsim_put_status(pic_id, value);
    
    StatusBar_update(sbw);

    return;
}

static void pc_callback(GtkWidget *entry, StatusBar_Window *sbw)
{
    char *text;
    unsigned int value;
    unsigned int pic_id;
    char *bad_position;

    pic_id = sbw->gp->pic_id;
    text=gtk_entry_get_text (GTK_ENTRY (sbw->pc->entry));
    
    value = strtoul(text, &bad_position, 16);
    if( strlen(bad_position) )
	return;  /* string contains an invalid number */

    gpsim_put_pc_value(pic_id, value);
    
    StatusBar_update(sbw);

    return;
}



/*
 * create_labeled_entry
 */

labeled_entry *create_labeled_entry(GtkWidget *box,char *label, int string_width)
{

  labeled_entry *le;


  le = (labeled_entry *)malloc(sizeof(labeled_entry));

  le->label = gtk_label_new (label);

  gtk_misc_set_alignment (GTK_MISC (le->label), 1.0, 0.5);
  gtk_widget_set_usize (le->label, 0, 15);
  gtk_box_pack_start (GTK_BOX (box), le->label, FALSE, FALSE, 0);
  gtk_widget_show (le->label);

  le->entry = gtk_entry_new ();
//  gtk_signal_connect(GTK_OBJECT(le->entry), "activate",
//		     GTK_SIGNAL_FUNC(enter_callback),
//		     label);
  gtk_entry_set_text (GTK_ENTRY (le->entry), "----");

  le->value.i32 = 0;

  gtk_widget_set_usize (le->entry,
			string_width * gdk_string_width (le->entry->style->font, "9") + 6,
			-1);
  gtk_box_pack_start (GTK_BOX (box), le->entry, FALSE, FALSE, 0);
  gtk_widget_show (le->entry);

  return(le);
}
/*
 * CreateStatusBar
 *
 * Create the status bar at the bottom of the window
 * 
 *
 * vbox_main - The box to which we will append.
 *
 */ 

void StatusBar_create(GtkWidget *vbox_main, StatusBar_Window *sbw)
{
  GtkWidget *hbox;

  /* --- Create h-box for holding the status line --- */
  hbox = gtk_hbox_new (FALSE, 0);

  /* --- Put up h-box --- */
  gtk_box_pack_end (GTK_BOX (vbox_main), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  sbw->status = create_labeled_entry(hbox,"Status:", 4);
  sbw->status->parent = sbw;
  gtk_signal_connect(GTK_OBJECT(sbw->status->entry), "activate",
		     GTK_SIGNAL_FUNC(status_callback),
		     sbw);

  sbw->W = create_labeled_entry(hbox,"W:", 4);
  sbw->W->parent = sbw;
  gtk_signal_connect(GTK_OBJECT(sbw->W->entry), "activate",
		     GTK_SIGNAL_FUNC(w_callback),
		     sbw);

  sbw->pc = create_labeled_entry(hbox,"PC:", 6);
  sbw->pc->parent = sbw;
  gtk_signal_connect(GTK_OBJECT(sbw->pc->entry), "activate",
		     GTK_SIGNAL_FUNC(pc_callback),
		     sbw);

  sbw->cycles = create_labeled_entry(hbox,"Cycles:", 18);
  sbw->cycles->parent = sbw;
  gtk_entry_set_editable(GTK_ENTRY(sbw->cycles->entry),0);

  sbw->created=1;
  
}

void StatusBar_update_xref(struct cross_reference_to_gui *xref, int new_value)
{
  StatusBar_Window *sbw;

  sbw  = (StatusBar_Window *) (xref->parent_window);
  StatusBar_update(sbw);
}

void StatusBar_new_processor(StatusBar_Window *sbw, GUI_Processor *gp)
{

  struct cross_reference_to_gui  *cross_reference;

  if(sbw == NULL || gp == NULL)
    return;

  sbw->gp = gp;
  gp->status_bar = sbw;


  /* Now create a cross-reference link that the simulator can use to
   * send information back to the gui
   */

  cross_reference = (struct cross_reference_to_gui *) malloc(sizeof(struct cross_reference_to_gui));
  cross_reference->parent_window_type =   WT_status_bar;
  cross_reference->parent_window = (gpointer) sbw;
  cross_reference->data = (gpointer) sbw;
  cross_reference->update = StatusBar_update_xref;
  cross_reference->remove = NULL;
  
  gpsim_assign_pc_xref(sbw->gp->pic_id, (gpointer) cross_reference);

  StatusBar_update(sbw);

}
#endif // HAVE_GUI
