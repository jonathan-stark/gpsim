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

#include <gtkextra/gtkbordercombo.h>
#include <gtkextra/gtkcolorcombo.h>
#include <gtkextra/gtksheet.h>

extern int gui_question(const char *question, const char *a, const char *b);

#include "gui.h"
#include "gui_src.h"

#include <assert.h>

#define PROGRAM_MEMORY_WINDOW_COLUMNS 4   //yuk
#define DEFAULT_ROWS  256
#define OPCODES_PER_ROW 16

#define PROFILE_COLUMN  0
#define ADDRESS_COLUMN  1
#define OPCODE_COLUMN   2
#define MNEMONIC_COLUMN 3

typedef enum {
    MENU_BREAK_CLEAR,
    MENU_BREAK_READ,
    MENU_BREAK_WRITE,
    MENU_BREAK_EXECUTE,
    MENU_ADD_WATCH,
    MENU_ASCII_1BYTE,
    MENU_ASCII_2BYTELSB,
    MENU_ASCII_2BYTEMSB,
    MENU_SETTINGS,
} menu_id;




typedef struct _menu_item {
    const char *name;
    menu_id id;
} menu_item;

static menu_item sheet_menu_items[] = {
    {"Clear breakpoints", MENU_BREAK_CLEAR},
    {"Set break on read", MENU_BREAK_READ},
    {"Set break on write", MENU_BREAK_WRITE},
    {"Set break on execute", MENU_BREAK_EXECUTE},
    {"Add watch", MENU_ADD_WATCH},
    {"Settings...",MENU_SETTINGS}
};

static menu_item sheet_submenu_items[] = {
    {"One byte per cell",             MENU_ASCII_1BYTE},
    {"Two bytes per cell, MSB first", MENU_ASCII_2BYTEMSB},
    {"Two bytes per cell, LSB first", MENU_ASCII_2BYTELSB},
};

static menu_item clist_menu_items[] = {
    {"Settings...",MENU_SETTINGS}
};

// Used only in popup menus
SourceBrowserOpcode_Window *popup_sbow;

static char profile_buffer[128];
static char address_buffer[128];
static char opcode_buffer[128];
static char mnemonic_buffer[128];
static char *row_text[PROGRAM_MEMORY_WINDOW_COLUMNS]={
    profile_buffer,address_buffer,opcode_buffer,mnemonic_buffer
};

static int dlg_x=200, dlg_y=200;

static int settings_dialog(SourceBrowserOpcode_Window *sbow);
extern int font_dialog_browse(GtkWidget *w, gpointer user_data);


//========================================================================

class SourceOpcodeXREF : public CrossReferenceToGUI
{
public:

  void Update(int new_value)
  {

    SourceBrowserOpcode_Window *sbow;

    sbow = (SourceBrowserOpcode_Window*)(parent_window);

    sbow->SetPC(new_value);

  }

  void Remove()
  {
  }

};

//========================================================================

// update ascii column in sheet
static void update_ascii( SourceBrowserOpcode_Window *sbow, gint row)
{
    gint i;
    gchar name[45];
    unsigned char byte;

    if(sbow == 0 || row<0 || row > GTK_SHEET(sbow->sheet)->maxrow)
    {
        printf("Warning update_ascii(%p,%x)\n",sbow,row);
        return;
    }

    if(row<0 || row>GTK_SHEET(sbow->sheet)->maxrow)
        return;

    switch(sbow->ascii_mode)
    {
    case 0:
        for(i=0; i<16; i++)
        {
            byte = sbow->memory[row*16 + i]&0xff;

            name[i] = byte;

            if( (name[i] < ' ') || (name[i]>'z'))
                name[i] = '.';
        }
        name[i] = 0;
        break;
    case 1: // two bytes, MSB first
        for(i=0; i<32; i++)
        {
            if(i%2)
                byte = sbow->memory[row*16 + i/2]&0xff;
            else
                byte = (sbow->memory[row*16 + i/2]&0xff00) >>8;

            name[i] = byte;

            if( (name[i] < ' ') || (name[i]>'z'))
                name[i] = '.';
        }
        name[i] = 0;
        break;
    case 2: // two bytes, LSB first
        for(i=0; i<32; i++)
        {

            if(i%2)
                byte = (sbow->memory[row*16 + i/2]&0xff00) >>8;
            else
                byte = sbow->memory[row*16 + i/2]&0xff;

            name[i] = byte;

            if( (name[i] < ' ') || (name[i]>'z'))
                name[i] = '.';
        }
        name[i] = 0;
        break;
    }
    gtk_sheet_set_cell(GTK_SHEET(sbow->sheet), row,OPCODES_PER_ROW, GTK_JUSTIFY_RIGHT,name);

}

