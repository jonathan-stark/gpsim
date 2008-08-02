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

#include "../src/interface.h"
#include "../src/cmd_gpsim.h"

#include "gui.h"
#include "preferences.h"
#include "gui_register.h"
#include "gui_regwin.h"
#include "gui_watch.h"

#define NAMECOL 0
#define DECIMALCOL 2
#define HEXCOL 3
#define ASCIICOL 4
#define BITCOL 5
#define LASTCOL BITCOL

static gchar *watch_titles[]={"name","address","dec","hex","ascii","bits"};

#define COLUMNS sizeof(watch_titles)/sizeof(char*)

class ColumnData
{
public:
  int column;
  int isVisible;
  bool bIsValid;
  Watch_Window *ww;

  void SetVisibility(bool bVisibility);
  void SetValidity(bool );
  bool isValid();
  void Show();
  ColumnData();
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
    const char *name;
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

//========================================================================

class WatchWindowXREF : public CrossReferenceToGUI
{
public:

  void Update(int new_value)
  {

    Watch_Window *ww  = (Watch_Window *) (parent_window);
    if (ww)
      ww->UpdateWatch((WatchEntry *)data);

  }
};

//========================================================================
WatchEntry::WatchEntry()
  : cpu(0)
{
}
//========================================================================
ColumnData::ColumnData()
  : isVisible(1), bIsValid(true), ww(0)
{
}

void ColumnData::SetVisibility(bool bVisibility)
{
  isVisible = bVisibility ? 1 : 0;
  Show();
}
void ColumnData::Show()
{
  if (ww) {
    int show = isVisible & (bIsValid ? 1 : 0);
    gtk_clist_set_column_visibility(GTK_CLIST(ww->watch_clist),column,show);
    config_set_variable(ww->name(), watch_titles[column],show);
  }
}
bool ColumnData::isValid()
{
  return bIsValid;
}
void ColumnData::SetValidity(bool newValid)
{
  bIsValid = newValid;
}
//========================================================================

void Watch_Window::ClearWatch(WatchEntry *entry)
{
  gtk_clist_remove(GTK_CLIST(watch_clist),current_row);
  watches=g_list_remove(watches,entry);
  entry->Clear_xref();
  free(entry);
}

void Watch_Window::UpdateMenus(void)
{
  GtkWidget *item;
  WatchEntry *entry;

  unsigned int i;

  for (i=0; i < (sizeof(menu_items)/sizeof(menu_items[0])) ; i++) {
    item=menu_items[i].item;
    if(menu_items[i].id!=MENU_COLUMNS) {

      entry = (WatchEntry*) gtk_clist_get_row_data(GTK_CLIST(watch_clist),current_row);
      if(menu_items[i].id!=MENU_COLUMNS &&
          (entry==0 ||
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
  }
}

int Watch_Window::set_config(void) {
  int iRet = GUI_Object::set_config();
  WriteSymbolList();
  return iRet;
}

void Watch_Window::WriteSymbolList()
{
  // delete previous list
  DeleteSymbolList();
  // write the current list
  WatchEntry *entry;
  guint uSize = g_list_length(watches);
  char cwv[100];
  char *vname;
  for (guint i = 0; i<uSize; i++) {
    snprintf(cwv, sizeof(cwv), "WV%d",i);
    vname = 0;
    entry = (WatchEntry*) g_list_nth_data(watches, i);
    if (entry && entry->pRegister)
      config_set_string(name(), cwv, entry->pRegister->name().c_str());
  }

}

void Watch_Window::DeleteSymbolList() {
  int i=0;
  char cwv[100];
  while (i<1000) {
    snprintf(cwv, sizeof(cwv), "WV%d",i++);
    if (config_remove(name(), cwv) == 0 ) {
      break;
    }
  }
}

void Watch_Window::ReadSymbolList()
{
  // now read symbols watched from a prior simulation session
  int i=0;
  char cwv[100];
  char *vname;
  while (i<1000) {
    snprintf(cwv, sizeof(cwv), "WV%d",i++);
    vname = 0;
    if (config_get_string(name(), cwv, &vname) ) {
      Value *val = globalSymbolTable().findValue(vname);
      if(val)
        Add(val);
    }
    else
      break;
  }
}

static void unselect_row(GtkCList *clist,
                         gint row,
                         gint column,
                         GdkEvent *event,
                         Watch_Window *ww)
{
  ww->UpdateMenus();
}

// called when user has selected a menu item
static void
popup_activated(GtkWidget *widget, gpointer data)
{
  menu_item *item;

  WatchEntry *entry;

  int value;

  if(widget==0 || data==0)
    {
      printf("Warning popup_activated(%p,%p)\n",widget,data);
      return;
    }

  item = (menu_item *)data;

  entry = (WatchEntry*) gtk_clist_get_row_data(GTK_CLIST(popup_ww->watch_clist),popup_ww->current_row);

  if(entry==0 && item->id!=MENU_COLUMNS)
    return;

  if(!entry || !entry->cpu)
    return;

  switch(item->id)
    {
    case MENU_REMOVE:
      popup_ww->ClearWatch(entry);
      //remove_entry(popup_ww,entry);
      break;
    case MENU_SET_VALUE:
      value = gui_get_value("value:");
      if(value<0)
        break; // Cancel
      entry->put_value(value);
      break;
    case MENU_BREAK_READ:
      get_bp().set_read_break(entry->cpu,entry->address);
      break;
    case MENU_BREAK_WRITE:
      get_bp().set_write_break(entry->cpu,entry->address);
      break;
    case MENU_BREAK_READ_VALUE:
      value = gui_get_value("value to read for breakpoint:");
      if(value<0)
        break; // Cancel
      get_bp().set_read_value_break(entry->cpu,entry->address,value);
      break;
    case MENU_BREAK_WRITE_VALUE:
      value = gui_get_value("value to write for breakpoint:");
      if(value<0)
        break; // Cancel
      get_bp().set_write_value_break(entry->cpu,entry->address,value);
      break;
    case MENU_BREAK_CLEAR:
      get_bp().clear_all_register(entry->cpu,entry->address);
      break;
    case MENU_COLUMNS:
      select_columns(popup_ww, popup_ww->watch_clist);
      break;
    default:
      puts("Unhandled menuitem?");
      break;
    }
}

//------------------------------------------------------------
// call back function to toggle column visibility in the configuration popup
static void set_column(GtkCheckButton *button, ColumnData *coldata)
{
  if (coldata)
    coldata->SetVisibility(button->toggle_button.active != 0);
}

static void select_columns(Watch_Window *ww, GtkWidget *clist)
{
    GtkWidget *dialog=0;
    GtkWidget *button;
    unsigned int i;

    dialog = gtk_dialog_new();

    gtk_container_set_border_width(GTK_CONTAINER(dialog),30);

    gtk_signal_connect_object(GTK_OBJECT(dialog),
                              "delete_event",GTK_SIGNAL_FUNC(gtk_widget_destroy),GTK_OBJECT(dialog));

    for(i=0;i<COLUMNS;i++)
      if (coldata[i].isValid()) {
        button=gtk_check_button_new_with_label(watch_titles[i]);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),coldata[i].isVisible);
        gtk_widget_show(button);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox),button,FALSE,FALSE,0);
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
  unsigned int i;


  if(sheet==0 || ww==0)
  {
      printf("Warning build_menu(%p,%p)\n",sheet,ww);
      return 0;
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
      gtk_widget_show(item);
      gtk_menu_append(GTK_MENU(menu),item);
  }

  ww->UpdateMenus();

  return menu;
}

// button press handler
static gint
do_popup(GtkWidget *widget, GdkEventButton *event, Watch_Window *ww)
{

  GtkWidget *popup;

  if(widget==0 || event==0 || ww==0)
    {
      printf("Warning do_popup(%p,%p,%p)\n",widget,event,ww);
      return 0;
    }

  popup=ww->popup_menu;

  if( (event->type == GDK_BUTTON_PRESS) &&  (event->button == 3) )
    gtk_menu_popup(GTK_MENU(popup), 0, 0, 0, 0,
                   3, event->time);

  /*
  WatchEntry *entry;

  if(event->type==GDK_2BUTTON_PRESS &&
     event->button==1)
    {
      int column=ww->current_column;
      int row=ww->current_row;

      entry = (WatchEntry*) gtk_clist_get_row_data(GTK_CLIST(ww->watch_clist), row);

      if(column>=MSBCOL && column<=LSBCOL) {

        int value;  // , bit;
        printf("column %d\n",column);
        // Toggle the bit.
        value = entry->get_value();

        value ^= (1<< (15-(column-MSBCOL)));
        entry->put_value(value);
      }
    }
  */
  return 0;
}

static gint
key_press(GtkWidget *widget,
          GdkEventKey *key,
          gpointer data)
{

  WatchEntry *entry;
  Watch_Window *ww = (Watch_Window *) data;

  if(!ww) return(FALSE);
  if(!ww->gp) return(FALSE);
  if(!ww->gp->cpu) return(FALSE);

  switch(key->keyval) {

  case GDK_Delete:
      entry = (WatchEntry*) gtk_clist_get_row_data(GTK_CLIST(ww->watch_clist),ww->current_row);
      if(entry!=0)
          ww->ClearWatch(entry);
      break;
  }
  return TRUE;
}

static gint watch_list_row_selected(GtkCList *watchlist,gint row, gint column,GdkEvent *event, Watch_Window *ww)
{
  WatchEntry *entry;
  GUI_Processor *gp;

  ww->current_row=row;
  ww->current_column=column;

  gp=ww->gp;

  entry = (WatchEntry*) gtk_clist_get_row_data(GTK_CLIST(ww->watch_clist), row);

  if(!entry)
    return TRUE;

  if(entry->type==REGISTER_RAM)
    gp->regwin_ram->SelectRegister(entry->address);
  else if(entry->type==REGISTER_EEPROM)
    gp->regwin_eeprom->SelectRegister(entry->address);


  ww->UpdateMenus();

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

//========================================================================
// UpdateWatch
// A single watch entry is updated here. Here's what's done:
//
// If the value has not changed since the last update, then the
// foreground and background colors are refreshed and we return.
//
// If the value has changed, then each of the value fields are refreshed.
// Then the foreground and background are refreshed.

void Watch_Window::UpdateWatch(WatchEntry *entry)
{
  int row;
  row=gtk_clist_find_row_from_data(GTK_CLIST(watch_clist),entry);
  if(row==-1)
    return;

  RegisterValue rvNewValue = entry->getRV();


  // If the value has not changed, then simply update the foreground and background
  // colors and return.

  if (entry->get_shadow() == rvNewValue) {
    gtk_clist_set_foreground(GTK_CLIST(watch_clist), row, gColors.normal_fg());
    gtk_clist_set_background(GTK_CLIST(watch_clist),
                             row,
                             (entry->hasBreak() ? gColors.breakpoint() : gColors.normal_bg()));
    return;
  }

  RegisterValue rvMaskedNewValue;
  unsigned int uBitmask;

  entry->put_shadow(rvNewValue);

  if(entry->pRegister) {
    rvMaskedNewValue = entry->pRegister->getRV_notrace();
    uBitmask = entry->pRegister->bit_mask; //getBitmask();
  }
  else {
    rvMaskedNewValue = entry->getRV();
    uBitmask = entry->cpu->register_mask();
  }

  char str[80];

  if(rvNewValue.init & uBitmask) {
    strcpy(str, "?");
  }
  else
    sprintf(str,"%d", rvNewValue.data);

  gtk_clist_set_text(GTK_CLIST(watch_clist), row, DECIMALCOL, str);

  // Hexadecimal representation:
  rvMaskedNewValue.toString(str, 80);
  gtk_clist_set_text(GTK_CLIST(watch_clist), row, HEXCOL, str);

  // ASCII representation
  str[0] = (rvNewValue.data>'0' && rvNewValue.data<='z') ? rvNewValue.data : 0;
  str[1] =0;
  gtk_clist_set_text(GTK_CLIST(watch_clist), row, ASCIICOL, str);

  // Bit representation
  char sBits[25];
  rvNewValue.toBitStr(sBits, 25, entry->cpu->register_mask(),
                               NULL);
  gtk_clist_set_text(GTK_CLIST(watch_clist), row, BITCOL, sBits);

  // Set foreground and background colors
  gtk_clist_set_foreground(GTK_CLIST(watch_clist), row, gColors.item_has_changed());
  gtk_clist_set_background(GTK_CLIST(watch_clist), row,
                           (entry->hasBreak() ? gColors.breakpoint() : gColors.normal_bg()));

}

//------------------------------------------------------------------------
// Update
//
//

void Watch_Window::Update()
{
  GList *iter;
  WatchEntry *entry;
  int clist_frozen=0;

  iter=watches;

  while(iter) {

    entry=(WatchEntry*)iter->data;
    if (entry)
      UpdateWatch(entry);

    iter=iter->next;
  }
  if(clist_frozen)
    gtk_clist_thaw(GTK_CLIST(watch_clist));
}
//------------------------------------------------------------------------
/*
void Watch_Window::Add( REGISTER_TYPE type, GUIRegister *reg)
{
  if(!gp || !gp->cpu || !reg || !reg->bIsValid())
    return;
  Register *cpu_reg = reg->get_register();
  register_symbol * pRegSym = get_symbol_table().findRegisterSymbol(
    cpu_reg->address);
  Add(type, reg, pRegSym);
}
*/

void Watch_Window::Add( REGISTER_TYPE type, GUIRegister *reg, Register * pReg)
{
  char vname[50], addressstring[50], empty[] = "";
  char *entry[COLUMNS]={vname, addressstring, empty, empty, empty, empty};
  int row;
  WatchWindowXREF *cross_reference;

  WatchEntry *watch_entry;

  if(!gp || !gp->cpu || !reg || !reg->bIsValid())
    return;

  if(!enabled)
    Build();


  pReg = pReg ? pReg : reg->get_register();
  if (!pReg)
    return;
  strncpy(vname,pReg?pReg->name().c_str():"NULLREG",sizeof(vname));

  /*
  if(pRegSym == 0) {
    cpu_reg = reg->get_register();
    strncpy(vname,cpu_reg->name().c_str(),sizeof(vname));
  }
  else {
    cpu_reg = pRegSym->getReg();
    strncpy(vname,pRegSym->name().c_str(),sizeof(vname));
  }
  */

  unsigned int uAddrMask = 0;
  unsigned int uLastAddr = gp->cpu->register_memory_size() - 1;
  while(uLastAddr) {
    uLastAddr>>=4;
    uAddrMask<<=4;
    uAddrMask |= 0xf;
  }
  strcpy(addressstring,
         GetUserInterface().FormatProgramAddress(pReg->getAddress(),
                                                 uAddrMask, IUserInterface::eHex));

  gtk_clist_freeze(GTK_CLIST(watch_clist));
  row=gtk_clist_append(GTK_CLIST(watch_clist), entry);

  watch_entry = new WatchEntry();
  watch_entry->address=reg->address;
  watch_entry->pRegister = pReg;
  watch_entry->cpu = gp->cpu;
  watch_entry->type=type;
  watch_entry->rma = reg->rma;

  gtk_clist_set_row_data(GTK_CLIST(watch_clist), row, (gpointer)watch_entry);

  watches = g_list_append(watches, (gpointer)watch_entry);

  UpdateWatch(watch_entry);

  cross_reference = new WatchWindowXREF();
  cross_reference->parent_window_type = WT_watch_window;
  cross_reference->parent_window = (gpointer) this;
  cross_reference->data = (gpointer) watch_entry;

  watch_entry->Assign_xref(cross_reference);
  gtk_clist_thaw(GTK_CLIST(watch_clist));

  UpdateMenus();

}

//---
// Add - given a symbol, verify that it is a RAM or EEPROM register
// symbol. If it is then extract the register and use it's address
// to get the GUI representation of the register.

void Watch_Window::Add( Value *regSym)
{
  if(regSym && gp) {

    Register *reg = dynamic_cast<Register *>(regSym);

    if(reg) {
      GUIRegister *greg = gp->m_pGUIRamRegisters->Get(reg->getAddress());
      Add(REGISTER_RAM, greg, reg);
    }
  }
}
//------------------------------------------------------------------------
// NewSymbols
//
void Watch_Window::NewProcessor(GUI_Processor *_gp)
{

  if (!gp || !gp->cpu)
    return;

  ReadSymbolList();
}
//------------------------------------------------------------------------
// ClearWatches
//
//

void Watch_Window::ClearWatches(void)
{
  GList *iter;
  WatchEntry *entry;
  int row;

  iter=watches;

  while(iter) {

    entry=(WatchEntry*)iter->data;
    row=gtk_clist_find_row_from_data(GTK_CLIST(watch_clist),entry);
    gtk_clist_remove(GTK_CLIST(watch_clist),row);
    entry->Clear_xref();
    free(entry);
    iter=iter->next;
  }

  while( (watches=g_list_remove_link(watches,watches))!=0)
    ;
}

//------------------------------------------------------------------------
// Build
//
//

void Watch_Window::Build(void)
{
  if(bIsBuilt)
    return;

  GtkWidget *vbox;
  GtkWidget *scrolled_window;

  int i;

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

  gtk_window_set_title(GTK_WINDOW(window), "Watch Viewer");

  gtk_window_set_default_size(GTK_WINDOW(window), width,height);
  gtk_widget_set_uposition(GTK_WIDGET(window),x,y);
  gtk_window_set_wmclass(GTK_WINDOW(window),name(),"Gpsim");

  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
                      GTK_SIGNAL_FUNC(delete_event), (gpointer)this);
  gtk_signal_connect_after(GTK_OBJECT(window), "configure_event",
                           GTK_SIGNAL_FUNC(gui_object_configure_event),this);

  watch_clist = gtk_clist_new_with_titles(COLUMNS,watch_titles);
  gtk_widget_show(watch_clist);

  for(i=0;i<LASTCOL;i++) {
    //gtk_clist_set_column_auto_resize(GTK_CLIST(watch_clist),i,TRUE);
    gtk_clist_set_column_resizeable(GTK_CLIST(watch_clist),i,TRUE);
    coldata[i].ww = this;
    coldata[i].column = i;
    coldata[i].Show();
  }

  gtk_clist_set_selection_mode (GTK_CLIST(watch_clist), GTK_SELECTION_BROWSE);

  gtk_signal_connect(GTK_OBJECT(watch_clist),"click_column",
                     (GtkSignalFunc)watch_click_column,0);
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

  scrolled_window=gtk_scrolled_window_new(0, 0);
  gtk_widget_show(scrolled_window);

  vbox = gtk_vbox_new(FALSE,1);
  gtk_widget_show(vbox);

  gtk_container_add(GTK_CONTAINER(scrolled_window), watch_clist);

  gtk_container_add(GTK_CONTAINER(window),vbox);

  gtk_box_pack_start_defaults(GTK_BOX(vbox),scrolled_window);

  popup_menu=build_menu(window,this);

  gtk_widget_show (window);


  enabled=1;

  bIsBuilt = true;

  UpdateMenuItem();

}


Watch_Window::Watch_Window(GUI_Processor *_gp)
{
  unsigned int i;

#define MAXROWS  (MAX_REGISTERS/REGISTERS_PER_ROW)
#define MAXCOLS  (REGISTERS_PER_ROW+1)

  menu = "<main>/Windows/Watch";

  set_name("watch_viewer");
  wc = WC_data;
  wt = WT_watch_window;
  window = 0;

  watches=0;
  current_row=0;

  gp = _gp;

  get_config();

  for(i=0;i<COLUMNS;i++) {

    if (!config_get_variable(name(),watch_titles[i],&coldata[i].isVisible))
      config_set_variable(name(),watch_titles[i],1);
  }

  {
    // Fix a db error in previous versions of gpsim
    int j;
    while (config_get_variable(name(),"hex",&j))
      config_remove(name(), "hex");

    const int hexIndex=3;
    config_set_variable(name(),watch_titles[hexIndex],coldata[hexIndex].isVisible);
  }

  if(enabled)
    Build();

}

#endif // HAVE_GUI
