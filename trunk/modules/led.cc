/*
   Copyright (C) 2000 T. Scott Dattalo

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

  led.cc

  This is an example module illustrating how gpsim modules may be created.
  Additional examples may also be found with the gpsim.

  This particular example creates a 7-segment, common cathode LED display.

  Pin Numbering of LED:
  --------------------
       a
      ---
   f | g | b    
      ---
   e |   | c
      ---
       d
  cc = common cathode


  Electrical:
  ----------

   a  ---|>|---+
   b  ---|>|---+
   c  ---|>|---+
   d  ---|>|---+
   e  ---|>|---+
   f  ---|>|---+
   g  ---|>|---+
               |
              cc

  How It Works:
  ------------

  Once the Led module has been built (and optionally installed), you
  can include it in your .stc file. See the examples subdirectory.

*/


/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include <errno.h>
#include <stdlib.h>
#include <string>
#include <iostream>

#include "../config.h"    // get the definition for HAVE_GUI

#ifdef HAVE_GUI
#include <gtk/gtk.h>
#include <math.h>

#include "../src/gpsim_interface.h"

#include "led.h"

//--------------------------------------------------------------
//
// Create an "interface" to gpsim
//


class LED_Interface : public Interface
{
private:
  Led_base *led;
  int lastport;

public:

  //virtual void UpdateObject (gpointer xref,int new_value);
  //virtual void RemoveObject (gpointer xref);
  virtual void SimulationHasStopped (gpointer object)
  {
    GuiUpdate(object);
  }

  //virtual void NewProcessor (unsigned int processor_id);
  //virtual void NewModule (Module *module);
  //virtual void NodeConfigurationChanged (Stimulus_Node *node);
  //virtual void NewProgram  (unsigned int processor_id);
  virtual void GuiUpdate  (gpointer object)
  {
    if(led)
    {
      int portval = led->port->get_value();
      if(lastport != portval)
      {
	lastport=portval;
	led->update();
      }
    }
  }


  LED_Interface(Led_base *_led) : Interface((gpointer *) _led)
  {
    led = _led;
    lastport = -1;
  }

};

Led_Port::Led_Port (unsigned int _num_iopins) : IOPORT(_num_iopins)
{

}

void Led_Port::trace_register_write(void)
{

  //get_trace().module1(value.get());
  get_trace().raw(value.get());
}


void Led_7Segments::update(void)
{
  update(darea, w_width,w_height);
}

void Led_7Segments::update(  GtkWidget *widget,
			     guint new_width,
			     guint new_height)
{

  guint i;

  w_width = new_width;
  w_height = new_height;
  GdkDrawable *drawable = widget->window;

  if(segment_gc==NULL)
  {
      segment_gc = gdk_gc_new(darea->window);
      gdk_gc_set_line_attributes(segment_gc,
				 5,
				 GDK_LINE_SOLID,
				 GDK_CAP_ROUND,
				 GDK_JOIN_ROUND);
      g_assert(segment_gc!=NULL);
  }


  // not a very O-O way of doing it... but here we go directly
  // to the I/O port and get the values of the segments
  int segment_states = port->get_value();

  GdkGC *gc = segment_gc;

  gdk_gc_set_foreground(gc,
			&led_background_color);

  gdk_draw_rectangle (drawable, gc,
		      TRUE,
		      0,
		      0,
		      w_width,
		      w_height);


  // cout << "expose led, segment states = " << segment_states << '\n';

  if( (segment_states & 1) == 0) {
    // common cathode, cathode must be low to turn
    //digits on.

    gdk_gc_set_foreground(gc,&led_segment_on_color);

    for(i=0; i<7; i++) {
      if(segment_states & (2<<i))
	gdk_draw_polygon ( drawable,
			   gc,
			   TRUE,
			   segments[i].p,
			   6);


    }
  }

  gdk_gc_set_foreground(gc,&led_segment_off_color);

  // turn off the segments that aren't being driven.

  for(i=0; i<7; i++) {
    if((segment_states & (2<<i)) == 0)
	gdk_draw_polygon ( drawable,
			   gc,
			   TRUE,
			   segments[i].p,
			   6);

  }
}