// called when user has selected a menu item
static void
popup_activated(GtkWidget *widget, gpointer data)
{
  GtkSheet *sheet;

  menu_item *item;
  int i,j;
  GtkSheetRange range;
  int pm_size;
  gint char_width;

  if(!widget || !data)
    return;

  if(!popup_sbow || !popup_sbow->gp || !popup_sbow->gp->cpu) {
    printf("%s:%d - 0 pointer \n",__FILE__,__LINE__);
    return;
  }
  item = (menu_item *)data;
  sheet=GTK_SHEET(popup_sbow->sheet);
  range = sheet->range;

  pm_size = popup_sbow->gp->cpu->program_memory_size();
#if GTK_MAJOR_VERSION >= 2
  char_width = gdk_string_width(gtk_style_get_font(popup_sbow->normal_style), "9");
#else
  char_width = gdk_string_width (popup_sbow->normal_style->font,"9");
#endif

  switch(item->id)
    {
    case MENU_BREAK_WRITE:
    case MENU_BREAK_READ:
      printf("This function is not implemented\n");

      for(j=range.row0;j<=range.rowi;j++)
        for(i=range.col0;i<=range.coli;i++) {
          unsigned address = popup_sbow->gp->cpu->map_pm_index2address(j*16+i);
          popup_sbow->gp->cpu->pma->toggle_break_at_address(address);
        }
      break;

    case MENU_BREAK_EXECUTE:

      for(j=range.row0;j<=range.rowi;j++)
        for(i=range.col0;i<=range.coli;i++) {
          unsigned address = popup_sbow->gp->cpu->map_pm_index2address(j*16+i);
          popup_sbow->gp->cpu->pma->set_break_at_address(address);
        }
      break;
    case MENU_BREAK_CLEAR:
      for(j=range.row0;j<=range.rowi;j++)
        for(i=range.col0;i<=range.coli;i++) {
          unsigned address = popup_sbow->gp->cpu->map_pm_index2address(j*16+i);
          popup_sbow->gp->cpu->pma->set_break_at_address(address);
        }
      break;
    case MENU_ADD_WATCH:
      puts("not implemented");
      /*
      for(j=range.row0;j<=range.rowi;j++)
        for(i=range.col0;i<=range.coli;i++) {
          unsigned address = popup_sbow->gp->cpu->map_pm_index2address(j*16+i);
          WatchWindow_add(popup_sbow->gui_obj.gp->watch_window,pic_id, popup_sbow->type, address);
        }
      */
      break;
    case MENU_ASCII_1BYTE:
      popup_sbow->ascii_mode=0;
      config_set_variable(popup_sbow->name(),"ascii_mode",popup_sbow->ascii_mode);
      gtk_sheet_set_column_width (GTK_SHEET(popup_sbow->sheet), 16, 16*char_width + 6);
      for(i=0;i<pm_size/16;i++)
        update_ascii(popup_sbow,i);
      break;
    case MENU_ASCII_2BYTEMSB:
      popup_sbow->ascii_mode=1;
      config_set_variable(popup_sbow->name(),"ascii_mode",popup_sbow->ascii_mode);
      gtk_sheet_set_column_width (GTK_SHEET(popup_sbow->sheet), 16, 32*char_width + 6);
      for(i=0;i<pm_size/16;i++)
        update_ascii(popup_sbow,i);
      break;
    case MENU_ASCII_2BYTELSB:
      popup_sbow->ascii_mode=2;
      config_set_variable(popup_sbow->name(),"ascii_mode",popup_sbow->ascii_mode);
      gtk_sheet_set_column_width (GTK_SHEET(popup_sbow->sheet), 16, 32*char_width + 6);
      for(i=0;i<pm_size/16;i++)
        update_ascii(popup_sbow,i);
      break;
    case MENU_SETTINGS:
      settings_dialog(popup_sbow);
      break;
    default:
      puts("Unhandled menuitem?");
      break;
    }
}


static GtkWidget *
build_menu_for_sheet(SourceBrowserOpcode_Window *sbow)
{
    GtkWidget *menu;
    GtkWidget *item;

    GSList *group=0;

    GtkWidget *submenu;

  unsigned int i;

  if(sbow==0)
  {
      printf("Warning build_menu_for_sheet(%p)\n",sbow);
      return 0;
  }

  popup_sbow=sbow;

  menu=gtk_menu_new();

  item = gtk_tearoff_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), item);
  gtk_widget_show (item);


  for (i=0; i < (sizeof(sheet_menu_items)/sizeof(sheet_menu_items[0])) ; i++){
      item=gtk_menu_item_new_with_label(sheet_menu_items[i].name);

      gtk_signal_connect(GTK_OBJECT(item),"activate",
                         (GtkSignalFunc) popup_activated,
                         &sheet_menu_items[i]);
      GTK_WIDGET_SET_FLAGS (item, GTK_SENSITIVE | GTK_CAN_FOCUS);

      if(sheet_menu_items[i].id==MENU_ADD_WATCH)
      {
          GTK_WIDGET_UNSET_FLAGS (item,
                                  GTK_SENSITIVE | GTK_CAN_FOCUS);
      }
      gtk_widget_show(item);
      gtk_menu_append(GTK_MENU(menu),item);
  }

    submenu=gtk_menu_new();
    for (i=0; i < (sizeof(sheet_submenu_items)/sizeof(sheet_submenu_items[0])) ; i++){
        item=gtk_radio_menu_item_new_with_label(group, sheet_submenu_items[i].name);

        group=gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(item));
        gtk_signal_connect(GTK_OBJECT(item),"activate",
                           (GtkSignalFunc) popup_activated,
                           &sheet_submenu_items[i]);

        GTK_WIDGET_SET_FLAGS (item, GTK_SENSITIVE | GTK_CAN_FOCUS);

        gtk_widget_show(item);

        if(i==sbow->ascii_mode)
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item),1);

        gtk_menu_append(GTK_MENU(submenu),item);
    }
    item = gtk_menu_item_new_with_label ("ASCII mode");
    gtk_menu_append (GTK_MENU (menu), item);
    gtk_widget_show (item);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);


  return menu;
}

