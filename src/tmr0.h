/*
   Copyright (C) 1998,1999,2000 T. Scott Dattalo

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

#ifndef __TMR0_H__
#define __TMR0_H__

//---------------------------------------------------------
// TMR0 - Timer
class TMR0 : public sfr_register, public BreakCallBack
{
public:
  unsigned int 
    prescale,
    prescale_counter;
  guint64
    synchronized_cycle,
    future_cycle;
  gint64
    last_cycle;   // can be negative ...


  virtual void callback(void);

  TMR0(void);

  virtual void put(unsigned int new_value);
  unsigned int get(void);
  virtual unsigned int get_value(void);
  void start(int new_value,int sync=0);
  virtual void increment(void);   // Used when tmr0 is attached to an external clock
  virtual void new_prescale(void);
  void new_clock_source(void);

};

#endif
