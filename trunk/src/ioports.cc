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

class SignalSource
{
};

class SignalSink
{
};

class SignalControl
{
};

/// PortModule
///
///    Data
///    Select  ======+
///                +-|-+  Outgoing
///    Source1 ===>| M |  data
///    Source2 ===>| U |=============+    
///    SourceN ===>| X |             |    
///                +---+             |    +-------+
///    Control                       +===>| IOPIN |
///    Select  ======+                    |       |
///                +-|-+  I/O Pin         |       |
///   Control1 ===>| M |  Direction       |       |<======> Physical
///   Control2 ===>| U |=================>|       |         Interface
///   ControlN ===>| X |                  |       |
///                +---+                  |       |
///    Sink Decode                   +===<|       |
///    Select  ======+               |    +-------+
///                +-|-+   Incoming  |
///    Sink1  <====| D |   data      |
///    Sink2  <====| E |<============|
///    SinkN  <====| C |
///                +---+
///
/// The PortModule models a Processor's I/O Ports. The schematic illustrates
/// the abstract description of the PortModule. Its job is to merge together
/// all of the Processor's peripherals that can control the processor's pins.
/// For example, a UART peripheral may be shared with a general purpose I/O
/// pin. The UART may have a transmit and receive pin and select whether it's
/// in control of the I/O pins. For the transmit pin, there are two data sources
/// one of which can get mapped onto the outgoing data bus. Similarly, there
/// are two control sources. The muxes select which one of these is directed
/// to the I/O pin. Similarly, if an I/O pin's state changes (either because
/// the out going data is driving it or some other device is connected to the
/// I/O pin's physical interface) then the incoming data can be directed by
/// the Decoder.
///
/// Each one of the sources can have one or more bits associated with it. So
/// all of the signal paths in the schematic can be assumed to be busses. The
/// PortModule handles most of these paths in parallel. However, the I/O pins
/// are bit-specific.
///

class PortModule
{
public:

  /// The SignalSource list is a list of all sources that can drive data
  list <SignalSource *> sources;

  /// The SignalSink list is a list of all sinks that can receive data
  list <SignalSink *> sinks;

  /// The SignalControl list is a list of all objects that control the output
  /// data direction.
  list <SignalControl *> controls;


  void add(SignalSource *);
  void add(SignalSink *);
  void add(SignalControl *);

  /// The iopins are the physical pins associated with the port.
  IOPIN **iopins;

private:
  unsigned int sourceState;
  unsigned int sinkState;
  unsigned int controlState;
};

class PortRegister : public Register
{
public:

  virtual void put(unsigned int new_value);
  virtual unsigned int get(unsigned int new_value);

};

class PortLatch : public Register
{
public:

private:
  PortRegister *port_register;
};

/// MuxSource - A single input to a Multiplexer.
///  A MuxSource is a pure virtual base class designed to be 
///  one of the base classes in in a multiple inheritance 
///  class design.
///
class MuxSource
{
public:
  virtual unsigned int mGet()=0;
private:
};

/// MuxControl - The control for a 2:1 multiplexer.
class MuxControl
{
public:
  virtual bool bGet()=0;
private:
};

/// Mux2_1 - A 2:1 multiplexer

class Mux2_1
{
public:
  Mux2_1(MuxSource *srcA, MuxSource *srcB, MuxControl *cont, unsigned int m);
  unsigned int get();
private:
  MuxSource *a;
  MuxSource *b;
  MuxControl *control;
  unsigned int valid_bits;
};


class DecoderSink
{
public:
  virtual void dPut(unsigned int)=0;
};
class DecoderControl
{
public:
  virtual bool bDecControl()=0;
};
class Decoder1_2
{
public:
  Decoder1_2();
  void put(unsigned int);
private:
  DecoderSink *a;
  DecoderSink *b;
  DecoderControl *control;
  unsigned int valid_bits;
};


//-------------------------------------------------------------------
Mux2_1::Mux2_1(MuxSource *srcA, MuxSource *srcB, MuxControl *cont, unsigned int m)
  : a(srcA), b(srcB), control(cont), valid_bits(m)
{
}
//-------------------------------------------------------------------
// Mux2_1::get()
//
// The mux output is driven from one of 2 inputs
//-------------------------------------------------------------------
unsigned int Mux2_1::get()
{
  return (control->bGet() ? a->mGet() : b->mGet()) & valid_bits;
}



