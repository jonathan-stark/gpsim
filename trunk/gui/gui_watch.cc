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
#include <sys/errno.h>

#include "../config.h"
#ifdef HAVE_GUI

#include <unistd.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <string.h>

#include <assert.h>

#include "../src/interface.h"

#include "gui.h"

struct watch_entry {
    unsigned int pic_id;
    REGISTER_TYPE type;
    unsigned int address;
    struct cross_reference_to_gui *xref;
    int last_value;
};

#define COLUMNS 15
#define BPCOL 0
#define NAMECOL 2
#define DECIMALCOL 4
#define HEXCOL 5
#define ASCIICOL 6
#define MSBCOL 7
#define LSBCOL 14
static char *watch_titles[COLUMNS]={"bp?", "type", "name","address","dec","hex","ascii","b7","b6","b5","b4","b3","b2","b1","b0"};

struct _coldata{
    GtkWidget *clist;
    int column;
    int visible; // Loaded on startup
    Watch_Window *ww;
} coldata[COLUMNS];
static void select_columns(Watch_Window *ww, GtkWidget *clist);


typedef enum {
    MENU_REMOVE,
    MENU_SET_VALUE,
    MENU_BREAK_CLEAR,
    MENU_BREAK_READ,
    MENU_BREAK_WRITE,
    MENU_BREAK_READ_VALUE,
    MENU_BREAK_WRITE_VALUE,
    MENU_COLUMNS,
} menu_id;


typedef struct _menu_item {
    char *name;
    menu_id id;
    GtkWidget *item;
} menu_item;

static menu_item menu_items[] = {
    {"Remove watch", MENU_REMOVE},
    {"Set value...", MENU_SET_VALUE},
    {"Clear breakpoints", MENU_BREAK_CLEAR},
    {"Set break on read", MENU_BREAK_READ},
    {"Set break on write", MENU_BREAK_WRITE},
    {"Set break on read value...", MENU_BREAK_READ_VALUE},
    {"Set break on write value...", MENU_BREAK_WRITE_VALUE},
    {"Columns...", MENU_COLUMNS},
};

// Used only in popup menus
Watch_Window *popup_ww;

static void remove_entry(Watch_Window *ww, struct watch_entry *entry)
{
    gtk_clist_remove(GTK_CLIST(ww->watch_clist),ww->current_row);
    ww->watches=g_list_remove(ww->watches,entry);
    gpsim_clear_register_xref(entry->pic_id, entry->type, entry->address, entry->xref);
    free(entry);
}

static void update_menus(Watch_Window *ww)
{
    GtkWidget *item;
    struct watch_entry *entry;
    int i;

    for (i=0; i < (sizeof(menu_items)/sizeof(menu_items[0])) ; i++){
	item=menu_items[i].item;
	if(menu_items[i].id!=MENU_COLUMNS)
	{
	    if(ww)
	    {
		entry = (struct watch_entry*) gtk_clist_get_row_data(GTK_CLIST(ww->watch_clist),ww->current_row);
		if(menu_items[i].id!=MENU_COLUMNS && (entry==NULL ||
						      (entry->type==REGISTER_EEPROM && menu_items[i].id==MENU_BREAK_CLEAR)||
						      (entry->type==REGISTER_EEPROM && menu_items[i].id==MENU_BREAK_READ)||
						      (entry->type==REGISTER_EEPROM && menu_items[i].id==MENU_BREAK_WRITE)||
						      (entry->type==REGISTER_EEPROM && menu_items[i].id==MENU_BREAK_READ_VALUE)||
						      (entry->type==REGISTER_EEPROM && menu_items[i].id==MENU_BREAK_WRITE_VALUE)
						     ))
		    gtk_widget_set_sensitive (item, FALSE);
		else
		    gtk_widget_set_sensitive (item, TRUE);
	    }
	    else
	    {
		gtk_widget_set_sensitive (item, FALSE);
	    }
	}
    }
}

static void unselect_row(GtkCList *clist,
			 gint row,
			 gint column,
			 GdkEvent *event,
			 Watch_Window *ww)
{
    update_menus(NULL);
}

