/*
   Copyright (C) 1998,1999,2000,2001
   T. Scott Dattalo and Ralf Forsberg
   Copyright (C) 2011 Roy R Rankin

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
#include "../xpms/break.xpm"
#include "../xpms/pc.xpm"

#include <assert.h>

#define OPCODES_PER_ROW 16

enum {
  ADDRESS_COLUMN,    // Address     - integer
  OPCODE_COLUMN,     // Opcode      - integer
  MNEMONIC_COLUMN,   // Mnemonic    - string
  PC_PIX_COLUMN,     // PC marker   - pixbuf
  BREAK_PIX_COLUMN,  // Break point - pixbuf
  NUM_COLUMNS
};

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
    const menu_id id;
} menu_item;

static const menu_item sheet_menu_items[] = {
    {"Clear breakpoints", MENU_BREAK_CLEAR},
    {"Set break on read", MENU_BREAK_READ},
    {"Set break on write", MENU_BREAK_WRITE},
    {"Set break on execute", MENU_BREAK_EXECUTE},
    {"Add watch", MENU_ADD_WATCH},
    {"Settings...",MENU_SETTINGS}
};

static const menu_item sheet_submenu_items[] = {
    {"One byte per cell",             MENU_ASCII_1BYTE},
    {"Two bytes per cell, MSB first", MENU_ASCII_2BYTEMSB},
    {"Two bytes per cell, LSB first", MENU_ASCII_2BYTELSB},
};

static menu_item const list_menu_items[] = {
    {"Settings...",MENU_SETTINGS}
};

// Used only in popup menus
SourceBrowserOpcode_Window *popup_sbow;

//========================================================================

class SourceOpcodeXREF : public CrossReferenceToGUI
{
public:

  void Update(int new_value)
  {

    SourceBrowserOpcode_Window *sbow;

    sbow = static_cast<SourceBrowserOpcode_Window*>(parent_window);

    sbow->SetPC(new_value);

  }

  void Remove()
  {
  }

};

class Scroll_Info {
public:
  SourceBrowserOpcode_Window *sbow;
  GtkTreeIter iter;
};

//========================================================================

// update ascii column in sheet
void SourceBrowserOpcode_Window::update_ascii(gint row)
{
    gint i;
    gchar name[45];
    unsigned char byte;

    if (row < 0 || row > GTK_SHEET(sheet)->maxrow)
    {
        printf("Warning update_ascii(%x)\n", row);
        return;
    }

    if (row < 0 || row > GTK_SHEET(sheet)->maxrow)
        return;

    switch(ascii_mode)
    {
    case 0:
        for(i=0; i<16; i++)
        {
            byte = memory[row*16 + i]&0xff;

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
                byte = memory[row*16 + i/2]&0xff;
            else
                byte = (memory[row*16 + i/2]&0xff00) >>8;

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
                byte = (memory[row*16 + i/2]&0xff00) >>8;
            else
                byte = memory[row*16 + i/2]&0xff;

            name[i] = byte;

            if( (name[i] < ' ') || (name[i]>'z'))
                name[i] = '.';
        }
        name[i] = 0;
        break;
    }
    gtk_sheet_set_cell(GTK_SHEET(sheet), row,OPCODES_PER_ROW, GTK_JUSTIFY_RIGHT,name);

}

// called when user has selected a menu item
void
SourceBrowserOpcode_Window::popup_activated(GtkWidget *widget, gpointer data)
{
  GtkSheet *sheet;

  int i,j;
  GtkSheetRange range;
  int pm_size;
  gint char_width;

  if(!popup_sbow || !popup_sbow->gp || !popup_sbow->gp->cpu) {
    printf("%s:%d - 0 pointer \n",__FILE__,__LINE__);
    return;
  }
  sheet=GTK_SHEET(popup_sbow->sheet);
  range = sheet->range;

  pm_size = popup_sbow->gp->cpu->program_memory_size();

  PangoFontMetrics *metrics = pango_context_get_metrics(
    gtk_widget_get_pango_context(GTK_WIDGET(sheet)), NULL, NULL);

  char_width = pango_font_metrics_get_approximate_digit_width(metrics);
  pango_font_metrics_unref(metrics);

  switch(GPOINTER_TO_SIZE(data))
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
        popup_sbow->update_ascii(i);
      break;
    case MENU_ASCII_2BYTEMSB:
      popup_sbow->ascii_mode=1;
      config_set_variable(popup_sbow->name(),"ascii_mode",popup_sbow->ascii_mode);
      gtk_sheet_set_column_width (GTK_SHEET(popup_sbow->sheet), 16, 32*char_width + 6);
      for(i=0;i<pm_size/16;i++)
        popup_sbow->update_ascii(i);
      break;
    case MENU_ASCII_2BYTELSB:
      popup_sbow->ascii_mode=2;
      config_set_variable(popup_sbow->name(),"ascii_mode",popup_sbow->ascii_mode);
      gtk_sheet_set_column_width (GTK_SHEET(popup_sbow->sheet), 16, 32*char_width + 6);
      for(i=0;i<pm_size/16;i++)
        popup_sbow->update_ascii(i);
      break;
    case MENU_SETTINGS:
      popup_sbow->settings_dialog();
      break;
    default:
      puts("Unhandled menuitem?");
      break;
    }
}


GtkWidget *
SourceBrowserOpcode_Window::build_menu_for_sheet()
{
    GtkWidget *menu;
    GtkWidget *item;

    GSList *group=0;

    GtkWidget *submenu;

  menu=gtk_menu_new();

  for (size_t i = 0; i < G_N_ELEMENTS(sheet_menu_items); i++){
      item=gtk_menu_item_new_with_label(sheet_menu_items[i].name);

      g_signal_connect(item,"activate",
                         G_CALLBACK (popup_activated),
                         GSIZE_TO_POINTER(sheet_menu_items[i].id));

      if(sheet_menu_items[i].id==MENU_ADD_WATCH)
      {
        gtk_widget_set_sensitive(item, FALSE);
      }
      gtk_widget_show(item);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu),item);
  }

    submenu=gtk_menu_new();
    for (size_t i = 0; i < G_N_ELEMENTS(sheet_submenu_items); i++){
        item=gtk_radio_menu_item_new_with_label(group, sheet_submenu_items[i].name);

        group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(item));
        g_signal_connect (item, "activate",
                           G_CALLBACK (popup_activated),
                           GSIZE_TO_POINTER(sheet_submenu_items[i].id));

        gtk_widget_show(item);

        if (i == ascii_mode)
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);

        gtk_menu_shell_append(GTK_MENU_SHELL(submenu),item);
    }
    item = gtk_menu_item_new_with_label ("ASCII mode");
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
    gtk_widget_show (item);
    gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);


  return menu;
}

GtkWidget *
SourceBrowserOpcode_Window::build_menu_for_list()
{
  GtkWidget *menu = gtk_menu_new();

  for (size_t i = 0; i < G_N_ELEMENTS(list_menu_items); i++){
      GtkWidget *item = gtk_menu_item_new_with_label(list_menu_items[i].name);

      g_signal_connect (item, "activate",
                         G_CALLBACK (popup_activated),
                         GSIZE_TO_POINTER(list_menu_items[i].id));

      if (list_menu_items[i].id == MENU_ADD_WATCH) {
        gtk_widget_set_sensitive(item, FALSE);
      }
      gtk_widget_show(item);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu),item);
  }

  return menu;
}

// button press handler
static gint
button_press(GtkWidget *widget, GdkEventButton *event, SourceBrowserOpcode_Window *sbow)
{
  if ((event->type == GDK_BUTTON_PRESS) && (event->button == 3)) {
    GtkWidget *popup;

    if (GTK_IS_TREE_VIEW(GTK_OBJECT(widget))) {
      popup = sbow->list_popup_menu;
      gtk_menu_popup(GTK_MENU(popup), 0, 0, 0, 0,
        3, event->time);
    } else {
      popup = sbow->sheet_popup_menu;
      gtk_menu_popup(GTK_MENU(popup), 0, 0, 0, 0,
        3, event->time);
    }
  }

  return FALSE;
}

static void
row_selected(GtkTreeView *tree_view,
  GtkTreePath *path, GtkTreeViewColumn *column,
  SourceBrowserOpcode_Window *sbow)
{
  GtkTreeIter iter;

  GtkTreeModel *model = gtk_tree_view_get_model(tree_view);
  if (gtk_tree_model_get_iter(model, &iter, path)) {
    gint break_row;
    gtk_tree_model_get(model, &iter, gint(ADDRESS_COLUMN), &break_row, -1);

    unsigned address = sbow->gp->cpu->map_pm_index2address(break_row);
    sbow->gp->cpu->pma->toggle_break_at_address(address);
  }
}

static void filter(char *clean, const char *dirty, int max)
{
  if (dirty != 0) {
    int i = 0;
    do {
      if (*dirty == '\t') {
        int j = 0;
        for (dirty++; j < 8 && i % 8; j++, i++)
          *clean++ = ' ';
      } else if (*dirty < ' ') {
        dirty++;
      } else {
        *clean++ = *dirty++;
      }
    } while (*dirty && ++i < max);
  }

  *clean = 0;
}

void SourceBrowserOpcode_Window::update_styles(int address)
{
  GtkSheetRange range;

  int index = address;

  if (gp->cpu)
    index = gp->cpu->map_pm_address2index(address);

  int row = index/16;
  int column = index%16;


  range.row0=row;
  range.rowi=row;
  range.col0=column;
  range.coli=column;

  if(!gp->cpu) {
    gtk_sheet_range_set_background(GTK_SHEET(sheet), &range, &normal_pm_bg_color);
    return;
  }

  GtkTreeIter iter;
  GdkPixbuf *pix = NULL;

  gtk_sheet_range_set_font(GTK_SHEET(sheet), &range, normalPFD);
  if (gp->cpu && gp->cpu->pma->address_has_break(address)) {
    pix = break_pix;

    gtk_sheet_range_set_background(GTK_SHEET(sheet), &range, &breakpoint_color);

  } else {
    if(gp->cpu->pma->isModified(address))
      gtk_sheet_range_set_background(GTK_SHEET(sheet), &range, &pm_has_changed_color);
    else
      gtk_sheet_range_set_background(GTK_SHEET(sheet), &range, &normal_pm_bg_color);
  }

  if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(list), &iter, NULL, index)) {
    gtk_list_store_set(list, &iter, gint(BREAK_PIX_COLUMN), pix, -1);
  }
}


static void update_label(SourceBrowserOpcode_Window *sbow, int address)
{
  char labeltext[128];
  char entrytext[128];
  GtkEntry *sheet_entry;

  if(!sbow || !sbow->gp || !sbow->gp->cpu)
    return;

  if(address<0) {
    entrytext[0]=0;
    strcpy(labeltext,"ASCII");
  } else {
    unsigned int oc = sbow->gp->cpu->pma->get_opcode(address);

    filter(labeltext,
           sbow->gp->cpu->pma->get_opcode_name(address,entrytext,sizeof(entrytext)),
           sizeof(labeltext));

    g_snprintf(entrytext, sizeof(entrytext), "0x%04X", oc);
  }

  sheet_entry = GTK_ENTRY(gtk_sheet_get_entry(GTK_SHEET(sbow->sheet)));
  gtk_label_set_text(GTK_LABEL(sbow->label), labeltext);
  gtk_entry_set_max_length(GTK_ENTRY(sbow->entry),
    gtk_entry_buffer_get_max_length(gtk_entry_get_buffer(sheet_entry)));

  gtk_entry_set_text(GTK_ENTRY(sbow->entry), entrytext);

}

void SourceBrowserOpcode_Window::update_values(int address)
{
  if (!gp || !gp->cpu || !memory)
    return;

  unsigned uMemoryIndex = gp->cpu->map_pm_address2index(address);
  int row = uMemoryIndex / 16;
  int column = uMemoryIndex % 16;

  unsigned int oc = gp->cpu->pma->get_opcode(address);

  if (oc != memory[uMemoryIndex]) {

    memory[address] = oc;
    // Put new values, in case they changed
    char oc_buf[128];
    char mn_buf[128];
    char buf[128];

    g_snprintf(oc_buf, sizeof(oc_buf), "%04X", oc);
    filter(mn_buf,
           gp->cpu->pma->get_opcode_name(address, buf, sizeof(buf)),
           sizeof(buf));

    GtkTreeIter iter;

    gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(list),
      &iter, NULL, address);
    gtk_list_store_set(list, &iter,
      OPCODE_COLUMN, oc,
      MNEMONIC_COLUMN, mn_buf,
      -1);

    gtk_sheet_set_cell(GTK_SHEET(sheet),
                       row, column,
                       GTK_JUSTIFY_RIGHT, oc_buf);
  }
}

void SourceBrowserOpcode_Window::update(int address)
{

  if(!gp->cpu)
    return;

  update_values(address);
  update_styles(address);
}

void SourceBrowserOpcode_Window::load_styles()
{
    GdkColor text_fg;
    GdkColor text_bg;
    GdkColormap *colormap = gdk_colormap_get_system();


    if (normal_style == 0)
    {
    	gdk_color_parse("black", &text_fg);
    	gdk_color_parse("light cyan", &text_bg);
    	gdk_colormap_alloc_color(colormap, &text_fg,FALSE,TRUE );
    	gdk_colormap_alloc_color(colormap, &text_bg,FALSE,TRUE );
        normal_style = gtk_style_new ();
        normal_style->fg[GTK_STATE_NORMAL] = text_fg;
        normal_style->base[GTK_STATE_NORMAL] = text_bg;
    }

    if (normalPFD)
	pango_font_description_free(normalPFD);
    normalPFD = 
	    pango_font_description_from_string(normalfont_string.c_str());
    if (normal_style->font_desc)
      pango_font_description_free(normal_style->font_desc);
    normal_style->font_desc = normalPFD;

    if (current_line_number_style == 0)
    {
    	text_bg.red   = 60000;
    	text_bg.green = 60000;
    	text_bg.blue  = 60000;
    	gdk_colormap_alloc_color(colormap, &text_bg,FALSE,TRUE );
    	current_line_number_style = gtk_style_new ();
    	current_line_number_style->fg[GTK_STATE_NORMAL] = text_fg;
    	current_line_number_style->base[GTK_STATE_NORMAL] = text_bg;
    }

    if (current_line_numberPFD)
	pango_font_description_free(current_line_numberPFD);

    current_line_numberPFD = 
	pango_font_description_from_string(pcfont_string.c_str());

    if (current_line_number_style->font_desc)
      pango_font_description_free(current_line_number_style->font_desc);
    current_line_number_style->font_desc = current_line_numberPFD;

    if (breakpoint_line_number_style == 0)
    {
        gdk_color_parse("red", &text_bg);
        breakpoint_color=text_bg;
        gdk_colormap_alloc_color(colormap, &breakpoint_color,FALSE,TRUE );
    	breakpoint_line_number_style = gtk_style_new ();
    	breakpoint_line_number_style->fg[GTK_STATE_NORMAL] = text_fg;
    	breakpoint_line_number_style->base[GTK_STATE_NORMAL] = text_bg;
    }

    if (breakpoint_line_numberPFD)
	pango_font_description_free(breakpoint_line_numberPFD);

    breakpoint_line_numberPFD
      = pango_font_description_from_string(breakpointfont_string.c_str());

    if (breakpoint_line_number_style->font_desc)
      pango_font_description_free(breakpoint_line_number_style->font_desc);
    breakpoint_line_number_style->font_desc = breakpoint_line_numberPFD;

    gdk_color_parse("white",&normal_pm_bg_color);
    gdk_colormap_alloc_color(colormap, &normal_pm_bg_color,FALSE,TRUE);
    gdk_color_parse("light gray",&pm_has_changed_color);
    gdk_colormap_alloc_color(colormap, &pm_has_changed_color,FALSE,TRUE);
}

/********************** Settings dialog ***************************/

