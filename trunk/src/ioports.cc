/*
   Copyright (C) 1998 Scott Dattalo

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


#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <string>


#include "../config.h"
#include "pic-processor.h"
#include "14bit-processors.h"  // %%% FIXME %%% remove the dependencies on this
#include "ioports.h"
#include "interface.h"
#include "p16x6x.h"
#include "p16f62x.h"

#include "stimuli.h"

#include "xref.h"

//-------------------------------------------------------------------
//
//                 ioports.cc
//
// The ioport infrastructure for gpsim is provided here. The class
// taxonomy for the IOPORT class is:
//
//  file_register 
//     |-> sfr_register
//            |-> IOPORT
//                  |-> PORTA
//                  |-> PORTB
//                  |-> PORTC
//                  |-> PORTD
//                  |-> PORTE
//                  |-> PORTF
// 
// Each I/O port has an associated array of I/O pins which provide an
// interface to the virtual external world of the stimuli.
//
//-------------------------------------------------------------------

//-------------------------------------------------------------------
// IOPORT
//

const int one_shifted_left_by_n [8] = 
{
  1 << 0,
  1 << 1,
  1 << 2,
  1 << 3,
  1 << 4,
  1 << 5,
  1 << 6,
  1 << 7
};


//-------------------------------------------------------------------
//
// PICIOPORT::update_stimuli
//
//   input: none
//  return: the states of the stimuli that are driving this ioport
//
//  This member function will update each node that is attached to the
//  iopins of this port. If there are no pins attached, 0 is returned.
//  
//
//-------------------------------------------------------------------
int IOPORT::update_stimuli(void)

{

  // ??? cout << "IOPORT::"<<__FUNCTION__ << "() is deprecated\n";

  // Loop through the io pins and determine if there are
  // any sources attached to the same node

  //cout << "updating the stimuli\n";
  guint64 time = cycles.value;
  int input = 0;

  for(int i = 0, m=1; i<IOPINS; i++, m <<= 1)
    if(stimulus_mask & m)
      {
        if(pins[i]->snode!=NULL)
	{
	    int t = pins[i]->snode->update(time);

	    //cout << name() << ' ' << i;
	    //cout << " pin " << pins[i]->name();
	    //cout <<  " node "<< pins[i]->snode->name() << " is ";

	    if(t  > pins[i]->l2h_threshold)  // %%% FIXME %%%
	    {
		//cout << "above";
		input |= m;
	    } else //if(!(tris->value & m))
		input |= (value & m);
	    //else cout << "below";

	    //cout << " threshold. node " << t  << " threshold " << pins[i]->threshold << '\n';
	}
      }

  // cout << " returning " << hex << input << '\n';

  return input;

}

int PIC_IOPORT::update_stimuli(void)

{
  // Loop through the io pins and determine if there are
  // any sources attached to the same node

  //cout << "updating the stimuli\n";
  guint64 time = cycles.value;
  int input = 0;

  for(int i = 0, m=1; i<IOPINS; i++, m <<= 1)
    if(stimulus_mask & m)
      {
        if(pins[i]->snode!=NULL)
	{
	    int t = pins[i]->snode->update(time);

	    //cout << name() << ' ' << i;
	    //cout << " pin " << pins[i]->name();
	    //cout <<  " node "<< pins[i]->snode->name() << " is ";

	    if(t  > pins[i]->l2h_threshold)  // %%% FIXME %%%
	    {
		//cout << "above";
		input |= m;
	    } else if(!(tris->value & m))
		input |= (value & m);
	    //else cout << "below";

	    //cout << " threshold. node " << t  << " threshold " << pins[i]->threshold << '\n';
	}
      }

  // cout << " returning " << hex << input << '\n';

  return input;

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
int IOPORT::get_bit_voltage(unsigned int bit_number)
{

  guint64 time = cycles.value;
  int v;

  if(pins[bit_number]) {
    if(pins[bit_number]->snode)
      v = pins[bit_number]->snode->update(time);
    else
      v = pins[bit_number]->get_voltage(time);
  }
  else
    v = (value &  one_shifted_left_by_n [bit_number]) ? 
	    (MAX_DRIVE / 2) : -(MAX_DRIVE / 2) ;

  float vf = v;
  vf = vf / MAX_ANALOG_DRIVE;
  //cout << __FUNCTION__ << "() for bit " << bit_number << ", voltage = " << v << ", vf = " << vf << " volts\n";

  return v;
}      

//-------------------------------------------------------------------
//-------------------------------------------------------------------
int IOPORT::get_bit(unsigned int bit_number)
{

  return( (value &  one_shifted_left_by_n [bit_number & 0x07]) ? 1 : 0);

}

//-------------------------------------------------------------------
//  IOPORT::get(void)
//
//   inputs:  none
//  returns:  the current state of the ioport
//
// If there are stimuli attached to the iopins, then their current
// state is obtained. If there aren't any attached, then the last
// value written to the ioport is obtained.
//
//-------------------------------------------------------------------

unsigned int IOPORT::get(void)
{

  //cout << "IOPORT::get()\n";

  // Update the stimuli - if there are any

  if(stimulus_mask)
    {
      value = ( (value & ~stimulus_mask) | update_stimuli());
    }

  trace.register_read(address,value);


  return(value);
}

//-------------------------------------------------------------------
//  IOPORT::put(unsigned int new_value)
//
//  inputs:  new_value - 
//                       
//  returns: none
//
//  The I/O Port is updated with the new value. If there are any stimuli
// attached to the I/O pins then they will be updated as well.
//
//-------------------------------------------------------------------

void IOPORT::put(unsigned int new_value)
{


  // The I/O Ports have an internal latch that holds the state of the last
  // write, even if the I/O pins are configured as inputs. If the tris port
  // changes an I/O pin from an input to an output, then the contents of this
  // internal latch will be placed onto the external I/O pin.

  internal_latch = new_value;

  // update only those bits that are really outputs
  //cout << "IOPORT::put trying to put " << new_value << '\n';

  value = new_value;

  //cout << " IOPORT::put just set port value to " << value << '\n';

  // Update the stimuli - if there are any
  if(stimulus_mask)
    update_stimuli();

  //cout << " IOPORT::put port value is " << value << " after updating stimuli\n";

  trace.register_write(address,value);

}

//-------------------------------------------------------------------
//  PIC_IOPORT::put(unsigned int new_value)
//
//  inputs:  new_value - here's where the I/O port is written (e.g.
//                       gpsim is executing a MOVWF IOPORT,F instruction.)
//  returns: none
//
//  The I/O Port is updated with the new value. If there are any stimuli
// attached to the I/O pins then they will be updated as well.
//
//-------------------------------------------------------------------

void PIC_IOPORT::put(unsigned int new_value)
{

  if(new_value > 255)
    cout << "PIC_IOPORT::put value >255\n";
  // The I/O Ports have an internal latch that holds the state of the last
  // write, even if the I/O pins are configured as inputs. If the tris port
  // changes an I/O pin from an input to an output, then the contents of this
  // internal latch will be placed onto the external I/O pin.

  internal_latch = new_value;

  // update only those bits that are really outputs
  //cout << "IOPORT::put trying to put " << new_value << '\n';

  value = (new_value & ~tris->value) | (value & tris->value);

  //cout << " IOPORT::put just set port value to " << value << '\n';

  // Update the stimuli - if there are any
  if(stimulus_mask)
    update_stimuli();

  //cout << " IOPORT::put port value is " << value << " after updating stimuli\n";

  trace.register_write(address,value);

}
//-------------------------------------------------------------------
// void IOPORT::put_value(unsigned int new_value)
//
//  When there's a gui initiated change to the IO port, we'll pass
// though here. There are three things that we do. First, we update
// the I/O port the way the gui asks us. Note however, that it's 
// possible that the gui's requested will go un-honored (if for example,
// we try to force an output to change states or if there's a stimulus
// driving the bus already). 
//   Next, after updating the IO port (and all of it's connected stimuli),
// we'll call the gui to update its windows. This is done through the
// xref->update call.
//   Finally, we'll check all of the I/O pins that have changed as a
// result of the IO port update and individually call each of their 
// cross references.
//
//-------------------------------------------------------------------
void IOPORT::put_value(unsigned int new_value)
{
  int i,j;
  int old_value = value;
  int diff;

  //  cout << "IOPORT::put_value trying to put " << new_value << '\n';
 
  put(new_value);

  //  cout << " IOPORT::put_value just set port value to " << value << '\n';

  if(xref)
    xref->update();

  // Find the pins that have changed states
  diff = (old_value ^ value) & valid_iopins;

  // Update the cross references for each pin that has changed.
  for(i=0,j=1; i<8; i++,j<<=1) {

    if(j & diff ) {

      if(pins[i]->xref) {
	pins[i]->xref->update();
	//cout << " IOPORT::put_value updating pin # " << i << '\n';
      }

    }

  }


}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
void IOPORT::setbit(unsigned int bit_number, bool new_value)
{

  int bit_mask = one_shifted_left_by_n[bit_number];
  //cout << name();

  if( ((bit_mask & value) != 0) ^ (new_value==1))
    {
      //if(verbose)
      //cout << " IOPORT::set_bit bit changed due to a stimulus. new_value = " << new_value <<'\n';
      value ^= bit_mask;

      trace_register_write();
    }
  //else cout <<  " IOPORT::set_bit bit did not change\n";

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void IOPORT::setbit_value(unsigned int bit_number, bool new_value)
{

  int bit_mask = one_shifted_left_by_n[bit_number];
  //cout << name();

  if( ((bit_mask & value) != 0) ^ (new_value==1))
  {
  //  cout << " IOPORT::setbit_value bit " << bit_number << " to " << new_value << '\n';
    put_value(value ^= bit_mask);
    //cout << "     is changing\n";
    if(xref)
      xref->update();
  }

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void IOPORT::change_pin_direction(unsigned int bit_number, bool new_direction)
{

  cout << " IOPORT::" << __FUNCTION__ <<'(' << bit_number << ',' << new_direction << ") doesn't do anything.\n";

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void PIC_IOPORT::change_pin_direction(unsigned int bit_number, bool new_direction)
{

  if(tris)
    tris->setbit(bit_number, new_direction);
}

//-------------------------------------------------------------------
// attach_iopin
//   This will store a pointer to the iopin that is associated with
// one of the bits of the I/O port.
//
//-------------------------------------------------------------------
void IOPORT::attach_iopin(IOPIN * new_pin, unsigned int bit_position)
{

  if(bit_position < num_iopins)

    pins[bit_position] = new_pin;

  else
    cout << "Warning: iopin pin number ("<<bit_position 
	 <<") is too large for " << name() << ". Max is " << num_iopins << '\n';

  if(verbose)
    cout << "attaching iopin to ioport " << name() << '\n';
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void IOPORT::attach_stimulus(stimulus *new_stimulus, unsigned int bit_position)
{
  //  cout << "trying to attach " << name() << " to " << new_stimulus->name() <<  '\n';

  //cout << bit_position  << "  " << (1<<bit_position) << '\n';


  if(pins  && (bit_position < num_iopins) && pins[bit_position]) {

    stimulus_mask |= (1<<bit_position);

    if(pins[bit_position]->snode == NULL)
      {
	// If this I/O pin is not attached to a node yet, 
	// then create a node and attach it.

	pins[bit_position]->snode = new Stimulus_Node();
	pins[bit_position]->snode->attach_stimulus(pins[bit_position]);
      }

    // attach the new stimulus to the same node as this I/O pin's

    pins[bit_position]->snode->attach_stimulus(new_stimulus);
  }

  //cout << "IOPORT attached new stimulus to " << pins[bit_position]->snode->name() << " named " <<
  //  pins[bit_position]->snode->name() << '\n';


}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void IOPORT::attach_node(Stimulus_Node *new_node, unsigned int bit_position)
{
  //  cout << "trying to attach " << new_node->name() << " to " << name() << " bit " << bit_position << '\n';

  if(pins[bit_position])
    {
      stimulus_mask |= (1<<bit_position);
      //      pins[bit_position]->snode == new_node;
    }
  else
    cout << "Error: attaching node to a non-existing I/O pin.\n";

}

//-------------------------------------------------------------------
// PIC_IOPORT::update_pin_directions(unsigned int new_tris)
//
//  Whenever a new value is written to a tris register, then we need
// to update the stimuli associated with the I/O pins. This is true
// even if the I/O pin are not connected to external stimuli (like a
// square wave). An example scenario would be like changing a port b
// pin from an output that's driving low to an input. If there's no
// stimulus attached to the port b I/O pin then the pull up (if enabled)
// will pull the I/O pin high.
//
//-------------------------------------------------------------------
void PIC_IOPORT::update_pin_directions(unsigned int new_tris)
{

  if(!tris)
    return;

  unsigned int diff = tris->value ^ new_tris;

  if(diff)
    {
      // Update the I/O port value to that of the internal latch
      value = (value & ~diff) | (internal_latch & diff);

      // Go through and update the direction of the I/O pins
      int i,m;
      for(i = 0, m=1; i<IOPINS; i++, m <<= 1)
	if(m & diff & valid_iopins)
	  {
	  pins[i]->update_direction(m & (~new_tris));
	  //cout << __FUNCTION__ << " name " << pins[i]->name() << " pin number " << i << '\n';
	  }
      // Now, update the nodes to which the(se) pin(s) may be attached

      guint64 time = cycles.value;
      for(i = 0, m=1; i<IOPINS; i++, m <<= 1)
	if(stimulus_mask & m & diff)
          if(pins[i]->snode!=NULL)
            pins[i]->snode->update(time);
    }
}

//-------------------------------------------------------------------
// trace_register_write
//   - a wrapper for trace.register_write
// This provides an option for IOPORTs derived from the IOPORT class 
// to override the behavior of IOPORT traces.
//-------------------------------------------------------------------
void IOPORT::trace_register_write(void)
{
  trace.register_write(address,value);
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
PIC_IOPORT::PIC_IOPORT(unsigned int _num_iopins) : IOPORT(_num_iopins)
{
  tris = NULL;
  latch = NULL;
}

IOPORT::IOPORT(unsigned int _num_iopins)
{
  break_point = 0;
  stimulus_mask = 0;
  num_iopins = _num_iopins;
  address = 0;
  value = 0;
  internal_latch = 0;

  pins = (IOPIN **) new char[sizeof (IOPIN *) * num_iopins];

  for(int i=0; i<num_iopins; i++)
    pins[i] = NULL;

  new_name("ioport");
}

IOPORT::~IOPORT()
{
    for(int i=0; i<num_iopins; i++)
    {
	if(pins[i] != NULL)
	    delete pins[i];
    }
    delete pins;
}

//-------------------------------------------------------------------
// IOPORT_TRIS
//
//-------------------------------------------------------------------

unsigned int IOPORT_TRIS::get(void)
{

  trace.register_read(address,value);

  return(value);
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void IOPORT_TRIS::put(unsigned int new_value)
{
  int save_port_latch = port->internal_latch;

  if(verbose)
    cout << "IOPORT_TRIS::put 0x"<<hex<<new_value<<'\n';
  port->update_pin_directions(new_value);

  value = new_value;

  port->put(save_port_latch);

  trace.register_write(address,value);

}

//-------------------------------------------------------------------
// void IOPORT_TRIS::setbit(unsigned int bit_number, bool new_value)
//
//  This routine will set the bit, 'bit_number', to the value 'new_value'
// If the new_value is different than the old one then we will also
// update the 
//
//-------------------------------------------------------------------
void IOPORT_TRIS::setbit(unsigned int bit_number, bool new_value)
{

  int diff = port->valid_iopins & (1<<bit_number) & (value ^ (new_value << bit_number));

  if(diff) {
    port->update_pin_directions(value ^ diff);

    value ^= diff;

    trace.register_write(address,value);
    if(xref)
      xref->update();
    if(port->pins[bit_number]->xref)
      port->pins[bit_number]->xref->update();

  }

}

//-------------------------------------------------------------------
// void IOPORT_TRIS::put_value(unsigned int new_value)
//
//  When the gui tries to change the tris register, we'll pass
// though here. There are three things that we do. First, we update
// the tris port the way the gui asks us. Next, we'll update any cross
// references, and finally we'll update any cross references for the
// I/O port associated with this tris register.
//-------------------------------------------------------------------

void IOPORT_TRIS::put_value(unsigned int new_value)
{
 
  put(new_value);


  if(xref)
    xref->update();

  //port->put_value(port->internal_latch);

  if(port->xref)           // The I/O pins have changed states
    port->xref->update();  // so tell the gui.

  for(int i=0; i<port->num_iopins; i++) {
    if(port->pins[i] && port->pins[i]->xref)
      port->pins[i]->xref->update();
  }

}

IOPORT_TRIS::IOPORT_TRIS(void)
{
  break_point = 0;
  port = NULL;
  valid_iopins = 0;
  new_name("ioport");
}

//-------------------------------------------------------------------
// IOPORT_LATCH
//
//-------------------------------------------------------------------

unsigned int IOPORT_LATCH::get(void)
{

  trace.register_read(address,value);

  return(value);
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void IOPORT_LATCH::put(unsigned int new_value)
{

  if(verbose)
    cout << "IOPORT_LATCH::put 0x"<<hex<<new_value<<'\n';

  value = new_value;

  port->put(value);

  trace.register_write(address,value);

}

//-------------------------------------------------------------------
// void IOPORT_LATCH::setbit(unsigned int bit_number, bool new_value)
//
//  This routine will set the bit, 'bit_number', to the value 'new_value'
// If the new_value is different than the old one then we will also
// update the 
//
//-------------------------------------------------------------------
void IOPORT_LATCH::setbit(unsigned int bit_number, bool new_value)
{

  port->setbit(bit_number,new_value);

}

//-------------------------------------------------------------------
// void IOPORT_LATCH::put_value(unsigned int new_value)
//
//  When the gui tries to change the tris register, we'll pass
// though here. There are three things that we do. First, we update
// the tris port the way the gui asks us. Next, we'll update any cross
// references, and finally we'll update any cross references for the
// I/O port associated with this tris register.
//-------------------------------------------------------------------

void IOPORT_LATCH::put_value(unsigned int new_value)
{
 
  put(new_value);
  port->put(new_value);

  if(xref)
    xref->update();

}

IOPORT_LATCH::IOPORT_LATCH(void)
{
  break_point = 0;
  port = NULL;
  valid_iopins = 0;
  new_name("ioport");
}


//-------------------------------------------------------------------
//
//  PORTB
//-------------------------------------------------------------------

PORTB::PORTB(void)
{
  new_name("portb");
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
void PORTB::reset(RESET_TYPE r)
{
  rbpu = 0xc0; // These are the same as the upper 
               // two bits of the option register


}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void PORTB::rbpu_intedg_update(unsigned int new_configuration)
{
  int i;

  // Set the state of the interrupt bit first.
  // The reason is because it may be that the pullups will cause a
  // an i/o to toggle and hence cause an interrupt.

  if((new_configuration ^ intedg) & intedg_MASK )
    {
      intedg = new_configuration & intedg_MASK;
    }

  if((new_configuration ^ rbpu) & rbpu_MASK )
    {
      rbpu = new_configuration & rbpu_MASK;
      int drive = rbpu ? 0 : MAX_DRIVE/2;

      for(i=0; i<8; i++)
	( (IO_bi_directional_pu *)pins[i]) ->pull_up_resistor->drive = drive;


      // Update each pin that has a stimulus connect:

      int temp_value = value;
      if(stimulus_mask)
	temp_value = ( (temp_value & ~stimulus_mask) | update_stimuli());

      // If the pullup resistors are being turned on, and the I/O's are
      // inputs, and there are no stimuli attached, then drive I/O's high.

      temp_value &= stimulus_mask;
      if(drive)
	temp_value = (tris->value & ~stimulus_mask);

      if(temp_value ^ value) {
	value = temp_value;
	trace.register_write(address,value);
      }
    }

}


//-------------------------------------------------------------------
//-------------------------------------------------------------------
unsigned int PORTB::get(void)
{
  unsigned int old_value;

  old_value = value;

  //  cout << "PORTB::get()\n";
  IOPORT::get();

  int diff = old_value ^ value; // The difference between old and new

  // If portb bit 0 changed states, check to see if an interrupt should occur
  if( diff & 1)
    {
      // If the option register is selecting 'interrupt on rising edge' AND
      // the old_value was low (and hence the new high) then set  INTF
      //                    OR
      // If the option register is selecting 'interrupt on falling edge' AND
      // the old_value was high (and hence the new low) then set  INTF

      // These two statements can be combined into an exclusive or:

      if( (intedg &  intedg_MASK) ^ (old_value & 1))
	cpu14->intcon->set_intf();
    }


  // If any of the upper 4 bits of port b changed states then set RBIF
  // Note, that the interrupt on change feature only works for I/O's
  // configured as inputs.
  if(diff & 0xf0 & tris->value)
    cpu14->intcon->set_rbif();

  return value;
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void PORTB::setbit(unsigned int bit_number, bool new_value)
{
  unsigned int old_value = value;

  //cout << "PORTB::setbit() bit " << bit_number << " to " << new_value << '\n';

  IOPORT::setbit( bit_number,  new_value);

  int diff = old_value ^ value; // The difference between old and new

  // If portb bit 0 changed states, check to see if an interrupt should occur
  if( diff & 1)
    {
      // If the option register is selecting 'interrupt on rising edge' AND
      // the old_value was low (and hence the new high) then set  INTF
      //                    OR
      // If the option register is selecting 'interrupt on falling edge' AND
      // the old_value was high (and hence the new low) then set  INTF

      // These two statements can be combined into an exclusive or:
      //cout << "PORTB::setbit() - bit changed states\n";
      if( (intedg &  intedg_MASK) ^ (old_value & 1))
	cpu14->intcon->set_intf();
    }


  // If any of the upper 4 bits of port b changed states then set RBIF
  // Note, that the interrupt on change feature only works for I/O's
  // configured as inputs.
  if(diff & 0xf0 & tris->value)
    cpu14->intcon->set_rbif();
}


//-------------------------------------------------------------------
//-------------------------------------------------------------------
PORTB_62x::PORTB_62x(void)
{
  //new_name("portb");
}

unsigned int PORTB_62x::get(void)
{
  unsigned int old_value;

  old_value = value;

  //  cout << "PORTB_62x::get()\n";

  IOPORT::get();

  int diff = old_value ^ value; // The difference between old and new

  // If portb bit 0 changed states, check to see if an interrupt should occur
  if( diff & 1)
    {
      // If the option register is selecting 'interrupt on rising edge' AND
      // the old_value was low (and hence the new high) then set  INTF
      //                    OR
      // If the option register is selecting 'interrupt on falling edge' AND
      // the old_value was high (and hence the new low) then set  INTF

      // These two statements can be combined into an exclusive or:

      if( (intedg &  intedg_MASK) ^ (old_value & 1))
	cpu14->intcon->set_intf();
    }


  // If any of the upper 4 bits of port b changed states then set RBIF
  // Note, that the interrupt on change feature only works for I/O's
  // configured as inputs.
  if(diff & 0xf0 & tris->value)
    cpu14->intcon->set_rbif();

  if(ccp1con && ( diff & CCP1) )
    ccp1con->new_edge(value & CCP1);

  if( usart && (diff & RX))
    usart->new_rx_edge(value & RX);

  return value;
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void PORTB_62x::setbit(unsigned int bit_number, bool new_value)
{
  unsigned int old_value = value;

  //cout << "PORTB_62x::setbit() bit " << bit_number << " to " << new_value << '\n';

  IOPORT::setbit( bit_number,  new_value);

  int diff = old_value ^ value; // The difference between old and new

  // If portb bit 0 changed states, check to see if an interrupt should occur
  if( diff & 1)
    {
      // If the option register is selecting 'interrupt on rising edge' AND
      // the old_value was low (and hence the new high) then set  INTF
      //                    OR
      // If the option register is selecting 'interrupt on falling edge' AND
      // the old_value was high (and hence the new low) then set  INTF

      // These two statements can be combined into an exclusive or:
      //cout << "PORTB_62x::setbit() - bit changed states\n";
      if( (intedg &  intedg_MASK) ^ (old_value & 1))
	cpu14->intcon->set_intf();
    }


  // If any of the upper 4 bits of port b changed states then set RBIF
  // Note, that the interrupt on change feature only works for I/O's
  // configured as inputs.
  if(diff & 0xf0 & tris->value)
    cpu14->intcon->set_rbif();

  if(ccp1con && ( diff & CCP1) )
    ccp1con->new_edge(value & CCP1);

  if( usart && (diff & RX))
    usart->new_rx_edge(value & RX);

}



//-------------------------------------------------------------------
//  PORTA_62x::put(unsigned int new_value)
//
//  inputs:  new_value - here's where the I/O port is written (e.g.
//                       gpsim is executing a MOVWF IOPORT,F instruction.)
//  returns: none
//
//  The I/O Port is updated with the new value. If there are any stimuli
// attached to the I/O pins then they will be updated as well.
// Note that for the 62x device, the comparator module overides the
// tris output control.
//
//-------------------------------------------------------------------

void PORTA_62x::put(unsigned int new_value)
{

  if(new_value > 255)
    cout << "PIC_IOPORT::put value >255\n";
  // The I/O Ports have an internal latch that holds the state of the last
  // write, even if the I/O pins are configured as inputs. If the tris port
  // changes an I/O pin from an input to an output, then the contents of this
  // internal latch will be placed onto the external I/O pin.

  internal_latch = new_value;

  // update only those bits that are really outputs
  cout << "PORTA_62X::put trying to put " << new_value << "  Tris is " << tris->value <<'\n';

  // Bit 4 is an open collector output (it can only drive low)
  // If we're trying to drive bit 4 high and bit 4 is an output
  // then don't change the value on the I/O pin in the same state.
  // Also, bit 5 is an input regardless of the TRIS setting

  //  pin_value = value = (((new_value & 0xcf) | (new_value & value & 0x10)) & ~tris->value) 
  //    | (value & (tris->value | 0x20));

  value = ((new_value & ~tris->value) | (value & tris->value)) & valid_iopins ;

  // FIXME - this is obviously wrong
  if(comparator && comparator->enabled()) 
    value &= (0xff & ~( AN0 | AN1 | AN2 | AN3));


  cout << " IOPORT::put just set port value to " << value << '\n';

  // Update the stimuli - if there are any
  if(stimulus_mask)
    update_stimuli();

  //cout << " PORTA_62X::put port value is " << value << " after updating stimuli\n";

  trace.register_write(address,value);

}


//-------------------------------------------------------------------
//  unsigned int PORTA_62x::get(void)
//
//  inputs:  none
//
//  returns: 
//
//-------------------------------------------------------------------

unsigned int PORTA_62x::get(void)
{
  unsigned int old_value;

  old_value = value;

  if(stimulus_mask) {
    value = ( (value & ~stimulus_mask) | update_stimuli());
  }

  // If the comparator is enabled, then all of the "analog" pins are
  // read as zero.

  if(comparator && comparator->enabled()) {

    value = value & (0xff & ~( AN0 | AN1 | AN2 | AN3));

  }

  //int diff = old_value ^ value; // The difference between old and new
  // cout << " PORTA_62X::get port value is " << value << " \n";

  return value;
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void PORTA_62x::setbit(unsigned int bit_number, bool new_value)
{

  unsigned int old_value = value;

  //cout << "PORTA::setbit() bit " << bit_number << " to " << new_value << '\n';

  IOPORT::setbit( bit_number,  new_value);

  int diff = old_value ^ value; // The difference between old and new

  // If porta bit 4 changed states, check to see if tmr0 should increment
  if( diff & 0x10)
    {
      //cout << "bit 4 changed\n";

      if(cpu14->option_reg.get_t0cs())
	{
	  //   cout << "tmr 0 external clock, porta new value " << value << " t0se "<<cpu14->option_reg.get_t0se()<< '\n';
	if( ((value & 0x10) == 0) ^ (cpu14->option_reg.get_t0se() == 0))
	  cpu14->tmr0.increment();
	}
    }

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
PORTA_62x::PORTA_62x(void)
{
  new_name("porta");
}

//-------------------------------------------------------------------
//
//  PORTA
//-------------------------------------------------------------------

PORTA::PORTA(void)
{
  new_name("porta");
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void PORTA::setbit(unsigned int bit_number, bool new_value)
{

  unsigned int old_value = value;

  //cout << "PORTA::setbit() bit " << bit_number << " to " << new_value << '\n';

  IOPORT::setbit( bit_number,  new_value);

  int diff = old_value ^ value; // The difference between old and new

  // If porta bit 4 changed states, check to see if tmr0 should increment
  if( diff & 0x10)
    {
      //cout << "bit 4 changed\n";

      if(cpu14->option_reg.get_t0cs())
	{
	  //   cout << "tmr 0 external clock, porta new value " << value << " t0se "<<cpu14->option_reg.get_t0se()<< '\n';
	if( ((value & 0x10) == 0) ^ (cpu14->option_reg.get_t0se() == 0))
	  cpu14->tmr0.increment();
	}
    }

}

//-------------------------------------------------------------------
//
// PORTC
//-------------------------------------------------------------------


PORTC::PORTC(void)
{
  new_name("portc");
  usart = NULL;
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
unsigned int PORTC::get(void)
{
  unsigned int old_value;

  old_value = value;

  // cout << "PORTC::get()\n";
  IOPORT::get();

  int diff = old_value ^ value; // The difference between old and new

  // 
  if( ccp1con && (diff & CCP1) )
    ccp1con->new_edge(value & CCP1);
 
  // if this cpu has a usart and there's been a change detected on
  // the RX pin, then we need to notify the usart
  if( usart && (diff & RX))
    usart->new_rx_edge(value & RX);

  return(value);
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void PORTC::setbit(unsigned int bit_number, bool new_value)
{

  unsigned int old_value = value;

  if(verbose)
    cout << "PORTC::setbit() bit " << bit_number << " to " << new_value << '\n';

  IOPORT::setbit( bit_number,  new_value);

  int diff = old_value ^ value; // The difference between old and new

  if(ccp1con && ( diff & CCP1) )
    ccp1con->new_edge(value & CCP1);

  if( usart && (diff & RX))
    usart->new_rx_edge(value & RX);

}

//-------------------------------------------------------------------
//
//  PORTD
//-------------------------------------------------------------------

PORTD::PORTD(void)
{
  new_name("portd");
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void PORTD::setbit(unsigned int bit_number, bool new_value)
{

  unsigned int old_value = value;

  //cout << "PORTD::setbit() bit " << bit_number << " to " << new_value << '\n';

  IOPORT::setbit( bit_number,  new_value);

}

//-------------------------------------------------------------------
//
//  PORTE
//-------------------------------------------------------------------

PORTE::PORTE(void)
{
  new_name("porte");
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void PORTE::setbit(unsigned int bit_number, bool new_value)
{

  unsigned int old_value = value;

  //cout << "PORTE::setbit() bit " << bit_number << " to " << new_value << '\n';

  IOPORT::setbit( bit_number,  new_value);

}

//-------------------------------------------------------------------
//
//  PORTF
//-------------------------------------------------------------------

PORTF::PORTF(void)
{
  new_name("portf");
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void PORTF::setbit(unsigned int bit_number, bool new_value)
{

  unsigned int old_value = value;

  //cout << "PORTF::setbit() bit " << bit_number << " to " << new_value << '\n';

  IOPORT::setbit( bit_number,  new_value);

}

//-------------------------------------------------------------------
//
//  PORTG
//-------------------------------------------------------------------

PORTG::PORTG(void)
{
  new_name("portg");
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void PORTG::setbit(unsigned int bit_number, bool new_value)
{

  unsigned int old_value = value;

  //cout << "PORTG::setbit() bit " << bit_number << " to " << new_value << '\n';

  IOPORT::setbit( bit_number,  new_value);

}



