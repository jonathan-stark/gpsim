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


#include "bitlog.h"
#include <gpsim/gpsim_time.h>

BoolEventLogger::BoolEventLogger(unsigned int _max_events)
  : max_events(_max_events)
{
  max_events = _max_events;

  // Make sure that max_events is an even power of 2
  if(max_events & (max_events - 1)) {
    max_events <<= 1;
    while(1) {
      if(max_events && (max_events & (max_events-1)))
	max_events &= max_events - 1;
      else
	break;

    }
  } else if(!max_events)
    max_events = 4096;
    
  buffer = new unsigned long long[max_events];

  gcycles = &get_cycles();

  max_events--;  // make the max_events a mask

  index = 0;

}


void BoolEventLogger::event(bool state)
{
  // If the new event is different the most recently logged one
  // then we need to log this event. (Note that the event is implicitly
  // logged in the "index". I.e. 1 events are at odd indices.

  if(state ^ (index & 1))
  {
    index = (index + 1) & max_events;
    buffer[index] = gcycles->get();
  }

}


void BoolEventLogger::dump(int start_index, int end_index)
{

    
  if((start_index > (int)max_events) || (start_index <= 0 ))
    start_index = 0;

  if(end_index == -1)
    end_index = index;

  if(start_index == end_index)
    return;

  // Loop through and dump events between the start and end points requested

  do {
    cout << hex << "0x" << start_index << " = 0x" << buffer[start_index];

    if(start_index & 1)
      cout << ": hi\n";
    else
      cout << ": lo\n";

    start_index = (start_index + 1) & max_events;

  }while ( start_index != end_index);

}

void BoolEventLogger::dump_ASCII_art(unsigned long long time_step,
				     unsigned long long start_time,
				     int end_index)
{


  int start_index = get_index(start_time);

  if((start_index > (int)max_events) || (start_index <= 0 )) {
    start_index = 0;
    start_time = buffer[0];
  }

  if(buffer[start_index] == 0) {
    start_index = 0;
    start_time = buffer[0];
  }

  if( (end_index > (int)max_events) || (end_index <= 0 ))
    end_index = index;

  if(start_index == end_index)
    return;

  if(time_step == 0)
    time_step = 1;

  // Loop through and dump events between the start and end points requested

  unsigned long long min_pulse = buffer[end_index] - buffer[start_index];
  unsigned long i = start_index;
  int j = (start_index+1) & max_events;

  do {

    if(  (buffer[j] - buffer[i]) < min_pulse )
      min_pulse = (buffer[j] - buffer[i]);

    i = j;
    j = ++j & max_events; 

  }while (j != end_index);

  //cout << "minimum pulse width :" << min_pulse << '\n';

  if(min_pulse == 0) { // bummer - there's an error in the log
    min_pulse = 1;
    cout << "log error - minimum pulse width shouldn't be zero\n";
  }

  int num_chars = 0;
  unsigned long long t = start_time; //buffer[start_index];
  unsigned long long stop_time = gcycles->get();

  i = start_index;
  do {
    if(t<=buffer[end_index])
      j = get_index(t);
    else
      j = end_index;

    switch(j-i) {
    case 0:
    case 1:
      if(i&1)
	cout <<'-';
      else
	cout <<'_';
      break;
    case 2:
      cout << '|';
      break;
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
      cout << (j-i);
      break;
    default:
      cout << '*';
    }
    i = j;
    t += time_step;
  } while( t<stop_time && num_chars++<1000);

  cout << '\n';

  //cout << "\nend of ASCII art\n";

}



unsigned int BoolEventLogger::get_index(unsigned long long event_time)
{
  unsigned long start_index, end_index, search_index, bstep;

  end_index = index;
  start_index = (index + 1) & max_events;

  bstep = (max_events+1) >> 1;
  search_index = (start_index + bstep) & max_events;
  bstep >>= 1;

  // Binary search for the event time:
  do {
    if(event_time < buffer[search_index])
      search_index = (search_index - bstep) & max_events;
    else
      search_index = (search_index + bstep) & max_events;

    //cout << hex << "search index "<< search_index << "  buffer[search_index] " << buffer[search_index] << '\n';
    bstep >>= 1;

  } while(bstep);

  if(event_time >= buffer[search_index])
    return search_index;
  else
    return (--search_index & max_events);

}




