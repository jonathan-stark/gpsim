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

  lcd.cc

  This is a fairly complete/complicated gpsim module that simulates
  an LCD display. This version is currently hardcoded for the Hitachi
  style 2 row by 20 column display. The font is also hardcoded to 5 by
  7 pixels.

  Hardware simulation:

  The Hitachi displays commonly have a 14pin interface:

  8 data lines
  3 control lines (E,RS,R/W)
  pwr
  gnd
  contrast

  This version only supports the 8 data and 3 control lines.

  Software simulation:

  This software uses an event driven behavior model to simulate
  the LCD display. This means that when one of the I/O lines toggle
  the module will be notified and will respond to the event. A state
  machine is used to control the behavior. 

*/


#include <errno.h>
#include <stdlib.h>
#include <string>
#include <iostream>

#include <gtk/gtk.h>

#include "lcd.h"
pic_processor *gpsim_get_active_cpu(void);
void  gpsim_set_break_delta(guint64 delta, BreakCallBack *f=NULL);
void gpsim_clear_break(gpointer b);





//------------------------------------------------------------------------
//
// LCD interface to the simulator
//
LCD_Interface::LCD_Interface(LcdDisplay *_lcd) : Interface ( (gpointer *) _lcd)
{

  lcd = _lcd;

}

//--------------------------------------------------
// SimulationHasStopped (gpointer)
//
// gpsim will call this function when the simulation
// has halt (e.g. a break point was hit.)

void LCD_Interface::SimulationHasStopped (gpointer)
{
  if(lcd)
    ((LcdDisplay *)lcd)->update();
}

void LCD_Interface::GuiUpdate (gpointer)
{
  if(lcd)
    ((LcdDisplay *)lcd)->update();
}

//--------------------------------------------------------------
// LcdPort class
//
// The LcdPort class is derived from the gpsim class "IOPORT".
// As such, it inherits all of the IOPROT's functionality (like
// tracing, stimulus interface, etc.). The LcdPort class extends
// IOPORT by redirecting changes to the LCD state machine.

Lcd_Port::Lcd_Port (unsigned int _num_iopins) : IOPORT(_num_iopins)
{

}
void Lcd_Port::setbit(unsigned int bit_number, bool new_value)
{

  int bit_mask = 1<<bit_number;

  if( ((bit_mask & value) != 0) ^ (new_value==1))
    {
      if(lcd && lcd->debug)
	cout << " Lcd+Port::set_bit bit changed due to a stimulus. new_value = " << new_value <<'\n';

      value ^= bit_mask;

      assert_event();
      trace_register_write();
    }
  //else cout <<  " IOPORT::set_bit bit did not change\n";

}

void Lcd_Port::assert_event(void)
{
  cout << __FUNCTION__ << " shouldn't be called\n";
}


DataPort::DataPort (unsigned int _num_iopins) : Lcd_Port(_num_iopins)
{
  value = 0;

}

void DataPort::setbit(unsigned int bit_number, bool new_value)
{
  if((lcd->control_port->value & 6) == 4){  //E is high and R/W is low
    //cout << "DataPort::" << __FUNCTION__ << " setting bit:"<<bit_number<<"  new_val" <<new_value<< "\n";
    Lcd_Port::setbit(bit_number, new_value);  // RW bit is low-> write
  } 
  //else cout << "DataPort::" << __FUNCTION__ << " ignoring data. bit:"<<bit_number<<"  new_val" <<new_value<< "\n";
}
void DataPort::update_pin_directions(unsigned int new_direction)
{

  if((new_direction ^ direction) & 1) {
    direction = new_direction & 1;

    //cout << __FUNCTION__ << " new direction " <<new_direction << "  current value = 0x" << hex<<value <<endl;

    // Change the directions of the I/O pins
    for(int i=0; i<8; i++) {
      if(pins[i]) {
	pins[i]->update_direction(direction);

	if(pins[i]->snode)
	  pins[i]->snode->update(0);
      }
    }
  }
}

unsigned int DataPort::get(void)
{

  for(int i=7; i>=0; i--) {
    if(pins[i]) {
      value = (value << 1) | ((pins[i]->state > 0) ? 1 : 0);
      //cout << "   Pin[" << i << "] = " << pins[i]->state << endl;
    }

  }

  value &= 0xff;

  //cout << "DataPort::get = 0x" << value << endl;

  return value;

}

