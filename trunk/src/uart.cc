/*
   Copyright (C) 1998,1999 Scott Dattalo

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

#include "../config.h"
#include "stimuli.h"
#include "uart.h"
#include "14bit-processors.h"
#include "14bit-tmrs.h"

#include "xref.h"

//-----------------------------------------------------------
_RCSTA::_RCSTA(void)
{
  //cout << "_RCSTA constructor\n";
}

//-----------------------------------------------------------
_TXSTA::_TXSTA(void)
{
  //cout << "_TXSTA constructor\n";
}

//-----------------------------------------------------------
_RCREG::_RCREG(void)
{
  //cout << "_RCREG constructor\n";
}

_TXREG::_TXREG(void)
{
  //cout << "_TXREG constructor\n";
  new_name("txhello");
}


_SPBRG::_SPBRG(void)
{
  //cout << "_SPBRG constructor\n";
}

//-----------------------------------------------------------
// TXREG - USART Transmit Register


void _TXREG::put(unsigned int new_value)
{

  value = new_value & 0xff;

  if(verbose)
    cout << "txreg just got a new value\n";

  // The transmit register has data,
  // so clear the TXIF flag

  full();

  if(txsta &&
     ( (txsta->value & (_TXSTA::TRMT | _TXSTA::TXEN)) == (_TXSTA::TRMT | _TXSTA::TXEN)))
    {
      // If the transmit buffer is empty and the transmitter is enabled
      // then transmit this new data now...

      txsta->start_transmitting();
    }

  trace.register_write(address,value);

}

void _TXREG::put_value(unsigned int new_value)
{

  put(new_value);

  if(xref)
    xref->update();
}

//-----------------------------------------------------------
// TXSTA - Transmit Register Status and Control

void _TXSTA::put_value(unsigned int new_value)
{

  put(new_value);

  if(xref)
    xref->update();

}

void _TXSTA::put(unsigned int new_value)
{

  unsigned int old_value = value;

  // The TRMT bit is controlled entirely by hardware.
  // It is high if the TSR has any data.

  value = (new_value & ~TRMT) | ( (bit_count) ? 0 : TRMT);

  if(verbose)
    cout << "TXSTA::put 0x" << value << '\n';


  if( (old_value ^ value) & TXEN) {

    // The TXEN bit has changed states.
    //
    // If transmitter is being enabled and the transmit register
    // has some data that needs to be sent, then start a
    // transmission.
    // If the transmitter is being disabled, then abort any
    // transmission.

    if(value & TXEN) {
      cout << "TXSTA - enabling transmitter\n";
      if(txreg) {
	cout << " TXSTA - does have a txreg\n";
	  txreg->empty();
#if 0
	if(txreg->is_empty()) {
	  txreg->empty();
	} else {
          cout << "start_transmitting1" << endl;
	  start_transmitting();
      }
#endif
      }
    } else 
      stop_transmitting();
  }

  trace.register_write(address,value);

}

// _TXSTA::stop_transmitting(void)
//
void _TXSTA::stop_transmitting(void)
{
  if(verbose)
    cout << "stopping a USART transmission\n";

  bit_count = 0;
  value |= TRMT;

  // It's not clear from the documentation as to what happens
  // to the TXIF when we are aborting a transmission. According
  // to the documentation, the TXIF is set when the TXEN bit
  // is set. In other words, when the Transmitter is enabled
  // the txreg is emptied (and hence TXIF set). But what happens
  // when TXEN is cleared? Should we clear TXIF too?
  // 
  // There is one sentence that says when the transmitter is
  // disabled that the whole transmitter is reset. So I interpret
  // this to mean that the TXIF gets cleared. I could be wrong
  // (and I don't have a way to test it on a real device).
  // 
  // Another interpretation is that TXIF retains it state 
  // through changing TXEN. However, when SPEN (serial port) is
  // set then the whole usart is reinitialized and TXIF will
  // get set.
  //
  //  txreg->full();   // Clear TXIF

}

void _TXSTA::start_transmitting(void)
{
  if(verbose)
    cout << "starting a USART transmission\n";

  // Build the serial byte that's about to be transmitted.
  // I doubt the pic really does this way, but gpsim builds
  // the entire bit stream including the start bit, 8 data 
  // bits, optional 9th data bit and the stop, and places
  // this into the tsr register. But since the contents of
  // the tsr are inaccessible, I guess we'll never know.
  //
  // (BTW, if you look carefully you may puzzle over why
  //  there appear to be 2 stop bits in the packet built
  //  below. Well, it's because the way gpsim implements
  //  the transmit logic. The second stop bit doesn't
  //  actually get transmitted - it merely causes the first
  //  stop bit to get transmitted before the TRMT bit is set.
  //  [Recall that the TRMT bit indicates when the tsr 
  //  {transmit shift register} is empty. It's not tied to
  //  an interrupt pin, so the pic application software
  //  most poll this bit. This bit is set after the STOP
  //  bit is transmitted.] This is a cheap trick that saves
  //  one comparison in the callback code.)

  // The start bit, which is always low, occupies bit position
  // zero. The next 8 bits come from the txreg.
  if(!txreg)
    return;

  tsr = txreg->value << 1;

  // Is this a 9-bit data transmission?
  if(value & TX9)
    {
      // Copy the stop bit and the 9th data bit to the tsr.
      // (See note above for the reason why two stop bits 
      // are appended to the packet.)

      tsr |= ( (value & TX9D) ? (7<<9) : (6<<9));
      bit_count = 12;  // 1 start, 9 data, 1 stop , 1 delay
    }
  else
    {
      // The stop bit is always high. (See note above
      // for the reason why two stop bits are appended to
      // the packet.)
      tsr |= (3<<9);
      bit_count = 11;  // 1 start, 8 data, 1 stop , 1 delay
    }


  // Set a callback breakpoint at the next SPBRG edge

  if(cpu)
    cycles.set_break(spbrg->get_cpu_cycle(1), this);

  // The TSR now has data, so clear the Transmit Shift 
  // Register Status bit.

  value &= ~TRMT;
  trace.register_write(address,value);

  // Tell the TXREG that its data has been consumed.

  txreg->empty();

}

void _TXSTA::transmit_a_bit(void)
{


  if(bit_count)
    {

      //cout << "Transmit bit #" << bit_count << ": " << (tsr&1) << '\n';
      if(txpin)
	txpin->put_state(tsr&1);

      tsr >>= 1;

      --bit_count;

    }

}

void _TXSTA::callback(void)
{

  //cout << "RCSTA callback " << (cycles.value) << '\n';

  transmit_a_bit();

  if(!bit_count) {

    // tsr is empty.
    // If there is any more data in the TXREG, then move it to
    // the tsr and continue transmitting other wise set the TRMT bit

    // (See the note above about the 'extra' stop bit that was stuffed
    // into the tsr register. 

    if(txreg && txreg->is_empty())
      value |= TRMT;
    else
      start_transmitting();

  } else  {
    // bit_count is non zero which means there is still
    // data in the tsr that needs to be sent.

    if(cpu) {
      cycles.set_break(spbrg->get_cpu_cycle(1),this);
    }
  }

}
bool _TXREG::is_empty(void)
{
  cout << "TXREG:: function not implemented\n";
  return 0;
}
void _TXREG::empty(void)
{
  cout << "TXREG:: function not implemented\n";
}
void _TXREG::full(void)
{
  cout << "TXREG:: function not implemented\n";
}
void _TXREG::assign_pir_set(PIR_SET *new_pir_set)
{
  cout << "TXREG:: function not implemented\n";
}

//-----------------------------------------------------------
// Receiver portion of the USART
//-----------------------------------------------------------
//
// First RCSTA -- Receiver Control and Status 
// The RCSTA class controls the usart reception. The PIC usarts have
// two modes: synchronous and asynchronous.
// Asynchronous reception:
//   Asynchronous reception means that there is no external clock
// available for telling the usart when to sample the data. Sampling
// timing is all based upon the PIC's oscillator. The SPBRG divides
// this clock down to a frequency that's appropriate to the data
// being received. (e.g. 9600 baud defines the rate at which data
// will be sent to the pic - 9600 bits per second.) The start bit,
// which is a high to low transition on the receive line, defines
// when the usart should start sampling the incoming data.
//   The pic usarts sample asynchronous data three times in "approximately
// the middle" of each bit. The data sheet is not exactly specific
// on what's the middle. Consequently, gpsim takes a stab/ educated
// guess on when these three samples are to be taken. Once the
// three samples are taken, then simple majority summing determines
// the sample e.g. if two out of three of the samples are high, then
// then the data bit is considered high.
//
//-----------------------------------------------------------
// RCSTA::put
//
void _RCSTA::put(unsigned int new_value)
{
  unsigned int diff;

  diff = new_value ^ value;
  value = ( value & (RX9D | OERR | FERR) )   |  (new_value & ~(RX9D | OERR | FERR));

  if(!txsta || !txsta->txreg)
    return;
  // First check whether or not the serial port is being enabled
  if(diff & SPEN) {

    if(value & SPEN) {
      spbrg->start();
      // Make the tx line high when the serial port is enabled.
      if(txsta->txpin)
	txsta->txpin->put_state(1);
      txsta->txreg->empty();
    } else {

      // Completely disable the usart:

      txsta->stop_transmitting();
      txsta->txreg->full();         // Turn off TXIF
      stop_receiving();

      trace.register_write(address,value);
      return;
    }

  }

  if(!(txsta->value & _TXSTA::SYNC)) {

    // Asynchronous receive.
    if( (value & (SPEN | CREN)) == (SPEN | CREN) ) {

      // The receiver is enabled. Now check to see if that just happened
      
      if(diff & (SPEN | CREN)) {
    
	// The serial port has just been enabled. 
	start_receiving();

	// If the rx line is low, then go ahead and start receiving now.
	if(uart_port && !uart_port->get_bit(rx_bit))
	  receive_start_bit();
      }

    } else {
      // The serial port is not enabled.

      state = RCSTA_DISABLED;
    }

  } else
    cout << "not doing syncronous receptions yet\n";

  trace.register_write(address,value);
  
}

void _RCSTA::put_value(unsigned int new_value)
{

  put(new_value);

  if(xref)
    xref->update();
}

//-----------------------------------------------------------
// RCSTA::receive_a_bit(unsigned int bit)
//
// A new bit needs to be copied to the the Receive Shift Register.
// If the receiver is receiving data, then this routine will copy
// the incoming bit to the rsr. If this is the last bit, then a 
// check will be made to see if we need to set up for the next 
// serial byte.
// If this is not the last bit, then the receive state machine.

void _RCSTA::receive_a_bit(unsigned int bit)
{

  // If we're waiting for the start bit and this isn't it then
  // we don't need to look any further
  // cout << "receive_a_bit state " << state << "bit " << bit << endl;
  if( state == RCSTA_MAYBE_START) {
    if (bit)
      state = RCSTA_WAITING_FOR_START;
    else
      state = RCSTA_RECEIVING;
    return;
  }
  if (bit_count == 0) {
    // we should now have the stop bit
    if (bit) {
      // got the stop bit
      // If the rxreg has data from a previous reception then
      // we have a receiver overrun error.
      // cout << "rcsta.rsr is full\n";

      if((value & RX9) == 0)
        rsr >>= 1;

      // copy the rsr to the fifo
      if(rcreg)
        rcreg->push( rsr & 0xff);
       //cout << "_RCSTA::receive_a_bit received 0x" << (rsr & 0xff) << endl;

    } else {
      //not stop bit; discard the data and go back receiving
    }
    // If we're continuously receiving, then set up for the next byte.
    // FIXME -- may want to set a half bit delay before re-starting...
    if(value & CREN)
      start_receiving();
    else
      state = RCSTA_DISABLED;
    return;
  }


  // Copy the bit into the Receive Shift Register
  if(bit)
    rsr |= 1<<9;

  //cout << "Receive bit #" << bit_count << ": " << (rsr&(1<<9)) << '\n';

  rsr >>= 1;
  bit_count--;

}

void _RCSTA::stop_receiving(void)
{

  rsr = 0;
  bit_count = 0;
  state = RCSTA_DISABLED;

}

void _RCSTA::start_receiving(void)
{
  //cout << "The USART is starting to receive data\n";

  rsr = 0;
  sample = 0;

  // Is this a 9-bit data reception?
  if(value & RX9)
    {
      bit_count = 9;
    }
  else
    {
      bit_count = 8;
    }

  state = RCSTA_WAITING_FOR_START;

}

void _RCSTA::set_callback_break(unsigned int spbrg_edge)
{
  //  last_cycle = cycles.value;

  if(cpu && spbrg)
    cycles.set_break(cycles.value + (spbrg->value + 1) * spbrg_edge, this);

}
void _RCSTA::receive_start_bit(void)
{

  //cout << "USART received a start bit\n";

  if((value & (CREN | SREN)) == 0) {
    cout << "  but not enabled\n";
    return;
  }
  
  if(txsta && (txsta->value & _TXSTA::BRGH))
    set_callback_break(BRGH_FIRST_MID_SAMPLE);
  else
    set_callback_break(BRGL_FIRST_MID_SAMPLE);

  sample = 0;
  sample_state = RCSTA_WAITING_MID1;
  state = RCSTA_MAYBE_START;
}

void _RCSTA::callback(void)
{

  //cout << "RCSTA callback " << (cycles.value) << '\n';

  switch(sample_state) {
  case RCSTA_WAITING_MID1:
    if(uart_port->get_bit(rx_bit))
      sample++;

    if(txsta && (txsta->value & _TXSTA::BRGH))
      set_callback_break(BRGH_SECOND_MID_SAMPLE - BRGH_FIRST_MID_SAMPLE);
    else
      set_callback_break(BRGL_SECOND_MID_SAMPLE - BRGL_FIRST_MID_SAMPLE);

    sample_state = RCSTA_WAITING_MID2;

    break;

  case RCSTA_WAITING_MID2:
    if(uart_port->get_bit(rx_bit))
      sample++;

    if(txsta && (txsta->value & _TXSTA::BRGH))
      set_callback_break(BRGH_THIRD_MID_SAMPLE - BRGH_SECOND_MID_SAMPLE);
    else
      set_callback_break(BRGL_THIRD_MID_SAMPLE - BRGL_SECOND_MID_SAMPLE);

    sample_state = RCSTA_WAITING_MID3;

    break;

  case RCSTA_WAITING_MID3:
    if(uart_port && uart_port->get_bit(rx_bit))
      sample++;

    receive_a_bit( (sample>=2));
    sample = 0;

    // If this wasn't the last bit then go ahead and set a break for the next bit.
    if(state==RCSTA_RECEIVING) {
      if(txsta && (txsta->value & _TXSTA::BRGH))
	set_callback_break(TOTAL_BRGH_STATES -(BRGH_THIRD_MID_SAMPLE - BRGH_FIRST_MID_SAMPLE));
      else
	set_callback_break(TOTAL_BRGL_STATES -(BRGL_THIRD_MID_SAMPLE - BRGL_FIRST_MID_SAMPLE));

      sample_state = RCSTA_WAITING_MID1;
    }

    break;

  default:
    //cout << "Error RCSTA callback with bad state\n";
    // The receiver was probably disabled in the middle of a reception.
    ;
  }

}

//-----------------------------------------------------------
// RCREG
//
void _RCREG::push(unsigned int new_value)
{
  if(fifo_sp >= 2)
    {
      rcsta->value |= _RCSTA::OERR;
      if(verbose)
	cout << "receive overrun\n";
    }
  else
    {
      //cout << "pushing uart reception onto rcreg stack\n";
      fifo_sp++;
      oldest_value = value;
      value = new_value;
      trace.register_write(address,value);
    }

}

void _RCREG::pop(void)
{

  if(fifo_sp == 0)
    return;

  if(--fifo_sp == 1)
    value = oldest_value;

}

unsigned int _RCREG::get_value(void)
{

  return value;

}

unsigned int _RCREG::get(void)
{

  pop();
  trace.register_read(address,value);
  return value;
}
void _RCREG::assign_pir_set(PIR_SET *new_pir_set)
{
  cout <<"_RCREG:: function not impl\n";
}


//-----------------------------------------------------------
// SPBRG - Serial Port Baud Rate Generator
//
// The SPBRG is essentially a continuously running programmable
// clock. (Note that this will slow the simulation down if the
// serial port is not used. Perhaps gpsim needs some kind of
// pragma type thing to disable cpu intensive peripherals...)

void _SPBRG::get_next_cycle_break(void)
{

  if(txsta && (txsta->value & _TXSTA::SYNC))
    {
      // Synchronous mode
      future_cycle = last_cycle + (value + 1)*4;
    }
  else
    {
      // Asynchronous mode
      if(txsta && (txsta->value & _TXSTA::BRGH))
	future_cycle = last_cycle + (value + 1)*16;
      else
	future_cycle = last_cycle + (value + 1)*64;
    }

  if(cpu)
    cycles.set_break(future_cycle, this);

  //cout << "SPBRG::callback next break at 0x" << hex << future_cycle <<'\n';
  
}

void _SPBRG::start(void)
{

  if(verbose)
    cout << "SPBRG::start\n";

  if(cpu)
    last_cycle = cycles.value;
  start_cycle = last_cycle;

  get_next_cycle_break();

  if(verbose)
    cout << " SPBRG::start   last_cycle = " << 
      hex << last_cycle << " future_cycle = " << future_cycle << '\n';

  if(verbose && cpu)
    cycles.dump_breakpoints();

}

//--------------------------
//guint64 _SPBRG::get_last_cycle(void)
//
// Get the cpu cycle corresponding to the last edge of the SPBRG
//

guint64 _SPBRG::get_last_cycle(void)
{

  // There's a chance that a SPBRG break point exists on the current
  // cpu cycle, but has not yet been serviced.
  if(cpu)
    return( (cycles.value == future_cycle) ? future_cycle : last_cycle);
  else
    return 0;
}
//--------------------------
//guint64 _SPBRG::get_cpu_cycle(unsigned int edges_from_now)
//
//  When the SPBRG is enabled, it becomes a free running counter
// that's synchronous with the cpu clock. The frequency of the
// counter depends on the mode of the usart:
//
//  Synchronous mode:
//    baud = cpu frequency / 4 / (spbrg.value + 1)
//
//  Asynchronous mode:
//   high frequency:
//     baud = cpu frequency / 16 / (spbrg.value + 1)
//   low frequency:
//     baud = cpu frequency / 64 / (spbrg.value + 1)
//
// What this routine will do is return the cpu cycle corresponding
// to a (rising) edge of the spbrg clock. 

guint64 _SPBRG::get_cpu_cycle(unsigned int edges_from_now)
{

  // There's a chance that a SPBRG break point exists on the current
  // cpu cycle, but has not yet been serviced. 
  guint64 cycle = (cycles.value == future_cycle) ? future_cycle : last_cycle;

  if(txsta && (txsta->value & _TXSTA::SYNC))
    {
      // Synchronous mode
      return ( edges_from_now * (value + 1)*4 + cycle);
    }
  else
    {
      // Asynchronous mode
      if(txsta && (txsta->value & _TXSTA::BRGH))
	return ( edges_from_now * (value + 1)*16 + cycle);
      else
	return ( edges_from_now * (value + 1)*64 + cycle);
    }


#if 0

const guint64 SPBRG_SYNC_MASK = ~((guint64) 3);
const guint64 SPBRG_ASYNC_LO  = ~((guint64) 63);
const guint64 SPBRG_ASYNC_HI = ~((guint64) 15);
  
  guint64 cycle;

  // Get the number of cycles the sprbg has been active
  cycle = (cycles.value - start_cycle);

  if(txsta->value & _TXSTA::SYNC)
    {
      // Synchronous mode
      cycle = (cycle & SPBRG_SYNC_MASK) + edges_from_now * (value + 1)*4 + start_cycle;
    }
  else
    {
      // Asynchronous mode
      if(txsta->value & _TXSTA::BRGH)
	cycle = (cycle & SPBRG_ASYNC_HI) + edges_from_now * (value + 1)*16 + start_cycle;
      else
	cycle = (cycle & SPBRG_ASYNC_LO) + edges_from_now * (value + 1)*64 + start_cycle;
    }

  return cycle;
#endif
}
void _SPBRG::callback(void)
{

  last_cycle = cycles.value;

  //cout << "SPBRG rollover at cycle " << last_cycle << '\n';

  if(rcsta && (rcsta->value & _RCSTA::SPEN))
    {

      // If the serial port is enabled, then set another 
      // break point for the next clock edge.
      get_next_cycle_break();

    }
}
//--------------------------------------------------

bool TXREG_14::is_empty(void)
{
  return(pir_set->get_txif());
}

void TXREG_14::empty(void)
{
  pir_set->set_txif();
}

void TXREG_14::full(void)
{
  pir_set->clear_txif();
}

void RCREG_14::push(unsigned int new_value)
{

  _RCREG::push(new_value);

  pir_set->set_rcif();

}

void RCREG_14::pop(void)
{

  _RCREG::pop();
  if(fifo_sp == 0)
    pir_set->clear_rcif();

}


//--------------------------------------------------
// member functions for the USART
//--------------------------------------------------
void USART_MODULE::initialize(IOPORT *uart_port, int rx_pin)
{

  if(spbrg) {
    spbrg->txsta = txsta;
    spbrg->rcsta = rcsta;
  }

  if(txreg) {
    txreg->assign_pir_set(0);
    txreg->txsta = txsta;
  }

  if(txsta) {
    txsta->txreg = txreg;
    txsta->spbrg = spbrg;
    txsta->txpin = 0; //uart_port->pins[6];
    txsta->bit_count = 0;
  }

  if(rcsta) {
    rcsta->rcreg = rcreg;
    rcsta->spbrg = spbrg;
    rcsta->txsta = txsta;
    rcsta->uart_port = uart_port;
    rcsta->rx_bit = rx_pin;
  }

  if(rcreg) {
    rcreg->assign_pir_set(0);
    rcreg->rcsta = rcsta;
  }

}

//--------------------------------------------------
void   USART_MODULE::new_rx_edge(unsigned int bit)
{
  //cout << "USART_MODULE::new_rx_edge - shouldn't get called!\n";

}
//--------------------------------------------------
USART_MODULE::USART_MODULE(void)
{
  //cout << "usart module constructor\n";
  txreg = 0;
  rcreg = 0;
  spbrg = 0;

  rcsta = new _RCSTA;
  txsta = new _TXSTA;

}

//--------------------------------------------------
USART_MODULE14::USART_MODULE14(void)
{

  txreg = new TXREG_14;
  rcreg = new RCREG_14;

  spbrg = new _SPBRG;

}

//--------------------------------------------------
void USART_MODULE14::initialize_14(_14bit_processor *new_cpu, PIR_SET *ps,
    IOPORT *uart_port, int rx_pin)
{
  _cpu14 = new_cpu;

  USART_MODULE::initialize(uart_port, rx_pin);

  //spbrg.txsta = &txsta;
  //spbrg.rcsta = &rcsta;

  if(txreg) //this should be unnecessary
    txreg->assign_pir_set(ps);

  //txreg.txsta = &txsta;

  //txsta.txreg = &txreg;
  //txsta.spbrg = &spbrg;
  if(txsta)
    txsta->txpin = uart_port->pins[6];
  //txsta.bit_count = 0;

  //rcsta.rcreg = &rcreg;
  //rcsta.spbrg = &spbrg;
  //rcsta.txsta = &txsta;
  //rcsta.uart_port = uart_port;
  //rcsta.rx_bit = 7;

  //rcreg.rcsta = &rcsta;
  if(rcreg) 
    rcreg->assign_pir_set(ps);

  //  spbrg.start();

}

// This gets called whenever there's a change detected on the RX pin.
// The usart is only interested in those changes when it is waiting
// for the start bit. Otherwise, the rcsta callback function will sample
// the rx pin (if we're receiving).

void   USART_MODULE14::new_rx_edge(unsigned int bit)
{
  if(rcsta)
    if( (rcsta->state == _RCSTA::RCSTA_WAITING_FOR_START) && !bit)
      rcsta->receive_start_bit();
}
