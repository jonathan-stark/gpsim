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

struct symbol_entry {
    unsigned int pic_id;
    unsigned int value; // symbol value
    struct cross_reference_to_gui *xref;
};

typedef enum {
    MENU_ADD_WATCH,
} menu_id;


typedef struct _menu_item {
    char *name;
    menu_id id;
    GtkWidget *item;
} menu_item;

static menu_item menu_items[] = {
    {"Add to watch window", MENU_ADD_WATCH},
};


// Used only in popup menus
Symbol_Window *popup_sw;

/*
unsigned int gpsim_reg_has_breakpoint(unsigned int processor_id, unsigned int register_number);
void  gpsim_assign_register_xref(unsigned int processor_id, unsigned int reg_number, gpointer xref);
*/


static void update_menus(Symbol_Window *sw)
{
    GtkWidget *item;
    int i;

    for (i=0; i < (sizeof(menu_items)/sizeof(menu_items[0])) ; i++){
	item=menu_items[i].item;
	if(sw)
	{
            sym *entry;
	    entry = (sym*) gtk_clist_get_row_data(GTK_CLIST(sw->symbol_clist),sw->current_row);
	    if(entry==NULL)
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

// called when user has selected a menu item
static void
popup_activated(GtkWidget *widget, gpointer data)
{
    menu_item *item;
    sym *entry;

    unsigned int pic_id;
    int value;

    if(widget==NULL || data==NULL)
    {
	printf("Warning popup_activated(%p,%p)\n",widget,data);
	return;
    }
    
    item = (menu_item *)data;
    pic_id = ((GUI_Object*)popup_sw)->gp->pic_id;

    entry = (sym*) gtk_clist_get_row_data(GTK_CLIST(popup_sw->symbol_clist),popup_sw->current_row);

    if(entry==NULL)
	return;

    switch(item->id)
    {
    case MENU_ADD_WATCH:
      //WatchWindow_add(popup_sw->gui_obj.gp->watch_window,
      popup_sw->gp->watch_window->Add(pic_id,
				      REGISTER_RAM,
				      entry->value);
	break;
    default:
	puts("Unhandled menuitem?");
	break;
    }
}

// helper function, called from do_popup
static GtkWidget *
build_menu(GtkWidget *sheet, Symbol_Window *sw)
{
  GtkWidget *menu;
  GtkWidget *item;
  int i;

  if(sheet==NULL || sw==NULL)
  {
      printf("Warning build_menu(%p,%p)\n",sheet,sw);
      return NULL;
  }
    
  popup_sw = sw;
  
  menu=gtk_menu_new();

  item = gtk_tearoff_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), item);
  gtk_widget_show (item);
  
  for (i=0; i < (sizeof(menu_items)/sizeof(menu_items[0])) ; i++){
      menu_items[i].item=item=gtk_menu_item_new_with_label(menu_items[i].name);

      gtk_signal_connect(GTK_OBJECT(item),"activate",
			 (GtkSignalFunc) popup_activated,
			 &menu_items[i]);

      gtk_widget_show(item);
      gtk_menu_append(GTK_MENU(menu),item);
  }

  update_menus(sw);
  
  return menu;
}

// button press handler
static gint
do_popup(GtkWidget *widget, GdkEventButton *event, Symbol_Window *sw)
{

    GtkWidget *popup;
//	GdkModifierType mods;

  if(widget==NULL || event==NULL || sw==NULL)
  {
      printf("Warning do_popup(%p,%p,%p)\n",widget,event,sw);
      return 0;
  }
  popup=sw->popup_menu;
    if( (event->type == GDK_BUTTON_PRESS) &&  (event->button == 3) )
    {

      gtk_menu_popup(GTK_MENU(popup), NULL, NULL, NULL, NULL,
		     3, event->time);
    }
    return FALSE;
}

static void unselect_row(GtkCList *clist,
			 gint row,
			 gint column,
			 GdkEvent *event,
			 Symbol_Window *sw)
{
    update_menus(sw);
}

void Symbol_Window::Update(void)
{

  char **entry; // 'name', 'type', 'typedata'
  sym *s;
  GList *iter;

  load_symbols=1;
    
  if(!enabled)
    return;
    

  gtk_clist_freeze(GTK_CLIST(symbol_clist));
    
  gtk_clist_clear(GTK_CLIST(symbol_clist));

  // free all old allocations
  for(iter=symbols;iter!=NULL;)
    {
      GList *next=iter->next;
      free(((sym*)iter->data)->name);
      free((sym*)iter->data);
      g_list_remove(iter,iter->data); // FIXME. I really need a tutorial
      iter=next;
      //	g_list_free_1(sa_xlate_list[id]);  // FIXME, g_list_free() difference?
    }
  symbols=NULL;

  gpsim_symbol_rewind((unsigned int)gp->pic_id);


  // FIXME memory leaks
  while(NULL != (s = gpsim_symbol_iter(gp->pic_id)))
    {
      int row;
      sym *e;
	
      if( (filter_addresses && s->type == SYMBOL_ADDRESS) ||
	  (filter_constants && s->type == SYMBOL_CONSTANT) ||
	  (filter_registers && s->type == SYMBOL_REGISTER) )
	continue;

#define SYMBOL_NR_COLUMNS 3
      entry=(char**)malloc(sizeof(char*)*SYMBOL_NR_COLUMNS);
	
      entry[0]=(char*)malloc(strlen(s->name)+1);
      strcpy(entry[0],s->name);
      entry[1]=(char*)malloc(64);
      switch(s->type)
	{
	case SYMBOL_ADDRESS:
	  strcpy(entry[1],"address");
	  break;
	case SYMBOL_CONSTANT:
	  strcpy(entry[1],"constant");
	  break;
	case SYMBOL_REGISTER:
	  strcpy(entry[1],"register");
	  break;
	case SYMBOL_IOPORT:
	  strcpy(entry[1],"ioport");
	  break;
	case SYMBOL_STIMULUS:
	  strcpy(entry[1],"stimulus");
	  break;
	case SYMBOL_BASE_CLASS:
	  strcpy(entry[1],"symbol base class");
	  break;
	default:
	  strcpy(entry[1],"unknown symbol type");
	  break;
	}
      entry[2]=(char*)malloc(32);
      if(s->type==SYMBOL_ADDRESS||
	 s->type==SYMBOL_CONSTANT||
	 s->type==SYMBOL_REGISTER)
	sprintf(entry[2],"0x%X",s->value);
      else
	strcpy(entry[2],"");
	
      e=(sym*)malloc(sizeof(sym));
      memcpy(e,s,sizeof(sym));
      e->name=(char*)malloc(strlen(s->name)+1);
      strcpy(e->name,s->name);
      symbols=g_list_append(symbols,e);

      row=gtk_clist_append(GTK_CLIST(symbol_clist),entry);
      gtk_clist_set_row_data(GTK_CLIST(symbol_clist),row,e);
    }
  gtk_clist_thaw(GTK_CLIST(symbol_clist));
}

static void do_symbol_select(Symbol_Window *sw, sym *e)
{
    // Do what is to be done when a symbol is selected.
    // Except for selecting the symbol row in the symbol_clist
    switch(e->type)
    {
    case SYMBOL_REGISTER:
	((GUI_Object*)sw)->gp->regwin_ram->SelectRegister(e->value);
	break;
    case SYMBOL_ADDRESS:
	(((GUI_Object*)sw)->gp->source_browser)->SelectAddress(e->value);
	(((GUI_Object*)sw)->gp->program_memory)->SelectAddress(e->value);
	break;
    default:
	// symbols that can't be 'selected' (e.g constants)
	break;
    }
}

static gint symbol_list_row_selected(GtkCList *symlist,gint row, gint column,GdkEvent *event, Symbol_Window *sw)
{
    sym *e=(sym*)gtk_clist_get_row_data(symlist,row);
    sw->current_row=row;
    do_symbol_select(sw,e);
    update_menus(sw);
    return 0;
}

/*
 pop up symbol window and select row with regnumber if it exists
 */
void SymbolWindow_select_symbol_regnumber(Symbol_Window *sw, int regnumber)
{
    GList *p;
    
    if(!sw->enabled)
	return;
    
    p=sw->symbols;
    while(p)
    {
	sym *e;
	e=(sym*)p->data;
	if(e->type==SYMBOL_REGISTER && e->value==regnumber)
	{
	    int row;
	    row=gtk_clist_find_row_from_data(GTK_CLIST(sw->symbol_clist),e);
	    if(row!=-1)
	    {
		gtk_clist_select_row(GTK_CLIST(sw->symbol_clist),row,0);
		gtk_clist_moveto(GTK_CLIST(sw->symbol_clist),row,0,0.5,0.5);

		do_symbol_select(sw,e);
	    }
	    break;
	}
	p=p->next;
    }
}

void SymbolWindow_select_symbol_name(Symbol_Window *sw, char *name)
{
    GList *p;
    sym *s;
    
    if(name==NULL)
	return;

    // If window is not displayed, then display it.
    if(!sw->enabled)
    {
	sw->ChangeView(VIEW_SHOW);
    }

    // See if the type of symbol selected is currently filtered out, and
    // if so we unfilter it.
    gpsim_symbol_rewind((unsigned int)gp->pic_id);
    while(NULL != (s = gpsim_symbol_iter(gp->pic_id)))
    {
	if(!strcasecmp(s->name,name))
	{
	    switch(s->type)
	    {
	    case SYMBOL_ADDRESS:
		if(sw->filter_addresses)
		{
		    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sw->addressesbutton), TRUE);
		    while(gtk_events_pending()) // FIXME. Not so nice...
                        gtk_main_iteration();
		}
		break;
	    case SYMBOL_CONSTANT:
		if(sw->filter_constants)
		{
		    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sw->constantsbutton), TRUE);
		    while(gtk_events_pending()) // FIXME. Not so nice...
                        gtk_main_iteration();
		}
		break;
	    case SYMBOL_REGISTER:
		if(sw->filter_registers)
		{
		    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sw->registersbutton), TRUE);
		    while(gtk_events_pending()) // FIXME. Not so nice...
                        gtk_main_iteration();
		}
		break;
	    default:
		break;
	    }
	    break;
	}
    }

    // Find the symbol and select it in the clist
    p=sw->symbols;
    while(p)
    {
	sym *e;
	e=(sym*)p->data;
	if(!strcasecmp(e->name,name))
	{
	    int row;
	    row=gtk_clist_find_row_from_data(GTK_CLIST(sw->symbol_clist),e);
	    if(row!=-1)
	    {
		gtk_clist_select_row(GTK_CLIST(sw->symbol_clist),row,0);
		gtk_clist_moveto(GTK_CLIST(sw->symbol_clist),row,0,0.5,0.5);
		
		do_symbol_select(sw,e);
		
	    }
	}
	p=p->next;
    }
}

