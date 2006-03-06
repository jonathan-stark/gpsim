/*
   Copyright (C) 1998-2006 T. Scott Dattalo

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

  usart_con.cc

  This is gpsim's universal synchronous/asynchronous receiver,
  derived from gpsim usart.cc module 2006-03-03 by Borut Razem,
  used for sdcc pic regression tests.

  Features:

    8 or 9 bit receiver
    0 or 1 start bits
    0 or 1 stop bits
    0 or 1 parity bits and even/odd selectable
    variable sized receive buffers

*/


/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include "usart_con.h"

#define DEFAULT_BAUD    9600

#include <errno.h>
#include <stdlib.h>
#include <string>
#include <math.h>  // for floor()

#include <gpsim/value.h>
#include <gpsim/modules.h>
#include <gpsim/packages.h>
#include <gpsim/stimuli.h>
#include <gpsim/ioports.h>
#include <gpsim/symbol.h>
#include <gpsim/interface.h>
#include <gpsim/pir.h>
#include <gpsim/trace.h>
#include <gpsim/uart.h>
#include <gpsim/bitlog.h>


#ifdef _MSC_VER
#define PRINTF_INT64_MODIFIER "I64"
#else
#define PRINTF_INT64_MODIFIER "ll"
#endif

//#define DEBUG
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

             gpsim's USART CONSOLE module


  The  USART module is a general purpose universal synchronous/asynchronous
serial receiver. In other words, it's a serial port. It's purpose is to provide
a tool to display characters, received from serial interface, to the console.
Users can load this module and tie it to the transmit pin of simulated PIC.

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
//

class USART_RXPIN : public IO_bi_directional_pu
{
public:

  USARTModule *usart;

