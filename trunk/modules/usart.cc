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

  usart.cc

  This is gpsim's universal synchronous/asynchronous receiver/transceiver.

  Features:

    8 or 9 bit receiver and transmitter
    0 or 1 start bits
    0 or 1 stop bits
    0 or 1 parity bits and even/odd selectable
    variable sized transmit and receive buffers

*/


#include <errno.h>
#include <stdlib.h>
#include <string>
#include <math.h>  // for floor()

#include "../config.h"    // get the definition for HAVE_GUI

#ifdef HAVE_GUI
#include <gtk/gtk.h>
#endif



#include "../src/attribute.h"
#include "../src/modules.h"
#include "../src/packages.h"
#include "../src/stimuli.h"
#include "../src/ioports.h"
#include "../src/symbol.h"
#include "../src/interface.h"
#include "../src/pir.h"

#include "usart.h"


void  gpsim_set_break(guint64 next_cycle, BreakCallBack *f=NULL);
void  gpsim_set_break_delta(guint64 delta, BreakCallBack *f=NULL);
pic_processor *gpsim_get_active_cpu(void);
guint64 gpsim_get_current_time(void);
guint64 gpsim_digitize_time(double time);

/**********************************************************************************

             gpsim's USART module


  The  USART module is a general purpose universal synchronous/asynchronous
serial receiver and transmitter. In other words, it's a serial port. It's 
purpose is to provide a tool to assist in the debugging of serial interfaces.
Users can load this module and tie it to their receive and transmit pins
of their simulated PIC's. Then experiments can be conducted on things like
baud rate variation, transmit inundation, protocol development, etc.

The design
of this dynamically loadable module mimics the USART peripheral found in PIC
microcontrollers. In fact, the USARTModule class is derived from the USART_MODULE
class that is instantiated by simulated PIC's. There are some notable differences,
however. For example, the registers from which the usart is constructed behave
differently. Most notably, the spbrg (serial port baud rate generator) is not
confined to the limited number of discrete baud rates.


**********************************************************************************/



/**********************************************************************
 * boolean event logging
 *
 * The boolean event logger is a simple class for logging the time
 * of boolean (i.e. 0/1) events.
 *
 * The event buffer is an array of 64-bit wide elements. Each element
 * stores the time at which an event occurs. The state of the event
 * is encoded in the position of the array. In other words, "high"
 * events are at the odd indices of the array and "low" ones at the
 * even ones.
 *
 * No effort is made to compress the 64-bit time entries into smaller
 * values. Consequently, a large amount of space is wasted. 
 *
 * Repeated events are not logged. E.g.. if two 1's are logged, the 
 * second one is ignored.
 * 
 * The total number of events is defined when the class is instantiated.
 * The only requirement is that the number of events be an even power
 * of 2. A check for this is made, and 
 */

class BoolEventLogger {
public:

#define EL_MAX_EVENTS  1024

  guint32  index;               // Index into the buffer
  guint64  *buffer;             // Where the time is stored
  guint32  max_events;          // Size of the event buffer

  /*
    boolean_event(bool state)

    Record a 0/1 event (e.g. the state of an I/O line).
   */

  inline void event(bool state)
  {
    // If the new event is different the most recently logged one
    // then we need to log this event. (Note that the event is implicitly
    // logged in the "index". I.e. 1 events are at odd indices.
    if(state ^ (index & 1))  {
      index = (index + 1) & max_events;
      //cout << "New event " << state << "  index = " << index << '\n';
      buffer[index] = gpsim_get_current_time();
    }

  }

  void dump(int start_index, int end_index=-1) {

    
    if((start_index > max_events) || (start_index <= 0 ))
      start_index = 0;

    if(end_index == -1)
      end_index = index;

    if(start_index == end_index)
      return;

    // Loop through and dump events between the start and end points requested

    do {
      cout << hex << "0x" << start_index << " = 0x" << buffer[start_index];

      if(start_index & 1)
	cout << ": hi\n";
      else
	cout << ": lo\n";

      start_index = (start_index + 1) & max_events;

    }while ( start_index != end_index);

  }

  void dump_ASCII_art(guint64 time_step, int start_index, int end_index=-1) {

    cout << "ascii art\n";

    if((start_index > max_events) || (start_index <= 0 ))
      start_index = 0;

    if(buffer[start_index] == 0) 
      start_index = 0;

    if( (end_index > max_events) || (end_index <= 0 ))
      end_index = index;

    if(start_index == end_index)
      return;

    if(time_step == 0)
      time_step = 1;

    // Loop through and dump events between the start and end points requested

    guint64 min_pulse = buffer[end_index] - buffer[start_index];
    guint32 i = start_index;
    guint32 j = (start_index+1) & max_events;

    do {

      if(  (buffer[j] - buffer[i]) < min_pulse )
	min_pulse = (buffer[j] - buffer[i]);

      i = j;
      j = ++j & max_events; 

    }while (j != end_index);

    cout << "minimum pulse width :" << min_pulse << '\n';

    if(min_pulse == 0) { // bummer - there's an error in the log
      min_pulse = 1;
      cout << "log error - minimum pulse width shouldn't be zero\n";
    }

    int num_chars = 0;
    guint64 t = buffer[start_index];
    i = start_index;
    do {
      j = get_index(t);
      switch(j-i) {
      case 0:
      case 1:
	if(i&1)
	  cout <<'-';
	else
	  cout <<'_';
	break;
      case 2:
	cout << '|';
	break;
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
      case 9:
	cout << (j-i);
	break;
      default:
	cout << '*';
      }
      i = j;
      t += time_step;
    } while( t<=buffer[end_index] && num_chars++<1000);
    cout << "\nend of ASCII art\n";

  }
  /*
    get_index - return the current index

    This is used by the callers to record where in the event
    buffer a specific event is stored. (e.g. The start bit
    of a usart bit stream.)
   */
  inline unsigned int get_index(void) {
    return index;
  }

  inline unsigned int get_index(guint64 event_time) {
    guint32 start_index, end_index, search_index, bstep;

    end_index = index;
    start_index = (index + 1) & max_events;

    bstep = (max_events+1) >> 1;
    search_index = (start_index + bstep) & max_events;
    bstep >>= 1;

    // Binary search for the event time:
    do {
      if(event_time < buffer[search_index])
	search_index = (search_index - bstep) & max_events;
      else
	search_index = (search_index + bstep) & max_events;

      //cout << hex << "search index "<< search_index << "  buffer[search_index] " << buffer[search_index] << '\n';
      bstep >>= 1;

    } while(bstep);

    if(event_time >= buffer[search_index])
      return search_index;
    else
      return (--search_index & max_events);

  }

