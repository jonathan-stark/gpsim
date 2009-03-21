/*
   Copyright (C) 2000,2001
    Ralf Forsberg

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

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <string.h>

#include <assert.h>

#include "../src/sim_context.h"
#include "../src/interface.h"

#include "../src/symbol.h"

#include "gui.h"
#include "gui_stack.h"
#include "gui_src.h"

struct stack_entry {
    unsigned int depth;           // index in stack array
    unsigned int retaddress;      // last known return address
};

#define COLUMNS 2
#define DEPTHCOL 0
#define RETADDRCOL 1
static gchar *stack_titles[COLUMNS]={"depth", "return address"};

/*
 */
static gint sigh_button_event(GtkWidget *widget,
                       GdkEventButton *event,
                       Stack_Window *sw)
{
  struct stack_entry *entry;
  assert(event&&sw);

  if(!sw->gp || !sw->gp->cpu)
    return 0;

  if(event->type==GDK_2BUTTON_PRESS &&
     event->button==1) {

    int row=sw->current_row;

    entry = (struct stack_entry*) gtk_clist_get_row_data(GTK_CLIST(sw->stack_clist), row);

    if(entry)
      sw->gp->cpu->pma->toggle_break_at_address(entry->retaddress);

    return 1;

  }

  return 0;
}

static gint stack_list_row_selected(GtkCList *stacklist,gint row, gint column,GdkEvent *event, Stack_Window *sw)
{
    struct stack_entry *entry;
    GUI_Processor *gp;

    sw->current_row=row;
    sw->current_column=column;

    gp=sw->gp;

    entry = (struct stack_entry*) gtk_clist_get_row_data(GTK_CLIST(sw->stack_clist), row);

    if(!entry)
        return TRUE;

    sw->gp->source_browser->SelectAddress(entry->retaddress);
    sw->gp->program_memory->SelectAddress(entry->retaddress);

    return 0;
}

static void stack_click_column(GtkCList *clist, int column)
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
                        Stack_Window *sw)
{
    sw->ChangeView(VIEW_HIDE);
    return TRUE;
}

// find name of label closest before 'address' and copy found data
// into name and offset
static int get_closest_label(Stack_Window *sw,
                              int address,
                              char *name,
                              int *offset)
{

  // this function assumes that address symbols are sorted

  Value *closest_symbol = 0;

#if 0
  int minimum_delta=0x2000000;
  int delta;

  Symbol_Table &st = CSimulationContext::GetContext()->GetSymbolTable();
  Symbol_Table::iterator symIt;
  Symbol_Table::iterator symItEnd = st.end();

  for(symIt=st.begin(); symIt != symItEnd; symIt++) {

    Value *s = *symIt;

    if(  (typeid(*s) == typeid(AddressSymbol)) /*||
         (typeid(*s) == typeid(line_number_symbol))*/) {
      int i;
      s->get(i);
      delta = abs( i - address);
      if(delta < minimum_delta) {
        minimum_delta = delta;
        closest_symbol = s;
      }
    }
  }
#endif
  cout << "FIXME gui_stack.cc get closest label\n";
  if(closest_symbol) {
    strcpy(name,closest_symbol->name().data());
    int i;
    closest_symbol->get(i);
    *offset=address-i;
    return 1;
  }


  return 0;

}

