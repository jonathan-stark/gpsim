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
#include <assert.h>

#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <string>
#include <iostream>
#include <math.h>

#include "../config.h"    // get the definition for HAVE_GUI

#ifdef HAVE_GUI
#include <gtk/gtk.h>
#endif

#include "../src/packages.h"
#include "../src/stimuli.h"
#include "../src/symbol.h"
#include "../src/gpsim_interface.h"
#include "../src/gpsim_time.h"

#include "switch.h"

namespace Switches {
  //------------------------------------------------------------------------
  // SwitchPin

  class SwitchPin : public IO_bi_directional
  {
  public:

                                                                                
                                                                                
    SwitchPin(Switch *parent, const char *_name);

    virtual void getThevenin(double &v, double &z, double &c);
    virtual void sumThevenin(double &current, double &conductance, double &Cth);
    virtual void Build_List(stimulus * st);

    virtual void set_nodeVoltage(double v);

    void PropagateVoltage(double v);
    double get_Zclosed() { return  m_pParent->getZclosed(); }
    double get_Zopen()   { return  m_pParent->getZopen(); }
    bool switch_closed() { return  m_pParent->switch_closed(); }
    SwitchPin * other_pin(SwitchPin *pin) { return m_pParent->other_pin(pin);}

  private:
    Switch *m_pParent;
    bool bRefreshing;

    double m_Zth;

    stimulus  **st_list;        // List of stimuli
    int         st_cnt;         // Size of list
    SwitchPin **sp_list;        // List of Switch pins we have seen
    int         sp_cnt;         // size of list
  };


  SwitchPin::SwitchPin(Switch *parent, const char *_name)
    : IO_bi_directional(_name), m_pParent(parent), bRefreshing(false),
      m_Zth(1e12)
  {
    assert(m_pParent);
    sp_cnt = 5;
    sp_list = (SwitchPin **)calloc(sp_cnt, sizeof(SwitchPin *));
    st_cnt = 10;
    st_list = (stimulus **)calloc(st_cnt, sizeof(stimulus *));

  }

  void SwitchPin::getThevenin(double &v, double &z, double &c)
  {
    m_pParent->getThevenin(this, v, z, c);
    if (verbose)
      cout << "SwitchPin::getThevenin :" << name() << " v=" << v 
           << " z=" << z << endl;
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
      {
        snode->set_nodeVoltage(v);
      }
  }