// called when user has selected a menu item
static void
popup_activated(GtkWidget *widget, gpointer data)
{
    menu_item *item;

    struct watch_entry *entry;

    unsigned int pic_id;
    int value;

    if(widget==NULL || data==NULL)
    {
	printf("Warning popup_activated(%p,%p)\n",widget,data);
	return;
    }
    
    item = (menu_item *)data;
    pic_id = ((GUI_Object*)popup_ww)->gp->pic_id;

    entry = (struct watch_entry*) gtk_clist_get_row_data(GTK_CLIST(popup_ww->watch_clist),popup_ww->current_row);

    if(entry==NULL && item->id!=MENU_COLUMNS)
	return;

    switch(item->id)
    {
    case MENU_REMOVE:
	remove_entry(popup_ww,entry);
	break;
    case MENU_SET_VALUE:
	value = gui_get_value("value:");
	if(value<0)
	    break; // Cancel
	gpsim_put_register_value(entry->pic_id,entry->type,entry->address, value);
	break;
    case MENU_BREAK_READ:
	gpsim_reg_set_read_breakpoint(entry->pic_id, entry->type, entry->address);
	break;
    case MENU_BREAK_WRITE:
	gpsim_reg_set_write_breakpoint(entry->pic_id, entry->type, entry->address);
	break;
    case MENU_BREAK_READ_VALUE:
	value = gui_get_value("value to read for breakpoint:");
	if(value<0)
	    break; // Cancel
	gpsim_reg_set_read_value_breakpoint(entry->pic_id, entry->type, entry->address, value);
	break;
    case MENU_BREAK_WRITE_VALUE:
	value = gui_get_value("value to write for breakpoint:");
	if(value<0)
	    break; // Cancel
	gpsim_reg_set_write_value_breakpoint(entry->pic_id, entry->type, entry->address, value);
	break;
    case MENU_BREAK_CLEAR:
	gpsim_reg_clear_breakpoints(entry->pic_id, entry->type,entry->address);
	break;
    case MENU_COLUMNS:
        select_columns(popup_ww, popup_ww->watch_clist);
	break;
    default:
	puts("Unhandled menuitem?");
	break;
    }
}

static void set_column(GtkCheckButton *button, struct _coldata *coldata)
{
    char str[256];
    if(button->toggle_button.active)
	gtk_clist_set_column_visibility(GTK_CLIST(coldata->clist),coldata->column,1);
    else
	gtk_clist_set_column_visibility(GTK_CLIST(coldata->clist),coldata->column,0);
    sprintf(str,"show_column%d",coldata->column);
    config_set_variable(coldata->ww->name,str,button->toggle_button.active);
}

static void select_columns(Watch_Window *ww, GtkWidget *clist)
{
    GtkWidget *dialog=NULL;
    GtkWidget *button;
    int i;

    dialog = gtk_dialog_new();

    gtk_container_set_border_width(GTK_CONTAINER(dialog),30);

    gtk_signal_connect_object(GTK_OBJECT(dialog),
			      "delete_event",GTK_SIGNAL_FUNC(gtk_widget_destroy),GTK_OBJECT(dialog));

    for(i=0;i<COLUMNS;i++)
    {
	button=gtk_check_button_new_with_label(watch_titles[i]);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),coldata[i].visible);
	gtk_widget_show(button);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),button,FALSE,FALSE,0);
	coldata[i].clist=clist;
	coldata[i].column=i;
	coldata[i].ww=ww;
	gtk_signal_connect(GTK_OBJECT(button),"clicked",
			   GTK_SIGNAL_FUNC(set_column),(gpointer)&coldata[i]);
    }

    button = gtk_button_new_with_label("OK");
    gtk_widget_show(button);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button,
		       FALSE,FALSE,10);
    gtk_signal_connect_object(GTK_OBJECT(button),"clicked",
			      GTK_SIGNAL_FUNC(gtk_widget_destroy),GTK_OBJECT(dialog));
    GTK_WIDGET_SET_FLAGS(button,GTK_CAN_DEFAULT);
    gtk_widget_grab_default(button);

    gtk_widget_show(dialog);
    
    return;
}

