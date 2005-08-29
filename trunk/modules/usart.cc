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


/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include <errno.h>
#include <stdlib.h>
#include <string>
#include <math.h>  // for floor()

#include "../config.h"    // get the definition for HAVE_GUI

#ifdef HAVE_GUI
#include <gtk/gtk.h>
#endif

#include "usart.h"


#include "../src/value.h"
#include "../src/modules.h"
#include "../src/packages.h"
#include "../src/stimuli.h"
#include "../src/ioports.h"
#include "../src/symbol.h"
#include "../src/interface.h"
#include "../src/pir.h"
#include "../src/trace.h"
#include "../src/uart.h"

#include "../src/bitlog.h"


#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d-%s() ",__FILE__,__LINE__,__FUNCTION__); printf arg; }
#else
#define Dprintf(arg) {}
#endif


static bool bIsLow(char state)
{
  return state=='0' || state=='w';
}
static bool bIsHigh(char state)
{
  return state=='1' || state=='W';
}

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


//--------------------------------------------------------------
//
//  BRG
//
// Serial Port Baud Rate Generator
//
//
//--------------------------------------------------------------

class BRG       //  : public TriggerObject
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
    last_time = get_cycles().get();

    cout << "SPBRG rollover at cycle " << last_time << '\n';

    //if(rcsta->value & _RCSTA::SPEN) {
      // If the serial port is enabled, then set another 
      // break point for the next clock edge.
      get_next_cycle_break();

      //}

  };
#endif

  virtual void start(void) {

    last_time = get_cycles().get();
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

    get_cycles().set_break(future_time, this);


  };