static gint
led7_expose_event (GtkWidget *widget,
		  GdkEvent  *event,
		  gpointer   user_data)
{


  Led_7Segments *led;
  guint max_width;
  guint max_height;

  g_return_val_if_fail (widget != NULL, TRUE);
  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);

  // de-reference the user_data into an led object
  led = (Led_7Segments *)user_data;

  max_width = widget->allocation.width;
  max_height = widget->allocation.height;

  led->update(widget,max_width,max_height);

  return TRUE;
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

//-------------------------------------------------------------------
// build_segments
//
// from Dclock.c (v.2.0) -- a digital clock widget.
// Copyright (c) 1988 Dan Heller <argv@sun.com>
// Modifications 2/93 by Tim Edwards <tim@sinh.stanford.edu>
// And further modifications by Scott Dattalo <scott@dattalo.com>
//
// Each segment on the LED is comprised of a 6 point polygon.
// This routine will calculate what those points should be and
// store them an arrary. 

void Led_7Segments::build_segments( int w, int h)
{
  XfPoint *pts;
  float spacer, hskip, fslope, bslope, midpt, seg_width, segxw;
  float invcosphi, invsinphi, invcospsi, invsinpsi, slope;
  float dx1, dx2, dx3, dx4, dx5, dx6, dy1, dy2, dy5, dy6;
  float xfactor, temp_xpts[4];


  w_width = w;
  w_height = h;

  // Hard code the display parameters...

  space_factor = (float)0.13;
  width_factor = (float)0.13;
  sxw = (float)0.13;
  angle = 6;

  /* define various useful constants */
 
  segxw = sxw * w;
  slope = angle;
  seg_width = width_factor * w;
  spacer = (float)w * space_factor;
  hskip = (float)(seg_width * 0.125);
  fslope = 1 / (segxw/seg_width + 1/slope);
  bslope = -1 / (segxw/seg_width - 1/slope);
  midpt = (float)h / 2;

  /* define some trigonometric values */
  /*  phi is the forward angle separating two segments; 
      psi is the reverse angle separating two segments. */

  invsinphi = sqrt(1 + fslope * fslope) / fslope;
  invcosphi = sqrt(1 + 1/(fslope * fslope)) * fslope;
  invsinpsi = sqrt(1 + bslope * bslope) / -bslope;
  invcospsi = sqrt(1 + 1/(bslope * bslope)) * bslope;

  /* define offsets from easily-calculated points for 6 situations */

  dx1 = hskip * invsinphi / (slope/fslope - 1);
  dy1 = hskip * invcosphi / (1 - fslope/slope);
  dx2 = hskip * invsinpsi / (1 - slope/bslope);
  dy2 = hskip * invcospsi / (bslope/slope - 1);
  dx3 = hskip * invsinphi;
  dx4 = hskip * invsinpsi;
  dx5 = hskip * invsinpsi / (1 - fslope/bslope);
  dy5 = hskip * invcospsi / (bslope/fslope - 1);
  dx6 = dy5;
  dy6 = dx5;

  /* calculate some simple reference points */

  temp_xpts[0] = spacer + (h - seg_width)/slope;
  temp_xpts[1] = spacer + (h - seg_width/2)/slope + segxw/2;
  temp_xpts[2] = spacer + h/slope + segxw;
  temp_xpts[3] = temp_xpts[0] + segxw;

  xfactor = w - 2 * spacer - h / slope - segxw;

  /*
  cout << "temp_xpts[2] " << temp_xpts[2] << '\n';
  cout << "dx3 " << dx3 << '\n';
  cout << "fslope " << fslope << '\n';
  */

  /* calculate the digit positions */

  pts = seg_pts[TOP];
  pts[0].y = pts[1].y = 0;
  pts[0].x = temp_xpts[2] - dx3;
  pts[1].x = w - spacer - segxw + dx4;
  pts[2].y = pts[5].y = (seg_width / 2) - dy5 - dy6;
  pts[5].x = temp_xpts[1] + dx5 - dx6;
  pts[2].x = pts[5].x + xfactor;
  pts[3].y = pts[4].y = seg_width;
  pts[4].x = temp_xpts[3] + dx4;
  pts[3].x = temp_xpts[0] + xfactor - dx3; 

  pts = &(seg_pts[MIDDLE][0]);
  pts[0].y = pts[1].y = midpt - seg_width/2;
  pts[0].x = spacer + (h - pts[0].y)/slope + segxw;
  pts[1].x = pts[0].x - segxw + xfactor;
  pts[2].y = pts[5].y = midpt;
  pts[3].y = pts[4].y = midpt + seg_width/2;
  pts[5].x = spacer + (h - pts[5].y)/slope + segxw/2;
  pts[2].x = pts[5].x + xfactor;
  pts[4].x = pts[0].x - seg_width/slope;
  pts[3].x = spacer + (h - pts[3].y)/slope + xfactor;

  pts = &(seg_pts[BOTTOM][0]);
  pts[3].y = pts[4].y = (float)h;
  pts[2].y = pts[5].y = h - (seg_width / 2) + dy5 + dy6;
  pts[0].y = pts[1].y = h - seg_width;
  pts[0].x = spacer + segxw + seg_width/slope + dx3;  
  pts[1].x = spacer + (h - pts[1].y)/slope + xfactor - dx4;
  pts[4].x = spacer + segxw - dx4;
  pts[5].x = spacer + segxw/2 + (h - pts[5].y)/slope + dx6 - dx5;
  pts[2].x = pts[5].x + xfactor;
  pts[3].x = spacer + xfactor + dx3;

  pts = &(seg_pts[TOP_LEFT][0]);
  pts[0].y = seg_width / 2 - dy6 + dy5;
  pts[1].y = seg_width + dy2;
  pts[2].y = seg_pts[MIDDLE][0].y - 2 * dy1;
  pts[3].y = seg_pts[MIDDLE][5].y - 2 * dy6;
  pts[4].y = seg_pts[MIDDLE][0].y;
  pts[5].y = seg_width - dy1;
  pts[0].x = temp_xpts[1] - dx5 - dx6;
  pts[1].x = temp_xpts[3] - dx2;
  pts[2].x = seg_pts[MIDDLE][0].x + 2 * dx1; 
  pts[3].x = seg_pts[MIDDLE][5].x - 2 * dx6;
  pts[4].x = spacer + (h - pts[4].y)/slope;
  pts[5].x = temp_xpts[0] + dx1;

  pts = &(seg_pts[BOT_LEFT][0]);
  pts[0].y = seg_pts[MIDDLE][5].y + 2 * dy5;
  pts[1].y = seg_pts[MIDDLE][4].y + 2 * dy2;
  pts[2].y = seg_pts[BOTTOM][0].y - dy1;
  pts[3].y = seg_pts[BOTTOM][5].y - 2 * dy6;
  pts[4].y = h - seg_width + dy2;
  pts[5].y = midpt + seg_width/2;
  pts[0].x = seg_pts[MIDDLE][5].x - 2 * dx5;
  pts[1].x = seg_pts[MIDDLE][4].x - 2 * dx2;
  pts[2].x = seg_pts[BOTTOM][0].x - dx3 + dx1;
  pts[3].x = seg_pts[BOTTOM][5].x - 2 * dx6;
  pts[4].x = spacer + seg_width / slope - dx2;
  pts[5].x = spacer + (midpt - seg_width/2) / slope;

  pts = &(seg_pts[TOP_RIGHT][0]);
  pts[0].y = seg_width/2 - dy5 + dy6;
  pts[1].y = seg_width - dy2;
  pts[2].y = midpt - seg_width/2;
  pts[3].y = midpt - 2 * dy5;
  pts[4].y = pts[2].y - 2 * dy2;
  pts[5].y = seg_width + dy1;
  pts[0].x = temp_xpts[1] + xfactor + dx5 + dx6;
  pts[1].x = temp_xpts[3] + xfactor + dx1;
  pts[2].x = seg_pts[MIDDLE][0].x + xfactor;
  pts[3].x = seg_pts[MIDDLE][5].x + xfactor + dx5 * 2;
  pts[4].x = seg_pts[TOP_LEFT][4].x + xfactor + dx2 * 2;
  pts[5].x = temp_xpts[0] + xfactor - dx1;

  pts = &(seg_pts[BOT_RIGHT][0]);
  pts[0].y = seg_pts[MIDDLE][2].y + 2 * dy6;
  pts[1].y = midpt + seg_width / 2;
  pts[2].y = h - seg_width + dy1;
  pts[3].y = h - (seg_width / 2) + dy6 - dy5;
  pts[4].y = h - seg_width - dy2;
  pts[5].y = seg_pts[MIDDLE][3].y + 2 * dy1;
  pts[0].x = seg_pts[MIDDLE][2].x + 2 * dx6;
  pts[1].x = seg_pts[MIDDLE][3].x + segxw;
  pts[2].x = seg_pts[BOTTOM][1].x + dx4 + segxw - dx1;
  pts[3].x = seg_pts[BOTTOM][2].x + 2 * dx5;
  pts[4].x = seg_pts[BOTTOM][1].x + dx4 + dx2;
  pts[5].x = seg_pts[MIDDLE][3].x - 2 * dx1;

  // Convert the floating point points into integers.
  int i,j;
  for(i=0; i<NUM_SEGS; i++) {

    for(j=0; j<MAX_PTS; j++) {

      segments[i].p[j].x = (int)seg_pts[i][j].x;
      segments[i].p[j].y = (int)seg_pts[i][j].y;
    }
  }

}

