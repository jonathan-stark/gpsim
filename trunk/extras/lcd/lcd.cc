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


/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include <errno.h>
#include <stdlib.h>
#include <string>
#include <iostream>

#include <gtk/gtk.h>

#include "lcd.h"
#include <gpsim/gpsim_time.h>


Trace *gTrace=0;                // Points to gpsim's global trace object.

//------------------------------------------------------------------------
// Tracing
//

LcdWriteTO::LcdWriteTO(LcdDisplay *_lcd)
  : LcdTraceObject(_lcd)
{
}

void LcdWriteTO::print(FILE *fp)
{
  fprintf(fp, "  Wrote: LCD");
}

LcdReadTO::LcdReadTO(LcdDisplay *_lcd)
  : LcdTraceObject(_lcd)
{
}

void LcdReadTO::print(FILE *fp)
{
  fprintf(fp, "  Read: LCD");
}

//----------------------------------------
LcdWriteTT::LcdWriteTT(LcdDisplay *_lcd, unsigned int t, unsigned int s)
  : LcdTraceType(_lcd,t,s)
{
}


TraceObject *LcdWriteTT::decode(unsigned int tbi)
{

  unsigned int tv = gTrace->get(tbi);

  LcdWriteTO *lto = new LcdWriteTO(lcd);
  gTrace->addToCurrentFrame(lto);

  return lto;

}
int LcdWriteTT::dump_raw(unsigned tbi, char *buf, int bufsize)
{
  int n = TraceType::dump_raw(tbi,buf,bufsize);

  buf += n;
  bufsize -= n;

  unsigned int tv = gTrace->get(tbi);
  
  int m = snprintf(buf, bufsize," LCD Write 0x%08x", tv);
  if(m>0)
    n += m;

  return n;
}
//----------------------------------------
LcdReadTT::LcdReadTT(LcdDisplay *_lcd, unsigned int t, unsigned int s)
  : LcdTraceType(_lcd,t,s)
{
}


TraceObject *LcdReadTT::decode(unsigned int tbi)
{

  unsigned int tv = gTrace->get(tbi);

  LcdReadTO *lto = new LcdReadTO(lcd);
  gTrace->addToCurrentFrame(lto);

  return lto;

}
int LcdReadTT::dump_raw(unsigned tbi, char *buf, int bufsize)
{
  int n = TraceType::dump_raw(tbi,buf,bufsize);

  buf += n;
  bufsize -= n;

  unsigned int tv = gTrace->get(tbi);
  
  int m = snprintf(buf, bufsize," LCD Read 0x%08x", tv);
  if(m>0)
    n += m;

  return n;
}


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
void LcdBusy::set(double waitTime)
{
  if(!bBusyState) {
    bBusyState = true;
    get_cycles().set_break(get_cycles().get(waitTime), this);
  }
}

void LcdBusy::clear(void)
{
  clear_break();
  bBusyState = false;
}

//--------------------------------------------------------------
void LcdBusy::callback(void)
{
  bBusyState = false;
}

//--------------------------------------------------------------
// LcdPort class
//
// The LcdPort class is derived from the gpsim class "IOPORT".
// As such, it inherits all of the IOPROT's functionality (like
// tracing, stimulus interface, etc.). The LcdPort class extends
// IOPORT by redirecting changes to the LCD state machine.

Lcd_Port::Lcd_Port (LcdDisplay *_lcd, unsigned int _num_iopins)
  : IOPORT(_num_iopins), lcd(_lcd)
{
  if(lcd) {
    set_write_trace(lcd->getWriteTT()->type);
    set_read_trace(lcd->getReadTT()->type);
  }

}
void Lcd_Port::setbit(unsigned int bit_number, bool new_value)
{

  int bit_mask = 1<<bit_number;
  unsigned int current_value = value.get();
  bool current_bit_value = (current_value & bit_mask) ? true : false;

  if( current_bit_value != new_value) {

    gTrace->raw(write_trace.get() | value.get());
  
    value.put(value.get() ^ bit_mask);
    assert_event();

    internal_latch = (current_value & bit_mask) | (internal_latch & ~bit_mask);
  }
}

void Lcd_Port::assert_event(void)
{
  cout << __FUNCTION__ << " shouldn't be called\n";
}


DataPort::DataPort (LcdDisplay *_lcd, unsigned int _num_iopins) 
  : Lcd_Port(_lcd, _num_iopins)
{
  value.put(0);
}

