/*
   Copyright (C) 2006 Scott Dattalo

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


#ifndef __MOD_STIMULI_H__
#define __MOD_STIMULI_H__


#include "../src/stimuli.h"
#include "../src/modules.h"
#include "../src/value.h"
#include <list>

namespace ExtendedStimuli {

  class PulsePin;
  class PulseAttribute;
  class PulsePeriodAttribute;

  class ValueStimulusData {
  public:
    guint64 time;
    Value  *v;

    inline bool operator < (ValueStimulusData &rValue) {
      return time < rValue.time;
    }
    inline bool operator == (ValueStimulusData rValue) {
      return time == rValue.time;
    }
  };

  class PulseGen : public Module, public TriggerObject
  {
  public:
    static Module *construct(const char *new_name);
    PulseGen(const char *_name);
    ~PulseGen();

    virtual void callback();
    virtual void callback_print();
    virtual void put_data(ValueStimulusData &data_point);
    virtual string toString();

    void create_iopin_map();
    void update();
    void update_period();

  private:
    IO_bi_directional *m_pin;
    PulseAttribute *m_set;
    PulseAttribute *m_clear;
    PulsePeriodAttribute *m_period;
    guint64 m_future_cycle;
    guint64 m_start_cycle;

    list<ValueStimulusData> samples;
    list<ValueStimulusData>::iterator sample_iterator;

    void setBreak(guint64 next_cycle, list<ValueStimulusData>::iterator );
  };

}

#endif // __MOD_STIMULI_H__
