/*
   Copyright (C) 1998,1999,2000,2001 T. Scott Dattalo

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

  resistor.cc

  This is gpsim's resistor component.

  resistor
  pullup
  pulldown

*/


/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include <errno.h>
#include <stdlib.h>
#include <string>

#include "../config.h"

#ifdef HAVE_GUI
#include <gtk/gtk.h>
#endif

#include "resistor.h"
#include "../src/stimuli.h"
#include "../src/ioports.h"
#include "../src/symbol.h"
#include "../src/value.h"


class ResistanceAttribute : public Float {

public:
  PullupResistor *pur;


  ResistanceAttribute(PullupResistor *ppur) 
    : Float(0.0)
  {

    pur = ppur;
    if(!pur)
      return;

    new_name("resistance");

    Float::set(pur->res.get_Zth());
  }


  void set(double r) {

    Float::set(r);

    if(!pur)
      return;

    pur->res.set_Zth(r);

  };


  void set(int r) {
    set(double(r));
  };
};


//--------------------------------------------------------------
void Resistor_IOPORT::trace_register_write(void)
{

  //get_trace().module1(value.get());
  get_trace().raw(value.get());
}

//--------------------------------------------------------------
Resistor::Resistor(void)
{

  name_str = strdup("Resistor");
}

Resistor::~Resistor(void)
{
  delete port;
}

//--------------------------------------------------------------
// create_iopin_map 
//
//  This is where the information for the Module's package is defined.
// Specifically, the I/O pins of the module are created.

void Resistor::create_iopin_map(void)
{


  // Create an I/O port to which the I/O pins can interface
  //   The module I/O pins are treated in a similar manner to
  //   the pic I/O pins. Each pin has a unique pin number that
  //   describes it's position on the physical package. This
  //   pin can then be logically grouped with other pins to define
  //   an I/O port. 

  port = new Resistor_IOPORT;
  port->new_name("res_port");

  // Define the physical package.
  //   The Package class, which is a parent of all of the modules,
  //   is responsible for allocating memory for the I/O pins.
  //
  //   The Resistor has only two pins.

  create_pkg(2);


  // Define the I/O pins and assign them to the package.
  //   There are two things happening here. First, there is
  //   a new I/O pin that is being created.The second thing is
  //   that the pins are "assigned" to the package. If we
  //   need to reference these newly created I/O pins (like
  //   below) then we can call the member function 'get_pin'.


  assign_pin(1, new IO_bi_directional(port, 0));
  assign_pin(2, new IO_bi_directional(port, 1));


  // Create an entry in the symbol table for the new I/O pins.
  // This is how the pins are accessed at the higher levels (like
  // in the CLI).

  get_symbol_table().add_stimulus(get_pin(1));
  get_symbol_table().add_stimulus(get_pin(2));

}

//--------------------------------------------------------------
// construct

Module * Resistor::construct(const char *_new_name)
{

  Resistor *resP = new Resistor;

  resP->new_name((char*)_new_name);
  resP->create_iopin_map();


  return resP;

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------

void PullupResistor::set_attribute(char *attr, char *val)
{
  /*
  cout << " Pull up resistor " << name_str << " update attribute\n";
  if(attr) {
    cout << "(attr = " << attr;
    if(val)
      cout << " , val = " << val;
    cout << ")\n";

  }
  */
  Module::set_attribute(attr,val);

}
//--------------------------------------------------------------
// PullupResistor::create_iopin_map 
//

void PullupResistor::create_iopin_map(void)
{

  //   The PullupResistor has only one pin.

  create_pkg(1);


  // Define the I/O pins and assign them to the package.
  //   There are two things happening here. First, there is
  //   a new I/O pin that is being created.The second thing is
  //   that the pins are "assigned" to the package. If we
  //   need to reference these newly created I/O pins (like
  //   below) then we can call the member function 'get_pin'.


  assign_pin(1, &res);


  // Create an entry in the symbol table for the new I/O pins.
  // This is how the pins are accessed at the higher levels (like
  // in the CLI).
  IOPIN *iop = get_pin(1);
  /*
  if(iop) {
    cout << "pullup resistor pin name: "<<iop->name() << '\n';
    cout << "voltage " << iop->get_Vth() << '\n';
  }
  */
  get_symbol_table().add_stimulus(get_pin(1));

}



//--------------------------------------------------------------

Module * PullupResistor::pu_construct(const char *_new_name)
{

  PullupResistor *pur = new PullupResistor(_new_name);

  if(_new_name) {
    pur->new_name((char*)_new_name);
    pur->res.new_name((char*)_new_name);
  }

  pur->create_iopin_map();

  pur->res.set_Vth(5.0);
  return pur;
}

//--------------------------------------------------------------
Module * PullupResistor::pd_construct(const char *_new_name)
{

  PullupResistor *pur = new PullupResistor(_new_name);

  if(_new_name) {
    pur->new_name((char*)_new_name);
    pur->res.new_name((char*)_new_name);
  }
  pur->create_iopin_map();

  pur->res.set_Vth(0);

  return pur;

}

//--------------------------------------------------------------
PullupResistor::PullupResistor(const char *init_name)
{

  ResistanceAttribute *attr;

  // Create the resistor:
  res.set_Zth(10e3);

  attr = new ResistanceAttribute(this);
  add_attribute(attr);

  new_name((char*)init_name);

#ifdef MANAGING_GUI
  pu_window = NULL;

  //if(use_gui) {
  if(1) {
    build_window();
    cout << "Creating resistor window\n";
  }
#endif
}

PullupResistor::~PullupResistor()
{

}

#ifdef MANAGING_GUI

static void pu_cb(GtkWidget *button, gpointer pur_class)
{
  PullupResistor *pur = (PullupResistor *)pur_class;
}

void PullupResistor::build_window(void)
{
  GtkWidget *buttonbox;
  GtkWidget *button;

  pu_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  buttonbox = gtk_hbox_new(FALSE,0);
  gtk_container_add (GTK_CONTAINER (pu_window), buttonbox);

  gtk_container_set_border_width (GTK_CONTAINER (buttonbox), 1);
      
  button = gtk_button_new_with_label (name().c_str());
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     (GtkSignalFunc) pu_cb, (gpointer)this);
  gtk_box_pack_start (GTK_BOX (buttonbox), button, TRUE, TRUE, 0);

  gtk_widget_show_all (pu_window);

}

#endif
