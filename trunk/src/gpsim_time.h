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

#ifndef __GPSIM_TIME_H__
#define __GPSIM_TIME_H__

//class invalid_file_register;   // Forward reference
//class _14bit_processor;
//class pic_processor;


//#include "trace.h"

//#include "breakpoints.h"



//---------------------------------------------------------
// Cycle Counter
//
// This not really a real PIC register. The cycle counter class
// is used to coordinate the timing between the different peripherals
// within a PIC and in some cases, the timing between several simulated
// PICs.

class Cycle_Counter_breakpoint_list
{
  // This class is used to implement a very simple single-linked-list.
  // See Cycle_Counter::set_break for more details on how it is used.

public:
  Cycle_Counter_breakpoint_list *next;
  //  unsigned int break_value_lo;
  //  unsigned int break_value_hi;
  guint64 break_value;
  unsigned int breakpoint_number;  // sytem bp (this is used if the user set a bp.
				   // It tells which system bp needs to be cleared
                                   // when the cycle break is hit. If a peripheral set
                                   // a cycle break, then there is no corresponding 
                                   // system break.)
  BreakCallBack *f;
};

class Cycle_Counter
{
public:

#define BREAK_ARRAY_SIZE  32
#define BREAK_ARRAY_MASK  (BREAK_ARRAY_SIZE -1)

  //  unsigned int value_lo;           // Current value of the cycle counter.
  //  unsigned int value_hi;           // High int of the Current value
  //  unsigned int break_on_this_lo;   // Break if the cycle counter reaches this value.
  //  unsigned int break_on_this_hi;   // Break if the cycle counter reaches this value.

  guint64 value;
  guint64 break_on_this;

  Cycle_Counter_breakpoint_list
    active,     // Head of the active breakpoint linked list
    inactive;   // Head of the inactive one.

  Cycle_Counter(void);
  void preset(guint64 new_value);     // not used currently.

  /*
    increment - This inline member function is called once or twice for every simulated
    instruction. Its purpose is to increment the cycle counter using roll over arithmetic.
    If there's a breakpoint set on the new value of the cycle counter then the simulation
    is either stopped or a callback function is invoked. In either case, the break point is
    cleared.
   */
						
  inline void increment(void)
    {
 
      // Increment the current cycle
      // we have a break point set here
      //if(++value.lo == 0)
      //++value.hi;
      value++;

      //if((value.lo == break_on_this.lo) && (value.hi == break_on_this.hi))
      if(value == break_on_this)
	{
	  // There's a break point set on this cycle. If there's a callback function, then call
	  // it other wise halt execution by setting the global break flag.

	  while(value == break_on_this)   // Loop in case there are multiple breaks
	    {
	      if(active.next->f != NULL)
		active.next->f->callback();
	      else
		bp.check_cycle_break(active.next->breakpoint_number);

	      clear_current_break();
	    }
	}

      trace.cycle_counter(value);
    }



  bool set_break(guint64 future_cycle,
		 BreakCallBack *f=NULL, unsigned int abp = MAX_BREAKPOINTS);
  bool set_break_delta(guint64 future_cycle,
		 BreakCallBack *f=NULL, unsigned int abp = MAX_BREAKPOINTS);
  bool reassign_break(guint64 old_cycle,guint64 future_cycle, BreakCallBack *f=NULL);
  void clear_current_break(void);
  void dump_breakpoints(void);

};


#endif