void Stack_Window::Update(void)
{
  int i=0;
  int nrofentries;
  //unsigned int pic_id;
  char depth_string[64];
  char retaddress_string[64];
  char labelname[64];
  int labeloffset;
  char *entry[COLUMNS]={depth_string,retaddress_string};
  unsigned int retaddress;
  struct stack_entry *stack_entry;

  if(!gp || !enabled)
    return;

  //pic_id = gp->pic_id;

  pic_processor *pic = dynamic_cast<pic_processor *>(gp->cpu);
  if(!pic)
    return;

  nrofentries = pic->stack->pointer & pic->stack->stack_mask;

  if(last_stacklen!=nrofentries) {

    // stack has changed, update stack clist

    gtk_clist_freeze (GTK_CLIST (stack_clist));

    while(last_stacklen!=nrofentries) {


      if(last_stacklen>nrofentries) {

        // Stack has shrunk

        // remove row 0
        stack_entry = (struct stack_entry*) gtk_clist_get_row_data(GTK_CLIST(stack_clist), 0);
        free(stack_entry);
        gtk_clist_remove(GTK_CLIST(stack_clist),0);

        last_stacklen--;
      } else {

        // stack has grown

        strcpy(depth_string,"");
        retaddress=pic->stack->contents[last_stacklen & pic->stack->stack_mask];

        if(get_closest_label(this,retaddress,labelname,&labeloffset))
          sprintf(retaddress_string,"0x%04x (%s+%d)",retaddress,labelname,labeloffset);
        else
          sprintf(retaddress_string,"0x%04x",retaddress);

        gtk_clist_insert (GTK_CLIST(stack_clist),
                          0,
                          entry);

        // FIXME this memory is never freed?
        stack_entry = (struct stack_entry*) malloc(sizeof(struct stack_entry));
        stack_entry->retaddress=retaddress;
        stack_entry->depth=i;

        gtk_clist_set_row_data(GTK_CLIST(stack_clist), 0, (gpointer)stack_entry);
        last_stacklen++;
      }
    }

    // update depth column
    for(i=0;i<nrofentries;i++)
      {
        sprintf(depth_string,"#%d",i);
        gtk_clist_set_text (GTK_CLIST(stack_clist),i,0,depth_string);
      }

    gtk_clist_thaw (GTK_CLIST (stack_clist));

  }
}

void Stack_Window::Build(void)
{
  if(bIsBuilt)
    return;

  GtkWidget *vbox;
  GtkWidget *scrolled_window;

  window=gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title(GTK_WINDOW(window), "Stack Viewer");

  gtk_window_set_default_size(GTK_WINDOW(window), width,height);
  gtk_widget_set_uposition(GTK_WIDGET(window),x,y);
  gtk_window_set_wmclass(GTK_WINDOW(window),name(),"Gpsim");

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
                      GTK_SIGNAL_FUNC (gtk_widget_destroyed), &window);
  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
                      GTK_SIGNAL_FUNC(delete_event), (gpointer)this);
  gtk_signal_connect_after(GTK_OBJECT(window), "configure_event",
                           GTK_SIGNAL_FUNC(gui_object_configure_event),this);
  gtk_signal_connect_after(GTK_OBJECT(window), "button_press_event",
                           GTK_SIGNAL_FUNC(sigh_button_event), this);

  stack_clist=gtk_clist_new_with_titles(COLUMNS,stack_titles);
  gtk_widget_show(stack_clist);


  gtk_clist_set_selection_mode (GTK_CLIST(stack_clist), GTK_SELECTION_BROWSE);

  gtk_signal_connect(GTK_OBJECT(stack_clist),"click_column",
                     (GtkSignalFunc)stack_click_column,0);
  gtk_signal_connect(GTK_OBJECT(stack_clist),"select_row",
                     (GtkSignalFunc)stack_list_row_selected,this);

  scrolled_window=gtk_scrolled_window_new(0, 0);
  gtk_widget_show(scrolled_window);

  vbox = gtk_vbox_new(FALSE,1);
  gtk_widget_show(vbox);

  gtk_container_add(GTK_CONTAINER(scrolled_window), stack_clist);

  gtk_container_add(GTK_CONTAINER(window),vbox);

  gtk_box_pack_start_defaults(GTK_BOX(vbox),scrolled_window);

  gtk_widget_show (window);


  bIsBuilt = true;

  UpdateMenuItem();

  Update();

}

//------------------------------------------------------------------------
// Create
//
//


Stack_Window::Stack_Window(GUI_Processor *_gp)
{
#define MAXROWS  (MAX_REGISTERS/REGISTERS_PER_ROW)
#define MAXCOLS  (REGISTERS_PER_ROW+1)

  menu = "<main>/Windows/Stack";

  gp = _gp;
  set_name("stack_viewer");
  wc = WC_data;
  wt = WT_stack_window;
  window = 0;

  last_stacklen=0;
  current_row=0;

  get_config();

  if(enabled)
    Build();
}

#endif // HAVE_GUI
