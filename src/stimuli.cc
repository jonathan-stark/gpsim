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
void  gpsim_set_break_delta(guint64 delta, BreakpointObject *f=0);


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

      if ( t->name() == sym->name_str)
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

	if ( t->name() == sym->name_str)
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

  //cout << "getting state of " << name() << '\n';

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
	  double Zt=0.0;

	  while(sptr) {
	    Zt += sptr->get_Zth();
	    sptr = sptr->next;
	  }

	  sptr = stimuli;

	  while(sptr) {
	    voltage += sptr->get_Vth() * (Zt - sptr->get_Zth());
	    sptr = sptr->next;
	  }

	  voltage /= Zt;

	  sptr = stimuli;
	  while(sptr)
	    sptr->set_nodeVoltage(voltage);

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

  current_state = next_state;
  guint64 current_cycle = future_cycle;

  if(verbose)
    cout << "asynchro cycle " << current_cycle << "  state " << current_state << '\n';

  // If there's a node attached to this stimulus, then update it.
  if(snode)
    snode->update(current_cycle);

#if 0
  // Get the next data point.
  if(current_sample) 
    current_sample = current_sample->next;
  else {

    // Either there never was data or the period is 0 
    // and we've already used it up.

    return;
  }

#endif

  // If we've passed through all of the states
  // then start over from the beginning.
  current_sample = current_sample->next;

  if(!current_sample)
    {
      // If the period is zero then we don't want to 
      // regenerate the pulse stream. Also, if the 
      // samples somehow got wiped out, ???


      if( (period == 0) || (!samples)) {

	future_cycle = 0;    // Acts like a flag - indicates this stimulus inactive
	return;
      }

      current_sample = samples;
      start_cycle += period;
      //future_cycle = *transition_cycles + start_cycle;

      //cout << "  stimulus rolled over\n";
      //cout << "   next start_cycle " << start_cycle << "  period " << period << '\n';
    }

  // get the cycle at which it will change

  future_cycle = ((StimulusDataType *)(current_sample->data))->time + start_cycle;
      

  if(future_cycle <= current_cycle)
    {

      // There's an error in the data. Set a break on the next simulation cycle
      // and see if it can be resolved.

      future_cycle = current_cycle+1;
    }
  else
    next_state = ((StimulusDataType *)(current_sample->data))->value;


  cycles.set_break(future_cycle, this);

  if(verbose) {
    cout <<"  next transition = " << future_cycle << '\n';
    cout <<"  next value = " << next_state << '\n';
  }

}

double asynchronous_stimulus::get_Vth() 
{
  //cout << "asy getting state "  << current_state << '\n';
  
  return current_state;
}

// start the asynchronous stimulus. This should be called after the user
// has completely initialized the stimulus.

// KNOWN BUG %%%FIX ME%%% - the asynchronous stimulus assumes that the
// data is sorted correctly (that is chronologically). If it isn't, weird
// things will happen.

void asynchronous_stimulus::start(void)
{

  if(verbose)
    cout << "Starting asynchronous stimulus\n";

  // FIXME - If a cpu has been specified then assign the active one
  //   (a cpu is only needed for the cycle counter)

  if(!cpu)
    cpu = active_cpu;

  if(cpu && samples) //  && transition_cycles)
    {

      current_index = 0;
      /*
      if(digital)
	initial_state = initial_state ? (MAX_DRIVE / 2) : -(MAX_DRIVE / 2);
      */

      current_state = initial_state;

      next_state = ((StimulusDataType *)(samples->data))->value;
      future_cycle = ((StimulusDataType *)(samples->data))->time + start_cycle;

      //if( (period!=0) && (period<transition_cycles[max_states-1]))
      //  cout << "Warning: Asynchronous Stimulus has a period shorter than its last event.\n";
      // This means that the stimulus will not rollover.\n";

      cycles.set_break(future_cycle, this);

      if(verbose) {

	cout << "Asynchronous " << ((digital)?"digital":"analog") << " stimulus\n";
	cout << "  states = " << max_states << '\n';

	current_sample = samples;
	while(current_sample) {
	  
	  cout << "    " << ((StimulusDataType *)(current_sample->data))->time
	       <<  '\t'  << ((StimulusDataType *)(current_sample->data))->value
	       << '\n';

	  current_sample = current_sample->next;
	}

	cout << "first break will be at cycle " <<future_cycle << '\n';
      }

      cout << "period = " << period << '\n'
	   << "phase = " << phase << '\n'
	   << "start_cycle = " << start_cycle << '\n'
	   << "Next break cycle = " << future_cycle << '\n';

      current_sample = samples;
    }
  else {
    if(cpu) 
      cout << "Warning: aynchronous stimulus has no data\n";
  }

  if(verbose)
    cout << "asy should've been started\n";


}

//========================================================================