  USART_RXPIN (USARTModule *_usart,
               unsigned int b, 
               char *opt_name=NULL) : IO_bi_directional_pu(0,b,opt_name) { 

    usart = _usart;

    string n(usart->name());
    n = n + ".RXPIN";
    new_name(n.c_str());

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


//=================================================================
//
//  RCREG 
//
// Create a receive register 
// 
//
char *RXError_str[] = {"OK", "NoStartBit", "NoStopBit", "BadParity", NULL };

class RCREG : public TriggerObject
{
 public:

  USART_RXPIN *rxpin;

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

  enum RXErrors {
    eNoError,
    eNoStartBit,
    eNoStopBit,
    eBadParity
  };

  /**************************/
  // RCREG constructor
  /**************************/
  RCREG(USARTModule *);

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

  virtual void callback();

  void start();
  RXErrors decode_byte(unsigned sindex, unsigned int &aByte);
  void new_rx_edge(bool bit);

private:
  USARTModule *m_usart;
  ThreeStateEventLogger *rx_event;

  char m_cLastRXState;
  unsigned int start_bit_event;


  guint32 error_flag;


  guint64 time_per_bit;
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
  unsigned int *fifo;



};

//------------------------------------------------------------------------

RCREG::RCREG(USARTModule *pUsart)
  : m_usart(pUsart), rcpin(0), start_bit_event(0), m_cLastRXState('?')
{
  assert(m_usart);

  rx_event = new ThreeStateEventLogger(1024);

  receive_state = RS_WAITING_FOR_START;

  autobaud = false;
  set_bits_per_byte(8);
  set_stop_bits(0.9);
  set_noparity();

  set_baud_rate(DEFAULT_BAUD);
}

//------------------------------------------------------------------------
void RCREG::callback() 
{

  Dprintf((" usart module RCREG time:0x%llx=%lld\n",get_cycles().get(),get_cycles().get()));


  switch(receive_state) {
  case RS_WAITING_FOR_START:
    Dprintf(("waiting for start\n"));
    break;
  case RS_RECEIVING:

    if (bIsHigh(m_cLastRXState)) {

      receive_state = RS_WAITING_FOR_START;

      Dprintf((" received a byte\n"));
      unsigned int rx_byte=0;
      RXErrors re = decode_byte(start_bit_event,rx_byte);
      if (re == eNoError) {
//      Dprintf((" Successfully recieved 0x%02x\n",rx_byte));
        putchar(rx_byte);
        m_usart->newRxByte(rx_byte);
      } else {
        printf("RCREG::callback Failed to decode byte %s \n", 
                RXError_str[re]);
      }

    } else {
      receive_state = RS_WAITING_FOR_START;
      cout << "Looks like we've overrun\n";
#if defined(DEBUG)
      cout << "Baud:" << baud << " = " << time_per_bit << " cycles\n";
      cout << "Log of received data:\n";
      rx_event->dump(rx_event->get_index(get_cycles().get() - 2000));
      rx_event->dump_ASCII_art(1,get_cycles().get() - 2000);
#endif
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

  future_time = rx_event->get_time(start_bit_event) + time_per_packet;

  if(!autobaud) {
    get_cycles().set_break(future_time, this);
  }

  Dprintf((" usart module RCREG current cycle=%lld future_cycle=%lld\n", get_cycles().value,future_time));
}

//------------------------------------------------------------------------
// decode_byte
// 
// decode_byte will examine the data logged from the receiver's I/O 
// pin and attempt to decipher it.
//
//    +---------------------------------------------  Edge of start bit
//    | +-------------------------------------------  Middle of start bit
//  __v v  ___ ___ ___ ___ ___ ___ ___ ___ ____
//    \___/___X___X___X___X___X___X___X___/    
//          ^   ^   ^   ^   ^   ^   ^   ^   ^
//          |   |   |   |   |   |   |   |   +-------  Middle of stop bit
//          +---+---+---+---+---+---+---+-----------  Middle of data bits
//
// The input to decode_byte is the index into the event log holding
// the captured waveform. The baud rate for the receiver is used to
// compute the bit time and this in turn is used to compute the times
// at which the data samples should be taken.
//

RCREG::RXErrors RCREG::decode_byte(unsigned int sindex, unsigned int &rx_byte)
{

  Dprintf((" decode_byte start_bit_index=%d \n", sindex));

  rx_byte = 0;

  if (!bIsLow(rx_event->get_state(sindex)))
    return eNoStartBit;

  guint64 sample_time = rx_event->get_time(sindex) + time_per_bit/2;

  Dprintf((" decode_byte sample_time=%lld start_bit_time=%lld time_per_bit=%d\n", sample_time,
           rx_event->get_time(sindex), time_per_bit));

  if (!bIsLow(rx_event->get_state(sample_time)))
    return eNoStartBit;

  sample_time += time_per_bit;

  unsigned int msb = 1<<(bits_per_byte-1);
  for (int i=0; i<bits_per_byte; i++, sample_time+=time_per_bit)
    rx_byte = (rx_byte>>1) | (bIsHigh(rx_event->get_state(sample_time))?msb:0);

  if (!bIsHigh(rx_event->get_state(sample_time)))
    return eNoStopBit;

  Dprintf((" decoded %d=%x=%c \n", rx_byte,rx_byte,rx_byte));

  return eNoError;
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
#if defined(DEBUG)
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
#endif

  // Save the event state
  char currentRXState = rxpin->getBitChar();
  rx_event->event(currentRXState);

  if (currentRXState != m_cLastRXState) {

    m_cLastRXState = currentRXState;

    switch(receive_state) {
    case RS_WAITING_FOR_START:
      if(bIsLow(currentRXState)) {
        start();
        Dprintf(("Start bit at t=0x%llx\n",rx_event->get_time(start_bit_event)));
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
  USART_IO (unsigned int b, char *opt_name=NULL) : IOPIN(0,b,opt_name) { };

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
//   rxbaud       9600
//   rxreg         --
//   parity        0
//   start_bits    1
//   stop_bits     1
//   


class RxBaudRateAttribute : public Integer
{

public:
  RCREG *rcreg;

  RxBaudRateAttribute(RCREG *prcreg)
    : Integer("rxbaud",DEFAULT_BAUD,"USART Module Receiver baud rate"), rcreg(prcreg)
  {
    assert(rcreg);
  }


  void set(Value *v) {


    Integer::set(v);

    gint64 b;
    get(b);
    rcreg->set_baud_rate(b);
    cout << "Setting Rx baud rate attribute to " << dec << b << "\n";

  };
  virtual string toString()
  {
    return Integer::toString("%" PRINTF_INT64_MODIFIER "d");
  }

};


class RxBuffer : public Integer
{
  RCREG *rcreg;
public:
  RxBuffer(RCREG *_rcreg)
    : Integer("rx",0,"UART Receive Register"),rcreg(_rcreg)
  {
  }
  virtual void set(gint64 i)
  {
    cout << "Receive buffer is read only\n";
  }
  virtual string toString()
  {
    return Integer::toString("%" PRINTF_INT64_MODIFIER "d");
  }

  void newByte(gint64 b) 
  {
    Dprintf((" RxBuffer received a byte: 0x%02x=%d=%c",b,b,b));
    Integer::set(b);
  }
};

//--------------------------------------------------------------
void USARTModule::new_rx_edge(unsigned int bit)
{
  if(m_rcreg)
    m_rcreg->new_rx_edge(bit ? true : false);
}

//--------------------------------------------------------------
void USARTModule::newRxByte(unsigned int aByte)
{
  m_RxBuffer->newByte(aByte);
}

//--------------------------------------------------------------
// create_iopin_map 
//
//  This is where the information for the Module's package is defined.
// Specifically, the I/O pins of the module are created.

#define USART_PKG_RXPIN        1
#define USART_PKG_CTSPIN       2

void USARTModule::create_iopin_map(void)
{

  // Define the physical package.
  //   The Package class, which is a parent of all of the modules,
  //   is responsible for allocating memory for the I/O pins.
  //
  //   USART I/O pins:
  //
  //    1 - Rx - Receive
  //    2 - CTS - Clear To Send

  create_pkg(2);


  // Define the I/O pins and assign them to the package.
  //   There are two things happening here. First, there is
  //   a new I/O pin that is being created.The second thing is
  //   that the pins are "assigned" to the package. If we
  //   need to reference these newly created I/O pins (like
  //   below) then we can call the member function 'get_pin'.

  USART_RXPIN *rxpin = new USART_RXPIN(this, 0,"RX");


  assign_pin(1, rxpin);
  assign_pin(2, new USART_IO(2,"CTS"));

  // Complete the usart initialization

  m_rcreg->rxpin = rxpin;

}
//--------------------------------------------------------------
void USARTModule::get(char *cP, int len)
{
  cout << "USARTModule::get(char *cP, int len)\n";
}

//--------------------------------------------------------------

Module * USARTModule::USART_construct(const char *_new_name)
{

  Dprintf(("USART construct\n"));

  USARTModule *um = new USARTModule( (_new_name ? _new_name : "USART_CON"));
  um->create_iopin_map();

  return um;

}

USARTModule::USARTModule(const char *_name)
{
  assert(_name);
  new_name(_name);

  m_rcreg = new RCREG(this);

  m_RxBaud = new RxBaudRateAttribute(m_rcreg);
  add_attribute(m_RxBaud);

  m_RxBuffer = new RxBuffer(m_rcreg);
  add_attribute(m_RxBuffer);

  CreateGraphics();


  assert(m_rcreg);
  assert(m_RxBaud);
  assert(m_RxBuffer);
}

USARTModule::~USARTModule()
{
    // FIXME
}

void USARTModule::CreateGraphics()
{
}
