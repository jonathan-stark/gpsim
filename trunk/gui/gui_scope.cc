/*
   Copyright (C) 2004 T. Scott Dattalo

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


/*
           S C O P E 

A simple waveform viewer for gpsim. Inspired from Nuno Sucena Almeida's
gpsim_la - plug in.

*/

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

#include "gui.h"
#include "gui_scope.h"

// Number of Input Ports:
#define NUM_PORTS 8
// Number of signal lines bit points
#define MAX_BIT_POINTS 10000

// Zoom:
#define ZOOM_STEP 2
#define ZOOM_MIN 10
#define ZOOM_MAX MAX_BIT_POINTS

#define ZOOM_IN 0
#define ZOOM_OUT !ZOOM_IN

// Update Delay:
#define DELAY_DEFAULT_VALUE 10
#define DELAY_MIN_VALUE 0
#define DELAY_MAX_VALUE 100


static GtkObject *bit_adjust; // ,*delay_adjust;
static GdkColor signal_line_color,grid_line_color,grid_v_line_color;
//static int bit_left,bit_right,bit_points,update_delay;


/*
  Waveform class

  This holds the gui information related with a gpsim waveform
*/
class Waveform
{
public:

  GtkWidget *drawing_area;   // The drawing container that holds the pixmap
  GdkPixmap *pixmap;         // The Waveform is rendered in this pixmap.
  int width, height;         // Pixmap size
  GdkGC *drawing_gc;         // Line styles, etc.
  Scope_Window *sw;          // Parent
  char *name;                // Name of the waveform. (FIXME)
  bool isBuilt;              // True after the gui has been built.
  bool isUpToDate;           // False when the waveform needs updating.

  GtkWidget *parent_table;
  int row;

  Waveform(Scope_Window *parent);

  void Build(GtkWidget *_parent_table, int _row);
  void Update(void);
  void Expose(void);
  void Resize(int width, int height);

};


Waveform::Waveform(Scope_Window *parent)
{
  isBuilt = false;
  isUpToDate = false;
  name = 0;
  drawing_area = 0;
  pixmap =0;
  drawing_gc =0;
  parent_table = 0;

  //Default pixmap size
  width  = 400;
  height = 25;

  sw = parent;
}


static gint Waveform_expose_event (GtkWidget *widget,
				   GdkEventExpose  *event,
				   gpointer   user_data)
{
  // cout <<  "function:" << __FUNCTION__ << "\n";    

  g_return_val_if_fail (widget != NULL, TRUE);
  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);

  Waveform *wf = (Waveform *)(user_data);
  if(!wf)
    return 0;
  /*
  cout << " event  "
       << ':' << event->area.x << ':' << event->area.y 
       << ':' << event->area.x << ':' <<  event->area.y
       << ':' << event->area.width << ':' << event->area.height
       << endl;
  */
  wf->Expose();
  return FALSE;
}


static gint Waveform_configure_event (GtkWidget *widget, GdkEventConfigure *event,
				      gpointer user_data)
{
  cout <<  "function:" << __FUNCTION__ << "\n";

  g_return_val_if_fail (widget != NULL, TRUE);
  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);


  Waveform *wf = (Waveform *)(user_data);
  if(!wf)
    return 0;

  return TRUE;
}

void Waveform::Build(GtkWidget *_parent_table, int _row)
{

  parent_table = _parent_table;
  row = _row;

  cout << "Waveform::" << __FUNCTION__ << "  row " << row << endl;

  drawing_area = gtk_drawing_area_new ();
  gtk_widget_set_usize (drawing_area,width,height);    
  gtk_widget_set_events (drawing_area, GDK_EXPOSURE_MASK );    
  gtk_table_attach_defaults (GTK_TABLE(parent_table),drawing_area,0,10,row,row+1);

  cout <<  "Waveform::" << __FUNCTION__ 
       << "  width " << width
       << "  height " << height << endl;

  if (pixmap)
    gdk_pixmap_unref(pixmap);
    
  pixmap = gdk_pixmap_new(drawing_area->window,
			  width,
			  height,
			  -1);

  gtk_signal_connect (GTK_OBJECT (drawing_area),
		      "expose_event",
		      GTK_SIGNAL_FUNC (Waveform_expose_event),
		      this);

  gtk_signal_connect (GTK_OBJECT(drawing_area),"configure_event",
		      (GtkSignalFunc) Waveform_configure_event,
		      this);

  // Graphics Context:
  drawing_gc = gdk_gc_new(drawing_area->window);
  gdk_gc_set_line_attributes(drawing_gc,1,GDK_LINE_SOLID,
			     GDK_CAP_ROUND,GDK_JOIN_ROUND);

  name = strdup("test");

  isBuilt = true;
  isUpToDate = false;

  Update();
}

