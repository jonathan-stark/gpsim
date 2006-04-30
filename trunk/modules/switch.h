/*
   Copyright (C) 2002 Ralf Forsberg

Thiss file is part of gpsim.

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


#ifndef __SWITCH_H__
#define __SWITCH_H__

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include "../src/stimuli.h"
#include "../src/modules.h"

#include <gtk/gtk.h>

namespace Switches {
  class SwitchPin;       // defined and implemented in switch.cc
  class SwitchAttribute; //    "            "

  class Switch : public Module, public TriggerObject

  {
    void create_widget(Switch *sw);
  public:

    Switch(const char *_name);
    ~Switch();


    void update();
    void setState(bool);

    void getThevenin(SwitchPin *, double &v, double &z, double &c);
    void set_nodeVoltage(SwitchPin *, double v);

    // Inheritances from the Package class
    virtual void create_iopin_map();

    // Inheritance from Module class
    const virtual char *type() { return ("switch"); };
    static Module *construct(const char *new_name);

    void buttonToggled();

    virtual double do_voltage(SwitchPin *pin);
    virtual void callback(void);
    virtual double get_nodeVoltage() { return voltage; }
    virtual bool switch_closed() { return m_bCurrentState; }

    // Attributes call back into the switch through here:
    double getZopen();
    double getZclosed();
  protected:
    SwitchPin *m_pinA;
    SwitchPin *m_pinB;

    // State of the switch
    bool m_bCurrentState;
    SwitchAttribute *m_aState;

    double voltage;
    double current_time_constant; // The most recent time constant
    double initial_voltage;       // node voltage at the instant of change
    double finalVoltage;          // Target voltage when settling
    guint64 settlingTimeStep;
    guint64 cap_start_cycles;

    // Switch open and closed resistance.
    // Fix me:
    Float *m_Zopen;
    Float *m_Zclosed;
    //double Zopen, Zclosed;

    // The switch's graphical representation.
    GtkToggleButton *m_button;

  private:
    static void cb_buttonToggle(GtkToggleButton *button, Switch *This);
  };

}
#endif //  __SWITCH_H__
