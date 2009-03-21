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

#include <assert.h>

#include "../src/sim_context.h"
#include "../src/interface.h"

#include "gui.h"
#include "gui_regwin.h"
#include "gui_symbols.h"
#include "gui_watch.h"
#include "gui_processor.h"
#include "gui_src.h"

struct symbol_entry {
    unsigned int value; // symbol value
    struct cross_reference_to_gui *xref;
};

typedef enum {
    MENU_ADD_WATCH,
} menu_id;


typedef struct _menu_item {
    const char *name;
    menu_id id;
    GtkWidget *item;
} menu_item;

static menu_item menu_items[] = {
    {"Add to watch window", MENU_ADD_WATCH},
};


class GUISymbol
{
public:
  symbol *s;

  virtual ~GUISymbol()
  {
  }

  virtual void select(void);

};

// Used only in popup menus
Symbol_Window *popup_sw;

static void update_menus(Symbol_Window *sw)
{
    GtkWidget *item;
    unsigned int i;

    for (i=0; i < (sizeof(menu_items)/sizeof(menu_items[0])) ; i++){
        item=menu_items[i].item;
        if(sw)
        {
            Value *entry;
            entry = (Value*) gtk_clist_get_row_data(GTK_CLIST(sw->symbol_clist),sw->current_row);
            if(entry==0)
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

  if(widget==0 || data==0)
    {
      printf("Warning popup_activated(%p,%p)\n",widget,data);
      return;
    }

  item = (menu_item *)data;

  Value *entry =
    (Value*) gtk_clist_get_row_data(GTK_CLIST(popup_sw->symbol_clist),popup_sw->current_row);

  if(!entry)
    return;

  switch(item->id) {

  case MENU_ADD_WATCH:
    {
      //GUIRegister *reg = (*popup_sw->gp->regwin_ram)[entry->value];
      //popup_sw->gp->watch_window->Add(popup_sw->gp->regwin_ram->type, reg);
      popup_sw->gp->watch_window->Add(entry);
    }
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
  unsigned int i;

  if(sheet==0 || sw==0)
  {
      printf("Warning build_menu(%p,%p)\n",sheet,sw);
      return 0;
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
//      GdkModifierType mods;

  if(widget==0 || event==0 || sw==0)
  {
      printf("Warning do_popup(%p,%p,%p)\n",widget,event,sw);
      return 0;
  }
  popup=sw->popup_menu;
    if( (event->type == GDK_BUTTON_PRESS) &&  (event->button == 3) )
    {

      gtk_menu_popup(GTK_MENU(popup), 0, 0, 0, 0,
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
#if 1
static Symbol_Window *lpSW=0;
static string table;

void updateOneSymbol(const SymbolEntry_t &sym)
{

  Value *pVal = dynamic_cast<Value *>(sym.second);
  if (lpSW && pVal) {

    Register *pReg = dynamic_cast<Register *>(pVal);
    if((typeid(*pVal) == typeid(LineNumberSymbol) ) |
       (lpSW->filter_addresses && (typeid(*pVal) == typeid(AddressSymbol)))  ||
       (lpSW->filter_constants && (typeid(*pVal) == typeid(Integer))) ||
       (lpSW->filter_constants && (typeid(*pVal) == typeid(Boolean))) ||
       (lpSW->filter_registers && (pReg)))
        return;

#define SYM_LEN 32
    const char *entry[3];
    string symbol_name;
    char type[SYM_LEN];
    char value[SYM_LEN];

    if (table != "__global__")
        symbol_name = table + "." + pVal->name();
    else
        symbol_name = pVal->name();

    entry[0] = symbol_name.c_str();
    strncpy(type, pVal->showType().c_str(), sizeof(type));
    type[SYM_LEN-1] = 0;
    entry[1] = type;
    entry[2] = value;
    if (pReg)
      snprintf(value, sizeof(value), "%02x / %d (0x%02x)", pReg->getAddress(), pReg->get_value(), pReg->get_value());
    else
      pVal->get(value,sizeof(value));

    char *pLF = strchr(value, '\n');
    if(pLF)
      *pLF = 0;

    lpSW->symbols=g_list_append(lpSW->symbols, pVal);

    int row = gtk_clist_append(GTK_CLIST(lpSW->symbol_clist),(gchar **)entry);
    gtk_clist_set_row_data(GTK_CLIST(lpSW->symbol_clist),row,pVal);
  }
}

static void updateSymbolTables(const SymbolTableEntry_t &st)
{
  if(verbose)cout << " gui Symbol Window: " << st.first << endl;
  table = st.first;
  (st.second)->ForEachSymbolTable(updateOneSymbol);
}
#endif

void Symbol_Window::Update(void)
{



  load_symbols=1;

  if(!enabled)
    return;


  gtk_clist_freeze(GTK_CLIST(symbol_clist));

  gtk_clist_clear(GTK_CLIST(symbol_clist));

  while (symbols)
    symbols = g_list_remove(symbols,symbols->data);
#if 1

  lpSW = this;
  globalSymbolTable().ForEachModule(updateSymbolTables);
  lpSW = 0;
  /*
  Symbol_Table &st = CSimulationContext::GetContext()->GetSymbolTable();
  Symbol_Table::iterator symIt;
  Symbol_Table::iterator symItEnd = st.end();

  for(symIt=st.begin(); symIt != symItEnd; symIt++) {
    Value *sym = *symIt;
    // ignore line numbers
    if((typeid(*sym) == typeid(LineNumberSymbol) )            ||
       (filter_addresses && (typeid(*sym) == typeid(AddressSymbol)))  ||
       (filter_constants && (typeid(*sym) == typeid(Integer))) ||
       (filter_registers && (typeid(*sym) == typeid(register_symbol)))) {

      continue;
    }


    char **entry = (char**)malloc(3*sizeof(char*));
    const int cMaxLength = 32;

    entry[0] = g_strndup(sym->name().c_str(), cMaxLength);
    entry[1] = g_strndup(sym->showType().c_str(), cMaxLength);
    entry[2] = (char*)malloc(cMaxLength);
    if (typeid(*sym) == typeid(register_symbol)) {
      Register * pReg = ((register_symbol*)sym)->getReg();
      int iValue;
      sym->get(iValue);
      snprintf(entry[2], cMaxLength, "%02x / %d (0x%02x)", pReg->address, iValue, iValue);
    }
    else {
      sym->get(entry[2],cMaxLength);
    }
    char *pLF = strchr(entry[2], '\n');
    if(pLF != 0) {
      *pLF = 0;
    }

    symbols=g_list_append(symbols,sym);

    int row = gtk_clist_append(GTK_CLIST(symbol_clist),entry);
    gtk_clist_set_row_data(GTK_CLIST(symbol_clist),row,sym);

  }
  */
#endif
  gtk_clist_thaw(GTK_CLIST(symbol_clist));

}

static void do_symbol_select(Symbol_Window *sw, Value *e)
{

  if(!sw || !sw->gp)
    return;

  // Do what is to be done when a symbol is selected.
  // Except for selecting the symbol row in the symbol_clist

  if(typeid(*e) == typeid(LineNumberSymbol) ||
     typeid(*e) == typeid(AddressSymbol)) {
    if(sw->gp->source_browser)
      sw->gp->source_browser->SelectAddress(e);
    if(sw->gp->program_memory)
      sw->gp->program_memory->SelectAddress(e);
  } else
    if(typeid(*e) == typeid(Register))
      if(sw->gp->regwin_ram)
        sw->gp->regwin_ram->SelectRegister(e);
}

static gint symbol_list_row_selected(GtkCList *symlist,gint row, gint column,GdkEvent *event, Symbol_Window *sw)
{
  if(!symlist || !sw)
    return 0;

  Value *e=(Value*)gtk_clist_get_row_data(symlist,row);
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
    if(!sw)
      return;
    if(!sw->enabled)
        return;

    p=sw->symbols;
    while(p)
    {
      Value *e = (Value*)p->data;

      if(typeid(*e) == typeid(Register)) {

        int i;

        e->get(i);

        if(i == regnumber) {
          int row;
          row=gtk_clist_find_row_from_data(GTK_CLIST(sw->symbol_clist),e);

          if(row!=-1) {
            gtk_clist_select_row(GTK_CLIST(sw->symbol_clist),row,0);
            gtk_clist_moveto(GTK_CLIST(sw->symbol_clist),row,0,0.5,0.5);

            do_symbol_select(sw,e);
          }
          break;
        }
        p=p->next;
      }
    }

}

void Symbol_Window::SelectSymbolName(char *symbol_name)
{
  cout << "SelectSymbolName is broken\n";
#if 0
  GList *p;
  //sym *s;

  if(!symbol_name)
    return;

  // If window is not displayed, then display it.
  if(!enabled)
    ChangeView(VIEW_SHOW);

  // See if the type of symbol selected is currently filtered out, and
  // if so we unfilter it.
  Value *sym=0;
  Symbol_Table_Iterator sti;


  for(sym=sti.begin(); sym != sti.end(); sym = sti.next()) {

    // ignore line numbers
    if(typeid(*sym) == typeid(line_number_symbol))
                  continue;

    if(!strcasecmp((*sti)->name().data(),symbol_name)) {

       if(filter_addresses && (typeid(*sym) == typeid(address_symbol)))
         gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (addressesbutton), TRUE);
       else if (filter_constants && (typeid(*sym) == typeid(Integer)))
         gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (constantsbutton), TRUE);
       else if (filter_registers && (typeid(*sym) == typeid(register_symbol)))
         gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (registersbutton), TRUE);
    }


/*
  list <symbol *>::iterator sti = st.begin();

  for(sti = st.begin(); sti != st.end(); sti++) {

    if(!strcasecmp((*sti)->name().data(),symbol_name)) {

      switch((*sti)->isa()) {

      case SYMBOL_ADDRESS:
        if(filter_addresses) {
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (addressesbutton), TRUE);
          while(gtk_events_pending()) // FIXME. Not so nice...
            gtk_main_iteration();
        }
        break;
      case SYMBOL_CONSTANT:
        if(filter_constants) {
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (constantsbutton), TRUE);
          while(gtk_events_pending()) // FIXME. Not so nice...
            gtk_main_iteration();
        }
        break;
      case SYMBOL_REGISTER:
        if(filter_registers) {
          gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (registersbutton), TRUE);
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
*/

  // Find the symbol and select it from the clist
  p=symbols;
  while(p) {

    Value *e;
    e=(sym*)p->data;
    if(!strcasecmp(e->name,symbol_name)) {

      int row;
      row=gtk_clist_find_row_from_data(GTK_CLIST(symbol_clist),e);
      if(row!=-1) {

                                gtk_clist_select_row(GTK_CLIST(symbol_clist),row,0);
        gtk_clist_moveto(GTK_CLIST(symbol_clist),row,0,0.5,0.5);

                                do_symbol_select(this,e);

      }
    }
    p=p->next;
  }

#endif //0
}

void Symbol_Window::NewSymbols(void)
{
  Update();
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

    if (!text1)
        assert(0);

    if(1==sscanf(text1,"%li",&val1))
    {
      if(1==sscanf(text2,"%li",&val2))
        return val1-val2;

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
    config_set_variable(sw->name(), "filter_addresses", sw->filter_addresses);
    sw->Update();
}

static void
toggle_constants (GtkToggleButton *button, Symbol_Window *sw)
{
    sw->filter_constants = !sw->filter_constants;
    config_set_variable(sw->name(), "filter_constants", sw->filter_constants);
    sw->Update();
}

static void
toggle_registers (GtkToggleButton *button, Symbol_Window *sw)
{
    sw->filter_registers = !sw->filter_registers;
    config_set_variable(sw->name(), "filter_registers", sw->filter_registers);
    sw->Update();
}

static gchar *symbol_titles[3]={"Name","Type","Address/Value"};

//------------------------------------------------------------------------
// Build
//

void Symbol_Window::Build(void)
{
  if(bIsBuilt)
    return;

  GtkWidget *vbox;
  GtkWidget *scrolled_window;
  GtkWidget *hbox;

  window=gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title(GTK_WINDOW(window), "Symbol Viewer");

  gtk_window_set_default_size(GTK_WINDOW(window), width,height);
  gtk_widget_set_uposition(GTK_WIDGET(window),x,y);
  gtk_window_set_wmclass(GTK_WINDOW(window),name(),"Gpsim");

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
                     (GtkSignalFunc)symbol_click_column,0);
  gtk_signal_connect(GTK_OBJECT(symbol_clist),"select_row",
                     (GtkSignalFunc)symbol_list_row_selected,this);
  gtk_signal_connect(GTK_OBJECT(symbol_clist),"unselect_row",
                     (GtkSignalFunc)unselect_row,this);
  gtk_signal_connect(GTK_OBJECT(symbol_clist),
                     "button_press_event",
                     (GtkSignalFunc) do_popup,
                     this);

  scrolled_window=gtk_scrolled_window_new(0, 0);
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


  bIsBuilt = true;

  if(load_symbols)
    NewSymbols();

  UpdateMenuItem();

  popup_menu=build_menu(window,this);

}


Symbol_Window::Symbol_Window(GUI_Processor *_gp)
{


#define MAXROWS  (MAX_REGISTERS/REGISTERS_PER_ROW)
#define MAXCOLS  (REGISTERS_PER_ROW+1)


  menu = "<main>/Windows/Symbols";

  gp = _gp;
  set_name("symbol_viewer");
  wc = WC_misc;
  wt = WT_symbol_window;
  window = 0;

  symbols=0;
  filter_addresses=0;
  filter_constants=1;
  filter_registers=0;

  load_symbols=0;

  get_config();

  config_get_variable(name(),"filter_addresses",&filter_addresses);
  config_get_variable(name(),"filter_constants",&filter_constants);
  config_get_variable(name(),"filter_registers",&filter_registers);

  if(enabled)
    Build();

}

#endif // HAVE_GUI
