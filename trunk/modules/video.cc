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

/*

A PAL video module.

It makes use of two digital inputs to generate four video levels.

in1	in2	result
0	0	sync
0	1	black
1	0	grey (not implemented)
1	1	white

*/

#include <errno.h>
#include <stdlib.h>
#include <string>

#include <gtk/gtk.h>

#include "video.h"

//--------------------------------------------------------------
//   This class is a minor extension of a normal IO_input. I may
// remove it later, but for now it does serve a simple purpose.
// Specifically, this derivation will intercept when a stimulus
// is being changed. 
void Another_Input::put_node_state( int new_state)
{

  int current_state = state;

  IO_input::put_node_state(new_state);

  if(current_state ^ state) {

    if(video)
      video->update_state();
  }
}

static void gui_update(gpointer callback_data)
{
    Video *video;
    video = (Video*)callback_data;
    video->refresh();
}

static void expose(GtkWidget *widget,
		   GdkEventExpose *event,
		   Video *video)
{
    video->refresh();
}
/*************************************************************
*
*  Video class
*/

Video::Video(void)
{
  //cout << "Video base class constructor\n";
  sync_time=0;
  scanline=0;
  line_nr=0;
  memset(line,0x80,XRES);

  cpu = get_processor(1);
  
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size(GTK_WINDOW(window), XRES,YRES);
  gtk_window_set_title(GTK_WINDOW(window), "Video");
  da = gtk_drawing_area_new();
  gtk_signal_connect(GTK_OBJECT(da),
		     "expose_event",
		     (GtkSignalFunc) expose,
		     this);
  
  gtk_container_add(GTK_CONTAINER(window), da);
  gtk_widget_show_all(window);
  
  GdkColormap *colormap = gdk_colormap_get_system();
  
  gdk_color_parse("black", &black_color);
  gdk_color_parse("white", &white_color);
  gdk_color_parse("grey", &grey_color);

  gdk_colormap_alloc_color(colormap, &black_color, FALSE, TRUE);
  gdk_colormap_alloc_color(colormap, &white_color, FALSE, TRUE);
  gdk_colormap_alloc_color(colormap, &grey_color, FALSE, TRUE);
  
  black_gc = gdk_gc_new(window->window);
  gdk_gc_set_foreground(black_gc, &black_color);
  
  white_gc = gdk_gc_new(window->window);
  gdk_gc_set_foreground(white_gc, &white_color);

  grey_gc = gdk_gc_new(window->window);
  gdk_gc_set_foreground(grey_gc, &grey_color);

  pixmap = gdk_pixmap_new(window->window,
		  XRES,
		  YRES,
		  -1);
  gdk_draw_rectangle (pixmap,
		      da->style->bg_gc[GTK_WIDGET_STATE (da)],
		      TRUE,
		      0, 0,
		      XRES,
		      YRES);

  interface_id = gpsim_register_interface((gpointer)this);
  gpsim_register_simulation_has_stopped(interface_id, gui_update);


}

Video::~Video(void)
{

    //cout << "Video base class destructor\n";

    gtk_widget_destroy(window);
    gpsim_unregister_interface(interface_id);
    delete port;
}

//--------------------------------------------------------------
// create_iopin_map 
//
//  This is where the information for the Module's package is defined.
// Specifically, the I/O pins of the module are created.

void Video::create_iopin_map(void)
{
  int i;

  // Create an I/O port to which the I/O pins can interface
  //   The module I/O pins are treated in a similar manner to
  //   the pic I/O pins. Each pin has a unique pin number that
  //   describes it's position on the physical package. This
  //   pin can then be logically grouped with other pins to define
  //   an I/O port. 

  port = new IOPORT;
  port->value = 0;

  // Here, we name the port `pin'. So in gpsim, we will reference
  //   the bit positions as U1.pin0, U1.pin1, ..., where U1 is the
  //   name of the logic gate (which is assigned by the user and
  //   obtained with the name() member function call).

  char *pin_name = name();   // Get the name of this logic gate
  if(pin_name) {
    port->new_name(pin_name);
  }
  else
    port->new_name("pin");


  // Define the physical package.
  //   The Package class, which is a parent of all of the modules,
  //   is responsible for allocating memory for the I/O pins.
  //

  create_pkg(2);


  // Define the I/O pins and assign them to the package.
  //   There are two things happening here. First, there is
  //   a new I/O pin that is being created. For the binary 
  //   indicator, both pins are inputs. The second thing is
  //   that the pins are "assigned" to the package. If we
  //   need to reference these newly created I/O pins (like
  //   below) then we can call the member function 'get_pin'.

  Another_Input *vi;
  vi=new Another_Input(port, 0, "in1");
  vi->new_logic_gate(this);
  assign_pin(1, vi);
  vi=new Another_Input(port, 1, "in2");
  vi->new_logic_gate(this);
  assign_pin(2, vi);
  
  // Create an entry in the symbol table for the new I/O pins.
  // This is how the pins are accessed at the higher levels (like
  // in the CLI).

  for(i= 1; i<=number_of_pins; i++)
    symbol_table.add_stimulus(Package::get_pin(i));

}

