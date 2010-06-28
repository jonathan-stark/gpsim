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

#include "../src/interface.h"
#include "../src/trace.h"
#include "../src/breakpoints.h"

#include "gui.h"
#include "preferences.h"
#include "gui_register.h"
#include "gui_statusbar.h"
#include "gui_regwin.h"
#include "gui_watch.h"


#define TRACE_FILE_FORMAT_ASCII 0
#define TRACE_FILE_FORMAT_LXT 1

extern int gui_question(const char *question, const char *a, const char *b);


// extern GUI_Processor *gp;

typedef enum {
    MENU_BREAK_CLEAR,
    MENU_BREAK_READ,
    MENU_BREAK_WRITE,
    MENU_BREAK_ON_CHANGE,
    MENU_BREAK_READ_VALUE,
    MENU_BREAK_WRITE_VALUE,
    MENU_ADD_WATCH,
    MENU_SETTINGS,
    MENU_LOG_SETTINGS,
    MENU_LOG_READ,
    MENU_LOG_WRITE,
    MENU_LOG_READ_VALUE,
    MENU_LOG_WRITE_VALUE,
    MENU_REGWIN_REFRESH,
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
  virtual ~MenuItem()
  {
  }

  virtual void execute()=0;
};

class RegWindowMenuItem : public MenuItem {
public:
  virtual ~RegWindowMenuItem()
  {
  }

private:
 Register_Window *rw;
};

class Menu_BreakClear : public RegWindowMenuItem {
public:
  virtual ~Menu_BreakClear()
  {
  }

  virtual void execute() {
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
    const char *name;
    menu_id id;
} menu_item;

static menu_item menu_items[] = {
    {"Clear breakpoints", MENU_BREAK_CLEAR},
    {"Set break on read", MENU_BREAK_READ},
    {"Set break on write", MENU_BREAK_WRITE},
    {"Set break on change", MENU_BREAK_ON_CHANGE},
    {"Set break on read value...", MENU_BREAK_READ_VALUE},
    {"Set break on write value...", MENU_BREAK_WRITE_VALUE},
    {"Set log settings...", MENU_LOG_SETTINGS},
    {"Set log on read", MENU_LOG_READ},
    {"Set log on write", MENU_LOG_WRITE},
    {"Set log on read value...", MENU_LOG_READ_VALUE},
    {"Set log on write value...", MENU_LOG_WRITE_VALUE},
    {"Add watch", MENU_ADD_WATCH},
    {"Refresh", MENU_REGWIN_REFRESH},
    {"Settings...", MENU_SETTINGS}
};

extern int font_dialog_browse(GtkWidget *w, gpointer user_data);
static int dlg_x=200, dlg_y=200;


// Used only in popup menus
Register_Window *popup_rw;

//========================================================================
//

//--------------------------------------------------
// get_register
// get the "real" register. If 'bTopLevelOnly' is true
//
Register *GUIRegister::get_register()
{
  if(!rma)
    return 0;

  return rma->get_register(address);
}

void GUIRegister::put_value(unsigned int new_value)
{

  Register *reg = get_register();

  if(reg)
    reg->put_value(new_value);

  // Shadow a copy of the register value so that we can tell if it has changed
  // when we go to perform an update in the future.

  shadow = reg->getRV_notrace();
}

void GUIRegister::put_shadow(RegisterValue new_value)
{
  // Update the shadow copy of the register without updating the register.
  shadow = new_value;
}

unsigned int GUIRegister::get_value()
{
  Register *reg = get_register();

  if(reg)
    return reg->get_value();

  return 0;
}


RegisterValue GUIRegister::getRV()
{
  Register *reg = get_register();

  if(reg)
    return reg->getRV_notrace();

  return RegisterValue(0,0);
}

char * GUIRegister::getValueAsString(char *str, int len, char *pFormat,
                                     RegisterValue value)
{

  if(!str || !len)
    return 0;

  if(bIsValid()) {

    char hex2ascii[] = "0123456789ABCDEF";
    int i;
    int min = (len < register_size*2) ? len : register_size*2;

    if(value.data == INVALID_VALUE)
      value.init = 0xfffffff;

    for(i=0; i < min; i++) {
      if(value.init & 0x0f)
        str[min-i-1] = '?';
      else
        str[min-i-1] = hex2ascii[value.data & 0x0f];
      value >>= 4;
    }
    str[min] = 0;

  } else
    *str = 0;

  return str;

}

bool GUIRegister::hasChanged(RegisterValue &current_value) const
{
  return (shadow != current_value);
}

void GUIRegister::Clear_xref()
{
  Register *reg = get_register();

  if(reg)
    reg->remove_xref((gpointer *)xref);
}

void GUIRegister::Assign_xref(CrossReferenceToGUI *new_xref)
{

  Register *reg = get_register();

  if(reg)
    reg->add_xref( (gpointer *)new_xref);

  xref = new_xref;
}

bool GUIRegister::hasBreak()
{

  if(rma)
    return rma->hasBreak(address);

  return false;
}

char *GUIRegister::name()
{

  static char buffer[128];

  Register *reg = get_register();
  if (!reg) {
    sprintf(buffer,"NULL");
    return buffer;
  }

  //register_symbol * pRegSym = get_symbol_table().findRegisterSymbol(reg->address);

  if(!reg || reg->isa()==Register::INVALID_REGISTER)
    return 0;

  // FIXME -- potential buffer overflow...
  if(bIsAliased)
    sprintf(buffer,"alias (%s)", reg->name().c_str());
  else
    sprintf(buffer,"%s", reg->name().c_str());

  /*  else {
    if(pRegSym == 0) {
      strcpy(buffer,reg->name().c_str());
    }
    else {
      strcpy(buffer,pRegSym->name().c_str());
    }
  }
  */
  return buffer;
}

bool GUIRegister::bIsValid()
{
  if(rma && (*rma)[address].getReg())
    return true;
  return false;
}

bool GUIRegister::bIsSFR()
{
  if(rma && (*rma)[address].isa() == Register::SFR_REGISTER)
    return true;
  return false;
}

GUIRegister:: GUIRegister()
{
  rma = 0;
  xref = 0;
}

GUIRegister::~GUIRegister()
{
  rma = 0;
  if(xref != NULL) {
    delete xref;
  }
}

//========================================================================

class RegisterWindowXREF : public CrossReferenceToGUI
{
public:

