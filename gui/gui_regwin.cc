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

#include "../config.h"
#ifdef HAVE_GUI

#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <string.h>

#include <assert.h>

#include <gtkextra/gtksheet.h>

/*#include <src/interface.h>*/

#include "../src/interface.h"

#include "gui.h"

#define DEFAULT_PRECISION 3
#define DEFAULT_SPACE 8

#define TRACE_FILE_FORMAT_ASCII 0
#define TRACE_FILE_FORMAT_LXT 1

extern int gui_question(char *question, char *a, char *b);

static void update_ascii(Register_Window *rw, gint row);

// extern GUI_Processor *gp;

typedef enum {
    MENU_BREAK_CLEAR,
    MENU_BREAK_READ,
    MENU_BREAK_WRITE,
    MENU_BREAK_READ_VALUE,
    MENU_BREAK_WRITE_VALUE,
    MENU_ADD_WATCH,
    MENU_SETTINGS,
    MENU_LOG_SETTINGS,
    MENU_LOG_READ,
    MENU_LOG_WRITE,
    MENU_LOG_READ_VALUE,
    MENU_LOG_WRITE_VALUE,
} menu_id;


/////////////////////
/////
/////  Experimental code to test an object oriented way of implementing menus
/////
/////////////////////
class MenuItem {

private:
  char *name;
  menu_id id;

public:

  virtual void execute(void)=0;

};

class RegWindowMenuItem : public MenuItem {

private:
 Register_Window *rw;

};

class Menu_BreakClear : public RegWindowMenuItem {
public:
  virtual void execute(void) {
    printf("BreakClear");
  }

} BreakClear;

MenuItem *__menu_items[] = {
  &BreakClear
};

/////////////////////
/////
/////  End of experimental code
/////
/////////////////////


typedef struct _menu_item {
    char *name;
    menu_id id;
} menu_item;

static menu_item menu_items[] = {
    {"Clear breakpoints", MENU_BREAK_CLEAR},
    {"Set break on read", MENU_BREAK_READ},
    {"Set break on write", MENU_BREAK_WRITE},
    {"Set break on read value...", MENU_BREAK_READ_VALUE},
    {"Set break on write value...", MENU_BREAK_WRITE_VALUE},
    {"Set log settings...", MENU_LOG_SETTINGS},
    {"Set log on read", MENU_LOG_READ},
    {"Set log on write", MENU_LOG_WRITE},
    {"Set log on read value...", MENU_LOG_READ_VALUE},
    {"Set log on write value...", MENU_LOG_WRITE_VALUE},
    {"Add watch", MENU_ADD_WATCH},
    {"Settings...", MENU_SETTINGS}
};

static int settings_dialog(Register_Window *rw);
extern int font_dialog_browse(GtkWidget *w, gpointer user_data);
static int dlg_x=200, dlg_y=200;


// Used only in popup menus
Register_Window *popup_rw;


// get_value
static void a_cb(GtkWidget *w, gpointer user_data)
{
    *(int*)user_data=TRUE;
//    gtk_main_quit();
}
static void b_cb(GtkWidget *w, gpointer user_data)
{
    *(int*)user_data=FALSE;
//    gtk_main_quit();
}
// used for reading a value from user when break on value is requested
int gui_get_value(char *prompt)
{
    static GtkWidget *dialog=NULL;
    static GtkWidget *label;
    static GtkWidget *entry;
    GtkWidget *button;
    GtkWidget *hbox;
    
    int retval=-1;

    int value;
    
    if(dialog==NULL)
    {
	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog),"enter value");
//	gtk_signal_connect(GTK_OBJECT(dialog),
//			   "configure_event",GTK_SIGNAL_FUNC(configure_event),0);
	gtk_signal_connect_object(GTK_OBJECT(dialog),
				  "delete_event",GTK_SIGNAL_FUNC(gtk_widget_hide),GTK_OBJECT(dialog));

	label=gtk_label_new("values can be entered in decimal, hexadecimal, and octal.\nFor example: 31 is the same as 0x1f and 037");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label,FALSE,FALSE,20);
	
	hbox = gtk_hbox_new(0,0);
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox,FALSE,FALSE,20);

	button = gtk_button_new_with_label("OK");
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button,
			   FALSE,FALSE,10);
	gtk_signal_connect(GTK_OBJECT(button),"clicked",
			   GTK_SIGNAL_FUNC(a_cb),(gpointer)&retval);
	
	button = gtk_button_new_with_label("Cancel");
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button,
			   FALSE,FALSE,10);
	gtk_signal_connect(GTK_OBJECT(button),"clicked",
			   GTK_SIGNAL_FUNC(b_cb),(gpointer)&retval);

	label=gtk_label_new(prompt);
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(hbox), label,
			   FALSE,FALSE, 20);

	entry=gtk_entry_new();
	gtk_widget_show(entry);
	gtk_box_pack_start(GTK_BOX(hbox), entry,FALSE,FALSE,20);

    }
    else
    {
	gtk_label_set_text(GTK_LABEL(label),prompt);
    }
    
//    gtk_widget_set_uposition(GTK_WIDGET(dialog),dlg_x,dlg_y);
    gtk_widget_show_now(dialog);

    gtk_grab_add(dialog);
    while(retval==-1 && GTK_WIDGET_VISIBLE(dialog))
	gtk_main_iteration();
    gtk_grab_remove(dialog);
    
    gtk_widget_hide(dialog);

    if(retval==TRUE)
    {
	char *end;
	const gchar *entry_text;
	entry_text = gtk_entry_get_text(GTK_ENTRY(entry));
	value = strtoul(entry_text,&end,0);
	if(*entry_text!='\0' && *end=='\0')
	    return value;
	else
	    return -1;
    }
    
    return -1;
}

// used for reading a value from user when break on value is requested
void gui_get_2values(char *prompt1, int *value1, char *prompt2, int *value2)
{
    static GtkWidget *dialog=NULL;
    GtkWidget *button;
    GtkWidget *hbox1, *hbox2;
    static GtkWidget *label1, *label2, *label;
    static GtkWidget *entry1, *entry2;

    int value;
    
    int retval=-1;

    if(dialog==NULL)
    {
	dialog = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(dialog),"enter values");
//	gtk_signal_connect(GTK_OBJECT(dialog),
//			   "configure_event",GTK_SIGNAL_FUNC(configure_event),0);
	gtk_signal_connect_object(GTK_OBJECT(dialog),
				  "delete_event",GTK_SIGNAL_FUNC(gtk_widget_hide),GTK_OBJECT(dialog));

	label=gtk_label_new("values can be entered in decimal, hexadecimal, and octal.\nFor example: 31 is the same as 0x1f and 037");
	gtk_widget_show(label);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label,FALSE,FALSE,20);
	
	button = gtk_button_new_with_label("OK");
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button,
			   FALSE,FALSE,10);
	gtk_signal_connect(GTK_OBJECT(button),"clicked",
			   GTK_SIGNAL_FUNC(a_cb),(gpointer)&retval);
	
	button = gtk_button_new_with_label("Cancel");
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button,
			   FALSE,FALSE,10);
	gtk_signal_connect(GTK_OBJECT(button),"clicked",
			   GTK_SIGNAL_FUNC(b_cb),(gpointer)&retval);

        // Value 1
	hbox1 = gtk_hbox_new(0,0);
	gtk_widget_show(hbox1);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox1,FALSE,FALSE,20);

	label1=gtk_label_new(prompt1);
	gtk_widget_show(label1);
	gtk_box_pack_start(GTK_BOX(hbox1), label1,
			   FALSE,FALSE, 20);

	entry1=gtk_entry_new();
	gtk_widget_show(entry1);
	gtk_box_pack_start(GTK_BOX(hbox1), entry1,FALSE,FALSE,20);

        // Value 2
	hbox2 = gtk_hbox_new(0,0);
	gtk_widget_show(hbox2);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox2,FALSE,FALSE,20);

	label2=gtk_label_new(prompt2);
	gtk_widget_show(label2);
	gtk_box_pack_start(GTK_BOX(hbox2), label2,
			   FALSE,FALSE, 20);

	entry2=gtk_entry_new();
	gtk_widget_show(entry2);
	gtk_box_pack_start(GTK_BOX(hbox2), entry2,FALSE,FALSE,20);

    }
    else
    {
	gtk_label_set_text(GTK_LABEL(label1),prompt1);
	gtk_label_set_text(GTK_LABEL(label2),prompt2);
    }
    