//--------------------------------------------------------------
// construct

ExternalModule * Video::construct(char *new_name)
{

  //cout << " AND2Gate  construct\n";

  Video *video = new Video ;

  video->new_name(new_name);
  video->create_iopin_map();

  //cout << "AND2Gate should be constructed\n";

  return video;

}

void Video::copy_scanline_to_pixmap(void)
{
	int i, y;
	int vbl=0;
	int last=line[0];

	// Fill unfilled values
	for(i=1;i<XRES;i++)
	{
		if(line[i]&0x80) // Not written, use last written value
			line[i]=last;
		last=line[i];
	}

	// Draw
	for(i=1;i<XRES;i++)
	{
		if(line_nr<313)
			y=line_nr*2;
		else
			y=(line_nr-313)*2+1;
		if(line[i]>1)
		{
			gdk_draw_point(pixmap, white_gc, i, y);
		}
		else
		{
			gdk_draw_point(pixmap, black_gc, i, y);
		}
	}
	// Copy line to drawing area
}

int Video::cycles_to_us(int cycles)
{
	float ret;

	ret = cycles*4000000.0/cpu->frequency;
	return (int) ret;
}


int Video::us_to_cycles(int us)
{
	float ret;

	ret = us*cpu->frequency/4000000.0;
	return (int) ret;
}

void Video::refresh(void)
{
    gdk_draw_pixmap(da->window,
		    da->style->fg_gc[GTK_WIDGET_STATE (da)],
		    pixmap,
		    0,0,
		    0,0,
		    XRES,YRES);
}

void Video::update_state(void)
{
  unsigned int cycletime;
  int index;
  static int last_port_value=0;
  int val=port->value;
  static int shortsync_counter, last_shortsync_counter;

  cycletime = gpsim_get_cycles_lo(1); // FIXME 64 bits?
  if(sync_time>cycletime)
  {
      // Cycle counter rolled over.
      sync_time+=us_to_cycles(64); // 64 us = 1 line
      assert(sync_time<=cycletime);
  }

  // Index into line buffer. Calculated from sync_time.
  index=cycles_to_us((cycletime-sync_time)*(XRES/64));

  if(cycletime-sync_time>us_to_cycles(70))
  {
      // Long time with no sync pulses
      // Shouldn't happen?
      // If there was a long time since last sync, we jump over a line.
	  sync_time+=us_to_cycles(64); // 64 us = 1 line
	  memset(line,0x80,XRES); // clear line buffer
  }
  
  if(last_port_value!=0 && port->value==0) // Start of sync
  {
	  // Every 32 or 64us there should be a sync.
	  
	  sync_time=cycletime;
	  
	  // Start measure on negative flank, when we have lots in buffer.
	  if(index>XRES-XRES/5)
	  {
              // Have a full line
	      if(shortsync_counter>0)
	      {
                  // We are on first line in the frame.
		  if(shortsync_counter > last_shortsync_counter)
		  {
		      line_nr=6;

		      // Draw the last full image
		      refresh();

		      // Clear pixmap
		      gdk_draw_rectangle (pixmap,
					  da->style->bg_gc[GTK_WIDGET_STATE (da)],
					  TRUE,
					  0, 0,
					  XRES,
					  YRES);
		  }
		  else if(shortsync_counter < last_shortsync_counter)
		  {
		      line_nr=318;
		  }
                  else
		  {
		      puts("VSYNC error");
                      printf("%d, %d\n",shortsync_counter, last_shortsync_counter);
                      //bp.halt(); useful for debug
		  }
		  last_shortsync_counter=shortsync_counter;
		  shortsync_counter=0;

	      }
	      copy_scanline_to_pixmap(); // display last line
	      line_nr++;

	      if(line_nr>=YRES)
		  line_nr=0;
	      memset(line,0x80,XRES);
	      index=0;
	  }
	  else if(index>XRES/3 && index<XRES-XRES/3)
	  {
	      // Shortsync
              //printf("shortsync %d\n",index);
	      shortsync_counter++;
	  }
  }
  if(last_port_value==0 && port->value!=0) // End of sync
  {
	  int us_time;
	  
	  // Should be 4 us (0.000004*12000000
	  us_time = cycles_to_us(cycletime-sync_time);
/*	  if(us_time > 2 &&
	     us_time < 6)
	  {
		  // Valid HSYNC
	  }*/
		  
	  if(us_time > 25 && us_time < 35)
	  {
		  // vertical sync (FIXME, how should this be?)
		  
		  shortsync_counter=0;
	  }
  }

  if(index>=XRES)
	  index=XRES-1;
  
  line[index]=val;
  last_port_value=port->value;
}

