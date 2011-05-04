/* lcd.c
   Copyright (C) 2000 Scott Dattalo

This is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

This is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with lcd.c; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include "config.h"
#ifdef HAVE_GUI

#include <gtk/gtk.h>

#include "lcd.h"
#include "lcdfont.h"
#include "hd44780.h"

/****************************************************************
 * CreateXPMdataFromLCDdata -
 *
 * It'd probably be easier to just create pixmaps straight from
 * xpm maps. However, that would confine the lcd characters to
 * just one size. This routine will take an lcd character (that
 * is defined somewhat like a bitmap) and create a pixmap. The
 * size of the pixmap is determined by the size that display
 * requires for each cell. (For example, on displays with a 5x7
 * cell, the lcd display will map several crt pixels into one
 * lcd pixel.)
 */

gchar  **CreateXPMdataFromLCDdata(LcdDisplay *lcdP, _5X7 *ch )
{

  guint i,j,k,rows,cols,m,ii,jj,colors;
  guint bc,pc;
  char buffer[256];
  gchar **xpm_template;

  // total rows in the xpm
  rows = 9 + lcdP->dots.y * lcdP->pixels.y;
  cols = 1 + lcdP->dots.x * lcdP->pixels.x;

  xpm_template = (char **)malloc(rows * sizeof(gchar *));

  colors = 3;
  sprintf(buffer, "%d %d %d 1", cols, 1+lcdP->dots.y * lcdP->pixels.y,
          colors);
  xpm_template[0] = (gchar *)strdup(buffer);
  xpm_template[1] = (gchar *)strdup("  c None");
  xpm_template[2] = (gchar *)strdup("B c #113311");
  xpm_template[3] = (gchar *)strdup("G c #668866");
  //  xpm_template[4] = (gchar *)strdup("Q c #789878");

  for(i=4; i<rows; i++) {
    xpm_template[i] = (gchar *)malloc(cols+1);
    memset(xpm_template[i], ' ', cols);
    xpm_template[i][cols] = 0;
    //xpm_template[i] = (gchar *)strdup("                ");
  }

  for(j=0; j<(guint)lcdP->dots.y; j++) {

    k = 5 + lcdP->pixels.y*j;


    for(i=0; i<(guint)lcdP->dots.x; i++) {
      pc = (ch[0][j][i] == '.') ? 'B' : ' ';
      bc = (ch[0][j][i] == '.') ? 'G' : ' ';

      //m = i*lcdP->pixels.y;
      m = i*lcdP->pixels.x;

      for(jj=k; jj<k+lcdP->pixels.y-1; jj++) {

        xpm_template[jj][m] = bc;
        for(ii=m+1; ii<m+lcdP->pixels.x; ii++)
          xpm_template[jj][ii] = pc;
      }
      for(ii=m; ii<m+lcdP->pixels.x; ii++)
        xpm_template[k+lcdP->pixels.y-1][ii] = bc;
    }

    //if we want to have the right edge a different color...
    //for(jj=k; jj<k+3; jj++)
    //  xpm_template[jj][5*3] = ' ';


  }

  return xpm_template;
}


/***************************************************************
 * CreateFont
 *   Here we read the lcdfont.h file and build LCD character
 * pixmaps.
 *
 */

LcdFont::LcdFont (gint characters,  GtkWidget *parent_window, LcdDisplay *lcdP)
{
  gint i;

  num_elements = characters;
  pixmaps = (GdkPixmap **)malloc( sizeof (GdkPixmap *) * num_elements);
  mywindow = parent_window->window;

  for(i=0; i<num_elements; i++) {

    if(strlen(test[i][0]) < 5)
      pixmaps[i] = NULL;
    else
      pixmaps[i] = gdk_pixmap_create_from_xpm_d (
                                 mywindow,
                                 NULL,
                                 lcdP->dot_color,
                                 CreateXPMdataFromLCDdata(lcdP,&test[i]));
  }
}

void LcdFont::update_pixmap(int pos, _5X7 *tempchar, LcdDisplay *lcdP)
{
    if (pixmaps[pos]) {
      g_free(pixmaps[pos]);
      pixmaps[pos] = NULL;
    }
    pixmaps[pos] = gdk_pixmap_create_from_xpm_d (
                               mywindow,
                               NULL,
                               lcdP->dot_color,
                               CreateXPMdataFromLCDdata(lcdP, tempchar));
}

GdkPixmap *LcdFont::getPixMap(unsigned int index)
{
  return ( (index < (unsigned int)num_elements) && pixmaps[index]) ? pixmaps[index] : pixmaps[0];
}

void LcdDisplay::update_cgram_pixmaps()
{
  int i, j, k;
  _5X7 tempchar;

  if (fontP == NULL)
    return;

  for (i = 0; i < CGRAM_SIZE / 8; i++) {
    for (j = 0; j < 7; j++) {
      for (k = 0; k < 5; k++) {
        if (m_hd44780->getCGRam(8 * i + j) & (1 << (4 - k)))
          tempchar[j][k] = '.';
        else
          tempchar[j][k] = ' ';
      }
      tempchar[j][5] = 0;
    }
    fontP->update_pixmap(i, &tempchar, this);

  }
  cgram_updated = false;
}

GdkPixmap *LcdDisplay::get_pixmap(gint row, gint col)
{

  if (cgram_updated)
    update_cgram_pixmaps();

  return fontP ? fontP->getPixMap(m_hd44780->getDDRam(row,col))  : 0;
}


