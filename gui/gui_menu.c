#include "../config.h"
#ifdef HAVE_GUI

#ifdef DOING_GNOME
#include <gnome.h>
#endif

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
#include "gui_callbacks.h"

static void
create_notebook (void);

typedef struct _note_book_item
{
  gchar        *name;
  GtkSignalFunc func;
} NotebookItem;

GtkItemFactory *item_factory;

static void 
do_quit_app(GtkWidget *widget) 
{
	exit_gpsim();
}


#ifdef DOING_GNOME
static void
about_cb (GtkWidget *widget, void *data)
{
	GtkWidget *about;
	const gchar *authors[] = {
		"Scott Dattalo - scott@dattalo.com",
		"Ralf Forsberg - rfg@home.se",
		NULL
	};
	
	about = gnome_about_new ( "The GNUPIC Simulator - ", GPSIM_VERSION,
				  /* copyright notice */
				  "(C) 1999 ",
				  authors,
				  "A simulator for Microchip PIC microcontrollers.",
				  NULL);
	gtk_widget_show (about);
	
	return;
}
#endif

static void
show_message (char *title, char *message)
{
  GtkWidget *window;
  GtkWidget *label;
  GtkWidget *button;

  window = gtk_dialog_new ();

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroyed), &window);

  gtk_window_set_title (GTK_WINDOW (window), title);
  gtk_container_set_border_width (GTK_CONTAINER (window), 0);


  button = gtk_button_new_with_label ("close");
  gtk_container_set_border_width (GTK_CONTAINER (button), 10);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     GTK_SIGNAL_FUNC(gtk_widget_destroy),
			     GTK_OBJECT (window));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default(button);
  gtk_widget_show(button);


  label = gtk_label_new (message);
  gtk_misc_set_padding (GTK_MISC (label), 10, 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), label, TRUE, TRUE, 0);

  gtk_widget_show (label);
  gtk_widget_show (window);

  gtk_grab_add (window);

}