static GtkWidget *
build_menu_for_clist(SourceBrowserOpcode_Window *sbow)
{
    GtkWidget *menu;
    GtkWidget *item;

  unsigned int i;

  if(sbow==0)
  {
      printf("Warning build_menu_for_sheet(%p)\n",sbow);
      return 0;
  }

  popup_sbow=sbow;

  menu=gtk_menu_new();

  item = gtk_tearoff_menu_item_new ();
  gtk_menu_append (GTK_MENU (menu), item);
  gtk_widget_show (item);


  for (i=0; i < (sizeof(clist_menu_items)/sizeof(clist_menu_items[0])) ; i++){
      item=gtk_menu_item_new_with_label(clist_menu_items[i].name);

      gtk_signal_connect(GTK_OBJECT(item),"activate",
                         (GtkSignalFunc) popup_activated,
                         &clist_menu_items[i]);
      GTK_WIDGET_SET_FLAGS (item, GTK_SENSITIVE | GTK_CAN_FOCUS);

      if(clist_menu_items[i].id==MENU_ADD_WATCH)
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
button_press(GtkWidget *widget, GdkEventButton *event, SourceBrowserOpcode_Window *sbow)
{
    GtkWidget *popup;
    int break_row;

    if(!sbow || !sbow->gp || !sbow->gp->cpu)
      return 0;

    if(widget==0 || event==0)
    {
        printf("Warning button_press(%p,%p,%p)\n",widget,event,sbow);
        return 0;
    }

    if( (event->type == GDK_BUTTON_PRESS) &&  (event->button == 3) )
    {
        popup_sbow = sbow;

        if(GTK_IS_CLIST(GTK_OBJECT(widget)))
        {
            popup=sbow->clist_popup_menu;
            gtk_menu_popup(GTK_MENU(popup), 0, 0, 0, 0,
                           3, event->time);
        }
        else
        {
            popup=sbow->sheet_popup_menu;
            gtk_menu_popup(GTK_MENU(popup), 0, 0, 0, 0,
                           3, event->time);
        }
    }

    if ((event->type == GDK_2BUTTON_PRESS) &&
        (event->button == 1))

    {
        if(GTK_IS_CLIST(GTK_OBJECT(widget)))
        {
            break_row =  GTK_CLIST (sbow->clist)->focus_row;

            unsigned address = sbow->gp->cpu->map_pm_index2address(break_row);
            sbow->gp->cpu->pma->toggle_break_at_address(address);
            return TRUE;
        }
    }
    return FALSE;
}


static void filter(char *clean, char *dirty, int max)
{

  int i=0,j;

  if(dirty!=0) {

    do {


      if(*dirty == '\t')
        for(j=0,dirty++; j<8 && i%8; j++,i++)
          *clean++ = ' ';
      else if (*dirty <' ')
        dirty++;
      else
        *clean++ = *dirty++;



    } while(*dirty && ++i < max);
  }

  *clean = 0;

}

static void update_styles(SourceBrowserOpcode_Window *sbow, int address)
{
  GtkSheetRange range;

  int index = address;

  if(sbow->gp->cpu)
    index = sbow->gp->cpu->map_pm_address2index(address);

  int row=index/16;
  int column=index%16;


  range.row0=row;
  range.rowi=row;
  range.col0=column;
  range.coli=column;

  if(!sbow->gp->cpu) {
    gtk_sheet_range_set_background(GTK_SHEET(sbow->sheet), &range, &sbow->normal_pm_bg_color);
    return;
  }

  if(sbow->gp->cpu && sbow->gp->cpu->pma->address_has_break(address)) {

    gtk_clist_set_row_style (GTK_CLIST (sbow->clist), index, sbow->breakpoint_line_number_style);
    gtk_sheet_range_set_background(GTK_SHEET(sbow->sheet), &range, &sbow->breakpoint_color);

  } else {

    gtk_clist_set_row_style (GTK_CLIST (sbow->clist), index, sbow->normal_style);

    if(sbow->gp->cpu->pma->isModified(address))
      gtk_sheet_range_set_background(GTK_SHEET(sbow->sheet), &range, &sbow->pm_has_changed_color);
    else
      gtk_sheet_range_set_background(GTK_SHEET(sbow->sheet), &range, &sbow->normal_pm_bg_color);
  }

}


static void update_label(SourceBrowserOpcode_Window *sbow, int address)
{
  char labeltext[128];
  char entrytext[128];
  GtkEntry *sheet_entry;
  unsigned int oc;

  if(!sbow || !sbow->gp || !sbow->gp->cpu)
    return;

  if(address<0) {
    entrytext[0]=0;
    strcpy(labeltext,"ASCII");
  } else {

    oc = sbow->gp->cpu->pma->get_opcode(address);

    filter(labeltext,
           sbow->gp->cpu->pma->get_opcode_name(address,entrytext,sizeof(entrytext)),
           sizeof(labeltext));

    sprintf(entrytext, "0x%04X", oc);
  }

  sheet_entry = GTK_ENTRY(gtk_sheet_get_entry(GTK_SHEET(sbow->sheet)));
  gtk_label_set(GTK_LABEL(sbow->label), labeltext);
  gtk_entry_set_max_length(GTK_ENTRY(sbow->entry),
                           GTK_ENTRY(sheet_entry)->text_max_length);
  gtk_entry_set_text(GTK_ENTRY(sbow->entry), entrytext);

}

static void update_values(SourceBrowserOpcode_Window *sbow, int address)
{
  if(!sbow || !sbow->gp || !sbow->gp->cpu)
    return;

  unsigned uMemoryIndex = sbow->gp->cpu->map_pm_address2index(address);
  int row=uMemoryIndex/16;
  int column=uMemoryIndex%16;
  char buf[128];
  unsigned int oc;

  oc = sbow->gp->cpu->pma->get_opcode(address);
  if(oc != sbow->memory[uMemoryIndex]) {

    sbow->memory[address]=oc;
    // Put new values, in case they changed
    sprintf (row_text[ADDRESS_COLUMN], "0x%04X", address);
    sprintf(row_text[OPCODE_COLUMN], "0x%04X", oc);
    filter(row_text[MNEMONIC_COLUMN],
           sbow->gp->cpu->pma->get_opcode_name(address,buf,sizeof(buf)),
           sizeof(buf));

    gtk_clist_set_text (GTK_CLIST (sbow->clist), address, OPCODE_COLUMN, row_text[OPCODE_COLUMN]);
    gtk_clist_set_text (GTK_CLIST (sbow->clist), address, MNEMONIC_COLUMN, row_text[MNEMONIC_COLUMN]);

    gtk_sheet_set_cell(GTK_SHEET(sbow->sheet),
                       row,column,
                       GTK_JUSTIFY_RIGHT,row_text[OPCODE_COLUMN]+2);
  }
}

static void update(SourceBrowserOpcode_Window *sbow, int address)
{

  if(!sbow->gp->cpu)
    return;

  update_values(sbow,address);
  update_styles(sbow,address);
}

static gint configure_event(GtkWidget *widget, GdkEventConfigure *e, gpointer data)
{
    if(widget->window==0)
        return 0;

    gdk_window_get_root_origin(widget->window,&dlg_x,&dlg_y);
    return 0; // what should be returned?, FIXME
}

static int load_styles(SourceBrowserOpcode_Window *sbow)
{
    GdkColor text_fg;
    GdkColor text_bg;
    GdkColormap *colormap = gdk_colormap_get_system();

    gdk_color_parse("black", &text_fg);
    gdk_color_parse("light cyan", &text_bg);
    gdk_colormap_alloc_color(colormap, &text_fg,FALSE,TRUE );
    gdk_colormap_alloc_color(colormap, &text_bg,FALSE,TRUE );

    sbow->normal_style = gtk_style_new ();
    sbow->normal_style->fg[GTK_STATE_NORMAL] = text_fg;
    sbow->normal_style->base[GTK_STATE_NORMAL] = text_bg;
#if GTK_MAJOR_VERSION >= 2
    gtk_style_set_font(sbow->normal_style,
      gdk_fontset_load(sbow->normalfont_string));
#else
    gdk_font_unref (sbow->normal_style->font);
    sbow->normal_style->font =
        gdk_fontset_load (sbow->normalfont_string);
#endif

    text_bg.red   = 30000;
    text_bg.green = 30000;
    text_bg.blue  = 30000;
    gdk_colormap_alloc_color(colormap, &text_bg,FALSE,TRUE );
    sbow->current_line_number_style = gtk_style_new ();
    sbow->current_line_number_style->fg[GTK_STATE_NORMAL] = text_fg;
    sbow->current_line_number_style->base[GTK_STATE_NORMAL] = text_bg;
#if GTK_MAJOR_VERSION >= 2
    gtk_style_set_font(sbow->current_line_number_style,
      gdk_fontset_load(sbow->pcfont_string));
#else
    gdk_font_unref (sbow->current_line_number_style->font);
    sbow->current_line_number_style->font =
        gdk_fontset_load (sbow->pcfont_string);
#endif

    gdk_color_parse("red", &text_bg);
    sbow->breakpoint_color=text_bg;
    gdk_colormap_alloc_color(colormap, &sbow->breakpoint_color,FALSE,TRUE );
    sbow->breakpoint_line_number_style = gtk_style_new ();
    sbow->breakpoint_line_number_style->fg[GTK_STATE_NORMAL] = text_fg;
    sbow->breakpoint_line_number_style->base[GTK_STATE_NORMAL] = text_bg;
#if GTK_MAJOR_VERSION >= 2
    gtk_style_set_font(sbow->breakpoint_line_number_style,
      gdk_fontset_load(sbow->breakpointfont_string));
#else
    gdk_font_unref (sbow->breakpoint_line_number_style->font);
    sbow->breakpoint_line_number_style->font =
        gdk_fontset_load (sbow->breakpointfont_string);
#endif

    gdk_color_parse("white",&sbow->normal_pm_bg_color);
    gdk_colormap_alloc_color(colormap, &sbow->normal_pm_bg_color,FALSE,TRUE);
    gdk_color_parse("light gray",&sbow->pm_has_changed_color);
    gdk_colormap_alloc_color(colormap, &sbow->pm_has_changed_color,FALSE,TRUE);

#if GTK_MAJOR_VERSION >= 2
    if (gtk_style_get_font(sbow->breakpoint_line_number_style) == 0 ||
      gtk_style_get_font(sbow->current_line_number_style) == 0 ||
      gtk_style_get_font(sbow->normal_style) == 0)
      return 0;
#else
    if(sbow->breakpoint_line_number_style->font==0)
        return 0;
    if(sbow->current_line_number_style->font==0)
        return 0;
    if(sbow->normal_style->font==0)
        return 0;
#endif

    return 1;
}

/********************** Settings dialog ***************************/
static int settings_active;
static void settingsok_cb(GtkWidget *w, gpointer user_data)
{
    if(settings_active)
    {
        settings_active=0;
    }
}
static int settings_dialog(SourceBrowserOpcode_Window *sbow)
{
    static GtkWidget *dialog=0;
    GtkWidget *button;
    static int retval;
    GtkWidget *hbox;
    static GtkWidget *normalfontstringentry;
    static GtkWidget *breakpointfontstringentry;
    static GtkWidget *pcfontstringentry;
    GtkWidget *label;
    int fonts_ok=0;

    if(dialog==0)
    {
        dialog = gtk_dialog_new();
        gtk_window_set_title (GTK_WINDOW (dialog), "Opcode browser settings");
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


        // Breakpoint font
        hbox = gtk_hbox_new(0,0);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox,FALSE,FALSE,20);
        gtk_widget_show(hbox);
        label=gtk_label_new("Breakpoint font:");
        gtk_box_pack_start(GTK_BOX(hbox), label,
                           FALSE,FALSE, 20);
        gtk_widget_show(label);
        breakpointfontstringentry=gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), breakpointfontstringentry,
                           TRUE, TRUE, 0);
        gtk_widget_show(breakpointfontstringentry);
        button = gtk_button_new_with_label("Browse...");
        gtk_widget_show(button);
        gtk_box_pack_start(GTK_BOX(hbox), button,
                           FALSE,FALSE,10);
        gtk_signal_connect(GTK_OBJECT(button),"clicked",
                           GTK_SIGNAL_FUNC(font_dialog_browse),(gpointer)breakpointfontstringentry);


        // PC font
        hbox = gtk_hbox_new(0,0);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox,FALSE,FALSE,20);
        gtk_widget_show(hbox);
        label=gtk_label_new("PC font:");
        gtk_box_pack_start(GTK_BOX(hbox), label,
                           FALSE,FALSE, 20);
        gtk_widget_show(label);
        pcfontstringentry=gtk_entry_new();
        gtk_box_pack_start(GTK_BOX(hbox), pcfontstringentry,
                           TRUE, TRUE, 0);
        gtk_widget_show(pcfontstringentry);
        button = gtk_button_new_with_label("Browse...");
        gtk_widget_show(button);
        gtk_box_pack_start(GTK_BOX(hbox), button,
                           FALSE,FALSE,10);
        gtk_signal_connect(GTK_OBJECT(button),"clicked",
                           GTK_SIGNAL_FUNC(font_dialog_browse),(gpointer)pcfontstringentry);


        // OK button
        button = gtk_button_new_with_label("OK");
        gtk_widget_show(button);
        gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button,
                           FALSE,FALSE,10);
        gtk_signal_connect(GTK_OBJECT(button),"clicked",
                           GTK_SIGNAL_FUNC(settingsok_cb),(gpointer)dialog);
    }

    gtk_entry_set_text(GTK_ENTRY(normalfontstringentry), sbow->normalfont_string);
    gtk_entry_set_text(GTK_ENTRY(breakpointfontstringentry), sbow->breakpointfont_string);
    gtk_entry_set_text(GTK_ENTRY(pcfontstringentry), sbow->pcfont_string);

    gtk_widget_set_uposition(GTK_WIDGET(dialog),dlg_x,dlg_y);
    gtk_widget_show_now(dialog);



    while(fonts_ok!=3)
    {
        char fontname[256];
#if GTK_MAJOR_VERSION >= 2
        PangoFontDescription *font;
#else
        GdkFont *font;
#endif

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
            if(gui_question("Normalfont did not load!","Try again","Ignore/Cancel")==FALSE)
                break;
        }
        else
        {
#if GTK_MAJOR_VERSION >= 2
#else
            gdk_font_unref(font);
#endif
            strcpy(sbow->normalfont_string,gtk_entry_get_text(GTK_ENTRY(normalfontstringentry)));
            config_set_string(sbow->name(),"normalfont",sbow->normalfont_string);
            fonts_ok++;
        }

        strcpy(fontname,gtk_entry_get_text(GTK_ENTRY(breakpointfontstringentry)));
