/*
   Copyright (C) 1998-2003 Scott Dattalo
                 2003 Mike Durian

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

#include <glib.h>	// for guint64
#include <assert.h>

#include "intcon.h"


#include "gpsim_classes.h"	// for RESET_TYPE
#include "trace.h"
#include "pir.h"
#include "16bit-registers.h"
#include "16bit-processors.h"
#include "breakpoints.h"

//--------------------------------------------------
// member functions for the INTCON base class
//--------------------------------------------------
INTCON::INTCON(void)
{
  break_point = 0;
  new_name("intcon");
}

void INTCON::set_T0IF(void)
{

  value |= T0IF;

  trace.register_write(address,value);

  if (value & (GIE | T0IE))
  {
    trace.interrupt();
  }
}

void INTCON::put(unsigned int new_value)
{

  value = new_value;
  trace.register_write(address,value);

  // Now let's see if there's a pending interrupt
  // The INTCON bits are:
  // GIE | ---- | TOIE | INTE | RBIE | TOIF | INTF | RBIF
  // There are 3 sources for interrupts, TMR0, RB0/INTF
  // and RBIF (RB7:RB4 change). If the corresponding interrupt
  // flag is set AND the corresponding interrupt enable bit
  // is set AND global interrupts (GIE) are enabled, THEN
  // there's an interrupt pending.
  // note: bit6 is not handled here because it is processor
  // dependent (e.g. EEIE for x84 and ADIE for x7x).

  if(value & GIE)
    {

      if( (((value>>3)&value) & (T0IF | INTF | RBIF)) )
	{
	  trace.interrupt();
	  bp.set_interrupt();
	}
      else if(value & XXIE)
	{
	  if(check_peripheral_interrupt())
	    {
	      trace.interrupt();
	      bp.set_interrupt();
	    }
	}
    }
}

void INTCON::peripheral_interrupt(void)
{
  if(  (value & (GIE | XXIE)) == (GIE | XXIE) )
    bp.set_interrupt();
}


bool INTCON_14_PIR::check_peripheral_interrupt(void)
{
  assert(pir_set != 0);

  return (pir_set->interrupt_status());
}


//----------------------------------------------------------------------
// INTCON_16 
//
// intcon for the 16-bit processor cores.
//
//----------------------------------------------------------------------
INTCON_16::INTCON_16()
{

  rcon = 0;
  intcon2 = 0;
  cpu = 0;
}



//----------------------------------------------------------------------
// void INTCON_16::clear_gies(void)
//
//  This routine clears the global interrupt enable bit(s). If priority
// interrupts are used (IPEN in RCON is set) then the appropriate gie
// bit (either giel or gieh) is cleared.
//
// This routine is called from 16bit_processor::interrupt().
//
//----------------------------------------------------------------------

void INTCON_16::clear_gies(void)
{

  assert(cpu != 0);

  if(cpu16->interrupt_vector == INTERRUPT_VECTOR_HI)
    put(get() & ~GIEH);
  else
    put(get() & ~GIEL);


}


//----------------------------------------------------------------------
// void INTCON_16::clear_gies(void)
//
//----------------------------------------------------------------------

void INTCON_16::set_gies(void)
{

  assert(rcon != 0);
  assert(intcon2 != 0);
  assert(cpu != 0);

  get();   // Update the current value of intcon
           // (and emit 'register read' trace).

  if(rcon->value & RCON::IPEN)
    {
      // Interrupt priorities are being used.

      if(0 == (value & GIEH))
	{
	  // GIEH is cleared, so we need to set it

	  put(value | GIEH);
	  return;

	}
      else
	{
	  // GIEH is set. This means high priority interrupts are enabled.
	  // So we most probably got here because of an RETFIE instruction
	  // after handling a low priority interrupt. We could check to see
	  // if GIEL is low before calling put(), but it's not necessary.
	  // So we'll just blindly re-enable giel, and continue with the
	  // simulation.

	  put(value | GIEL);
	  return;

	}
    }
  else
    {

      // Interrupt priorities are not used, so re-enable GIEH (which is in
      // the same bit-position as GIE on the mid-range core).

      put(value | GIEH);
      return;

    }
}


//----------------------------------------------------------------------
// void INTCON_16::put(unsigned int new_value)
//
//  Here's were the 18cxxx interrupt logic is primarily handled. 
//
// inputs: new_value - 
// outputs: none
//
//----------------------------------------------------------------------
extern void DEBUG_print_interrupt_vector(Processor *cpu);

void INTCON_16::put(unsigned int new_value)
{

  value = new_value;
  trace.register_write(address,value);
  //cout << " INTCON_16::put\n";
  // Now let's see if there's a pending interrupt
  // if IPEN is set in RCON, then interrupt priorities
  // are being used. (In other words, there are two
  // interrupt priorities on the 18cxxx core. If a
  // low priority interrupt is being serviced, it's
  // possible for a high priority interrupt to interject.

  if(rcon->value & RCON::IPEN)
    {
      unsigned int i1;

      // Use interrupt priorities

      if( 0 == (value & GIEH))
	return;    // Interrupts are disabled

      // Now we just go through the interrupt logic of the 18cxxx
      // First we check the high priorities and then we check the
      // low ones. When ever we detect an interrupt, then the 
      // bp.interrupt flag is set (which will cause the interrupt
      // to be handled at the high level) and additional checks
      // are aborted.

      // If TO, INT, or RB flags are set AND their correspond
      // interrupts are enabled, then the lower three bits of
      // i1 will reflect this. Note that INTF does NOT have an
      // associated priority bit!

      i1 =  ( (value>>3)&value) & (T0IF | INTF | RBIF);

      if(i1 & ( (intcon2->value & (T0IF | RBIF)) | INTF))
	{
	  //cout << " selecting high priority vector\n";
	  cpu16->interrupt_vector = INTERRUPT_VECTOR_HI;
	  trace.interrupt();
	  bp.set_interrupt();
	  return;
	}


      // If we reach here, then there are no high priority
      // interrupts pending. So let's check for the low priority
      // ones.

      if(i1 & ( (~intcon2->value & (T0IF | RBIF)) | INTF))
	{
	  //cout << " selecting low priority vector\n";
	  cpu16->interrupt_vector = INTERRUPT_VECTOR_LO;
	  trace.interrupt();
	  bp.set_interrupt();
	  return;
	}


    }
  else
    {
      // ignore interrupt priorities

      cout << " ignoring interrupt priorities, selecting high priority vector:" << INTERRUPT_VECTOR_HI << endl;
  DEBUG_print_interrupt_vector(cpu);


      cpu16->interrupt_vector = INTERRUPT_VECTOR_HI;
  DEBUG_print_interrupt_vector(cpu);
      if(value & GIE) 
	{
	  if( ( (value>>3)&value) & (T0IF | INTF | RBIF) )
	    {
	      trace.interrupt();
	      bp.set_interrupt();
	    }
	  else if(value & XXIE)
	    {
	      if(check_peripheral_interrupt())
		{
		  trace.interrupt();
		  bp.set_interrupt();
		}
	    }

	}

    }
}