//    gtk_widget_set_uposition(GTK_WIDGET(dialog),dlg_x,dlg_y);
    gtk_widget_show_now(dialog);

    gtk_grab_add(dialog);
    while(retval==-1 && GTK_WIDGET_VISIBLE(dialog))
	gtk_main_iteration();
    gtk_grab_remove(dialog);
    
    gtk_widget_hide(dialog);

    if(retval==TRUE)
    {
	// "Ok"

	char *end;
	const gchar *entry_text;

	entry_text = gtk_entry_get_text(GTK_ENTRY(entry1));
	value = strtoul(entry_text,&end,0);
	if(*entry_text=='\0' || *end!='\0')
	{
	    *value1=-1;
            *value2=-1;
	    return;
	}
        *value1=value;

	entry_text = gtk_entry_get_text(GTK_ENTRY(entry2));
	value = strtoul(entry_text,&end,0);
	if(*entry_text=='\0' || *end!='\0')
	{
	    *value1=-1;
            *value2=-1;
	    return;
	}
        *value2=value;
        return;
    }

    // "Cancel"

    *value1=-1;
    *value2=-1;
    return;
}


static const char *file_selection_name;
static int filemode;
static int fs_done;

static void
file_selection_ok (GtkWidget        *w,
		   GtkFileSelection *fs)
{
    char *file;

    file_selection_name=gtk_file_selection_get_filename (fs);

    fs_done=1;
}

static void
file_selection_cancel (GtkWidget        *w,
		       GtkFileSelection *fs)
{
    file_selection_name=NULL;
    fs_done=1;
}

static void
modepopup_activated(GtkWidget *widget, gpointer data)
{
    char *modestring;

    modestring=(char*)data;

    if(!strcmp(modestring,"ASCII"))
        filemode=TRACE_FILE_FORMAT_ASCII;
    if(!strcmp(modestring,"LXT"))
	filemode=TRACE_FILE_FORMAT_LXT;
}

static char *gui_get_log_settings(const char **filename, int *mode)
{
    static GtkWidget *window = NULL;

    GtkWidget *hbox, *optionmenu, *label;

    GtkWidget *menu;
    GtkWidget *item;

    char *prompt="Log settings";

    if (!window)
    {

	window = gtk_file_selection_new (prompt);

	gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (window));

	gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);

	gtk_signal_connect_object(GTK_OBJECT(window),
				  "delete_event",GTK_SIGNAL_FUNC(gtk_widget_hide),GTK_OBJECT(window));
//	gtk_signal_connect_object (GTK_OBJECT (window), "destroy",
//				   GTK_SIGNAL_FUNC(gtk_widget_destroyed),
//				   GTK_OBJECT(window));

	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (window)->ok_button),
			    "clicked", GTK_SIGNAL_FUNC(file_selection_ok),
			    window);
	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (window)->cancel_button),
			    "clicked", GTK_SIGNAL_FUNC(file_selection_cancel),
			    window);

	hbox = gtk_hbox_new(0,0);
        gtk_widget_show(hbox);

	gtk_box_pack_end(GTK_BOX(GTK_FILE_SELECTION (window)->action_area),
			 hbox,
			 FALSE,FALSE,20);

	label = gtk_label_new("File format:");
	gtk_box_pack_start(GTK_BOX(hbox),
			 label,
			 FALSE,FALSE,20);
        gtk_widget_show(label);

	optionmenu = gtk_option_menu_new();
        gtk_widget_show(optionmenu);
	gtk_box_pack_end(GTK_BOX(hbox),
			 optionmenu,
			 FALSE,FALSE,20);

	menu=gtk_menu_new();

	item=gtk_menu_item_new_with_label("ASCII");
	gtk_signal_connect(GTK_OBJECT(item),"activate",
			   (GtkSignalFunc) modepopup_activated,
			   (gpointer)"ASCII");
//      GTK_WIDGET_SET_FLAGS (item, GTK_SENSITIVE | GTK_CAN_FOCUS);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(menu),item);
	item=gtk_menu_item_new_with_label("LXT");
	gtk_signal_connect(GTK_OBJECT(item),"activate",
			   (GtkSignalFunc) modepopup_activated,
			   (gpointer)"LXT");
//      GTK_WIDGET_SET_FLAGS (item, GTK_SENSITIVE | GTK_CAN_FOCUS);
	gtk_widget_show(item);
	gtk_menu_append(GTK_MENU(menu),item);

        gtk_option_menu_set_menu(GTK_OPTION_MENU(optionmenu), menu);

    }

    file_selection_name=NULL;
    gtk_widget_show_now(window);

    fs_done=0;
    file_selection_name=NULL;
    gtk_grab_add(window);
    while(!fs_done && GTK_WIDGET_VISIBLE(window))
	gtk_main_iteration();
    gtk_grab_remove(window);
    
    gtk_widget_hide(window);

    if(file_selection_name==NULL)
    {
        *filename=NULL;
	return NULL;
    }

    *filename=file_selection_name;
    *mode=filemode;
    return NULL;
}

extern int gui_question(char *question, char *a, char *b);