static gint
lcd_expose_event (GtkWidget *widget,
                  GdkEvent  *event,
                  gpointer   user_data)
{

  LcdDisplay *lcdP;
  guint max_width;
  guint max_height;


  g_return_val_if_fail (widget != NULL, TRUE);
  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);

  lcdP = (LcdDisplay *)user_data;

  max_width = widget->allocation.width;
  max_height = widget->allocation.height;

  lcdP->update(widget,max_width,max_height);

  return TRUE;
}

void LcdDisplay::update(  GtkWidget *widget,
                             guint new_width,
                             guint new_height)
{


  GdkDrawable *drawable;
  GdkGC *lcd_gc;
  guint i,j;


  drawable = widget->window;
  lcd_gc = gdk_gc_new(widget->window);
  gdk_gc_set_foreground(lcd_gc,dot_color);

  w_width = new_width;
  w_height = new_height;

  gdk_draw_rectangle (drawable, lcd_gc,
                      TRUE,
                      0,
                      0,
                      w_width,
                      w_height);


  // Don't display anything if the display is disabled
  //if(display_is_off())
  //  return;

  // If there is no font, then go create it.

  if(!fontP) {
    fontP = new LcdFont(FONT_LEN,widget,this);
    //CreateFont(widget,this); //,xpm_test);
  }

  gint cw = get_char_width();
  gint ch = get_char_height();
  gint border = get_border();


  if (!(disp_type & TWO_ROWS_IN_ONE)) {
    for(j=0; j<rows; j++)
      for(i=0; i<cols; i++)
        gdk_draw_pixmap (widget->window,lcd_gc,get_pixmap(j,i),
                         0,0,
                         border+i*cw, border+j*(ch+border),
                         cw,ch);
  }
  else {
    guint pos;
    for(pos=0,j=0; j<rows; j++)
      for(i=0; i<cols; pos++,i++)
        gdk_draw_pixmap (widget->window,lcd_gc,get_pixmap(j,i),
                         0,0,
                         border+pos*cw, border,
                         cw,ch);
  }
}

static gint
cursor_event (GtkWidget          *widget,
              GdkEvent           *event,
              gpointer  *user_data)
{
  if ((event->type == GDK_BUTTON_PRESS) &&
      ((event->button.button == 1) ||
       (event->button.button == 3)))
    {
      return TRUE;
    }

  return FALSE;
}


GdkColor *NewColor(gint32 red, gint32 green, gint32 blue)
{
  GdkColor *c = (GdkColor *) g_malloc(sizeof(GdkColor));


  c->red = red;
  c->green = green;
  c->blue = blue;

  gdk_colormap_alloc_color(gdk_colormap_get_system(), c, FALSE, TRUE);

  return c;

}


/**********************************************************
 *
 *
 */

void LcdDisplay::CreateGraphics (void)
{

//  GtkWidget *window;
  GtkWidget *main_vbox;
  GtkWidget *frame;
  GtkWidget *vbox;
  char buf[48];

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  if(window) {


    gtk_window_set_wmclass(GTK_WINDOW(window),type(),"Gpsim");
    //
    //   Allocate memory for the LCD font.
    //

    sprintf(buf,"%d X %d",rows,cols);
    if (disp_type & TWO_ROWS_IN_ONE)
      strcat(buf," (in one row)");
    title = (gchar *)strdup(buf);

    gtk_widget_realize (window);

    gtk_window_set_title(GTK_WINDOW(window), "LCD");

//    gtk_signal_connect (GTK_OBJECT (window), "destroy",
//                      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

    main_vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 0);
    gtk_container_add (GTK_CONTAINER (window), main_vbox);

    vbox =
      gtk_widget_new (gtk_vbox_get_type (),
                      "GtkBox::homogeneous", FALSE,
                      //"GtkBox::spacing", 5,
                      //"GtkContainer::border_width", 10,
                      "GtkWidget::parent", main_vbox,
                      "GtkWidget::visible", TRUE,
                      NULL);


    frame =
      gtk_widget_new (gtk_frame_get_type (),
                      "GtkFrame::shadow", GTK_SHADOW_ETCHED_IN,
                      "GtkFrame::label_xalign", 0.5,
                      "GtkFrame::label", title,
                      //"GtkContainer::border_width", 10,
                      "GtkWidget::parent", vbox,
                      "GtkWidget::visible", TRUE,
                      NULL);


    darea = gtk_drawing_area_new ();
    if (!(disp_type & TWO_ROWS_IN_ONE)) {
      gtk_widget_set_usize (darea,
                            cols*get_char_width()+2*get_border(),
                            rows*(get_char_height()+get_border())+get_border());
    }
    else {
      gtk_widget_set_usize (darea,
                            rows*cols*get_char_width()+2*get_border(),
                            get_char_height()+2*get_border());
    }
    gtk_container_add (GTK_CONTAINER (frame), darea);

    gtk_signal_connect (GTK_OBJECT (darea),
                        "expose_event",
                        GTK_SIGNAL_FUNC (lcd_expose_event),
                        this);
    gtk_widget_set_events (darea, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
    gtk_signal_connect (GTK_OBJECT (darea),
                        "button_press_event",
                        GTK_SIGNAL_FUNC (cursor_event),
                        NULL);

    gtk_widget_show (darea);

    dot_color = NewColor(0x7800,0xa800,0x7800);
    gc = gdk_gc_new(darea->window);
    g_assert(gc!= (GdkGC*)NULL);

    gtk_widget_show_all (window);

  }


}

void LcdDisplay::move_cursor(unsigned int new_row, unsigned int new_column)
{

  if( new_row < rows && new_column < cols ) {
    cursor.row = new_row;
    cursor.col = new_column;
  }
  m_hd44780->moveCursor(new_row, new_column);

}

void LcdDisplay::clear_display(void)
{
  m_hd44780->clearDisplay();
  m_hd44780->moveCursor(0,0);
}
#endif //HAVE_GUI