#if GTK_MAJOR_VERSION >= 2
        if((font=pango_font_description_from_string(fontname))==0)
#else
        if((font=gdk_fontset_load(fontname))==0)
#endif
        {
            if(gui_question("Breakpointfont did not load!","Try again","Ignore/Cancel")==FALSE)
                break;
        }
        else
        {
#if GTK_MAJOR_VERSION >= 2
#else
            gdk_font_unref(font);
#endif
            strcpy(sbow->breakpointfont_string,gtk_entry_get_text(GTK_ENTRY(breakpointfontstringentry)));
            config_set_string(sbow->name(),"breakpointfont",sbow->breakpointfont_string);
            fonts_ok++;
        }

        strcpy(fontname,gtk_entry_get_text(GTK_ENTRY(pcfontstringentry)));
#if GTK_MAJOR_VERSION >= 2
        if((font=pango_font_description_from_string(fontname))==0)
#else
        if((font=gdk_fontset_load(fontname))==0)
#endif
        {
            if(gui_question("PCfont did not load!","Try again","Ignore/Cancel")==FALSE)
                break;
        }
        else
        {
#if GTK_MAJOR_VERSION >= 2
#else
            gdk_font_unref(font);
#endif
            strcpy(sbow->pcfont_string,gtk_entry_get_text(GTK_ENTRY(pcfontstringentry)));
            config_set_string(sbow->name(),"pcfont",sbow->pcfont_string);
            fonts_ok++;
        }
    }


    sbow->Build();

    gtk_widget_hide(dialog);

    return retval;
}


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

  /*
  if(retval > 255)
    errno = ERANGE;
  */

  return(retval);
}