  unsigned int get_event(int index) {

    return index & 1;
  }

  bool get_state(guint64 event_time) {
    return get_index(event_time) & 1;
  }

  int get_edges(guint64 start_time, guint64 end_time) {
    return ( get_index(end_time) - get_index(start_time) ) & max_events;
  }

  BoolEventLogger(guint32 _max_events = 4096) {

    max_events = _max_events;

    // Make sure that max_events is an even power of 2
    if(max_events & (max_events - 1)) {
      max_events <<= 1;
      while(1) {
	if(max_events && (max_events & (max_events-1)))
	  max_events &= max_events - 1;
	else
	  break;

      }
    } else if(!max_events)
      max_events = 4096;
    
    buffer = new guint64[max_events];

    max_events--;  // make the max_events a mask

    index = 0;

  }
};

//--------------------------------------------------------------
//
//  BRG
//
// Serial Port Baud Rate Generator
//
//
//--------------------------------------------------------------

class BRG       //  : public BreakCallBack
{
public:
  double baud;
  guint64 time_per_bit;
  guint64 last_time;
  guint64 start_time;
  guint64 future_time;


  BRG(void) {
    set_baud_rate(125000); // Default baud rate.
    
  }

#if 0
  virtual void callback(void) {
    cout << "SPBRG::" << __FUNCTION__ << " is not implemented\n";

    //pic_processor *cpu = gpsim_get_active_cpu();
    last_time = gpsim_get_current_time();

    cout << "SPBRG rollover at cycle " << last_time << '\n';

    //if(rcsta->value & _RCSTA::SPEN) {
      // If the serial port is enabled, then set another 
      // break point for the next clock edge.
      get_next_cycle_break();

      //}

  };
#endif

  virtual void start(void) {

    last_time = gpsim_get_current_time();
    start_time = last_time;

    //future_time = last_time + time_per_bit;

    //gpsim_set_break(future_time, this);
    //get_next_cycle_break();


    //if(verbose)
    //cout << "SPBRG::start   last_cycle = " << 
    //  hex << last_time << " future_cycle = " << future_time << '\n';

  };

#if 0
  virtual void get_next_cycle_break(void) {
    future_time = last_time + time_per_bit;

    gpsim_set_break(future_time, this);


  };
#endif

  void set_baud_rate(double new_baud) {
    //cout << "SPBRG::" << __FUNCTION__ << "\n";

    baud = new_baud;

    pic_processor *cpu = gpsim_get_active_cpu();
    if(cpu && baud>0.0) {

      time_per_bit = cpu->time_to_cycles(1.0/baud);

    }
  };

#if 0
  virtual guint64 get_cpu_cycle(unsigned int edges_from_now) {
    cout << "SPBRG::" << __FUNCTION__ << " is not implemented\n";
  };
#endif

};





//--------------------------------------------------------------
//
//

class USART_RXPIN : public IO_input
{
public:

  USART_CORE *usart;

  USART_RXPIN(void) {
    //cout << "USART_RXPIN constructor - do nothing\n";
  }
  USART_RXPIN (USART_CORE *_usart,
	       IOPORT *i, 
	       unsigned int b, 
	       char *opt_name=NULL) : IO_input(i,b,opt_name) { 

    usart = _usart;

    // Let the pin think it's in the high state. If this is wrong,
    // then the I/O pin driving it will correct it. (Starting off
    // low prevents the start bit from being captured.) 
    // Note, may want to add a flag that indicates if the pin
    // has ever been driven at all. This way, we can capture the
    // first edge. Or we could add another parameter to the constructor.

    digital_state = 1;

  };

  virtual void put_node_state(int new_state) {

    //cout << "USART_RXPIN put_node_state " << new_state << '\n';

    state = new_state;

    if( state < h2l_threshold) {
      state = l2h_threshold + 1;
      put_digital_state(0);

    } else {

      if(state > l2h_threshold) {
	state = h2l_threshold - 1;
	put_digital_state(1);
      }
    }

  }

  void put_digital_state(bool new_dstate) { 
    bool diff = new_dstate ^ digital_state;
    digital_state = new_dstate;

    //cout << "usart rx put_digital_state " << digital_state << '\n';
    if( usart && diff ) {

      usart->new_rx_edge(digital_state);

      if(iop) // this check should not be necessary...
	iop->setbit(iobit,digital_state);


    }

  }

  //virtual void put_state( int new_state);
  virtual int get_voltage(guint64 current_time) {
    return 42;
  }
};




//--------------------------------------------------------------
//
//

class USART_TXPIN : public IO_bi_directional
{
public:

  USART_CORE *usart;

  USART_TXPIN(void) {
    //cout << "USART_TXPIN constructor - do nothing\n";
  }
  USART_TXPIN (USART_CORE *_usart,
	       IOPORT *i, 
	       unsigned int b, 
	       char *opt_name=NULL) : IO_bi_directional(i,b,opt_name) { 

    usart = _usart;

    digital_state = 1;
    update_direction(1);   // Make the TX pin an output.

  };

  virtual void put_node_state(int new_state) {

    // cout << "USART_TXPIN put_node_state " << new_state << '\n';
    /*
    state = new_state;

    if( state < h2l_threshold) {
      state = l2h_threshold + 1;
      put_digital_state(0);

    } else {

      if(state > l2h_threshold) {
	state = h2l_threshold - 1;
	put_digital_state(1);
      }
    }
    */
  }

  void put_digital_state(bool new_dstate) { 

    //cout << "usart tx put_digital_state " << new_dstate << '\n';
    /*
    bool diff = new_dstate ^ digital_state;
    digital_state = new_dstate;

    cout << "usart tx put_digital_state " << digital_state << '\n';
    if( usart && diff ) {

      usart->new_rx_edge(digital_state);

      if(iop) // this check should not be necessary...
	iop->setbit(iobit,digital_state);
    }
    */

  }

  void put_state( int new_digital_state) {

    Register *port = get_iop();

    if(port) {
      // If the new state to which the stimulus is being set is different than
      // the current state of the bit in the ioport (to which this stimulus is
      // mapped), then we need to update the ioport.

      if((new_digital_state!=0) ^ ( port->value & (1<<iobit))) {

	port->setbit(iobit,new_digital_state);

	digital_state = new_digital_state;
	// If this stimulus is attached to a node, then let the node be updated
	// with the new state as well.
	if(snode)
	  snode->update(0);
	// Note that this will auto magically update
	// the io port.

      }
    }
  }

  virtual int get_voltage(guint64 current_time) {
    //cout << "USART_TXPIN::" <<__FUNCTION__ <<  " digital state=" << digital_state << '\n';
    
    if(digital_state)
      return drive;
    else
      return -drive;
  }

};





