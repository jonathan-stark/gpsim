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

#include "breakpoints.h"

//---------------------------------------------------------
// Cycle Counter
//
// The cycle counter class is used to coordinate the timing 
// between the different peripherals within a processor and
// in some cases, the timing between several simulated processors
// and modules.

class Cycle_Counter_breakpoint_list
{
  // This class is used to implement a very simple doubly-linked-list.
  // See Cycle_Counter::set_break for more details on how it is used.

public:
  Cycle_Counter_breakpoint_list *next;
  Cycle_Counter_breakpoint_list *prev;

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

  guint64 value;          // Current value of the cycle counter.
  guint64 break_on_this;  // If there's a pending cycle break point, then it'll be this

  bool reassigned;        // Set true when a break point is reassigned (or deleted)

  Cycle_Counter_breakpoint_list
    active,     // Head of the active breakpoint linked list
    inactive;   // Head of the inactive one.

  bool bSynchronous; // a flag that's true when the time per counter tick is constant

  double cycles_per_second; // The number of cycles that correspond to one second
                            // i.e. this is the frequency.
  double seconds_per_cycle;

  Cycle_Counter(void);
  void preset(guint64 new_value);     // not used currently.

 private:
  /*
    breakpoint
    when the member function "increment()" encounters a break point, 
    breakpoint() is called.
  */

  void breakpoint(void)
    {
      // There's a break point set on this cycle. If there's a callback function, then call
      // it other wise halt execution by setting the global break flag.

      // Loop in case there are multiple breaks
      while(value == break_on_this && active.next) {
	

	reassigned = 0;    // This flag will get set true if the call back
	// function moves the break point to another cycle.

	if(active.next->f)
	  active.next->f->callback();
	else 
	  bp.check_cycle_break(active.next->breakpoint_number);

	if(!reassigned)    // don't try to clear if the break point was reassigned.
	  clear_current_break();
      }
    }

 public:
  /*
    increment - This inline member function is called once or 
    twice for every simulated instruction. Its purpose is to
    increment the cycle counter using roll over arithmetic.
    If there's a breakpoint set on the new value of the cycle
    counter then the simulation is either stopped or a callback
    function is invoked. In either case, the break point is
    cleared.
   */
						
  inline void increment(void)
    {
 
      // Increment the current cycle then check if
      // we have a break point set here

      value++;

      if(value == break_on_this)
	breakpoint();

      // Note that it's really inefficient to trace every cycle increment. 
      // Instead, we implicitly trace the increments with the instruction traces.

    }

  /*
    advance the Cycle Counter by more than one instruction quantum.
    This is almost identical to the increment() function except that
    we allow the counter to be advanced by an arbitrary amount.
    They're separated only for efficiency reasons. This one runs slower.
   */    
  inline void advance(guint64 step)
    {

      value += step;
      
      if(value >= break_on_this)
	{
	  // There's a break point set on this cycle. If there's a callback function, then call
	  // it other wise halt execution by setting the global break flag.

	  while(value >= break_on_this)   // Loop in case there are multiple breaks
	    {
	      if(active.next->f)
		active.next->f->callback();
	      else
		bp.check_cycle_break(active.next->breakpoint_number);

	      clear_current_break();
	    }
	}

    }


  // Return the current cycle counter value
  guint64 get(void) 
  {
    return value;
  }

  // Return the cycle counter for some time off in the future:
  guint64 get(double future_time_from_now);

  bool set_break(guint64 future_cycle,
		 BreakCallBack *f=0, unsigned int abp = MAX_BREAKPOINTS);
  bool set_break_delta(guint64 future_cycle,
		 BreakCallBack *f=0, unsigned int abp = MAX_BREAKPOINTS);
  bool reassign_break(guint64 old_cycle,guint64 future_cycle, BreakCallBack *f=0);
  void clear_current_break(void);
  void dump_breakpoints(void);

  void clear_break(guint64 at_cycle);
  void clear_break(BreakCallBack *f);
  void set_cycles_per_second(guint64 cps);
};

extern Cycle_Counter cycles;



// The stopwatch object is used to keep track of the amount of
// time between events.
class StopWatch 
{
 public:
  bool enabled;
  bool count_dir;

  guint64 rollover;
  guint64 offset;
  guint64 value;

  guint64 get(void);
  double get_time(void);

  void start(void);
  void stop(void);
  void set_rollover(guint64 new_rollover);
  void set_offset(guint64 new_rollover);

  StopWatch(void);

};

extern StopWatch stop_watch;


#endif
