/*
   Copyright (C) 2004
   T. Scott Dattalo

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

#include <gtk/gtk.h>

#include <stdio.h>

#include "regwin.h"

//------------------------------------------------------------------------
static gint
RegWindow_ButtonPressEvent(GtkWidget *widget, GdkEventButton *event, RegWindow *rw)
{
  printf("regwindow event\n");
  if(widget && event && rw) {
    return rw->ButtonPressEvent(widget,event);
  }
  return FALSE;
}

static gint
RegCell_ButtonPressEvent(GtkWidget *widget, GdkEventButton *event, RegCell *rc)
{

  if(widget && event && rc) {
    return rc->ButtonPressEvent(widget,event);
  }
  return FALSE;
}


//------------------------------------------------------------------------

RegWindow::RegWindow()
{
  nCols = 18;
  nRows = 5;
}

GtkWidget *RegWindow::Build()
{
  GtkWidget *frame = gtk_frame_new("Table");

  GtkAdjustment *hadj = (GtkAdjustment*)gtk_adjustment_new(0.0,0.0,0.0,0.0,0.0,0.0);
  GtkAdjustment *vadj = (GtkAdjustment*)gtk_adjustment_new(0.0,0.0,0.0,0.0,0.0,0.0);
  GtkWidget *scrolled_window = gtk_scrolled_window_new(hadj,vadj);
  gtk_widget_show(scrolled_window);
  gtk_container_add(GTK_CONTAINER(frame),scrolled_window);
  GtkWidget *table = gtk_table_new (64,18,TRUE);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window),table);

  gtk_table_set_row_spacings (GTK_TABLE(table),0);

  char buf[32];
  int row,col;
  GtkWidget *label;
  for(col=0; col<nCols;col++) {
    snprintf(buf,sizeof(buf),"%02X",col);
    label = gtk_label_new(buf);
    gtk_table_attach_defaults(GTK_TABLE(table),label, col+1, col+2, 0, 1);
  }
  label = gtk_label_new("ASCII");
  gtk_table_attach_defaults(GTK_TABLE(table),label, nCols+1, nCols+5, 0, 1);

  for(row=0; row<nRows;row++) {
    snprintf(buf,sizeof(buf),"%02X",row);
    label = gtk_label_new(buf);
    gtk_table_attach_defaults(GTK_TABLE(table),label, 0, 1, row+1, row+2);
  }

  for(row=0; row<nRows; row++) {
    for(col=0; col<nCols;col++) {

      snprintf(buf,sizeof(buf),"%02X%02X",row,col);

      RegCell *rc = new RegCell(row * nCols + col, buf);

      gtk_table_attach_defaults(GTK_TABLE(table),rc->getWidget(), col+1, col+2, row+1, row+2);

    }
  
    snprintf(buf,sizeof(buf),"%02X23456789ABCDEF",row);

    RegCell *rc = new RegCell(row * nCols + nCols+1, buf, nCols);

    gtk_table_attach_defaults(GTK_TABLE(table),rc->getWidget(), nCols+1, nCols+5, row+1, row+2);
  }
  gtk_signal_connect(GTK_OBJECT(table),
		     "button_press_event",
		     (GtkSignalFunc) RegWindow_ButtonPressEvent, 
		     this);

  gtk_table_resize(GTK_TABLE(table), nRows +1 , nCols+1);
  return frame;

}

gint RegWindow::ButtonPressEvent(GtkWidget *widget, GdkEventButton *event)
{
  if(!event || !widget)
    return FALSE;

  printf("ButtonPressEvent for RegWindow\n");
  if( (event->type == GDK_BUTTON_PRESS) &&  (event->button == 3) ) {
    printf("button 3 pressed\n");
    return TRUE;
  }
  return FALSE;
}

//------------------------------------------------------------------------
// RegCell
RegCell::RegCell(int _address, const char *initialText, int _cell_width)
  : address(_address), cell_width(_cell_width)
{
  entry = gtk_entry_new ();

  PangoContext *pc = gtk_widget_get_pango_context(entry);
  PangoFontDescription *pfd = pango_context_get_font_description(pc);
  pango_font_description_set_family_static(pfd, "Monospace");
  gtk_widget_modify_font(entry, pfd);

  GdkColor color;
  gdk_color_parse("light cyan", &color);
  //gtk_widget_modify_bg(entry, state, &color);
  gtk_widget_modify_base (entry, GTK_STATE_NORMAL, &color);
  //gtk_widget_modify_text(entry,state, &color);

  gtk_entry_set_width_chars(GTK_ENTRY (entry), cell_width);
  gtk_entry_set_text (GTK_ENTRY (entry), initialText);

  gtk_signal_connect(GTK_OBJECT(entry),
		     "button_press_event",
		     (GtkSignalFunc) RegCell_ButtonPressEvent, 
		     this);
}

static void debugFont(PangoContext *pc)
{
  if(!pc)
    return;

  PangoFontDescription *pfd = pango_context_get_font_description(pc);
  G_CONST_RETURN char *fontDesc = pango_font_description_get_family(pfd);
  gint fsize = pango_font_description_get_size(pfd);
  printf("Font: %s size %d\n",fontDesc,fsize);

  char *fullDesc = pango_font_description_to_string(pfd);
  if(fullDesc) {
    printf("full description: %s\n",fullDesc);
    g_free(fullDesc);
  }

  int i,n_families=0;
  PangoFontFamily **families;

  pango_context_list_families(pc, &families, &n_families);

  printf("Number of families for the context:%d\n",n_families);
  for(i=0; i<n_families; i++) {
    fullDesc = (char *)pango_font_family_get_name(families[i]);
    if(fullDesc)
      printf(" %s\n",fullDesc);
  }
  g_free(families);


}

static void debugWidget(GtkWidget *widget)
{
  guint i;
  guint n_prop=0;
  if(!widget)
    return;

  printf("getting widget style properties\n");
  GParamSpec **gparams = gtk_widget_class_list_style_properties( (GtkWidgetClass *)widget, &n_prop);
  if(gparams) {
    printf("There are %d parameters\n",n_prop);

    for(i=0;i<n_prop;i++) {
      if(gparams[i])
	printf(" %s\n",gparams[i]->name);
    }
    g_free(gparams);
  } else
    printf("There are no parameters\n");
}

gint RegCell::ButtonPressEvent(GtkWidget *widget, GdkEventButton *event)
{
  if(!event || !widget)
    return FALSE;

  printf("ButtonPressEvent for RegCell, address = 0x%x\n",address);
  if( (event->type == GDK_BUTTON_PRESS) &&  (event->button == 3) ) {
    printf("button 3 pressed\n");

    //debugFont(gtk_widget_get_pango_context(entry));
    //debugWidget(entry);

    /*
    GdkColormap *cmap = gdk_colormap_get_system();
    GdkColor color;
    gdk_color_parse("blue", &color);
    gboolean bWriteable=FALSE, bBestMatch=FALSE;
    gdk_colormap_alloc_color(cmap,&color, bWriteable, bBestMatch);

    GtkRcStyle *pWidgetRCStyle = gtk_widget_get_modifier_style(GTK_WIDGET(entry));
    GtkRcStyle *styleCopy = gtk_rc_style_copy(pWidgetRCStyle);

    styleCopy->base[GTK_STATE_NORMAL] = color;
    gtk_widget_modify_style(GTK_WIDGET(entry),styleCopy);
    */


    static GtkStateType state = GTK_STATE_NORMAL;
    switch(state) {
    case GTK_STATE_NORMAL:
      state = GTK_STATE_ACTIVE;
      break;
    case GTK_STATE_ACTIVE:
      state = GTK_STATE_PRELIGHT;
      break;
    case GTK_STATE_PRELIGHT:
      state = GTK_STATE_SELECTED;
      break;
    case GTK_STATE_SELECTED:
      state = GTK_STATE_INSENSITIVE;
      break;
    case GTK_STATE_INSENSITIVE:
    default:
      state = GTK_STATE_NORMAL;
      break;
    }

    GdkColor color;
    gdk_color_parse("blue", &color);
    //gtk_widget_modify_bg(entry, state, &color);

    gtk_widget_modify_base (entry, state, &color);

    //gtk_widget_modify_text(entry,state, &color);


    return TRUE;
  }
  return FALSE;
}