//----------------------------------------
void Waveform::Resize(int w, int h)
{

  if(pixmap && w==width && h==height)
    return;

  if(w<100 || h<5)
    return;

  cout << "Waveform::" << __FUNCTION__ << endl;

    
  width = w;
  height = h;


  if (pixmap)
    gdk_pixmap_unref(pixmap);
  pixmap = gdk_pixmap_new(drawing_area->window,
			  width,
			  height,
			  -1);
  //Build(row);

  isUpToDate = false;

  Update();

}
//----------------------------------------
//
// Waveform Update
//

void Waveform::Update(void)
{
  int x,y;
  GdkRectangle update_rect;
#if 0

  int line_separation,pin_number;
  int point,y_text,y_0,y_1;
  float x_scale,y_scale;
  int max_str,new_str,br_length;
  char *s,ss[10];

  if(!isBuilt || isUpToDate)
    return;

  if(!pixmap) {
    cout << __FUNCTION__ << " pixmap is NULL\n";
    return;
  }

  cout << "Waveform::" << __FUNCTION__ << endl;


  gdk_draw_rectangle (pixmap,
		      drawing_area->style->black_gc,
		      TRUE,
		      0, 0,
		      width,
		      height);
  y_scale = (float)height / (float)(NUM_PORTS);
    

  
  char ntest[] = "test0";
  // Draw pin name:
  max_str = 0;
  for (pin_number=1;pin_number<=NUM_PORTS;pin_number++)
    {
      y = (int)((float)y_scale*(float)(pin_number)-(float)(y_scale/4));
      //s = Package::get_pin_name(pin_number);
      ntest[4] = pin_number + '0';
      gdk_draw_text (pixmap,drawing_area->style->font,
		     drawing_area->style->white_gc,0,y,ntest,strlen(ntest));
      new_str = gdk_text_width (drawing_area->style->font,ntest,strlen(ntest));
      if (new_str>max_str)
	max_str=new_str;
    }
  y_text = y;
#endif

  //
  // Draw Vertical Grid Lines:
  //

  gdk_gc_set_foreground(drawing_gc,&grid_line_color);

  for(x=0; x<width; x+= width/20)
    gdk_draw_line(pixmap,drawing_gc,x,1,x,height-1);

  //
  // Draw Horizontal Grid Lines:
  //

  gdk_gc_set_foreground(drawing_gc,&grid_line_color);    
  gdk_draw_line(pixmap,drawing_gc,0,height-1,width,height-1);
  

#if 0
  // Draw Vertical Grid Lines:
  gdk_gc_set_foreground(drawing_gc,&grid_v_line_color);
  x_scale = (float)(width-max_str)/(float)bit_points;
  //    cout << "x_scale:" << x_scale << "\n";
  //    cout << "y_scale:" << y_scale << "\n";    
    
  for (point=0;point<bit_points;point++)
    {
      x = (int)(((float)point)*x_scale+max_str);
      gdk_draw_line(pixmap,drawing_gc,x,0,x,height);
    }
  // Draw Horizontal Grid Lines:
  gdk_gc_set_foreground(drawing_gc,&grid_line_color);    
  for (pin_number=0;pin_number<(NUM_PORTS-1);pin_number++)
    {
      y = (int)(y_scale*(float)(pin_number+1));
      gdk_draw_line(pixmap,drawing_gc,0,y,width,y;)
    }

#endif


  // Draw Signals:
  gdk_gc_set_foreground(drawing_gc,&signal_line_color);    

  x = 0;
  y = 0;
  int nextx;
  int nexty;

  int h = height-1;

  while(x <width) {
    nextx = x + (rand() % 10);
    if(nextx >= width)
      nextx = width;

    gdk_draw_line(pixmap,drawing_gc,x,h-y*10,nextx,h-y*10);

    x = nextx;
    nexty = y ^ 1;
    gdk_draw_line(pixmap,drawing_gc,x,h-y*10,x,h-nexty*10);
    y = nexty;

  }

#if 0
  // Draw Signals:
  gdk_gc_set_foreground(drawing_gc,&signal_line_color);    
  for (pin_number=1;pin_number<=NUM_PORTS;pin_number++)
    {
      y_0 = (int)(y_scale*(float)(pin_number)-y_scale/4.0);
      y_1 = (int)(y_scale*(float)(pin_number)-y_scale*3.0/4.0);
      //        cout << "Name:" << Package::get_pin_name(pin_number) << "\n";
      for (point=0;point<bit_points;point++)
        {
	  x = (int)((float)point*x_scale+max_str);
	  ///if (port->get_pin_bit_value(bit_left+point,pin_number)==0)
	  if(y & 1)
	    y = y_0;
	  else
	    y = y_1;
	  gdk_draw_line(pixmap,drawing_gc,x,y,(int)(x+(float)x_scale),y);
        }
    }
#endif

#if 0
  // Draw bit positions:
  sprintf (ss,"[%d]",bit_left);
  gdk_draw_text (pixmap,drawing_area->style->font,
		 drawing_area->style->white_gc,
		 max_str,(int)y,ss,strlen(ss));
  sprintf (ss,"[%d]",bit_right);
  br_length = gdk_text_width (drawing_area->style->font,ss,strlen(ss));
  gdk_draw_text (pixmap,drawing_area->style->font,
		 drawing_area->style->white_gc,
		 width-br_length,(int)y,ss,strlen(ss));

#endif

  isUpToDate = true;

  update_rect.x = 0;
  update_rect.y = 0;
  update_rect.width = width;
  update_rect.height = height;
  gtk_widget_draw (drawing_area,&update_rect);

  Expose();
}

