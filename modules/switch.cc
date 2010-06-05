/*
   Copyright (C) 2002 Ralf Forsberg

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
#include <typeinfo>

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

  class SwitchPin : public IOPIN
  {
  public:

                                                                                
                                                                                
    SwitchPin(SwitchBase *parent, const char *_name);

    virtual void getThevenin(double &v, double &z, double &c);
    virtual void sumThevenin(double &current, double &conductance, double &Cth);
    virtual void Build_List(stimulus * st);

    virtual void set_Refreshing() { bRefreshing = true; }


    double get_Zopen()   { return  m_pParent->getZopen(); }
    double get_Zclosed() { return  m_pParent->getZclosed(); }
    bool switch_closed() { return  m_pParent->switch_closed(); }
    SwitchPin * other_pin(SwitchPin *pin) { return m_pParent->other_pin(pin);}

  private:
    SwitchBase *m_pParent;
    bool bRefreshing;

    stimulus  **st_list;        // List of stimuli
    int         st_cnt;         // Size of list
    SwitchPin **sp_list;        // List of Switch pins we have seen
    int         sp_cnt;         // size of list
  };


  SwitchPin::SwitchPin(SwitchBase *parent, const char *_name)
    : IOPIN(_name), m_pParent(parent), bRefreshing(false)
  {
    assert(m_pParent);
    sp_cnt = 5;
    sp_list = (SwitchPin **)calloc(sp_cnt, sizeof(SwitchPin *));
    st_cnt = 10;
    st_list = (stimulus **)calloc(st_cnt, sizeof(stimulus *));

  }

  /*
  ** If the switch is closed, look through switch to get values
  */
  void SwitchPin::getThevenin(double &v, double &z, double &c)
  {
    if (switch_closed())
    {
      SwitchPin * op = other_pin(this);
      double current = 0.;
      double conductance = 0.;
      double Cth = 0.;

      op->sumThevenin(current, conductance, Cth);
      z = 1./conductance;
      v = current * z;
      z += (get_Zclosed() ? get_Zclosed() : 0.0);
      c = Cth;
      if (!bRefreshing && op->snode)	// Not called from other pin
      {
	op->set_Refreshing();
	op->snode->update();	// update other pin node
      }
      bRefreshing = false;
    }
    else
    {
       v = 0.;
       z = get_Zopen();
       c = 0;
    }
    set_Vth(v);
    set_Zth(z);
    set_Cth(c);

    if (verbose)
      cout << "SwitchPin::getThevenin :" << name() << " v=" << v 
           << " z=" << z << " Cth=" << c << endl;
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
	    bool state = sp_ptr->switch_closed();

	    if (verbose)
	       cout << "SwitchPin::Build_List " << this->name()
		 << " found " << st->name() << "switch state=" 
		  << (state?"closed":"open") << endl;

	    if (state)	// Switch is closed
	    {
		int i;
		SwitchPin **sp_pt = sp_list;

		// Scan list, stop on match or end of list
		for(i = 0; 
			(i < sp_cnt) && *sp_pt && (*sp_pt != sp_ptr); 
			i++, sp_pt++)
		{}
		if (i+1 >= st_cnt)  // need to grow list
	    	{
		  if (verbose)
			cout << "\tIncrease size of SwitchPin list\n";
		  sp_cnt += 5;
	      	  sp_list = (SwitchPin **)realloc(sp_list, sp_cnt * sizeof(SwitchPin *));
	      	  sp_pt = sp_list + i;

	    	}

		// have not seen this switch pin, add to end of list
		// add stimuli on other pin of switch
		if (*sp_pt != sp_ptr)	
		{
		   *sp_pt++ = sp_ptr;
		   *sp_pt = NULL;

		   if (verbose)
		     cout << "\t" << sp_ptr->name() << " other=" 
		       << sp_ptr->other_pin(sp_ptr)->name() << endl;

		   // Add stimuli on other pin of closed swich
		   if (sp_ptr->other_pin(sp_ptr)->snode)
		     Build_List(sp_ptr->other_pin(sp_ptr)->snode->stimuli);
		}
	    }
	}
	else	// other type of stimulus
	{
	  int i;
	  stimulus **st_pt = st_list;

	  // Scan list, stop on match or end of list
	  for(i = 0; (i < st_cnt) && *st_pt && (*st_pt != st); i++, st_pt++)
		{}

	  if (i+1 >= st_cnt)  // need to grow list
	  {
	    if (verbose)
		cout << "\tIncrease size of stimlui list\n";
	    st_cnt += 5;
	    st_list = (stimulus **)realloc(st_list, st_cnt * sizeof(stimulus *));
	    st_pt = st_list + i;
	  }
	  if (*st_pt != st) // Add stimulus to list
	  {
            if (verbose)
	      cout << "Build_List adding " << st->name() << endl;
	    *st_pt++ = st;
	    *st_pt = NULL;
	  }
	}
      }
    }
  }

  /* sumThevenin
  ** Sum the Thevenin parameters for all stimuli connected to pin except for the 
  ** pin stimulus itself. This is a helper function for getThevenin and do_voltage().
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
                                                                                

    if (verbose)
     cout << "SwitchPin::sumThevenin " << name() << endl;
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
    SwitchBase *m_sw;


    ResistanceAttribute(SwitchBase *psw,
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

    SwitchAttribute(SwitchBase *_parent)
      : Boolean("state",false,"Query or Change the switch"), m_pParent(_parent)
    {
      assert(m_pParent);
    }

    virtual void set(bool  b);
    virtual void set(Value *v);
    virtual void set(const char *buffer, int buf_size = 0);
    virtual void get(char *return_str, int len);
    virtual bool Parse(const char *pValue, bool &bValue);
    void setFromButton(bool  b);
  private:
    SwitchBase *m_pParent;
  };

bool SwitchAttribute::Parse(const char *pValue, bool &bValue) {


  if(strncmp("true", pValue, sizeof("true")) == 0 ||
     strncmp("closed", pValue, sizeof("closed")) == 0)
  {
    bValue = true;
    return true;
  }
  else if(strncmp("false", pValue, sizeof("false")) == 0 ||
     strncmp("open", pValue, sizeof("open")) == 0)
  {
    bValue = false;
    return true;
  }
  return false;
}
void SwitchAttribute::set(Value *v)
{

  if (typeid(*v) == typeid(Boolean))
  {
      bool d;

      v->get(d);
      set(d);
  }
  else if ( typeid(*v) == typeid(String))
  {
     char buff[20];

     v->get((char *)buff, sizeof(buff));
     set(buff);
  }
  else
    throw new TypeMismatch(string("set "), "SwitchAttribute", v->showType());
}


void SwitchAttribute::set(const char *buffer, int buf_size)
{
  if(buffer) {
    bool bValue;
    if(Parse(buffer, bValue)) {
      set(bValue);
    }
  }
}


  void SwitchAttribute::set(bool b)
  {
    Boolean::set(b);
    m_pParent->setState(b);
  }

  void SwitchAttribute::setFromButton(bool b)
  {
    Boolean::set(b);
  }
void SwitchAttribute::get(char *return_str, int len)
{
  if(return_str) {
    bool b;
    Boolean::get(b);
    snprintf(return_str,len,"%s",(b ? "closed" : "open"));
  }
}



  //--------------------------------------------------------------
  // create_iopin_map 
  //
  //  This is where the information for the Module's package is defined.
  // Specifically, the I/O pins of the module are created.

  void SwitchBase::create_iopin_map(void)
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

  //------------------------------------------------------------------------
  //
  void SwitchBase::setState(bool bNewState)
  {
    if ( switch_closed() != bNewState) {
      m_bCurrentState = bNewState;
      update();
    }
  }

  //------------------------------------------------------------------------
  /* 
     If the switch is closed, we send an update to one side only as 
     SwitchPin::getThevenin will send it to the other pin.
     If the switch is open, then update both sides
  */
  void SwitchBase::update()
  {
    if (switch_closed()) // equalise voltage if capacitance involved
      do_voltage();
    if (m_pinA->snode)
      m_pinA->snode->update();

    if (!switch_closed() && m_pinB->snode)
      m_pinB->snode->update();
  }

  //------------------------------------------------------------------------
  double SwitchBase::getZopen()
  {
    return m_Zopen ? m_Zopen->getVal() : 1e8;
  }

  double SwitchBase::getZclosed()
  {
    return m_Zclosed ? m_Zclosed->getVal() : 10;
  }

  SwitchPin * SwitchBase::other_pin(SwitchPin *pin)
  {
	return( (pin == m_pinA)? m_pinB: m_pinA);
  }

  //------------------------------------------------------------------------

  SwitchBase::SwitchBase(const char *_new_name, const char *_desc)
    : Module(_new_name,_desc), 
      m_pinA(0), m_pinB(0), m_bCurrentState(false), m_aState(0)
  {

    // Default module attributes.
    //initializeAttributes();
    m_Zopen   = new ResistanceAttribute(this, 1e8,
                                        "Ropen",
                                        "Resistance of opened switch");
    m_Zclosed = new ResistanceAttribute(this, 10, 	 
					"Rclosed", 	 
					"Resistance of closed switch");
    m_aState = new SwitchAttribute(this);

    addSymbol(m_aState);
    addSymbol(m_Zopen);
    addSymbol(m_Zclosed);
  }

  SwitchBase::~SwitchBase(void)
  {
    removeSymbol(m_aState);
    removeSymbol(m_Zopen);
    removeSymbol(m_Zclosed);
    delete m_Zclosed;
    delete m_Zopen;
    delete m_aState;
  }

  /*
  ** do_voltage computes the initial switch voltage when the switch is closed
  ** and capacitance is present. It is assumed that when the switch closes,
  ** charge will instantaneously be exchanged between the capacitors until 
  ** they have the same voltage.
  */
  void SwitchBase::do_voltage()
  {

    double conductance=0.0;   // Thevenin conductance.
    double Cth=0;             // Thevenin capacitance
    double current=0.0;       // current flowing through the switch
    double C1, C2;
    double V1, V2;
    double Vth;

    V1 = m_pinA->get_nodeVoltage();
    m_pinA->sumThevenin(current, conductance, Cth);
    C1 = Cth;

    V2 = m_pinB->get_nodeVoltage();
    m_pinB->sumThevenin(current, conductance, Cth);
    C2 = Cth - C1;

    if (verbose)
      cout << "\nSwitch::do_voltage " << name() 
	   << " V.A=" << V1
	   << " V.B=" << V2
	   << endl;

    if (Cth)
    {
      Vth = (V1*C1 + V2*C2)/Cth;
      if (verbose)
	cout << "Switch::do_voltage " << name() << " equilise voltage to " 
	  << Vth << endl << " V1=" << V1 << " V2=" << V2 << " C1=" << C1 
	  << " C2=" << C2 << endl;

      if (m_pinA->snode)
        m_pinA->snode->set_nodeVoltage(Vth);
      if (m_pinB->snode)
        m_pinB->snode->set_nodeVoltage(Vth);
    }
  }

  //========================================================================

  Switch::Switch(const char *_new_name=0)
    : SwitchBase(_new_name, "\
Two port switch\n\
 Attributes:\n\
 .state - true if switch is pressed\n\
"), 
      m_button(0)
  {

  }

  Switch::~Switch()
  {
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
    bool b = gtk_toggle_button_get_active(m_button) ? true : false;

    if (! m_pinA->snode || ! m_pinB->snode)
      {
        cout << "\n WARNING both pins of " << name() 
             << " must be connected to nodes\n";
        return;
      }
    m_aState->set(b); 

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
    SwitchBase::setState(bNewState);
  }
  //------------------------------------------------------------------------
  void Switch::create_widget(Switch *sw)
  {
#ifdef HAVE_GUI
    GtkWidget *box1;


    box1 = gtk_vbox_new (FALSE, 0);

    m_button = GTK_TOGGLE_BUTTON(gtk_toggle_button_new_with_label ((char*)sw->name().c_str()));
    gtk_container_set_border_width (GTK_CONTAINER (m_button), 1);
    gtk_signal_connect (GTK_OBJECT (m_button), "toggled",
                        GTK_SIGNAL_FUNC (toggle_cb), (gpointer)sw);
    gtk_widget_show(GTK_WIDGET(m_button));
    gtk_box_pack_start (GTK_BOX (box1), GTK_WIDGET(m_button), FALSE, FALSE, 0);

    gtk_widget_show_all (box1);
    // Tell gpsim which widget to use in breadboard.
   sw->set_widget(box1);
#endif
  }

}