void SourceBrowserOpcode_Window::settings_dialog()
{
  GtkWidget *dialog = gtk_dialog_new_with_buttons(
    "Opcode browser settings",
    GTK_WINDOW(window),
    GTK_DIALOG_DESTROY_WITH_PARENT,
    "_Cancel", GTK_RESPONSE_CANCEL,
    "_OK", GTK_RESPONSE_OK,
    NULL
    );

  GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
  GtkWidget *table = gtk_table_new(3, 2, FALSE);
  gtk_table_set_row_spacings(GTK_TABLE(table), 6);
  gtk_table_set_col_spacings(GTK_TABLE(table), 6);
  gtk_container_add(GTK_CONTAINER(content_area), table);
  gtk_container_set_border_width(GTK_CONTAINER(table), 18);

  GtkWidget *label;
  GtkWidget *normal_font_btn, *break_font_btn, *pc_font_btn;

  // Normal font
  label = gtk_label_new("Normal font");
  normal_font_btn = gtk_font_button_new_with_font(normalfont_string.c_str());
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
  gtk_table_attach_defaults(GTK_TABLE(table), normal_font_btn, 1, 2, 0, 1);

  // Breakpoint font
  label = gtk_label_new("Breakpoint font");
  break_font_btn = gtk_font_button_new_with_font(breakpointfont_string.c_str());
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);
  gtk_table_attach_defaults(GTK_TABLE(table), break_font_btn, 1, 2, 1, 2);

  // PC font
  label = gtk_label_new("PC font");
  pc_font_btn = gtk_font_button_new_with_font(pcfont_string.c_str());
  gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);
  gtk_table_attach_defaults(GTK_TABLE(table), pc_font_btn, 1, 2, 2, 3);

  int fonts_ok;

  gtk_widget_show_all(dialog);

  do {
    fonts_ok = 0;
    int retval = gtk_dialog_run(GTK_DIALOG(dialog));
    if (retval != GTK_RESPONSE_OK)
      break;

    const char *fontname;
    PangoFontDescription *font;

    fontname = gtk_font_button_get_font_name(GTK_FONT_BUTTON(normal_font_btn));
    font = pango_font_description_from_string(fontname);
    if (font) {
      normalfont_string = fontname;
      config_set_string(name(), "normalfont", normalfont_string.c_str());
      pango_font_description_free(font);
      ++fonts_ok;
    }

    fontname = gtk_font_button_get_font_name(GTK_FONT_BUTTON(break_font_btn));
    font = pango_font_description_from_string(fontname);
    if (font) {
      normalfont_string = fontname;
      config_set_string(name(), "breakpointfont", breakpointfont_string.c_str());
      pango_font_description_free(font);
      ++fonts_ok;
    }

    fontname = gtk_font_button_get_font_name(GTK_FONT_BUTTON(pc_font_btn));
    font = pango_font_description_from_string(fontname);
    if (font) {
      normalfont_string = fontname;
      config_set_string(name(), "pcfont", pcfont_string.c_str());
      pango_font_description_free(font);
      ++fonts_ok;
    }

    if (fonts_ok) {
      load_styles();
      Fill();
    }
  } while (!fonts_ok);

  gtk_widget_destroy(dialog);
}