  void Update(int new_value)
  {
    GUIRegister *reg;
    Register_Window *rw;
    int address;


    reg = (GUIRegister *) (data);
    rw  = (Register_Window *) (parent_window);

    if(reg->row > GTK_SHEET(rw->register_sheet)->maxrow)
      {
      puts("Warning reg->row > maxrow in xref_update_cell");
      return;
      }

    address = rw->row_to_address[reg->row]+reg->col;

    rw->registers->Get(address)->bUpdateFull=true;
    rw->UpdateRegisterCell(address);

    rw->UpdateASCII(reg->row);
  }

};


//========================================================================
//
// Create a class for an invalid register and instantiate a single instance
// of it. The purpose of this is to provide a place holder for the gui
// register array.
//

class InvalidGuiRegister : public GUIRegister {
public:
  void put_value(unsigned int new_value)
  {
    printf("(gui_regwin)Warning: writing to invalid register\n");
  };
  unsigned int get_value() { return 0;};

  InvalidGuiRegister() {
    rma=0;
  }
private:
  void operator delete(void *ignore) {};
};

static InvalidGuiRegister THE_invalid_register;

//========================================================================
// GtkSheet extensions
//
// The gtk_sheet api does not provide access to row and column labels.
// This means we can manipulate the font the way the cells can be manipulated

#if 0 // defined but not used
static GtkSheetButton *
gtk_sheet_row_button_get(GtkSheet *sheet, gint row)
{
  g_return_val_if_fail (sheet != NULL, NULL);
  g_return_val_if_fail (GTK_IS_SHEET (sheet), NULL);

  if(row < 0 || row > sheet->maxrow) return NULL;

  return (&sheet->row[row].button);
}
#endif

static void
gtk_sheet_REALLY_set_row_height(GtkSheet *sheet, gint row, gint height)
{
  g_return_if_fail (sheet != NULL);
  g_return_if_fail (GTK_IS_SHEET (sheet));

  if (row < 0 || row > sheet->maxrow)
    return;

  sheet->row[row].height = height;
}

//========================================================================
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
int gui_get_value(const char *prompt)
{
    static GtkWidget *dialog=0;
    static GtkWidget *label;
    static GtkWidget *entry;
    GtkWidget *button;
    GtkWidget *hbox;

    int retval=-1;

    int value;

    if(dialog==0) {
      dialog = gtk_dialog_new();
      gtk_window_set_title(GTK_WINDOW(dialog),"enter value");
      //        gtk_signal_connect(GTK_OBJECT(dialog),
      //                           "configure_event",GTK_SIGNAL_FUNC(configure_event),0);
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
    else {
      gtk_label_set_text(GTK_LABEL(label),prompt);
    }

//    gtk_widget_set_uposition(GTK_WIDGET(dialog),dlg_x,dlg_y);
    gtk_widget_show_now(dialog);

    gtk_grab_add(dialog);
    while(retval==-1 && GTK_WIDGET_VISIBLE(dialog))
      gtk_main_iteration();
    gtk_grab_remove(dialog);

    gtk_widget_hide(dialog);

    if(retval==(int)TRUE)
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
void gui_get_2values(const char *prompt1, int *value1, const char *prompt2, int *value2)
{
    static GtkWidget *dialog=0;
    GtkWidget *button;
    GtkWidget *hbox1, *hbox2;
    static GtkWidget *label1, *label2, *label;
    static GtkWidget *entry1, *entry2;

    int value;

    int retval=-1;

    if(dialog==0)
    {
        dialog = gtk_dialog_new();
        gtk_window_set_title(GTK_WINDOW(dialog),"enter values");
//      gtk_signal_connect(GTK_OBJECT(dialog),
//                         "configure_event",GTK_SIGNAL_FUNC(configure_event),0);
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

    if(retval==(int)TRUE)
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
    file_selection_name=gtk_file_selection_get_filename (fs);
    fs_done=1;
}

static void
file_selection_cancel (GtkWidget        *w,
                       GtkFileSelection *fs)
{
    file_selection_name=0;
    fs_done=1;
}

static void
modepopup_activated(GtkWidget *widget, gpointer data)
{
  Dprintf((" modepopup_activated\n"));

    char *modestring;

    modestring=(char*)data;

    if(!strcmp(modestring,"ASCII"))
        filemode=TRACE_FILE_FORMAT_ASCII;
    if(!strcmp(modestring,"LXT"))
        filemode=TRACE_FILE_FORMAT_LXT;
}

static char *gui_get_log_settings(const char **filename, int *mode)
{
    static GtkWidget *window = 0;

    GtkWidget *hbox, *optionmenu, *label;

    GtkWidget *menu;
    GtkWidget *item;

    const char *prompt="Log settings";

    if (!window)
    {

        window = gtk_file_selection_new (prompt);

        gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (window));

        gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);

        gtk_signal_connect_object(GTK_OBJECT(window),
                                  "delete_event",GTK_SIGNAL_FUNC(gtk_widget_hide),GTK_OBJECT(window));
//      gtk_signal_connect_object (GTK_OBJECT (window), "destroy",
//                                 GTK_SIGNAL_FUNC(gtk_widget_destroyed),
//                                 GTK_OBJECT(window));

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

    file_selection_name=0;
    gtk_widget_show_now(window);

    fs_done=0;
    file_selection_name=0;
    gtk_grab_add(window);
    while(!fs_done && GTK_WIDGET_VISIBLE(window))
        gtk_main_iteration();
    gtk_grab_remove(window);

    gtk_widget_hide(window);

    if(file_selection_name==0)
    {
        *filename=0;
        return 0;
    }

    *filename=file_selection_name;
    *mode=filemode;
    return 0;
}

// called when user has selected a menu item
static void
popup_activated(GtkWidget *widget, gpointer data)
{
  GtkSheet *sheet;

  menu_item *item;
  int i,j;
  GtkSheetRange range;
  unsigned int address;
  int value, mask;
  const char *filename;
  int mode;

  Dprintf((" popup_activated\n"));

  if(widget==0 || data==0)
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


  for(j=range.row0;j<=range.rowi;j++)
    for(i=range.col0;i<=range.coli;i++) {

      address=popup_rw->row_to_address[j]+i;
      switch(item->id) {
      case MENU_BREAK_READ:
        get_bp().set_read_break(popup_rw->gp->cpu, address);
        break;

      case MENU_BREAK_WRITE:
        get_bp().set_write_break(popup_rw->gp->cpu, address);
        break;
      case MENU_BREAK_ON_CHANGE:
        get_bp().set_change_break(popup_rw->gp->cpu, address);
        break;
      case MENU_BREAK_READ_VALUE:
        value = gui_get_value("value to read for breakpoint:");
        if(value<0)
          break; // Cancel
        get_bp().set_read_value_break(popup_rw->gp->cpu,address,value);
        break;
      case MENU_BREAK_WRITE_VALUE:
        value = gui_get_value("value to write for breakpoint:");
        if(value<0)
          break; // Cancel
        get_bp().set_write_value_break(popup_rw->gp->cpu,address,value);
        break;
      case MENU_BREAK_CLEAR:
        get_bp().clear_all_register(popup_rw->gp->cpu,address);
        break;
      case MENU_ADD_WATCH:
        popup_rw->gp->watch_window->Add(popup_rw->type, popup_rw->registers->Get(address));
        break;
      case MENU_LOG_READ:
        GetTraceLog().enable_logging();
        // FIXME the register type is ignored here (and in all other cases
        // where we're logging -- it's assumed that the register address is
        // for ram, even if in fact the user requests eeprom.
        get_bp().set_notify_read(popup_rw->gp->cpu,address);
        break;
      case MENU_LOG_WRITE:
        get_bp().set_notify_write(popup_rw->gp->cpu,address);
        break;
      case MENU_LOG_READ_VALUE:
        gui_get_2values("Value that the read must match for logging it:", &value,
                        "Bitmask that specifies the bits to bother about:", &mask);
        if(value<0)
          break; // Cancel
        get_bp().set_notify_read_value(popup_rw->gp->cpu,address, value, mask);
        break;
      case MENU_LOG_WRITE_VALUE:
        gui_get_2values("Value that the write must match for logging it:", &value,
                        "Bitmask that specifies the bits to bother about:", &mask);
        if(value<0)
          break; // Cancel
        get_bp().set_notify_write_value(popup_rw->gp->cpu,address, value, mask);
        break;
      case MENU_SETTINGS:
        popup_rw->SettingsDialog();
        return;
        break;
      case MENU_LOG_SETTINGS:
        gui_get_log_settings(&filename, &mode);
        if(filename!=0)
          GetTraceLog().enable_logging(filename,mode);
        return;
        break;

      case MENU_REGWIN_REFRESH:
        popup_rw->Update();
        return;
        break;
      default:
        puts("Unhandled menuitem?");
        break;

      }
    }

#if 0
  switch(item->id)
    {
    case MENU_BREAK_READ:
      for(j=range.row0;j<=range.rowi;j++)
        for(i=range.col0;i<=range.coli;i++)
          {
            address=popup_rw->row_to_address[j]+i;
            get_bp().set_read_break(popup_rw->gp->cpu, address);
          }
      break;
    case MENU_BREAK_WRITE:
      for(j=range.row0;j<=range.rowi;j++)
        for(i=range.col0;i<=range.coli;i++)
          {
            address=popup_rw->row_to_address[j]+i;
            get_bp().set_write_break(popup_rw->gp->cpu, address);
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
            get_bp().set_read_value_break(popup_rw->gp->cpu,address,value);
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
            get_bp().set_write_value_break(popup_rw->gp->cpu,address,value);
          }
      break;
    case MENU_BREAK_CLEAR:
      for(j=range.row0;j<=range.rowi;j++)
        for(i=range.col0;i<=range.coli;i++)
          {
            address=popup_rw->row_to_address[j]+i;
            get_bp().clear_all_register(popup_rw->gp->cpu,address);
          }
      break;
    case MENU_ADD_WATCH:
      for(j=range.row0;j<=range.rowi;j++)
        for(i=range.col0;i<=range.coli;i++)
          {
            address=popup_rw->row_to_address[j]+i;
            popup_rw->gp->watch_window->Add(popup_rw->type, popup_rw->registers->Get(address));
          }
      break;
    case MENU_SETTINGS:
      popup_rw->SettingsDialog();
      break;
    case MENU_LOG_SETTINGS:
      gui_get_log_settings(&filename, &mode);
      if(filename!=0)
        GetTraceLog().enable_logging(filename,mode);
      break;
    case MENU_LOG_READ:
      for(j=range.row0;j<=range.rowi;j++) {
        for(i=range.col0;i<=range.coli;i++)
        {
          address=popup_rw->row_to_address[j]+i;
          GetTraceLog().enable_logging();
          // FIXME the register type is ignored here (and in all other cases
          // where we're logging -- it's assumed that the register address is
          // for ram, even if in fact the user requests eeprom.
          get_bp().set_notify_read(popup_rw->gp->cpu,address);
        }
      }
      break;
    case MENU_LOG_WRITE:
      for(j=range.row0;j<=range.rowi;j++)
        for(i=range.col0;i<=range.coli;i++)
          {
            address=popup_rw->row_to_address[j]+i;
            get_bp().set_notify_write(popup_rw->gp->cpu,address);
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
            get_bp().set_notify_read_value(popup_rw->gp->cpu,address, value, mask);

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
          get_bp().set_notify_write_value(popup_rw->gp->cpu,address, value, mask);
          }
      break;
    case MENU_REGWIN_REFRESH:
      popup_rw->Update();
      break;
    default:
      puts("Unhandled menuitem?");
      break;
    }
#endif // 0
}


static GtkWidget *
build_menu(Register_Window *rw)
{
  GtkWidget *menu;
  GtkWidget *item;
//  GtkAccelGroup *accel_group;
  int i;


  if(rw==0)
  {
      printf("Warning build_menu(%p)\n",rw);
      return 0;
  }

  menu=gtk_menu_new();


  item = gtk_tearoff_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), item);
  gtk_widget_show (item);


  for (i=0; i < (int)(sizeof(menu_items)/sizeof(menu_items[0])) ; i++){
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
  //    GdkModifierType mods;
  GtkSheet *sheet;

  popup=rw->popup_menu;

  if(widget==0 || event==0 || rw==0)
    {
      printf("Warning do_popup(%p,%p,%p)\n",widget,event,rw);
      return 0;
    }

  sheet=GTK_SHEET(widget);

  if( (event->type == GDK_BUTTON_PRESS) &&  (event->button == 3) )
    {

      popup_rw = rw;

      gtk_menu_popup(GTK_MENU(popup), 0, 0, 0, 0,
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

  if(number_string==0)
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
  Dprintf((" set_cell\n"));

  GtkSheet *sheet;
  const gchar *text;
  int n=0;
  //int crow, ccol;

  sheet=GTK_SHEET(widget);

  if(widget==0 ||
     row>sheet->maxrow || row<0 ||
     col>sheet->maxcol || col<0 || rw==0)
    {
      printf("Warning set_cell(%p,%x,%x,%p)\n",widget,row,col,rw);
      return;
    }


  GUIRegister *reg = rw->getRegister(row,col);


  if(!reg)
    return; // ignore user changes in ascii column for right now

  // extract value from sheet cell
  GtkWidget * sheet_entry = gtk_sheet_get_entry(sheet);
  if (!sheet_entry)
    return;

  text = gtk_entry_get_text(GTK_ENTRY(sheet_entry));

  errno = 0;
  if(text!=0 && strlen(text)>0)
    n = get_number_in_string(text);
  else
    errno = ERANGE;

  if(errno != 0)
    {
      n = reg->get_value();
      reg->put_shadow(RegisterValue(INVALID_VALUE,INVALID_VALUE));
    }

  // n is the value in the sheet cell

  if(errno != EINVAL && n != (int) reg->get_shadow().data)
    {
      reg->put_value(n & gpGuiProcessor->cpu->register_mask());
      rw->UpdateASCII(row);
    }

}

//------------------------------------------------------------------------
// int column_width(int col)
//
// Return the width of one of the register sheet columns.
//
// Column = -1 is the row label
// Columns 0-REGISTERS_PER_ROW are ram/registers
// Column REGISTERS_PER_ROW is an ASCII string of the row data.
//
// The width of the column is based on width of a single character,
// char_width. This width is depends on the font and is computed in
// LoadStyles()
//
// FIXME: column_width assumes there are fewer than 0x1000 registers.
int Register_Window::column_width(int col)
{
  if(!char_width)
    return 0;

  // Row Labels
  if(col < 0)
    return char_width * 3;

  // Register data
  if(col < REGISTERS_PER_ROW)
    return char_width * chars_per_column;

  // ASCII column
  return char_width * (REGISTERS_PER_ROW + 1) + char_width/2;
}

//------------------------------------------------------------------------
int Register_Window::row_height(int row)
{
  return char_height ? char_height : 20;
}

//------------------------------------------------------------------------
// UpdateLabel
//
//

void Register_Window::UpdateLabel()
{
  int row = -1, col = -1;

  if(register_sheet != 0) {
    gtk_sheet_get_active_cell(register_sheet, &row, &col);

    if(col > -1 && row > -1) {
      if(col >= REGISTERS_PER_ROW)
        gtk_label_set(GTK_LABEL(location), "  ascii  ");
      else {

        GUIRegister *reg = getRegister(row,col);

        const char *n = reg ? reg->name() : "INVALID_REGISTER";

        gtk_label_set(GTK_LABEL(location), n);
      }
    }
  }
}

//------------------------------------------------------------------------
// UpdateEntry
//
//

void Register_Window::UpdateEntry()
{
  gint row, col;

  const char *text;
  GtkWidget * sheet_entry;

  if(register_sheet != 0) {
    sheet_entry = gtk_sheet_get_entry(register_sheet);
    gtk_sheet_get_active_cell(register_sheet, &row, &col);

    if(row_to_address[row] < 0)
      return;

    GUIRegister *reg = getRegister(row,col);

    if(reg && reg->bIsValid() )
    {
      if((text=gtk_entry_get_text (GTK_ENTRY(sheet_entry))))
          gtk_entry_set_text(GTK_ENTRY(entry), text);
    }
  }
}

//------------------------------------------------------------------------
// UpdateLabelEntry
//
//

void Register_Window::UpdateLabelEntry()
{
  UpdateLabel();
  UpdateEntry();
}

static gint configure_event(GtkWidget *widget, GdkEventConfigure *e, gpointer data)
{

  if(widget->window==0)
    return 0;

  gdk_window_get_root_origin(widget->window,&dlg_x,&dlg_y);
  return 0; // what should be returned?, FIXME
}

//------------------------------------------------------------------------
void Register_Window::UpdateStyle()
{
  if (!register_sheet || !normalfont)
    return;

  GtkSheetRange range;

  //gtk_sheet_freeze(register_sheet);

  // Update the font for the cells
  range.row0=0;
  range.rowi=register_sheet->maxrow;
  range.col0=0;
  range.coli=register_sheet->maxcol;
  gtk_sheet_range_set_font(register_sheet, &range, normalfont);

  // Update the font for the row and column labels
  gtk_widget_modify_font(GTK_WIDGET(register_sheet),normalfont);

  // Adjust the cell sizes based on the font size
  int i;
  for(i=0; i<=register_sheet->maxcol; i++)
    gtk_sheet_set_column_width (register_sheet, i, column_width(i));
  for(i=0; i<=register_sheet->maxrow; i++)
    gtk_sheet_REALLY_set_row_height (register_sheet, i, row_height(i));


  gtk_sheet_set_row_titles_width(register_sheet, column_width(-1));
  gtk_sheet_set_column_titles_height(register_sheet, row_height(0));
  //gtk_sheet_thaw(register_sheet);


}

//------------------------------------------------------------------------
int Register_Window::LoadStyles()
{
#if GTK_MAJOR_VERSION >= 2
  normalfont = pango_font_description_from_string(normalfont_string);
#else
  normalfont=gdk_fontset_load (normalfont_string);
#endif

  if(!normalfont)
  {
    char_width = 0;
    char_height = 0;
    return 0;
  }

#if GTK_MAJOR_VERSION >= 2
  {
    PangoRectangle rect;
    PangoLayout *layout;

    layout = gtk_widget_create_pango_layout (GTK_WIDGET(register_sheet), "A");
    pango_layout_set_font_description (layout, normalfont);

    pango_layout_get_extents (layout, NULL, &rect);
    char_width =  PANGO_PIXELS(rect.width);
    char_height = PANGO_PIXELS(rect.height + (rect.height<<1))>>1;
    g_object_unref(G_OBJECT(layout));
  }

#else
  char_width = gdk_string_width (normalfont,"9");
#endif

  return 1;
}

/********************** Settings dialog ***************************/
static int settings_active;
static void settingsok_cb(GtkWidget *w, gpointer user_data)
{
  if(settings_active)
    settings_active=0;
}

int Register_Window::SettingsDialog()
{
  static GtkWidget *dialog=0;    // fixme
  GtkWidget *button;
  GtkWidget *hbox;
  static GtkWidget *normalfontstringentry;   //fixme
  GtkWidget *label;
  int fonts_ok=0;


  if(!dialog)
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

  gtk_entry_set_text(GTK_ENTRY(normalfontstringentry), normalfont_string);

  gtk_widget_set_uposition(GTK_WIDGET(dialog),dlg_x,dlg_y);
  gtk_widget_show_now(dialog);



#if GTK_MAJOR_VERSION >= 2
  PangoFontDescription *font=0;
#else
  GdkFont *font=0;
#endif

  while(fonts_ok!=1)
  {
    char fontname[256];

    settings_active=1;
    while(settings_active)
      gtk_main_iteration();

    fonts_ok=0;

    strcpy(fontname,gtk_entry_get_text(GTK_ENTRY(normalfontstringentry)));
#if GTK_MAJOR_VERSION >= 2
    if((font=pango_font_description_from_string(fontname))==0)
#else
      if((font=gdk_fontset_load(fontname))==0)
#endif
        {
          if(gui_question("Font did not load!","Try again","Ignore/Cancel")==FALSE)
            break;
        }
      else
        {
#if GTK_MAJOR_VERSION >= 2
#else
          gdk_font_unref(font);
#endif
          strcpy(normalfont_string,gtk_entry_get_text(GTK_ENTRY(normalfontstringentry)));
          config_set_string(name(),"normalfont",normalfont_string);
          fonts_ok++;
        }
  }

  if(!LoadStyles())
  {
    printf("no font is available\n");
    return 0;
  }

  gtk_sheet_freeze(register_sheet);
  UpdateStyle();
  gtk_sheet_thaw(register_sheet);
  gtk_widget_hide(dialog);

  return 0;
}

static gboolean
clipboard_handler(GtkWidget *widget, GdkEventKey *key)
{
  Dprintf((" clipboard_handler\n"));

  GtkSheet *sheet;

  sheet = GTK_SHEET(widget);

  if(key->state & GDK_CONTROL_MASK || key->keyval==GDK_Control_L ||
     key->keyval==GDK_Control_R){
    if((key->keyval=='c' || key->keyval == 'C') && sheet->state != GTK_STATE_NORMAL){
#if GTK_MAJOR_VERSION >= 2
      /*
        --- tsd - commented out because this function
        is not defined in the official gtkextra-2.0 release.
      if (gtk_sheet_in_clip(sheet))
        gtk_sheet_unclip_range(sheet);
      */
#else
      if(GTK_SHEET_IN_CLIP(sheet))
        gtk_sheet_unclip_range(sheet);
#endif
      gtk_sheet_clip_range(sheet, &sheet->range);
    }
    if(key->keyval=='x' || key->keyval == 'X')
            gtk_sheet_unclip_range(sheet);
  }
  return 0;
}

static void
resize_handler(GtkWidget *widget, GtkSheetRange *old_range,
                                  GtkSheetRange *new_range,
                                  Register_Window *rw)
{
  Dprintf((" resize_handler\n"));

  int i, j, cti, ctj;
  int from, to;

  if(widget==0 || old_range==0 || new_range==0 || rw==0)
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
          rw->registers->Get(to)->put_value(rw->registers->Get(from)->get_value());
        }
    }
}

static void
move_handler(GtkWidget *widget,
             GtkSheetRange *old_range,
             GtkSheetRange *new_range,
             Register_Window *rw)
{
  Dprintf((" move_handler\n"));

  int i, j, cti, ctj;
  int from, to;

  if(!widget || !old_range || !new_range || !rw)
      return;

  cti = new_range->coli - new_range->col0 + 1;
  ctj = new_range->rowi - new_range->row0 + 1;

  for(j=0;j<ctj;j++)
    {
      for(i=0;i<cti;i++)
        {
          from = rw->row_to_address[old_range->row0+j]+old_range->col0+i;
          to = rw->row_to_address[new_range->row0+j]+new_range->col0+i;
          rw->registers->Get(to)->put_value(rw->registers->Get(from)->get_value());
        }
    }
}


/* when the entry above the sheet is changed (typed a digit), we
   copy it to the cell entry */
static void
show_sheet_entry(GtkWidget *widget, Register_Window *rw)
{
  Dprintf((" show_sheet_entry\n"));

  const char *text;
  GtkSheet *sheet;
  GtkEntry *sheet_entry;

  int row,col;

  if(!widget || !rw)
    {
      printf("Warning show_sheet_entry(%p,%p)\n",widget,rw);
      return;
    }

  if(!GTK_WIDGET_HAS_FOCUS(widget)) return;

  sheet=GTK_SHEET(rw->register_sheet);
  sheet_entry = GTK_ENTRY(gtk_sheet_get_entry(sheet));

  //row=sheet->active_cell.row; col=sheet->active_cell.col;
  gtk_sheet_get_active_cell(sheet, &row, &col);

  GUIRegister *reg = rw->getRegister(row,col);

  if(reg && reg->bIsValid() )
    if((text=gtk_entry_get_text (GTK_ENTRY(rw->entry))) && sheet_entry)
      gtk_entry_set_text(sheet_entry, text);

}

/* when we have new data in the entry above the sheet, we
 copy the data to the cells/registers

 this don't get called when it is clicked
 in, only when we hit return
 */
static void
activate_sheet_entry(GtkWidget *widget, Register_Window *rw)
{
  Dprintf((" activity_sheet_entry\n"));

  GtkSheet *sheet;

  gint row, col;

  if(widget==0|| rw==0)
  {
      printf("Warning activate_sheet_entry(%p,%p)\n",widget,rw);
      return;
  }

  sheet=GTK_SHEET(rw->register_sheet);
  //row=sheet->active_cell.row; col=sheet->active_cell.col;
  gtk_sheet_get_active_cell(sheet, &row, &col);

  // if there are text written in the entry above the sheet, then
  // the same data is in the sheet cell (because of show_sheet_entry())

  // so we use set_cell() to write the changes from the sheet cell to gpsim
  set_cell(GTK_WIDGET(sheet),row,col,rw);
  rw->UpdateASCII(row);

}

/*
 we get here when the entry in a cell is changed (typed a digit), we
 copy it to the entry above the sheet.
 */
static void
show_entry(GtkWidget *widget, Register_Window *rw)
{
  Dprintf((" show_entry\n"));

    if(widget==0|| rw==0)
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
  Dprintf((" activate_sheet_cell rma=%p\n",(rw? rw->rma :0)));

  GtkSheet *sheet = rw ? rw->register_sheet : 0;

  if(!sheet)
    return 0;

  if(widget==0 || row>sheet->maxrow || row<0||
     column>sheet->maxcol || column<0 || rw==0)
    {
      printf("Warning activate_sheet_cell(%p,%x,%x,%p)\n",widget,row,column,rw);
      return 0;
    }

  GUIRegister *reg = rw->getRegister(row,column);

  if(reg && reg->bIsValid() )
    // enable editing valid cells
    gtk_entry_set_editable(GTK_ENTRY(gtk_sheet_get_entry(rw->register_sheet)), 1);
  else
    // disable editing invalid cells
    gtk_entry_set_editable(GTK_ENTRY(gtk_sheet_get_entry(rw->register_sheet)), 0);


  rw->UpdateLabelEntry();

  return TRUE;
}

GUIRegister *Register_Window::getRegister(int row, int col)
{

  if(registers && col < REGISTERS_PER_ROW && row < MAX_ROWS) {

    int reg_address = row_to_address[row];

    if(reg_address < 0)
      return 0;

    if(reg_address+ col < MAX_REGISTERS)
      return registers->Get(reg_address+col);
  }

  return 0;

}

//-------------------------------------------------------------------
GUIRegister *Register_Window::operator [] (int address)
{

  if(!registers || address>=MAX_REGISTERS  || address<0)
    return 0;

  return registers->Get(address);
}

void Register_Window::SelectRegister(int regnumber)
{
  GtkSheetRange range;
  int row, col;

  if(regnumber > MAX_REGISTERS || regnumber<0) {
    printf("Warning: %s - regnumber = %x\n",__FUNCTION__,regnumber);
    return;
  }

  if(!gp || !gp->cpu ||!registers || !registers->Get(regnumber)) {
    printf("SelectRegister is not ready yet\n");
    return;
  }
  row=registers->Get(regnumber)->row;
  col=registers->Get(regnumber)->col;
  range.row0=range.rowi=row;
  range.col0=range.coli=col;

  gtk_sheet_select_range(GTK_SHEET(register_sheet),&range);
  if(register_sheet != NULL &&
     (GTK_SHEET(register_sheet)->view.col0>range.col0 ||
     GTK_SHEET(register_sheet)->view.coli<range.coli ||
     GTK_SHEET(register_sheet)->view.row0>range.row0 ||
      GTK_SHEET(register_sheet)->view.rowi<range.rowi))
    gtk_sheet_moveto(GTK_SHEET(register_sheet),row,col,0.5,0.5);

  UpdateLabelEntry();

}
void Register_Window::SelectRegister(Value *regSym)
{
  Register *pReg = dynamic_cast<Register *>(regSym);
  if (pReg && register_sheet)
    SelectRegister(pReg->getAddress());
  /*
  if(regSym  && typeid(*regSym) == typeid(register_symbol) &&
      register_sheet != NULL) {
    Register* pReg = (Register*)((register_symbol*)regSym)->getReg();
    SelectRegister(pReg->address);
  }
  */
}
static void
build_entry_bar(GtkWidget *main_vbox, Register_Window *rw)
{
  Dprintf((" build_entry_bar\n"));

  GtkRequisition request;
  GtkWidget *status_box;

  if(main_vbox == 0 || rw==0)
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

void Register_Window::UpdateASCII(gint row)
{
  gint i;
  gchar name[32];

  if(row<0 || row > register_sheet->maxrow)
  {
      printf("Warning update_ascii(%x)\n",row);
      return;
  }

  if(!registers_loaded)
      return;

  for(i=0; i<REGISTERS_PER_ROW; i++)
  {

    name[i] = registers->Get(row_to_address[row] + i)->get_shadow().data;

    if( (name[i] < ' ') || (name[i]>'~'))
      name[i] = '.';

  }

  name[REGISTERS_PER_ROW] = 0;

  gtk_sheet_set_cell(GTK_SHEET(register_sheet), row,REGISTERS_PER_ROW, GTK_JUSTIFY_RIGHT,name);

}

gboolean Register_Window::UpdateRegisterCell(unsigned int reg_number)

{
  static gboolean bTrace = false;
  gchar name[16];

  GtkSheetRange range;
  gboolean retval=FALSE;

  if(reg_number<0 || reg_number>=MAX_REGISTERS)
  {
      printf("Warning update_register_cell(%x)\n",reg_number);
      return 0;
  }

  if(!enabled)
    return 0;      // Don't read registers when hidden. Esp with ICD.

  GUIRegister *guiReg = registers->Get(reg_number);

  if (!guiReg || !guiReg->rma)
    return 0;

  if(reg_number >= guiReg->rma->get_size())
    return 0;

  range.row0=guiReg->row;
  range.rowi=guiReg->row;
  range.col0=guiReg->col;
  range.coli=guiReg->col;

  // bulk mode stuff is for the ICD.
  gpsim_set_bulk_mode(1);
  RegisterValue new_value = guiReg->getRV();
  gpsim_set_bulk_mode(0);

  RegisterValue last_value=guiReg->get_shadow();

  if(bTrace)
    printf("UpdateRegisterCell() Entry: regID=%3d, Full=%s, hasChanged=%s\n",
      reg_number, guiReg->bUpdateFull ? "true " : "false",\
      guiReg->hasChanged(new_value) ? "true " : "false");
  if(guiReg->bUpdateFull) {

    // A 'Full Update' means that the foreground and background colors
    // need to be repainted.

    guiReg->bUpdateFull=false;

    if(guiReg->row<=register_sheet->maxrow) {

      guiReg->getValueAsString(name,sizeof(name),pCellFormat, new_value);

      gtk_sheet_set_cell(GTK_SHEET(register_sheet),
                         guiReg->row,
                         guiReg->col,
                         GTK_JUSTIFY_RIGHT,name);
    }
    // else the register is invalid and out of the register sheet


    //if(new_value != last_value) {
    if(guiReg->hasChanged(new_value)) {

      guiReg->put_shadow(new_value);
      guiReg->bUpdateFull=true;
      if(bTrace)
        printf("UpdateRegisterCell()    regID=3%d, bUpdateFull set to true 1\n", reg_number);
      gtk_sheet_range_set_foreground(GTK_SHEET(register_sheet), &range, gColors.item_has_changed());
    } else
      gtk_sheet_range_set_foreground(GTK_SHEET(register_sheet), &range, gColors.normal_fg());

    if(bTrace)
      printf("UpdateRegisterCell()    Background\n");
    if(guiReg->hasBreak())
      gtk_sheet_range_set_background(GTK_SHEET(register_sheet), &range, gColors.breakpoint());
    else if(!guiReg->bIsValid())
      gtk_sheet_range_set_background(GTK_SHEET(register_sheet), &range, gColors.invalid());
    else if(guiReg->bIsAliased)
      gtk_sheet_range_set_background(GTK_SHEET(register_sheet), &range, gColors.alias());
    else if(guiReg->bIsSFR())
      gtk_sheet_range_set_background(GTK_SHEET(register_sheet), &range, gColors.sfr_bg());
    else
      gtk_sheet_range_set_background(GTK_SHEET(register_sheet), &range, gColors.normal_bg());


    retval=TRUE;
  } else if(guiReg->hasChanged(new_value)) { //new_value!=last_value) {

    if(new_value.data==INVALID_VALUE) {

      guiReg->put_shadow(RegisterValue(INVALID_VALUE,INVALID_VALUE));
      sprintf (name, "??");
    } else {

      // the register has changed since last update
      guiReg->put_shadow(new_value);
      guiReg->getValueAsString(name,sizeof(name),pCellFormat, new_value);
      //sprintf (name, pCellFormat, new_value.data);
    }

    gtk_sheet_set_cell(GTK_SHEET(register_sheet),
                       guiReg->row,
                       guiReg->col,
                       GTK_JUSTIFY_RIGHT,name);

    guiReg->bUpdateFull=true;
    if(bTrace)
      printf("UpdateRegisterCell()    regID=3%d, bUpdateFull set to true 2\n", reg_number);
    gtk_sheet_range_set_foreground(GTK_SHEET(register_sheet), &range, gColors.item_has_changed());

    retval=TRUE;
  }

  gint row,col;
  gtk_sheet_get_active_cell(register_sheet, &row, &col);

  if((int)reg_number==(row_to_address[row]+col))
  {
    // if sheet cursor is standing on a cell that is changed, then
    // we update the entry above the sheet
    if(new_value.data!=last_value.data)
      UpdateEntry();
  }

  if(bTrace)
    printf("UpdateRegisterCell() Exit:  regID=%3d, Full=%s, hasChanged=%s, retval=%s\n",
      reg_number, guiReg->bUpdateFull
      ? "true " : "false",
    guiReg->hasChanged(new_value) ? "true " : "false", retval ? "true " : "false");
  return retval;
}


void Register_Window::Update()
{

  int address;
  bool bRowChanged;
  int j, i;

  if(!enabled)
    return;

  if(!GTK_WIDGET_VISIBLE(window))
    return;

  if(!registers_loaded)
    return;

  if(!gp || !gp->cpu || !register_sheet || !gp->cpu->isHardwareOnline()) {
    puts("Warning can't update register window");
    return;
  }

  gtk_sheet_freeze(register_sheet);
  for(j = 0; j<=GTK_SHEET(register_sheet)->maxrow; j++) {

    if(row_to_address[j]==-1)
      continue;

    bRowChanged = false;
    for(i = 0; i<REGISTERS_PER_ROW; i++) {
      address = row_to_address[j]+i;
      GUIRegister * pGuiReg = registers->Get(address);
      if(pGuiReg != &THE_invalid_register &&
        (pGuiReg->get_shadow().data!=INVALID_VALUE ||
        pGuiReg->bUpdateFull)) {

        if(UpdateRegisterCell(address) == (gboolean)true)
          bRowChanged = true;
      }
    }
    if(bRowChanged)
      UpdateASCII(j);
  }

  gtk_sheet_thaw(register_sheet);

}

//------------------------------------------------------------------------
void Register_Window::SetRegisterSize()
{
  if(gp && gp->cpu)
    register_size = gp->cpu->register_size();
  else
    register_size = 1;

  chars_per_column = 1 + 2*register_size;

  if(pCellFormat)
    free(pCellFormat);

  pCellFormat = (char *) malloc(10 * sizeof(char));
  sprintf(pCellFormat,"%%0%dx",register_size*2);

  if(register_sheet) {

    int i;
    char buffer[10];

    // Column labels
    for(i=0; i<register_sheet->maxcol; i++){
      sprintf(buffer,"%02x",i);
      gtk_sheet_column_button_add_label(register_sheet, i, buffer);
      gtk_sheet_set_column_title(register_sheet, i, buffer);
      gtk_sheet_set_column_width (register_sheet, i, column_width(i));
    }


    // ASCII column
    i = REGISTERS_PER_ROW;
    sprintf(buffer,"ASCII");
    gtk_sheet_column_button_add_label(register_sheet, i, buffer);
    gtk_sheet_set_column_title(register_sheet, i, buffer);

    gtk_sheet_set_column_width (register_sheet, i, column_width(i));

    gtk_sheet_set_row_titles_width(register_sheet, column_width(-1));

  }



}

GUIRegisterList::GUIRegisterList(RegisterMemoryAccess *pRMA) {
  m_pRMA = pRMA;
  unsigned int uAddress;
  unsigned int uRegisterSize;

  uRegisterSize = (pRMA->get_size() < MAX_REGISTERS) ? (pRMA->get_size()) : MAX_REGISTERS;

  for(uAddress=0; uAddress < uRegisterSize; uAddress++) {
    GUIRegister *pReg = new GUIRegister();
    pReg->rma = m_pRMA;
    pReg->address = uAddress;
    pReg->register_size = m_pRMA->get_cpu()->register_size();
    pReg->bIsAliased = (*m_pRMA)[uAddress].address != (unsigned int)uAddress;
    m_paRegisters[uAddress] = pReg;
  }
  for(;uAddress < MAX_REGISTERS; uAddress++)
    m_paRegisters[uAddress] = &THE_invalid_register;
}

GUIRegisterList::~GUIRegisterList() {
  unsigned int nRegs;
  unsigned int uAddress;

  nRegs = (m_pRMA->get_size() < MAX_REGISTERS) ? (m_pRMA->get_size()) : MAX_REGISTERS;
  for(uAddress=0; uAddress < nRegs; uAddress++) {
    delete m_paRegisters[uAddress];
  }
}

//------------------------------------------------------------------------
void Register_Window::NewProcessor(GUI_Processor *_gp)
{
  gint i,j, border_mask, border_width;
  unsigned int reg_number;
  CrossReferenceToGUI *cross_reference;
  gboolean row_created;
  GtkSheetRange range;

  if(!gp || !gp->cpu || !rma || !gp->cpu->isHardwareOnline())
    return;

  if( !enabled)
    return;

  if(!register_sheet){
    printf("Warning %s:%d\n",__FUNCTION__,__LINE__);
    return;
  }

  row_created=FALSE;
  unsigned int nRegs;
  nRegs = (rma->get_size() < MAX_REGISTERS) ? (rma->get_size()) : MAX_REGISTERS;

  if (!nRegs)
    return;

  gtk_sheet_freeze(register_sheet);

  j=0;
  i=0;

  gtk_sheet_REALLY_set_row_height (register_sheet, j, row_height(i));

  SetRegisterSize();


  for(reg_number=0;reg_number<nRegs;reg_number++) {
    i=reg_number%REGISTERS_PER_ROW;

    if(i==0 && row_created) {
      j++;
      row_created=FALSE;
    }

    GUIRegister *pGReg = registers->Get(reg_number);
    pGReg->row = j;
    pGReg->col = i;
    pGReg->put_shadow(RegisterValue(INVALID_VALUE,INVALID_VALUE));
    pGReg->bUpdateFull=true;

    if(pGReg->bIsValid()) {

      gpsim_set_bulk_mode(1);
      pGReg->put_shadow(registers->Get(reg_number)->getRV());
      gpsim_set_bulk_mode(0);

      /* Now create a cross-reference link that the simulator can use to
       * send information back to the gui
       */

      cross_reference = new RegisterWindowXREF();
      cross_reference->parent_window_type =   WT_register_window;
      cross_reference->parent_window = (gpointer) this;
      cross_reference->data = (gpointer) pGReg;
      pGReg->Assign_xref(cross_reference);

      if(!row_created)
      {
        char row_label[100];
        if(register_sheet->maxrow<j)
        {
          gtk_sheet_add_row(register_sheet,1);
          gtk_sheet_REALLY_set_row_height (register_sheet, j, row_height(0));
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

  UpdateStyle();
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

  gtk_sheet_thaw(register_sheet);

  // set values in the sheet
  Update();
  SelectRegister(0);
}

static int show_event(GtkWidget *widget,
                      Register_Window *rw)
{
  Dprintf((" show_event\n"));

  rw->Update();
  return TRUE;
}

static int delete_event(GtkWidget *widget,
                        GdkEvent  *event,
                        Register_Window *rw)
{
  Dprintf((" delete_event\n"));
  rw->ChangeView(VIEW_HIDE);
  return TRUE;
}

//------------------------------------------------------------------------
// Build
//
//

void Register_Window::Build()
{

  if(bIsBuilt)
    return;

  Dprintf((" Register_Window::Build()\n"));
  GtkWidget *main_vbox;
  GtkWidget *scrolled_window;

#define MAXROWS  (MAX_REGISTERS/REGISTERS_PER_ROW)
#define MAXCOLS  (REGISTERS_PER_ROW+1)

  char *fontstring;

  if(window!=0) {
    gtk_widget_destroy(window);
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

    RAM_RegisterWindow *rrw = dynamic_cast<RAM_RegisterWindow *>(this);
    if(rrw && rrw->sbw)
      rrw->sbw->Create(main_vbox);
  }
  else
  {
    register_sheet=GTK_SHEET(gtk_sheet_new(1,MAXCOLS,"gpsim Register Viewer [EEPROM]"));
    gtk_window_set_title(GTK_WINDOW(window), "register viewer [EEPROM]");
  }
  //gtk_sheet_hide_column_titles(register_sheet);
  //gtk_sheet_hide_row_titles(register_sheet);
  /* create popupmenu */
  popup_menu=build_menu(this);

  build_entry_bar(main_vbox,this);

  gtk_window_set_default_size(GTK_WINDOW(window), width,height);
  gtk_widget_set_uposition(GTK_WIDGET(window),x,y);
  gtk_window_set_wmclass(GTK_WINDOW(window),name(),"Gpsim");

  /**************************** load fonts *********************************/
#if GTK_MAJOR_VERSION >= 2
#define DEFAULT_NORMALFONT "Monospace 10"
#else
#define DEFAULT_NORMALFONT "-adobe-courier-*-r-*-*-*-140-*-*-*-*-*-*"
#endif
  strcpy(normalfont_string,DEFAULT_NORMALFONT);
  if(config_get_string(name(),"normalfont",&fontstring))
      strcpy(normalfont_string,fontstring);

  while(!LoadStyles())
  {
    if(gui_question("Some fonts did not load.","Open font dialog","Try defaults")==FALSE)
      {
      strcpy(normalfont_string,DEFAULT_NORMALFONT);
      config_set_string(name(),"normalfont",normalfont_string);
      }
      else
      {
      SettingsDialog();
      }
  }
  UpdateStyle();
  gtk_signal_connect(GTK_OBJECT (window), "delete_event",
                     GTK_SIGNAL_FUNC(delete_event), this);

  gtk_signal_connect(GTK_OBJECT (window), "show",
                  GTK_SIGNAL_FUNC(show_event), this);

  scrolled_window=gtk_scrolled_window_new(0, 0);

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

  gtk_signal_connect(GTK_OBJECT(register_sheet),
                     "key_press_event",
                     (GtkSignalFunc) clipboard_handler,
                     0);

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

  SetRegisterSize();

  gtk_widget_show (window);

  gtk_widget_grab_default(location);

  //  GTKWAIT;

  bIsBuilt = true;

  NewProcessor(gp);

  UpdateMenuItem();
  Dprintf((" regwin is built\n"));
}

//------------------------------------------------------------------------
Register_Window::Register_Window()
{
  register_sheet = NULL;
  printf("WARNING: calling default constructor: %s\n",__FUNCTION__);

}

Register_Window::Register_Window(GUI_Processor *_gp)
{
  int i;

  gp = _gp;
  window = 0;
  wc = WC_data;
  wt = WT_register_window;
  pCellFormat = 0;
  char_width = 0;
  chars_per_column = 3; // assume byte-sized registers
  register_sheet = NULL;

  registers_loaded=0;

  registers = 0;

  for(i=0;i<MAX_REGISTERS/REGISTERS_PER_ROW;i++)
    row_to_address[i]=-1;
}


RAM_RegisterWindow::RAM_RegisterWindow(GUI_Processor *_gp) :
  Register_Window(_gp)
{
  menu = "<main>/Windows/Ram";
  type = REGISTER_RAM;

  set_name("register_viewer_ram");
  // Add a status bar
  sbw = new StatusBar_Window();

  get_config();

  if(enabled)
      Build();
}


void RAM_RegisterWindow::NewProcessor(GUI_Processor *_gp)
{

  if(!_gp || !_gp->cpu)
    return;

  rma = &_gp->cpu->rma;
  registers = _gp->m_pGUIRamRegisters;
  Dprintf((" RAM_RegisterWindow::NewProcessor rma=%p\n",rma));

  Register_Window::NewProcessor(_gp);
  if(sbw)
    sbw->NewProcessor(_gp, rma);

}

void RAM_RegisterWindow::Update()
{
  Register_Window::Update();
  if(sbw)
    sbw->Update();
}

EEPROM_RegisterWindow::EEPROM_RegisterWindow(GUI_Processor *_gp) :
  Register_Window(_gp)
{
  menu = "<main>/Windows/EEPROM";
  type = REGISTER_EEPROM;

  set_name("register_viewer_eeprom");

  get_config();

  if(enabled)
      Build();
}


void EEPROM_RegisterWindow::NewProcessor(GUI_Processor *_gp)
{

  if(!_gp || !_gp->cpu)
    return;

  rma = &_gp->cpu->ema;
  registers = _gp->m_pGUIEEPromRegisters;

  Dprintf((" EEPROM_RegisterWindow::NewProcessor rma=%p\n",rma));

  Register_Window::NewProcessor(_gp);
}

#endif // HAVE_GUI