static void 
about_cb (gpointer             callback_data,
	  guint                callback_action,
	  GtkWidget           *widget)
{
  gchar *version = "The GNUPIC Simulator - " GPSIM_VERSION;
  show_message(  version, "A simulator for Microchip PIC microcontrollers.
by T. Scott Dattalo - mailto:scott@dattalo.com
   Ralf Forsberg - mailto:rfg@home.se
\ngpsim homepage: http://www.dattalo.com/gnupic/gpsim.html
");

}

/*
static void
entry_toggle_editable (GtkWidget *checkbutton,
		       GtkWidget *entry)
{
   gtk_entry_set_editable(GTK_ENTRY(entry),
			  GTK_TOGGLE_BUTTON(checkbutton)->active);
}

static void
entry_toggle_sensitive (GtkWidget *checkbutton,
			GtkWidget *entry)
{
   gtk_widget_set_sensitive (entry, GTK_TOGGLE_BUTTON(checkbutton)->active);
}

static void
entry_toggle_visibility (GtkWidget *checkbutton,
			GtkWidget *entry)
{
   gtk_entry_set_visibility(GTK_ENTRY(entry),
			 GTK_TOGGLE_BUTTON(checkbutton)->active);
}

*/

//--------------------------------------------------------------
// new_processor_dialog
//
// This will display the 'new processor' dialog window.
//
//

static GtkWidget *new_processor_name_entry;

/*
static void
new_processor_dialog (void)
{
  static GtkWidget *window = NULL;

  GtkWidget *box1;
  GtkWidget *box2;
  GtkWidget *cb;
  GtkWidget *button;
  GtkWidget *separator;
  GList *cbitems = NULL;

  if (!window)
    {
      //      for(i=3; i<number_of_available_processors; i++)
      //	cbitems = g_list_append(cbitems, available_processors[i].names[2]);


      window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

      gtk_signal_connect (GTK_OBJECT (window), "destroy",
			  GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			  &window);

      gtk_window_set_title (GTK_WINDOW (window), "New Processor (not supported)");
      gtk_container_set_border_width (GTK_CONTAINER (window), 0);


      box1 = gtk_vbox_new (FALSE, 0);
      gtk_container_add (GTK_CONTAINER (window), box1);
      gtk_widget_show (box1);


      box2 = gtk_vbox_new (FALSE, 10);
      gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
      gtk_box_pack_start (GTK_BOX (box1), box2, TRUE, TRUE, 0);
      gtk_widget_show (box2);

      cb = gtk_combo_new ();
      gtk_combo_set_popdown_strings (GTK_COMBO (cb), cbitems);
      cbitems = g_list_first(cbitems);
      gtk_entry_set_text (GTK_ENTRY (GTK_COMBO(cb)->entry), (gchar *)cbitems->data);
      gtk_editable_select_region (GTK_EDITABLE (GTK_COMBO(cb)->entry),
				  0, -1);
      gtk_box_pack_start (GTK_BOX (box2), cb, TRUE, TRUE, 0);
      gtk_widget_show (cb);

      new_processor_name_entry = gtk_entry_new ();
      gtk_entry_set_text (GTK_ENTRY (new_processor_name_entry), "no_name");
      gtk_editable_select_region (GTK_EDITABLE (new_processor_name_entry), 0, 5);
      gtk_box_pack_start (GTK_BOX (box2), new_processor_name_entry, TRUE, TRUE, 0);
      gtk_widget_show (new_processor_name_entry);


      separator = gtk_hseparator_new ();
      gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);
      gtk_widget_show (separator);


      box2 = gtk_hbox_new (FALSE, 10);
      gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
      gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, TRUE, 0);
      gtk_widget_show (box2);


      button = gtk_button_new_with_label ("NOT");
      gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
				 GTK_SIGNAL_FUNC(gtk_widget_destroy),
				 GTK_OBJECT (window));
      gtk_box_pack_start (GTK_BOX (box2), button, TRUE, TRUE, 0);
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_widget_grab_default (button);
      gtk_widget_show (button);

      button = gtk_button_new_with_label ("YET");
      gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
				 GTK_SIGNAL_FUNC(gtk_widget_destroy),
				 GTK_OBJECT (window));
      gtk_box_pack_start (GTK_BOX (box2), button, TRUE, TRUE, 0);
      gtk_widget_show (button);
    }

  if (!GTK_WIDGET_VISIBLE (window))
    gtk_widget_show (window);
  else
    gtk_widget_destroy (window);
}
*/

void
file_selection_hide_fileops (GtkWidget *widget,
			     GtkFileSelection *fs)
{
  gtk_file_selection_hide_fileop_buttons (fs);
}

extern int gui_message(char *message);

void
file_selection_ok (GtkWidget        *w,
		   GtkFileSelection *fs)
{
    unsigned int pic_id;
    char *file;
    char msg[200];
    if(gp)
    {
	pic_id=gp->pic_id;
	file=gtk_file_selection_get_filename (fs);
	if(!gpsim_open(pic_id, file))
	{
	    sprintf(msg,"Open failedCould not open \"%s\"",file);
	    gui_message(msg);
	}
    }
    gtk_widget_destroy (GTK_WIDGET (fs));
}

static GtkItemFactoryCallback 
fileopen_dialog(gpointer             callback_data,
	      guint                callback_action,
	      GtkWidget           *widget)
{
  static GtkWidget *window = NULL;
  GtkWidget *button;

  if (!window)
    {
      window = gtk_file_selection_new ("file selection dialog");

      gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (window));

      gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);

      gtk_signal_connect (GTK_OBJECT (window), "destroy",
			  GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			  &window);

      gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (window)->ok_button),
			  "clicked", GTK_SIGNAL_FUNC(file_selection_ok),
			  window);
      gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (window)->cancel_button),
				 "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy),
				 GTK_OBJECT (window));
      
      button = gtk_button_new_with_label ("Hide Fileops");
      gtk_signal_connect (GTK_OBJECT (button), "clicked",
			  (GtkSignalFunc) file_selection_hide_fileops, 
			  (gpointer) window);
      gtk_box_pack_start (GTK_BOX (GTK_FILE_SELECTION (window)->action_area), 
			  button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      button = gtk_button_new_with_label ("Show Fileops");
      gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
				 (GtkSignalFunc) gtk_file_selection_show_fileop_buttons, 
				 (gpointer) window);
      gtk_box_pack_start (GTK_BOX (GTK_FILE_SELECTION (window)->action_area), 
			  button, FALSE, FALSE, 0);
      gtk_widget_show (button);
    }
    gtk_widget_show (window);
}