// helper function, called from do_popup
static GtkWidget *
build_menu(GtkWidget *sheet, Watch_Window *ww)
{
  GtkWidget *menu;
  GtkWidget *item;
  int i;


  if(sheet==NULL || ww==NULL)
  {
      printf("Warning build_menu(%p,%p)\n",sheet,ww);
      return NULL;
  }
    
  popup_ww = ww;
  
  menu=gtk_menu_new();

  item = gtk_tearoff_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), item);
  gtk_widget_show (item);
  
  for (i=0; i < (sizeof(menu_items)/sizeof(menu_items[0])) ; i++){
      menu_items[i].item=item=gtk_menu_item_new_with_label(menu_items[i].name);

      gtk_signal_connect(GTK_OBJECT(item),"activate",
			 (GtkSignalFunc) popup_activated,
			 &menu_items[i]);
//      GTK_WIDGET_UNSET_FLAGS (item, GTK_SENSITIVE | GTK_CAN_FOCUS);

/*      entry = (struct watch_entry*) gtk_clist_get_row_data(GTK_CLIST(popup_ww->watch_clist),popup_ww->current_row);

      if(menu_items[i].id!=MENU_COLUMNS && (
	 entry==NULL ||
	 (entry->type==REGISTER_EEPROM && menu_items[i].id==MENU_BREAK_CLEAR)||
	 (entry->type==REGISTER_EEPROM && menu_items[i].id==MENU_BREAK_READ)||
	 (entry->type==REGISTER_EEPROM && menu_items[i].id==MENU_BREAK_WRITE)||
	 (entry->type==REGISTER_EEPROM && menu_items[i].id==MENU_BREAK_READ_VALUE)||
	 (entry->type==REGISTER_EEPROM && menu_items[i].id==MENU_BREAK_WRITE_VALUE)
	))
	  GTK_WIDGET_UNSET_FLAGS (item,
				  GTK_SENSITIVE | GTK_CAN_FOCUS);
*/
      
/*      if(ww->type == REGISTER_EEPROM)
      {
	  GTK_WIDGET_UNSET_FLAGS (item,
				  GTK_SENSITIVE | GTK_CAN_FOCUS);
      }*/
/*      switch(menu_items[i].id){
      case MENU_BREAK_READ:
      case MENU_BREAK_WRITE:
      case MENU_BREAK_CLEAR:
	  break;
      default:
          GTK_WIDGET_UNSET_FLAGS (item,
             GTK_SENSITIVE | GTK_CAN_FOCUS);
	  break;
      }*/
      
      gtk_widget_show(item);
      gtk_menu_append(GTK_MENU(menu),item);
  }

  update_menus(ww);
  
  return menu;
}

// button press handler
static gint
do_popup(GtkWidget *widget, GdkEventButton *event, Watch_Window *ww)
{

    GtkWidget *popup;
//	GdkModifierType mods;

  if(widget==NULL || event==NULL || ww==NULL)
  {
      printf("Warning do_popup(%p,%p,%p)\n",widget,event,ww);
      return 0;
  }
  popup=ww->popup_menu;
    if( (event->type == GDK_BUTTON_PRESS) &&  (event->button == 3) )
    {

      gtk_menu_popup(GTK_MENU(popup), NULL, NULL, NULL, NULL,
		     3, event->time);
    }
    return FALSE;
}

static gint
key_press(GtkWidget *widget,
	  GdkEventKey *key, 
	  gpointer data)
{

    struct watch_entry *entry;
    Watch_Window *ww = (Watch_Window *) data;

  if(!ww) return(FALSE);
  if(!ww->gp) return(FALSE);
  if(!ww->gp->pic_id) return(FALSE);

  switch(key->keyval) {

  case GDK_Delete:
      entry = (struct watch_entry*) gtk_clist_get_row_data(GTK_CLIST(ww->watch_clist),ww->current_row);
      if(entry!=NULL)
	  remove_entry(ww,entry);
      break;
  }
  return TRUE;
}

/*
 */
