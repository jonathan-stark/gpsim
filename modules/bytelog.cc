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


#include "bytelog.h"




ByteLogger::ByteLogger(int _bufsize) 
  : bufsize(_bufsize)
{
  index = 0;
  buffer = (TimedByte *) new char[bufsize * sizeof(TimedByte)];

}

int ByteLogger::modIndex(int i)
{
  if(i<0) {
    i+=bufsize;
    if(i<0)
      return index;
  } else  if(i>=bufsize)
    return index;

  return i;
}


void ByteLogger::start(unsigned long long t)
{
  buffer[index].start = t;
}
void ByteLogger::stop(unsigned long long t) 
{
  buffer[index].stop = t;

  if(++index > bufsize)
    index  = 0;
}
void ByteLogger::byte(unsigned int b) 
{
  buffer[index].b = b&0xff;
}
void ByteLogger::rts(unsigned long long r) 
{
  buffer[index].rts = r;
}
/*
void ByteLogger::statistics(int i=-1)
{
  i = modIndex(i+index);

  unsigned long long t = (buffer[i].stop-buffer[i].start);
  double td= t/2.0;

  cout << "0x" << hex << buffer[i].b 
       << " start time: 0x" << buffer[i].start
       << " byte time: 0x" << t << " cycles = "
       << td  << " uS\n";
}
*/
unsigned long long ByteLogger::getStart(int i)
{

  return buffer[modIndex(i+index)].start;
}

void ByteLogger::get(int i, TimedByte &b)
{

  b = buffer[modIndex(i+index)];
}