// Menuhandler for Windows menu buttons
static GtkItemFactoryCallback 
toggle_window (gpointer             callback_data,
	      guint                callback_action,
	      GtkWidget           *widget)
{
    GUI_Object *gui_object;
    GtkWidget *menu_item;

//    g_message ("\"%s\" is not supported yet.", gtk_item_factory_path_from_widget (widget));

    menu_item = gtk_item_factory_get_item (item_factory,
					   gtk_item_factory_path_from_widget (widget));
    if(gp)
    {
	switch(callback_action)
	{
	case 1:
	    gui_object=(GUI_Object*)gp->program_memory;
	    break;
	case 2:
	    gui_object=(GUI_Object*)gp->source_browser;
	    break;
	case 3:
	    gui_object=(GUI_Object*)gp->regwin_ram;
	    break;
	case 4:
	    gui_object=(GUI_Object*)gp->regwin_eeprom;
	    break;
	case 5:
	    gui_object=(GUI_Object*)gp->watch_window;
	    break;
	case 6:
	    gui_object=(GUI_Object*)gp->symbol_window;
	    break;
	case 7:
	    gui_object=(GUI_Object*)gp->breadboard_window;
            break;
	default:
	    puts("unknown menu action");
	}
	
	if(gui_object!=NULL)
	{
	    if(GTK_CHECK_MENU_ITEM(menu_item)->active)
		gui_object->change_view(gui_object,VIEW_SHOW);
	    else
		gui_object->change_view(gui_object,VIEW_HIDE);
	}

//	printf("%s\n",gtk_widget_get_name(widget));
    }
}

static GtkItemFactoryCallback
gtk_ifactory_cb (gpointer             callback_data,
		 guint                callback_action,
		 GtkWidget           *widget)
{
  g_message ("\"%s\" is not supported yet.", gtk_item_factory_path_from_widget (widget));

}

void set_toggle_menu_states()
{
    GUI_Object *gui_object;
    GtkWidget *menu_item;
    menu_item = gtk_item_factory_get_item (item_factory,"<main>/Windows/Disassembly");
    gui_object=(GUI_Object*)gp->program_memory;
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),gui_object->enabled);
    menu_item = gtk_item_factory_get_item (item_factory,"<main>/Windows/Source");
    gui_object=(GUI_Object*)gp->source_browser;
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),gui_object->enabled);
    menu_item = gtk_item_factory_get_item (item_factory,"<main>/Windows/Ram");
    gui_object=(GUI_Object*)gp->regwin_ram;
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),gui_object->enabled);
    menu_item = gtk_item_factory_get_item (item_factory,"<main>/Windows/EEPROM");
    gui_object=(GUI_Object*)gp->regwin_eeprom;
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),gui_object->enabled);
    menu_item = gtk_item_factory_get_item (item_factory,"<main>/Windows/Watch");
    gui_object=(GUI_Object*)gp->watch_window;
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),gui_object->enabled);
    menu_item = gtk_item_factory_get_item (item_factory,"<main>/Windows/Symbols");
    gui_object=(GUI_Object*)gp->symbol_window;
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),gui_object->enabled);
    menu_item = gtk_item_factory_get_item (item_factory,"<main>/Windows/Breadboard");
    gui_object=(GUI_Object*)gp->breadboard_window;
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),gui_object->enabled);
}

