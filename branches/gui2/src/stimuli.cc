/*
   Copyright (C) 1998 T. Scott Dattalo

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


#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string>
#include <list>
#include <vector>

#include "../config.h"
#include "pic-processor.h"
#include "stimuli.h"
#include "stimulus_orb.h"
#include "symbol.h"
#include "interface.h"

list <Stimulus_Node *> node_list;
list <Stimulus_Node *> :: iterator node_iterator;

list <stimulus *> stimulus_list;
list <stimulus *> :: iterator stimulus_iterator;
 
static char num_nodes = 'a';
static char num_stimuli = 'a';
void  gpsim_set_break_delta(guint64 delta, TriggerObject *f=0);


extern Processor *active_cpu;
/*
 * stimulus.cc
 *
 * This file contains some rudimentary infrastructure to support simulating
 * the environment outside of the pic. Simple net lists interconnecting pic
 * I/O pins and various signal generators may be created. 
 *
 * Details:
 * There are two basic concepts behind the stimulus code: nodes and stimuli.
 * The nodes are like wires and the stimuli are like sources and loads. The
 * nodes define the interconnectivity between the stimuli. In most cases there
 * will be only two stimuli connected by one node. For example, you may wish
 * to simulate the effects of a clock input connected to porta.0 . In this case,
 * the stimuli would be the external clock and the pic I/O pin.
 */

//------------------------------------------------------------------
Stimulus_Node * find_node (string name)  // %%% FIX ME %%% * name ???
{
  for (node_iterator = node_list.begin();  node_iterator != node_list.end(); node_iterator++)
    {
      Stimulus_Node *t = *node_iterator;

      if ( t->name() == name)
	{
	  return (t);
	}
    }
  return ((Stimulus_Node *)0);
}

Stimulus_Node * find_node (symbol *sym)
{
  for (node_iterator = node_list.begin();  node_iterator != node_list.end(); node_iterator++)
    {
      Stimulus_Node *t = *node_iterator;

      if ( t->name() == sym->name())
	{
	  return (t);
	}
    }
  return ((Stimulus_Node *)0);
}

void add_node(char *node_name)
{
  
  Stimulus_Node *sn = find_node(string(node_name));

  if(sn)
    cout << "Warning node `" << node_name << "' is already in the node list.\n(You can't have duplicate nodes in the node list.)\n";
  else
    sn = new Stimulus_Node(node_name);

}

void add_node(Stimulus_Node * new_node)
{

  //  if(!node_list.find(new_node))
    node_list.push_back(new_node);
}

void remove_node(Stimulus_Node * node)
{
    node_list.remove(node);
}

void dump_node_list(void)
{
  cout << "Node List\n";

  for (node_iterator = node_list.begin();  node_iterator != node_list.end(); node_iterator++)
    {
      Stimulus_Node *t = *node_iterator;
      cout << t->name() << '\n';
      if(t->stimuli)
	{
	  stimulus *s = t->stimuli;
	  while(s)
	    {
	      cout << '\t' << s->name() << '\n';
	      s = s->next;
	    }
	}
    }
}

void dump_bus_list(void)
{
  dump_node_list();
}


void add_bus(char *bus_name)
{

  /*
  Stimulus_Node *sn = find_node(string(node_name));

  if(sn)
    cout << "Warning node `" << node_name << "' is already in the node list.\n(You can't have duplicate nodes in the node list.)\n";
  else
    sn = new Stimulus_Node(node_name);
  */
  cout << "add_bus -- not supported\n";
}
/*
void add_bus(Stimulus_Node * new_node)
{
  node_list.push_back(new_node);
}
*/

/*****************************************************************
*
*  stimulus * find_stimulus (string name) 
* 
*  Helper function that will search the stimulus list for the stimulus
* called `name' and return a pointer to it if it's found.
*
*/

stimulus * find_stimulus (string name)  // %%% FIX ME %%% * name ???
{
  for (stimulus_iterator = stimulus_list.begin();
       stimulus_iterator != stimulus_list.end(); 
       stimulus_iterator++)
    {
      stimulus *t = *stimulus_iterator;

      if ( t->name() == name)
	{
	  return (t);
	}
    }

  return ((stimulus *)0);
}