//=================================================================
//
//  TXREG
//
//  Create a transmit register based upon the transmit register
// defined in the main gpsim code.
//

class TXREG : public BreakCallBack
{
 public:
  USART_TXPIN *txpin;
  USART_CORE  *usart;
  //BRG         *brg;

  bool empty_flag;
  double baud;

  guint64 time_per_bit;
  guint64 last_time;
  guint64 start_time;
  guint64 future_time;

  unsigned int start_bit_time;
  unsigned int start_bit_index;
  bool last_bit;
  int bits_per_byte;

  double  stop_bits;
  guint64 time_per_packet;

  unsigned int txr;    // Transmit register
  int bit_count;       // used while transmitting.
  unsigned int tx_byte;

  enum TX_STATES {
    TX_TRANSMITTING
  }  transmit_state;

  bool use_parity;
  bool parity;         // 0 = even, 1 = odd


  virtual bool is_empty(void) { return empty_flag;};
  virtual void empty(void) {empty_flag = 0;};
  virtual void full(void)  {empty_flag = 1;};
  virtual void assign_pir_set(PIR_SET *new_pir_set){};

  TXREG(void) {
    txpin = NULL;

    baud = 9600;
    bits_per_byte = 8;
    stop_bits = 1;
    use_parity = 0;

    tx_byte = '0';

    update_packet_time();

  }

  void set_bits_per_byte(int num_bits) {
    bits_per_byte = num_bits;
    update_packet_time();
  }



  void update_packet_time(void) {
    // for now the stop bit time is included in the total packet time

    if(baud <= 0.0)
      baud = 9600;  //arbitrary

    /*
      Calculate the total time to send a "packet", i.e. start bit, data, parity, and stop
    */

    time_per_packet = gpsim_digitize_time( (1.0 +                 // start bit
					    bits_per_byte +       // data bits
					    stop_bits  +          // stop bit(s)
					    use_parity)           //
					    /baud);
    time_per_bit = gpsim_digitize_time( 1.0/baud );
    //cout << "update_packet_time ==> 0x" << hex<< time_per_packet << "\n";
  }

  void set_baud_rate(double new_baud) {
    //cout << "TXREG::" << __FUNCTION__ << "\n";

    baud = new_baud;
    update_packet_time();

  };

  void set_stop_bits(double new_stop_bits) {

    stop_bits = new_stop_bits;

  }

  void set_noparity(void) {
    use_parity = 0;
  }

  void set_parity(bool new_parity) {
    use_parity = 1;
    parity = new_parity;
  }



  virtual void callback(void) {
    if(0) {
      cout << "\n\n";
      cout << "TXREG::" << __FUNCTION__ << "\n";
      cout << "\n\n";
    }

    last_time = gpsim_get_current_time();
    start_time = last_time;

    if(txpin) {
      txpin->put_state(txr & 1);
      //cout << "usart tx module sent a " << (txr&1) <<  " bit count " << bit_count << '\n';
    }

    if(bit_count) {
      txr >>= 1;
      bit_count--;
      future_time = last_time + time_per_bit;
    } else {

      if(usart)
	tx_byte = usart->get_tx_byte();
      else
	tx_byte++;

      build_tx_packet(tx_byte);
      future_time = last_time + time_per_bit * 12;
      
    }

    gpsim_set_break(future_time, this);
  }

  void build_tx_packet(unsigned int tb) {


    tx_byte = tb &  ((1<<bits_per_byte) - 1);

    txr =  ((3 << bits_per_byte) | tx_byte) << 1;

    // total bits = byte + start and stop bits
    bit_count = bits_per_byte + 1 + 1;

    //cout << hex << "TXREG::" << __FUNCTION__ << " byte to send 0x" << tb <<" txr 0x" << txr << "  bits " << bit_count << '\n';

  }

  void enable(void) {
    if(0) {
      cout << "\n\n";
      cout << "TXREG::" << __FUNCTION__ << "\n";
      cout << "\n\n";
    }

    build_tx_packet(tx_byte);
    last_time = gpsim_get_current_time();
    future_time = last_time + time_per_bit;
    gpsim_set_break(future_time, this);
  }
};

//=================================================================
//
//  RCREG 
//
// Create a receive register based upon the receive register defined
// in the main gpsim code
//

class RCREG : public BreakCallBack // : public _RCREG
{
 public:

  USART_RXPIN *rxpin;

#define DEFAULT_BAUD    250000
#define MAX_PW  0xfffffff

#define RX_ERR_OVERRUN        1
#define RX_ERR_UNDERRUN       2
#define RX_ERR_TOOMANY_EDGES  3

  enum RX_STATES {
    RS_WAITING_FOR_START,
    RS_0,
    RS_1,
    RS_2,
    RS_3,
    RS_4,
    RS_RECEIVING,
    RS_STOPPED,
    RS_OVERRUN
  } receive_state;

  BoolEventLogger *rx_event;

  guint32 error_flag;

  double baud;

  guint64 time_per_bit;
  guint64 last_time;
  guint64 start_time;
  guint64 future_time;

  unsigned int start_bit_time;
  unsigned int start_bit_index;
  bool last_bit;
  int bits_per_byte;

  double  stop_bits;
  guint64 time_per_packet;

  bool use_parity;
  bool parity;         // 0 = even, 1 = odd

  bool autobaud;

  IOPIN   *rcpin;
  //BRG     brg;
  unsigned int *fifo;


  struct _pw {
    guint64 width;
    guint64 sumofwidths;
    guint32 occurs;
  } pulses[64];
  guint32 start_index;

  //  virtual void push(unsigned int);
  //  virtual void pop(void);

  //  virtual void assign_pir_set(PIR_SET *new_pir_set){};

  /**************************/
  // RCREG constructor
  /**************************/
  RCREG(void) {
    rcpin = NULL;

    start_bit_time = 0;
    start_bit_index = 0;
    start_index = 0;
    last_bit = 1;
    rx_event = new BoolEventLogger(1024);

    for(int i = 0; i<64; i++) {
      pulses[i].width = MAX_PW;
      pulses[i].sumofwidths = 0;
      pulses[i].occurs = 0;
    }

    receive_state = RS_WAITING_FOR_START;

    autobaud = 0;
    set_bits_per_byte(8);
    set_stop_bits(1.0);
    set_noparity();

    //set_baud_rate(DEFAULT_BAUD);
    set_baud_rate(250000);
  }

  void set_bits_per_byte(int num_bits) {
    bits_per_byte = num_bits;
    update_packet_time();
  }

