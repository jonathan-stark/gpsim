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
  Logic.cc

This is an example module library for interfacing with gpsim.

In here you'll find some simple logic devices:

  AND2Gate - A 2-input AND gate
  OR2Gate - A 2-input OR gate

*/
#include "../config.h"    // get the definition for HAVE_GUI

#ifdef HAVE_GUI

/* XPM */
static char * and2_pixmap[] = {
"32 32 3 1",
" 	c black",
".	c None",
"X	c white",
"                    ............",
"                        ........",
"  XXXXXXXXXXXXXXXXXX    ........",
"  XXXXXXXXXXXXXXXXXXXXXX  ......",
"  XXXXXXXXXXXXXXXXXXXXXX  ......",
"  XXXXXXXXXXXXXXXXXXXXXXXX  ....",
"  XXXXXXXXXXXXXXXXXXXXXXXX   ...",
"  XXXXXXXXXXXXXXXXXXXXXXXXX  ...",
"  XXXXXXXXXXXXXXXXXXXXXXXXXXX  .",
"  XXXXXXXXXXXXXXXXXXXXXXXXXXX  .",
"  XXXXXXXXXXXXXXXXXXXXXXXXXXX  .",
"  XXXXXX XXX XXX X    XXXXXXX  .",
"  XXXXX X XX  XX XX XX XXXXXX  .",
"  XXXX XXX X  XX XX XX XXXXXXX  ",
"  XXXX XXX X X X XX XX XXXXXXX  ",
"  XXXX XXX X X X XX XX XXXXXXX  ",
"  XXXX     X XX  XX XX XXXXXXX  ",
"  XXXX XXX X XX  XX XX XXXXXXX  ",
"  XXXX XXX X XXX XX XX XXXXXXX  ",
"  XXXX XXX X XXX X    XXXXXXX  .",
"  XXXXXXXXXXXXXXXXXXXXXXXXXXX  .",
"  XXXXXXXXXXXXXXXXXXXXXXXXXXX  .",
"  XXXXXXXXXXXXXXXXXXXXXXXXXXX  .",
"  XXXXXXXXXXXXXXXXXXXXXXXXXXX  .",
"  XXXXXXXXXXXXXXXXXXXXXXXXX  ...",
"  XXXXXXXXXXXXXXXXXXXXXXXX   ...",
"  XXXXXXXXXXXXXXXXXXXXXXXX  ....",
"  XXXXXXXXXXXXXXXXXXXXXX  ......",
"  XXXXXXXXXXXXXXXXXXXXXX  ......",
"  XXXXXXXXXXXXXXXXXX    ........",
"                        ........",
"                    ............"};

/* XPM */
static char * or2_pixmap[] = {
"32 32 3 1",
" 	c black",
".	c None",
"X	c white",
"                    ............",
"                        ........",
"  XXXXXXXXXXXXXXXXXX    ........",
"  XXXXXXXXXXXXXXXXXXXXXX  ......",
"  XXXXXXXXXXXXXXXXXXXXXX  ......",
".  XXXXXXXXXXXXXXXXXXXXXXX  ....",
".  XXXXXXXXXXXXXXXXXXXXXXX   ...",
".  XXXXXXXXXXXXXXXXXXXXXXXX  ...",
"..  XXXXXXXXXXXXXXXXXXXXXXXXX  .",
"..  XXXXXXXXXXXXXXXXXXXXXXXXX  .",
"..  XXXXXXXXXXXXXXXXXXXXXXXXX  .",
"..  XXXXXXX   XX    XXXXXXXXX  .",
"...  XXXXX XXX X XXX XXXXXXXX  .",
"...  XXXXX XXX X XXX XXXXXXXXX  ",
"...  XXXXX XXX X XXX XXXXXXXXX  ",
"...  XXXXX XXX X    XXXXXXXXXX  ",
"...  XXXXX XXX X X XXXXXXXXXXX  ",
"...  XXXXX XXX X XX XXXXXXXXXX  ",
"...  XXXXX XXX X XXX XXXXXXXXX  ",
"...  XXXXXX   XX XXX XXXXXXXX  .",
"..  XXXXXXXXXXXXXXXXXXXXXXXXX  .",
"..  XXXXXXXXXXXXXXXXXXXXXXXXX  .",
"..  XXXXXXXXXXXXXXXXXXXXXXXXX  .",
"..  XXXXXXXXXXXXXXXXXXXXXXXXX  .",
".  XXXXXXXXXXXXXXXXXXXXXXXX  ...",
".  XXXXXXXXXXXXXXXXXXXXXXX   ...",
".  XXXXXXXXXXXXXXXXXXXXXXX  ....",
"  XXXXXXXXXXXXXXXXXXXXXX  ......",
"  XXXXXXXXXXXXXXXXXXXXXX  ......",
"  XXXXXXXXXXXXXXXXXX    ........",
"                        ........",
"                    ............"};