stimulus * find_stimulus (symbol *sym)
{
  if(sym) {
    for (stimulus_iterator = stimulus_list.begin();
	 stimulus_iterator != stimulus_list.end(); 
	 stimulus_iterator++)
      {
	stimulus *t = *stimulus_iterator;

	if ( t->name() == sym->name())
	  return (t);
      }
  }

  return ((stimulus *)0);
}

void add_stimulus(stimulus * new_stimulus)
{
  stimulus_list.push_back(new_stimulus);
}

void remove_stimulus(stimulus * stimulus)
{
  stimulus_list.remove(stimulus);
}

void dump_stimulus_list(void)
{
  cout << "Stimulus List\n";

  for (stimulus_iterator = stimulus_list.begin();  
       stimulus_iterator != stimulus_list.end(); 
       stimulus_iterator++)
    {

      stimulus *t = *stimulus_iterator;

      if(t) {
	cout << "stimulus ";

	cout << t->name();

	if(t->snode)
	  cout << " attached to " << t->snode->name();

	cout << '\n';
      }
    }
  cout << "returning from dump\n";
}


//========================================================================

Stimulus_Node::Stimulus_Node(const char *n)
{

  stimuli = 0;
  nStimuli = 0;
  voltage = 0;
  warned  = 0;

  if(n)
    {
      new_name((char *)n);
      symbol_table.add_stimulus_node(this);
    }
  else
    {
      char name_str[100];
      snprintf(name_str,sizeof(name_str),"node%d",num_nodes);
      num_nodes++;    // %%% FIX ME %%%
      new_name(name_str);
    }

  add_node(this);

  gi.node_configuration_changed(this);
}

Stimulus_Node::~Stimulus_Node()
{
    cout << "Stimulus_Node destructor" <<endl;
    stimulus *sptr;

    sptr = stimuli;
    while(sptr)
    {
        cout << "detach " << sptr->name() <<" from node "<<name()<<endl;
	sptr->detach(this);
	sptr = sptr->next;
    }

    remove_node(this);
}

//
// Add the stimulus 's' to the stimulus list for this node
//

void Stimulus_Node::attach_stimulus(stimulus *s)
{
  stimulus *sptr;

  warned = 0;

  if(stimuli)
    {
      sptr = stimuli;
      bool searching=1;
      int nTotalStimuliConnected = 1;

      while(searching)
	{
	  if(s == sptr)
	    return;      // The stimulus is already attached to this node.

	  nTotalStimuliConnected++;
	  if(sptr->next == 0)
	    {
	      sptr->next = s;
	      // s->next = 0;  This is done below
	      searching=0;
	    }
	  sptr = sptr->next;
	}

      nStimuli = nTotalStimuliConnected;
    }
  else
    {
      stimuli = s;     // This is the first stimulus attached to this node.
      nStimuli = 1;
    }

  // If we reach this point, then it means that the stimulus that we're
  // trying to attach has just been placed at the end of the the stimulus
  // list for this node. So we need to 0 terminate the singly-linked list.

  s->next = 0;

  // Now tell the stimulus to attach itself to the node too
  // (If it hasn't already.)

  s->attach(this);
  
  gi.node_configuration_changed(this);

}

//
// Search for the stimulus 's' in the stimulus list for this node.
// If it is found, then remove it from the list.
//

void Stimulus_Node::detach_stimulus(stimulus *s)
{
  stimulus *sptr;


  if(!s)          // You can't remove a non-existant stimulus
    return;

  if(stimuli)
    {
      if(s == stimuli)
	{
	  // This was the first stimulus in the list.

	  stimuli = s->next;
	  s->detach(this);
	}
      else
	{

	  sptr = stimuli;

	  do
	    {
	      if(s == sptr->next)
		{
		  sptr->next = s->next;
		  s->detach(this);
		  nStimuli--;
		  //gi.node_configuration_changed(this);
		  return;
		}

	      sptr = sptr->next;
	    } while(sptr);

	} 
    }
}