  void update_packet_time(void) {
    // for now the stop bit time is included in the total packet time

    if(baud <= 0.0)
      baud = DEFAULT_BAUD;  //arbitrary

    /*
      Calculate the total time to send a "packet", i.e. start bit, data, parity, and stop
    */

    time_per_packet = gpsim_digitize_time( (1.0 +                 // start bit
					    bits_per_byte +       // data bits
					    stop_bits  +          // stop bit(s)
					    use_parity)           //
					    /baud);
    time_per_bit = gpsim_digitize_time( 1.0/baud );
    //cout << "update_packet_time ==> 0x" << hex<< time_per_packet << "\n";
  }

  void set_baud_rate(double new_baud) {
    //cout << "RCREG::" << __FUNCTION__ << "\n";

    baud = new_baud;
    update_packet_time();

  };

  void set_stop_bits(double new_stop_bits) {

    stop_bits = new_stop_bits;

  }

  void set_noparity(void) {
    use_parity = 0;
  }

  void set_parity(bool new_parity) {
    use_parity = 1;
    parity = new_parity;
  }

  void se(guint64 t) {
    cout << "Search for event at t = 0x"<<t<<'\n';

    if(rx_event->get_state(t))
      cout << "high";
    else
      cout << "low";

    cout <<" at t = 0x"<<t << '\n';
  }
    

  virtual void callback(void) {
    if(0) {
      cout << "\n\n";
      cout << "RCREG::" << __FUNCTION__ << "\n";
      cout << "\n\n";
    }

    //// process the data.....

    //rx_event->dump(-1);  // dump all events
    rx_event->dump_ASCII_art( time_per_bit/4, start_bit_index );  // time_per_packet/10,-1);

    guint64 current_time =  gpsim_get_current_time();
    int edges = rx_event->get_edges(start_time, current_time);
    //cout << " gpsim time is " << current_time << "\n";
    //cout << " # of edges for one byte time " << edges << '\n';

    //if(edges > (1+bits_per_byte))
    //  cout << "noisy\n";

    // Decipher the byte:

    //if(!rx_event->get_state(current_time))
    //  cout << "no stop bit\n";

    switch(receive_state) {
    case RS_WAITING_FOR_START:
      //cout << "waiting for start\n";
      break;
    case RS_RECEIVING:
      if(last_bit) {
	// The last edge was a rising one and if the baud
	// is correct it was also the last rising edge before
	// the stop bit. So process the event queue and 
	// decipher the byte from it...
	//cout << "Looks like we've definitely received a stop bit\n";
	receive_state = RS_WAITING_FOR_START;

	unsigned int b = decode_byte(start_bit_index, time_per_bit);
	//cout << "RCREG: decoded to 0x" << b << "\n";

      } else {
	receive_state = RS_OVERRUN;
	//cout << "Looks like we've overrun\n";
      }

      break;
    case RS_STOPPED:
      receive_state = RS_WAITING_FOR_START;
      //cout << "received a stop bit\n";
      break;
    }

    // If the rx line is sitting low, then we can just start
    // receiving the next byte:

//     if(!last_bit) 
//       start();

    //if(!autobaud)
    //  gpsim_set_break(future_time, this);


  };


  void start(void) {

    last_time = gpsim_get_current_time();
    start_time = last_time;

    receive_state = RS_RECEIVING;
    start_bit_time = last_time;
    start_bit_index = rx_event->get_index();
    //rx_event->log_time(last_time);

    future_time = last_time + time_per_packet;

    if(!autobaud) {
      gpsim_set_break(future_time, this);
      //cout << "RCREG::start Setting Break\n";
    }
    //cout << "RCREG::start   last_cycle = " << 
    //  hex << last_time << " future_cycle = " << future_time << '\n';

  }

  // Insert a pulse into the array. small pulses are at the beginning.
  void add_pulse(guint64 pw) {

    guint64 scaled_pw = pw;
    cout << " add_pulse\n";
    for(int i=0; i<64; i++) {

      if(pulses[i].width == scaled_pw) {
	pulses[i].sumofwidths += pw;
	pulses[i].occurs++;
	cout << "incrementing pulse pw=" << pw << "  occurs="<< pulses[i].occurs<<"at index " << i <<'\n';
	cout << "average pw in bin " << (pulses[i].sumofwidths/pulses[i].occurs) << '\n';
	return;
      }

      if(pw < pulses[i].width) {
	// Insert into the array
	for(int j=63; j>i; j--) {
	  pulses[j].width = pulses[j-1].width;
	  pulses[j].occurs = pulses[j-1].occurs;
	  pulses[j].sumofwidths = pulses[j-1].sumofwidths;
	}
	pulses[i].width = scaled_pw;
	pulses[i].sumofwidths = pw;
	pulses[i].occurs = 1;

	cout << "inserting pulse " << pw << "  at index " << i <<'\n';
	return;
      }
    }
    cout << " add_pulse did not add\n";

  }

  // remove a pulse from the array
  void del_pulse(guint64 pw) {

    guint64 scaled_pw = pw;

    for(int i=0; i<64; i++) {

      if(pulses[i].width == scaled_pw) {

	cout << "deleting pulse " << pw << "  at index " << i << '\n';
	if(pulses[i].occurs > 1) {
	  pulses[i].occurs--;
	  pulses[i].sumofwidths -= pw;
	  return;
	}

	// Delete from the array
	for(int j=i; j<63; j++) {
	  pulses[j].width = pulses[j+1].width;
	  pulses[j].occurs = pulses[j+1].occurs;
	  pulses[j].sumofwidths = pulses[j+1].sumofwidths;
	}
	pulses[63].width = MAX_PW;
	pulses[63].occurs = 0;
	pulses[63].sumofwidths =0;

	return;
      }
    }
  }

  void dump_pulses(void) {
    for(int i=0; i<64; i++) {
      if(pulses[i].occurs)
	cout << "width 0x"<<hex<< pulses[i].width << "  avg width 0x"<< (pulses[i].sumofwidths/pulses[i].occurs)
	     << "  occurs 0x"<<pulses[i].occurs <<'\n';
      else
	return;
    }

  }