static unsigned long get_number_in_string(const char *number_string)
{
  char *bad_position;
  int current_base = 16;

  errno = 0;

  unsigned long retval = strtoul(number_string, &bad_position, current_base);

  if (*bad_position)
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

  sheet=GTK_SHEET(widget);

  if(row>sheet->maxrow || row<0 ||
     col>sheet->maxcol || col<0)
  {
      printf("Warning parse_numbers(%p,%x,%x,%p)\n",widget,row,col,sbow);
      return;
  }

  if(sbow->memory==0)
      return;

  if(col < OPCODES_PER_ROW)
    {

      int reg = row*16+col;
      unsigned int n=0;

      text = gtk_entry_get_text(GTK_ENTRY(sheet->sheet_entry));

      errno = 0;
      if (*text)
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
          printf("Writing new value, new %u, last %u\n", n, sbow->memory[reg]);
          sbow->memory[reg]=n;
          sbow->gp->cpu->pma->put_opcode(reg, n);
          sbow->update_ascii(row);
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

 //RRRint row,col;

 if(widget==0|| sbow==0)
  {
      printf("Warning show_sheet_entry(%p,%p)\n",widget,sbow);
      return;
  }

 if(!gtk_widget_has_focus(widget)) return;

 sheet=GTK_SHEET(sbow->sheet);
 sheet_entry = GTK_ENTRY(gtk_sheet_get_entry(sheet));

 //RRRrow=sheet->active_cell.row; col=sheet->active_cell.col;

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

  update_label(sbow,sbow->gp->cpu->map_pm_index2address(row*16+col));
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
 // gint row, col;

  if(widget==0|| sbow==0)
  {
      printf("Warning show_entry(%p,%p)\n",widget,sbow);
      return;
  }

 if(!gtk_widget_has_focus(widget)) return;

 sheet=GTK_SHEET(sbow->sheet);
 sheet_entry = gtk_sheet_get_entry(sheet);

 //row=sheet->active_cell.row; col=sheet->active_cell.col;

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

    if (!sbow || !sbow->gp || ! sbow->gp->cpu)
	return 0;

    sheet=GTK_SHEET(sbow->sheet);

    if(widget==0 || row>sheet->maxrow || row<0||
       column>sheet->maxcol || column<0)
    {
        printf("Warning activate_sheet_cell(%p,%x,%x)\n",widget,row,column);
        return 0;
    }

    if(column<16)
    {
        update_label(sbow,sbow->gp->cpu->map_pm_index2address(row*16+column));
    }
    else
        update_label(sbow,-1);

    gtk_sheet_get_attributes(sheet,sheet->active_cell.row,
                             sheet->active_cell.col, &attributes);
    gtk_editable_set_editable(GTK_EDITABLE(sbow->entry), attributes.is_editable);
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

  GtkTreeIter iter;
  GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
  gtk_tree_selection_unselect_all(sel);
  gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(list), &iter, NULL, row);
  gtk_tree_selection_select_iter(sel, &iter);

  GtkTreePath *path
    = gtk_tree_model_get_path(GTK_TREE_MODEL(list), &iter);

  gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tree),
    path, NULL, TRUE, 0.5, 0.0);
  gtk_tree_path_free(path);
}

