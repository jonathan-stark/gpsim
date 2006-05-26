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

  class PulseAttribute;
  class PulsePeriodAttribute;
  class FileNameAttribute;

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

  class StimulusBase : public Module, public TriggerObject
  {
  public:
    StimulusBase(const char *_name, const char *_desc);
    virtual void callback_print();
    void create_iopin_map();
    
  protected:
    IO_bi_directional *m_pin;
  };

  //----------------------------------------------------------------------
  class PulseGen : public StimulusBase
  {
  public:
    static Module *construct(const char *new_name);
    PulseGen(const char *_name);
    ~PulseGen();

    virtual void callback();
    virtual void put_data(ValueStimulusData &data_point);
    virtual string toString();

    void update();
    void update_period();

  private:
    PulseAttribute *m_set;
    PulseAttribute *m_clear;
    PulsePeriodAttribute *m_period;
    guint64 m_future_cycle;
    guint64 m_start_cycle;

    list<ValueStimulusData> samples;
    list<ValueStimulusData>::iterator sample_iterator;

    void setBreak(guint64 next_cycle, list<ValueStimulusData>::iterator );
  };


  class FileStimulus : public StimulusBase
  {
  public:
    static Module *construct(const char *new_name);
    FileStimulus(const char *_name);
    ~FileStimulus();

    void parse(const char *);
    void newFile();
    virtual void callback();
    virtual string toString();

  private:
    FileNameAttribute *m_file;
    guint64 m_future_cycle;
  };
}

#endif // __MOD_STIMULI_H__
