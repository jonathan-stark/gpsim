/*
   Copyright (C) 1998,1999,2000 T. Scott Dattalo

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


#ifndef __LED_H__
#define __LED_H__

#include <../src/stimuli.h>
#include <../src/ioports.h>
#include <../src/symbol.h>
#include <../src/trace.h>
#include <../src/interface.h>

#include <gtk/gtk.h>

// Create a few classes from which an LED may be constructed

class Led_Port : public IOPORT
{
public:

  virtual void trace_register_write(void);

  Led_Port (unsigned int _num_iopins=8);

};

// Create a class derived from the IO_input class that
// will allow us to intercept when the I/O input is being
// driven. (This isn't done for PIC I/O pins because the
// logic for handling I/O pin changes resides in the IOPORT
// class.)

class Led_Input : public IO_input
{
public:

  virtual void put_node_state( int new_state);

  Led_Input (IOPORT *i, unsigned int b, char *opt_name=NULL) : IO_input(i,b,opt_name) { };

};

// define a point
typedef struct {
   float x;
   float y;
} XfPoint;

#define MAX_PTS		6	/* max # of pts per segment polygon */
#define NUM_SEGS	7	/* number of segments in a digit */
/*
 * These constants give the bit positions for the segmask[]
 * digit masks.
 */
#define TOP		0
#define TOP_RIGHT	1
#define BOT_RIGHT	2
#define BOTTOM		3
#define BOT_LEFT	4
#define TOP_LEFT	5
#define MIDDLE		6
//#define DECIMAL_POINT   7

typedef XfPoint segment_pts[NUM_SEGS][MAX_PTS];

class Led_7Segments : public ExternalModule
{
public:

  /*
  struct {
    GdkPoint *p;  // Segments built from little polygons whose endpoint are here
    int n;        // Here's the number of endpoints.
  } segments[7];
  */

  struct {
    GdkPoint p[7];  // Segments 
  } segments[7];

  GdkPoint offset;
  int height,
    slant,
    segment_thickness;

  float sxw;

  float		angle;		// rise over run 
  float		width_factor;   // ratio of digit to segment width 
  float		small_ratio;	// ratio of small to large digits
  float		sec_gap;	// gap between normal digits and 
                                // seconds, as ratio to digit width 
  float		space_factor;   /* ratio of digit width to border sp.*/


  guint w_width;
  guint w_height;

  segment_pts seg_pts;

  GtkWidget *darea;
  GdkGC *segment_gc;
  //  GdkGC led_segment_off_gc;

  GdkColor led_segment_on_color;
  GdkColor led_segment_off_color;
  GdkColor led_background_color;

  gpointer cbp;  // cycle break point pointer (need to delete in destructor)

  Led_Port  *port;

  Led_7Segments(void);
  ~Led_7Segments(void);  


  void build_segments( int w, int h);
  void build_window( void );
  void update(void);
  void update( GtkWidget *drawable,   
	       guint max_width,
	       guint max_height);

  // Inheritances from the Package class
  virtual void create_iopin_map(void);

  // Inheritance from Module class
  const virtual char *type(void) { return ("led_7segments"); };
  static ExternalModule *construct(char *new_name);


};

#endif //  __LED_H__
