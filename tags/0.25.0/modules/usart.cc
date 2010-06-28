/*
   Copyright (C) 1998,1999,2000,2001 T. Scott Dattalo

This file is part of the libgpsim_modules library of gpsim

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see 
<http://www.gnu.org/licenses/lgpl-2.1.html>.
*/

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
#define DEFAULT_BAUD    9600

#include <errno.h>
#include <stdlib.h>
#include <string>
#include <math.h>  // for floor()

#include "../config.h"    // get the definition for HAVE_GUI
#ifdef HAVE_GUI
#define GTK_ENABLE_BROKEN
#include <gtk/gtk.h>
#endif

#include "usart.h"


#include <cstdio>
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


//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d-%s() ",__FILE__,__LINE__,__FUNCTION__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

#define HAVE_TXFIFO

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
//

class USART_RXPIN : public IO_bi_directional_pu
{
public:

  USARTModule *usart;

  USART_RXPIN (USARTModule *_usart,
               const char *opt_name=NULL) : IO_bi_directional_pu(opt_name) {

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




//--------------------------------------------------------------
//
//

class USART_TXPIN : public IO_bi_directional
{
private:
  USART_TXPIN() {
  }
public:

  USARTModule *usart;

  USART_TXPIN (USARTModule *_usart,
               const char *opt_name=NULL)
  {

    usart = _usart;

    string n(usart->name());
    n = n + ".TXPIN";
    IO_bi_directional(n.c_str());
    new_name(n.c_str());
    bDrivingState = true;
    update_direction(1,true);   // Make the TX pin an output.

  };

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
  USARTModule *usart;

  virtual bool is_empty() { return empty_flag;};
  virtual void empty() {empty_flag = 1;};
  virtual void full()  {empty_flag = 0;};
  virtual void assign_pir_set(PIR_SET *new_pir_set){};

  TXREG(void) {
    txpin = 0;
    usart = 0;

    bits_per_byte = 8;
    stop_bits = 1;
    use_parity = 0;
    set_baud_rate(DEFAULT_BAUD);

    tx_byte = '0';

    update_packet_time();

    empty();
  }



  void update_packet_time(void) {

    if(baud <= 0.0)
      baud = 9600;  //arbitrary

    // Calculate the total time to send a "packet", i.e. start bit, data, parity, and stop
    // The stop bit time is included in the total packet time

    if(get_active_cpu()) {
      time_per_packet =
        (guint64)( get_cycles().instruction_cps() * (
                          (1.0 +             // start bit
                          bits_per_byte +   // data bits
                          stop_bits  +      // stop bit(s)
                          use_parity)       //
                         /baud));
      time_per_bit = (guint64)(get_cycles().instruction_cps() / baud);
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
    if(0) {
      cout << " usart module TXREG::" << __FUNCTION__ << "\n";
    }

    last_time = get_cycles().get();
    start_time = last_time;

    if(txpin) {
      txpin->putState((txr & 1) ? true : false);
      if (0)
      {
          cout << "usart tx module sent a " << (txr&1) <<
                " bit count " << bit_count << '\n';
      }
    }

    if(bit_count) {
      txr >>= 1;
      bit_count--;
      future_time = last_time + time_per_bit;
      get_cycles().set_break(future_time, this);
    } else {
      // We've sent the whole byte.
      /* output data from buffer if configured */
#ifdef HAVE_TXFIFO
      if(usart && usart->mGetTxByte(tx_byte))
        mSendByte(tx_byte);
      else
#endif
        empty();
    }

  }


  void mSendByte(unsigned _tx_byte)
  {

    if(0) {
      cout << "\n\n";
      cout << "TXREG::" << __FUNCTION__ << "\n";
      cout << "\n\n";
    }

    mBuildTXpacket(_tx_byte);
    last_time = get_cycles().get();
    future_time = last_time + time_per_bit;
    get_cycles().set_break(future_time, this);
    full();
  }

private:
  void mBuildTXpacket(unsigned int tb) {


    tx_byte = tb &  ((1<<bits_per_byte) - 1);

    txr =  ((3 << bits_per_byte) | tx_byte) << 1;

    // total bits = byte + start and stop bits
    bit_count = bits_per_byte + 1 + 1;

    if (0)
    {
        cout << hex << "TXREG::" << __FUNCTION__ << " byte to send 0x" << tb
         <<" txr 0x" << txr << "  bits " << bit_count << '\n';
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
    RS_RECEIVING,
    RS_STOPPED,
    RS_OVERRUN,
    RS_START_BIT
  } receive_state;

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
        (guint64)(get_cycles().instruction_cps() * (1.0 +     // start bit
                               bits_per_byte +   // data bits
                               stop_bits  +      // stop bit(s)
                               use_parity)       //
                               /baud);

      time_per_bit = (guint64)(get_cycles().instruction_cps() / baud);
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
  void new_rx_edge(bool bit);

private:
  USARTModule *m_usart;

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
  unsigned int rx_byte;
  int     rx_count;

  guint64 time_per_packet;

  bool autobaud;

  IOPIN   *rcpin;
  unsigned int *fifo;



};

//------------------------------------------------------------------------

RCREG::RCREG(USARTModule *pUsart)
  : m_usart(pUsart), m_cLastRXState('?'), start_bit_event(0), rcpin(0)
{
  assert(m_usart);


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

  Dprintf((" usart module RCREG time:0x%"PRINTF_GINT64_MODIFIER"x=%"PRINTF_GINT64_MODIFIER"d\n", get_cycles().get(), get_cycles().get()));


  switch(receive_state) {
  case RS_WAITING_FOR_START:
    Dprintf(("waiting for start\n"));
    break;

  case RS_START_BIT:    // should now be in middle of start bit
     if (bIsLow(m_cLastRXState))
     {
        receive_state = RS_RECEIVING;
        rx_count = bits_per_byte + use_parity;
        rx_byte = 0;
        future_time = get_cycles().get() + time_per_bit;

        if(!autobaud)
          get_cycles().set_break(future_time, this);

     }
     else // Not valid start bit
     {
       receive_state = RS_WAITING_FOR_START;
     }
     break;

  case RS_RECEIVING:

    if (rx_count--)
    {
        rx_byte = (rx_byte >> 1) | (bIsHigh(m_cLastRXState) ?
                1<<(bits_per_byte-1) : 0);
        future_time = get_cycles().get() + time_per_bit;

        if(!autobaud)
          get_cycles().set_break(future_time, this);
    }
    else if (bIsHigh(m_cLastRXState)) // on stop bit
    {
        m_usart->newRxByte(rx_byte);
        m_usart->show_tx(rx_byte);
        receive_state = RS_WAITING_FOR_START;
    }
    else
    {
        cout << "USART module RX overrun error\n";
        receive_state = RS_WAITING_FOR_START;
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

  receive_state = RS_START_BIT;


  future_time = get_cycles().get() + time_per_bit/2;

  if(!autobaud) {
    get_cycles().set_break(future_time, this);
  }

  Dprintf((" usart module RCREG current cycle=0x%"PRINTF_GINT64_MODIFIER"x future_cycle=0x%"PRINTF_GINT64_MODIFIER"x\n", get_cycles().get(),future_time));
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

  // Save the event state
  char currentRXState = rxpin->getBitChar();

  if (currentRXState != m_cLastRXState) {

    m_cLastRXState = currentRXState;

    switch(receive_state) {
    case RS_WAITING_FOR_START:
      if(bIsLow(currentRXState)) {
        start();
        Dprintf(("Start bit at t=0x%"PRINTF_GINT64_MODIFIER"x\n",get_cycles().get()));
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
class USART_IO : public IO_bi_directional_pu
{
public:

  USARTModule *usart;

  USART_IO(void) {
    cout << "USART_IO constructor - do nothing\n";
  }

  USART_IO ( USARTModule *_usart,
             unsigned int b,
             const char *opt_name ) : IO_bi_directional_pu(opt_name) {
    usart = _usart;

    string n(usart->name());
    n = n + "." + opt_name;
    new_name(n.c_str());

    bDrivenState = true;
    update_direction(0,true);   // Make the RX pin an input.

    bPullUp = true;
    Zpullup = 10e3;
  }

  void setDrivenState(bool new_dstate) {
    bool diff = new_dstate ^ bDrivenState;

//    Dprintf((" usart module %s new state=%d\n",name(),new_dstate));

    if( usart && diff ) {

      bDrivenState = new_dstate;
      IOPIN::setDrivenState(new_dstate);

    }

  }
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

class TxBaudRateAttribute : public Integer
{

public:
  TXREG *txreg;

  TxBaudRateAttribute(TXREG *ptxreg)
    : Integer("txbaud",DEFAULT_BAUD,"USART Module Transmitter baud rate"), txreg(ptxreg)
  {
    assert(txreg);
  }

  void set(Value *v)
  {
    Integer::set(v);

    gint64 b;
    get(b);
    txreg->set_baud_rate(b);
    cout << "Setting Tx baud rate attribute to " << dec << b << "\n";
  }
  virtual string toString()
  {
    return Integer::toString("%" PRINTF_INT64_MODIFIER "d");
  }

};

class TxBuffer : public Integer
{
  USARTModule *usart;
public:
  TxBuffer(USARTModule *_usart)
    : Integer("tx",0,"UART Transmit Register"),usart(_usart)
  {
  }
  virtual void set(gint64 i)
  {
    i &= 0xff;

    //cout << name() << " sending byte 0x" << hex << i << endl;

    if(usart)
      usart->SendByte(i);

    Integer::set(i);
  }
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
    Dprintf((" RxBuffer received a byte: 0x%02x=%d=%c",(int)b, (int)b, (int)b));
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
  if(m_loop->getVal())
  {
        SendByte(aByte);
  }
}

//--------------------------------------------------------------
#ifndef HAVE_TXFIFO
static unsigned int _tx_index=0;
static unsigned char Test_Hello[] = {
  0x1b,0xff, 0x87,0x05, 'H', 'E',  'L', 'L', 'O', 0x17, 0x55
};
bool USARTModule::mGetTxByte(unsigned int &aByte)
{
  if (_tx_index > sizeof(Test_Hello))
    return false;

  aByte = Test_Hello[_tx_index++];
  return true;

}
#else
bool USARTModule::mGetTxByte(unsigned int &aByte)
{
    if ( m_FifoHead == m_FifoTail )
        return false;
    aByte = m_TxFIFO[m_FifoTail];
    if ( m_FifoTail < m_FifoLen-1 )
        m_FifoTail++;
    else
        m_FifoTail = 0;
    return true;
}
#endif

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

  USART_TXPIN *txpin = new USART_TXPIN(this, "TXPIN");
  USART_RXPIN *rxpin = new USART_RXPIN(this, "RXPIN");


  assign_pin(1, txpin);
  assign_pin(2, rxpin);
  assign_pin(3, new USART_IO(this, 2, "CTS"));
  assign_pin(4, new USART_IO(this, 3, "RTS"));

  // Complete the usart initialization

  m_txreg->txpin = txpin;
  m_txreg->usart = this; // Point back to the module
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

  USARTModule *um = new USARTModule( (_new_name ?_new_name:"USART"));
  um->create_iopin_map();

  return um;

}

USARTModule::USARTModule(const char *_name) : Module(_name, "USART")
{

#ifdef HAVE_TXFIFO
  m_TxFIFO = new unsigned char[64];
  m_FifoLen = 64;
  m_FifoHead = m_FifoTail = 0;
#endif

  m_rcreg = new RCREG(this);
  m_txreg = new TXREG;

  m_RxBaud = new RxBaudRateAttribute(m_rcreg);
  addSymbol(m_RxBaud);

  m_TxBaud = new TxBaudRateAttribute(m_txreg);
  addSymbol(m_TxBaud);

  m_RxBuffer = new RxBuffer(m_rcreg);
  addSymbol(m_RxBuffer);

  m_TxBuffer = new TxBuffer(this);
  addSymbol(m_TxBuffer);

  m_CRLF = new Boolean("crlf", true, "if true, carriage return and linefeeds generate new lines in the terminal");
  addSymbol(m_CRLF);

  m_ShowHex = new Boolean("hex", false, "if true, display received data in hex - i.e. assume binary");
  addSymbol(m_ShowHex);

  m_loop = new Boolean("loop", false, "if true, received characters looped back to transmit");
  addSymbol(m_loop);

  m_console = new Boolean("console", false, "if true, display received character to the terminal window");
  addSymbol(m_console);

  CreateGraphics();


  assert(m_rcreg);
  assert(m_txreg);
  assert(m_RxBaud);
  assert(m_TxBaud);
  assert(m_RxBuffer);
  assert(m_TxBuffer);

}

USARTModule::~USARTModule()
{
#ifdef HAVE_GUI
    if(window)
        gtk_widget_destroy(window);
#endif

#ifdef HAVE_TXFIFO
    delete m_TxFIFO;
#endif

    removeSymbol(m_RxBaud);
    removeSymbol(m_TxBaud);
    removeSymbol(m_RxBuffer);
    removeSymbol(m_TxBuffer);
    removeSymbol(m_CRLF);
    removeSymbol(m_ShowHex);
    removeSymbol(m_loop);
    removeSymbol(m_console);

    delete m_rcreg;
    delete m_txreg;
    delete m_RxBaud;
    delete m_TxBaud;
    delete m_RxBuffer;
    delete m_TxBuffer;
    delete m_CRLF;
    delete m_ShowHex;
    delete m_loop;
    delete m_console;
}

//--------------------------------------------------------------
void USARTModule::SendByte(unsigned tx_byte)
{
#ifdef HAVE_TXFIFO
    Dprintf (( "SendByte <%02X> : head=%d, tail=%d, txreg=%p\n",
               tx_byte, m_FifoHead, m_FifoTail, m_txreg ))
    if ( m_FifoHead != m_FifoTail || !m_txreg || !m_txreg->is_empty() )
    {
        int newHead;

        m_TxFIFO[m_FifoHead] = tx_byte;
        newHead = m_FifoHead+1;
        if ( newHead >= m_FifoLen )
            newHead = 0;
        if ( newHead == m_FifoTail )
        {
            int newLen = m_FifoLen + 32;
            unsigned char * newFIFO;
            newFIFO = new unsigned char[newLen];
            int oldTail = m_FifoTail;
            int dIdx = 0;
            int sIdx;
            for ( sIdx = oldTail; sIdx < m_FifoLen; )
                newFIFO[dIdx++] = m_TxFIFO[sIdx++];
            for ( sIdx = 0; sIdx < newHead; )
                newFIFO[dIdx++] = m_TxFIFO[sIdx++];
            unsigned char * oldFIFO = m_TxFIFO;
            m_TxFIFO = newFIFO;
            m_FifoTail -= oldTail;
            m_FifoHead = dIdx;
            m_FifoLen = newLen;
            delete oldFIFO;
        }
        else
            m_FifoHead = newHead;
        //cout << "Byte added to queue\n";
    }
    else
#endif
        if (m_txreg)
            m_txreg->mSendByte(tx_byte);
};

#ifdef HAVE_GUI
static bool ctl = false;        // true when ctrl key is down

static gint
key_press(GtkWidget *widget,
          GdkEventKey *key,
          gpointer data)
{
        unsigned int c = key->keyval;

        gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "key_press_event");
        if(c == 0xffe3 || c == 0xffe4) // key is left or right ctrl
        {
            ctl = true;
            return(1);
        }
        if (ctl && c < 0xff00)  // build control character
        {
                Dprintf(("CTL 0x%02x\n", c));
                c &= 0x1f;
        }
        if ( c < 0xff20)        // send character to usart
        {
            c &= 0xff;
            ((USARTModule *)data)->USARTModule::SendByte(c);
            Dprintf(("Send %c 0x%x\n", c,c));
        }
        else
        {
                Dprintf(( "0x%02x\n", c));
        }
        return(1);
}
static gint
key_release(GtkWidget *widget,
          GdkEventKey *key,
          gpointer data)
{
        unsigned int c = key->keyval;
        if(c == 0xffe3 || c == 0xffe4)  // Capture release of ctrl key
        {
            ctl = false;
        }
        return(1);
}
#endif //HAVE_GUI

// Display character from usart on GUI text window
void USARTModule::show_tx(unsigned int data)
{
  data &= 0xff;
  bool IsAscii = true;

  if ( m_ShowHex->getVal() )
    IsAscii = false;
  else if ( (isascii(data) && isprint(data)) )
    IsAscii = true;
  else if (m_CRLF->getVal() && ('\n' == data || '\r' == data))
    IsAscii = true;
  else
    IsAscii = false;

  if(m_console->getVal()) {
    if ( IsAscii )
      putchar(data);
    else
      printf("<%02X>", data);
  }

#ifdef HAVE_GUI
  if(get_interface().bUsingGUI()) {
      GtkTextBuffer *buff = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
      GtkTextIter iter;
      GtkTextMark *insert_mark;
      gtk_text_buffer_get_end_iter(buff, &iter);
      if (IsAscii) {
        char ch = data;
        gtk_text_buffer_insert(buff, &iter, &ch, 1);
      }
      else {
        char hex[5];
        sprintf (hex, "<%02X>", data);
        gtk_text_buffer_insert(buff, &iter, hex, 4);
      }

      /* get end iter again */
      gtk_text_buffer_get_end_iter (buff, &iter);
 
      /* get the current ( cursor )mark name */
      insert_mark = gtk_text_buffer_get_insert (buff);
 
      /* move mark and selection bound to the end */
      gtk_text_buffer_place_cursor(buff, &iter);

      /* scroll to the end view */
      gtk_text_view_scroll_to_mark( GTK_TEXT_VIEW (text),
                insert_mark, 0.0, TRUE, 0.0, 1.0); }
#endif //HAVE_GUI
}

// Create a GUI text window
void USARTModule::CreateGraphics()
{
#ifdef HAVE_GUI
  if(get_interface().bUsingGUI()) {
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "USART");
    gtk_window_set_default_size (GTK_WINDOW (window), 300, 100);

    GtkWidget *pSW = gtk_scrolled_window_new (0,0);
    gtk_container_add (GTK_CONTAINER (window), pSW);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (pSW),
                                    GTK_POLICY_AUTOMATIC,
                                    GTK_POLICY_AUTOMATIC);
    text = gtk_text_view_new ();
    gtk_text_view_set_editable (GTK_TEXT_VIEW (text), TRUE);
    gtk_container_add (GTK_CONTAINER (pSW), text);

    /* Change default font throughout the widget */
    PangoFontDescription *font_desc;
    font_desc = pango_font_description_from_string ("Courier 10");
    gtk_widget_modify_font (text, font_desc);
    pango_font_description_free (font_desc);

    gtk_widget_add_events(window, GDK_KEY_RELEASE_MASK);

    gtk_signal_connect(GTK_OBJECT(text),"key_press_event",
                       (GtkSignalFunc) key_press,
                       this);
    gtk_signal_connect(GTK_OBJECT(text),"key_release_event",
                       (GtkSignalFunc) key_release,
                       this);

    gtk_signal_connect (GTK_OBJECT (window), "destroy",
                        GTK_SIGNAL_FUNC (gtk_widget_destroy), window);

    gtk_widget_show_all(window);

  } else {
    window = 0;
    text = 0;
  }
#endif  // HAVE_GUI
}