  unsigned int decode_byte(guint32 sindex, guint64 bit_time) {
    //rx_event->buffer[start_bit_index]
  
    guint32 cur_index = rx_event->get_index();
    if(sindex & 1) {
      sindex = (sindex + 1) & rx_event->max_events;
      if(sindex == cur_index)
	return 0x400;
    }

    guint64 cur_time = gpsim_get_current_time();
    guint64 t1 = rx_event->buffer[sindex] + bit_time + bit_time/2;

    guint32 index1 = rx_event->get_index(t1);
    guint32 index2 = (index1 + 1) & rx_event->max_events;
    bool decoding = 1;
    guint32 b = 0;

    cout << "decode_byte current time 0x"<<hex <<cur_time << " start bit time 0x"<< rx_event->buffer[sindex]<<'\n';
    if(t1 >= cur_time) 
      return 0x800;

    int i = 0;

    while(i<8  && decoding) {
      b = (b>>1) | ((index1 & 0x0001) << 7);
      cout << " time: 0x" << hex << t1 << " evt index: 0x" << index1 <<" b "<<b << '\n';
      t1 += bit_time;
      if(t1>=cur_time)
	decoding = 0;
      if(t1 > rx_event->buffer[index2]) {
	index1 = index2;
	index2 = (index2 + 1) & rx_event->max_events;
	if(index2 == cur_index) {
	  decoding = 0;
	  if(!autobaud)
	    b >>= 7-i;
	}
      }
      i++;
    }

    if(autobaud) {
      // t1 is now pointing to the middle of the stop bit
      // and index1 has the most recent edge prior to t1
      if(decoding) {
	if((index1 & 0x0001)==0)
	  b |= 0x100;             // Error: The state is high 
      } else
	b |= 0x200;               // Error: under run
    }

    return b;



#if 0
    if(index2 & 1  && index2 != cur_index) {
      index2 = (index2 + 1) & rx_event->max_events;
    }

    t1 = rx_event->buffer[index2] + bit_time + bit_time/2;
    if(t1>cur_time)
      decoding = 0;
    index1 = rx_event->get_index(t1);
    index2 = (index1 + 1) & rx_event->max_events;
#endif

  }

  /*
      new_rx_edge(bool bit)

      This routine get's called when there's a change on the
      RX line. The time the edge occurred is stored into an
      event buffer.

      If we're "autobauding", that is trying to dynamically 
      ascertain the baud rate, then additional processing 
      is performed. A simple algorithm (that took a really 
      long time to figure out) extracts the baud rate from
      the capture data. Here's how it works:

      As data edges are received, they're stuffed into the
      event buffer (iff there's a change of state). The time
      since the previous edge is stored in a "pulse width"
      buffer. This buffer acts as a median filter because 
      the pulses get stored in a sorted order. For example,
      narrow pulses are placed at the beginning of the buffer.
      If two pulses of the same size are received, then a
      counter associated with the pulse will keep track of
      this.

      If there's a whole lot of jitter in the data, one would
      expect many pulses of approximately the same width will
      occupy adjacent positions in the pulse width buffer. To
      get around this, a trunctated version of the pulse is
      actually stored. The truncation is simply a shift right
      N bits (compile time selected). Consequently, the lsb's
      are discarded and the msb's are lumped together. The
      actual pulse widths are still stored by accumulation.
      In other words, as pulses are received into the truncated
      bins, their actual widths are tallied into a sum. The
      average pulse width of the bin is easily obtained by
      dividing this tally by the total number of pulses 
      received.

      Only the 64 most recent edges are used to compute the
      pulse widths. So starting with the 65'th edge, old pulse
      widths are removed from the buffer in away analogous to
      the way the new ones are added. That is, the lsb's are
      discarded, the msb's are searched for in the buffer, and
      the pulse width tally is reduced by the amount of the
      actual pulse width. 
      
  */