static gint sigh_button_event(GtkWidget *widget,
		       GdkEventButton *event,
		       Watch_Window *ww)
{
    struct watch_entry *entry;
    assert(event&&ww);

    if(event->type==GDK_2BUTTON_PRESS &&
       event->button==1)
    {
	int column=ww->current_column;
	int row=ww->current_row;
	
	entry = (struct watch_entry*) gtk_clist_get_row_data(GTK_CLIST(ww->watch_clist), row);
    
	if(column>=MSBCOL && column<=LSBCOL)
	{
	  int value;  // , bit;
	    
	    // Toggle the bit.
	    value = gpsim_get_register_value(entry->pic_id,entry->type, entry->address);
	    //bit = (value &(1<< (7-(column-MSBCOL)) ))?1:0;
	    //bit=!bit;
	    //
	    //if(bit)
	    //    value |= (1<< (7-(column-MSBCOL)) );
	    //else
	    //    value &= ~(value &(1<< (7-(column-MSBCOL)) ));

	    value ^= (1<< (7-(column-MSBCOL)));
	    gpsim_put_register_value(entry->pic_id,entry->type, entry->address,value);
	}
    }

    return 0;
}

static gint watch_list_row_selected(GtkCList *watchlist,gint row, gint column,GdkEvent *event, Watch_Window *ww)
{
    struct watch_entry *entry;
    //    int bit;
    GUI_Processor *gp;
    
    ww->current_row=row;
    ww->current_column=column;

    gp=ww->gp;
    
    entry = (struct watch_entry*) gtk_clist_get_row_data(GTK_CLIST(ww->watch_clist), row);

    if(!entry)
	return TRUE;
    
    if(entry->type==REGISTER_RAM)
    {
	gp->regwin_ram->SelectRegister(entry->address);
    }
    else if(entry->type==REGISTER_EEPROM)
    {
	gp->regwin_eeprom->SelectRegister(entry->address);
    }

    update_menus(ww);
    
/*    if(column>=MSBCOL && column<=LSBCOL)
    {
	// Toggle the bit.
	value = gpsim_get_register_value(entry->pic_id,entry->type, entry->address);
	value ^= (1<< (7-(column-MSBCOL)));
	//bit = (value &(1<< (7-(column-MSBCOL)) ))?1:0;
	//bit=!bit;
	//if(bit)
	//    value |= (1<< (7-(column-MSBCOL)) );
	//else
	//    value &= ~(value &(1<< (7-(column-MSBCOL)) ));
	gpsim_put_register_value(entry->pic_id,entry->type, entry->address,value);
    }*/
    return 0;
}

static void watch_click_column(GtkCList *clist, int column)
{
    static int last_col=-1;
    static GtkSortType last_sort_type=GTK_SORT_DESCENDING;
    
    if(last_col==-1)
	last_col=column;

    if(last_col == column)
    {
	if(last_sort_type==GTK_SORT_DESCENDING)
	{
	    gtk_clist_set_sort_type(clist,GTK_SORT_ASCENDING);
	    last_sort_type=GTK_SORT_ASCENDING;
	}
	else
	{
	    gtk_clist_set_sort_type(clist,GTK_SORT_DESCENDING);
	    last_sort_type=GTK_SORT_DESCENDING;
	}
    }

    gtk_clist_set_sort_column(clist,column);
    gtk_clist_sort(clist);
    last_col=column;
}

static int delete_event(GtkWidget *widget,
			GdkEvent  *event,
                        Watch_Window *ww)
{
  ww->ChangeView(VIEW_HIDE);
  return TRUE;
}

static void update(Watch_Window *ww, struct watch_entry *entry, int new_value)
{
    char str[80];
    int i;

    int row;

    row=gtk_clist_find_row_from_data(GTK_CLIST(ww->watch_clist),entry);
    if(row==-1)
    {
	puts("\n\nwhooopsie\n");
	return;
    }

    sprintf(str,"%d",new_value);
    gtk_clist_set_text(GTK_CLIST(ww->watch_clist), row, DECIMALCOL, str);

    sprintf(str,"0x%02x",new_value);
    gtk_clist_set_text(GTK_CLIST(ww->watch_clist), row, HEXCOL, str);

    if(new_value>=32)
	sprintf(str,"%c",new_value);
    else
        str[0]=0;
    gtk_clist_set_text(GTK_CLIST(ww->watch_clist), row, ASCIICOL, str);

    for(i=7;i>=0;i--)
    {
      //int bit; 

	//bit=new_value/(1<<i);

	//sprintf(str, "%d", bit);
        gtk_clist_set_text(GTK_CLIST(ww->watch_clist), row, MSBCOL+ i, //(7-i), str);
			   ((new_value&1) ? "1" : "0"));
	new_value >>= 1;
	//new_value%=(1<<i);
    }
    if(gpsim_reg_has_breakpoint(entry->pic_id, entry->type, entry->address))
	gtk_clist_set_text(GTK_CLIST(ww->watch_clist), row, BPCOL, "yes");
    else
	gtk_clist_set_text(GTK_CLIST(ww->watch_clist), row, BPCOL, "no");
}