double Stimulus_Node::update(guint64 current_time)
{

  //cout << "getting state of Node " << name() << " " << nStimuli << " stim are attached\n";

  if(stimuli != 0)
    {
      stimulus *sptr = stimuli;


      voltage = 0.0;

      switch (nStimuli) {

      case 0:
	// hmm, strange nStimuli is 0, but the stimuli pointer is non null.
	break;

      case 1:
	// Only one stimulus is attached.
	voltage = 0;
	break;

      case 2:
	// 2 stimuli are attached to the node. This is the typical case
	// and we'll optimize for it.
	{
	  stimulus *sptr2 = sptr ? sptr->next : 0;
	  if(!sptr2)
	    break;     // error, nStimuli is two, but there aren't two stimuli

	  double Z1 = sptr->get_Zth();
	  double Z2 = sptr2->get_Zth();
	  double Zt = Z1 + Z2;
	  voltage = (sptr->get_Vth()*Z2  + sptr2->get_Vth()*Z1) / Zt;

	  /*
	  cout << " *N1: " <<sptr->name() 
	       << " V=" << sptr->get_Vth() 
	       << " Z=" << sptr->get_Zth() << endl;
	  cout << " *N2: " <<sptr2->name() 
	       << " V=" << sptr2->get_Vth() 
	       << " Z=" << sptr2->get_Zth() << endl;
	  cout << " * ==>:  V=" << voltage
	       << " Z=" << Zt << endl;
	  */

	  sptr->set_nodeVoltage(voltage);
	  sptr2->set_nodeVoltage(voltage);
	}
	break;

      default:
	{
	  /*
	    There are 3 or more stimuli connected to this node. Recall
	    that these are all in parallel. The Thevenin voltage and 
	    impedance for this is:

	    Thevenin impedance:
	      Zt = 1 / sum(1/Zi)

	    Thevenin voltage:

	    Vt = sum( Vi / ( ((Zi - Zt)/Zt) + 1) )
	       = sum( Vi * Zt /Zi)
	       = Zt * sum(Vi/Zi)
	  */

	  double Ct=0.0;	// Thevenin conductance.

	  while(sptr) {
	    double Cs = 1 / sptr->get_Zth();
	    voltage += sptr->get_Vth() * Cs;
	    Ct += Cs;
	    sptr = sptr->next;
	  }
	  voltage /= Ct;

	  sptr = stimuli;
	  while(sptr) {
	    sptr->set_nodeVoltage(voltage);
	    sptr = sptr->next;
	  }
	}
      }

    }
  else
    if(!warned)
      {
	cout << "Warning: No stimulus is attached to node: \"" << name_str << "\"\n";
	warned = 1;
      }

  return(voltage);

}
//------------------------------------------------------------------------
stimulus::stimulus(char *n)
{
  new_name("stimulus");

  snode = 0;
  digital_state = 0;
  next = 0;

  Vth = 5.0;   // Volts
  Zth = 250;   // Ohms

}

stimulus::~stimulus(void)
{
}

//========================================================================


square_wave::square_wave(unsigned int p, unsigned int dc, unsigned int ph, char *n)
{
      
  //cout << "creating sqw stimulus\n";

  if(n)
    new_name(n);
  else
    {
      char name_str[100];
      snprintf(name_str,sizeof(name_str),"s%d_square_wave",num_stimuli);
      num_stimuli++;
      new_name(name_str);
    }


  period = p;   // cycles
  duty   = dc;  // # of cycles over the period for which the sq wave is high
  phase  = ph;  // phase of the sq wave wrt the cycle counter
  time   = 0;   // simulation time
  snode = 0;
  next = 0;

  add_stimulus(this);
}

double square_wave::get_Vth()
{
  guint64 current_time = cycles.value;

  if(verbose)
    cout << "Getting new state of the square wave.\n";

  if( ((current_time+phase) % period) <= duty)
    return  Vth;
  else
    return  0.0;
}


//========================================================================
//
// triangle_wave

triangle_wave::triangle_wave(unsigned int p, unsigned int dc, unsigned int ph, char *n)
{
      
  //cout << "creating sqw stimulus\n";

  if(n)
    new_name(n);
  else
    {
      char name_str[100];
      snprintf(name_str,sizeof(name_str),"s%d_triangle_wave",num_stimuli);
      num_stimuli++;
      new_name(name_str);
    }

  if(p==0)  //error
    p = 1;

  // copy the square wave stuff
  period = p;   // cycles
  duty   = dc;  // # of cycles over the period for which the sq wave is high
  phase  = ph;  // phase of the sq wave wrt the cycle counter
  time   = 0;   // simulation time
  snode = 0;
  next = 0;

  //cout << "duty cycle " << dc << " period " << p << " drive " << drive << '\n';

  // calculate the slope and the intercept for the two lines comprising
  // the triangle wave:

  if(duty)
    m1 = Vth/duty;
  else
    m1 = Vth/period;   // m1 will not be used if the duty cycle is zero

  b1 = 0;

  if(period != duty)
    m2 = Vth/(duty - period);
  else
    m2 = Vth;

  b2 = -m2 * period;

  //cout << "m1 = " << m1 << " b1 = " << b1 << '\n';
  //cout << "m2 = " << m2 << " b2 = " << b2 << '\n';

  add_stimulus(this);
}