static GtkItemFactoryEntry menu_items[] =
{
  { "/_File",            NULL,         0,                     0, "<Branch>" },
  { "/File/tearoff1",    NULL,         (GtkItemFactoryCallback)gtk_ifactory_cb,       0, "<Tearoff>" },
  { "/File/_New",        "<control>N", (GtkItemFactoryCallback)gtk_ifactory_cb,       0 },
  { "/File/_Open",       "<control>O", (GtkItemFactoryCallback)fileopen_dialog,       0 },
  { "/File/_Save",       "<control>S", (GtkItemFactoryCallback)gtk_ifactory_cb,       0 },
  { "/File/Save _As...", NULL,         (GtkItemFactoryCallback)gtk_ifactory_cb,       0 },
  { "/File/sep1",        NULL,         (GtkItemFactoryCallback)gtk_ifactory_cb,       0, "<Separator>" },
  { "/File/_Quit",       "<control>Q", (GtkItemFactoryCallback)do_quit_app,         0 },

  //  { "/_Processor",     	 NULL,       0,               0, "<Branch>" },
  //  { "/_Processor/New",   NULL,       (GtkItemFactoryCallback)new_processor_dialog,       0 },
  //  { "/_Processor/Delete",NULL,       (GtkItemFactoryCallback)gtk_ifactory_cb,       0 },
  //  { "/_Processor/Switch",NULL,       (GtkItemFactoryCallback)gtk_ifactory_cb,       0 },


  //  { "/_Break",     	 NULL, 0,               0, "<Branch>" },
  //  { "/_Break/Set",       NULL, (GtkItemFactoryCallback)gtk_ifactory_cb,       0 },
  //  { "/_Break/Clear",     NULL, (GtkItemFactoryCallback)gtk_ifactory_cb,       0 },

  { "/_Windows",     NULL, 0,               0, "<Branch>" },
  { "/Windows/Dis_assembly", NULL, (GtkItemFactoryCallback)toggle_window,1,"<ToggleItem>" },
  { "/Windows/_Source", NULL, (GtkItemFactoryCallback)toggle_window,2,"<ToggleItem>" },
  { "/Windows/sep1",   NULL, (GtkItemFactoryCallback)gtk_ifactory_cb,0,"<Separator>"  },
  { "/Windows/_Ram",    NULL, (GtkItemFactoryCallback)toggle_window,3,"<ToggleItem>" },
  { "/Windows/_EEPROM", NULL, (GtkItemFactoryCallback)toggle_window,4,"<ToggleItem>" },
  { "/Windows/_Watch",  NULL, (GtkItemFactoryCallback)toggle_window,5,"<ToggleItem>" },
  { "/Windows/sep2",   NULL, (GtkItemFactoryCallback)gtk_ifactory_cb,0,"<Separator>"  },
  { "/Windows/Symbo_ls",NULL, (GtkItemFactoryCallback)toggle_window,6,"<ToggleItem>" },
  { "/Windows/_Breadboard",NULL, (GtkItemFactoryCallback)toggle_window,7,"<ToggleItem>" },

  { "/_Preferences",          NULL, 0,               0, "<Branch>" },
  { "/_Preferences/Windows",  NULL, (GtkItemFactoryCallback)create_notebook,       0 },


  { "/_Help",            NULL,         0,                     0, "<LastBranch>" },
  { "/Help/_About",      NULL,         (GtkItemFactoryCallback)about_cb,       0 },

};

static int nmenu_items = sizeof (menu_items) / sizeof (menu_items[0]);


GtkWidget *dispatcher_window = NULL;