void DataPort::setbit(unsigned int bit_number, bool new_value)
{
  if((lcd->control_port->value.get() & 6) == 4){  //E is high and R/W is low

    Lcd_Port::setbit(bit_number, new_value);  // RW bit is low-> write
  } 

}
void DataPort::update_pin_directions(bool new_direction)
{

  if(new_direction != direction) {

    direction = new_direction;

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

  return IOPORT::get();

#if 0
  unsigned int v=0;

#ifdef BROKEN_GET_METHOD
  
  for(int i=7; i>=0; i--) {
    if(pins[i]) {
      v = (v << 1) | ((pins[i]->getDrivingState()) ? 1 : 0);
    }

  }

#else

  //cout << "LCD DataPort::get - stim_mask is " << stimulus_mask << "\n";
  if(stimulus_mask)
    {
      v = update_stimuli();
      // cout << "DataPort::get - stimuli give " << v << "\n";
      v |= (value.get() & ~stimulus_mask);
    }

#endif

  value.put(v);

  gTrace->raw(read_trace.get() | value.get());

  return value.get();
#endif
}

//-----------------------------------------------------

ControlPort::ControlPort (LcdDisplay *_lcd, unsigned int _num_iopins) 
  : Lcd_Port(_lcd, _num_iopins)
{
  break_delta = 100000;
}

void ControlPort::assert_event(void)
{
  
  lcd->advanceState(lcd->ControlEvents[value.get() & 7].e);
}

void ControlPort::put(unsigned int new_value)
{

  unsigned int old_value = value.get();

  Lcd_Port::put(new_value);

  if((old_value ^ value.get()) & 2) {  // R/W bit has changed states
    if(value.get() & 2)
      lcd->data_port->update_pin_directions(true);   // It's high, so make data lines outputs
    else
      lcd->data_port->update_pin_directions(false);  // make them inputs.
  }

  if( (old_value ^ value.get()) & 0x7) 
    assert_event();
}

void ControlPort::callback(void)
{
  if(lcd && lcd->debug & LCD_DEBUG_ENABLE)
    cout << "ControlPort::callback(void)\n";

  get_cycles().set_break_delta(break_delta, this);

}

void DataPort::assert_event(void)
{
  lcd->advanceState(DataChange);
}

void DataPort::put(unsigned int new_value)
{

  unsigned int old_value = value.get();

  Lcd_Port::put(new_value);

  if( (old_value ^ value.get()) & 0xff)
    lcd->advanceState(DataChange);

}

//---------------------------------------------------------------
Lcd_bi_directional::Lcd_bi_directional(IOPORT *i, unsigned int b,char *opt_name) :
  IO_bi_directional(i, b,opt_name)
{
  // FIXME - we may want to readjust the Pin impedances
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


  control_port = new ControlPort(this,8);
  control_port->value.put(0);

  get_cycles().set_break_delta(1000, control_port);

  data_port = new DataPort(this, 8);
  data_port->value.put(0);

  // Here, we name the port `pin'. So in gpsim, we will reference
  //   the bit positions as U1.pin0, U1.pin1, ..., where U1 is the
  //   name of the logic gate (which is assigned by the user and
  //   obtained with the name() member function call).

  char *pin_name = (char*)name().c_str();   // Get the name of this LCD
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

  data_port->update_pin_directions(false);

  // Create an entry in the symbol table for the new I/O pins.
  // This is how the pins are accessed at the higher levels (like
  // in the CLI).

  for(int i =1; i<get_pin_count(); i++) {
    IOPIN *p = get_pin(i);
    if(p)
      get_symbol_table().add_stimulus(p);
  }
}

//--------------------------------------------------------------
TraceType *LcdDisplay::getWriteTT()
{
  if(!writeTT) {
    writeTT = new LcdWriteTT(this,0,1);
    gTrace->allocateTraceType(writeTT);
  }

  return writeTT;

}

TraceType *LcdDisplay::getReadTT()
{
  if(!readTT) {
    readTT = new LcdReadTT(this,0,1);
    gTrace->allocateTraceType(readTT);
  }
  return readTT;
}

//--------------------------------------------------------------
// construct

Module * LcdDisplay::construct(const char *new_name=NULL)
{

  LcdDisplay *lcdP = new LcdDisplay(2,20);
  lcdP->new_name((char*)new_name);
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
  new_name("Lcd Display");

  mode_flag = _8BIT_MODE_FLAG;
  data_latch = 0;
  data_latch_phase = 1;
  last_event = eWC;

  disp_type=aType;
  set_pixel_resolution();
  set_crt_resolution();
  set_contrast();

  rows = aRows;
  cols = aCols;
  cursor.row = 0;
  cursor.col = 0;

  in_cgram = FALSE;
  cgram_updated = FALSE;
  cgram_cursor = 0;
  memset(&cgram[0], 0xff, sizeof(cgram));
  // The font is created dynamically later on.
  fontP = NULL;

  // If you want to get diagnostic info, change debug to non-zero.
  debug = 0;
  if (getenv("GPSIM_LCD_DEBUG"))
    debug = atoi(getenv("GPSIM_LCD_DEBUG"));

  gTrace = &get_trace();
  writeTT = new LcdWriteTT(this,0,1);
  readTT = new LcdReadTT(this,0,1);

  interface = new LCD_Interface(this);
  get_interface().add_interface(interface);

  CreateGraphics();


}

LcdDisplay::~LcdDisplay()
{
  if (verbose)
      cout << "LcdDisplay destructor\n";

  //gpsim_unregister_interface(interface_id);
  delete data_port;
  get_cycles().clear_break(control_port); // Hmmmm. FIXME.
  delete control_port;
  delete interface;

  gtk_widget_destroy(window);

  // FIXME free all data...

}

//-----------------------------------------------------------------
// Displaytech 161A, added by Salvador E. Tropea <set@computer.org>
// construct

Module * LcdDisplayDisplaytech161A::construct(const char *new_name=NULL)
{

  if (verbose)
     cout << " LCD 161A display constructor\n";

  LcdDisplayDisplaytech161A *lcdP = new LcdDisplayDisplaytech161A(2,8,TWO_ROWS_IN_ONE);
  lcdP->new_name((char*)new_name);
  lcdP->create_iopin_map();

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