void Lcd_Port::trace_register_write(void)
{

  trace.module1(value);
}

//-----------------------------------------------------

ControlPort::ControlPort (unsigned int _num_iopins) : Lcd_Port(_num_iopins)
{
  break_delta = 100000;
}

void ControlPort::assert_event(void)
{
  
  lcd->advanceState(lcd->ControlEvents[value & 7].e);
}

void ControlPort::put(unsigned int new_value)
{

  unsigned int old_value = value;

  //cout << __FUNCTION__ << " new value " << new_value << endl;

  Lcd_Port::put(new_value);

  if((old_value ^ value) & 2) {  // R/W bit has changed states
    if(value & 2)
      lcd->data_port->update_pin_directions(1);   // It's high, so make data lines outputs
    else
      lcd->data_port->update_pin_directions(0);   // make them inputs.
  }

  if( (old_value ^ value) & 0x7) 
    assert_event();
}

void ControlPort::callback(void)
{
  if(lcd && lcd->debug)
    cout << "ControlPort::callback(void)\n";

  gpsim_set_break_delta(break_delta, this);

}

void DataPort::assert_event(void)
{
  lcd->advanceState(DataChange);
}

void DataPort::put(unsigned int new_value)
{

  unsigned int old_value = value;

  cout << "DataPort::" <<__FUNCTION__ << " new value " << new_value << endl;

  Lcd_Port::put(new_value);

  if( (old_value ^ value) & 0xff)
    lcd->advanceState(DataChange);

}

//--------------------------------------------------------------
// Lcd_Input
//   This class is a minor extension of a normal IO_input. I may
// remove it later, but for now it does serve a simple purpose.
// Specifically, this derivation will intercept when a stimulus
// is being changed. 

void Lcd_Input::put_node_state( int new_state)
{

  int diff = state^new_state;

  //cout << " " << name() << " Lcd_Input::put_node_state = " << new_state << endl;

  IO_input::put_node_state(new_state);
/*
  if(diff)
    cout << "Lcd Input " << name() << " changed to new state: " << state << '\n';
  else
    cout << "Lcd Input " << name() << " didn't change states\n";
*/
}
//---------------------------------------------------------------
Lcd_bi_directional::Lcd_bi_directional(IOPORT *i, unsigned int b,char *opt_name) :
  IO_bi_directional(i, b,opt_name)
{
  drive = MAX_DRIVE/4;
}

void Lcd_bi_directional::put_node_state( int new_state)
{

  int old_state = state;

  if(driving) {
    
    //cout << " " << name() << " Lcd_bi_directional::put_node_state -- driving, so ignoring new state of " << new_state << endl;
    return;
  }

  //cout << " " << name() << " Lcd_bi_directional::put_node_state = " << new_state << endl;
  IO_bi_directional::put_node_state(new_state);
/*
  if(old_state ^ state) {
    cout << "Lcd bi di " << name() << " changed to new state: " << state << '\n';
  }
*/
}

//---------------------------------------------------------------