void SymbolWindow_new_symbols(Symbol_Window *sw, GUI_Processor *gp)
{
  sw->Update();
}

/*
 the function comparing rows of symbol list for sorting
 FIXME this can be improved. When we have equal cells in sort_column
 of the two rows, compare another column instead of returning 'match'.
 */
static gint
symbol_compare_func(GtkCList *clist, gconstpointer ptr1,gconstpointer ptr2)
{
    char *text1, *text2;
    long val1, val2;
    GtkCListRow *row1 = (GtkCListRow *) ptr1;
    GtkCListRow *row2 = (GtkCListRow *) ptr2;
//    char *p;

    switch (row1->cell[clist->sort_column].type)
    {
    case GTK_CELL_TEXT:
	text1 = GTK_CELL_TEXT (row1->cell[clist->sort_column])->text;
	break;
    case GTK_CELL_PIXTEXT:
	text1 = GTK_CELL_PIXTEXT (row1->cell[clist->sort_column])->text;
	break;
    default:
	assert(0);
	break;
    }

    switch (row2->cell[clist->sort_column].type)
    {
    case GTK_CELL_TEXT:
	text2 = GTK_CELL_TEXT (row2->cell[clist->sort_column])->text;
	break;
    case GTK_CELL_PIXTEXT:
	text2 = GTK_CELL_PIXTEXT (row2->cell[clist->sort_column])->text;
	break;
    default:
	assert(0);
	break;
    }

    if (!text2)
	assert(0);
    //	return (text1 != NULL);

    if (!text1)
	assert(0);
    //	return -1;

    if(1==sscanf(text1,"%li",&val1))
    {
	if(1==sscanf(text2,"%li",&val2))
	{
//	    printf("Value %d %d\n",val1,val2);
	    return val1-val2;
	}
    }
    return strcmp(text1,text2);
}