void Led_7Segments::build_window(void)
{
  GtkWidget *main_vbox;
  GtkWidget *vbox;

    main_vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 0);

    vbox =
      gtk_widget_new (gtk_vbox_get_type (),
		      "GtkBox::homogeneous", FALSE,
		      //"GtkBox::spacing", 5,
		      //"GtkContainer::border_width", 10,
		      "GtkWidget::parent", main_vbox,
		      "GtkWidget::visible", TRUE,
		      NULL);
    gtk_widget_show(vbox);


    darea = gtk_drawing_area_new ();

    gtk_widget_set_usize (darea, 
			  100, 
			  100);
    gtk_container_add (GTK_CONTAINER (vbox), darea);

    gtk_signal_connect (GTK_OBJECT (darea),
			"expose_event",
			GTK_SIGNAL_FUNC (led7_expose_event),
			this);
    gtk_widget_set_events (darea, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
    gtk_signal_connect (GTK_OBJECT (darea),
			"button_press_event",
			GTK_SIGNAL_FUNC (cursor_event),
			NULL);

    gtk_widget_show (darea);

    set_widget(main_vbox);

    segment_gc=NULL;

    // The 'on' color is bright red
    led_segment_on_color.red = 0xc000;
    led_segment_on_color.green = 0x0000;
    led_segment_on_color.blue = 0x0000;

    gdk_color_alloc(gdk_colormap_get_system(), &led_segment_on_color);

    // The `off' color is dark red
    led_segment_off_color.red = 0x4000;
    led_segment_off_color.green = 0x0000;
    led_segment_off_color.blue = 0x0000;

    gdk_color_alloc(gdk_colormap_get_system(), &led_segment_off_color);

    // The background is black like my coffee
    led_background_color.red = 0x0000;
    led_background_color.green = 0x0000;
    led_background_color.blue = 0x0000;

    gdk_color_alloc(gdk_colormap_get_system(), &led_background_color);

//  }


}