void LcdDisplay::update(void)
{
  update(darea, w_width,w_height);
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

//--------------------------------------------------------------
// create_iopin_map 
//
//  This is where the information for the Module's package is defined.
// Specifically, the I/O pins of the module are created.

void LcdDisplay::create_iopin_map(void)
{


  // Create an I/O port to which the I/O pins can interface
  //   The module I/O pins are treated in a similar manner to
  //   the pic I/O pins. Each pin has a unique pin number that
  //   describes it's position on the physical package. This
  //   pin can then be logically grouped with other pins to define
  //   an I/O port. 


  control_port = new ControlPort(8);
  control_port->value = 0;
  control_port->lcd = this;

  gpsim_set_break_delta(1000, control_port);

  data_port = new DataPort(8);
  data_port->value = 0;
  data_port->lcd = this;


  // Here, we name the port `pin'. So in gpsim, we will reference
  //   the bit positions as U1.pin0, U1.pin1, ..., where U1 is the
  //   name of the logic gate (which is assigned by the user and
  //   obtained with the name() member function call).

  char *pin_name = name();   // Get the name of this LCD
  if(pin_name) {
    data_port->new_name(pin_name);
    control_port->new_name(pin_name);
  }



  // Define the physical package.
  //   The Package class, which is a parent of all of the modules,
  //   is responsible for allocating memory for the I/O pins.
  //


  create_pkg(14);


  // Define the I/O pins and assign them to the package.
  //   There are two things happening here. First, there is
  //   a new I/O pin that is being created. For the binary 
  //   indicator, both pins are inputs. The second thing is
  //   that the pins are "assigned" to the package. If we
  //   need to reference these newly created I/O pins (like
  //   below) then we can call the member function 'get_pin'.

  assign_pin(4, new Lcd_Input(control_port, 0,"DC"));  // Control
  assign_pin(5, new Lcd_Input(control_port, 1,"RW"));
  assign_pin(6, new Lcd_Input(control_port, 2,"E"));

  assign_pin(7, new Lcd_bi_directional(data_port, 0,"d0"));  // Data bus
  assign_pin(8, new Lcd_bi_directional(data_port, 1,"d1"));
  assign_pin(9, new Lcd_bi_directional(data_port, 2,"d2"));
  assign_pin(10, new Lcd_bi_directional(data_port,3,"d3"));
  assign_pin(11, new Lcd_bi_directional(data_port,4,"d4"));
  assign_pin(12, new Lcd_bi_directional(data_port,5,"d5"));
  assign_pin(13, new Lcd_bi_directional(data_port,6,"d6"));
  assign_pin(14, new Lcd_bi_directional(data_port,7,"d7"));

  data_port->update_pin_directions(0);

  // Create an entry in the symbol table for the new I/O pins.
  // This is how the pins are accessed at the higher levels (like
  // in the CLI).

  for(int i =1; i<get_pin_count(); i++) {
    IOPIN *p = Package::get_pin(i);
    if(p)
      symbol_table.add_stimulus(p);
  }
}

//--------------------------------------------------------------
// construct

ExternalModule * LcdDisplay::construct(const char *new_name=NULL)
{

  cout << " LCD display constructor\n";

  LcdDisplay *lcdP = new LcdDisplay(2,20);
  lcdP->new_name(new_name);
  lcdP->create_iopin_map();


  lcdP->set_pixel_resolution(5,7);
  lcdP->set_crt_resolution(3,3);
  lcdP->set_contrast(1.0);

  lcdP->InitStateMachine();

  return lcdP;

}

LcdDisplay::LcdDisplay(int aRows, int aCols, unsigned aType)
{

  if (verbose)
     cout << "LcdDisplay constructor\n";
  name_str = "Lcd Display";

  mode_flag = _8BIT_MODE_FLAG;
  data_latch_phase = 1;

  disp_type=aType;
  set_pixel_resolution();
  set_crt_resolution();
  set_contrast();

  rows = aRows;
  cols = aCols;

  // The font is created dynamically later on.
  fontP = NULL;

  // If you want to get diagnostic info, change debug to non-zero.
  debug = 0;

  interface = new LCD_Interface(this);
  gi.add_interface(interface);

  CreateGraphics();


}

LcdDisplay::~LcdDisplay()
{
  if (verbose)
      cout << "LcdDisplay destructor\n";

  //gpsim_unregister_interface(interface_id);
  delete data_port;
  gpsim_clear_break(control_port); // Hmmmm. FIXME.
  delete control_port;
  delete interface;

  gtk_widget_destroy(window);

  // FIXME free all data...

}

//-----------------------------------------------------------------
// Displaytech 161A, added by Salvador E. Tropea <set@computer.org>
// construct

ExternalModule * LcdDisplayDisplaytech161A::construct(const char *new_name=NULL)
{

  if (verbose)
     cout << " LCD 161A display constructor\n";

  LcdDisplayDisplaytech161A *lcdP = new LcdDisplayDisplaytech161A(2,8,TWO_ROWS_IN_ONE);
  lcdP->new_name(new_name);
  lcdP->create_iopin_map();

  // Are these really needed? the constructor already does it.
  //lcdP->set_pixel_resolution(5,7);
  //lcdP->set_crt_resolution(3,3);
  //lcdP->set_contrast(1.0);

  lcdP->InitStateMachine();

  return lcdP;

}

LcdDisplayDisplaytech161A::LcdDisplayDisplaytech161A(int aRows, int aCols,
  unsigned aType) :
  LcdDisplay(aRows,aCols,aType)
{
}

LcdDisplayDisplaytech161A::~LcdDisplayDisplaytech161A()
{
}