static char * xor2_pixmap[] = {
"40 32 3 1",
" 	c None",
".	c black",
"X	c white",
"        ....................            ",
"  ..    ....................            ",
" ....   ........................        ",
"  ...   ..XXXXXXXXXXXXXXXXXX....        ",
"  ...   ..XXXXXXXXXXXXXXXXXXXXXX..      ",
"   ...  ..XXXXXXXXXXXXXXXXXXXXXX..      ",
"   ....  ..XXXXXXXXXXXXXXXXXXXXXXX..    ",
"    ...  ..XXXXXXXXXXXXXXXXXXXXXXX...   ",
"    ...  ..XXXXXXXXXXXXXXXXXXXXXXXX..   ",
"    ...   ..XXXXXXXXXXXXXXXXXXXXXXXXX.. ",
"    ....  ..XXXXXXXXXXXXXXXXXXXXXXXXX.. ",
"     ...  ..XXXXXXXXXXXXXXXXXXXXXXXXX.. ",
"     ...  ..XXXX.XXX.XX...XX....XXXXX.. ",
"      ..   ..XXX.XXX.X.XXX.X.XXX.XXXX.. ",
"      ..   ..XXXX.X.XX.XXX.X.XXX.XXXXX..",
"      ..   ..XXXX.X.XX.XXX.X.XXX.XXXXX..",
"      ...  ..XXXXX.XXX.XXX.X....XXXXXX..",
"      ...  ..XXXX.X.XX.XXX.X.X.XXXXXXX..",
"      ...  ..XXXX.X.XX.XXX.X.XX.XXXXXX..",
"      ...  ..XXX.XXX.X.XXX.X.XXX.XXXXX..",
"      ...  ..XXX.XXX.XX...XX.XXX.XXXX.. ",
"      ... ..XXXXXXXXXXXXXXXXXXXXXXXXX.. ",
"      ..  ..XXXXXXXXXXXXXXXXXXXXXXXXX.. ",
"     ...  ..XXXXXXXXXXXXXXXXXXXXXXXXX.. ",
"     ...  ..XXXXXXXXXXXXXXXXXXXXXXXXX.. ",
"    ...  ..XXXXXXXXXXXXXXXXXXXXXXXX..   ",
"    ...  ..XXXXXXXXXXXXXXXXXXXXXXX...   ",
"    ...  ..XXXXXXXXXXXXXXXXXXXXXXX..    ",
"   .... ..XXXXXXXXXXXXXXXXXXXXXX..      ",
"  ..... ..XXXXXXXXXXXXXXXXXXXXXX..      ",
" ....   ..XXXXXXXXXXXXXXXXXX....        ",
"  ..    ........................        "};

static char * not_pixmap[] = {
"32 32 3 1",
" 	c black",
".	c None",
"X	c white",
" ...............................",
"   .............................",
"     ...........................",
"  X   ..........................",
"  XXX   ........................",
"  XXXX    ......................",
"  XXXXXX    ....................",
"  XXXXXXXX   ...................",
"  XXXXXXXXXX   .................",
"  XXXXXXXXXXX    ...............",
"  XXXXXXXXXXXXX   ..............",
"  XXXXXXXXXXXXXXX   ............",
"  XXXXXXXXXXXXXXXX    .....   ..",
"  XXXXXXXXXXXXXXXXXX    .. XXX .",
"  XXXXXXXXXXXXXXXXXXXX    XXXXX ",
"  XXXXXXXXXXXXXXXXXXXXXX  XXXXX ",
"  XXXXXXXXXXXXXXXXXXXX    XXXXX ",
"  XXXXXXXXXXXXXXXXXXX   .. XXX .",
"  XXXXXXXXXXXXXXXXX   .....   ..",
"  XXXXXXXXXXXXXXX    ...........",
"  XXXXXXXXXXXXXX   .............",
"  XXXXXXXXXXXX   ...............",
"  XXXXXXXXXXX   ................",
"  XXXXXXXXX   ..................",
"  XXXXXXX    ...................",
"  XXXXXX   .....................",
"  XXXX   .......................",
"  XXX   ........................",
"  X   ..........................",
"     ...........................",
"   .............................",
" ..............................."};