  void new_rx_edge(bool bit) {
/*
    cout << "USART MODULE RCREG::" << __FUNCTION__ << "\n";
    switch(receive_state) {
    case RS_WAITING_FOR_START:
      cout << "state = WAITING_FOR_START\n";
      break;
    case RS_RECEIVING:
      cout << "state = RECEIVING\n";
      break;
    case RS_STOPPED:
      cout << "state = STOPPED\n";
      break;
    case RS_OVERRUN:
      cout << "state = OVERRUN\n";
      break;

    }
*/
    // If this bit is different from the last one we got
    // then save it in the event buffer.

    if(bit ^ last_bit) {
      guint32 cur_index = rx_event->get_index();

      rx_event->event(bit);            // log the event
      if(cur_index != start_index)     // true after first edge
	add_pulse(rx_event->buffer[cur_index] - 
		  rx_event->buffer[(cur_index - 1) & rx_event->max_events]);

      // If we have received more than 64 edges, then start removing the
      // old pulses.

      if( ((cur_index - start_index) & rx_event->max_events) > 63) {

	guint32 old_index = (start_index + 1) & rx_event->max_events;

	del_pulse(rx_event->buffer[old_index] - 
		  rx_event->buffer[start_index]);

	start_index = old_index;
      }

      //dump_pulses();


      last_bit ^= 1;                 // change current state


      if(autobaud) {

	if(!bit && receive_state == RS_WAITING_FOR_START) {
	  // Looks like we just got a start bit.
	  start_bit_time = gpsim_get_current_time();
	  cout  << "Start bit at t = 0x" << start_bit_time << '\n';
	  receive_state = RS_RECEIVING;
	}


	if(bit) {
	  guint64 pw[64];  // pulse widths will get stored here.
	  cur_index = rx_event->get_index();
	  guint32 edges = (cur_index - start_bit_index) & rx_event->max_events;


	  // Don't bother autobauding if we haven't received more than 4 pulses (8 edges)
	  if(edges >= 8) {
	    int i,j;
	    cout << "Auto bauding\n";

	    if(edges > 64) {
	      start_bit_index = (start_bit_index + 8 ) & rx_event->max_events;
	      start_bit_time = rx_event->buffer[start_bit_index];
	      edges -= 8;
	    }
	    rx_event->dump_ASCII_art( pulses[0].width/2, (cur_index - edges+3) & rx_event->max_events, cur_index );

	    // Analyze...

	    guint64 min = pulses[0].width;
	    double w = 1.0*pulses[0].width;

	    cout << "Bit times based on minimum:\n";
	    bool suspicious=0;
	    j = 0;
	    int istart = 0;
	    int istop = 3;
	    do {
	      do {
		suspicious=0;
		for(i=istart; i<istop && pulses[i].occurs; i++) {
		  //double p = pulses[i].width / w;
		  double p = pulses[i].sumofwidths / w / pulses[i].occurs;
		  cout << i << ": " << p ;
		  if( (p - floor(p)) > 0.25) {
		    cout << "  <-- suspicious";
		    suspicious = 1;
		  }
		  cout << '\n';
		}
		if(suspicious) {
		  w /= 2.0;
		  min >>= 1;
		  cout << "halving the measured pulse width\n";
		}
	      }while (suspicious && ++j<4);

	      // the first pulse must be a glitch. So repeat with the next.

	      if(suspicious) {
		istart++;
		istop++;
		min = pulses[1].width;
		w = 1.0*min;
		cout << "Hmm, moving to next pulse\n";
	      }
	    }while (suspicious && j<8);

	    if(suspicious)
	      cout << "Unable to determine the baud rate.\n";

	    cout << "Minimum pulse width " << hex << min << " Baud = " << (gpsim_digitize_time(1.0)/w) <<'\n';

	    // Assume the baud rate is correct.
	    // use it to decode the bit stream.

	    suspicious = 0;  // assume all bytes decode correctly

	    //unsigned int b = decode_byte(rx_event->buffer[start_bit_index] + min + min/2, min);
	    // unsigned int b = decode_byte(start_bit_index, min);

	    //guint32 index1 = rx_event->get_index(rx_event->buffer[cur_index] - min * 11);
	    guint32 index1 = (rx_event->get_index() - edges)& rx_event->max_events;

	    if(index1&1)
	      index1 &= 0xfffffe;

	    guint32 b=0;

	    j=0;
	    do {

	      b = decode_byte(index1, min);

	      cout <<j<< ": Decoded byte 0x"<< (b&0xff) << " is ";
	      if(b >=0x100) {
		index1 = (index1 + 2) & rx_event->max_events;
		cout << "invalid b=0x" <<b<<'\n';
	      } else {
		index1 = rx_event->get_index(rx_event->buffer[index1] + 11*min);
		cout <<"valid\n";
	      }

	    } while ( ++j < 8);

#if 0
	    if( (max / min) > 9)
	      cout << " max / min > 9\n";

	    // If there was a falling edge nine bit times ago, then we have
	    // captured a valid byte.

	    //double start_edge = rx_event->buffer[cur_index] - min * 9;
	    guint32 index1 = rx_event->get_index(rx_event->buffer[cur_index] - (min * 9  + (min >> 2)));
	    guint32 index2 = rx_event->get_index(rx_event->buffer[cur_index] - (min * 9  - (min >> 2)));
	    guint32 delta  = (index2 - index1) & rx_event->max_events;

	    if( (delta == 1) && ( (index2&1) == 0)) {
	      cout << " !! Found a valid byte !!\n";
	      receive_state = RS_WAITING_FOR_START;
	      cout << " Captured Start bit time vs calculated start bit time\n";
	      cout << hex << "0x"<<start_bit_time << " vs 0x" << (rx_event->buffer[cur_index] - (min * 9)) << '\n';
	      guint64 t1 = rx_event->buffer[cur_index] - min / 2; 
	      index1 = (cur_index-1) & rx_event->max_events;
	      guint32 b = 0;
	      i = 0;
	      //for(j=0; j<4  &&  (  ((index1-prev_index) & rx_event->max_events) > 0  ); j++) {
	      do {
		b = (b<<1) | (index1 & 0x0001);
		cout << " time: 0x" << hex << t1 << " evt index: 0x" << index1 << '\n';
		if(t1 < rx_event->buffer[index1])
		  index1 = (index1 - 1) & rx_event->max_events;
		t1 -= min;
		i++;
	      } while(i<=8);
	      cout << "Most recent byte: 0x" <<hex<< b << '\n';
	      //}


	    }

	    rx_event->dump_ASCII_art( min/2, (cur_index - edges+3) & rx_event->max_events, cur_index );
#endif
	  }

	}
      } else {

	cur_index = rx_event->get_index();
	guint32 edges = (cur_index - start_bit_index) & rx_event->max_events;

	/* Not autobauding */ 
	switch(receive_state) {
	case RS_WAITING_FOR_START:
	  if(!bit) {
	    // Looks like we just got a start bit.
	    start_bit_time = gpsim_get_current_time();

	    cout  << "Start bit at t = 0x" << start_bit_time << '\n';

	    start();
	  }

	  break;
	case RS_RECEIVING:
	  cout << "edges " << edges << "\n";
	  if(edges > 8)
	    receive_state = RS_OVERRUN;

	  break;
	case RS_OVERRUN:
	  if(bit) {
	    cout << "Clearing overrun condition\n";
	    receive_state = RS_WAITING_FOR_START;
	  }
	  break;
	}

      }


    }

  }


};

class USART_IO : public IOPIN
{
public:


  USART_IO(void) {
    cout << "USART_IO constructor - do nothing\n";
  }
  USART_IO (IOPORT *i, unsigned int b, char *opt_name=NULL) : IOPIN(i,b,opt_name) { };

  virtual void put_node_state(int new_state) {
    cout << "USART_IO put_node_state\n";
  }

  //virtual void put_state( int new_state);
  virtual int get_voltage(guint64 current_time) {
    return 42;
  }
};




class USART_IOPORT : public IOPORT
{
public:
  USARTModule *usart;

  void trace_register_write(void) {
    cout << "USART_IOPORT::trace_register_write\n";
  }

  USART_IOPORT (USARTModule *new_usart, unsigned int _num_iopins=4): IOPORT(_num_iopins) {
    usart = new_usart;
  }
  void put(unsigned int new_value) {
    cout << "writing " << new_value <<" to usart port\n";
    //IOPORT::put(new_value);
  }
  void setbit(unsigned int bit_number, bool new_value) {
    int mask = 1<<bit_number;

    value = (value & ~mask) | (new_value ? mask : 0);

    if(usart)
      //trace.module1( (usart->interface_id << 4 ) | (value & 0xf));
      trace.module1( value & 0xf);
  }
};


//--------------------------------------------------------------
//  Usart Attributes - derived from gpsim's FloatAttribute
// 
//--------------------------------------------------------------
class UsartAttribute : public FloatAttribute {

public:
  enum UA_TYPE {
    UA_BAUDRATE,
    UA_STARTBITS,
    UA_STOPBITS,
  } ;

  UA_TYPE type;


  UsartAttribute(char *_name, UA_TYPE new_type, double def_val) {

    cout << "USART Attribute constructor\n";

    type = new_type;
    new_name(_name);
    value = def_val;

  }


  void set(double new_val) {

    cout << "Setting usart attribute\n";

    value = new_val;

  };


  void set(int v) {
    set(double(v));
  };
};


//
//  USART attributes
//
//  Provide attributes that allow the user to dynamically 
// configure the USART module
//
// Attribute    Default
//    Name      Value
// -------------------
//   baud       9600
//   parity        0
//   start_bits    1
//   stop_bits     1
//   


class BaudRateAttribute : public FloatAttribute {

public:
  USARTModule *usart;

  BaudRateAttribute(USARTModule *pusart, char *_name=NULL) {

    usart = pusart;
    //brg = pbrg;
    if(!pusart)// || !pbrg)
      return;

    if(_name)
      new_name(_name);
    else 
      new_name("baud");
    cout << "BaudRateAttribute constr\n";

  };


  void set(double b) {


    cout << "Setting baud rate attribute!\n";
    cout << " old value: " << value << " New value: " << b << endl;
    value = b;
  };