//------------------------------------------------------------------------
// Update
//
//

void Watch_Window::Update(void)
{
  GList *iter;
  struct watch_entry *entry;
  int clist_frozen=0;
  int value;

  iter=watches;

  while(iter) {
   
    entry=(struct watch_entry*)iter->data;

    value = gpsim_get_register_value(entry->pic_id,entry->type,entry->address);
	
    if(entry->last_value != value)
      {
	if(clist_frozen==0)
	  {
	    gtk_clist_freeze(GTK_CLIST(watch_clist));
	    clist_frozen=1;
	  }

	// Update value in clist
	update(this,entry,value);
	entry->last_value=value;
      }
    iter=iter->next;
  }
  if(clist_frozen)
    gtk_clist_thaw(GTK_CLIST(watch_clist));
}

static void xref_update(struct cross_reference_to_gui *xref, int new_value)
{
    struct watch_entry *entry;
    Watch_Window *ww;

    if(xref == NULL)
    {
	printf("Warning gui_watch.c: xref_update: xref=%p\n",xref);
/*	if(xref->data == NULL || xref->parent_window==NULL)
	{
	    printf("Warning gui_watch.c: xref_update: xref->data=%x, xref->parent_window=%x\n",(unsigned int)xref->data,(unsigned int)xref->parent_window);
	}
*/
	return;
    }

    entry = (struct watch_entry*) xref->data;
    ww  = (Watch_Window *) (xref->parent_window);

    //    update(ww,entry,new_value);
    ww->Update();
}

void Watch_Window::Add(unsigned int pic_id, REGISTER_TYPE type, int address)
{
  char name[50], addressstring[50], typestring[30];
  char *entry[COLUMNS]={"",typestring,name, addressstring, "", "","","","","","","","","",""};
  int row;
  struct cross_reference_to_gui *cross_reference;
  char *regname;

  struct watch_entry *watch_entry;
    
  if(!enabled)
    Build();

  regname = gpsim_get_register_name(pic_id,type,address);

  if(!regname)
    return;  // INVALID_REGISTER

  strncpy(name,regname,50);
  sprintf(addressstring,"0x%02x",address);
  strncpy(typestring,type==REGISTER_RAM?"RAM":"EEPROM",30);

  row=gtk_clist_append(GTK_CLIST(watch_clist), entry);

  // FIXME this memory is never freed?
  watch_entry = (struct watch_entry*) malloc(sizeof(struct watch_entry));
  watch_entry->address=address;
  watch_entry->pic_id=pic_id;
  watch_entry->type=type;
  watch_entry->last_value=-1; // non-normal value to force first update

  gtk_clist_set_row_data(GTK_CLIST(watch_clist), row, (gpointer)watch_entry);
    
  watches = g_list_append(watches, (gpointer)watch_entry);

  update(this, watch_entry,gpsim_get_register_value(watch_entry->pic_id,watch_entry->type,watch_entry->address) );

  cross_reference = (struct cross_reference_to_gui *) malloc(sizeof(struct cross_reference_to_gui));
  cross_reference->parent_window_type = WT_watch_window;
  cross_reference->parent_window = (gpointer) this;
  cross_reference->data = (gpointer) watch_entry;
  cross_reference->update = xref_update;
  cross_reference->remove = NULL;
  gpsim_assign_register_xref(pic_id, type, address, (gpointer) cross_reference);

  watch_entry->xref=cross_reference;
    
  update_menus(this);
}

//------------------------------------------------------------------------
// ClearWatches
//
//

