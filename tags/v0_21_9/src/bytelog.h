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

#if !defined(__BYTELOG_H__)
#define __BYTELOG_H__

class Cycle_Counter;

namespace gpsim {

  //========================================================================

  struct TimedByte {
    unsigned long long start;
    unsigned long long stop;
    unsigned long long rts;
    unsigned int b;
  };

  class ByteLogger
    {
    protected:
      int index;
      int bufsize;
      TimedByte *buffer;

    public:

      ByteLogger(int _bufsize=128);

      int modIndex(int i);
      void start(unsigned long long t);
      void stop(unsigned long long t);
      void byte(unsigned int b);
      void rts(unsigned long long r);
      //  void statistics(int i=-1);
      unsigned long long getStart(int i=-1);
      void get(int i, TimedByte &b);

    };

} // end of namespace gpsim


#endif //!defined(__BYTELOG_H__)