// called when user has selected a menu item
static void
popup_activated(GtkWidget *widget, gpointer data)
{
  GtkSheet *sheet;

  menu_item *item;
  int i,j;
  unsigned int pic_id;
  GtkSheetRange range;
  unsigned int address;
  int value, mask;
  const char *filename;
  int mode;

  if(widget==NULL || data==NULL)
    {
      printf("Warning popup_activated(%p,%p)\n",widget,data);
      return;
    }
  if(!popup_rw || !popup_rw->gp || !popup_rw->gp->cpu) {
    printf(" no cpu\n");
    return;
  }
    
  item = (menu_item *)data;
  sheet=GTK_SHEET(popup_rw->register_sheet);
  range = sheet->range;
  //pic_id = ((GUI_Object*)popup_rw)->gp->pic_id;
  pic_id = popup_rw->gp->pic_id;

  switch(item->id)
    {
    case MENU_BREAK_READ:
      for(j=range.row0;j<=range.rowi;j++)
	for(i=range.col0;i<=range.coli;i++)
	  {
	    address=popup_rw->row_to_address[j]+i;
	    printf("break on read \n");
	    bp.set_read_break(popup_rw->gp->cpu, address);
	  }
      break;
    case MENU_BREAK_WRITE:
      for(j=range.row0;j<=range.rowi;j++)
	for(i=range.col0;i<=range.coli;i++)
	  {
	    address=popup_rw->row_to_address[j]+i;
	    bp.set_write_break(popup_rw->gp->cpu, address);
	  }
      break;
    case MENU_BREAK_READ_VALUE:
      value = gui_get_value("value to read for breakpoint:");
      if(value<0)
	break; // Cancel
      for(j=range.row0;j<=range.rowi;j++)
	for(i=range.col0;i<=range.coli;i++)
	  {
	    address=popup_rw->row_to_address[j]+i;
	    bp.set_read_value_break(popup_rw->gp->cpu,address,value);
	  }
      break;
    case MENU_BREAK_WRITE_VALUE:
      value = gui_get_value("value to write for breakpoint:");
      if(value<0)
	break; // Cancel
      for(j=range.row0;j<=range.rowi;j++)
	for(i=range.col0;i<=range.coli;i++)
	  {
	    address=popup_rw->row_to_address[j]+i;
	    bp.set_write_value_break(popup_rw->gp->cpu,address,value);
	  }
      break;
    case MENU_BREAK_CLEAR:
      for(j=range.row0;j<=range.rowi;j++)
	for(i=range.col0;i<=range.coli;i++)
	  {
	    address=popup_rw->row_to_address[j]+i;
	    bp.clear_all_register(popup_rw->gp->cpu,address);
	  }
      break;
    case MENU_ADD_WATCH:
      for(j=range.row0;j<=range.rowi;j++)
	for(i=range.col0;i<=range.coli;i++)
	  {
	    address=popup_rw->row_to_address[j]+i;
	    popup_rw->gp->watch_window->Add(pic_id, popup_rw->type, address);
	  }
      break;
    case MENU_SETTINGS:
      settings_dialog(popup_rw);
      break;
    case MENU_LOG_SETTINGS:
      gui_get_log_settings(&filename, &mode);
      if(filename!=NULL)
	gpsim_set_log_name(pic_id,filename,mode);
      break;
    case MENU_LOG_READ:
      for(j=range.row0;j<=range.rowi;j++)
	for(i=range.col0;i<=range.coli;i++)
	  {
	    address=popup_rw->row_to_address[j]+i;
	    gpsim_reg_set_read_logging(pic_id, popup_rw->type, address);
	  }
      break;
    case MENU_LOG_WRITE:
      for(j=range.row0;j<=range.rowi;j++)
	for(i=range.col0;i<=range.coli;i++)
	  {
	    address=popup_rw->row_to_address[j]+i;
	    gpsim_reg_set_write_logging(pic_id, popup_rw->type, address);
	  }
      break;
    case MENU_LOG_READ_VALUE:
      gui_get_2values("Value that the read must match for logging it:", &value,
		      "Bitmask that specifies the bits to bother about:", &mask);
      if(value<0)
	break; // Cancel
      for(j=range.row0;j<=range.rowi;j++)
	for(i=range.col0;i<=range.coli;i++)
	  {
	    address=popup_rw->row_to_address[j]+i;
	    gpsim_reg_set_read_value_logging(pic_id, popup_rw->type, address, value, mask);
	  }
      break;
    case MENU_LOG_WRITE_VALUE:
      gui_get_2values("Value that the write must match for logging it:", &value,
		      "Bitmask that specifies the bits to bother about:", &mask);
      if(value<0)
	break; // Cancel
      for(j=range.row0;j<=range.rowi;j++)
	for(i=range.col0;i<=range.coli;i++)
	  {
	    address=popup_rw->row_to_address[j]+i;
	    gpsim_reg_set_write_value_logging(pic_id, popup_rw->type, address, value, mask);
	  }
      break;
    default:
      puts("Unhandled menuitem?");
      break;
    }
}


static GtkWidget *
build_menu(Register_Window *rw)
{
  GtkWidget *menu;
  GtkWidget *item;
//  GtkAccelGroup *accel_group;
  int i;


  if(rw==NULL)
  {
      printf("Warning build_menu(%p)\n",rw);
      return NULL;
  }
    
  menu=gtk_menu_new();


  item = gtk_tearoff_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), item);
  gtk_widget_show (item);
  
  
  for (i=0; i < (sizeof(menu_items)/sizeof(menu_items[0])) ; i++){
      item=gtk_menu_item_new_with_label(menu_items[i].name);

      gtk_signal_connect(GTK_OBJECT(item),"activate",
			 (GtkSignalFunc) popup_activated,
			 &menu_items[i]);
      GTK_WIDGET_SET_FLAGS (item, GTK_SENSITIVE | GTK_CAN_FOCUS);

      if(rw->type == REGISTER_EEPROM
	 && menu_items[i].id!=MENU_ADD_WATCH
	 &&menu_items[i].id!=MENU_SETTINGS)
      {
	  GTK_WIDGET_UNSET_FLAGS (item,
				  GTK_SENSITIVE | GTK_CAN_FOCUS);
      }
      gtk_widget_show(item);
      gtk_menu_append(GTK_MENU(menu),item);
  }
  
  return menu;
}

// button press handler
static gint
do_popup(GtkWidget *widget, GdkEventButton *event, Register_Window *rw)
{

    GtkWidget *popup;
//	GdkModifierType mods;
    GtkSheet *sheet;

    popup=rw->popup_menu;
    
  if(widget==NULL || event==NULL || rw==NULL)
  {
      printf("Warning do_popup(%p,%p,%p)\n",widget,event,rw);
      return 0;
  }
  
    sheet=GTK_SHEET(widget);

    if( (event->type == GDK_BUTTON_PRESS) &&  (event->button == 3) )
    {

	popup_rw = rw;
  
	gtk_menu_popup(GTK_MENU(popup), NULL, NULL, NULL, NULL,
			   3, event->time);
    }
    return FALSE;
}

// The following routine will convert the first number it finds in
// a string to an unsigned long integer. All of the hard work is done
// in the library function strtoul (string to unsigned long).

static unsigned long get_number_in_string(const char *number_string)
{
  unsigned long retval = 0;
  char *bad_position;
  int current_base = 16;
  
  if(number_string==NULL)
  {
      printf("Warning get_number_in_string(%p)\n",number_string);
      errno = EINVAL;
      return (unsigned long)-1;
  }


  errno = 0;

  retval = strtoul(number_string, &bad_position, current_base);

  if( strlen(bad_position) ) 
    errno = EINVAL;  /* string contains an invalid number */

  return(retval);
}

// when a new cell is selected, we write changes in
// previously selected cell to gpsim
// (the name of the signal seems a bit strange)
static void
set_cell(GtkWidget *widget, int row, int col, Register_Window *rw)
{
  GtkSheet *sheet;
  const gchar *text;
  int justification,n=0;
  //int crow, ccol;

  GUI_Processor *gp;
  
  sheet=GTK_SHEET(widget);
  
  if(widget==NULL ||
     row>sheet->maxrow || row<0 ||
     col>sheet->maxcol || col<0 || rw==NULL)
  {
      printf("Warning set_cell(%p,%x,%x,%p)\n",widget,row,col,rw);
      return;
  }

  gp = rw->gp;
  
  if(gp->pic_id==0)
    return;
  
  justification=GTK_JUSTIFY_RIGHT;

  if(col < REGISTERS_PER_ROW)
    {

      int reg = rw->row_to_address[row] + col;

      if( rw->row_to_address[row] == -1)
      {
	  puts("Warning row_to_address[row] == -1 in set_cell");
	  return;
      }
	  
      // extract value from sheet cell
      text = gtk_entry_get_text(GTK_ENTRY(sheet->sheet_entry));

      errno = 0;
      if(text!=NULL && strlen(text)>0)
	n = get_number_in_string(text);
      else
	errno = ERANGE;

      if(errno != 0)
	{
	  n = gpsim_get_register_value(gp->pic_id, rw->type, reg);
	  rw->registers[reg]->value = -1;
	}

      // n=value in sheet cell

      // check if value has changed, and write if so
      if(gpsim_get_register_name(gp->pic_id,rw->type, reg))
      {
	  if(n != rw->registers[reg]->value)
	  {
	      //puts("Writing new value");
	      gpsim_put_register_value(gp->pic_id, rw->type, reg, n&0xff);
	      update_ascii(rw,row);
	  }
      }
    }
  else
      ; // ignore user changes in ascii column for right now

}