//----------------------------------------
//
// Waveform Expose
//

void Waveform::Expose(void)
{

  if(!isBuilt || !pixmap || !drawing_area)
    return;

  if(!isUpToDate)
    Update();

  cout <<  "function:" << __FUNCTION__ << "\n";    


  gdk_draw_pixmap(drawing_area->window,
		  drawing_area->style->fg_gc[GTK_WIDGET_STATE (drawing_area)],
		  pixmap,
		  0,0,   // x,y
		  0,0,
		  width,height);

  gtk_widget_show(drawing_area);

}


//------------------------------------------------------------------------
// Signals


//----------------------------------------
//
// When a user clicks on the "X" in the window managers border of the
// scope window, we'll capture that event and hide the scope.
//
static int delete_event(GtkWidget *widget,
			GdkEvent  *event,
                        Scope_Window *sw)
{

  sw->ChangeView(VIEW_HIDE);
  return TRUE;
}

static gint
analyzer_clear_callback (GtkWidget *widget, gpointer user_data)
{
  //    Analyzer_Screen *as=(Analyzer_Screen*)user_data;
  cout <<  "function:" << __FUNCTION__ << "\n";    

  //as->port->init_bit_points();
  //  as->update();

  cout <<  "End of function:" << __FUNCTION__ << "\n";
  return(FALSE);    
}

static gint
analyzer_update_scale (GtkAdjustment *adj,gpointer user_data)
{
  //    Analyzer_Screen *as=(Analyzer_Screen*)user_data;
//    cout << "value:" << (int)adj->value << "\n";
  cout <<  "function:" << __FUNCTION__ << "\n";    
  //as->set_bit_left((int)adj->value);
  //as->update();
  return(FALSE);
}

/*
static gint
analyzer_update_delay (GtkAdjustment *adj,gpointer user_data)
{
  //((Analyzer_Screen*)user_data)->set_update_delay((int)adj->value);
  cout <<  "function:" << __FUNCTION__ << "\n";    
  return(FALSE);
}
*/

static gint Scope_Window_expose_event (GtkWidget *widget,
				   GdkEventExpose  *event,
				   gpointer   user_data)
{
  cout <<  "function:" << __FUNCTION__ << "\n";    

  g_return_val_if_fail (widget != NULL, TRUE);
  //  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);

  Scope_Window *sw = (Scope_Window *)user_data;
  if(sw)
    sw->Update();

  return TRUE;
}
//------------------------------------------------------------------------
//
// Scope_Window member functions
//
//


Waveform *signals[8];   // hack
int aw=0;
int ah=0;

//------------------------------------------------------------------------

void Scope_Window::Build(void)
{

  GtkWidget *table;
  GtkWidget *scroll_bar,*button;
  GtkTooltips *tooltips;    

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  if (!window)
    return;


  gtk_widget_realize (window);
  gtk_container_set_border_width(GTK_CONTAINER(window),5);    

  gtk_window_set_title(GTK_WINDOW(window), "Scope");


  tooltips = gtk_tooltips_new();

  //
  // The Scope window is built on top of a 10X10 packing table
  //

  table = gtk_table_new (10,10,TRUE);
  gtk_table_set_col_spacings(GTK_TABLE(table),5);

  //
  // Control buttons
  // (this is changing...)

  gtk_container_add (GTK_CONTAINER (window),table);
  button = gtk_button_new_with_label ("Clear");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (analyzer_clear_callback),this);
  gtk_table_attach_defaults (GTK_TABLE(table),button,0,2,9,10);


  gtk_signal_connect(GTK_OBJECT (window), "delete_event",
		     GTK_SIGNAL_FUNC(delete_event), this);

  gtk_signal_connect (GTK_OBJECT (window),
		      "expose_event",
		      GTK_SIGNAL_FUNC (Scope_Window_expose_event),
		      this);