void asynchronous_stimulus::re_start(guint64 new_start_time)
{

  if(verbose)
    cout << "Re starting asynchronous stimulus\n";

  // FIXME - If a cpu has been specified then assign the active one
  //   (a cpu is only needed for the cycle counter)

  if(!cpu)
    cpu = active_cpu;

  if(cpu && samples) //  && transition_cycles)
    {
      guint64 old_future_cycle = future_cycle;

      current_index = 0;
      start_cycle = new_start_time;

      /*
      if(digital)
	initial_state = initial_state ? (MAX_DRIVE / 2) : -(MAX_DRIVE / 2);
      */

      current_state = initial_state;

      next_state = ((StimulusDataType *)(samples->data))->value;
      future_cycle = ((StimulusDataType *)(samples->data))->time + start_cycle;

      //if( (period!=0) && (period<transition_cycles[max_states-1]))
      //  cout << "Warning: Asynchronous Stimulus has a period shorter than its last event.\n";
      // This means that the stimulus will not rollover.\n";

      if(old_future_cycle) 
	cycles.reassign_break(old_future_cycle,future_cycle, this);
      else
	cycles.set_break(future_cycle, this);

      if(verbose) {

	cout << "Asynchronous " << ((digital)?"digital":"analog") << " stimulus\n";
	cout << "  states = " << max_states << '\n';

	current_sample = samples;
	while(current_sample) {
	  
	  cout << "    " << ((StimulusDataType *)(current_sample->data))->time
	       <<  '\t'  << ((StimulusDataType *)(current_sample->data))->value
	       << '\n';

	  current_sample = current_sample->next;
	}

	cout << "first break will be at cycle " <<future_cycle << '\n';
      }

      cout << "period = " << period << '\n'
	   << "phase = " << phase << '\n'
	   << "start_cycle = " << start_cycle << '\n'
	   << "Next break cycle = " << future_cycle << '\n';

      current_sample = samples;
    }
  else {
    if(cpu) 
      cout << "Warning: aynchronous stimulus has no cpu\n";
  }

  if(verbose)
    cout << "asy should've been started\n";


}


//-------------------------------------------------------------
// put_data
//
// FIXME - this sucks, but as it stands now (0.19.0) gpsim doesn't
// allow you to specify any kind of typing info with asy stimuli data. So 
// it's easy to get the time and value mixed up.
//
// Place the next integer into the 'samples' linked list.
// 
void asynchronous_stimulus::put_data(double data_point)
{

  if(data_flag) {

    ((StimulusDataType *)(current_sample->data))->value = data_point;


  } else {


    // put time
    current_sample = g_slist_append(current_sample, (void *)(new StimulusDataType) );
    max_states++;


    if(!samples)
      samples = current_sample;
    else
      current_sample = current_sample->next;

    ((StimulusDataType *)(current_sample->data))->time = (guint64) data_point;

  }

  data_flag ^= 1;
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

  digital = 1;
  current_sample = 0;
  data_flag = 0;
  samples = 0;

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
  Register *port = get_iop();

  nodeVoltage = new_nodeVoltage;

  if( digital_state &&  (nodeVoltage < h2l_threshold)) {

    // the input is currently high, but the voltage being applied is lower
    // than the switching threshold
  
    set_digital_state(false);

    if(port)
      port->setbit(iobit,false);

  } 
  else if(!digital_state && (nodeVoltage > l2h_threshold)) {

    set_digital_state(true);

    if(port)
      port->setbit(iobit,true);

  }

  // If there's a node attached to this pin, but the pin is not
  // part of an I/O port, then we'll go ahead update the node.
  //if(snode && !port)
  //  snode->update(0);

  //else cout << " no change in IO_input state\n";

}

void IOPIN::put_digital_state(bool new_state)
{
  Register *port = get_iop();
  if(port)
    port->setbit(iobit, new_state);
  else {

    if(new_state != digital_state) {
      digital_state = new_state;
      Vth = digital_state ? 5.0 : 0.3;
      if(snode)
	snode->update(0);
    }
  }
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
  if(iop) 
    digital_state = (iop->value.get() & (1<<iobit)) ? true : false;

  if(driving)
    return digital_state ? Vth : 0;
  else
    return digital_state ? VthIn : 0;

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
  if(iop) 
    digital_state = (iop->value.get() & (1<<iobit)) ? true : false;

  // If the pin is configured as an output, then the driving voltage
  // depends on the pin state. If the pin is an input, and the pullup resistor
  // is enabled, then the pull-up resistor will 'drive' the output. The
  // open circuit voltage in this case will be Vth (the thevenin voltage, 
  // which is assigned to be same as the processor's supply voltage).

  if(driving)
    return digital_state ? Vth : 0;
  else
    return bPullUp ? Vth : VthIn;

}
IO_open_collector::IO_open_collector(IOPORT *i, unsigned int b,char *opt_name, Register **_iopp)
  : IO_bi_directional(i,b,opt_name,_iopp)
{

}


double IO_open_collector::get_Vth()
{
  return 0.0;
}


double IO_open_collector::get_Zth()
{
  return (driving && !digital_state) ? Zth : ZthIn;

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