//------------------------------------------------------------------------
// UpdateLabel
//
//

void Register_Window::UpdateLabel(void)
{
  gint row, col;

  int regnumber;
  char cell[100],*n;


  row=register_sheet->active_cell.row;
  col=register_sheet->active_cell.col;

  
  if(row_to_address[row] < 0) {
    printf("row_to_address[%d]=0x%x\n",row,row_to_address[row]);
    return;
  }

  regnumber = row_to_address[row]+col;

  // get the string to put in label
  cell[0] = 0;

  if(gp) {
    if(col < REGISTERS_PER_ROW) {

      if(gp->pic_id != 0) {
	n = gpsim_get_register_name(gp->pic_id, type, regnumber);
	if(n==NULL)
	  n="INVALID REGISTER";
      }  else
	n = "00"; // FIXME

      strncpy(cell,n,100);
    }
    else
      sprintf(cell,"  ascii  ");
  }
  else
    {
      puts("**************** Warning not gp?");
      sprintf(cell," 0x%02x  ", regnumber);
    }
  // cell is now the string we want. Set the label
  gtk_label_set(GTK_LABEL(location), cell);

}

//------------------------------------------------------------------------
// UpdateEntry
//
//

void Register_Window::UpdateEntry(void)
{
  gint row, col;

  const char *text; 
  GtkWidget * sheet_entry;

  sheet_entry = gtk_sheet_get_entry(register_sheet);
  row=register_sheet->active_cell.row;
  col=register_sheet->active_cell.col;

  
  // ******************************** update entry:
  if(row_to_address[row] < 0) {
    printf("row_to_address[%d]=0x%x",row,row_to_address[row]);
    return;
  }

  if(gpsim_get_register_name(gp->pic_id,type, row_to_address[row]+col))
    {
      if((text=gtk_entry_get_text (GTK_ENTRY(sheet_entry))))
	gtk_entry_set_text(GTK_ENTRY(entry), text);
    }
}

//------------------------------------------------------------------------
// UpdateLabelEntry
//
//

void Register_Window::UpdateLabelEntry(void)
{
  UpdateLabel();
  UpdateEntry();
}

static gint configure_event(GtkWidget *widget, GdkEventConfigure *e, gpointer data)
{


  if(widget->window==NULL)
    return 0;
    
  gdk_window_get_root_origin(widget->window,&dlg_x,&dlg_y);
  return 0; // what should be returned?, FIXME
}

static int load_styles(Register_Window *rw)
{
    GdkColormap *colormap = gdk_colormap_get_system();

#if GTK_MAJOR_VERSION >= 2
    rw->normalfont = pango_font_description_from_string(rw->normalfont_string);
#else
    rw->normalfont=gdk_fontset_load (rw->normalfont_string);
#endif
    gdk_color_parse("light cyan", &rw->normal_bg_color);
    gdk_color_parse("black", &rw->normal_fg_color);
    gdk_color_parse("blue", &rw->item_has_changed_color);
    gdk_color_parse("red", &rw->breakpoint_color);
    gdk_color_parse("light gray", &rw->alias_color);
    gdk_color_parse("black", &rw->invalid_color);
    gdk_color_parse("cyan", &rw->sfr_bg_color);

    gdk_colormap_alloc_color(colormap, &rw->normal_bg_color,FALSE,TRUE );
    gdk_colormap_alloc_color(colormap, &rw->normal_fg_color,FALSE,TRUE );
    gdk_colormap_alloc_color(colormap, &rw->item_has_changed_color,FALSE,TRUE);
    gdk_colormap_alloc_color(colormap, &rw->breakpoint_color,FALSE,TRUE);
    gdk_colormap_alloc_color(colormap, &rw->alias_color,FALSE,TRUE);
    gdk_colormap_alloc_color(colormap, &rw->invalid_color,FALSE,TRUE);
    gdk_colormap_alloc_color(colormap, &rw->sfr_bg_color,FALSE,TRUE);

    if(rw->normalfont==NULL)
	return 0;
    return 1;
}

/********************** Settings dialog ***************************/
static int settings_active;
static void settingsok_cb(GtkWidget *w, gpointer user_data)
{
    if(settings_active)
    {
        settings_active=0;
//	gtk_main_quit();
    }
}
static int settings_dialog(Register_Window *rw)
{
    static GtkWidget *dialog=NULL;
    GtkWidget *button;
    static int retval;
    GtkWidget *hbox;
    static GtkWidget *normalfontstringentry;
    GtkWidget *label;
    int fonts_ok=0;
    GtkSheet *sheet;
    GtkSheetRange range;
    gint row_height,column_width,char_width;
    int i;

    sheet=GTK_SHEET(rw->register_sheet);
    
    if(dialog==NULL)
    {
	dialog = gtk_dialog_new();
	gtk_window_set_title (GTK_WINDOW (dialog), "Register window settings");
	gtk_signal_connect(GTK_OBJECT(dialog),
			   "configure_event",GTK_SIGNAL_FUNC(configure_event),0);
	gtk_signal_connect_object(GTK_OBJECT(dialog),
			   "delete_event",GTK_SIGNAL_FUNC(gtk_widget_hide),GTK_OBJECT(dialog));


	// Normal font
	hbox = gtk_hbox_new(0,0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox,FALSE,FALSE,20);
	gtk_widget_show(hbox);
	label=gtk_label_new("Normal font:");
	gtk_box_pack_start(GTK_BOX(hbox), label,
			   FALSE,FALSE, 20);
	gtk_widget_show(label);
	normalfontstringentry=gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), normalfontstringentry,
			   TRUE, TRUE, 0);
	gtk_widget_show(normalfontstringentry);
	button = gtk_button_new_with_label("Browse...");
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(hbox), button,
			   FALSE,FALSE,10);
	gtk_signal_connect(GTK_OBJECT(button),"clicked",
			   GTK_SIGNAL_FUNC(font_dialog_browse),(gpointer)normalfontstringentry);


	// OK button
	button = gtk_button_new_with_label("OK");
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button,
			   FALSE,FALSE,10);
	gtk_signal_connect(GTK_OBJECT(button),"clicked",
			   GTK_SIGNAL_FUNC(settingsok_cb),(gpointer)dialog);
    }
    
    gtk_entry_set_text(GTK_ENTRY(normalfontstringentry), rw->normalfont_string);

    gtk_widget_set_uposition(GTK_WIDGET(dialog),dlg_x,dlg_y);
    gtk_widget_show_now(dialog);



    while(fonts_ok!=1)
    {
	char fontname[256];
	GdkFont *font;

        settings_active=1;
	while(settings_active)
	    gtk_main_iteration();

	fonts_ok=0;

	strcpy(fontname,gtk_entry_get_text(GTK_ENTRY(normalfontstringentry)));
	if((font=gdk_fontset_load(fontname))==NULL)
	{
	    if(gui_question("Font did not load!","Try again","Ignore/Cancel")==FALSE)
		break;
	}
	else
	{
            gdk_font_unref(font);
	    strcpy(rw->normalfont_string,gtk_entry_get_text(GTK_ENTRY(normalfontstringentry)));
	    config_set_string(rw->name,"normalfont",rw->normalfont_string);
            fonts_ok++;
	}
    }

    load_styles(rw);

    gtk_sheet_freeze(rw->register_sheet);
    range.row0=0;
    range.rowi=sheet->maxrow;
    range.col0=0;
    range.coli=sheet->maxcol;
    gtk_sheet_range_set_font(sheet, &range, rw->normalfont);