void Watch_Window::ClearWatches(void)
{
  GList *iter;
  struct watch_entry *entry;
  int row;

  iter=watches;

  while(iter) {

    entry=(struct watch_entry*)iter->data;
    row=gtk_clist_find_row_from_data(GTK_CLIST(watch_clist),entry);
    gtk_clist_remove(GTK_CLIST(watch_clist),row);
    gpsim_clear_register_xref(entry->pic_id, entry->type, entry->address, entry->xref);
    free(entry);
    iter=iter->next;
  }

  while( (watches=g_list_remove_link(watches,watches))!=NULL)
    ;
}

//------------------------------------------------------------------------
// Build
//
//

void Watch_Window::Build(void)
{
  GtkWidget *vbox;
  GtkWidget *scrolled_window;

  int i;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title(GTK_WINDOW(window), "Watch Viewer");

  gtk_window_set_default_size(GTK_WINDOW(window), width,height);
  gtk_widget_set_uposition(GTK_WIDGET(window),x,y);
  gtk_window_set_wmclass(GTK_WINDOW(window),name,"Gpsim");
  
  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC(delete_event), (gpointer)this);
  gtk_signal_connect_after(GTK_OBJECT(window), "configure_event",
			   GTK_SIGNAL_FUNC(gui_object_configure_event),this);
  gtk_signal_connect_after(GTK_OBJECT(window), "button_press_event",
		     GTK_SIGNAL_FUNC(sigh_button_event), this);
  
  watch_clist = gtk_clist_new_with_titles(COLUMNS,watch_titles);
  gtk_widget_show(watch_clist);

  for(i=0;i<MSBCOL;i++) {
    gtk_clist_set_column_auto_resize(GTK_CLIST(watch_clist),i,TRUE);
    gtk_clist_set_column_visibility(GTK_CLIST(watch_clist),i,coldata[i].visible);
  }
  
  gtk_clist_set_selection_mode (GTK_CLIST(watch_clist), GTK_SELECTION_BROWSE);

  gtk_signal_connect(GTK_OBJECT(watch_clist),"click_column",
		     (GtkSignalFunc)watch_click_column,NULL);
  gtk_signal_connect(GTK_OBJECT(watch_clist),"select_row",
		     (GtkSignalFunc)watch_list_row_selected,this);
  gtk_signal_connect(GTK_OBJECT(watch_clist),"unselect_row",
		     (GtkSignalFunc)unselect_row,this);
  
  gtk_signal_connect(GTK_OBJECT(watch_clist),
		     "button_press_event",
		     (GtkSignalFunc) do_popup,
		     this);
  gtk_signal_connect(GTK_OBJECT(window),"key_press_event",
		     (GtkSignalFunc) key_press,
		     (gpointer) this);

  scrolled_window=gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_show(scrolled_window);

  vbox = gtk_vbox_new(FALSE,1);
  gtk_widget_show(vbox);
  
  gtk_container_add(GTK_CONTAINER(scrolled_window), watch_clist);
  
  gtk_container_add(GTK_CONTAINER(window),vbox);

  gtk_box_pack_start_defaults(GTK_BOX(vbox),scrolled_window);
  
  popup_menu=build_menu(window,this);
  
  gtk_widget_show (window);
  
  
  enabled=1;

  is_built=1;

  //update_menu_item((GUI_Object*)ww);
  UpdateMenuItem();

}

int Watch_Window::Create(GUI_Processor *_gp)
{
  int i;
    
#define MAXROWS  (MAX_REGISTERS/REGISTERS_PER_ROW)
#define MAXCOLS  (REGISTERS_PER_ROW+1)


  gp = _gp;
  gp->watch_window = this;


  get_config();

  for(i=0;i<COLUMNS;i++) {
    char str[128];
    sprintf(str,"show_column%d",i);
    coldata[i].visible=1; // default
    config_get_variable(name,str,&coldata[i].visible);
  }

  if(enabled)
    Build();
  
  return 1;
}

Watch_Window::Watch_Window(void)
{
  menu = "<main>/Windows/Watch";

  name = "watch_viewer";
  wc = WC_data;
  wt = WT_watch_window;
  window = NULL;
  is_built = 0;

  watches=NULL;
  current_row=0;

}

#endif // HAVE_GUI