//--------------------------------------------------------------

Led_7Segments::Led_7Segments(void)
{

  //cout << "7-segment led constructor\n";
  name_str = strdup("Led 7-segments");


  build_segments(100, 100);
  build_window();

  interface = new LED_Interface(this);
  get_interface().add_interface(interface);
}

Led_7Segments::~Led_7Segments(void)
{
  delete port;
}

//--------------------------------------------------------------
// create_iopin_map 
//
//  This is where the information for the Module's package is defined.
// Specifically, the I/O pins of the module are created.

void Led_7Segments::create_iopin_map(void)
{


  // Create an I/O port to which the I/O pins can interface
  //   The module I/O pins are treated in a similar manner to
  //   the pic I/O pins. Each pin has a unique pin number that
  //   describes it's position on the physical package. This
  //   pin can then be logically grouped with other pins to define
  //   an I/O port. 


  port = new Led_Port(8);
  port->value.put(0);

  // Here, we name the port `pin'. So in gpsim, we will reference
  //   the bit positions as U1.pin0, U1.pin1, ..., where U1 is the
  //   name of the logic gate (which is assigned by the user and
  //   obtained with the name() member function call).

  char *pin_name = (char*)name().c_str();   // Get the name of this logic gate
  if(pin_name) {
    port->new_name(pin_name);
  }
  else
    port->new_name("pin");


  // Define the physical package.
  //   The Package class, which is a parent of all of the modules,
  //   is responsible for allocating memory for the I/O pins.
  //
  //   The 7-segment LED has 8 pins

  create_pkg(8);

  // Position pins on left side of package
  float pos=0.0;
  const float dp = (float)(0.9999/7.0);
  package->set_pin_position(1,pos);pos+=dp;
  package->set_pin_position(2,pos);pos+=dp;
  package->set_pin_position(3,pos);pos+=dp;
  package->set_pin_position(4,pos);pos+=dp;
  package->set_pin_position(5,pos);pos+=dp;
  package->set_pin_position(6,pos);pos+=dp;
  package->set_pin_position(7,pos);pos+=dp;
  package->set_pin_position(8,pos);pos+=dp;

  // Define the I/O pins and assign them to the package.
  //   There are two things happening here. First, there is
  //   a new I/O pin that is being created. For the binary 
  //   indicator, both pins are inputs. The second thing is
  //   that the pins are "assigned" to the package. If we
  //   need to reference these newly created I/O pins (like
  //   below) then we can call the member function 'get_pin'.

  // Now, I'd normally put this is loop, but to be explicit...
  assign_pin(1, new Led_Input(port, 0,"cc"));  // cathode
  assign_pin(2, new Led_Input(port, 1,"seg0"));  // segment 0 (a)
  assign_pin(3, new Led_Input(port, 2,"seg1"));  // segment 1 (b)
  assign_pin(4, new Led_Input(port, 3,"seg2"));  // segment 2 (c)
  assign_pin(5, new Led_Input(port, 4,"seg3"));  // segment 3 (d)
  assign_pin(6, new Led_Input(port, 5,"seg4"));  // segment 4 (e)
  assign_pin(7, new Led_Input(port, 6,"seg5"));  // segment 5 (f)
  assign_pin(8, new Led_Input(port, 7,"seg6"));  // segment 6 (g)



  // Create an entry in the symbol table for the new I/O pins.
  // This is how the pins are accessed at the higher levels (like
  // in the CLI).

  // again, this could be looped (and even combined with the above)
  get_symbol_table().add_stimulus(get_pin(1));
  get_symbol_table().add_stimulus(get_pin(2));
  get_symbol_table().add_stimulus(get_pin(3));
  get_symbol_table().add_stimulus(get_pin(4));
  get_symbol_table().add_stimulus(get_pin(5));
  get_symbol_table().add_stimulus(get_pin(6));
  get_symbol_table().add_stimulus(get_pin(7));
  get_symbol_table().add_stimulus(get_pin(8));


}