double triangle_wave::get_Vth()
{
  guint64 current_time = cycles.value;

  //cout << "Getting new state of the triangle wave.\n";

  guint64 t = (current_time+phase) % period;

  double ret_val;

  if( t <= duty)
    ret_val = b1 + m1 * t;
  else
    ret_val = b2 + m2 * t;
  
  //  cout << "Triangle wave: t = " << t << " value = " << ret_val << '\n';
  return ret_val;

}

//========================================================================
//
// Event

Event::Event(void)
{
  current_state = 0;
}

//========================================================================
//
void Event::callback(void)
{


  // If there's a node attached to this stimulus, then update it.
  if(snode)
    snode->update(cycles.value);

  // If the event is inactive.

  if(current_state == 0) {
    cycles.set_break_delta(1,this);
    current_state = 1;
  } else {
    current_state = 0;
  }

}

void source_stimulus::callback_print(void)
{
  cout << "stimulus " << name() << " CallBack ID " << CallBackID << '\n';

}


//========================================================================
//
// asynchronous_stimulus
//
// an asynchronous stimulus is a stream of data that can change values at
// arbitrary times. An array called 'transition_cycles' stores the times
// and an array 'values' stores the values.
//   When first initialized, the stimulus is driven to its initial state.
// A break point is set on the cycle counter for the next cpu cycle that
// the stimulus is expected to change values. When the break occurs,
// the current state is updated to the next value  and then a break is set
// for the next expect change. This cycle occurs until all of the values
// have been generated. When the end is reached, the asynchronous stimulus
// will restart from the beginning. The member variable 'period' describes
// the magnitude of the rollover (if it's zero then there is no rollover).
//   
//

void source_stimulus::callback(void)
{
  cout << "shouldn't be called\n";
}

void asynchronous_stimulus::callback(void)
{

  guint64 current_cycle = future_cycle;

  if(digital)
    current_state = (current_sample.value > 0.0) ? Vth : 0.0;
  else
    current_state = current_sample.value;

  if(verbose)
    cout << "asynchro cycle " << current_cycle << "  state " << current_state << '\n';

  // If there's a node attached to this stimulus, then update it.
  if(snode)
    snode->update(current_cycle);

  ++sample_iterator;

  if(sample_iterator == samples.end()) {

    // We've gone through all of the data. Now let's try to start over

    sample_iterator = samples.begin();

    // If the period is zero or if there's no data then we don't want to 
    // regenerate the data stream.

    if( (period == 0) || (sample_iterator == samples.end())) {

      future_cycle = 0;    // Acts like a flag - indicates this stimulus inactive
      return;
    }

    current_sample = *sample_iterator;
    start_cycle += period;

    if(verbose) {
      cout << "  asynchronous stimulus rolled over\n"
	   << "   next start_cycle " << start_cycle << "  period " << period << '\n';
    }
  } else
    current_sample = *sample_iterator;

  if(verbose) {
    cout << "  current_sample (" << current_sample.time << "," 
	 << current_sample.value << ")\n";
    cout << " start cycle " << start_cycle << endl;
  }

  // get the cycle when the data will change next

  future_cycle = current_sample.time + start_cycle;
      

  if(future_cycle <= current_cycle)
    {

      // There's an error in the data. Set a break on the next simulation cycle
      // and see if it can be resolved.

      future_cycle = current_cycle+1;
    }
  else
    next_state = current_sample.value;


  cycles.set_break(future_cycle, this);

  if(verbose) {
    cout <<"  next transition = " << future_cycle << '\n';
    //cout <<"  next value = " << next_state << '\n';
  }

}

double asynchronous_stimulus::get_Vth() 
{
  return current_state;
}

//------------------------------------------------------------------------
// start the asynchronous stimulus.