#endif

  void set_baud_rate(double new_baud) {
    //cout << "SPBRG::" << __FUNCTION__ << "\n";

    baud = new_baud;

    if(get_active_cpu() && baud>0.0) {

      time_per_bit = (guint64)(get_active_cpu()->get_frequency()/baud);

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

class USART_RXPIN : public IO_bi_directional_pu
{
public:

  USART_CORE *usart;
  /*
  USART_RXPIN(void) {
    //cout << "USART_RXPIN constructor - do nothing\n";
  }
  */
  USART_RXPIN (USART_CORE *_usart,
	       IOPORT *i, 
	       unsigned int b, 
	       char *opt_name=NULL) : IO_bi_directional_pu(i,b,opt_name) { 

    usart = _usart;

    // Let the pin think it's in the high state. If this is wrong,
    // then the I/O pin driving it will correct it. (Starting off
    // low prevents the start bit from being captured.) 
    // Note, may want to add a flag that indicates if the pin
    // has ever been driven at all. This way, we can capture the
    // first edge. Or we could add another parameter to the constructor.

    bDrivenState = true;
    update_direction(0,true);   // Make the RX pin an input.

    bPullUp = true;
    Zpullup = 10e3;

  };


  void setDrivenState(bool new_dstate) { 
    bool diff = new_dstate ^ bDrivenState;

    Dprintf((" usart module rxpin new state=%d\n",new_dstate));

    if( usart && diff ) {

      bDrivenState = new_dstate;
      IOPIN::setDrivenState(new_dstate);
      usart->new_rx_edge(bDrivenState);


    }

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

    bDrivingState = true;
    update_direction(1,true);   // Make the TX pin an output.

  };
#if 0
  virtual void put_node_state(int new_state) {

    // cout << "USART_TXPIN put_node_state " << new_state << '\n';
    /*
    state = new_state;

    if( state < h2l_threshold) {
      state = l2h_threshold + 1;
      putState(0);

    } else {

      if(state > l2h_threshold) {
	state = h2l_threshold - 1;
	putState(1);
      }
    }
    */
  }
#endif
  //  void putState(bool new_dstate) { 

    //cout << "usart tx putState " << new_dstate << '\n';
    /*
    bool diff = new_dstate ^ bDrivingState;
    bDrivingState = new_dstate;

    cout << "usart tx putState " << bDrivingState << '\n';
    if( usart && diff ) {

      usart->new_rx_edge(bDrivingState);

      if(iop) // this check should not be necessary...
	iop->setbit(iobit,bDrivingState);
    }
    */

  //}
#if 0
  virtual void putState(bool newDrivingState) {

    Register *port = get_iop();

    if(port) {
      // If the new state to which the stimulus is being set is different than
      // the current state of the bit in the ioport (to which this stimulus is
      // mapped), then we need to update the ioport.

      if((newDrivingState!=0) ^ ( port->value.get() & (1<<iobit))) {

	bool bNewState = newDrivingState ? true : false;
	port->setbit(iobit,bNewState);

	bDrivingState = bNewState;
	// If this stimulus is attached to a node, then let the node be updated
	// with the new state as well.
	if(snode)
	  snode->update(0);
	// Note that this will auto magically update
	// the io port.

      }
    }
  }
#endif

};





//=================================================================
//
//  TXREG
//
//  Create a transmit register based upon the transmit register
// defined in the main gpsim code.
//

class TXREG : public TriggerObject
{
private:
  //BRG         *brg;

  bool empty_flag;
  double baud;

  guint64 time_per_bit;
  guint64 last_time;
  guint64 start_time;
  guint64 future_time;

  guint64 start_bit_time;
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

 public:
  USART_TXPIN *txpin;
  USART_CORE  *usart;

  virtual bool is_empty() { return empty_flag;};
  virtual void empty() {empty_flag = 0;};
  virtual void full()  {empty_flag = 1;};
  virtual void assign_pir_set(PIR_SET *new_pir_set){};

  TXREG(void) {
    txpin = 0;
    usart = 0;

    baud = 9600;
    bits_per_byte = 8;
    stop_bits = 1;
    use_parity = 0;

    tx_byte = '0';

    update_packet_time();

  }



  void update_packet_time(void) {

    if(baud <= 0.0)
      baud = 9600;  //arbitrary

    // Calculate the total time to send a "packet", i.e. start bit, data, parity, and stop
    // The stop bit time is included in the total packet time

    if(get_active_cpu()) {
      time_per_packet = 
	(guint64)( get_active_cpu()->get_frequency() * ( (1.0 +             // start bit
							  bits_per_byte +   // data bits
							  stop_bits  +      // stop bit(s)
							  use_parity)       //
							 /baud));
      time_per_bit = (guint64)(get_active_cpu()->get_frequency() / baud);
    } else
      time_per_packet = time_per_bit = 0;

    //cout << "update_packet_time ==> 0x" << hex<< time_per_packet << "\n";
  }

  void set_bits_per_byte(int num_bits) {
    bits_per_byte = num_bits;
    update_packet_time();
  }

  void set_baud_rate(double new_baud) {
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
    if(1) {
      cout << " usart module TXREG::" << __FUNCTION__ << "\n";
    }

    last_time = get_cycles().get();
    start_time = last_time;

    if(txpin) {
      txpin->putState((txr & 1) ? true : false);
      cout << "usart tx module sent a " << (txr&1) <<  " bit count " << bit_count << '\n';
    }

    if(bit_count) {
      txr >>= 1;
      bit_count--;
      future_time = last_time + time_per_bit;
      get_cycles().set_break(future_time, this);
    } else {
      // We've sent the whole byte. 

      if(usart && usart->mGetTxByte(tx_byte))
	mSendByte(tx_byte);
      else
	empty();
    }

  }


  void mSendByte(unsigned _tx_byte) 
  {
    
    if(1) {
      cout << "\n\n";
      cout << "TXREG::" << __FUNCTION__ << "\n";
      cout << "\n\n";
    }

    mBuildTXpacket(tx_byte);
    last_time = get_cycles().get();
    future_time = last_time + time_per_bit;
    get_cycles().set_break(future_time, this);
  }

private:
  void mBuildTXpacket(unsigned int tb) {


    tx_byte = tb &  ((1<<bits_per_byte) - 1);

    txr =  ((3 << bits_per_byte) | tx_byte) << 1;

    // total bits = byte + start and stop bits
    bit_count = bits_per_byte + 1 + 1;

    cout << hex << "TXREG::" << __FUNCTION__ << " byte to send 0x" << tb 
	 <<" txr 0x" << txr << "  bits " << bit_count << '\n';

  }

};

//=================================================================
//
//  RCREG 
//
// Create a receive register based upon the receive register defined
// in the main gpsim code
//

class RCREG : public TriggerObject // : public _RCREG
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


  //  virtual void push(unsigned int);
  //  virtual void pop(void);

  //  virtual void assign_pir_set(PIR_SET *new_pir_set){};

  /**************************/
  // RCREG constructor
  /**************************/
  RCREG();

  void set_bits_per_byte(int num_bits) {
    bits_per_byte = num_bits;
    update_packet_time();
  }

  void update_packet_time() {
    // for now the stop bit time is included in the total packet time

    if(baud <= 0.0)
      baud = DEFAULT_BAUD;  //arbitrary

    /*
      Calculate the total time to send a "packet", i.e. start bit, data, parity, and stop
    */

    if(get_active_cpu()) {
      time_per_packet = 
	(guint64)(get_active_cpu()->get_frequency() * (1.0 +     // start bit
						       bits_per_byte +   // data bits
						       stop_bits  +      // stop bit(s)
						       use_parity)       //
		  /baud);

      time_per_bit = (guint64)(get_active_cpu()->get_frequency() / baud);
    } else
      time_per_packet = time_per_bit = 0;

    //cout << "update_packet_time ==> 0x" << hex<< time_per_packet << "\n";
  }

  void set_baud_rate(double new_baud) {
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

  /*
  void se(guint64 t) {
    cout << "Search for event at t = 0x"<<t<<'\n';

    if(rx_event->get_state(t))
      cout << "high";
    else
      cout << "low";

    cout <<" at t = 0x"<<t << '\n';
  }
  */

  virtual void callback();

  void start();
  unsigned int decode_byte(guint32 sindex, guint64 bit_time);
  void new_rx_edge(bool bit);

private:

  ThreeStateEventLogger *rx_event;

  char m_cLastRXState;
  unsigned int start_bit_event;


  guint32 error_flag;


  guint64 time_per_bit;
  guint64 last_time;
  guint64 start_time;
  guint64 future_time;

  // Configuration information
  int     bits_per_byte;
  double  stop_bits;
  bool    use_parity;
  bool    parity;         // 0 = even, 1 = odd
  double  baud;

  guint64 time_per_packet;

  bool autobaud;

  IOPIN   *rcpin;
  //BRG     brg;
  unsigned int *fifo;



};

//------------------------------------------------------------------------

RCREG::RCREG(void)
  : rcpin(0), start_bit_event(0), m_cLastRXState('?')
{

  rx_event = new ThreeStateEventLogger(1024);

  receive_state = RS_WAITING_FOR_START;

  autobaud = false;
  set_bits_per_byte(8);
  set_stop_bits(1.0);
  set_noparity();

  //set_baud_rate(DEFAULT_BAUD);
  set_baud_rate(250000);
}

//------------------------------------------------------------------------
void RCREG::callback() 
{

  Dprintf((" usart module RCREG\n"));


  switch(receive_state) {
  case RS_WAITING_FOR_START:
    Dprintf(("waiting for start\n"));
    break;
  case RS_RECEIVING:

    if (bIsHigh(m_cLastRXState)) {

      receive_state = RS_WAITING_FOR_START;
      decode_byte(start_bit_event,0);

    } else {
      receive_state = RS_WAITING_FOR_START;
      cout << "Looks like we've overrun\n";
    }

    break;
  case RS_STOPPED:
    receive_state = RS_WAITING_FOR_START;
    cout << "received a stop bit\n";
    break;
  default:
    break;
  }


};

//------------------------------------------------------------------------
void RCREG::start() 
{

  receive_state = RS_RECEIVING;

  start_bit_event = rx_event->get_index();

  future_time = last_time + time_per_packet;

  if(!autobaud) {
    get_cycles().set_break(future_time, this);
  }

  Dprintf((" usart module RCREG last_cycle=%llx future_cycle=%llx\n", last_time,future_time));
}

//------------------------------------------------------------------------

unsigned int RCREG::decode_byte(guint32 sindex, guint64 bit_time)
{

  return '?';
}


//------------------------------------------------------------------------
//  new_rx_edge(bool bit)
//
//  This routine get's called when there's a change on the
//  RX line. The time the edge occurred is stored into an
//  event buffer. No effort is made here to decode a byte;
//  instead, decoding will take place in callback().

void RCREG::new_rx_edge(bool bit) 
{
  /**/
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
  case RS_0:
    cout << "state = RS_0\n";
    break;
  case RS_1:
    cout << "state = RS_1\n";
    break;
  case RS_2:
    cout << "state = RS_2\n";
    break;
  case RS_3:
    cout << "state = RS_3\n";
    break;
  case RS_4:
    cout << "state = RS_4\n";
    break;

  }

  // Save the event state
  char currentRXState = rxpin->getBitChar();
  rx_event->event(currentRXState);

  if (currentRXState != m_cLastRXState) {

    m_cLastRXState = currentRXState;

    switch(receive_state) {
    case RS_WAITING_FOR_START:
      if(bIsLow(currentRXState)) {
	Dprintf(("Start bit at t=0x%llx\n",rx_event->get_time(start_bit_event)));
	start();
      }

      break;
    case RS_RECEIVING:
      break;
    case RS_OVERRUN:
      break;
    default:
      break;
    }

  /**/


  }

}



//------------------------------------------------------------------------
class USART_IO : public IOPIN
{
public:


  USART_IO(void) {
    cout << "USART_IO constructor - do nothing\n";
  }
  USART_IO (IOPORT *i, unsigned int b, char *opt_name=NULL) : IOPIN(i,b,opt_name) { };

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

    value.put( (value.get() & ~mask) | (new_value ? mask : 0));

    if(usart)
      //get_trace().module1( value.get() & 0xf);
      get_trace().raw( value.get() & 0xf);
  }
};