//--------------------------------------------------------------
// construct

Module * Led_7Segments::construct(const char *_new_name=0)
{

//  cout << " 7-segment LED display constructor\n";

  Led_7Segments *l7sP = new Led_7Segments ;
  l7sP->new_name((char*)_new_name);
  l7sP->create_iopin_map();

  return l7sP;

}




//-------------------------------------------------------------
// Led (simple)
//-------------------------------------------------------------
void Led::update(void)
{
  update(darea, w_width,w_height);
}

void Led::update(  GtkWidget *widget,
		   guint new_width,
		   guint new_height)
{

  w_width = new_width;
  w_height = new_height;
  GdkDrawable *drawable = widget->window;

  if(gc==NULL)
  {
      gc = gdk_gc_new(darea->window);
      gdk_gc_set_line_attributes(gc,
				 5,
				 GDK_LINE_SOLID,
				 GDK_CAP_ROUND,
				 GDK_JOIN_ROUND);
      g_assert(gc!=NULL);
  }


  int state = port->get_value();

  gdk_gc_set_foreground(gc,&led_segment_off_color);
  gdk_draw_rectangle (drawable, gc,
		      TRUE,
		      0,
		      0,
		      w_width,
		      w_height);

    if(state)
    {
	gdk_gc_set_foreground(gc,&led_segment_on_color);
	gdk_draw_arc(drawable, gc,
		     TRUE,
		     0,
		     0,
		     w_width,
		     w_height,
                     0,64*360);
    }
}