//-------------------------------------------------------------------
//-------------------------------------------------------------------
// Experiment:

//class PortARegister : public Register , public 

//-------------------------------------------------------------------
//
// IOPORT::update_stimuli
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

  unsigned int v = value.get();

  return v ^ get_value();
}



//-------------------------------------------------------------------
//-------------------------------------------------------------------
double IOPORT::get_bit_voltage(unsigned int bit_number)
{

  double v;

  if(pins[bit_number]) {
    if(pins[bit_number]->snode) {
      cout << "Warning IOPORT::get_bit_voltage has changed\n";
      v = pins[bit_number]->snode->get_nodeVoltage();
    }
    else
      v = pins[bit_number]->get_Vth();
  }
  else
    v = (value.get() &  (1<<bit_number)) ?  5.0 : 0.0;


  return v;
}      

//-------------------------------------------------------------------
//-------------------------------------------------------------------
bool IOPORT::get_bit(unsigned int bit_number)
{
  //cout << "get_bit, latch " << internal_latch << " bit " << bit_number << endl;
  return (internal_latch &  (1<<bit_number )) ? true : false;

}

//-------------------------------------------------------------------
//  IOPORT::get_value(void)
//
//   inputs:  none
//  returns:  the current state of the ioport
//
// If there are stimuli attached to the iopins, then their current
// state is obtained. If there aren't any attached, then the last
// value written to the ioport is obtained.
//
//-------------------------------------------------------------------

unsigned int IOPORT::get_value(void)
{

  // Update the stimuli - if there are any

  if(stimulus_mask) {

    unsigned int current_value = value.get();

    unsigned int i=0;
    unsigned int m=1;

    for(i=0; i<num_iopins; i++, m<<=1) {
      if(pins[i] && pins[i]->snode) {

	double v = pins[i]->snode->get_nodeVoltage();

	if(current_value & m) {
	  // this io bit is currently a high
	  if(v <= pins[i]->h2l_threshold)
	    current_value ^= m;
	} else
	  if (v > pins[i]->l2h_threshold)
	    current_value ^= m;
      }
    }

    value.put(current_value);

  }

  return(value.get());
}
//-------------------------------------------------------------------
//  IOPORT::get(void)
//
//   inputs:  none
//  returns:  the current state of the ioport
//
// get is identical to get_value except that tracing is performed.
//
//-------------------------------------------------------------------

