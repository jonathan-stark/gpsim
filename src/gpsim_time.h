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
//
// The smallest quantum of simulated time is called a 'cycle'.
// The simuluation engine increments a 'Cycle Counter' at quantum
// simulation step. Simulation objects that wished to be notified
// at a specific instance in time can set a cycle counter break
// point that will get invoked whenever the cycle counter reaches
// that instance.



//------------------------------------------------------------
//
// Cycle counter breakpoint list
//
// This is a friend class to the Cycle Counter class. Its purpose
// is to maintain a doubly linked list of cycle counter break
// points.

class Cycle_Counter_breakpoint_list
{

public:
  // This is the value compared to the cycle counter.
  guint64 break_value;

  // True when this break is active.
  bool bActive;

  // The breakpoint_number is a number uniquely identifying this
  // cycle counter break point. Note, this number is used only 
  // when the break point was assigned by a user

  unsigned int breakpoint_number;

  // If non-null, the TriggerObject will point to an object that will get invoked
  // when the breakpoint is encountered.

  TriggerObject *f;

  // Doubly-linked list mechanics..
  // (these will be made private eventually)
  Cycle_Counter_breakpoint_list *next;
  Cycle_Counter_breakpoint_list *prev;


  Cycle_Counter_breakpoint_list *getNext();
  Cycle_Counter_breakpoint_list *getPrev();
  void clear();
  void invoke();
  
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

  void breakpoint(void);

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
		get_bp().check_cycle_break(active.next->breakpoint_number);

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
		 TriggerObject *f=0, unsigned int abp = MAX_BREAKPOINTS);
  bool set_break_delta(guint64 future_cycle,
		 TriggerObject *f=0, unsigned int abp = MAX_BREAKPOINTS);
  bool reassign_break(guint64 old_cycle,guint64 future_cycle, TriggerObject *f=0);
  void clear_current_break(void);
  void dump_breakpoints(void);

  void clear_break(guint64 at_cycle);
  void clear_break(TriggerObject *f);
  void set_cycles_per_second(guint64 cps);
};

#ifdef IN_MODULE
// we are in a module: don't access cycles object directly!
Cycle_Counter &get_cycles(void);
#else
// we are in gpsim: use of get_cycles() is recommended,
// even if cycles object can be accessed directly.
extern Cycle_Counter cycles;

inline Cycle_Counter &get_cycles(void)
{
  return cycles;
}
#endif



/// The stopwatch object is used to keep track of the amount of
/// time between events. It can be controlled either through the
/// class API or through its attributes
class StopWatch : public TriggerObject
{
 public:

  StopWatch(void);

  guint64 get(void);
  double get_time(void);

  void set_enable(bool);
  void set_direction(bool);
  void set_rollover(guint64);
  void set_value(guint64);

  void set_break(bool);

  void update();

  virtual void callback(void);
  virtual void callback_print(void);

private:
  Integer *value;
  Integer *rollover;
  Boolean *enable;
  Boolean *direction;

  bool count_dir;

  guint64 offset;
  guint64 break_cycle;

};

extern StopWatch stop_watch;


#endif