void asynchronous_stimulus::start(void)
{

  if(verbose)
    cout << "Starting asynchronous stimulus\n";

  sample_iterator = samples.begin();

  if(sample_iterator != samples.end()) {

    current_sample = *sample_iterator;

    if(digital)
      initial_state = (initial_state > 0.0) ? Vth : 0.0;

    current_state = initial_state;

    next_state = current_sample.value;
    future_cycle = current_sample.time + start_cycle;

    cycles.set_break(future_cycle, this);

    if(verbose) {

      cout << "  states = " << samples.size() << '\n';

      list<StimulusData>::iterator si;

      for(si = samples.begin();
	  si != samples.end();
	  ++si) {
	  
	cout << "    " << (*si).time
	     <<  '\t'  << (*si).value
	     << '\n';

      }

      cout << "first break will be at cycle " <<future_cycle << '\n';


      cout << "period = " << period << '\n'
	   << "phase = " << phase << '\n'
	   << "start_cycle = " << start_cycle << '\n'
	   << "Next break cycle = " << future_cycle << '\n';

    }

  }

  if(verbose)
    cout << "asy should've been started\n";


}

//========================================================================


void asynchronous_stimulus::re_start(guint64 new_start_time)
{

  if(verbose)
    cout << "Re starting asynchronous stimulus\n";

  sample_iterator = samples.begin();

  if(sample_iterator != samples.end()) {

    guint64 old_future_cycle = future_cycle;

    start_cycle = new_start_time;

    current_state = initial_state;

    next_state = current_sample.value;
    future_cycle = current_sample.time + start_cycle;

    if(old_future_cycle) 
      cycles.reassign_break(old_future_cycle,future_cycle, this);
    else
      cycles.set_break(future_cycle, this);

  }

  if(verbose)
    cout << "asy should've been started\n";


}


//-------------------------------------------------------------
// put_data
//
void asynchronous_stimulus::put_data(StimulusData &data_point)
{
  samples.push_back(data_point);
}


// Create an asynchronous stimulus. If invoked with a non-null name, then
// give the stimulus that name, other wise create one.
// Note that most of the stimulus' initialization must be performed outside
// of the constructor.

asynchronous_stimulus::asynchronous_stimulus(char *n)
{
  cpu = 0;

  snode = 0;
  next = 0;

  period = 0;
  duty   = 0;
  phase  = 0;
  initial_state  = 0.0;
  start_cycle    = 0;

  if(n)
    new_name(n);
  else
    {
      char name_str[100];
      snprintf(name_str,sizeof(name_str),"s%d_asynchronous_stimulus",num_stimuli);
      num_stimuli++;
      new_name(name_str);
    }

  add_stimulus(this);
  symbol_table.add_stimulus(this);

}

//========================================================================
//

IOPIN::IOPIN(IOPORT *i, unsigned int b,char *opt_name, Register **_iopp)
{
  iop = i;
  iopp = _iopp;
  iobit=b;
  l2h_threshold = 2.0;
  h2l_threshold = 1.0;

  Zth = 1e8;
  Vth = 5.0;

  snode = 0;

  if(iop) {
    iop->attach_iopin(this,b);

    // assign the name to the I/O pin.
    // If one was passed to us (opt_name), then use it
    // otherwise, derive the name from the I/O port to 
    // which this pin is attached.

    char name_str[100];
    if(opt_name) {
      snprintf(name_str,sizeof(name_str),"%s.%s",
	       iop->name().c_str(),
	       opt_name);

    } else {

      char bs[3];

      strncpy(name_str, iop->name().c_str(),sizeof(name_str) - sizeof(bs));
      if(iobit < 10) {
	bs[0] = iobit+'0';
	bs[1] = 0;
      } else {
	bs[0] = (iobit / 10) + '0';
	bs[1] = (iobit % 10) + '0';
	bs[2] = 0;
      }

      strcat(name_str,bs);
    }

    new_name(name_str);
  } else {
    // there's no IO port associated with this pin.

    // If a name was provided, use it:
    if(opt_name)
      new_name(opt_name);
  }

  add_stimulus(this);
  symbol_table.add_stimulus(this);
}

IOPIN::IOPIN(void)
{

  cout << "IOPIN default constructor\n";

  iop = 0;
  iopp = 0;
  iobit=0;
  digital_state = false;
  l2h_threshold = 2.0;
  h2l_threshold = 1.0;
  Vth = 0.3;
  Zth = 1e8;
  snode = 0;

  add_stimulus(this);

}