// when a new cell is selected, we write changes in
// previously selected cell to gpsim
static void
parse_numbers(GtkWidget *widget, int row, int col, SourceBrowserOpcode_Window *sbow)
{

  if(!sbow || !sbow->gp || !sbow->gp->cpu || !widget)
    return;

  GtkSheet *sheet;
  const gchar *text;
  int justification;

  sheet=GTK_SHEET(widget);

  if(row>sheet->maxrow || row<0 ||
     col>sheet->maxcol || col<0)
  {
      printf("Warning parse_numbers(%p,%x,%x,%p)\n",widget,row,col,sbow);
      return;
  }

  if(sbow->memory==0)
      return;


  justification=GTK_JUSTIFY_RIGHT;

  if(col < OPCODES_PER_ROW)
    {

      int reg = row*16+col;
      unsigned int n=0;

      text = gtk_entry_get_text(GTK_ENTRY(sheet->sheet_entry));

      errno = 0;
      if(strlen(text)>0)
        n = get_number_in_string(text);
      else
        errno = ERANGE;

      if(errno != 0)
      {
          n = sbow->gp->cpu->pma->get_opcode(reg);
          sbow->memory[reg] = INVALID_VALUE;
      }

      if(n != sbow->memory[reg])
      {
          printf("Writing new value, new %d, last %d\n",n,sbow->memory[reg]);
          sbow->memory[reg]=n;
          sbow->gp->cpu->pma->put_opcode(reg, n);
          update_ascii(sbow,row);
      }
    }
  else
      ; // ignore user changes in ascii column for right now
}

/* when the entry above the sheet is changed (typed a digit), we
   copy it to the cell entry */
static void
show_sheet_entry(GtkWidget *widget, SourceBrowserOpcode_Window *sbow)
{
 const char *text;
 GtkSheet *sheet;
 GtkEntry *sheet_entry;

 int row,col;

 if(widget==0|| sbow==0)
  {
      printf("Warning show_sheet_entry(%p,%p)\n",widget,sbow);
      return;
  }

 if(!GTK_WIDGET_HAS_FOCUS(widget)) return;

 sheet=GTK_SHEET(sbow->sheet);
 sheet_entry = GTK_ENTRY(gtk_sheet_get_entry(sheet));

 row=sheet->active_cell.row; col=sheet->active_cell.col;

 if((text=gtk_entry_get_text (GTK_ENTRY(sbow->entry))))
     gtk_entry_set_text(sheet_entry, text);

}

