/*
   Copyright (C) 2006 Scott Dattalo

This file is part of the libgpsim_modules library of gpsim

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


#ifndef __MOD_STIMULI_H__
#define __MOD_STIMULI_H__

#ifdef HAVE_GUI
#include <gtk/gtk.h>
#endif
#include <glib.h>
#include "../src/stimuli.h"
#include "../src/modules.h"
#include "../src/value.h"
#include <list>

class Register;
class PicPortRegister;
class PicTrisRegister;
class PicLatchRegister;

namespace ExtendedStimuli {

  class PulseAttribute;
  class PulseInitial;
  class PulsePeriodAttribute;
  class FileNameAttribute;

  class ValueStimulusData {
  public:
    gint64 time;
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
    PulseInitial   *m_init;
    PulsePeriodAttribute *m_period;
    guint64 m_future_cycle;
    guint64 m_start_cycle;

    list<ValueStimulusData> samples;
    list<ValueStimulusData>::iterator sample_iterator;

    void setBreak(guint64 next_cycle, list<ValueStimulusData>::iterator );
  };


  //----------------------------------------------------------------------
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

  //----------------------------------------------------------------------
  class RegisterAddressAttribute; // used for mapping registers into a processor memory
  class PortPullupRegister;     
  class PortStimulus : public Module, public TriggerObject
  {
  public:
    static Module *construct8(const char *new_name);
    static Module *construct16(const char *new_name);
    static Module *construct32(const char *new_name);
    PortStimulus(const char *_name, int nPins);
    ~PortStimulus();
    virtual void callback_print();
    void create_iopin_map();
    
  protected:
    int m_nPins;
    PicPortRegister  *mPort;
    PicTrisRegister  *mTris;
    PicLatchRegister *mLatch;
    PortPullupRegister *mPullup;
    RegisterAddressAttribute *mPortAddress;
    RegisterAddressAttribute *mTrisAddress;
    RegisterAddressAttribute *mLatchAddress;
    RegisterAddressAttribute *mPullupAddress;
  };

}

#endif // __MOD_STIMULI_H__