#if GTK_MAJOR_VERSION >= 2
    char_width = gdk_string_width(gdk_font_from_description(rw->normalfont), "9");
#else
    char_width = gdk_string_width (rw->normalfont,"9");
#endif
    row_height = 3 * char_width + 6;
    column_width = 3 * char_width + 6;
    for(i=0; i<rw->register_sheet->maxcol; i++){
        gtk_sheet_set_column_width (rw->register_sheet, i, column_width);
        gtk_sheet_set_row_height (rw->register_sheet, i, row_height);
    }
    gtk_sheet_set_column_width (rw->register_sheet, i, REGISTERS_PER_ROW*char_width + 6);
    gtk_sheet_set_row_titles_width(rw->register_sheet, column_width);
    gtk_sheet_set_column_titles_height(rw->register_sheet, row_height);
    gtk_sheet_thaw(rw->register_sheet);

    gtk_widget_hide(dialog);

    return retval;
}

static void
clipboard_handler(GtkWidget *widget, GdkEventKey *key)
{
  GtkSheet *sheet;

  sheet = GTK_SHEET(widget);

  if(key->state & GDK_CONTROL_MASK || key->keyval==GDK_Control_L ||
     key->keyval==GDK_Control_R){
    if((key->keyval=='c' || key->keyval == 'C') && sheet->state != GTK_STATE_NORMAL){
#if GTK_MAJOR_VERSION >= 2
      if (gtk_sheet_in_clip(sheet))
        gtk_sheet_unclip_range(sheet);
#else
      if(GTK_SHEET_IN_CLIP(sheet))
        gtk_sheet_unclip_range(sheet);
#endif
      gtk_sheet_clip_range(sheet, &sheet->range);
    }
    if(key->keyval=='x' || key->keyval == 'X')
            gtk_sheet_unclip_range(sheet);    
  }
}

static void 
resize_handler(GtkWidget *widget, GtkSheetRange *old_range, 
                                  GtkSheetRange *new_range, 
                                  Register_Window *rw)
{
    int i, j, cti, ctj;
    int from, to;
    int value;
    
  if(widget==NULL || old_range==NULL || new_range==NULL || rw==NULL)
  {
      printf("Warning resize_handler(%p,%p,%p,%p)\n",widget,old_range,new_range,rw);
      return;
  }

    cti = new_range->coli - new_range->col0 + 1;
    ctj = new_range->rowi - new_range->row0 + 1;

    // We always copy from this one cell.
    from = rw->row_to_address[old_range->row0]+old_range->col0;
    
    for(j=0;j<ctj;j++)
    {
	for(i=0;i<cti;i++)
	{
	    to = rw->row_to_address[new_range->row0+j]+new_range->col0+i;
	    value=gpsim_get_register_value(rw->gp->pic_id,rw->type,from);
	    gpsim_put_register_value(rw->gp->pic_id, rw->type, to, value);
	}
    }
}

static void 
move_handler(GtkWidget *widget, GtkSheetRange *old_range, 
                                  GtkSheetRange *new_range, 
                                  Register_Window *rw)
{
    int i, j, cti, ctj;
    int from, to;
    int value;

  if(widget==NULL || old_range==NULL || new_range==NULL || rw==NULL)
  {
      printf("Warning move_handler(%p,%p,%p,%p)\n",widget,old_range,new_range,(unsigned int)rw);
      return;
  }
    cti = new_range->coli - new_range->col0 + 1;
    ctj = new_range->rowi - new_range->row0 + 1;

    for(j=0;j<ctj;j++)
    {
	for(i=0;i<cti;i++)
	{
	    from = rw->row_to_address[old_range->row0+j]+old_range->col0+i;
	    to = rw->row_to_address[new_range->row0+j]+new_range->col0+i;
	    value=gpsim_get_register_value(rw->gp->pic_id, rw->type, from);
	    gpsim_put_register_value(rw->gp->pic_id, rw->type, to, value);
	}
    }
}


/* when the entry above the sheet is changed (typed a digit), we
   copy it to the cell entry */
static void
show_sheet_entry(GtkWidget *widget, Register_Window *rw)
{
 const char *text;
 GtkSheet *sheet;
 GtkEntry *sheet_entry;

 int row,col;
 
 if(widget==NULL|| rw==NULL)
  {
      printf("Warning show_sheet_entry(%p,%p)\n",widget,rw);
      return;
  }

 if(!GTK_WIDGET_HAS_FOCUS(widget)) return;
 
 sheet=GTK_SHEET(rw->register_sheet);
 sheet_entry = GTK_ENTRY(gtk_sheet_get_entry(sheet));

 row=sheet->active_cell.row; col=sheet->active_cell.col;

 if(gpsim_get_register_name(gp->pic_id,rw->type, rw->row_to_address[row]+col))
 {
     if((text=gtk_entry_get_text (GTK_ENTRY(rw->entry))))
	 gtk_entry_set_text(sheet_entry, text);
 }

}

/* when we have new data in the entry above the sheet, we
 copy the data to the cells/registers

 this don't get called when it is clicked
 in, only when we hit return
 */
static void
activate_sheet_entry(GtkWidget *widget, Register_Window *rw)
{
  GtkSheet *sheet;

  gint row, col;

  if(widget==NULL|| rw==NULL)
  {
      printf("Warning activate_sheet_entry(%p,%p)\n",widget,rw);
      return;
  }
  
  sheet=GTK_SHEET(rw->register_sheet);
  row=sheet->active_cell.row; col=sheet->active_cell.col;

  // if there are text written in the entry above the sheet, then
  // the same data is in the sheet cell (because of show_sheet_entry())

  // so we use set_cell() to write the changes from the sheet cell to gpsim
  set_cell(GTK_WIDGET(sheet),row,col,rw);
  update_ascii(rw,row);
      
}

/*
 we get here when the entry in a cell is changed (typed a digit), we
 copy it to the entry above the sheet.
 */
static void
show_entry(GtkWidget *widget, Register_Window *rw)
{
    if(widget==NULL|| rw==NULL)
    {
	printf("Warning show_entry(%p,%p)\n",widget,rw);
	return;
    }
    
    if(!GTK_WIDGET_HAS_FOCUS(widget)) return;
    
    rw->UpdateEntry();

}

/* when the sheet cursor has activated a new cell, we set the
   label and entry above the sheet
 */