IOPIN::~IOPIN()
{
  if(snode)
    snode->detach_stimulus(this);

  remove_stimulus(this);
}


void IOPIN::attach(Stimulus_Node *s)
{
  if(iop)
    iop->attach_node(s,iobit);

  snode = s;
}

//
// Accomodate breakpoints by providing an indirect way
// through which the ioport is selected. The breakpoint
// engine is cabable of intercepting this indirect access.
//
Register *IOPIN::get_iop(void)
{
  if(iopp)
    return *iopp;
  else if(iop)
    return iop;
  else
    return 0;
}

//--------------------
// set_nodeVoltage()
//
// 
void IOPIN::set_nodeVoltage(double new_nodeVoltage)
{

  // cout << name()<< " set_nodeVoltage old="<<nodeVoltage <<" new="<<new_nodeVoltage<<endl;
  
  nodeVoltage = new_nodeVoltage;

  if( nodeVoltage < h2l_threshold) {

    // The voltage is below the low threshold
    set_digital_state(false);

  } 
  else if(nodeVoltage > l2h_threshold) {

    // The voltage is above the high threshold
    set_digital_state(true);

  }  else {
    // The voltage is between the low and high thresholds,
    // so do nothing
  }

}

void IOPIN::put_digital_state(bool new_state)
{
  Register *port = get_iop();
  if(port)
    port->setbit(iobit, new_state);

  if(new_state != digital_state) {
    digital_state = new_state;
    Vth = digital_state ? 5.0 : 0.3;
    if(snode)
      snode->update(0);
  }
}

void IOPIN::set_digital_state(bool new_state)
{ 
  digital_state = new_state;

  Register *port = get_iop();
  if(port)
    port->setbit(iobit, new_state);
}

bool IOPIN::get_digital_state(void)
{
  Register *port = get_iop();

  if(port)
    digital_state = port->get_bit(iobit);

  return digital_state;
}

void IOPIN::toggle(void)
{
  put_digital_state(get_digital_state() ^ true);
}
//========================================================================
//
IO_input::IO_input(IOPORT *i, unsigned int b,char *opt_name, Register **_iopp)
  : IOPIN(i,b,opt_name,_iopp)
{
}

IO_input::IO_input(void)
{
  cout << "IO_input default constructor\n";


}


/*************************************
 *  int IO_input::get_voltage(guint64 current_time)
 *
 * If this iopin has a stimulus attached to it then
 * the voltage will be dictated by the stimulus. Otherwise,
 * the voltage is determined by the state of the ioport register
 * that is inside the pic. For an input (like this), the pic code
 * that is being simulated can not change the state of the I/O pin.
 * However, the user has the ability to modify the state of
 * this register either by writing directly to it in the cli,
 * or by clicking in one of many places in the gui.
 */
double IO_input::get_Vth()
{
  if(!snode && iop)
    return ( (iop->value.get() & (1<<iobit)) ? Vth : 0.0);

  return Vth;

}


//========================================================================
//
IO_bi_directional::IO_bi_directional(IOPORT *i, unsigned int b,char *opt_name, Register **_iopp)
  : IO_input(i,b,opt_name,_iopp)
{

  driving = false;

  // Thevenin equivalent while configured as an output 
  Vth = 5.0;
  Zth = 250;

  // Thevenin equivalent while configured as an input 
  VthIn = 0.3;
  ZthIn = 1e8;

}

IO_bi_directional::IO_bi_directional(void)
{
  cout << "IO_bi_directional constructor shouldn't be called\n";
}


void IO_bi_directional::set_nodeVoltage( double new_nodeVoltage)
{
  IOPIN::set_nodeVoltage(new_nodeVoltage);
}

double IO_bi_directional::get_Vth()
{
  if(driving)
    return get_digital_state() ? Vth : 0;
  else
    return get_digital_state() ? VthIn : 0;

}


double IO_bi_directional::get_Zth()
{
  return driving ? Zth : ZthIn;

}



//---------------
//::update_direction(unsigned int new_direction)
//
//  This is called when a new value is written to the tris register
// with which this bi-direction pin is associated.

void IO_bi_directional::update_direction(unsigned int new_direction)
{

  driving = new_direction ? true : false;

  // If this pin is not associated with an IO Port, but it's tied
  // to a stimulus, then we need to update the stimulus.
  if(!iop && snode)
    snode->update(0);
}


