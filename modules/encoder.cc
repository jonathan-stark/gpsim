/*
   Copyright (C) 2004 Chris Emerson
   (Based on the push_button Copyright (C) 2002 Carlos Ghirardelli)

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

  encoder.cc


*/

#include <time.h>
#include <stdio.h>

#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <stdlib.h>
#include <string>
#include <iostream>

#include "../config.h"    // get the definition for HAVE_GUI
#ifdef HAVE_GUI

#include <gtk/gtk.h>
#include "../src/gpsim_time.h"

#include "encoder.h"

#define PIN_A	(0x01)
#define PIN_B	(0x02)

//--------------------------------------------------------------
// create_iopin_map 
//
//  This is where the information for the Module's package is defined.
// Specifically, the I/O pins of the module are created.

void Encoder::create_iopin_map(void)
{


  // Create an I/O port to which the I/O pins can interface
  //   The module I/O pins are treated in a similar manner to
  //   the pic I/O pins. Each pin has a unique pin number that
  //   describes it's position on the physical package. This
  //   pin can then be logically grouped with other pins to define
  //   an I/O port.


  enc_port = new IOPORT(2);
  enc_port->value = 0;
  enc_port->valid_iopins = 0x03;


  // Here, we name the port `pin'. So in gpsim, we will reference
  //   the bit positions as U1.pin0, U1.pin1, ..., where U1 is the
  //   name of the logic gate (which is assigned by the user and
  //   obtained with the name() member function call).

  char *pin_name = (char*)name().c_str();   // Get the name of this switch
  if(pin_name) {
    enc_port->new_name(pin_name);
  }



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

  assign_pin(1, new IO_bi_directional(enc_port, 0,"a"));
  package->set_pin_position(1,0.0);
  assign_pin(2, new IO_bi_directional(enc_port, 1,"b"));
  package->set_pin_position(2,0.9999);

  // Create an entry in the symbol table for the new I/O pins.
  // This is how the pins are accessed at the higher levels (like
  // in the CLI).

  a_pin = get_pin(1);
  if(a_pin)
    {
      symbol_table.add_stimulus(a_pin);
      a_pin->update_direction(1);
      if(a_pin->snode)
	a_pin->snode->update(0);
      //enc_port->attach_iopin(a_pin, 0);
    }
  b_pin = get_pin(2);
  if(b_pin)
    {
      symbol_table.add_stimulus(b_pin);
      b_pin->update_direction(1);
      if(b_pin->snode)
	b_pin->snode->update(0);
      //enc_port->attach_iopin(b_pin, 1);
    }
}

//--------------------------------------------------------------
// GUI
static void
cw_cb (GtkButton *button, Encoder *enc)

{
    enc->send_cw();
}

static void
ccw_cb (GtkButton *button, Encoder *enc)

{
    enc->send_ccw();
}

void Encoder::create_widget(Encoder *enc)
{

  GtkWidget	*box1;
  GtkWidget	*buttonl, *buttonr;
	
  box1 = gtk_hbox_new (FALSE, 0);

  buttonl = gtk_button_new_with_label ("ccw");
  buttonr = gtk_button_new_with_label (" cw");
  gtk_container_set_border_width (GTK_CONTAINER (buttonl), 5);
  gtk_container_set_border_width (GTK_CONTAINER (buttonr), 5);

  gtk_signal_connect (GTK_OBJECT (buttonl), "pressed",			    
		      GTK_SIGNAL_FUNC (ccw_cb), (gpointer)enc);
  gtk_signal_connect (GTK_OBJECT (buttonr), "pressed",			    
		      GTK_SIGNAL_FUNC (cw_cb), (gpointer)enc);
	
		
  gtk_widget_show(buttonl);
  gtk_widget_show(buttonr);
  gtk_box_pack_start (GTK_BOX (box1), buttonl, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (box1), buttonr, FALSE, FALSE, 0);
	
	
  // Tell gpsim which widget to use in breadboard.
  enc->set_widget(box1);
}

//--------------------------------------------------------------
// construct
Module * Encoder::construct(const char *new_name=NULL)
{

  Encoder *enc_p = new Encoder ;
  enc_p->new_name((char*)new_name);
  enc_p->create_iopin_map();

  enc_p->create_widget(enc_p);

  return enc_p;

}

Encoder::Encoder(void)
    : rs(rot_detent)
{
  name_str = strdup("Encoder");
}

Encoder::~Encoder(void)
{
  delete enc_port;
}

void
Encoder::send_cw(void)
{
    switch (rs) {
	case rot_detent:
	    rs = rot_moving_cw;
	    toggle_a(); /* A toggles before B going clockwise */
	    schedule_tick();
	    break;
	default:
	    // XXX: ought to do something here as well
	    break;
    }
}

void
Encoder::send_ccw(void)
{
    switch (rs) {
	case rot_detent:
	    rs = rot_moving_ccw;
	    toggle_b();
	    schedule_tick();
	    break;
	default:
	    // XXX: ought to do something here as well
	    break;
    }
}

void
Encoder::toggle_a()
{
    enc_port->put_value(enc_port->value^PIN_A);
}

void
Encoder::toggle_b()
{
    enc_port->put_value(enc_port->value^PIN_B);
}

void
Encoder::schedule_tick()
{
    /* XXX: make the time delay configurable */
    if (!cycles.set_break_delta(100, this)) {
	std::cerr << "Encoder: error setting breakpoint." << std::endl;
    }
}

void
Encoder::callback()
{
    switch (rs) {
	case rot_detent:
	    assert(false);
	    break;
	case rot_moving_cw:
	    toggle_b();
	    assert(!(enc_port->value & PIN_A) ==
		   !(enc_port->value & PIN_B));
	    rs = rot_detent;
	    break;
	case rot_moving_ccw:
	    toggle_a();
	    assert(!(enc_port->value & PIN_A) ==
		   !(enc_port->value & PIN_B));
	    rs = rot_detent;
	    break;
	default:
	    abort();
    }
}

#endif // HAVE_GUI