static void symbol_click_column(GtkCList *clist, int column)
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
                        Symbol_Window *sw)
{
    sw->ChangeView(VIEW_HIDE);
    return TRUE;
}

static void
toggle_addresses (GtkToggleButton *button, Symbol_Window *sw)
{
    sw->filter_addresses = !sw->filter_addresses;
    config_set_variable(sw->name, "filter_addresses", sw->filter_addresses);
    sw->Update();
}

static void
toggle_constants (GtkToggleButton *button, Symbol_Window *sw)
{
    sw->filter_constants = !sw->filter_constants;
    config_set_variable(sw->name, "filter_constants", sw->filter_constants);
    sw->Update();
}

static void
toggle_registers (GtkToggleButton *button, Symbol_Window *sw)
{
    sw->filter_registers = !sw->filter_registers;
    config_set_variable(sw->name, "filter_registers", sw->filter_registers);
    sw->Update();
}

static char *symbol_titles[3]={"Name","Type","Address/Value"};

//------------------------------------------------------------------------
// Build
//

void Symbol_Window::Build(void)
{
  GtkWidget *vbox;
  GtkWidget *scrolled_window;
  GtkWidget *hbox;

  window=gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title(GTK_WINDOW(window), "Symbol Viewer");
  
  gtk_window_set_default_size(GTK_WINDOW(window), width,height);
  gtk_widget_set_uposition(GTK_WIDGET(window),x,y);
  gtk_window_set_wmclass(GTK_WINDOW(window),name,"Gpsim");
  
  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC(delete_event), (gpointer)this);

  symbol_clist=gtk_clist_new_with_titles(3,symbol_titles);
  gtk_widget_show(symbol_clist);
  gtk_clist_set_column_auto_resize(GTK_CLIST(symbol_clist),0,TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(symbol_clist),1,TRUE);
  gtk_clist_set_column_auto_resize(GTK_CLIST(symbol_clist),2,TRUE);
  gtk_clist_set_auto_sort(GTK_CLIST(symbol_clist),TRUE);
  gtk_clist_set_compare_func(GTK_CLIST(symbol_clist),
			     (GtkCListCompareFunc)symbol_compare_func);

  gtk_signal_connect(GTK_OBJECT(symbol_clist),"click_column",
		     (GtkSignalFunc)symbol_click_column,NULL);
  gtk_signal_connect(GTK_OBJECT(symbol_clist),"select_row",
		     (GtkSignalFunc)symbol_list_row_selected,this);
  gtk_signal_connect(GTK_OBJECT(symbol_clist),"unselect_row",
		     (GtkSignalFunc)unselect_row,this);
  gtk_signal_connect(GTK_OBJECT(symbol_clist),
		     "button_press_event",
		     (GtkSignalFunc) do_popup,
		     this);

  scrolled_window=gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_show(scrolled_window);

  vbox = gtk_vbox_new(FALSE,1);
  
  gtk_container_add(GTK_CONTAINER(scrolled_window), symbol_clist);
  
  gtk_container_add(GTK_CONTAINER(window),vbox);



  hbox = gtk_hbox_new(FALSE,1);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE,FALSE,0);
  gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);

  addressesbutton = gtk_check_button_new_with_label ("addresses");
  gtk_box_pack_start (GTK_BOX (hbox), addressesbutton, TRUE, TRUE, 5);
  if(filter_addresses)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (addressesbutton), FALSE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (addressesbutton), TRUE);
  gtk_signal_connect (GTK_OBJECT (addressesbutton), "toggled",
		      GTK_SIGNAL_FUNC (toggle_addresses), (gpointer)this);

  
  constantsbutton = gtk_check_button_new_with_label ("constants");
  gtk_box_pack_start (GTK_BOX (hbox), constantsbutton, TRUE, TRUE, 5);
  if(filter_constants)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (constantsbutton), FALSE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (constantsbutton), TRUE);
  gtk_signal_connect (GTK_OBJECT (constantsbutton), "toggled",
		      GTK_SIGNAL_FUNC (toggle_constants), (gpointer)this);

  
  registersbutton = gtk_check_button_new_with_label ("registers");
  gtk_box_pack_start (GTK_BOX (hbox), registersbutton, TRUE, TRUE, 5);
  if(filter_registers)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (registersbutton), FALSE);
  else
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (registersbutton), TRUE);
  gtk_signal_connect (GTK_OBJECT (registersbutton), "toggled",
		      GTK_SIGNAL_FUNC (toggle_registers), (gpointer)this);

  gtk_signal_connect_after(GTK_OBJECT(window), "configure_event",
			   GTK_SIGNAL_FUNC(gui_object_configure_event),this);

  
  gtk_widget_show_all (window);
  
  enabled=1;

  is_built=1;

  if(load_symbols)
    SymbolWindow_new_symbols(this, gp);

  UpdateMenuItem();
  
  popup_menu=build_menu(window,this);
  
}

int Symbol_Window::Create(GUI_Processor *_gp)
{

#define MAXROWS  (MAX_REGISTERS/REGISTERS_PER_ROW)
#define MAXCOLS  (REGISTERS_PER_ROW+1)


  gp = _gp;
  name = "symbol_viewer";
  wc = WC_misc;
  wt = WT_symbol_window;
  window = NULL;
  is_built = 0;
  gp->symbol_window = this;

  symbols=NULL;
  filter_addresses=0;
  filter_constants=1;
  filter_registers=0;

  load_symbols=0;
  
  get_config();

  config_get_variable(name,"filter_addresses",&filter_addresses);
  config_get_variable(name,"filter_constants",&filter_constants);
  config_get_variable(name,"filter_registers",&filter_registers);

  if(enabled)
    Build();
  
  return 1;
}

Symbol_Window::Symbol_Window(void)
{

  menu = "<main>/Windows/Symbols";

}

#endif // HAVE_GUI