  /*
	Build_list, given the first stimuli from a node with a connected
	switch, builds a list of all the stimuli of the node except for the
	switch itself. If it encounters other switches on the node, it will
	do one of two things. If the switch is open, the switch stimuli will 
	be ignored.  However, if the switch is closed and the switch pin has
	not already been seen (to stop recursion),  Build_List will look 
	through the switch and add all the stimuli connected to it's other 
	pin.  Stimuli may be encountered more than once, but will only be 
	added to the list the first time.

	The resulting stimulus list is used by sumThevenin.
  */
  void SwitchPin::Build_List(stimulus *st)
  {
    for(; st; st = st->next) 
    {
      if (name() != st->name()) 
      {
	if (typeid(*st) == typeid(*this))  // This is a SwitchPin stimulus
	{
	    SwitchPin *sp_ptr = (SwitchPin *)st;

	    if (sp_ptr->switch_closed())	// Switch is closed
	    {
		int i;
		SwitchPin **sp_pt = sp_list;

		for(i = 0; 
			(i < sp_cnt) && *sp_pt && (*sp_pt != sp_ptr); 
			i++, sp_pt++)
		{}
		if (i+1 >= st_cnt)  // need to grow list
	    	{
		  sp_cnt += 5;
		  if (verbose)
		    cout << name() << "increasing switch pin list size to " 
			<< sp_cnt << endl;
		  sp_cnt += 5;
	      	  sp_list = (SwitchPin **)realloc(sp_list, sp_cnt * sizeof(SwitchPin *));
	      	  sp_pt = sp_list + i;
	    	}
		if (*sp_pt != sp_ptr)	// have not seen this switch pin, add
		{
		   *sp_pt++ = sp_ptr;
		   *sp_pt = NULL;
		   if (verbose)
			cout << name() << " adding switch pin " <<
			  sp_ptr->name() << " to list\n";
		   if (sp_ptr->other_pin(sp_ptr)->snode)
		     Build_List(sp_ptr->other_pin(sp_ptr)->snode->stimuli);
		}
		else if (verbose)
		{
		    cout << name() << " We have already seen " << st->name() << endl;
		}
	    }
	}
	else	// other stimuli
	{
	  int i;
	  stimulus **st_pt = st_list;
	  for(i = 0; (i < st_cnt) && *st_pt && (*st_pt != st); i++, st_pt++)
		{}
	  if (i+1 >= st_cnt)  // need to grow list
	  {
	    st_cnt += 5;
	    if (verbose)
	      cout << name() << "increasing stimuli list size to " 
		  << st_cnt << endl;
	    st_list = (stimulus **)realloc(st_list, st_cnt * sizeof(stimulus *));
	    st_pt = st_list + i;
	  }
	  if (*st_pt != st) // Add stimulus to list
	  {
	    if (verbose)
	      cout << name() << " adding stimulus " << st->name() << endl;
	    *st_pt++ = st;
	    *st_pt = NULL;
	  }
	}
      }
    }
  }
  /* sumThevenin
  ** Sum the Thevenin parameters for all stimuli connected to pin except for the 
  ** pin stimulus itself. This is a helper function for do_voltage().
  **
  **        +---------------+ node         +----------------+
  ** Switch |   +---Zth--+--|-------+------|--+----Z1---+   | First Stimulus
  **   Pin  |  Vth      Cth | -->   | -->  |  C1        V1  | connected to
  **        |   |        |  | I     |  I1  |  |         |   | the switch.
  **        |  ///      /// |       |      | ///       ///  |
  **        +---------------+       :      +----------------+
  **                                :  
  **                                |      +----------------+
  **                                +------|--+----Zn---+   | Last Stimulus
  **                                  -->  |  Cn        Vn  | connected to
  **                                   In  |  |         |   | the switch.
  **                                       | ///       ///  |
  **                                       +----------------+
  **
  */
  void SwitchPin::sumThevenin(double &current, double &conductance,
                              double &Cth)
  {
    stimulus **sptr;

    if (!snode) return;

    *sp_list = NULL;
    *st_list = NULL;
                                                                                
    Build_List(snode->stimuli);
    
    for(sptr = st_list; *sptr ; sptr++)
    {

      double V1,Z1,C1;

        // Get the thevenin parameters of the stimulus connected to the switch pin

        (*sptr)->getThevenin(V1,Z1,C1);
        if (verbose)
          cout << " N: " <<(*sptr)->name() << " V=" << V1
               << " Z=" << Z1 << " C=" << C1 << endl;
                                                                                
        double Cs = 1 / Z1;
	// The Thevenin current is the Thevenin voltage divided by
	// the Thevenin resistance:
        current += V1 * Cs;
        conductance += Cs;
        Cth += C1;
    }
  }

  //========================================================================

  //----------------------------------------

  class ResistanceAttribute : public Float {

  public:
    Switch *m_sw;


    ResistanceAttribute(Switch *psw,
                         double resistance,
                         const char *_name, 
                         const char *_desc)
      : Float(_name,resistance,_desc),
        m_sw(psw)
    {
    }

    virtual void set(int r) {
      double dr = r;
      set(dr);
    }
    virtual void set(double r) {
      Float::set(r);
      if(m_sw)
        m_sw->update();
    }
  };
  //----------------------------------------
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

  }

  //--------------------------------------------------------------
  // GUI