void create_dispatcher (void)
{
  if (!dispatcher_window)
    {
      GtkWidget *box1;
      GtkWidget *box2;
      GtkWidget *separator;
      GtkWidget *button;
      GtkAccelGroup *accel_group;
      
      int x,y,width,height;
      
      dispatcher_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

      if(!config_get_variable("dispatcher", "x", &x))
	  x=10;
      if(!config_get_variable("dispatcher", "y", &y))
	  y=10;
      if(!config_get_variable("dispatcher", "width", &width))
	  width=1;
      if(!config_get_variable("dispatcher", "height", &height))
	  height=1;
      gtk_window_set_default_size(GTK_WINDOW(dispatcher_window), width,height);
      gtk_widget_set_uposition(GTK_WIDGET(dispatcher_window),x,y);
      
      
      gtk_signal_connect (GTK_OBJECT (dispatcher_window), "destroy",
			  GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			  &dispatcher_window);
      gtk_signal_connect (GTK_OBJECT (dispatcher_window), "delete-event",
			  GTK_SIGNAL_FUNC (gtk_true),
			  NULL);
      
      accel_group = gtk_accel_group_get_default ();
      item_factory = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group);
      gtk_object_set_data_full (GTK_OBJECT (dispatcher_window),
				"<main>",
				item_factory,
				(GtkDestroyNotify) gtk_object_unref);
//      gtk_accel_group_attach (accel_group, GTK_OBJECT (dispatcher_window));
      gtk_item_factory_create_items (item_factory, nmenu_items, menu_items, NULL);
      gtk_window_set_title (GTK_WINDOW (dispatcher_window), GPSIM_VERSION);
      gtk_container_set_border_width (GTK_CONTAINER (dispatcher_window), 0);
      
      box1 = gtk_vbox_new (FALSE, 0);
      gtk_container_add (GTK_CONTAINER (dispatcher_window), box1);
      
      gtk_box_pack_start (GTK_BOX (box1),
			  gtk_item_factory_get_widget (item_factory, "<main>"),
			  FALSE, FALSE, 0);


      separator = gtk_hseparator_new ();
      gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 0);


      box2 = gtk_vbox_new (FALSE, 10);
      gtk_container_set_border_width (GTK_CONTAINER (box2), 10);
      gtk_box_pack_start (GTK_BOX (box1), box2, FALSE, TRUE, 0);

      button = gtk_button_new_with_label ("Quit gpsim");
      /*      gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
				 GTK_SIGNAL_FUNC(gtk_widget_destroy),
				 GTK_OBJECT (dispatcher_window));
      */
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc) do_quit_app, NULL);

      gtk_box_pack_start (GTK_BOX (box2), button, TRUE, TRUE, 0);
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_widget_grab_default (button);

      gtk_widget_show_all (dispatcher_window);
      
      set_toggle_menu_states();
    }
  else
    gtk_widget_destroy (dispatcher_window);
}
/*
static GtkWidget *
build_option_menu (OptionMenuItem items[],
		   gint           num_items,
		   gint           history,
		   gpointer       data)
{
  GtkWidget *omenu;
  GtkWidget *menu;
  GtkWidget *menu_item;
  GSList *group;
  gint i;

  omenu = gtk_option_menu_new ();
      
  menu = gtk_menu_new ();
  group = NULL;
  
  for (i = 0; i < num_items; i++)
    {
      menu_item = gtk_radio_menu_item_new_with_label (group, items[i].name);
      gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
			  (GtkSignalFunc) items[i].func, data);
      group = gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (menu_item));
      gtk_menu_append (GTK_MENU (menu), menu_item);
      if (i == history)
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item), TRUE);
      gtk_widget_show (menu_item);
    }

  gtk_option_menu_set_menu (GTK_OPTION_MENU (omenu), menu);
  gtk_option_menu_set_history (GTK_OPTION_MENU (omenu), history);
  
  return omenu;
}
*/

static char * book_open_xpm[] = {
"16 16 4 1",
"       c None s None",
".      c black",
"X      c #808080",
"o      c white",
"                ",
"  ..            ",
" .Xo.    ...    ",
" .Xoo. ..oo.    ",
" .Xooo.Xooo...  ",
" .Xooo.oooo.X.  ",
" .Xooo.Xooo.X.  ",
" .Xooo.oooo.X.  ",
" .Xooo.Xooo.X.  ",
" .Xooo.oooo.X.  ",
"  .Xoo.Xoo..X.  ",
"   .Xo.o..ooX.  ",
"    .X..XXXXX.  ",
"    ..X.......  ",
"     ..         ",
"                "};