#if 0
  GtkWidget *spin_button;
  button = gtk_button_new_with_label ("Zoom In");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (analyzer_zoom_in_callback),this);    
  gtk_table_attach_defaults (GTK_TABLE(table),button,2,4,9,10);    
  button = gtk_button_new_with_label ("Zoom Out");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (analyzer_zoom_out_callback),this);    
  gtk_table_attach_defaults (GTK_TABLE(table),button,4,6,9,10);
  button = gtk_button_new_with_label ("About");
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (analyzer_screen_about),this);    
  gtk_table_attach_defaults (GTK_TABLE(table),button,6,8,9,10);    
#endif


#define port_get_max_bit_points 200

  //
  // Horizontal Scroll Bar
  //

  bit_adjust = gtk_adjustment_new(0,0,port_get_max_bit_points,1,10,ZOOM_MIN);
  gtk_signal_connect (GTK_OBJECT (bit_adjust), "value_changed",
		      GTK_SIGNAL_FUNC (analyzer_update_scale), this);
  scroll_bar = gtk_hscrollbar_new(GTK_ADJUSTMENT(bit_adjust));
  gtk_table_attach_defaults (GTK_TABLE(table),scroll_bar,0,10,8,9);


#if 0

  // do we really want to have an update delay?

  delay_adjust = gtk_adjustment_new(update_delay,
				    DELAY_MIN_VALUE,
				    DELAY_MAX_VALUE,1,5,0);
  gtk_signal_connect (GTK_OBJECT (delay_adjust), "value_changed",
		      GTK_SIGNAL_FUNC (analyzer_update_delay), this);
  spin_button = gtk_spin_button_new(GTK_ADJUSTMENT(delay_adjust),0.5,0);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin_button),TRUE);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(spin_button),FALSE);
  gtk_spin_button_set_snap_to_ticks(GTK_SPIN_BUTTON(spin_button),TRUE);
  gtk_table_attach_defaults (GTK_TABLE(table),spin_button,8,10,9,10);
  gtk_tooltips_set_tip(tooltips,spin_button,"Set Update Delay",NULL);
#endif

  //
  // Define the drawing colors
  //


  // The signal color is bright red
  signal_line_color.red = 0xff00;
  signal_line_color.green = 0x0000;
  signal_line_color.blue = 0x0000;
  gdk_color_alloc(gdk_colormap_get_system(), &signal_line_color);
  // The grid color is bright green
  grid_line_color.red = 0x4000;
  grid_line_color.green = 0x4000;
  grid_line_color.blue = 0x4000;
  gdk_color_alloc(gdk_colormap_get_system(), &grid_line_color);
  // The vertical grid color is dark green
  grid_v_line_color.red = 0x0000;
  grid_v_line_color.green = 0x2200;
  grid_v_line_color.blue = 0x0000;
  gdk_color_alloc(gdk_colormap_get_system(), &grid_v_line_color);


  //
  // Create the signals for the scope window.
  //

  for(int i=0; i<8; i++) {

    signals[i] = new Waveform(this);
    signals[i]->Build(table, i);
  }


  
  gtk_widget_show_all (window);
    
  //    cout <<  "end function:" << __FUNCTION__ << "\n";
  is_built = 1;


  aw = window->allocation.width;
  ah = window->allocation.height;

}


void Scope_Window::Update(void)
{
  int i;
  if(!is_built)
    Build();

  cout << "function:" << __FUNCTION__ << "\n";
  cout << " a  x "  << window->allocation.x
       << " a y "  << window->allocation.y
       << " a  width "  << window->allocation.width
       << " a height "  << window->allocation.height
       << endl;
  cout << " r  width "  << window->requisition.width
       << " r height "  << window->requisition.height
       << endl;

  if(aw != window->allocation.width ||
     ah != window->allocation.height) {
    
    aw = window->allocation.width;
    ah = window->allocation.height;

    for(i=0; i<8; i++) {
      if(signals[i])
	signals[i]->Resize(aw-15,(ah-10)/10);
    }

  }

  for(i=0; i<8; i++) {

    if(signals[i])
      signals[i]->Update();

  }

  gtk_widget_show_all(window);

    
}

Scope_Window::Scope_Window(GUI_Processor *_gp)
{

  gp = _gp;
  window = 0;
  wc = WC_data;
  wt = WT_scope_window;

  menu = "<main>/Windows/Scope";
  set_name("scope");

  get_config();

  if(enabled)
    Build();

}

#endif //HAVE_GUI
