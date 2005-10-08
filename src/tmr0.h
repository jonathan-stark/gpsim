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

#include "ioports.h"

//---------------------------------------------------------
// TMR0 - Timer
class TMR0 : public sfr_register, public TriggerObject, public SignalSink
{
public:
  unsigned int 
    prescale,
    prescale_counter,
    old_option,       // Save option register contents here.
    state;            // Either on or off right now.
  guint64
    synchronized_cycle,
    future_cycle;
  gint64
    last_cycle;   // can be negative ...


  virtual void callback(void);

  TMR0(void);

  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  virtual unsigned int get(void);
  virtual unsigned int get_value(void);
  virtual void start(int new_value,int sync=0);
  virtual void stop(void);
  virtual void increment(void);   // Used when tmr0 is attached to an external clock
  virtual void new_prescale(void);
  virtual unsigned int get_prescale(void);
  virtual unsigned int max_counts(void) {return 256;};
  void new_clock_source(void);
  virtual bool get_t0cs();
  virtual bool get_t0se();
  virtual void set_t0if(void);
  virtual void reset(RESET_TYPE r);
  virtual void clear_break(void); 
  virtual void callback_print(void);

  virtual void set_cpu(Processor *, PortRegister *, unsigned int pin);
  virtual void set_cpu(Processor *new_cpu, PinModule *pin);
  virtual void setSinkState(char);

private:
  bool m_bLastClockedState;
};

#endif