void SourceBrowserOpcode_Window::UpdateLine(int address)
{
  if (!enabled)
    return;

  if (address >= 0)
    update(address);
}

void SourceBrowserOpcode_Window::SetPC(int address)
{
  gint last_address = current_address;

  if(!enabled)
    return;

  current_address = address;

  if(address != last_address) {
      UpdateLine(last_address);

    int index = last_address;
    if (gp->cpu)
      index = gp->cpu->map_pm_address2index(last_address);

    GtkTreeIter iter;
    if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(list), &iter, NULL, index)) {
      gtk_list_store_set(list, &iter, gint(PC_PIX_COLUMN), NULL, -1);
    }
  }

  UpdateLine(address);

  int index = address;
  if(gp->cpu)
    index = gp->cpu->map_pm_address2index(address);

  GtkTreeIter iter;
  if (gtk_tree_model_iter_nth_child(GTK_TREE_MODEL(list), &iter, NULL, index)) {
    gtk_list_store_set(list, &iter, gint(PC_PIX_COLUMN), pc_pix, -1);

    GtkTreePath *path
      = gtk_tree_model_get_path(GTK_TREE_MODEL(list), &iter);

    gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tree),
      path, NULL, TRUE, 0.5, 0.0);
    gtk_tree_path_free(path);
  }

  GtkSheetRange range;

  range.row0 = range.rowi = index / 16;
  range.col0 = range.coli = index % 16;

  gtk_sheet_range_set_background(GTK_SHEET(sheet), &range, 
    &(current_line_number_style->base[GTK_STATE_NORMAL]));
}