#include <errno.h>
#include <stdlib.h>
#include <string>

#include <gtk/gtk.h>

#include "logic.h"

//--------------------------------------------------------------
// Led_Input
//   This class is a minor extension of a normal IO_input. I may
// remove it later, but for now it does serve a simple purpose.
// Specifically, this derivation will intercept when a stimulus
// is being changed. 

void Logic_Input::put_node_state( int new_state)
{

  int current_state = state;


  IO_input::put_node_state(new_state);

  if(current_state ^ state) {
//    cout << "logic Input " << name() << " changed to new state: " <<
//      state << '\n';

    if(LGParent)
      LGParent->update_state();
  }
}


/*************************************************************
*
*  LogicGate class
*/

LogicGate::LogicGate(void)
{

  //cout << "LogicGate base class constructor\n";
}

LogicGate::~LogicGate(void)
{

    //cout << "LogicGate base class destructor\n";

    delete port;
}

//--------------------------------------------------------------
// create_iopin_map 
//
//  This is where the information for the Module's package is defined.
// Specifically, the I/O pins of the module are created.

void LogicGate::create_iopin_map(void)
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

  char *pin_name = (char*)name().c_str();   // Get the name of this logic gate
  if(pin_name) {
    port->new_name(pin_name);
  }
  else
    port->new_name("pin");


  // Define the physical package.
  //   The Package class, which is a parent of all of the modules,
  //   is responsible for allocating memory for the I/O pins.
  //

  create_pkg(number_of_pins);


  // Define the I/O pins and assign them to the package.
  //   There are two things happening here. First, there is
  //   a new I/O pin that is being created. For the binary 
  //   indicator, both pins are inputs. The second thing is
  //   that the pins are "assigned" to the package. If we
  //   need to reference these newly created I/O pins (like
  //   below) then we can call the member function 'get_pin'.


  // all logic gates have one or more inuts, but only one
  // output. The output is arbitrarily assigned to position
  // 0 on the I/O port while the inputs go to positions 1 and above

#define OUTPUT_BITPOSITION 0
#define INPUT_FIRST_BITPOSITION (OUTPUT_BITPOSITION + 1)

  Logic_Output *LOP = new Logic_Output(port, OUTPUT_BITPOSITION, "out");
  LOP->new_logic_gate(this);
  LOP->update_direction(1);                 // make the bidirectional an output

  // Position pin on middle right side of package
  package->set_pin_position(1,2.5);
  assign_pin(OUTPUT_BITPOSITION + 1, LOP);  // output

  Logic_Input *LIP;

  char p[4] = "in0";
  int j;

  for(i=j=INPUT_FIRST_BITPOSITION; i<number_of_pins; i++) {
    p[2] = i-j +'0';
    LIP = new Logic_Input(port, i,p);
    LIP->new_logic_gate(this);
    if(number_of_pins==2)
      package->set_pin_position(i+1, 0.5); // Left side of package
    else
      package->set_pin_position(i+1, (i-INPUT_FIRST_BITPOSITION)*0.9999); // Left side of package
    assign_pin(i+1, LIP );       //  Pin numbers begin at 1
  }

  // Form the logic gate bit masks
  // The output is at port bit position 0
  output_bit_mask = 1 << OUTPUT_BITPOSITION;
  input_bit_mask = (1<< (number_of_pins)) - 2;

  
  //cout << hex << "  input_bit_mask = " << input_bit_mask << '\n';

  // Create an entry in the symbol table for the new I/O pins.
  // This is how the pins are accessed at the higher levels (like
  // in the CLI).

  for(i= 1; i<=number_of_pins; i++)
    symbol_table.add_stimulus(get_pin(i));


  //cout << "Iopin map should be created\n";
}

GtkWidget *LogicGate::create_pixmap(char **pixmap_data)
{
    GtkStyle *style;
    GdkBitmap *mask;
    GdkPixmap *pixmap;

    style = gtk_style_new();
    
    pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL,
						   gdk_colormap_get_system(),
						   &mask,
						   &style->bg[GTK_STATE_NORMAL],
						   pixmap_data);

    return gtk_pixmap_new(pixmap,mask);
}

