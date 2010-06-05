/*
   Copyright (C) 2003 Ralf Forsberg

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


#ifndef __VIDEO_H__
#define __VIDEO_H__

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include "../src/stimuli.h"
#include "../src/ioports.h"
#include "../src/symbol.h"
#include "../src/modules.h"

#include <gtk/gtk.h>

class Video;
class Video_Interface;

/*********************************************************
 *
 * Create a class derived from the IOPIN class that
 * will allow us to intercept when the I/O input is being
 * driven. (This isn't done for PIC I/O pins because the
 * logic for handling I/O pin changes resides in the ???
 * class.)
 */


class IOPIN_Monitor : public IOPIN
{
private:
  Video *video;
public:

  virtual void setDrivenState( bool new_state);

  IOPIN_Monitor (Video *v, const char *opt_name) 
    : IOPIN(opt_name) , video(v)
    { 
    }

};


#define XRES 640
#define YRES 625

class Video : public Module
{
public:

  IOPIN * sync_pin;
  IOPIN * lume_pin;
  guint64 sync_time; // gpsim cycle counter at last H-sync
  int scanline;
  unsigned char line[XRES]; // buffer for one line
  unsigned char shadow[XRES][YRES]; // pixmap mirror
  Processor *cpu;

  GtkWidget *window;
  GtkWidget *da;
  GdkGC *white_gc, *black_gc, *grey_gc;
  GdkPixmap *pixmap;
  int line_nr;
  int last_line_nr;

  GdkColor black_color, grey_color, white_color;
  
  Video(const char *);
  ~Video(void);

  // Inheritances from the Package class
  virtual void create_iopin_map(void);

  virtual void update_state(void);
  virtual int get_num_of_pins(void) {return 2;};
  void copy_scanline_to_pixmap(void);
  int check_for_vrt1(void);
  int check_for_vrt2(void);
  guint64 cycles_to_us(guint64 cycles);
  guint64 us_to_cycles(guint64 cycles);
  void refresh(void);
  static Module *construct(const char *new_name);

private:
    Video_Interface *interface;

};

#endif //  __VIDEO_H__