/* when we have new data in the entry above the sheet, we
 copy the data to the cells/registers

 this doesn't get called when it is clicked
 in, only when we hit return
 */
static void
activate_sheet_entry(GtkWidget *widget, SourceBrowserOpcode_Window *sbow)
{
  GtkSheet *sheet;
  gint row, col;

  if(widget==0|| sbow==0)
  {
      printf("Warning activate_sheet_entry(%p,%p)\n",widget,sbow);
      return;
  }

  sheet=GTK_SHEET(sbow->sheet);
  row=sheet->active_cell.row;
  col=sheet->active_cell.col;

  parse_numbers(GTK_WIDGET(sheet),sheet->active_cell.row,sheet->active_cell.col,sbow);

  update_label(sbow,row*16+col);
}

/*
 we get here when the entry in a cell is changed (typed a digit), we
 copy it to the entry above the sheet.
 */
static void
show_entry(GtkWidget *widget, SourceBrowserOpcode_Window *sbow)
{
 const char *text;
 GtkSheet *sheet;
 GtkWidget * sheet_entry;
  gint row, col;

  if(widget==0|| sbow==0)
  {
      printf("Warning show_entry(%p,%p)\n",widget,sbow);
      return;
  }

 if(!GTK_WIDGET_HAS_FOCUS(widget)) return;

 sheet=GTK_SHEET(sbow->sheet);
 sheet_entry = gtk_sheet_get_entry(sheet);

 row=sheet->active_cell.row; col=sheet->active_cell.col;

 if((text=gtk_entry_get_text (GTK_ENTRY(sheet_entry))))
     gtk_entry_set_text(GTK_ENTRY(sbow->entry), text);
}

/* when a cell is activated, we set the label and entry above the sheet
 */
static gint
activate_sheet_cell(GtkWidget *widget, gint row, gint column, SourceBrowserOpcode_Window *sbow)
{
    GtkSheet *sheet;
    GtkSheetCellAttr attributes;

    sheet=GTK_SHEET(sbow->sheet);

    if(widget==0 || row>sheet->maxrow || row<0||
       column>sheet->maxcol || column<0 || sbow==0)
    {
        printf("Warning activate_sheet_cell(%p,%x,%x,%p)\n",widget,row,column,sbow);
        return 0;
    }

    if(column<16)
        update_label(sbow,row*16+column);
    else
        update_label(sbow,-1);

    gtk_sheet_get_attributes(sheet,sheet->active_cell.row,
                             sheet->active_cell.col, &attributes);
    gtk_entry_set_editable(GTK_ENTRY(sbow->entry), attributes.is_editable);
    gtk_sheet_range_set_justification(sheet, &sheet->range, GTK_JUSTIFY_RIGHT);


    return TRUE;
}


void SourceBrowserOpcode_Window::SelectAddress(int address)
{
  if(!enabled)
    return;
  unsigned int row = address;
  if(gp->cpu)
    row = gp->cpu->map_pm_address2index(address);

  gtk_clist_unselect_all(GTK_CLIST(clist));
  gtk_clist_select_row(GTK_CLIST(clist),row,0);
  if(GTK_VISIBILITY_FULL != gtk_clist_row_is_visible (GTK_CLIST (clist),row))
    gtk_clist_moveto (GTK_CLIST (clist), row, 0, .5, 0.0);

}

void SourceBrowserOpcode_Window::UpdateLine(int address)
{


  if(!enabled)
    return;

  if(address >= 0)
    update(this,address);
}

void SourceBrowserOpcode_Window::SetPC(int address)
{
  gint last_address;

  if(!enabled)
    return;

  last_address = current_address;
  current_address = address;

  if(address != last_address)
    {
      UpdateLine(last_address);
      gtk_clist_set_row_style (GTK_CLIST (clist),
                               gp->cpu->map_pm_address2index(last_address),
                               normal_style);

      UpdateLine(address);
      gtk_clist_set_row_style (GTK_CLIST (clist),
                               gp->cpu->map_pm_address2index(address),
                               current_line_number_style);
    }

  unsigned int current_row = gp->cpu->map_pm_address2index(current_address);
  if(GTK_VISIBILITY_FULL != gtk_clist_row_is_visible (GTK_CLIST (clist),
                                                      current_row))
    {
      gtk_clist_moveto (GTK_CLIST (clist),
                        current_row,
                        0, .5, 0.0);
    }
}

//========================================================================
// Fill()
//
// copy the processor's program memory contents to the both the disassembly
// and opcode windows.