#ifdef HAVE_GUI
  static void toggle_cb (GtkToggleButton *button, Switch *sw)
  {
    if (sw)
      sw->buttonToggled();
  }

  void Switch::buttonToggled ()
  {
    bool b = (gtk_toggle_button_get_active(m_button)==TRUE) ? true : false;

    if (! m_pinA->snode || ! m_pinB->snode)
      {
        cout << "\n WARNING both pins of " << name() 
             << " must be connected to nodes\n";
        return;
      }
    m_aState->set(b); 

    //update();
  }
#endif

  //------------------------------------------------------------------------
  //
  void Switch::setState(bool bNewState)
  {
#ifdef HAVE_GUI
    if (m_button)
      gtk_toggle_button_set_active(m_button, bNewState ? TRUE : FALSE);
#endif
    if ( switch_closed() != bNewState) {
      m_bCurrentState = bNewState;
      update();
    }
  }

  //------------------------------------------------------------------------
  void Switch::update()
  {
    if (m_pinA->snode)
      m_pinA->snode->update();

    if (!switch_closed() && m_pinB->snode)
      m_pinB->snode->update();
  }

  //------------------------------------------------------------------------
  double Switch::getZopen()
  {
    return m_Zopen ? m_Zopen->getVal() : 1e8;
  }
  double Switch::getZclosed()
  {
    return m_Zclosed ? m_Zclosed->getVal() : 10;
  }

  //------------------------------------------------------------------------
  // getThevenin
  //
  // This is called by one of the switch's pins. The purpose is to obtain
  // the electrical state of the switch. If the switch is closed, then the
  // voltage of the closed switch is computed based on the stimulus on both
  // sides of the switch. Each pin of the switch then acts as a voltage source.
  // If the switch is open, the pin impedance is set to a very high value
  // so it has no effect on the other stimulus on the pin's node.

  void Switch::getThevenin(SwitchPin *pin, double &v, double &z, double &c)
  {
    c = 0;
    if (switch_closed()) {      // switch is closed
      v = do_voltage(pin);    // switch voltage
      z = getZclosed();               // low impedance to act as voltage source
    } 
    else {                      // Switch is open set high impedance
      settlingTimeStep = 0;
      v = initial_voltage;
      initial_voltage = -0.3;       // undefined value
      z = getZopen();
    }

  }
  SwitchPin * Switch::other_pin(SwitchPin *pin)
  {
        return( (pin == m_pinA)? m_pinB: m_pinA);
  }

  //------------------------------------------------------------------------
  void Switch::set_nodeVoltage(SwitchPin *pin, double v)
  {
    if (switch_closed()) {

      // The switch is closed.

      other_pin(pin)->PropagateVoltage(v);

    }
  }

  //------------------------------------------------------------------------
  void Switch::create_widget(Switch *sw)
  {
#ifdef HAVE_GUI
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
#endif
  }

  //--------------------------------------------------------------
  // construct
  Module * Switch::construct(const char *_new_name=0)
  {

    Switch *switchP = new Switch(_new_name);
    //switchP->new_name(_new_name);
    switchP->create_iopin_map();

    if(get_interface().bUsingGUI()) 
      switchP->create_widget(switchP);

    return switchP;
  }

  Switch::Switch(const char *_new_name=0)
    : Module(_new_name, "\
Two port switch\n\
 Attributes:\n\
 .state - true if switch is pressed\n\
"), 
      m_pinA(0), m_pinB(0), m_aState(0),
      m_button(0), m_bCurrentState(false)
  {

    // Default module attributes.
    initializeAttributes();
    m_Zopen   = new ResistanceAttribute(this, 1e8,
                                        "Ropen",
                                        "Resistance of opened switch");
    m_Zclosed = new ResistanceAttribute(this, 10,
                                        "Rclosed",
                                        "Resistance of closed switch");
    m_aState = new SwitchAttribute(this);

    add_attribute(m_aState);
    add_attribute(m_Zopen);
    add_attribute(m_Zclosed);

    initial_voltage = -0.5;     // undefined value
  }

  Switch::~Switch(void)
  {
    delete m_Zopen;
    delete m_Zclosed;
    delete m_aState;
  }

  /*
  ** do_voltage computes the voltage at the closed switch by doing a Thevenin
  ** computation on all stimuli connected to both pins of the switch 
  ** excluding the pin stimuli.
  */
  double Switch::do_voltage(SwitchPin *pin)
  {

    double conductance=0.0;   // Thevenin conductance.
    double Cth=0;             // Thevenin capacitance
    double current=0.0;       // current flowing through the switch
    double C1, C2;
    double V1, V2;
    double Zth;
                                                                                
    if (verbose)
      cout << "\nmulti-node summing: " << pin->name() << endl;


    SwitchPin *otherPin = pin == m_pinA ? m_pinB : m_pinA;

    V2 = otherPin->get_nodeVoltage();
    otherPin->sumThevenin(current, conductance, Cth);
    C2 = Cth;

    V1 = pin->get_nodeVoltage();
    pin->sumThevenin(current, conductance, Cth);
    C1 = Cth - C2;

    Zth = 1.0/conductance;
    finalVoltage = current * Zth;
    current_time_constant = Cth * Zth;
                                                                                
    //
    // it takes about 4 time constants to be resonably close to the DC value
    // just use the DC value if the time constant is too short
    //
    if((guint64)(current_time_constant*get_cycles().cycles_per_second()) < 1000) {

      if (verbose)
	cout << "Switch::do_voltage() use DC as current_time_constant=" << 
	  current_time_constant << endl;
      initial_voltage = -0.5;
      voltage = finalVoltage;
      return(voltage);
    } else {
                                                                                
      //
      // If finalVoltage != initial_voltage then start a new calculation.
      // This can mean either a new calculation or one of the node 
      // voltages changed
      //
      if (fabs(finalVoltage - initial_voltage) > 0.1) 
        {
          initial_voltage = finalVoltage;
          voltage = (V1*C1 + V2*C2)/Cth;
          cap_start_cycles = 0;
          if (verbose)
            {
              cout << "Switch::do_voltage() current_time_constant " << 
                current_time_constant ;
              cout << " cps "<< dec << get_cycles().cycles_per_second() << 
                endl;
              cout << "\tV1=" << V1 << " C1=" << C1 << " V2=" << V2 << 
                " C2=" << C2 << endl;
              cout << "\tCapacitance voltage =" <<voltage << endl;
            }
          settlingTimeStep = (guint64) (0.11 * get_cycles().cycles_per_second() * current_time_constant);
        }
      else
        {
          double Time_Step;
          double expz;
          //
          // increase time step as capacitor charges more slowly as final
          // voltage is approached.
          //
          settlingTimeStep  = (guint64) (1.5 * settlingTimeStep);

          //
          // The following is an exact calculation, assuming no circuit 
          // changes,  regardless of time step.
          //
          Time_Step = (get_cycles().value - cap_start_cycles)/
            (get_cycles().cycles_per_second()*current_time_constant);
          expz = exp(-Time_Step);
          voltage = finalVoltage* (1.-expz) + voltage * expz;
        }
      // Are we there yet ?
      if (fabs(finalVoltage - voltage) < 0.1)
        {
          if (verbose)
            {
              cout << "Switch::do_voltage() Reached final Voltage " << 
                finalVoltage << " voltage " << voltage << endl;
            }
          voltage = finalVoltage;
          settlingTimeStep = 0;
          initial_voltage = -0.5;
          return(voltage);
        }
      cap_start_cycles = get_cycles().value;

      if (verbose & 2)
        {

          cout << "Switch::do_voltage() Target voltage=" <<
            finalVoltage << " current voltage=" << voltage <<
            "  Next TimeStep=" <<dec <<  settlingTimeStep << endl;
        }
                                                                                
                                                                                
      get_cycles().set_break(get_cycles().value + settlingTimeStep,this);
    }

    return(voltage);
  }

  void Switch::callback()
  {
    update();
  }


}