static gint
activate_sheet_cell(GtkWidget *widget, gint row, gint column, Register_Window *rw) 
{

    GtkSheet *sheet=NULL;
    int regnumber;
    
    if(rw)
	sheet=rw->register_sheet;

    if(widget==NULL || row>sheet->maxrow || row<0||
       column>sheet->maxcol || column<0 || rw==NULL)
    {
	printf("Warning activate_sheet_cell(%p,%x,%x,%p)\n",widget,row,column,rw);
	return 0;
    }

    regnumber = rw->row_to_address[row]+column;

    if(!gpsim_get_register_name(rw->gp->pic_id, rw->type, regnumber))
    {
	// disable editing invalid cells
	gtk_entry_set_editable(GTK_ENTRY(gtk_sheet_get_entry(rw->register_sheet)), 0);
    }
    else
    {
	// enable editing valid cells
	gtk_entry_set_editable(GTK_ENTRY(gtk_sheet_get_entry(rw->register_sheet)), 1);
    }
    
    rw->UpdateLabelEntry();

  return TRUE;
}


void Register_Window::SelectRegister(int regnumber)
{
  GtkSheetRange range;
  int row, col;
    
  if(regnumber > MAX_REGISTERS || regnumber<0) {
    printf("Warning: %s - regnumber = %x\n",__FUNCTION__,regnumber);
    return;
  }
  
  if(registers[regnumber] == NULL)
      return;
  
  row=registers[regnumber]->row;
  col=registers[regnumber]->col;
  range.row0=range.rowi=row;
  range.col0=range.coli=col;
  gtk_sheet_select_range(GTK_SHEET(register_sheet),&range);
  if(GTK_SHEET(register_sheet)->view.col0>range.col0 ||
     GTK_SHEET(register_sheet)->view.coli<range.coli ||
     GTK_SHEET(register_sheet)->view.row0>range.row0 ||
     GTK_SHEET(register_sheet)->view.rowi<range.rowi)
    gtk_sheet_moveto(GTK_SHEET(register_sheet),row,col,0.5,0.5);

  UpdateLabelEntry();
    
}

static void
build_entry_bar(GtkWidget *main_vbox, Register_Window *rw)
{
  GtkRequisition request; 
  GtkWidget *status_box;
  
  if(main_vbox == NULL || rw==NULL)
  {
      printf("Warning build_entry_bar(%p,%p)\n",main_vbox,rw);
      return;
  }
  
  status_box=gtk_hbox_new(FALSE, 1);
  gtk_container_set_border_width(GTK_CONTAINER(status_box),0);
  gtk_box_pack_start(GTK_BOX(main_vbox), status_box, FALSE, TRUE, 0);
  gtk_widget_show(status_box);

  rw->location=gtk_label_new("");
  gtk_widget_size_request(rw->location, &request);
  gtk_widget_set_usize(rw->location, 160, request.height);
  gtk_box_pack_start(GTK_BOX(status_box), rw->location, FALSE, TRUE, 0);
  GTK_WIDGET_SET_FLAGS(rw->location,GTK_CAN_DEFAULT);
  gtk_widget_show(rw->location);

  rw->entry=gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(status_box), rw->entry,
		     TRUE, TRUE, 0); 
  gtk_widget_show(rw->entry);

}

static void update_ascii(Register_Window *rw, gint row)
{
  gint i;
  gchar name[32];

  if(rw == NULL || row<0 || row > rw->register_sheet->maxrow)
  {
      printf("Warning update_ascii(%p,%x)\n",rw,row);
      return;
  }

  if(!rw->registers_loaded)
      return;
  
  if(row<0 || row>rw->register_sheet->maxrow)
      return;
  
  for(i=0; i<REGISTERS_PER_ROW; i++)
    {

	name[i] = rw->registers[rw->row_to_address[row] + i]->value;

	if( (name[i] < ' ') || (name[i]>'z'))
	    name[i] = '.';

    }

  name[REGISTERS_PER_ROW] = 0;

  gtk_sheet_set_cell(GTK_SHEET(rw->register_sheet), row,REGISTERS_PER_ROW, GTK_JUSTIFY_RIGHT,name);

}

gboolean Register_Window::UpdateRegisterCell(unsigned int reg_number)

{
  gchar name[16];

  GtkSheetRange range;
  gboolean retval=FALSE;

  int new_value;
  int last_value;
  int valid_register=0;
  
  if(reg_number<0 || reg_number>MAX_REGISTERS)
  {
      printf("Warning update_register_cell(%x)\n",reg_number);
      return 0;
  }
  
  if(!enabled) 
    return 0;	   // Don't read registers when hidden. Esp with ICD.
  
  
  if((reg_number >= MAX_REGISTERS) || 
     (reg_number >= gpsim_get_register_memory_size(gp->pic_id,type)))
    return 0;

  range.row0=registers[reg_number]->row;
  range.rowi=registers[reg_number]->row;
  range.col0=registers[reg_number]->col;
  range.coli=registers[reg_number]->col;

  gpsim_set_bulk_mode(1);
  new_value=gpsim_get_register_value(gp->pic_id, type,reg_number);
  gpsim_set_bulk_mode(0);
  last_value=registers[reg_number]->value;
  if(gpsim_get_register_name(gp->pic_id, type,reg_number))
      valid_register=1;
  
  if(registers[reg_number]->update_full) {

    registers[reg_number]->update_full=FALSE;
      
    if(valid_register) {

      if(new_value==INVALID_VALUE)
	sprintf (name, "??");
      else
	sprintf (name, "%02x", new_value);
    }
    else {
      new_value=-1; // magic value
      strcpy(name, "");
    }

    if(registers[reg_number]->row<=register_sheet->maxrow) {

      gtk_sheet_set_cell(GTK_SHEET(register_sheet),
			 registers[reg_number]->row,
			 registers[reg_number]->col,
			 GTK_JUSTIFY_RIGHT,name);
    }
    // else the register is invalid and out of the register sheet
 

    if(new_value != last_value) {

      registers[reg_number]->value = new_value;
      registers[reg_number]->update_full=TRUE;
      gtk_sheet_range_set_foreground(GTK_SHEET(register_sheet), &range, &item_has_changed_color);
    } else
      gtk_sheet_range_set_foreground(GTK_SHEET(register_sheet), &range, &normal_fg_color);

    if(gpsim_reg_has_breakpoint(gp->pic_id, type, reg_number))
      gtk_sheet_range_set_background(GTK_SHEET(register_sheet), &range, &breakpoint_color);
    else if(!valid_register)
      gtk_sheet_range_set_background(GTK_SHEET(register_sheet), &range, &invalid_color);
    else if(gpsim_register_is_alias(gp->pic_id, type, reg_number))
      gtk_sheet_range_set_background(GTK_SHEET(register_sheet), &range, &alias_color);
    else {
      if(gpsim_register_is_sfr(gp->pic_id, type, reg_number))
	gtk_sheet_range_set_background(GTK_SHEET(register_sheet), &range, &sfr_bg_color);
      else
	gtk_sheet_range_set_background(GTK_SHEET(register_sheet), &range, &normal_bg_color);
    }

    retval=TRUE;
  } else if(new_value!=last_value) {

    if(new_value==INVALID_VALUE) {
      
      registers[reg_number]->value = -1;
      sprintf (name, "??");
    } else {

      // the register is changed since last update
      registers[reg_number]->value = new_value;
      sprintf (name, "%02x", new_value);
    }

    gtk_sheet_set_cell(GTK_SHEET(register_sheet),
		       registers[reg_number]->row,
		       registers[reg_number]->col,
		       GTK_JUSTIFY_RIGHT,name);

    registers[reg_number]->update_full=TRUE;
    gtk_sheet_range_set_foreground(GTK_SHEET(register_sheet), &range, &item_has_changed_color);

    retval=TRUE;
  }

  if(reg_number==(row_to_address[register_sheet->active_cell.row]+
		  register_sheet->active_cell.col))
  {
    // if sheet cursor is standing on a cell that is changed, then
    // we update the entry above the sheet
    if(new_value!=last_value)
      UpdateEntry();
  }

  return retval;
}