void SourceBrowserOpcode_Window::Fill()
{
  if(!bIsBuilt)
    Build();

  if(!gp || !gp->cpu)
    return;

  char buf[128];
  unsigned int opcode;
  gint i;
  int pm_size;
  int pc;

  // Clearing and appending is faster than changing
  gtk_clist_clear(GTK_CLIST(clist));

  pm_size = gp->cpu->program_memory_size();

  if(memory!=0)
    free(memory);
  memory=(unsigned int*)malloc(pm_size*sizeof(*memory));


  for(i=0; i < pm_size; i++) {
    int address = gp->cpu->map_pm_index2address(i);
    opcode = gp->cpu->pma->get_opcode(address);
    memory[i]=opcode;
    sprintf (row_text[ADDRESS_COLUMN], "0x%04X", address);
    sprintf(row_text[OPCODE_COLUMN], "0x%04X", opcode);
    filter(row_text[MNEMONIC_COLUMN],
           gp->cpu->pma->get_opcode_name(address,buf,sizeof(buf)),
           128);

    if(GTK_SHEET(sheet)->maxrow<i/16)
      gtk_sheet_add_row(GTK_SHEET(sheet),1);

    gtk_sheet_set_cell(GTK_SHEET(sheet),
                       i/16,
                       i%16,
                       GTK_JUSTIFY_RIGHT,row_text[OPCODE_COLUMN]+2);

    gtk_clist_append (GTK_CLIST (clist), row_text);
    update_styles(this,i);
  }

  for(i=0;i<pm_size/16;i++)
    update_ascii(this,i);

  gtk_clist_set_row_style (GTK_CLIST (clist), 0, current_line_number_style);

  pc=gp->cpu->pma->get_PC();
  SetPC(pc);
  update_label(this,pc);


}
void SourceBrowserOpcode_Window::NewSource(GUI_Processor *_gp)
{
  if(!gp)
    return;

  current_address=0;

  if(!enabled)
    return;
  if(!bIsBuilt)
    Build();

  assert(wt==WT_opcode_source_window);

  /* Now create a cross-reference link that the
   * simulator can use to send information back to the gui
   */
  if(gp->cpu && gp->cpu->pc) {
    SourceOpcodeXREF *cross_reference;

    cross_reference = new SourceOpcodeXREF();
    cross_reference->parent_window_type =   WT_status_bar;
    cross_reference->parent_window = (gpointer) this;
    cross_reference->data = (gpointer) this;

    gp->cpu->pc->add_xref((gpointer) cross_reference);

  }

  Fill();

}

void SourceBrowserOpcode_Window::NewProcessor(GUI_Processor *_gp)
{

  GtkSheetRange range;

  if(!gp || !gp->cpu)
    return;

  current_address=0;

  if(!enabled)
    return;

  if(!bIsBuilt)
    Build();

  assert(wt==WT_opcode_source_window);

  pma = gp->cpu->pma;

  Fill();

  range.row0=0;range.col0=0;
  range.rowi=GTK_SHEET(sheet)->maxrow;
  range.coli=GTK_SHEET(sheet)->maxcol;
  gtk_sheet_range_set_background(GTK_SHEET(sheet), &range, &normal_pm_bg_color);
#if GTK_MAJOR_VERSION >= 2
  // --tsd - commented this out when the new gtkextra package was released.
  //gtk_sheet_range_set_font(GTK_SHEET(sheet), &range, normal_style->font_desc);
#else
  gtk_sheet_range_set_font(GTK_SHEET(sheet), &range, normal_style->font);
#endif


  range.row0=range.rowi=0;
  range.col0=range.coli=0;
  gtk_sheet_select_range(GTK_SHEET(sheet),&range);

  update_label(this,0);

}

void SourceBrowserOpcode_Window::Build(void)
{
  if(bIsBuilt)
    return;

  GtkWidget *hbox;
  GtkWidget *scrolled_win;
  GtkRequisition request;

  gchar _name[10];
  gint column_width,char_width;
  gint i;

  static GtkStyle *style=0;
  char *fontstring;

  if(window!=0)
    gtk_widget_destroy(window);

  SourceBrowser_Window::Create();


  gtk_window_set_title (GTK_WINDOW (window), "Program memory");

  notebook = gtk_notebook_new();
  gtk_widget_show(notebook);

  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);

  gtk_window_set_default_size(GTK_WINDOW(window), width,height);
  gtk_widget_set_uposition(GTK_WIDGET(window),x,y);


  /**************************** load fonts *********************************/
#if GTK_MAJOR_VERSION >= 2
#define DEFAULT_NORMALFONT "Courier Roman 14"
#define DEFAULT_BREAKPOINTFONT "Courier Bold 14"
#define DEFAULT_PCFONT "Courier Bold 14"
#else
#define DEFAULT_NORMALFONT "-adobe-courier-*-r-*-*-*-140-*-*-*-*-*-*"
#define DEFAULT_BREAKPOINTFONT "-adobe-courier-bold-r-*-*-*-140-*-*-*-*-*-*"
#define DEFAULT_PCFONT "-adobe-courier-bold-r-*-*-*-140-*-*-*-*-*-*"
#endif

  strcpy(normalfont_string,DEFAULT_NORMALFONT);
  if(config_get_string(name(),"normalfont",&fontstring))
      strcpy(normalfont_string,fontstring);

  strcpy(breakpointfont_string,DEFAULT_BREAKPOINTFONT);
  if(config_get_string(name(),"breakpointfont",&fontstring))
    strcpy(breakpointfont_string,fontstring);

  strcpy(pcfont_string,DEFAULT_PCFONT);
  if(config_get_string(name(),"pcfont",&fontstring))
      strcpy(pcfont_string,fontstring);

  while(!load_styles(this))
  {
      if(gui_question("Some fonts did not load.","Open font dialog","Try defaults")==FALSE)
      {
          strcpy(normalfont_string,DEFAULT_NORMALFONT);
          strcpy(breakpointfont_string,DEFAULT_BREAKPOINTFONT);
          strcpy(pcfont_string,DEFAULT_PCFONT);
          config_set_string(name(),"normalfont",normalfont_string);
          config_set_string(name(),"breakpointfont",breakpointfont_string);
          config_set_string(name(),"pcfont",pcfont_string);
      }
      else
      {
          settings_dialog(this);
      }
  }


  /////////////////////////////////////////////////////////////////
  // create clist
  /////////////////////////////////////////////////////////////////
  scrolled_win = gtk_scrolled_window_new (0, 0);
  gtk_widget_show(scrolled_win);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_win), 5);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);

  /* create GtkCList here so we have a pointer to throw at the
   * button callbacks -- more is done with it later */
  clist = gtk_clist_new_with_titles (columns, column_titles);
  gtk_widget_show(clist);

  gtk_container_add (GTK_CONTAINER (scrolled_win), clist);

  /* Add a signal handler for button press events. This will capture
   * commands for setting and/or clearing break points
   */
  gtk_signal_connect(GTK_OBJECT(clist),"button_press_event",
                     (GtkSignalFunc) button_press,
                     (gpointer) this);


  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           scrolled_win,
                           gtk_label_new("Assembly"));

  gtk_clist_set_row_height (GTK_CLIST (clist), 18);
  gtk_widget_set_usize (clist, 300, 100);
  gtk_clist_set_selection_mode (GTK_CLIST (clist), GTK_SELECTION_EXTENDED);

  for (i = 0; i < columns; i++)
    {
      gtk_clist_set_column_width (GTK_CLIST (clist), i, 80);

      gtk_clist_set_column_auto_resize (GTK_CLIST (clist), i, FALSE);
      gtk_clist_set_column_justification (GTK_CLIST (clist), i,
                                          GTK_JUSTIFY_LEFT);
      // %%% FIX ME
      //   Hide the profile column for now...
      if(i == 0)
        gtk_clist_set_column_visibility (GTK_CLIST (clist), i, FALSE);
    }


  for (i = 0; i < DEFAULT_ROWS; i++)
    {
      sprintf (row_text[ADDRESS_COLUMN], "0x%04X", i);
      gtk_clist_append (GTK_CLIST (clist), row_text);
      gtk_clist_set_row_style (GTK_CLIST (clist), i, normal_style);

    }



  /////////////////////////////////////////////////////////////////
  // create sheet
  /////////////////////////////////////////////////////////////////
  vbox=gtk_vbox_new(FALSE,1);
  gtk_widget_show(vbox);

  // Create entry bar
  hbox=gtk_hbox_new(FALSE,1);
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

  label=gtk_label_new("");
  style=gtk_style_new();