//========================================================================
// Fill()
//
// copy the processor's program memory contents to the both the disassembly
// and opcode windows.

void SourceBrowserOpcode_Window::Fill()
{
  GtkSheetRange range;

  if(!bIsBuilt)
    Build();

  if(!gp || !gp->cpu)
    return;

  char buf[128];
  gint i;
  int pm_size;
  int pc;
  gint column_width,char_width;

  // Clearing list
  gtk_list_store_clear(list);

  pm_size = gp->cpu->program_memory_size();

  delete[] memory;
  memory = new unsigned int[pm_size];

  gchar name_buf[10];
  gtk_sheet_freeze(GTK_SHEET(sheet));

    PangoRectangle rect;
    PangoLayout *layout;

    layout = gtk_widget_create_pango_layout (GTK_WIDGET(sheet), "A");
    pango_layout_set_font_description (layout, normalPFD);

    pango_layout_get_extents (layout, NULL, &rect);

  char_width = PANGO_PIXELS(rect.width);
  column_width = 4 * char_width + 5;
  g_object_unref(layout);

  for(i=0; i<GTK_SHEET(sheet)->maxcol; i++){

    g_snprintf(name_buf, sizeof(name_buf), "%02x", gp->cpu->map_pm_index2address(i));
    gtk_sheet_column_button_add_label(GTK_SHEET(sheet), i, name_buf);
    gtk_sheet_set_column_title(GTK_SHEET(sheet), i, name_buf);
    gtk_sheet_set_column_width (GTK_SHEET(sheet), i, column_width);
  }

  range.row0=0;range.col0=0;
  range.rowi=GTK_SHEET(sheet)->maxrow;
  range.coli=GTK_SHEET(sheet)->maxcol;
  gtk_sheet_range_set_font(GTK_SHEET(sheet), &range, normalPFD);
  for(i=0; i < pm_size; i++) {
    int address = gp->cpu->map_pm_index2address(i);
    unsigned int opcode = gp->cpu->pma->get_opcode(address);
    memory[i]=opcode;

    char oc_buf[128];
    char mn_buf[128];
    g_snprintf(oc_buf, sizeof(oc_buf), "%04X", opcode);
    filter(mn_buf,
           gp->cpu->pma->get_opcode_name(address, buf, sizeof(buf)),
           128);
 
    if(GTK_SHEET(sheet)->maxrow < i/16)
    {
        int j = i/16;
        gtk_sheet_add_row(GTK_SHEET(sheet),1);

        g_snprintf(name_buf, sizeof(name_buf), "%04x", gp->cpu->map_pm_index2address(i));
        gtk_sheet_row_button_add_label(GTK_SHEET(sheet), j, name_buf);
        gtk_sheet_set_row_title(GTK_SHEET(sheet), j, name_buf);
    }

    gtk_sheet_set_cell(GTK_SHEET(sheet),
                       i/16,
                       i%16,
                       GTK_JUSTIFY_RIGHT, oc_buf);

    GtkTreeIter iter;
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter,
      gint(ADDRESS_COLUMN), address,
      gint(OPCODE_COLUMN), opcode,
      gint(MNEMONIC_COLUMN), mn_buf,
      -1);

    update_styles(address);
  }

  for(i=0;i<pm_size/16;i++)
    update_ascii(i);

  gtk_sheet_thaw(GTK_SHEET(sheet));

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


  range.row0=0;range.col0=0;
  range.rowi=GTK_SHEET(sheet)->maxrow;
  range.coli=GTK_SHEET(sheet)->maxcol;
  gtk_sheet_range_set_background(GTK_SHEET(sheet), &range, &normal_pm_bg_color);

  // --tsd - commented this out when the new gtkextra package was released.
  //gtk_sheet_range_set_font(GTK_SHEET(sheet), &range, normal_style->font_desc);

  range.row0=range.rowi=0;
  range.col0=range.coli=0;
  gtk_sheet_select_range(GTK_SHEET(sheet),&range);

  update_label(this,0);

}

