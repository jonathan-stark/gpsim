/*
   Copyright (C) 1998,1999,2000 T. Scott Dattalo
   Copyright (C)	2017	Roy R. Rankin

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


#ifndef __RAW_LCD_H__
#define __RAW_LCD_H__

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include <gtk/gtk.h>


  class RAW_LCD_Input;
  //------------------------------------------------------------------------
  // RAW_LCD base class

  class RAW_LCD_base
  {
  public:
    virtual ~RAW_LCD_base() {}

    virtual void build_window() = 0;
    virtual void update() = 0;

    unsigned int interface_seq_no;
  };


  //------------------------------------------------------------------------
  // 7-segment lcds

  // define a point
  typedef struct {
    double x;
    double y;
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

  class LCD_7Segments : public Module, public RAW_LCD_base
  {
  public:
    guint w_width;
    guint w_height;

    segment_pts seg_pts;

    GtkWidget *darea;

    LCD_7Segments(const char *);
    ~LCD_7Segments();  

    void build_segments( int w, int h);

    virtual void build_window();
    virtual void update();
    unsigned int getPinState();

    // Inheritances from the Package class
    virtual void create_iopin_map();

    // Inheritance from Module class
    const virtual char *type() { return ("lcd_7segments"); };
    static Module *construct(const char *new_name);
  private:
    static gboolean lcd7_expose_event(GtkWidget *widget, GdkEvent *event,
      gpointer user_data);

    //unsigned int m_segmentStates;
    RAW_LCD_Input *m_pins[8];
    int m_nPins;
    unsigned int segments;

  };

#endif //  __RAW_LCD_H__
