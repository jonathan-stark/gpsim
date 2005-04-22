/*
   Copyright (C) 1998-2003 T. Scott Dattalo

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

#if !defined(__BITLOG_H__)
#define __BITLOG_H__

class Cycle_Counter;


/**********************************************************************
 * boolean event logging
 *
 * The boolean event logger is a simple class for logging the time
 * of boolean (i.e. 0/1) events.
 *
 * The event buffer is an array of 64-bit wide elements. Each element
 * stores the time at which an event occurs. The state of the event
 * is encoded in the position of the array. In other words, "high"
 * events are at the odd indices of the array and "low" ones at the
 * even ones.
 *
 * No effort is made to compress the 64-bit time entries into smaller
 * values. Consequently, a large amount of space is wasted. 
 *
 * Repeated events are not logged. E.g.. if two 1's are logged, the 
 * second one is ignored.
 * 
 * The total number of events is defined when the class is instantiated.
 * The only requirement is that the number of events be an even power
 * of 2. A check for this is made, and 
 */

class BoolEventLogger {

private:
  Cycle_Counter *gcycles;           // Point to gpsim's cycle counter.
  unsigned int  index;              // Index into the buffer
  unsigned long long  *buffer;      // Where the time is stored
  unsigned int max_events;          // Size of the event buffer

public:

  BoolEventLogger(unsigned int _max_events = 4096);

  void event(bool state);
  /*
    get_index - return the current index

    This is used by the callers to record where in the event
    buffer a specific event is stored. (e.g. The start bit
    of a usart bit stream.)
   */
  inline unsigned int get_index(void) {
    return index;
  }

  /// mod_index - get the index modulo buffer size (this
  /// exposes an implementation detail...)
  unsigned int mod_index(unsigned int unmodded_index)
  {
    return unmodded_index & max_events;
  }

  unsigned int get_index(unsigned long long event_time);

  void dump(int start_index, int end_index=-1);
  void dump_ASCII_art(unsigned long long time_step, 
		      unsigned long long start_time,
		      int end_index=-1);

  unsigned int get_event(int index)
  {
    return index & 1;
  }

  bool get_state(unsigned long long event_time) 
  {
    return (get_index(event_time) & 1) ? true : false;
  }

  int get_edges(unsigned long long start_time, unsigned long long end_time)
    {
    return ( get_index(end_time) - get_index(start_time) ) & max_events;
  }

  unsigned long long get_time(unsigned int index)
  {
    return buffer[mod_index(index)];
  }
};


#endif //!defined(__BITLOG_H__)