void
SourceBrowserOpcode_Window::cell_renderer(GtkTreeViewColumn *tree_column,
  GtkCellRenderer *cell, GtkTreeModel *tree_model,
  GtkTreeIter *iter, gpointer data)
{
  gint addr;
  gchar buf[64];

  gtk_tree_model_get(tree_model, iter,
    GPOINTER_TO_INT(g_object_get_data(G_OBJECT(cell), "col")), &addr, -1);

  g_snprintf(buf, sizeof(buf), "0x%04x", addr);

  g_object_set(cell, "text", buf, NULL);
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

  GtkStyle *style;
  char *fontstring;

  if(window!=0)
    gtk_widget_destroy(window);

  SourceBrowser_Window::Create();


  gtk_window_set_title (GTK_WINDOW (window), "Program memory");

  notebook = gtk_notebook_new();
  gtk_widget_show(notebook);

  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);

  gtk_window_set_default_size(GTK_WINDOW(window), width,height);
  gtk_window_move(GTK_WINDOW(window), x, y);


  /**************************** load fonts *********************************/
  style  = gtk_style_new();

  normalfont_string = pango_font_description_to_string(style->font_desc);
  breakpointfont_string = normalfont_string;
  pcfont_string = normalfont_string;

  if(config_get_string(name(),"normalfont",&fontstring))
      normalfont_string = fontstring;

  if(config_get_string(name(),"breakpointfont",&fontstring))
    breakpointfont_string = fontstring;

  if(config_get_string(name(),"pcfont",&fontstring))
      pcfont_string = fontstring;

  load_styles();

  //
  // create list
  //
  scrolled_win = gtk_scrolled_window_new (0, 0);
  gtk_widget_show(scrolled_win);
  gtk_container_set_border_width (GTK_CONTAINER (scrolled_win), 6);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);

  list = gtk_list_store_new(NUM_COLUMNS,
    G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING, GDK_TYPE_PIXBUF, GDK_TYPE_PIXBUF);

  tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list));

  GtkTreeSelection *sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));

  gtk_tree_selection_set_mode(sel, GTK_SELECTION_MULTIPLE);

  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  column = gtk_tree_view_column_new();
  renderer = gtk_cell_renderer_pixbuf_new();
  gtk_tree_view_column_pack_start(column, renderer, FALSE);
  gtk_tree_view_column_set_attributes(column, renderer,
    "pixbuf", BREAK_PIX_COLUMN, NULL);

  renderer = gtk_cell_renderer_pixbuf_new();
  gtk_tree_view_column_pack_start(column, renderer, FALSE);
  gtk_tree_view_column_set_attributes(column, renderer,
    "pixbuf", PC_PIX_COLUMN, NULL);

  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("address",
    renderer, "text", ADDRESS_COLUMN, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  g_object_set_data(G_OBJECT(renderer),
    "col", GINT_TO_POINTER(gint(ADDRESS_COLUMN)));

  gtk_tree_view_column_set_cell_data_func(column, renderer,
    cell_renderer, NULL, NULL);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("opcode",
    renderer, "text", OPCODE_COLUMN, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
  g_object_set_data(G_OBJECT(renderer),
    "col", GINT_TO_POINTER(gint(OPCODE_COLUMN)));

  gtk_tree_view_column_set_cell_data_func(column, renderer,
    cell_renderer, NULL, NULL);

  renderer = gtk_cell_renderer_text_new();
  column = gtk_tree_view_column_new_with_attributes("instruction",
    renderer, "text", MNEMONIC_COLUMN, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

  gtk_widget_set_style(tree, normal_style);
  gtk_widget_show(tree);

  gtk_container_add (GTK_CONTAINER (scrolled_win), tree);

  /* Add a signal handler for button press events. This will capture
   * commands for setting and/or clearing break points
   */
  g_signal_connect (tree, "button_press_event",
                     G_CALLBACK (button_press),
                     (gpointer) this);

  g_signal_connect(tree, "row-activated",
    G_CALLBACK(row_selected), gpointer(this));

  gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
                           scrolled_win,
                           gtk_label_new("Assembly"));


  //
  // create sheet
  //
  vbox=gtk_vbox_new(FALSE,1);
  gtk_widget_show(vbox);

  // Create entry bar
  hbox=gtk_hbox_new(FALSE,1);
  gtk_widget_show(hbox);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

  label = gtk_label_new(NULL);

  gtk_widget_modify_font(label, normalPFD);

  gtk_widget_size_request(label, &request);
  gtk_widget_set_size_request(label, 160, request.height);
  gtk_widget_show(label);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);

  entry = gtk_entry_new();

  gtk_widget_modify_font(entry, normalPFD);
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

    PangoRectangle rect;
    PangoLayout *layout;

    layout = gtk_widget_create_pango_layout (GTK_WIDGET(sheet), "A");
    pango_layout_set_font_description (layout, normalPFD);

    pango_layout_get_extents (layout, NULL, &rect);

  char_width = PANGO_PIXELS(rect.width);
  column_width = 4 * char_width + 5;
  g_object_unref(layout);


  for(i=0; i<GTK_SHEET(sheet)->maxcol; i++){

    g_snprintf(_name, sizeof(_name), "%02x", i);
    gtk_sheet_column_button_add_label(GTK_SHEET(sheet), i, _name);
    gtk_sheet_set_column_title(GTK_SHEET(sheet), i, _name);
    gtk_sheet_set_column_width (GTK_SHEET(sheet), i, column_width);
  }
  sprintf(_name,"ASCII");
  gtk_sheet_column_button_add_label(GTK_SHEET(sheet), i, _name);
  gtk_sheet_set_column_title(GTK_SHEET(sheet), i, _name);
  gtk_sheet_set_row_titles_width(GTK_SHEET(sheet), column_width);


  g_signal_connect(sheet,
                     "button_press_event",
                     G_CALLBACK (button_press),
                     this);

  g_signal_connect(gtk_sheet_get_entry(GTK_SHEET(sheet)),
                     "changed", G_CALLBACK (show_entry), this);

  g_signal_connect(sheet,
                     "activate", G_CALLBACK (activate_sheet_cell),
                     (gpointer) this);

  g_signal_connect(entry,
                     "changed", G_CALLBACK (show_sheet_entry), this);

  g_signal_connect(entry,
                     "activate", G_CALLBACK (activate_sheet_entry),
                     this);
  g_signal_connect(sheet,
                     "set_cell",
                     G_CALLBACK (parse_numbers),
                     this);
  /////////////////////////////////////////////////////////////////




  gtk_widget_show(scrolled_win);
  gtk_widget_show(sheet);

  g_signal_connect_after(window, "configure_event",
                           G_CALLBACK (gui_object_configure_event), this);

  gtk_widget_show(window);

  bIsBuilt = true;

  NewProcessor(gp);
  NewSource(gp);

  // create popupmenu for sheet
  sheet_popup_menu = build_menu_for_sheet();

  // create popupmenu for clist
  list_popup_menu = build_menu_for_list();

  UpdateMenuItem();
}

const char *SourceBrowserOpcode_Window::name()
{
  return "program_memory";
}

SourceBrowserOpcode_Window::SourceBrowserOpcode_Window(GUI_Processor *_gp)
  : current_address(0), normalPFD(0), current_line_numberPFD(0),
    breakpoint_line_numberPFD(0), normal_style(0), current_line_number_style(0),
    breakpoint_line_number_style(0), memory(0)
{
  popup_sbow = this;

  menu = "/menu/Windows/Program memory";

  pma =0;
  status_bar = 0;

  gp = _gp;
  wc = WC_source;
  wt = WT_opcode_source_window;

  break_pix = gdk_pixbuf_new_from_xpm_data(break_xpm);
  pc_pix = gdk_pixbuf_new_from_xpm_data(pc_xpm);

  int tmp=0;
  config_get_variable(name(),"ascii_mode",&tmp);
  ascii_mode = tmp;

  get_config();

  if(enabled)
    Build();
}

#endif // HAVE_GUI