IO_bi_directional_pu::IO_bi_directional_pu(IOPORT *i, unsigned int b,char *opt_name, Register **_iopp)
  : IO_bi_directional(i, b,opt_name,_iopp)
{

  Zpullup = 10e3;

}

IO_bi_directional_pu::~IO_bi_directional_pu(void)
{

}

double IO_bi_directional_pu::get_Zth()
{
  return driving ? Zth : (bPullUp ? Zpullup : ZthIn);
}

double IO_bi_directional_pu::get_Vth()
{
  
  /*
  cout << name() << "get_Vth "
       << " driving=" << driving
       << " digital_state=" << digital_state
       << " bPullUp=" << bPullUp << endl;
  */  

  // If the pin is configured as an output, then the driving voltage
  // depends on the pin state. If the pin is an input, and the pullup resistor
  // is enabled, then the pull-up resistor will 'drive' the output. The
  // open circuit voltage in this case will be Vth (the thevenin voltage, 
  // which is assigned to be same as the processor's supply voltage).

  if(driving)
    return get_digital_state() ? Vth : 0;
  else
    return bPullUp ? Vth : VthIn;

}
IO_open_collector::IO_open_collector(IOPORT *i, unsigned int b,char *opt_name, Register **_iopp)
  : IO_bi_directional_pu(i,b,opt_name,_iopp)
{

}


double IO_open_collector::get_Vth()
{
  if(driving && !get_digital_state())
    return 0.0;

  return bPullUp ? Vth : VthIn;
}


double IO_open_collector::get_Zth()
{
  if(driving && !get_digital_state())
    return Zth;

  return bPullUp ? Zpullup : ZthIn;

}


//========================================================================
// 
// helper functions follow here


//--------------------------------------------------------
// Char list.
// Here's a singly linked-list of char *'s.

struct char_list {
  char *name;
  char_list *next;
};

void stimorb_attach(char *node, char_list *stimuli)
{
  if(verbose&2)
    cout << " doing an attach (stimuli.cc) node: " << node << '\n';

  if(!node)
    return;

  string s = string(node);
  Stimulus_Node *sn = find_node (s);

  if(sn)
    {
	
      stimulus *st;
      while(stimuli)
	{
	  s = string(stimuli->name);
	  st = find_stimulus(s);
	  if(st) {
	    sn->attach_stimulus(st);
	    if(verbose&2)
	      cout << " attaching stimulus: " << s << '\n';
	  }
	  else
	    cout << "Warning, stimulus: " << s << " not attached\n";

  	  stimuli = stimuli->next;
	}

      sn->update(0);
    }
  else {
    cout << "Warning: Node \"" << node << "\" was not found in the node list\n";
  }

}

//========================================================================
//  stimuli_attach(list <string> * sl)
//
//  Attach stimuli to a node
//
// The first item in the input list is the name of the node.
// The remaining items are the names of the stimuli.

void stimuli_attach(StringList_t *sl)
{
  if (!sl)
    return;

  list <string> :: iterator si;

  si = sl->begin();

  Stimulus_Node *sn = find_node (*si);

  if(sn)
    {
      for(++si; si != sl->end(); ++si)
	{
	  string s = *si;
	  stimulus *st = find_stimulus(s);

	  if(st) {
	    sn->attach_stimulus(st);
	    if(verbose&2)
	      cout << " attaching stimulus: " << s << '\n';
	  }
	  else
	    cout << "Warning, stimulus: " << s << " not attached\n";
	}

      sn->update(0);
    }
  else {
    cout << "Warning: Node \"" << (*si) << "\" was not found in the node list\n";
  }
}


void stimuli_attach(SymbolList_t *sl)
{
  if (!sl)
    return;

  list <symbol*> :: iterator si;

  si = sl->begin();

  Stimulus_Node *sn = find_node (*si);

  if(sn)
    {
      for(++si; si != sl->end(); ++si)
	{
	  symbol *s = *si;
	  stimulus *st = find_stimulus(s);

	  if(st) {
	    sn->attach_stimulus(st);
	    if(verbose&2)
	      cout << " attaching stimulus: " << s << '\n';
	  }
	  else
	    cout << "Warning, stimulus: " << s << " not attached\n";
	}

      sn->update(0);
    }
  else {
    cout << "Warning: Node \"" << (*si) << "\" was not found in the node list\n";
  }
}