void Register_Window::Update(void)
{

  int address;
  gboolean row_changed;
  int j, i;

  if(!enabled)
    return;
    
  if(!GTK_WIDGET_VISIBLE(window))
    return;

  if(!registers_loaded)
    return;
  
  if(gp==NULL || gp->pic_id==0 || register_sheet==NULL) {
    puts("Warning gp or gp->pic_id == NULL in RegWindow_update");
    return;
  }


  for(j = 0; j<=GTK_SHEET(register_sheet)->maxrow; j++) {

    if(row_to_address[j]==-1)
      continue;
    row_changed = FALSE;
    for(i = 0; i<REGISTERS_PER_ROW; i++) {
      address = row_to_address[j]+i;
      if(registers[address]->value!=-1 || registers[address]->update_full) {

	if(UpdateRegisterCell(row_to_address[j]+i) == TRUE)
	  row_changed = TRUE;
      }
    }
    if(row_changed)
      update_ascii(this,j);
  }
}


static void xref_update_cell(struct cross_reference_to_gui *xref, int new_value)
{
  GUIRegister *reg;
  Register_Window *rw;
  int address;

  if(xref == NULL)
  {
      printf("Warning update_register_cell: xref=%p\n",xref);
      return;
  }
  
  reg = (GUIRegister *) (xref->data);
  rw  = (Register_Window *) (xref->parent_window);

  if(reg->row > GTK_SHEET(rw->register_sheet)->maxrow)
  {
      puts("Warning reg->row > maxrow in xref_update_cell");
      return;
  }

  address = rw->row_to_address[reg->row]+reg->col;

  rw->registers[address]->update_full=TRUE;
  rw->UpdateRegisterCell(address);
  
  update_ascii(rw,reg->row);
}

static void xref_remove_cell(struct cross_reference_to_gui *xref)
{
  if(xref == NULL)
    return;

  if(verbose)
    printf("%s() doesn't do anything\n", __FUNCTION__);

}

void Register_Window::NewProcessor(GUI_Processor *gp)
{


#define NAME_SIZE 32
  gint i,j,reg_number, border_mask, border_width;
  struct cross_reference_to_gui *cross_reference;
  gboolean row_created;
  GtkSheetRange range;
  int pic_id;
  int row_height, char_width;
    
  if(gp == NULL)
    return;

  has_processor=true;
    
  if( !enabled)
    return;
    
  pic_id = gp->pic_id;

  for(i=0;i<MAX_REGISTERS;i++){
    if(registers[i]!=NULL)
      free(registers[i]);
    registers[i]=NULL;
  }

  if(register_sheet == NULL){
    printf("Warning %s:%d\n",__FUNCTION__,__LINE__);
    return;
  }
      
  row_created=FALSE;

  gtk_sheet_freeze(register_sheet);
    
  j=0;
#if GTK_MAJOR_VERSION >= 2
  char_width = gdk_string_width(gdk_font_from_description(normalfont), "9");
#else
  char_width = gdk_string_width (normalfont,"9");
#endif
  row_height = 3 * char_width + 6;
  gtk_sheet_set_row_height (register_sheet, j, row_height);
  for(reg_number=0;reg_number<gpsim_get_register_memory_size(pic_id, type);reg_number++) {
    i=reg_number%REGISTERS_PER_ROW;
	
    if(i==0 && row_created)
      {
	j++;
	row_created=FALSE;
      }
	
    registers[reg_number] = new GUIRegister; //(GUIRegister  *)malloc(sizeof(GUIRegister));
    registers[reg_number]->row = j;
    registers[reg_number]->col = i;
    registers[reg_number]->value = -1;
    registers[reg_number]->update_full=TRUE;
    if(gpsim_get_register_name (pic_id, type,reg_number)) {

      gpsim_set_bulk_mode(1);
      registers[reg_number]->value = gpsim_get_register_value(pic_id, type,reg_number);
      gpsim_set_bulk_mode(0);

      /* Now create a cross-reference link that the simulator can use to
       * send information back to the gui
       */

      cross_reference = (struct cross_reference_to_gui *) malloc(sizeof(struct cross_reference_to_gui));
      cross_reference->parent_window_type =   WT_register_window;
      cross_reference->parent_window = (gpointer) this;
      cross_reference->data = (gpointer) registers[reg_number];
      cross_reference->update = xref_update_cell;
      cross_reference->remove = xref_remove_cell;
      gpsim_assign_register_xref(pic_id, type, reg_number, (gpointer) cross_reference);

      if(!row_created)
	{
	  char row_label[100];
	  if(register_sheet->maxrow<j)
	    {
	      gtk_sheet_add_row(register_sheet,1);
	      gtk_sheet_set_row_height (register_sheet, j, row_height);
	    }

	  sprintf(row_label,"%x0",reg_number/REGISTERS_PER_ROW);
	  gtk_sheet_row_button_add_label(register_sheet, j, row_label);
	  gtk_sheet_set_row_title(register_sheet, j, row_label);

	  row_to_address[j] = reg_number - reg_number%REGISTERS_PER_ROW;
	  row_created=TRUE;
	}
    }
  }

  if(j < register_sheet->maxrow)
    gtk_sheet_delete_rows(register_sheet,j,register_sheet->maxrow-j);

  registers_loaded = 1;
    
  range.row0=0;
  range.rowi=register_sheet->maxrow;
  range.col0=0;
  range.coli=register_sheet->maxcol;

  gtk_sheet_range_set_font(register_sheet, &range, normalfont);

  border_mask = GTK_SHEET_RIGHT_BORDER |
    GTK_SHEET_LEFT_BORDER |
    GTK_SHEET_BOTTOM_BORDER |
    GTK_SHEET_TOP_BORDER;

  border_width = 1;

  gtk_sheet_range_set_border(register_sheet, &range, border_mask, border_width, 0);

  border_mask = GTK_SHEET_LEFT_BORDER;
  border_width = 3;

  range.col0=REGISTERS_PER_ROW;
  range.coli=REGISTERS_PER_ROW;

  gtk_sheet_range_set_border(register_sheet, &range, border_mask, border_width, 0);

  // set values in the sheet
  Update();

  gtk_sheet_thaw(register_sheet);
    
  SelectRegister(0);

}

static int show_event(GtkWidget *widget,
                        Register_Window *rw)
{
  rw->Update();
  return TRUE;
}

static int delete_event(GtkWidget *widget,
			GdkEvent  *event,
                        Register_Window *rw)
{

  rw->ChangeView(VIEW_HIDE);
  return TRUE;
}

//------------------------------------------------------------------------
// Build
//
//