//--------------------------------------------------------------
//  Usart Attributes - derived from gpsim's FloatAttribute
// 
//--------------------------------------------------------------
class UsartAttribute : public Float {

public:
  enum UA_TYPE {
    UA_BAUDRATE,
    UA_STARTBITS,
    UA_STOPBITS,
  } ;

  UA_TYPE type;


  UsartAttribute(char *_name, UA_TYPE new_type, double def_val) :
    Float(def_val)
  {

    cout << "USART Attribute constructor\n";

    type = new_type;
    new_name(_name);
  }


  void set(double new_val) {

    cout << "Setting usart attribute\n";

    Float::set(new_val);

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
//   txbaud       9600
//   rxbaud       9600
//   txreg         --
//   rxreg         --
//   parity        0
//   start_bits    1
//   stop_bits     1
//   

class BaudRateAttribute : public Integer 
{

public:
  USARTModule *usart;

  BaudRateAttribute(USARTModule *pusart, char *_name=NULL)
    : Integer(9600)
  {

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


  void set(Value *v) {

    cout << "Setting baud rate attribute!\n";
    gint64 old_value;
    get(old_value);
    Integer::set(v);

    gint64 new_value;
    get(new_value);
    cout << " old value: " << dec << old_value << " New value: " << new_value << endl;

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


  void set(Value *v) {


    cout << "Setting Rx baud rate attribute!\n";
    BaudRateAttribute::set(v);

    gint64 b;
    get(b);
    if(rcreg)
      rcreg->set_baud_rate(b);

  };


};

class TxBaudRateAttribute : public BaudRateAttribute
{

public:
  TXREG *txreg;

  TxBaudRateAttribute(USARTModule *pusart):BaudRateAttribute(pusart,"txbaud") {

    if(!pusart || !pusart->usart || !pusart->usart->txreg)// || !pbrg)
      return;

    txreg = pusart->usart->txreg;

    cout << "TxBaudRateAttribute constructor\n";

  };


  void set(Value *v) 
  {
    cout << "Setting Tx baud rate attribute!\n";
    BaudRateAttribute::set(v);

    gint64 b;
    get(b);
    if(txreg)
      txreg->set_baud_rate(b);

  }
};

class TxBuffer : public Integer
{
  TXREG *txreg;
public:
  TxBuffer(TXREG *_txreg)
    : Integer("tx",0,"UART Transmit Register"),txreg(_txreg)
  {
  }
  virtual void set(gint64 i)
  {
    i &= 0xff;

    cout << name() << " sending byte 0x" << hex << i << endl;

    if(txreg)
      txreg->mSendByte(i);

    Integer::set(i);
  }

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
	  rcreg->new_rx_edge(bit ? true : false);

}

//--------------------------------------------------------------
void USART_CORE::initialize(USART_IOPORT *new_iop)
{

  port = new_iop;

  rcreg = new RCREG;
  txreg = new TXREG;
  //txreg->enable();

}
//--------------------------------------------------------------
static unsigned int _tx_index=0;
static unsigned char Test_Hello[] = {
  0x1b,0xff, 0x87,0x05, 'H', 'E',  'L', 'L', 'O', 0x17, 0x55
};
bool USART_CORE::mGetTxByte(unsigned int &aByte)
{
  if (_tx_index > sizeof(Test_Hello))
    return false;

  aByte = Test_Hello[_tx_index++]; 
  return true;

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

  char *pin_name = (char*)name().c_str();   // Get the name of this usart
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

  get_symbol_table().add_stimulus(get_pin(1));
  get_symbol_table().add_stimulus(get_pin(2));
  get_symbol_table().add_stimulus(get_pin(3));
  get_symbol_table().add_stimulus(get_pin(4));


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

Module * USARTModule::USART_construct(const char *_new_name)
{

  cout << "USART construct \n";

  USARTModule *um = new USARTModule(_new_name);
  /*
  if(new_name) {
    um->new_name((char*)new_name);
  }
  */
  um->create_iopin_map();

  RxBaudRateAttribute *rxattr = new RxBaudRateAttribute(um);
  um->add_attribute(rxattr);

  TxBaudRateAttribute *txattr = new TxBaudRateAttribute(um);
  um->add_attribute(txattr);

  return um;

}

USARTModule::USARTModule(const char *_name)
{

  port = NULL;
  usart = NULL;

  new_name(_name);
}

USARTModule::~USARTModule()
{
    // FIXME
}

