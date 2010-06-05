/*
   Copyright (C) 2000 T. Scott Dattalo

This file is part of the libgpsim_modules library of gpsim

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see 
<http://www.gnu.org/licenses/lgpl-2.1.html>.
*/

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
#include <typeinfo>

#include "../config.h"    // get the definition for HAVE_GUI

#ifdef HAVE_GUI
#include <gtk/gtk.h>
#include <math.h>

#include "../src/gpsim_interface.h"
#include "../src/gpsim_time.h"

#include "led.h"
#include "../src/packages.h"

namespace Leds {

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

    virtual void SimulationHasStopped (gpointer object)
    {
      GuiUpdate(object);
    }
    virtual void GuiUpdate  (gpointer object)
    {
      if(led)
        led->update();
      /* {
        int portval = led->port->get_value();
        if(lastport != portval) {
          lastport=portval;
          led->update();
        }

      }
      */
    }


    LED_Interface(Led_base *_led) : Interface((gpointer *) _led)
    {
      led = _led;
      lastport = -1;
    }

  };


  class Led_Input : public IOPIN
  {
  public:
    Led_Input(const char *n, Led_base *pParent);

    virtual void setDrivenState(bool);

  private:
    Led_base *m_pParent;
  };


  //------------------------------------------------------------------------
  //
  Led_Input::Led_Input(const char *n, Led_base *pParent)
    : IOPIN(n), m_pParent(pParent)
  {
  }

  void Led_Input::setDrivenState(bool bNewState)
  {
    IOPIN::setDrivenState(bNewState);
  }

  //------------------------------------------------------------------------
  void Led_7Segments::update()
  {
    update(darea, w_width,w_height);
  }

  void Led_7Segments::callback()
  {

    get_cycles().set_break_delta( get_interface().get_update_rate()+1, this);
    update();

  }

  void Led_7Segments::update(  GtkWidget *widget,
                               guint new_width,
                               guint new_height)
  {

    guint i;

    if(!get_interface().bUsingGUI())
      return;

    w_width = new_width;
    w_height = new_height;
    GdkDrawable *drawable = widget->window;

    if(!GTK_WIDGET_REALIZED(widget))
      return;

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
    int segment_states = getPinState();

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

  void Led_7Segments::build_window()
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
                          110);
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

    gdk_colormap_alloc_color(gdk_colormap_get_system(), &led_segment_on_color, FALSE, TRUE);

    // The `off' color is dark red
    led_segment_off_color.red = 0x4000;
    led_segment_off_color.green = 0x0000;
    led_segment_off_color.blue = 0x0000;

    gdk_colormap_alloc_color(gdk_colormap_get_system(), &led_segment_off_color, FALSE, TRUE);

    // The background is black like my coffee
    led_background_color.red = 0x0000;
    led_background_color.green = 0x0000;
    led_background_color.blue = 0x0000;

    gdk_colormap_alloc_color(gdk_colormap_get_system(), &led_background_color, FALSE, TRUE);

    //  }


  }

  //--------------------------------------------------------------

  Led_7Segments::Led_7Segments(const char *name) : Module(name, "7 Segment LED")
  {

    if(get_interface().bUsingGUI()) {
      build_segments(100, 110);
      build_window();
    }

    led_interface = new LED_Interface(this);
    get_interface().add_interface(led_interface);
    callback();
    create_iopin_map();
  }

  Led_7Segments::~Led_7Segments()
  {
    /* Deleted elsewhere RRR
    for (int i=0; i<8; i++)
      delete m_pins[i];
    delete [] m_pins;
   */
  }

  //--------------------------------------------------------------
  // create_iopin_map
  //
  //  This is where the information for the Module's package is defined.
  // Specifically, the I/O pins of the module are created.

  void Led_7Segments::create_iopin_map()
  {

    // Define the physical package.
    //   The Package class, which is a parent of all of the modules,
    //   is responsible for allocating memory for the I/O pins.
    //
    //   The 7-segment LED has 8 pins

    create_pkg(8);
    m_pins = new Led_Input *[8];

    float ypos = 6.0;
    for (int i = 1; i <= 8; i++)
    {
        package->setPinGeometry(i, 0.0, ypos, 0, false);
        ypos += 12.;
    }

    // Here, we create and name the I/O pins. In gpsim, we will reference
    //   the bit positions as LED.seg0, LED.seg1, ..., where LED is the
    //   user-assigned name of the 7-segment LED

    m_pins[0] = new Led_Input( (name() + ".cc").c_str(), this);
    int i;
    char ch;
    for (ch='0',i = 1; i<8; i++,ch++)
      m_pins[i] = new Led_Input((name() + ".seg"+ch).c_str(), this);

    for (i=0; i<8; i++)
      assign_pin(i+1,m_pins[i]);

    //initializeAttributes();

  }

  //--------------------------------------------------------------
  unsigned int Led_7Segments::getPinState()
  {
    unsigned int s=0;
    for (int i=0; i<8; i++)
      s = (s>>1) | (m_pins[i]->getDrivenState() ? 0x80 : 0);
    return s;

  }

  //--------------------------------------------------------------
  // construct

  Module * Led_7Segments::construct(const char *_new_name=0)
  {

    Led_7Segments *l7sP = new Led_7Segments(_new_name) ;

    return l7sP;

  }


class ColorAttribute : public Value
{
  public:
        ColorAttribute(Led *_led) :
                Value("color", "On color of LED"), m_led(_led)
        {
        }

        virtual void get(char *return_str, int len);
        virtual void set(const char *buffer, int buf_size = 0);
        virtual void set(Value *v);
        virtual bool Parse(const char *pValue, Colors &bValue);