static char * book_closed_xpm[] = {
"16 16 6 1",
"       c None s None",
".      c black",
"X      c red",
"o      c yellow",
"O      c #808080",
"#      c white",
"                ",
"       ..       ",
"     ..XX.      ",
"   ..XXXXX.     ",
" ..XXXXXXXX.    ",
".ooXXXXXXXXX.   ",
"..ooXXXXXXXXX.  ",
".X.ooXXXXXXXXX. ",
".XX.ooXXXXXX..  ",
" .XX.ooXXX..#O  ",
"  .XX.oo..##OO. ",
"   .XX..##OO..  ",
"    .X.#OO..    ",
"     ..O..      ",
"      ..        ",
"                "};
/*
static char * mini_page_xpm[] = {
"16 16 4 1",
"       c None s None",
".      c black",
"X      c white",
"o      c #808080",
"                ",
"   .......      ",
"   .XXXXX..     ",
"   .XoooX.X.    ",
"   .XXXXX....   ",
"   .XooooXoo.o  ",
"   .XXXXXXXX.o  ",
"   .XooooooX.o  ",
"   .XXXXXXXX.o  ",
"   .XooooooX.o  ",
"   .XXXXXXXX.o  ",
"   .XooooooX.o  ",
"   .XXXXXXXX.o  ",
"   ..........o  ",
"    oooooooooo  ",
"                "};

static char * gtk_mini_xpm[] = {
"15 20 17 1",
"       c None",
".      c #14121F",
"+      c #278828",
"@      c #9B3334",
"#      c #284C72",
"$      c #24692A",
"%      c #69282E",
"&      c #37C539",
"*      c #1D2F4D",
"=      c #6D7076",
"-      c #7D8482",
";      c #E24A49",
">      c #515357",
",      c #9B9C9B",
"'      c #2FA232",
")      c #3CE23D",
"!      c #3B6CCB",
"               ",
"      ***>     ",
"    >.*!!!*    ",
"   ***....#*=  ",
"  *!*.!!!**!!# ",
" .!!#*!#*!!!!# ",
" @%#!.##.*!!$& ",
" @;%*!*.#!#')) ",
" @;;@%!!*$&)'' ",
" @%.%@%$'&)$+' ",
" @;...@$'*'*)+ ",
" @;%..@$+*.')$ ",
" @;%%;;$+..$)# ",
" @;%%;@$$$'.$# ",
" %;@@;;$$+))&* ",
"  %;;;@+$&)&*  ",
"   %;;@'))+>   ",
"    %;@'&#     ",
"     >%$$      ",
"      >=       "};
*/

/*
 * GtkNotebook
 */

GdkPixmap *book_open;
GdkPixmap *book_closed;
GdkBitmap *book_open_mask;
GdkBitmap *book_closed_mask;
GtkWidget *sample_notebook;

static void
page_switch (GtkWidget *widget, GtkNotebookPage *page, gint page_num)
{
  GtkNotebookPage *oldpage;
  GtkWidget *pixwid;

  oldpage = GTK_NOTEBOOK (widget)->cur_page;

  if (page == oldpage)
    return;
  pixwid = ((GtkBoxChild*)
	    (GTK_BOX (page->tab_label)->children->data))->widget;
  gtk_pixmap_set (GTK_PIXMAP (pixwid), book_open, book_open_mask);
  pixwid = ((GtkBoxChild*)
	    (GTK_BOX (page->menu_label)->children->data))->widget;
  gtk_pixmap_set (GTK_PIXMAP (pixwid), book_open, book_open_mask);

  if (oldpage)
    {
      pixwid = ((GtkBoxChild*)
		(GTK_BOX (oldpage->tab_label)->children->data))->widget;
      gtk_pixmap_set (GTK_PIXMAP (pixwid), book_closed, book_closed_mask);
      pixwid = ((GtkBoxChild*)
		(GTK_BOX (oldpage->menu_label)->children->data))->widget;
      gtk_pixmap_set (GTK_PIXMAP (pixwid), book_closed, book_closed_mask);
    }
}

