/*
   Copyright (C) 1998-2003 T. Scott Dattalo

This file is part of the libgpsim library of gpsim

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

#if !defined(__BITLOG_H__)
#define __BITLOG_H__

class Cycle_Counter;

// include the absolute minimum portion of GLIB to get the definitions
// for guint64, etc.
#include <glibconfig.h>

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
  unsigned int   index;             // Index into the buffer
  guint64       *buffer;            // Where the time is stored
  unsigned int   max_events;        // Size of the event buffer

public:

  BoolEventLogger(unsigned int _max_events = 4096);

  void event(bool state);
  /*
    get_index - return the current index

    This is used by the callers to record where in the event
    buffer a specific event is stored. (e.g. The start bit
    of a usart bit stream.)
   */
  inline unsigned int get_index() {
    return index;
  }

  /// mod_index - get the index modulo buffer size (this
  /// exposes an implementation detail...)
  unsigned int mod_index(unsigned int unmodded_index)
  {
    return unmodded_index & max_events;
  }

  unsigned int get_index(guint64 event_time);

  void dump(int start_index, int end_index=-1);
  void dump_ASCII_art(guint64 time_step, 
		      guint64 start_time,
		      int end_index=-1);

  unsigned int get_event(int index)
  {
    return index & 1;
  }

  bool get_state(guint64 event_time) 
  {
    return (get_index(event_time) & 1) ? true : false;
  }

  int get_edges(guint64 start_time, guint64 end_time)
    {
    return ( get_index(end_time) - get_index(start_time) ) & max_events;
  }

  guint64 get_time(unsigned int index)
  {
    return buffer[mod_index(index)];
  }
};


/**********************************************************************
 * ThreeState event logging
 *
 * The ThreeState event logger is a simple class for logging the time
 * of 3-state events. (FixMe - the bitlog and bytelog classes should
 * be deprecated and merged into this class).
 *
 * The event buffer stores both the event state and the 64-bit time
 * at which it occurred. Event states are 'chars' so it is up to the
 * client of this class to interpret what the events mean.
 *
 * Repeated events are not logged. E.g.. if two 1's are logged, the 
 * second one is ignored.
 * 
 */

class ThreeStateEventLogger {

private:
  Cycle_Counter *gcycles;           // Point to gpsim's cycle counter.
  unsigned int   index;             // Index into the buffer
  guint64       *pTimeBuffer;       // Where the time is stored
  char          *pEventBuffer;      // Where the events are stored
  unsigned int   max_events;        // Size of the event buffer
  bool           bHaveEvents;       // True if any events have been acquired
public:

  ThreeStateEventLogger(unsigned int _max_events = 4096);

  /// Log an Event
  void event(char state);

  inline unsigned int get_index() {
    return index;
  }

  unsigned int get_index(guint64 event_time);
  unsigned int get_nEvents(guint64 start_time,guint64 stop_time);
  unsigned int get_nEvents(unsigned int start_index, unsigned int stop_index);
  char get_state(unsigned int index)
  {
    return pEventBuffer[index & max_events];
  }

  char get_state(guint64 event_time) 
  {
    return get_state(get_index(event_time));
  }

  guint64 get_time(unsigned int index)
  {
    return pTimeBuffer[index & max_events];
  }
  void dump(int start_index, int end_index=-1);
  void dump_ASCII_art(guint64 time_step, 
		      guint64 start_time,
		      int end_index=-1);


};

#endif //!defined(__BITLOG_H__)
