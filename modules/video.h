/*
   Copyright (C) 2003 Ralf Forsberg

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
 * Create a class derived from the IO_input class that
 * will allow us to intercept when the I/O input is being
 * driven. (This isn't done for PIC I/O pins because the
 * logic for handling I/O pin changes resides in the IOPORT
 * class.)
 */


class Another_Input : public IOPIN
{
private:
  Video *video;
public:

  virtual void put_digital_state( bool new_state);

  Another_Input (Video *v, IOPORT *i, unsigned int b, char *opt_name=NULL) 
    : IOPIN(i,b,opt_name) , video(v)
    { 
    }

};

#define XRES 320
#define YRES 625

class Video : public Module
{
public:

  IOPORT  *port;
  guint64 sync_time; // gpsim cycle counter at last H-sync
  int scanline;
  unsigned char line[XRES]; // buffer for one line
  Processor *cpu;

  GtkWidget *window;
  GtkWidget *da;
  GdkGC *white_gc, *black_gc, *grey_gc;
  GdkPixmap *pixmap;
  int line_nr;

  GdkColor black_color, grey_color, white_color;
  
  Video(void);
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