#if GTK_MAJOR_VERSION >= 2
  gtk_style_set_font(style, gtk_style_get_font(normal_style));
#else
  style->font=normal_style->font;
#endif
  gtk_widget_set_style(label,style);
  gtk_widget_size_request(label, &request);
  gtk_widget_set_usize(label, 160, request.height);
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

  entry=gtk_entry_new();
  style=gtk_style_new();
#if GTK_MAJOR_VERSION >= 2
  gtk_style_set_font(style, gtk_style_get_font(normal_style));
#else
  style->font=normal_style->font;
#endif
  gtk_widget_set_style(entry,style);
  gtk_widget_show(entry);
  gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, TRUE, 0);

  // Create sheet
  scrolled_win=gtk_scrolled_window_new(0, 0);
  gtk_widget_show(scrolled_win);
  gtk_box_pack_start(GTK_BOX(vbox), scrolled_win, TRUE, TRUE, 0);

  sheet=gtk_sheet_new(1,17,"where does this string go?");
  gtk_widget_show(sheet);
  gtk_container_add(GTK_CONTAINER(scrolled_win), sheet);

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           vbox,
                           gtk_label_new("Opcodes"));

#if GTK_MAJOR_VERSION >= 2
  char_width = gdk_string_width(gtk_style_get_font(normal_style), "9");
#else
  char_width = gdk_string_width (normal_style->font,"9");
#endif
  column_width = 5 * char_width + 6;

  for(i=0; i<GTK_SHEET(sheet)->maxcol; i++){

    sprintf(_name,"%02x",i);
    gtk_sheet_column_button_add_label(GTK_SHEET(sheet), i, _name);
    gtk_sheet_set_column_title(GTK_SHEET(sheet), i, _name);
    gtk_sheet_set_column_width (GTK_SHEET(sheet), i, column_width);
  }
  sprintf(_name,"ASCII");
  gtk_sheet_column_button_add_label(GTK_SHEET(sheet), i, _name);
  gtk_sheet_set_column_title(GTK_SHEET(sheet), i, _name);
  gtk_sheet_set_row_titles_width(GTK_SHEET(sheet), column_width);


  gtk_signal_connect(GTK_OBJECT(sheet),
                     "button_press_event",
                     (GtkSignalFunc) button_press,
                     this);

  gtk_signal_connect(GTK_OBJECT(gtk_sheet_get_entry(GTK_SHEET(sheet))),
                     "changed", (GtkSignalFunc)show_entry, this);

  gtk_signal_connect(GTK_OBJECT(sheet),
                     "activate", (GtkSignalFunc)activate_sheet_cell,
                     (gpointer) this);

  gtk_signal_connect(GTK_OBJECT(entry),
                     "changed", (GtkSignalFunc)show_sheet_entry, this);

  gtk_signal_connect(GTK_OBJECT(entry),
                     "activate", (GtkSignalFunc)activate_sheet_entry,
                     this);
  gtk_signal_connect(GTK_OBJECT(sheet),
                     "set_cell",
                     (GtkSignalFunc) parse_numbers,
                     this);
  /////////////////////////////////////////////////////////////////




  gtk_widget_show(scrolled_win);
  gtk_widget_show(sheet);

  gtk_signal_connect_after(GTK_OBJECT(window), "configure_event",
                           GTK_SIGNAL_FUNC(gui_object_configure_event),this);

  gtk_widget_show(window);

  bIsBuilt = true;

  //GTKWAIT;

  NewProcessor(gp);
  NewSource(gp);

  /* create popupmenu for sheet */
  sheet_popup_menu=build_menu_for_sheet(this);

  /* create popupmenu for clist */
  clist_popup_menu=build_menu_for_clist(this);

  UpdateMenuItem();
}

SourceBrowserOpcode_Window::SourceBrowserOpcode_Window(GUI_Processor *_gp)
{
  static gchar *titles[] =
    {
      "profile", "address", "opcode", "instruction"
    };

  menu = "<main>/Windows/Program memory";

  window = 0;
  pma =0;
  status_bar = 0;

  column_titles = titles;
  columns = 4;


  gp = _gp;
  set_name("program_memory");
  wc = WC_source;
  wt = WT_opcode_source_window;

  memory=0;
  current_address=0;

  ascii_mode=1; /// default, two bytes/cell, MSB first
  int tmp=0;
  config_get_variable(name(),"ascii_mode",&tmp);
  ascii_mode = tmp;

  get_config();

  if(enabled)
    Build();

}

#endif // HAVE_GUI
