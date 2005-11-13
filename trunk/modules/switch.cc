/*
   Copyright (C) 2002 Ralf Forsberg

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

/*

  switch.cc

  This is a module that displays a togglebutton on the screen and
  puts the togglebutton state on its output pin.

*/

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include <time.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include <iostream>

#include "../config.h"    // get the definition for HAVE_GUI
#ifdef HAVE_GUI

#include <gtk/gtk.h>
#include "../src/packages.h"
#include "../src/stimuli.h"
#include "../src/symbol.h"
#include "../src/gpsim_interface.h"

#include "switch.h"

//------------------------------------------------------------------------
// SwitchPin

class SwitchPin : public IO_bi_directional
{
public:
  SwitchPin(Switch *parent, const char *_name);

  virtual void   getThevenin(double &v, double &z, double &c);

  virtual double get_Zth() { return m_Zopen;}
  virtual void set_nodeVoltage(double v);

  void PropagateVoltage(double v);
  double get_Zclosed() { return  m_Zclosed; }
private:
  Switch *m_pParent;
  bool bRefreshing;

  double m_Zclosed;
  double m_Zopen;
};


SwitchPin::SwitchPin(Switch *parent, const char *_name)
  : IO_bi_directional(0, 0, _name), m_pParent(parent), bRefreshing(false),
    m_Zclosed(100), m_Zopen(1e12)
{
  assert(m_pParent);
}

void SwitchPin::getThevenin(double &v, double &z, double &c)
{
  if (!bRefreshing) {
    bRefreshing = true;
    m_pParent->getThevenin(this, v, z, c);
    bRefreshing = false;
  } else {
    v = 0;
    z = m_Zopen;
    c = 0;
  }
}

void SwitchPin::set_nodeVoltage(double v)
{

  if (!bRefreshing) {
    bRefreshing = true;

    IOPIN::set_nodeVoltage(v);
    m_pParent->set_nodeVoltage(this,v);
    bRefreshing = false;
  }
}

void SwitchPin::PropagateVoltage(double v)
{

  if (!bRefreshing)
    snode->set_nodeVoltage(v);
}

//========================================================================

class SwitchAttribute : public Boolean
{

public:

  SwitchAttribute(Switch *_parent)
    : Boolean("state",false,"Query or Change the switch"), m_pParent(_parent)
  {
    assert(m_pParent);
  }

  virtual void set(bool  b);
  void setFromButton(bool  b);
private:
  Switch *m_pParent;
};

void SwitchAttribute::set(bool b)
{
  Boolean::set(b);
  m_pParent->setState(b);
}

void SwitchAttribute::setFromButton(bool b)
{
  Boolean::set(b);
}


//--------------------------------------------------------------
// create_iopin_map 
//
//  This is where the information for the Module's package is defined.
// Specifically, the I/O pins of the module are created.

void Switch::create_iopin_map(void)
{

  // Define the physical package.
  //   The Package class, which is a parent of all of the modules,
  //   is responsible for allocating memory for the I/O pins.
  //


  create_pkg(2);


  // Define the I/O pins and assign them to the package.
  //   There are two things happening here. First, there is
  //   a new I/O pin that is being created. For the binary
  //   indicator, both pins are inputs. The second thing is
  //   that the pins are "assigned" to the package. If we
  //   need to reference these newly created I/O pins (like
  //   below) then we can call the member function 'get_pin'.

  string nameA = name() + ".A";
  m_pinA = new SwitchPin(this,nameA.c_str());

  string nameB = name() + ".B";
  m_pinB = new SwitchPin(this,nameB.c_str());

  assign_pin(1, m_pinA);
  assign_pin(2, m_pinB);

  package->set_pin_position(1,2.5);
  package->set_pin_position(2,0.5);

  m_aState = new SwitchAttribute(this);
  add_attribute(m_aState);

}

//--------------------------------------------------------------
// GUI
static void toggle_cb (GtkToggleButton *button, Switch *sw)
{
  if (sw)
    sw->buttonToggled();
}

void Switch::buttonToggled ()
{
  bool b = (gtk_toggle_button_get_active(m_button)==TRUE) ? true : false;

  m_aState->set(b); 

  update();
}

//------------------------------------------------------------------------
//
void Switch::setState(bool bNewState)
{
  if (m_button)
    gtk_toggle_button_set_active(m_button, bNewState ? TRUE : FALSE);
  if ( m_bCurrentState != bNewState) {
    m_bCurrentState = bNewState;
    update();
  }
}

//------------------------------------------------------------------------
void Switch::update()
{
  if (m_pinA->snode)
    m_pinA->snode->update();

  if (!m_bCurrentState)
    m_pinB->snode->update();
}
//------------------------------------------------------------------------
// getThevenin
//
// This is called by one of the switch's pins. The purpose is to obtain
// the electrical state of the switch. If the switch is closed, then the
// electrical state on the other side (that is on the other side as referred
// to the callee's side) is obtained and the switch impedance (resistance)
// is added in. If the switch is open, then the call is reflected back
// down to the callee.

void Switch::getThevenin(SwitchPin *pin, double &v, double &z, double &c)
{
  if (m_bCurrentState) {

    // The switch is closed.

    if (pin == m_pinA)
      m_pinB->getThevenin(v, z, c);
    else if (pin == m_pinB)
      m_pinA->getThevenin(v, z, c);
    else
      return;  // shouldn't ever happen.

    z += pin->get_Zclosed();

  } else
    pin->getThevenin(v, z, c);

}
//------------------------------------------------------------------------
void Switch::set_nodeVoltage(SwitchPin *pin, double v)
{
  if (m_bCurrentState) {

    // The switch is closed.

    if (pin == m_pinA)
      m_pinB->PropagateVoltage(v);
    else if (pin == m_pinB)
      m_pinA->PropagateVoltage(v);
  }
}

//------------------------------------------------------------------------
void Switch::create_widget(Switch *sw)
{

  GtkWidget *box1;

  box1 = gtk_vbox_new (FALSE, 0);

  m_button = GTK_TOGGLE_BUTTON(gtk_toggle_button_new_with_label ((char*)sw->name().c_str()));
  gtk_container_set_border_width (GTK_CONTAINER (m_button), 5);
  gtk_signal_connect (GTK_OBJECT (m_button), "toggled",
		      GTK_SIGNAL_FUNC (toggle_cb), (gpointer)sw);
  gtk_widget_show(GTK_WIDGET(m_button));
  gtk_box_pack_start (GTK_BOX (box1), GTK_WIDGET(m_button), FALSE, FALSE, 0);

  // Tell gpsim which widget to use in breadboard.
  sw->set_widget(box1);

  //sw->setState(false);
}

//--------------------------------------------------------------
// construct
Module * Switch::construct(const char *_new_name=0)
{

  Switch *switchP = new Switch ;
  switchP->new_name(_new_name);
  switchP->create_iopin_map();

  if(get_interface().bUsingGUI()) 
    switchP->create_widget(switchP);

  return switchP;
}

Switch::Switch()
  : m_pinA(0), m_pinB(0), m_aState(0), m_button(0),
    Zopen(1e8), Zclosed(10)
{

  name_str = strdup("Switch");
  
  //interface_id = gpsim_register_interface((gpointer)this);
}

Switch::~Switch(void)
{

}
#endif // HAVE_GUI