void Register_Window::Build(void)
{
  GtkWidget *main_vbox;
  GtkWidget *scrolled_window;

#define MAXROWS  (MAX_REGISTERS/REGISTERS_PER_ROW)
#define MAXCOLS  (REGISTERS_PER_ROW+1)
    
  gchar buffer[10];
  gint i;
  gint column_width,char_width;

  char *fontstring;

  if(window!=NULL) {

    gtk_widget_destroy(window);
    for(i=0;i<MAX_REGISTERS;i++)
      {
	if(registers[i]!=NULL)
	  free(registers[i]);
	registers[i]=NULL;
      }
  }
	
  window=gtk_window_new(GTK_WINDOW_TOPLEVEL);

  main_vbox=gtk_vbox_new(FALSE,1);
  gtk_container_set_border_width(GTK_CONTAINER(main_vbox),0); 
  gtk_container_add(GTK_CONTAINER(window), main_vbox);
  gtk_widget_show(main_vbox);

  if(type==REGISTER_RAM)
  {
    register_sheet=GTK_SHEET(gtk_sheet_new(1,MAXCOLS,"gpsim Register Viewer [RAM]"));
    gtk_window_set_title(GTK_WINDOW(window), "register viewer [RAM]");
    // Add a status bar
    //StatusBar_create(main_vbox,gp->status_bar);
    gp->status_bar->Create(main_vbox);
  }
  else
  {
    register_sheet=GTK_SHEET(gtk_sheet_new(1,MAXCOLS,"gpsim Register Viewer [EEPROM]"));
    gtk_window_set_title(GTK_WINDOW(window), "register viewer [EEPROM]");
  }
    
  GTK_WIDGET_UNSET_FLAGS(register_sheet,GTK_CAN_DEFAULT);

  /* create popupmenu */
  popup_menu=build_menu(this);

  build_entry_bar(main_vbox,this);

  gtk_window_set_default_size(GTK_WINDOW(window), width,height);
  gtk_widget_set_uposition(GTK_WIDGET(window),x,y);
  gtk_window_set_wmclass(GTK_WINDOW(window),name,"Gpsim");

  /**************************** load fonts *********************************/
#if GTK_MAJOR_VERSION >= 2
#define DEFAULT_NORMALFONT "Courier 12"
#else
#define DEFAULT_NORMALFONT "-adobe-courier-*-r-*-*-*-140-*-*-*-*-*-*"
#endif
  strcpy(normalfont_string,DEFAULT_NORMALFONT);
  if(config_get_string(name,"normalfont",&fontstring))
      strcpy(normalfont_string,fontstring);

  while(!load_styles(this))
  {
    if(gui_question("Some fonts did not load.","Open font dialog","Try defaults")==FALSE)
      {
	strcpy(normalfont_string,DEFAULT_NORMALFONT);
	config_set_string(name,"normalfont",normalfont_string);
      }
      else
      {
	settings_dialog(this);
      }
  }

  gtk_signal_connect(GTK_OBJECT (window), "delete_event",
		     GTK_SIGNAL_FUNC(delete_event), this);

  gtk_signal_connect(GTK_OBJECT (window), "show",
		  GTK_SIGNAL_FUNC(show_event), this);

  scrolled_window=gtk_scrolled_window_new(NULL, NULL);

  gtk_container_add(GTK_CONTAINER(scrolled_window), GTK_WIDGET(register_sheet));
  
#if GTK_MAJOR_VERSION >= 2
  GTK_SHEET_CLIP_TEXT(register_sheet);
#else
  GTK_SHEET_SET_FLAGS(register_sheet, GTK_SHEET_CLIP_TEXT);
#endif

  gtk_widget_show(GTK_WIDGET(register_sheet));

  gtk_widget_show(scrolled_window);

  gtk_box_pack_start(GTK_BOX(main_vbox), scrolled_window, TRUE, TRUE, 0);

  gtk_signal_connect(GTK_OBJECT(gtk_sheet_get_entry(GTK_SHEET(register_sheet))),
		     "changed", (GtkSignalFunc)show_entry, this);

  gtk_signal_connect(GTK_OBJECT(register_sheet),
		     "activate", (GtkSignalFunc)activate_sheet_cell,
		     this);

  gtk_signal_connect(GTK_OBJECT(entry),
		     "changed", (GtkSignalFunc)show_sheet_entry, this);

  gtk_signal_connect(GTK_OBJECT(entry),
		     "activate", (GtkSignalFunc)activate_sheet_entry,
		     this);

//  gtk_widget_realize(window);

#if GTK_MAJOR_VERSION >= 2
  char_width = gdk_string_width(gdk_font_from_description(normalfont), "9");
#else
  char_width = gdk_string_width (normalfont,"9");
#endif
  column_width = 3 * char_width + 6;

  for(i=0; i<register_sheet->maxcol; i++){

    sprintf(buffer,"%02x",i);
    gtk_sheet_column_button_add_label(register_sheet, i, buffer);
    gtk_sheet_set_column_title(register_sheet, i, buffer);
    gtk_sheet_set_column_width (register_sheet, i, column_width);
  }

  i = REGISTERS_PER_ROW;
  sprintf(buffer,"ASCII");
  gtk_sheet_column_button_add_label(register_sheet, i, buffer);
  gtk_sheet_set_column_title(register_sheet, i, buffer);

  gtk_sheet_set_column_width (register_sheet, i, REGISTERS_PER_ROW*char_width + 6);

  gtk_sheet_set_row_titles_width(register_sheet, column_width);

  gtk_signal_connect(GTK_OBJECT(register_sheet),
		     "key_press_event",
		     (GtkSignalFunc) clipboard_handler, 
		     NULL);

  gtk_signal_connect(GTK_OBJECT(register_sheet),
		     "resize_range",
		     (GtkSignalFunc) resize_handler, 
		     this);

  gtk_signal_connect(GTK_OBJECT(register_sheet),
		     "move_range",
		     (GtkSignalFunc) move_handler, 
		     this);
	
  gtk_signal_connect(GTK_OBJECT(register_sheet),
		     "button_press_event",
		     (GtkSignalFunc) do_popup, 
		     this);

  gtk_signal_connect(GTK_OBJECT(register_sheet),
		     "set_cell",
		     (GtkSignalFunc) set_cell,
		     this);

  gtk_signal_connect_after(GTK_OBJECT(window), "configure_event",
			   GTK_SIGNAL_FUNC(gui_object_configure_event),
			   this);


  gtk_widget_show (window);

  gtk_widget_grab_default(location);
  

  enabled=1;
  
  is_built=1;
  
  for(i=0;i<MAX_REGISTERS;i++)
      registers[i]=NULL;
  
  if(has_processor)
  {
    NewProcessor(gp);
  }

  UpdateMenuItem();
}

Register_Window::Register_Window(void)
{
  printf("WARNING: calling default constructor: %s\n",__FUNCTION__);

}

Register_Window::Register_Window(GUI_Processor *_gp)
{
  int i;

  gp = _gp;
  window = NULL;
  wc = WC_data;
  wt = WT_register_window;
  is_built = 0;
  enabled = 0;

  registers_loaded=0;
  has_processor=false;
  
  registers = (GUIRegister  **)malloc(MAX_REGISTERS*sizeof(GUIRegister *));
  for(i=0;i<MAX_REGISTERS;i++)
    registers[i]=NULL;

  for(i=0;i<MAX_REGISTERS/REGISTERS_PER_ROW;i++)
    row_to_address[i]=-1;



}


RAM_RegisterWindow::RAM_RegisterWindow(GUI_Processor *_gp) :
  Register_Window(_gp)
{
  menu = "<main>/Windows/Ram";
  type = REGISTER_RAM;

  int i;

  name = "register_viewer_ram";
  // Add a status bar
  gp->status_bar= (StatusBar_Window *)malloc(sizeof(StatusBar_Window));
  gp->status_bar->created=0;

  get_config();

  if(enabled)
      Build();
}




EEPROM_RegisterWindow::EEPROM_RegisterWindow(GUI_Processor *_gp) :
  Register_Window(_gp)
{
  menu = "<main>/Windows/EEPROM";
  type = REGISTER_EEPROM;

  name = "register_viewer_eeprom";
  
  get_config();

  if(enabled)
      Build();
}


#endif // HAVE_GUI