/*
static void
tab_fill (GtkToggleButton *button, GtkWidget *child)
{
  gboolean expand;
  GtkPackType pack_type;

  gtk_notebook_query_tab_label_packing (GTK_NOTEBOOK (sample_notebook), child,
					&expand, NULL, &pack_type);
  gtk_notebook_set_tab_label_packing (GTK_NOTEBOOK (sample_notebook), child,
				      expand, button->active, pack_type);
}

static void
tab_expand (GtkToggleButton *button, GtkWidget *child)
{
  gboolean fill;
  GtkPackType pack_type;

  gtk_notebook_query_tab_label_packing (GTK_NOTEBOOK (sample_notebook), child,
					NULL, &fill, &pack_type);
  gtk_notebook_set_tab_label_packing (GTK_NOTEBOOK (sample_notebook), child,
				      button->active, fill, pack_type);
}

static void
tab_pack (GtkToggleButton *button, GtkWidget *child)
	  
{ 
  gboolean expand;
  gboolean fill;

  gtk_notebook_query_tab_label_packing (GTK_NOTEBOOK (sample_notebook), child,
					&expand, &fill, NULL);
  gtk_notebook_set_tab_label_packing (GTK_NOTEBOOK (sample_notebook), child,
				      expand, fill, button->active);
}
*/

static void
toggle_view (GtkToggleButton *button, struct _gui_object *go)
{

  if(!go) return;

  if(GTK_TOGGLE_BUTTON(button)->active)
      go->change_view(go,VIEW_SHOW);
  else
      go->change_view(go,VIEW_HIDE);

}

NotebookItem source_pages[] =
{
  { "Source", NULL },
  { "Object", toggle_view },
  { ".asm",   toggle_view },
  { ".lst",   NULL }
};