//--------------------------------------------------------------
// construct

Module * AND2Gate::construct(const char *new_name)
{


  AND2Gate *a2gP = new AND2Gate ;

  a2gP->new_name((char*)new_name);
  a2gP->set_number_of_pins(3);
  a2gP->create_iopin_map();

  return a2gP;

}

AND2Gate::AND2Gate(void)
{

  set_widget(create_pixmap(and2_pixmap));

}
AND2Gate::~AND2Gate(void)
{
}


void ANDGate::update_state(void)
{
  unsigned int old_value = port->value;

  //cout << "update_state of ANDGate\n";
  if((port->value & input_bit_mask) == input_bit_mask)
    port->value |= output_bit_mask;
  else
    port->value &= ~output_bit_mask;

  if( (old_value ^ port->value) & output_bit_mask) {

    if(port->pins[0]->snode) {
      port->pins[0]->snode->update(0);
    }
//    cout << "logic gate output just went " <<
//      ( (port->value & output_bit_mask) ? "HIGH" : "LOW") << '\n';
  }

}

//--------------------------------------------------------------
// construct


OR2Gate::OR2Gate(void)
{

  set_widget(create_pixmap(or2_pixmap));

}
OR2Gate::~OR2Gate(void)
{

}

Module * OR2Gate::construct(const char *new_name)
{


  OR2Gate *o2gP = new OR2Gate ;

  o2gP->new_name((char*)new_name);
  o2gP->set_number_of_pins(3);
  o2gP->create_iopin_map();

  return o2gP;

}

void ORGate::update_state(void)
{
  unsigned int old_value = port->value;

  //cout << "update_state of ORGate\n";
  if(port->value & input_bit_mask) 
    port->value |= output_bit_mask;
  else
    port->value &= ~output_bit_mask;

  if( (old_value ^ port->value) & output_bit_mask) {

    if(port->pins[0]->snode) {
      port->pins[0]->snode->update(0);
    }
//    cout << "logic gate output just went " <<
//      ( (port->value & output_bit_mask) ? "HIGH" : "LOW") << '\n';
  }

}


//--------------------------------------------------------------
// construct NOT
Module * NOTGate::construct(const char *new_name)
{

  NOTGate *a2gP = new NOTGate ;

  a2gP->new_name((char*)new_name);
  a2gP->set_number_of_pins(2);
  a2gP->create_iopin_map();

  return a2gP;

}

NOTGate::NOTGate(void)
{

  set_widget(create_pixmap(not_pixmap));

}
NOTGate::~NOTGate(void)
{

}

void NOTGate::update_state(void)
{
  unsigned int old_value = port->value;

  //cout << "update_state of NOTGate\n";
  if((port->value & input_bit_mask) == input_bit_mask)
    port->value &= ~output_bit_mask;
  else
    port->value |= output_bit_mask;

  if( (old_value ^ port->value) & output_bit_mask) {

    if(port->pins[0]->snode) {
      port->pins[0]->snode->update(0);
    }
//    cout << "logic gate output just went " <<
//      ( (port->value & output_bit_mask) ? "HIGH" : "LOW") << '\n';
  }

}

//--------------------------------------------------------------
// construct


XOR2Gate::XOR2Gate(void)
{

  set_widget(create_pixmap(xor2_pixmap));

}
XOR2Gate::~XOR2Gate(void)
{
}

Module * XOR2Gate::construct(const char *new_name)
{

  XOR2Gate *o2gP = new XOR2Gate ;

  o2gP->new_name((char*)new_name);
  o2gP->set_number_of_pins(3);
  o2gP->create_iopin_map();

  return o2gP;

}

void XORGate::update_state(void)
{
  unsigned int old_value = port->value;
  int i;
  int out_value=0;

  //cout << "update_state of XORGate\n";

  for(i=INPUT_FIRST_BITPOSITION; i<number_of_pins; i++) {
      if(port->value & (1<<i))
	  out_value++;
  }

  //printf("out_value %d\n",out_value);

  if(out_value&1)
    port->value |= output_bit_mask;
  else
    port->value &= ~output_bit_mask;

  if( (old_value ^ port->value) & output_bit_mask) {

    if(port->pins[0]->snode) {
      port->pins[0]->snode->update(0);
    }
//    cout << "logic gate output just went " <<
//      ( (port->value & output_bit_mask) ? "HIGH" : "LOW") << '\n';
  }

}

#endif // HAVE_GUI