unsigned int IOPORT::get(void)
{

  trace.raw(read_trace.get() | value.get());
  return get_value();
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

  trace.raw(write_trace.get() | value.get());

  unsigned int current_value = value.get();

  value.put(new_value);

  if(stimulus_mask && (current_value != new_value)) {

    unsigned int diff = current_value ^ new_value;

    guint64 time = get_cycles().value;

    // Update all I/O pins that have stimuli attached to
    // them and their state is being changed by this put() operation.

    for(unsigned int i = 0; i<num_iopins; i++,diff>>=1)
      if((diff&1) && pins[i] && pins[i]->snode)
	pins[i]->snode->update(time);
  }

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
  unsigned int i,j;
  unsigned int old_value = value.get();
  unsigned int diff;

 
  value.put(new_value);

  // Update the stimuli - if there are any
  if(stimulus_mask)
    update_stimuli();

  
  update();
  
  // Find the pins that have changed states
  diff = (old_value ^ value.get()) & valid_iopins;

  // Update the cross references for each pin that has changed.
  for(i=0,j=1; i<num_iopins; i++,j<<=1) {

    if((j & diff) && pins[i])
      pins[i]->update();

  }

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void IOPORT::setbit(unsigned int bit_number, bool new_value)
{

  int bit_mask = 1<<bit_number;
  unsigned int current_value = value.get();
  bool current_bit_value = (current_value & bit_mask) ? true : false;

  if( current_bit_value != new_value)
    {
      trace_register_write();
      value.put(current_value ^ bit_mask);

      internal_latch = (current_value & bit_mask) | (internal_latch & ~bit_mask);
    }

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void IOPORT::change_pin_direction(unsigned int bit_number, bool new_direction)
{

  cout << " IOPORT::" << __FUNCTION__ <<'(' << bit_number << ',' << new_direction << ") doesn't do anything.\n";

}

//-------------------------------------------------------------------
// getIO(unsigned int pin_number)
//  return the I/O pin at the bit position requested.
//-------------------------------------------------------------------
IOPIN *IOPORT::getIO(unsigned int pin_number)
{
  if(pins && pin_number < num_iopins)
    return pins[pin_number];

  return 0;
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
	 <<") is invalid for " << name() << ". Max iopins " << num_iopins << '\n';

  if(verbose)
    cout << "attaching iopin to ioport " << name() << '\n';
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void IOPORT::attach_stimulus(stimulus *new_stimulus, unsigned int bit_position)
{

  if(pins  && (bit_position < num_iopins) && pins[bit_position]) {

    stimulus_mask |= (1<<bit_position);

    if(pins[bit_position]->snode == 0)
      {
	// If this I/O pin is not attached to a node yet, 
	// then create a node and attach it.

	pins[bit_position]->snode = new Stimulus_Node();
	pins[bit_position]->snode->attach_stimulus(pins[bit_position]);
      }

    // attach the new stimulus to the same node as this I/O pin's

    pins[bit_position]->snode->attach_stimulus(new_stimulus);
  }

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void IOPORT::attach_node(Stimulus_Node *new_node, unsigned int bit_position)
{

  if(pins[bit_position])
    {
      stimulus_mask |= (1<<bit_position);
      //      pins[bit_position]->snode == new_node;
    }
  else
    cout << "Error: attaching node to a non-existing I/O pin.\n";

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
  //cout << name() << "::put 0x" << hex << new_value << endl;

  RegisterValue oldValue = value;
  IOPORT::put(new_value);
  check_peripherals(oldValue);
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
void PIC_IOPORT::change_pin_direction(unsigned int bit_number, bool new_direction)
{

  if(tris)
    tris->setbit(bit_number, new_direction);
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

  unsigned int diff = tris->value.get() ^ new_tris;

  if(diff)
    {
      // Update the I/O port value to that of the internal latch
      value.put((value.get() & ~diff) | (internal_latch & diff));

      // Go through and update the direction of the I/O pins
      unsigned int i,m;
      for(i = 0, m=1; i<num_iopins; i++, m <<= 1)
	if((m & diff) && pins[i])
	  pins[i]->update_direction(m & (~new_tris));

      // Now, update the nodes to which the(se) pin(s) may be attached

      guint64 time = get_cycles().value;
      for(i = 0, m=1; i<num_iopins; i++, m <<= 1)
	if(stimulus_mask & m & diff)
          if(pins[i] && pins[i]->snode!=0)
            pins[i]->snode->update(time);
    }
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
unsigned int PIC_IOPORT::get(void)
{
  RegisterValue oldValue = value;
  IOPORT::get();
  check_peripherals(oldValue);

  return value.get();
}


//-------------------------------------------------------------------
//-------------------------------------------------------------------
bool PIC_IOPORT::get_bit(unsigned int bit_number)
{
  //cout << "get_bit, latch " << internal_latch << " bit " << bit_number << endl;
  return (internal_latch &  (1<<bit_number )) ? true : false;

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void PIC_IOPORT::setbit(unsigned int bit_number, bool new_value)
{
  RegisterValue oldValue = value;
  IOPORT::setbit( bit_number,  new_value);
  check_peripherals(oldValue);
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
PIC_IOPORT::PIC_IOPORT(unsigned int _num_iopins) : IOPORT(_num_iopins)
{
  tris = 0;
  latch = 0;


  latch_data_out = 0;
  peripheral_data_out = 0;
  data_out_select = 0;

  latch_tris_out = 0;
  peripheral_tris_out = 0;
  tris_out_select = 0;

  data_in = 0;
  peripheral_data_in = 0;

}

//-------------------------------------------------------------------
// trace_register_write
//   - a wrapper for trace.register_write
// This provides an option for IOPORTs derived from the IOPORT class 
// to override the behavior of IOPORT traces.
//-------------------------------------------------------------------
void IOPORT::trace_register_write(void)
{
  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());
}

IOPORT::IOPORT(unsigned int _num_iopins)
  : sfr_register()
{
  stimulus_mask = 0;
  num_iopins = _num_iopins;
  address = 0;
  value.put(0);
  internal_latch = 0;

  pins = (IOPIN **) new char[sizeof (IOPIN *) * num_iopins];

  for(unsigned int i=0; i<num_iopins; i++)
    pins[i] = 0;

  new_name("ioport");
}

IOPORT::~IOPORT()
{
    for(unsigned int i=0; i<num_iopins; i++)
    {
	if(pins[i] != 0)
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

  trace.raw(read_trace.get() | value.get());

  return(value.get());
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void IOPORT_TRIS::put(unsigned int new_value)
{
  int save_port_latch = port->internal_latch;

  trace.raw(write_trace.get() | value.get());

  port->update_pin_directions(new_value);

  value.put(new_value);

  port->put(save_port_latch);


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

  int diff = port->valid_iopins & (1<<bit_number) & (value.get() ^ (new_value << bit_number));

  if(diff) {

    trace.raw(write_trace.get() | value.get());
    //trace.register_write(address,value.get());

    port->update_pin_directions(value.get() ^ diff);

    value.put(value.get() ^ diff);

    update();
    port->pins[bit_number]->update();

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

  update();

  port->update();

  for(unsigned int i=0; i<port->num_iopins; i++)
    port->pins[i]->update();

}

IOPORT_TRIS::IOPORT_TRIS(void)
{
  port = 0;
  valid_iopins = 0;
  new_name("ioport");
}

//-------------------------------------------------------------------
// IOPORT_LATCH
//
//-------------------------------------------------------------------

unsigned int IOPORT_LATCH::get(void)
{

  trace.raw(read_trace.get() | value.get());
  //trace.register_read(address,value.get());

  return(value.get());
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void IOPORT_LATCH::put(unsigned int new_value)
{

  if(verbose)
    cout << "IOPORT_LATCH::put 0x"<<hex<<new_value<<'\n';

  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());

  value.put(new_value);

  port->put(value.get());


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
  update();

}

IOPORT_LATCH::IOPORT_LATCH(void)
{
  port = 0;
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

  // Set the state of the interrupt bit first.
  // The reason is because it may be that the pullups will cause a
  // an i/o to toggle and hence cause an interrupt.

  if((new_configuration ^ intedg) & intedg_MASK )
    intedg = new_configuration & intedg_MASK;

  if((new_configuration ^ rbpu) & rbpu_MASK ){

    // Save the state of the I/O pins before any (potential) changes
    // are made.

    unsigned int old_value = value.get();

    rbpu = new_configuration & rbpu_MASK;
    bool pullup = rbpu ? false : true;

    // Update each I/O pin's pullup resistor.
    for(unsigned int i=0; i<num_iopins; i++) {
      pins[i]->update_pullup(pullup);
      if(pins[i]->snode)
	pins[i]->snode->update();
    }

    // Get the new state of each I/O pin 

    unsigned int temp_value = get_value();

    // If the pullup resistors are being turned on, and the I/O's are
    // inputs, and there are no stimuli attached, then drive I/O's high.

    temp_value &= stimulus_mask;
    if(pullup)
      temp_value |= (tris->value.get() & ~stimulus_mask);

    // If anything changed, then we record the new state.
    if(temp_value ^ old_value) {
      trace.raw(write_trace.get() | old_value);
      value.put(temp_value);
    }
  }

}


//-------------------------------------------------------------------
//-------------------------------------------------------------------
void PORTB::check_peripherals(RegisterValue oldValue)
{

  int diff = oldValue.get() ^ value.get(); // The difference between old and new

  // If portb bit 0 changed states, check to see if an interrupt should occur
  if( diff & 1)
    {
      // If the option register is selecting 'interrupt on rising edge' AND
      // the old_value was low (and hence the new high) then set  INTF
      //                    OR
      // If the option register is selecting 'interrupt on falling edge' AND
      // the old_value was high (and hence the new low) then set  INTF

      bool bInterruptOnRisingEdge = (intedg &  intedg_MASK) ? true : false;
      bool bRisingEdge = (value.get() & 1) ? true : false;

      if(verbose)
	cout << "RB0 changed\n";
      if(bInterruptOnRisingEdge == bRisingEdge) {
	if(verbose)
	  cout << "Now setting intf\n";

	cpu14->intcon->set_intf();
      }
    }


  // If any of the upper 4 bits of port b changed states then set RBIF
  // Note, that the interrupt on change feature only works for I/O's
  // configured as inputs.
  if(diff & 0xf0 & tris->value.get())
    cpu14->intcon->set_rbif();


}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
unsigned int PORTB::get(void)
{
  //  cout << "PORTB::get  -- value at entry 0x" << hex << value.get() << endl;

  RegisterValue oldValue = value;

  IOPORT::get();

  check_peripherals(oldValue);

  //  cout << "PORTB::get  -- value at exit 0x" << hex << value.get() << endl;

  return value.get();
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void PORTB::setbit(unsigned int bit_number, bool new_value)
{
  RegisterValue oldValue = value;
  IOPORT::setbit( bit_number,  new_value);
  check_peripherals(oldValue);
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void PORTB::put(unsigned int new_value)
{
  PIC_IOPORT::put(new_value);
  
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

  old_value = value.get();

  IOPORT::get();

  int diff = old_value ^ value.get(); // The difference between old and new

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
  if(diff & 0xf0 & tris->value.get())
    cpu14->intcon->set_rbif();

  if(ccp1con && ( diff & CCP1) )
    ccp1con->new_edge(value.get() & CCP1);

  if( usart && (diff & RX))
    usart->new_rx_edge(value.get() & RX);

  return value.get();
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void PORTB_62x::setbit(unsigned int bit_number, bool new_value)
{
  unsigned int old_value = value.get();

  IOPORT::setbit( bit_number,  new_value);

  int diff = old_value ^ value.get(); // The difference between old and new

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
  if(diff & 0xf0 & tris->value.get())
    cpu14->intcon->set_rbif();

  if(ccp1con && ( diff & CCP1) )
    ccp1con->new_edge(value.get() & CCP1);

  if( usart && (diff & RX))
    usart->new_rx_edge(value.get() & RX);

}


void PORTB_62x::check_peripherals(RegisterValue oldValue)
{
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

  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());

  // The I/O Ports have an internal latch that holds the state of the last
  // write, even if the I/O pins are configured as inputs. If the tris port
  // changes an I/O pin from an input to an output, then the contents of this
  // internal latch will be placed onto the external I/O pin.

  internal_latch = new_value;

  // update only those bits that are really outputs
  //cout << "PORTA_62X::put trying to put " << new_value << "  Tris is " << tris->value <<'\n';

  // Bit 4 is an open collector output (it can only drive low)
  // If we're trying to drive bit 4 high and bit 4 is an output
  // then don't change the value on the I/O pin in the same state.
  // Also, bit 5 is an input regardless of the TRIS setting

  value.put(((new_value & ~tris->value.get()) | (value.get() & tris->value.get())) & valid_iopins);

  // FIXME - this is obviously wrong
  if(comparator && comparator->enabled()) 
    value.put(value.get() & (0xff & ~( AN0 | AN1 | AN2 | AN3)));


  // Update the stimuli - if there are any
  if(stimulus_mask)
    update_stimuli();

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

  old_value = value.get();

  if(stimulus_mask) {
    value.put(  (value.get() & ~stimulus_mask) | update_stimuli());
  }

  // If the comparator is enabled, then all of the "analog" pins are
  // read as zero.

  if(comparator && comparator->enabled()) {

    value.put(value.get() & (0xff & ~( AN0 | AN1 | AN2 | AN3)));

  }

  int diff = old_value ^ value.get(); // The difference between old and new
  
  if( ssp && (diff & SS))
    ssp->new_ss_edge(value.get() & SS);
  
  // cout << " PORTA_62X::get port value is " << value << " \n";

  return value.get();
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void PORTA_62x::setbit(unsigned int bit_number, bool new_value)
{

  unsigned int old_value = value.get();


  IOPORT::setbit( bit_number,  new_value);

  int diff = old_value ^ value.get(); // The difference between old and new

  // If porta bit 4 changed states, check to see if tmr0 should increment
  if( diff & 0x10)
    {

      if(cpu14->option_reg.get_t0cs())
	{

	if( ((value.get() & 0x10) == 0) ^ (cpu14->option_reg.get_t0se() == 0))
	  cpu14->tmr0.increment();
	}
    }

  if( ssp && (diff & SS) ) {
	  ssp->new_ss_edge(new_value ? SS : 0);
  }
  
}
void PORTA_62x::check_peripherals(RegisterValue oldValue)
{
}


//-------------------------------------------------------------------
//-------------------------------------------------------------------
PORTA_62x::PORTA_62x(void)
{
  new_name("porta");
  comparator = 0;
  ssp = 0;
}

//-------------------------------------------------------------------
//
//  PORTA
//-------------------------------------------------------------------

PORTA::PORTA(void)
{
  new_name("porta");
  ssp = 0;
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void PORTA::check_peripherals(RegisterValue oldValue)
{
  int diff = oldValue.get() ^ value.get(); // The difference between old and new

  // If porta bit 4 changed states, check to see if tmr0 should increment
  if( diff & 0x10) {

    if(cpu14->option_reg.get_t0cs()) {

      bool bClockOnRisingEdge = cpu14->option_reg.get_t0se() ? false : true;
      bool bRisingEdge = (value.get() & 0x10) ? true : false;

      if(verbose)
	cout << "PORTA check peripherals RA4 changed\n";
      if(bClockOnRisingEdge == bRisingEdge) {
	cpu14->tmr0.increment();
      }
    }
  }

  if( ssp && (diff & SS) ) {
    ssp->new_ss_edge( (value.get() & SS) ? SS : 0);
  }
  

}

unsigned int PORTA::get(void)
{

  RegisterValue oldValue = value;

  IOPORT::get();

  check_peripherals(oldValue);

  return(value.get());
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void PORTA::setbit(unsigned int bit_number, bool new_value)
{

  RegisterValue oldValue = value;
  IOPORT::setbit( bit_number,  new_value);
  check_peripherals(oldValue);

}

//-------------------------------------------------------------------
//
// PORTC
//-------------------------------------------------------------------


PORTC::PORTC(void)
{
  new_name("portc");
  usart = 0;
  ssp = 0;
  tmrl = 0;
  ccp1con = 0;
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
unsigned int PORTC::get(void)
{
  RegisterValue oldValue = value;
  IOPORT::get();
  check_peripherals(oldValue);

  return(value.get());

}

void PORTC::check_peripherals(RegisterValue oldValue)
{
  int diff = oldValue.get() ^ value.get(); // The difference between old and new

  if( ccp1con && (diff & CCP1) )
    ccp1con->new_edge(value.get() & CCP1);
 
  // if this cpu has a usart and there's been a change detected on
  // the RX pin, then we need to notify the usart
  if( usart && (diff & RX))
    usart->new_rx_edge(value.get() & RX);

  if( ssp && (diff & SCK))
    ssp->new_sck_edge(value.get() & SCK);

  if(tmrl && (diff & T1CKI))
    tmrl->increment();

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void PORTC::setbit(unsigned int bit_number, bool new_value)
{

  if(verbose)
    cout << "PORTC::setbit() bit " << bit_number << " to " << new_value << '\n';

  PIC_IOPORT::setbit( bit_number,  new_value);

  if(verbose)
    cout << "PORTC::setbit() bit " 
	 << bit_number << " is done new value is "
	 << ( (value.get() & (1<<bit_number)) ? "high" : "low")
	 << endl;
  
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
void PORTD::check_peripherals(RegisterValue oldValue)
{
}

//-------------------------------------------------------------------
//
//  PORTE
//-------------------------------------------------------------------

PORTE::PORTE(void)
{
  new_name("porte");
}
void PORTE::check_peripherals(RegisterValue oldValue)
{
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
void PORTF::check_peripherals(RegisterValue oldValue)
{
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
void PORTG::check_peripherals(RegisterValue oldValue)
{
}

