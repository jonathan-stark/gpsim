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
#include <iostream.h>

#include "uart.h"
#include "14bit-processors.h"
#include "14bit-tmrs.h"

//-----------------------------------------------------------
// TXREG - USART Transmit Register


void _TXREG::put_value(unsigned int new_value)
{

  value = new_value & 0xff;

  if(verbose)
    cout << "txreg just got a new value\n";

  if(txsta->value & _TXSTA::TRMT)
    {
      // If the transmit buffer is empty, 
      // then transmit this new data now...

      txsta->start_transmitting();
    }
  else
    {
      // The transmit buffer is busy transmitting,
      // So clear the TXIF flag and wait.
      full();
    }
}

void _TXREG::put(unsigned int new_value)
{

  put_value(new_value);

  trace.register_write(address,value);

}

//-----------------------------------------------------------
// TXSTA - Transmit Register Status and Control

void _TXSTA::put_value(unsigned int new_value)
{

  value = (new_value & ~TRMT) | ( (bit_count) ? 0 : TRMT);

}

void _TXSTA::put(unsigned int new_value)
{

  put_value(new_value);

  trace.register_write(address,value);

}

void _TXSTA::start_transmitting(void)
{
  cout << "starting a USART transmission\n";

  // Build the serial byte that's about to be transmitted.
  // I doubt the pic really does this way, but gpsim builds
  // the entire bit stream including the start bit, 8 data 
  // bits, optional 9th data bit and the stop, and places
  // this into the tsr register. But since the contents of
  // the tsr are inaccessible, I guess we'll never know.


  // The start bit, which is always low, occupies bit position
  // zero. The next 8 bits come from the txreg.

  tsr = txreg->value << 1;

  // Is this a 9-bit data transmission?
  if(value & TX9)
    {
      // Copy the stop bit and the 9th data bit to the tsr.
      tsr |= ( (value & TX9D) ? (3<<9) : (2<<9));
      bit_count = 11;
    }
  else
    {
      // The stop bit is always high.
      tsr |= (1<<9);
      bit_count = 10;
    }

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

      cout << "Transmit bit #" << bit_count << ": " << (tsr&1) << '\n';
      txpin->put_state(tsr&1);

      tsr >>= 1;

      if(--bit_count == 0)
	{
	  // tsr is empty.
	  // If there is any more data in the TXREG, then move it to
	  // the tsr and continue transmitting other wise set the TRMT bit

	  if(txreg->is_empty())
	    value |= TRMT;
	  else
	    start_transmitting();

	}
    }

}

//-----------------------------------------------------------
// Receiver portion of the USART
//
// First RCSTA -- Receiver Control and Status 
//


void _RCSTA::put_value(unsigned int new_value)
{
  unsigned int diff;

  diff = new_value ^ value;
  value = ( value & (RX9D | OERR | FERR) )   |  (new_value & ~(RX9D | OERR | FERR));

  if(diff & SPEN)
    spbrg->start();

  if(value & SPEN)
    {
      // If the serial port is enabled, check to see if a new rx is about to happen.
      if(txsta->value & _TXSTA::SYNC)
	{
	  cout << "not doing syncronous receptions yet\n";
	}
      else
	{
	  if(diff & CREN)
	    start_receiving();
	}
    }
  
}

void _RCSTA::put(unsigned int new_value)
{

  put_value(new_value);

  trace.register_write(address,value);

}

void _RCSTA::receive_a_bit(void)
{

  if(bit_count)
    {

      //new_bit >>= 1;  // TEST!!!!

      if(uart_port->get_bit(rx_bit))
	rsr |= 1<<9;

      cout << "Receive bit #" << bit_count << ": " << (rsr&(1<<9)) << '\n';

      rsr >>= 1;

      if(--bit_count == 0)
	{
	  // rsr is full.

	  // If the rxreg has data from a previous reception then
	  // we have a receiver overrun error.
	  cout << "rcsta.rsr is full\n";

	  if((value & RX9) == 0)
	    rsr >> 0;

	  rcreg->push( rsr & 0xff);

	  if(value & CREN)
	    start_receiving();

	}
    }

}

void _RCSTA::start_receiving(void)
{
  cout << "The USART is starting to receive data\n";

  //  new_bit = (3<<9) | (0x5a << 1); // test

  rsr = 0;

  // Is this a 9-bit data transmission?
  if(value & RX9)
    {
      bit_count = 10;
    }
  else
    {
      bit_count = 9;
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
      cout << "receive overrun\n";
    }
  else
    {
      cout << "pushing uart reception onto rcreg stack\n";
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


//-----------------------------------------------------------
// SPBRG - Serial Port Baud Rate Generator
//
// The SPBRG is essentially a continuously running programmable
// clock. (Note that this will slow the simulation down if the
// serial port is not used. Perhaps gpsim needs some kind of
// pragma type thing to disable cpu intensive peripherals...)

void _SPBRG::get_next_cycle_break(void)
{

  if(txsta->value & _TXSTA::SYNC)
    {
      // Synchronous mode
      future_cycle = last_cycle + (value + 1)*4;
    }
  else
    {
      // Asynchronous mode
      if(txsta->value & _TXSTA::BRGH)
	future_cycle = last_cycle + (value + 1)*16;
      else
	future_cycle = last_cycle + (value + 1)*64;
    }
}

void _SPBRG::start(void)
{

  if(verbose)
    cout << "SPBRG::start\n";

  last_cycle = cpu->cycles.value;

  get_next_cycle_break();

  if(verbose)
    cout << " SPBRG::start   last_cycle = " << 
      hex << last_cycle << " future_cycle = " << future_cycle << '\n';

  cpu->cycles.set_break(future_cycle, this);

  if(verbose)
    cpu->cycles.dump_breakpoints();

}

void _SPBRG::callback(void)
{

  last_cycle = cpu->cycles.value;

  cout << "SPBRG rollover at cycle " << last_cycle << '\n';

  if(rcsta->value & _RCSTA::SPEN)
    {
      if(txsta->value & _TXSTA::TXEN)
	txsta->transmit_a_bit();

      if(rcsta->value & _RCSTA::CREN)
	rcsta->receive_a_bit();

      // If the serial port is enabled, then set another 
      // break point for the next clock edge.

      get_next_cycle_break();

      cpu->cycles.set_break(future_cycle, this);
    }
}
//--------------------------------------------------

bool TXREG_14::is_empty(void)
{
  return(pir1->get_txif());
}

void TXREG_14::empty(void)
{
  pir1->set_txif();
}

void TXREG_14::full(void)
{
  pir1->clear_txif();
}

void RCREG_14::push(unsigned int new_value)
{

  _RCREG::push(new_value);

  pir1->set_rcif();

}


//--------------------------------------------------
// member functions for the USART
//--------------------------------------------------
void USART_MODULE14::initialize(_14bit_processor *new_cpu, PIR1 *pir1, IOPORT *uart_port)
{
  cpu = new_cpu;

  spbrg.txsta = &txsta;
  spbrg.rcsta = &rcsta;

  txreg.pir1 = pir1;
  txreg.txsta = &txsta;

  txsta.txreg = &txreg;
  txsta.txpin = uart_port->pins[6];
  txsta.bit_count = 0;

  rcsta.rcreg = &rcreg;
  rcsta.spbrg = &spbrg;
  rcsta.txsta = &txsta;
  rcsta.uart_port = uart_port;
  rcsta.rx_bit = 7;

  rcreg.rcsta = &rcsta;
  rcreg.pir1 = pir1;

  //  spbrg.start();

}