  void set(int r) {
    set(double(r));
  };
};


class RxBaudRateAttribute : public BaudRateAttribute  {

public:
  RCREG *rcreg;

  RxBaudRateAttribute(USARTModule *pusart):BaudRateAttribute(pusart,"rxbaud") {

    if(!pusart || !pusart->usart || !pusart->usart->rcreg)// || !pbrg)
      return;

    rcreg = pusart->usart->rcreg;

    cout << "RxBaudRateAttribute constructor\n";

  };


  void set(double b) {


    cout << "Setting Rx baud rate attribute!\n";
    cout << " old value: " << value << " New value: " << b << endl;
    value = b;
    if(rcreg)
      rcreg->set_baud_rate(value);

  };


  void set(int r) {
    set(double(r));
  };
};

class TxBaudRateAttribute : public BaudRateAttribute  {

public:
  TXREG *txreg;

  TxBaudRateAttribute(USARTModule *pusart):BaudRateAttribute(pusart,"txbaud") {

    if(!pusart || !pusart->usart || !pusart->usart->txreg)// || !pbrg)
      return;

    txreg = pusart->usart->txreg;

    cout << "TxBaudRateAttribute constructor\n";

  };


  void set(double b) {


    cout << "Setting Tx baud rate attribute!\n";
    cout << " old value: " << value << " New value: " << b << endl;
    value = b;
    if(txreg)
      txreg->set_baud_rate(value);

  };


  void set(int r) {
    set(double(r));
  };
};
//--------------------------------------------------------------
USART_CORE::USART_CORE(void)
{

  cout << "new USART_CORE\n";

  baud_rate = new UsartAttribute("BAUD", UsartAttribute::UA_BAUDRATE, 1200);

  txreg=NULL;
  rcreg=NULL;

}

//--------------------------------------------------------------
void USART_CORE::new_rx_edge(unsigned int bit)
{
  if(rcreg)
    rcreg->new_rx_edge(bit);

}

//--------------------------------------------------------------
void USART_CORE::initialize(USART_IOPORT *new_iop)
{

  port = new_iop;

  rcreg = new RCREG;
  txreg = new TXREG;
  txreg->enable();

}
//--------------------------------------------------------------
static int _tx_index=0;
static char Test_Hello[] = {
  0x1b,0xff, 0x87,0x05, 'H', 'E',  'L', 'L',
  'O', 0x17, 0x55
};
int USART_CORE::get_tx_byte(void)
{
  if(_tx_index > sizeof(Test_Hello))
    _tx_index = 0;

  return Test_Hello[_tx_index++];

}
//--------------------------------------------------------------
// create_iopin_map 
//
//  This is where the information for the Module's package is defined.
// Specifically, the I/O pins of the module are created.

#define USART_PKG_TXPIN        1
#define USART_PKG_RXPIN        2
#define USART_PKG_CTSPIN       3
#define USART_PKG_RTXPIN       4

void USARTModule::create_iopin_map(void)
{


  // Create the usart core from the usart class in the main
  // gpsim source code.

  usart = new USART_CORE; 

  //  usart->brg = new BRG;

  // Create an I/O port to which the I/O pins can interface
  //   The module I/O pins are treated in a similar manner to
  //   the pic I/O pins. Each pin has a unique pin number that
  //   describes it's position on the physical package. This
  //   pin can then be logically grouped with other pins to define
  //   an I/O port. 

  port = new USART_IOPORT(this, 4);


  // Here, we name the port `pins'. So in gpsim, we will reference
  //   the bit positions as U1.pin0, U1.pin1, ..., where U1 is the
  //   name of the usart (which is assigned by the user and
  //   obtained with the name() member function call).

  char *pin_name = name();   // Get the name of this usart
  if(pin_name) 
    port->new_name(pin_name);
  else
    port->new_name("usart_port");


  usart->initialize(port);

  // Define the physical package.
  //   The Package class, which is a parent of all of the modules,
  //   is responsible for allocating memory for the I/O pins.
  //
  //   USART I/O pins:
  //
  //    1 - Tx - Transmit
  //    2 - Rx - Receive
  //    3 - CTS - Clear To Send
  //    4 - RTS - Request To Send


  create_pkg(4);


  // Define the I/O pins and assign them to the package.
  //   There are two things happening here. First, there is
  //   a new I/O pin that is being created.The second thing is
  //   that the pins are "assigned" to the package. If we
  //   need to reference these newly created I/O pins (like
  //   below) then we can call the member function 'get_pin'.

  USART_TXPIN *txpin = new USART_TXPIN(usart,port, 0,"TX");
  USART_RXPIN *rxpin = new USART_RXPIN(usart,port, 1,"RX");
  assign_pin(1, txpin);
  assign_pin(2, rxpin);
  assign_pin(3, new USART_IO(port, 2,"CTS"));
  assign_pin(4, new USART_IO(port, 3,"RTS"));


  // Create an entry in the symbol table for the new I/O pins.
  // This is how the pins are accessed at the higher levels (like
  // in the CLI).

  symbol_table.add_stimulus(Package::get_pin(1));
  symbol_table.add_stimulus(Package::get_pin(2));
  symbol_table.add_stimulus(Package::get_pin(3));
  symbol_table.add_stimulus(Package::get_pin(4));


  // Complete the usart initialization

/*
  new txreg
  new rcreg
  
*/

  if(usart->txreg) {
    usart->txreg->txpin = txpin; 
    usart->txreg->usart = usart; // Point back to the module

  }

  if(usart->rcreg) {
    usart->rcreg->rxpin = rxpin;
  }

  //usart->brg->put(0);
  //usart->spbrg->set_baud_rate(38400);
  //usart->rcsta->put(_RCSTA::SPEN | _RCSTA::CREN);

}


//--------------------------------------------------------------

ExternalModule * USARTModule::USART_construct(const char *new_name)
{

  cout << "USART construct \n";

  USARTModule *um = new USARTModule(new_name);

  if(new_name) {
    um->new_name(new_name);
    //um->res->put_name(new_name);
  }

  um->create_iopin_map();

  RxBaudRateAttribute *rxattr = new RxBaudRateAttribute(um);
  um->add_attribute(rxattr);

  TxBaudRateAttribute *txattr = new TxBaudRateAttribute(um);
  um->add_attribute(txattr);

  return um;

}

//--------------------------------------------------------------

void USARTModule::set_attribute(char *attr, char *val)
{

  cout << "--USARTModule::set_attribute\n";

  Module::set_attribute(attr,val);
}


USARTModule::USARTModule(const char *_name)
{

  port = NULL;
  usart = NULL;

}

