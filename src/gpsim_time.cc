/*
   Copyright (C) 1998-2000 T. Scott Dattalo

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
#include <iomanip.h>

#include "14bit-processors.h"
#include "interface.h"

#include <string>
#include "stimuli.h"

#include "../config.h"
#include "xref.h"
//#include "gpsim_time.h"

//#define __DEBUG_CYCLE_COUNTER__

// Largest cycle counter value

#define END_OF_TIME 0xFFFFFFFFFFFFFFFF

//--------------------------------------------------
// member functions for the Cycle_Counter class
//--------------------------------------------------

//--------------------------------------------------

void Cycle_Counter::preset(guint64 new_value)
{
  value = new_value;
  trace.cycle_counter(value);
}

//--------------------------------------------------
// set_break
// set a cycle counter break point. Return 1 if successful.
//
//  The break points are stored in a singly-linked-list sorted by the
// order in which they will occur. When this routine is called, the
// value of 'future_cycle' is compared against the values in the
// 'active' list.

bool Cycle_Counter::set_break(guint64 future_cycle, BreakCallBack *f=NULL, unsigned int bpn=MAX_BREAKPOINTS)
{

  Cycle_Counter_breakpoint_list  *l1 = &active, *l2;


#ifdef __DEBUG_CYCLE_COUNTER__
  cout << "Cycle_Counter::set_break  cycle = 0x" << hex<<future_cycle;

    if(f)
      cout << " has callback\n";
    else
      cout << " does not have callback\n";
#endif


  if(inactive.next == NULL)
    {
      cout << " too many breaks are set on the cycle counter \n";
      return 0;
    }
  else if(future_cycle <= value)
    {
      cout << "Cycle break point was ignored because cycle " << future_cycle << " has already gone by\n";
      cout << "current cycle is " << value << '\n';
      return 0;
    }
  else
    {
      // place the future cycle at which we intend to break into the
      // sorted break list

      bool break_set = 0;

      while( (l1->next) && !break_set)
	{

	  // If the next break point is at a cycle greater than the
	  // one we wish to set, then we found the insertion point.
	  // Otherwise 
	  if(l1->next->break_value >= future_cycle)
	    break_set = 1;
	  else
	    l1 = l1->next;

	}

      l2 = l1->next;
      l1->next = inactive.next;
      inactive.next = l1->next->next;
      l1->next->next = l2;
      l1->next->break_value = future_cycle;
      l1->next->f = f;
      l1->next->breakpoint_number = bpn;

#ifdef __DEBUG_CYCLE_COUNTER__
      cout << "cycle break " << future_cycle << " bpn " << bpn << '\n';
#endif

    }

  break_on_this = active.next->break_value;

  return 1;
}

//--------------------------------------------------
// set_break_delta
// set a cycle counter break point relative to the current cpu cycle value. Return 1 if successful.
//

bool Cycle_Counter::set_break_delta(guint64 delta, BreakCallBack *f=NULL, unsigned int bpn=MAX_BREAKPOINTS)
{

#ifdef __DEBUG_CYCLE_COUNTER__
  cout << "Cycle_Counter::set_break_delta  delta = 0x" << hex<<delta;

  if(f)
    cout << " has callback\n";
  else
    cout << " does not have callback\n";
#endif

  return set_break(value+delta,f,bpn);

  

}
// reassign_break
//   change the cycle of an existing break point.
//
//  This is only called by the internal peripherals and not (directly) by the user. It's
// purpose is to accommodate the dynamic and unpredictable needs of the internal cpu timing.
// For example, if tmr0 is set to roll over on a certain cycle and the program changes the
// pre-scale value, then the break point has to be moved to the new cycle.

bool Cycle_Counter::reassign_break(guint64 old_cycle, guint64 new_cycle, BreakCallBack *f=NULL)
{

  Cycle_Counter_breakpoint_list  *l1 = &active, *l2;

  bool found_old = 0;
  bool break_set = 0;

#ifdef __DEBUG_CYCLE_COUNTER__
  cout << "Cycle_Counter::reassign_break, old " << old_cycle << " new " << new_cycle << '\n';
  dump_breakpoints();
#endif

  while( (l1->next) && !found_old)
    {
      
      // If the next break point is at a cycle greater than the
      // one we wish to set, then we found the insertion point.
      // Otherwise 
	  if(l1->next->break_value ==  old_cycle)
	    {
#ifdef __DEBUG_CYCLE_COUNTER__
	      cout << " cycle match ";
#endif
	      if(l1->next->f == f)
		found_old = 1;
	      else
		l1 = l1->next;
	    }
	  else
	    l1 = l1->next;

    }

  if(found_old)
    {
      // Now move the break point
#ifdef __DEBUG_CYCLE_COUNTER__
      cout << " found old ";
#endif

      if(new_cycle > old_cycle)
	{
	  // First check to see if we can stay in the same relative position within the list

	  // Is this the last one in the list? (or equivalently, is the one after this one a NULL)
	  if(l1->next->next == NULL)
	    {

	      l1->next->break_value = new_cycle;
	      break_on_this = active.next->break_value;
#ifdef __DEBUG_CYCLE_COUNTER__
	      cout << " replaced at current position (next is NULL)\n";
	      dump_breakpoints();   // debug
#endif
	      return 1;
	    }

	  // Is the next one in the list still beyond this one?
	  if(l1->next->next->break_value >= new_cycle)
	    {
	      l1->next->break_value = new_cycle;
	      break_on_this = active.next->break_value;
#ifdef __DEBUG_CYCLE_COUNTER__
	      cout << " replaced at current position (next is greater)\n";
	      dump_breakpoints();   // debug
#endif
	      return 1;
	    }

	  // Darn. Looks like we have to move it.

#ifdef __DEBUG_CYCLE_COUNTER__
	  cout << " moving \n";
#endif

	  l2 = l1->next;                        // l2 now points to this break point

	  l1->next = l1->next->next;            // Unlink this break point

	  while( (l1->next) && !break_set)
	    {

	      // If the next break point is at a cycle greater than the
	      // one we wish to set, then we found the insertion point.
	      // Otherwise 
	      if(l1->next->break_value > new_cycle)
		break_set = 1;
	      else
		l1 = l1->next;

	    }

	  l2->next = l1->next;
	  l1->next = l2;

	  break_on_this = active.next->break_value;

	  l2->break_value = new_cycle;

#ifdef __DEBUG_CYCLE_COUNTER__
	  dump_breakpoints();   // debug
#endif
	}
      else      // old_cycle < new_cycle
	{
	  // First check to see if we can stay in the same relative position within the list

#ifdef __DEBUG_CYCLE_COUNTER__
	  cout << " old cycle is less than new one\n";
#endif
	  // Is this the first one in the list?
	  if(l1 == &active)
	    {
	      l1->next->break_value = new_cycle;
	      break_on_this = new_cycle;
#ifdef __DEBUG_CYCLE_COUNTER__
	      cout << " replaced at current position\n";
	      dump_breakpoints();   // debug
#endif
	      return 1;
	    }

	  // Is the previous one in the list still before this one?
	  if(l1->break_value < new_cycle)
	    {
	      l1->next->break_value = new_cycle;
#ifdef __DEBUG_CYCLE_COUNTER__
	      cout << " replaced at current position\n";
	      dump_breakpoints();   // debug
#endif
	      return 1;
	    }

	  // Darn. Looks like we have to move it.

	  l2 = l1->next;                        // l2 now points to this break point

	  l1->next = l1->next->next;            // Unlink this break point

	  l1 = &active;                         // Start searching from the beginning of the list

	  while( (l1->next) && !break_set)
	    {

	      // If the next break point is at a cycle greater than the
	      // one we wish to set, then we found the insertion point.
	      // Otherwise 
	      if(l1->next->break_value > new_cycle)
		break_set = 1;
	      else
		l1 = l1->next;

	    }

	  l2->next = l1->next;
	  l1->next = l2;

	  l2->break_value = new_cycle;

	  break_on_this = active.next->break_value;

#ifdef __DEBUG_CYCLE_COUNTER__
	  dump_breakpoints();   // debug
#endif
	}
    }
  else {
    // If the break point was not found, it can't be moved. So let's just create
    // a new break point.
    cout << " Warning: Cycle_Counter::reassign_break - didn't find the old one\n";
    set_break(new_cycle, f);
  }
}

void Cycle_Counter::clear_current_break(void)
{

  if(active.next == NULL)
    return;

  if(value == break_on_this)
    {
#ifdef __DEBUG_CYCLE_COUNTER__
      cout << "clearing current cycle break " << hex << setw(16) << setfill('0') << break_on_this <<'\n';
      if(active.next->next)
	cout << "  but there's one pending at the same cycle\n";
#endif
      Cycle_Counter_breakpoint_list  *l1;

      l1 = inactive.next;                  // ptr to 1st inactive bp
      inactive.next = active.next;         // let the 1st active bp become the 1st inactive one
      active.next = active.next->next;     // The 2nd active bp is now the 1st
      inactive.next->next = l1;            // The 2nd inactive bp used to be the 1st

      if(active.next != NULL)
	break_on_this = active.next->break_value;
      else
	break_on_this = END_OF_TIME;
    }
  else
    {
      // If 'value' doesn't equal 'break_on_this' then what's most probably
      // happened is that the breakpoint associated with 'break_on_this'
      // has invoked a callback function that then did a ::reassign_break().
      // There's a slight chance that we have a bug - but very slight...
      if(verbose & 4) {
	cout << "debug::Didn't clear the current cycle break because != break_on_this\n";
	cout << "value = " << value << "\nbreak_on_this = " << break_on_this <<'\n';
      }
    }
}

void Cycle_Counter::dump_breakpoints(void)
{
  Cycle_Counter_breakpoint_list  *l1 = &active;

  cout << "Next scheduled cycle break " << hex << setw(16) << setfill('0') << break_on_this << '\n';

  while(l1->next)
    {
      //cout << cpu->name_str << "  " << "internal cycle break  " <<
      cout << "internal cycle break  " <<
	hex << setw(16) << setfill('0') <<  l1->next->break_value;

      if(l1->next->f)
	cout << " has callback\n";
      else
	cout << " does not have callback\n";

      l1 = l1->next;
    }

}


Cycle_Counter::Cycle_Counter(void)
{
  value         = 0;
  break_on_this = END_OF_TIME;
  time_step     = 1;

  active.next   = NULL;
  inactive.next = NULL;

  Cycle_Counter_breakpoint_list  *l1 = &inactive;

  for(int i=0; i<BREAK_ARRAY_SIZE; i++)
    {
      l1->next = new Cycle_Counter_breakpoint_list;
      l1 = l1->next;
    }
  l1->next = NULL;


}
