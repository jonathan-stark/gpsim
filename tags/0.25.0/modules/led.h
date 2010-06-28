/*
   Copyright (C) 1998,1999,2000 T. Scott Dattalo

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


#ifndef __LED_H__
#define __LED_H__

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include "../src/stimuli.h"
#include "../src/ioports.h"
#include "../src/symbol.h"
#include "../src/trace.h"
#include "../src/interface.h"

#include <gtk/gtk.h>


namespace Leds {

  class LED_Interface;  // Defined in led.cc
  class Led_Input;

  enum Colors
  {
	RED = 0,
	ORANGE,
	GREEN,
	YELLOW,
	BLUE		// Must be last
  };
	

  //------------------------------------------------------------------------
  // LED base class

  class Led_base //: public Module
  {
  public:
    virtual ~Led_base()
    {
    }

    virtual void build_window() = 0;
    virtual void update() = 0;
    virtual void update( GtkWidget *drawable,   
                         guint max_width,
                         guint max_height) = 0;
    LED_Interface *led_interface;
  };


  //------------------------------------------------------------------------
  // 7-segment leds

  // define a point
  typedef struct {
    float x;
    float y;
  } XfPoint;

#define MAX_PTS         6       /* max # of pts per segment polygon */
#define NUM_SEGS        7       /* number of segments in a digit */
  /*
   * These constants give the bit positions for the segmask[]
   * digit masks.
   */
#define TOP             0
#define TOP_RIGHT       1
#define BOT_RIGHT       2
#define BOTTOM          3
#define BOT_LEFT        4
#define TOP_LEFT        5
#define MIDDLE          6
  //#define DECIMAL_POINT   7

  typedef XfPoint segment_pts[NUM_SEGS][MAX_PTS];

  class Led_7Segments : public Module, public Led_base, public TriggerObject
  {
  public:

    struct {
      GdkPoint p[7];  // Segments 
    } segments[7];

    GdkPoint offset;
    int height,
      slant,
      segment_thickness;

    float sxw;

    float               angle;          // rise over run 
    float               width_factor;   // ratio of digit to segment width 
    float               small_ratio;    // ratio of small to large digits
    float               sec_gap;        // gap between normal digits and 
    // seconds, as ratio to digit width 
    float               space_factor;   /* ratio of digit width to border sp.*/


    guint w_width;
    guint w_height;

    segment_pts seg_pts;

    GtkWidget *darea;
    GdkGC *segment_gc;

    GdkColor led_segment_on_color;
    GdkColor led_segment_off_color;
    GdkColor led_background_color;

    Led_7Segments(const char *);
    ~Led_7Segments();  


    void build_segments( int w, int h);

    virtual void callback();
    virtual void build_window();
    virtual void update();
    virtual void update( GtkWidget *drawable,   
                         guint max_width,
                         guint max_height);
    unsigned int getPinState();

    // Inheritances from the Package class
    virtual void create_iopin_map();

    // Inheritance from Module class
    const virtual char *type() { return ("led_7segments"); };
    static Module *construct(const char *new_name);
  private:
    //unsigned int m_segmentStates;
    Led_Input **m_pins;
    int m_nPins;

  };

  //------------------------------------------------------------------------
  // Simple LED
  //
  class ColorAttribute;

  class Led: public Module, public Led_base, public TriggerObject
  {
  public:

    GtkWidget *darea;
    GdkGC *gc;
    GdkColor led_on_color[BLUE+1];
    GdkColor led_segment_off_color;
    int w_width, w_height;

    gpointer cbp;  // cycle break point pointer (need to delete in destructor)

    Led(const char *);
    ~Led();

    virtual void callback();
    virtual void build_window();
    virtual void update();
    virtual void update( GtkWidget *drawable,   
                         guint max_width,
                         guint max_height);

    // Inheritances from the Package class
    virtual void create_iopin_map();

    // Inheritance from Module class
    const virtual char *type() { return ("led"); };
    static Module *construct(const char *new_name);
    Colors get_on_color() { return on_color; }
    void set_on_color(Colors color);
    

  private:
    Led_Input *m_pin;
    Colors on_color;
    ColorAttribute *m_colorAttribute;
  };

} // end of namespace Led
#endif //  __LED_H__