USARTModule::~USARTModule()
{
    // FIXME
}






#if 0

class EventBuffer {
public:
  static const unsigned int EVENT_BUFFER_SIZE = 1024;

  guint32 *event_buffer;          // Buffer that holds the events
  guint32  event_index;           // Index into the buffer
  //guint32  event_buffer_size;     // buffer's size

  guint64  start_time;
  guint64  end_time;
    

  bool log_event(guint32 new_event) {

    event_buffer[event_index] = new_event;
    if(++event_index >= EVENT_BUFFER_SIZE) {
      // This buffer is full
      event_index--;
      end_time = gpsim_get_current_time();
      return 1;
    }
    return 0;
  }

  void start(void) {
    event_index = 0;
    start_time = gpsim_get_current_time();
  }

  unsigned int get_event(int index) {
    if(index < 0) {
      index += EVENT_BUFFER_SIZE;
      if(index < 0)
	index = 0;
    } else if(index >= EVENT_BUFFER_SIZE) {
      index -= EVENT_BUFFER_SIZE;
      if(index >= EVENT_BUFFER_SIZE)
	index = EVENT_BUFFER_SIZE - 1;
    }

    return event_buffer[index];
  }


  EventBuffer(void) {

    event_buffer = new guint32[EVENT_BUFFER_SIZE];
    event_index  = 0;
    start_time   = 0;
    end_time     = 0;
  }

};

class EventLogger {
public:
  enum ELEventTypes {
    ELET_NOTHING=0,
    ELET_DELTA_HI          = 0x01<<24,
    ELET_DELTA_LO          = 0x02<<24,
    ELET_DELTA_BYTE        = 0x03<<24,
    ELET_TIME_LO           = 0x80<<24,
    ELET_TIME_HI           = 0x40<<24
  };

  static const unsigned int MAX_DELTA = 1<<24;
  static const unsigned int NBUFFERS  = 8;
  guint32 buffer_index;

  EventBuffer *event_buffers;    // Buffers that holds the events
  EventBuffer *current_buffer;    // The buffer that's active

  guint32  event_index;           // Index into the buffer
  guint32  event_buffer_size;     // buffer's size

  guint64 latest_event_time;      // The time of the most recently captured event
  guint64 latest_dump_time;       // The time of the most recently dumped event

  /*
    boolean_event(guint64 event_time, bool state)

    Record a 0/1 event (e.g. the state of an I/O line).
    An attempt is made to store the event as a 'delta'
    since the last event. Presumably, events happen close
    to one another. If this presumption is wrong, then
    the whole 64 bits of the event_time get stashed away
    into the event log.
   */

  inline void boolean_event(guint64 not_used_event_time, bool state)
  {
    guint64 event_time = gpsim_get_current_time();
    guint64 delta  = event_time - latest_event_time;

    latest_event_time = event_time;
    
    if( delta >= MAX_DELTA){

      // The time between events is so large that it really 
      // doesn't matter if we try to be ultra-efficient in 
      // logging this event.

      // Capture the time

      log_time(latest_event_time);

      delta = 0;
    }

    if(state)
      log_event(ELET_DELTA_HI | delta); 
    else
      log_event(ELET_DELTA_LO | delta);


  }

  /*
    get_index - return the current index

    This is used by the callers to record where in the event
    buffer a specific event is stored. (e.g. The start bit
    of a usart bit stream.)
   */
  inline unsigned int get_index(void) {
    return event_index;
  }


  unsigned int get_event(int index) {

    return current_buffer->get_event(index);
  }
  /*
    log_time(guint64 event_time)

    Record the full 64 bit time in the event array. The 64 bit
    variable is split into two 32 bit variables and then written.
   */

  inline void log_time(guint64 event_time) {

    // Check to make sure we're not logging two consecutive time events.

    //if(event_buffer[get_index(event_index - 1)] & (ELET_TIME_LO | ELET_TIME_HI)) {
    //
    //} else {

    log_event(ELET_TIME_LO | event_time & 0xffffffff);
    log_event(ELET_TIME_HI | (event_time>>32) | (event_time & ELET_TIME_LO));
    // }
  }

  inline void log_event(unsigned int new_event) {
    if(current_buffer->log_event(new_event) {
      buffer_index = (buffer_index + 1) & 7;
      current_buffer = &event_buffers[buffer_index];
      current_buffer->start();
    }
  }


  int dump1(int index) {

    if(current_buffer->event_buffer[index] & (ELET_TIME_LO | ELET_TIME_HI)) {

      cout << " time event\n";

      unsigned int hi = get_event(index + 1);
      unsigned int lo = current_buffer->event_buffer[index];

      if(!(hi & ELET_TIME_HI)) {
	hi = lo;
	lo = get_event(index - 1);
      }

      guint64 t = hi & ~(ELET_TIME_LO | ELET_TIME_HI);
      t = (t<<32) | ( (lo & ~ELET_TIME_LO) | (t & ELET_TIME_LO));
      cout << hex << "0x" << t << '\n';

      latest_dump_time = t;

      return 2;
	
    }


    // The upper byte is the event type

    switch(current_buffer->event_buffer[index] & 0xff000000) {

    case ELET_NOTHING:
      cout << " empty event\n";
      break;

    case ELET_DELTA_LO:
      latest_dump_time += current_buffer->event_buffer[index] & 0xffffff;
      cout << "0x" << latest_dump_time << ": delta lo\n";
      break;

    case ELET_DELTA_HI:
      latest_dump_time += current_buffer->event_buffer[index] & 0xffffff;
      cout << "0x" << latest_dump_time << ": delta hi\n";
      break;

    case ELET_DELTA_BYTE:
      cout << " delta byte\n";
      break;
    default:
      cout << "bad event\n";
    }

    return 1;
  }

  void dump(int start_index, int end_index=-1) {

    
    if((start_index >= event_buffer_size) || (start_index <= 0 ))
      start_index = 0;

    if(end_index == -1)
      end_index = event_index;

    if(start_index == end_index)
      return;

    int event_size = 0;

    // Loop through and dump events between the start and end points requested

    do {

      // If on a previous iteration the event occupied more than one
      // slot in the queue, then skip dumping this time.

      if(event_size <= 0)
	event_size = dump1(start_index);

      event_size--;
      start_index++;

    }while ( start_index != end_index);

  }

  EventLogger(unsigned int max_events = 1024) {

    event_buffers = new EventBuffer[8];
    current_buffer = &event_buffers[0];
    buffer_index = 0;

    //event_buffer = new unsigned int[max_events];
    //event_buffer_size = max_events;
    latest_event_time = 0;
    latest_dump_time = 0;
    event_index = 0;
  }
};

#endif
