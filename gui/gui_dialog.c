#include <stdio.h>
#include <stdlib.h>

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

/*
 * GtkDialog
 */

static GtkWidget *dialog_window = NULL;

static void enter_callback(GtkWidget *entry, char *label)
{
  gchar *entry_text;
  entry_text = gtk_entry_get_text(GTK_ENTRY(entry));
  printf("label %s, entry contents: %s\n", label, entry_text);

  
  
}

void create_labeled_boxes(GtkWidget *box, char **labels, int num_labels)
{
	int i;
	GtkWidget *label;
	GtkWidget *entry;

	/* Create the labels and entry boxes associated with them: */

	for(i=0; i<num_labels; i++) {

		label = gtk_label_new (labels[i]);
		gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
		gtk_widget_set_usize (label, 0, 15);
		gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
		gtk_widget_show (label);

		entry = gtk_entry_new ();
		gtk_signal_connect(GTK_OBJECT(entry), "activate",
				   GTK_SIGNAL_FUNC(enter_callback),
				   entry);
		gtk_entry_set_text (GTK_ENTRY (entry), "0x18");

		gtk_widget_set_usize (entry,
				      gdk_string_width (entry->style->font, "9999")+6,
				      -1);
		gtk_box_pack_start (GTK_BOX (box), entry, FALSE, FALSE, 0);
		gtk_widget_show (entry);

	}
}



/*
static void
label_toggle (GtkWidget  *widget,
	      GtkWidget **label)
{
  if (!(*label))
    {
      *label = gtk_label_new ("Dialog Test");
      gtk_signal_connect (GTK_OBJECT (*label),
			  "destroy",
			  GTK_SIGNAL_FUNC (gtk_widget_destroyed),
			  label);
      gtk_misc_set_padding (GTK_MISC (*label), 10, 10);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->vbox), 
			  *label, TRUE, TRUE, 0);
      gtk_widget_show (*label);
    }
  else
    gtk_widget_destroy (*label);
}
*/

static void
cancel_dialog (GtkWidget  *widget,
	      GtkWidget **data)
{
  printf("cancelling\n");
    gtk_widget_destroy (dialog_window);

}

void
fill_range (void)
{
  GtkWidget *hbox;
  static GtkWidget *label;
  char *labels[] = {
	  "Start Address: ",
	  "   End Address: ",
	  "   Fill Value: "
  };
  int num_labels = sizeof(labels) / sizeof(labels[0]);

  GtkWidget *button;

  if (!dialog_window)
    {
      dialog_window = gtk_dialog_new ();

      gtk_signal_connect (GTK_OBJECT (dialog_window), "destroy",
			  GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			  &dialog_window);

      gtk_window_set_title (GTK_WINDOW (dialog_window), "Fill Range");
      gtk_container_set_border_width (GTK_CONTAINER (dialog_window), 0);
      gtk_widget_set_usize (dialog_window, 400, 110);

      hbox = gtk_hbox_new (FALSE, 0);

      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->vbox), hbox, TRUE, TRUE, 0);
      gtk_widget_show (hbox);

      create_labeled_boxes( hbox, labels,  num_labels);

      button = gtk_button_new_with_label ("OK");
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  GTK_SIGNAL_FUNC (cancel_dialog),
			  &label);
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->action_area), 
			  button, TRUE, TRUE, 0);
      gtk_widget_grab_default (button);
      gtk_widget_show (button);

      button = gtk_button_new_with_label ("Cancel");
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  GTK_SIGNAL_FUNC (cancel_dialog),
			  &label);
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_window)->action_area),
			  button, TRUE, TRUE, 0);
      gtk_widget_show (button);

      label = NULL;
    }

  if (!GTK_WIDGET_VISIBLE (dialog_window))
    gtk_widget_show (dialog_window);
  else
    gtk_widget_destroy (dialog_window);
}

#endif