static gint
led_expose_event (GtkWidget *widget,
		  GdkEvent  *event,
		  gpointer   user_data)
{


  Led *led;
  guint max_width;
  guint max_height;

  g_return_val_if_fail (widget != NULL, TRUE);
  g_return_val_if_fail (GTK_IS_DRAWING_AREA (widget), TRUE);

  // de-reference the user_data into an led object
  led = (Led *)user_data;

  max_width = widget->allocation.width;
  max_height = widget->allocation.height;

  led->update(widget,max_width,max_height);

  return TRUE;
}
void Led::build_window(void)
{
    GtkWidget *main_vbox;

    main_vbox = gtk_vbox_new (FALSE, 5);
    gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 0);

    darea = gtk_drawing_area_new ();

    w_height=20;
    w_width=20;

    gtk_widget_set_usize (darea, 
			  w_height,
			  w_width);
    gtk_signal_connect (GTK_OBJECT (darea),
			"expose_event",
			GTK_SIGNAL_FUNC (led_expose_event),
			this);
    gtk_widget_set_events (darea, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
    gtk_widget_show (darea);

    set_widget(darea);

    gc=NULL;

    // The 'on' color is bright red
    led_segment_on_color.red = 0xc000;
    led_segment_on_color.green = 0x0000;
    led_segment_on_color.blue = 0x0000;

    gdk_color_alloc(gdk_colormap_get_system(), &led_segment_on_color);

    // The `off' color is dark red
    led_segment_off_color.red = 0x4000;
    led_segment_off_color.green = 0x0000;
    led_segment_off_color.blue = 0x0000;

    gdk_color_alloc(gdk_colormap_get_system(), &led_segment_off_color);

}

//--------------------------------------------------------------

Led::Led(void)
{
  name_str = strdup("Led");

  build_window();

  interface = new LED_Interface(this);
  get_interface().add_interface(interface);
}

Led::~Led(void)
{
    //cout << "7-segment led destructor\n";

    //gpsim_unregister_interface(interface_id);
    //gpsim_clear_break(cbp);
    delete port;
}

//--------------------------------------------------------------
// create_iopin_map 
//
//  This is where the information for the Module's package is defined.
// Specifically, the I/O pins of the module are created.

void Led::create_iopin_map(void)
{


  // Create an I/O port to which the I/O pins can interface
  //   The module I/O pins are treated in a similar manner to
  //   the pic I/O pins. Each pin has a unique pin number that
  //   describes it's position on the physical package. This
  //   pin can then be logically grouped with other pins to define
  //   an I/O port. 


  port = new Led_Port(1);
  port->value.put(0);

  // Here, we name the port `pin'. So in gpsim, we will reference
  //   the bit positions as U1.pin0, U1.pin1, ..., where U1 is the
  //   name of the logic gate (which is assigned by the user and
  //   obtained with the name() member function call).

  char *pin_name = (char*)name().c_str();   // Get the name of this logic gate
  if(pin_name) {
    port->new_name(pin_name);
  }
  else
    port->new_name("pin");


  // Define the physical package.
  //   The Package class, which is a parent of all of the modules,
  //   is responsible for allocating memory for the I/O pins.
  //

  create_pkg(1);

  // Position pin on left side of package
  package->set_pin_position(1,0.5);

  // Define the I/O pins and assign them to the package.
  //   There are two things happening here. First, there is
  //   a new I/O pin that is being created. For the binary 
  //   indicator, both pins are inputs. The second thing is
  //   that the pins are "assigned" to the package. If we
  //   need to reference these newly created I/O pins (like
  //   below) then we can call the member function 'get_pin'.

  // Now, I'd normally put this is loop, but to be explicit...
  assign_pin(1, new Led_Input(port, 0,"in"));  // cathode



  // Create an entry in the symbol table for the new I/O pins.
  // This is how the pins are accessed at the higher levels (like
  // in the CLI).

  // again, this could be looped (and even combined with the above)
  get_symbol_table().add_stimulus(get_pin(1));

}

//--------------------------------------------------------------
// construct

Module * Led::construct(const char *_new_name=0)
{

//  cout << " 7-segment LED display constructor\n";

  Led *ledP = new Led;
  ledP->new_name((char*)_new_name);
  ledP->create_iopin_map();

  return ledP;

}

#endif //HAVE_GUI