  private:
        Led *m_led;
};

void ColorAttribute::set(Value *v)
{
  if ( typeid(*v) == typeid(String))
  {
     char buff[20];

     v->get((char *)buff, sizeof(buff));
     set(buff);
  }
  else
    throw new TypeMismatch(string("set "), "ColorAttribute", v->showType());
}

void ColorAttribute::set(const char *buffer, int len)
{
  if(buffer) {
    Colors color;

    if(Parse(buffer, color)) {
      m_led->set_on_color(color);
    }
    else
    {
        cout << "ColorAttribute::set " << buffer << " unknown color\n";
   }
  }

}
void ColorAttribute::get(char *return_str, int len)
{
  if(return_str) {

    switch(m_led->get_on_color())
    {
    case RED:
        strncpy(return_str, "red", len);
        break;

    case ORANGE:
        strncpy(return_str, "orange", len);
        break;

    case GREEN:
        strncpy(return_str, "green", len);
        break;

    case YELLOW:
        strncpy(return_str, "yellow", len);
        break;

    case BLUE:
        strncpy(return_str, "blue", len);
        break;

    }
  }
}

bool ColorAttribute::Parse(const char *pValue, Colors &bValue)
{


    if(strncmp("red", pValue, sizeof("red")) == 0)
    {
        bValue = RED;
        return true;
    }
    else if (strncmp("orange", pValue, sizeof("orange")) == 0)
    {
        bValue = ORANGE;
        return true;
    }
    else if (strncmp("green", pValue, sizeof("green")) == 0)
    {
        bValue = GREEN;
        return true;
    }
    else if (strncmp("yellow", pValue, sizeof("yellow")) == 0)
    {
        bValue = YELLOW;
        return true;
    }
    else if (strncmp("blue", pValue, sizeof("blue")) == 0)
    {
        bValue = BLUE;
        return true;
    }
    return false;
}

  //-------------------------------------------------------------
  // Led (simple)
  //-------------------------------------------------------------
  void Led::update()
  {
    update(darea, w_width,w_height);
  }

  void Led::callback()
  {

    get_cycles().set_break_delta( get_interface().get_update_rate()+1, this);
    update();

  }
  void Led::update(  GtkWidget *widget,
                     guint new_width,
                     guint new_height)
  {
    if(!get_interface().bUsingGUI())
      return;

    w_width = new_width;
    w_height = new_height;
    GdkDrawable *drawable = widget->window;

    if(!GTK_WIDGET_REALIZED(widget))
      return;

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


    gdk_gc_set_foreground(gc,&led_segment_off_color);
    gdk_draw_rectangle (drawable, gc,
                        TRUE,
                        0,
                        0,
                        w_width,
                        w_height);

    if(m_pin->getDrivenState()) {
        gdk_gc_set_foreground(gc,&led_on_color[on_color]);
        gdk_draw_arc(drawable, gc,
                   TRUE,
                   0,
                   0,
                   w_width,
                   w_height,
                   0,64*360);
    }
  }

  void Led::set_on_color(Colors color)
  {
        if (color != on_color)
        {
            on_color = color;
            if (get_interface().bUsingGUI() )
            {
                if (!led_on_color[color].pixel) // color not allocated
                        gdk_colormap_alloc_color(gdk_colormap_get_system(),
                                            &led_on_color[color], FALSE, TRUE);
                update();
            }
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

  void Led::build_window()
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

    // The default 'on' color is bright red

    for(int i = RED; i <= BLUE; i++ )
        led_on_color[i].pixel = 0;

    gdk_color_parse("red3", &led_on_color[RED]);
    gdk_color_parse("orange", &led_on_color[ORANGE]);
    gdk_color_parse("green", &led_on_color[GREEN]);
    gdk_color_parse("yellow", &led_on_color[YELLOW]);
    gdk_color_parse("blue", &led_on_color[BLUE]);

    GdkColormap *colormap = gdk_colormap_get_system();

    for(int i = RED; i <= BLUE; i++ )
        gdk_colormap_alloc_color(colormap, &led_on_color[i], FALSE, TRUE);

    // The `off' color is dark red
    led_segment_off_color.red = 0x4000;
    led_segment_off_color.green = 0x0000;
    led_segment_off_color.blue = 0x0000;

    gdk_colormap_alloc_color(colormap, &led_segment_off_color, FALSE, TRUE);
  }

  //--------------------------------------------------------------

  Led::Led(const char *name) : Module(name, "Simple LED")
  {

    create_iopin_map();
    if(get_interface().bUsingGUI())
      build_window();


    on_color = RED;
    m_colorAttribute = new ColorAttribute(this);
    addSymbol(m_colorAttribute);
    led_interface = new LED_Interface(this);
    get_interface().add_interface(led_interface);
    callback();
  }

  Led::~Led()
  {

    delete m_colorAttribute;
    /* deleted elsewhere RRR
    delete m_pin;
    */
  }

  //--------------------------------------------------------------
  // create_iopin_map
  //
  //  This is where the information for the Module's package is defined.
  // Specifically, the I/O pins of the module are created.

  void Led::create_iopin_map()
  {


    create_pkg(1);

    // Position pin on left side of package
    package->set_pin_position(1,0.5);

    // Define the LED Cathode. (The anode is implicitly tied to VCC)

    m_pin = new Led_Input((name() + ".in").c_str(), this);
    assign_pin(1, m_pin);
    //initializeAttributes();
  }

  //--------------------------------------------------------------
  // construct

  Module * Led::construct(const char *_new_name=0)
  {

    Led *ledP = new Led(_new_name);
    //ledP->new_name(_new_name);

    return ledP;

  }

}
#endif //HAVE_GUI