static void
create_page (GUI_Processor *gp, GtkNotebook *notebook, NotebookItem *page, int items)
{
  GtkWidget *child = NULL;
  GtkWidget *button;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *label_box;
  GtkWidget *menu_box;
  GtkWidget *pixwid;
  gint i;
  char *buffer=NULL;

  GUI_Object *go;
  GList *gol=NULL;


  for(i=0; i<3; i++)
    {
      switch(i)
	{
	case 0:
	  gol = gp->source_windows;
	  buffer = "Source";
	  break;

	case 1:
	  gol = gp->data_windows;
	  buffer = "Data";
	  break;

	case 2:
	  gol = gp->misc_windows;
	  buffer = "Misc";
	  break;

	}

      child = gtk_frame_new (buffer);
      gtk_container_set_border_width (GTK_CONTAINER (child), 10);
      
      vbox = gtk_vbox_new (TRUE,0);
      gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
      gtk_container_add (GTK_CONTAINER (child), vbox);

      hbox = gtk_hbox_new (TRUE,0);
      gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 5);

      for( ; gol; gol = gol->next)
	{
	  if(gol->data)
	    {

	      go = (GUI_Object *) gol->data;
	      button = gtk_check_button_new_with_label ( go->name);
	      gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, TRUE, 5);

	      if( go->enabled && GTK_WIDGET_VISIBLE(go->window))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
	      else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);
      
	      gtk_signal_connect (GTK_OBJECT (button), "toggled",
				  GTK_SIGNAL_FUNC (toggle_view), (gpointer)go);

	    }
	}

      gtk_widget_show_all (child);


      label_box = gtk_hbox_new (FALSE, 0);
      pixwid = gtk_pixmap_new (book_closed, book_closed_mask);
      gtk_box_pack_start (GTK_BOX (label_box), pixwid, FALSE, TRUE, 0);
      gtk_misc_set_padding (GTK_MISC (pixwid), 3, 1);
      label = gtk_label_new (buffer);
      gtk_box_pack_start (GTK_BOX (label_box), label, FALSE, TRUE, 0);
      gtk_widget_show_all (label_box);

      menu_box = gtk_hbox_new (FALSE, 0);
      pixwid = gtk_pixmap_new (book_closed, book_closed_mask);
      gtk_box_pack_start (GTK_BOX (menu_box), pixwid, FALSE, TRUE, 0);
      gtk_misc_set_padding (GTK_MISC (pixwid), 3, 1);
      label = gtk_label_new (buffer);
      gtk_box_pack_start (GTK_BOX (menu_box), label, FALSE, TRUE, 0);
      gtk_widget_show_all (menu_box);
      gtk_notebook_append_page_menu (notebook, child, label_box, menu_box);
    }

}
/*
static void
notebook_popup (GtkToggleButton *button,
		GtkNotebook     *notebook)
{
  if (button->active)
    gtk_notebook_popup_enable (notebook);
  else
    gtk_notebook_popup_disable (notebook);
}

static void
notebook_homogeneous (GtkToggleButton *button,
		      GtkNotebook     *notebook)
{
  gtk_notebook_set_homogeneous_tabs (notebook, button->active);
}
*/
static void
create_notebook (void)
{
  static GtkWidget *window = NULL;
  GtkWidget *box1;
  GtkWidget *button;
  GtkWidget *separator;
  GdkColor *transparent = NULL;

  if (!window)
    {
      window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

      gtk_signal_connect (GTK_OBJECT (window), "destroy",
			  GTK_SIGNAL_FUNC(gtk_widget_destroyed),
			  &window);

      gtk_window_set_title (GTK_WINDOW (window), "Window Selector");
      gtk_container_set_border_width (GTK_CONTAINER (window), 0);

      box1 = gtk_vbox_new (FALSE, 0);
      gtk_container_add (GTK_CONTAINER (window), box1);

      sample_notebook = gtk_notebook_new ();
      gtk_signal_connect (GTK_OBJECT (sample_notebook), "switch_page",
			  GTK_SIGNAL_FUNC (page_switch), NULL);
      gtk_notebook_set_tab_pos (GTK_NOTEBOOK (sample_notebook), GTK_POS_TOP);
      gtk_box_pack_start (GTK_BOX (box1), sample_notebook, TRUE, TRUE, 0);
      gtk_container_set_border_width (GTK_CONTAINER (sample_notebook), 10);

      gtk_widget_realize (sample_notebook);
      book_open = gdk_pixmap_create_from_xpm_d (sample_notebook->window,
						&book_open_mask, 
						transparent, 
						book_open_xpm);
      book_closed = gdk_pixmap_create_from_xpm_d (sample_notebook->window,
						  &book_closed_mask,
						  transparent, 
						  book_closed_xpm);

      create_page (gp, GTK_NOTEBOOK (sample_notebook), source_pages,
		   sizeof (source_pages) / sizeof (source_pages[0]));

      //      create_page (GTK_NOTEBOOK (sample_notebook), "Registers" );

      separator = gtk_hseparator_new ();
      gtk_box_pack_start (GTK_BOX (box1), separator, FALSE, TRUE, 5);

      button = gtk_button_new_with_label ("close");
      gtk_container_set_border_width (GTK_CONTAINER (button), 5);
      gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
				 GTK_SIGNAL_FUNC (gtk_widget_destroy),
				 GTK_OBJECT (window));
      gtk_box_pack_start (GTK_BOX (box1), button, FALSE, FALSE, 0);
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_widget_grab_default (button);
    }

  if (!GTK_WIDGET_VISIBLE (window))
    gtk_widget_show_all (window);
  else
    gtk_widget_destroy (window);
}
#endif // HAVE_GUI
