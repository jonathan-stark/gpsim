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
along with gpasm; see the file COPYING.  If not, write to
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
#include "xref.h"

list <Stimulus_Node *> node_list;
list <Stimulus_Node *> :: iterator node_iterator;

list <stimulus *> stimulus_list;
list <stimulus *> :: iterator stimulus_iterator;
 
static char num_nodes = 'a';
static char num_stimuli = 'a';
void  gpsim_set_break_delta(guint64 delta, BreakCallBack *f=0);


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
  register int i;

  //cout << "searching for " << name << '\n';

  for (node_iterator = node_list.begin();  node_iterator != node_list.end(); node_iterator++)
    {
      Stimulus_Node *t = *node_iterator;
      string s(t->name_str);
      if ( s == name)
	{
	  //cout << "found it\n";
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
  cout << "add_bus\n";
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
  register int i;


  for (stimulus_iterator = stimulus_list.begin();
       stimulus_iterator != stimulus_list.end(); 
       stimulus_iterator++)
    {
      stimulus *t = *stimulus_iterator;
      string s(t->name_str);
      if ( s == name)
	{
	  //cout << "found it\n";
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

		if(t)
		{
			cout << "stimulus ";
			if(t->name())
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
  warned  = 0;

  if(n)
    {
      strcpy(name_str,n);
    }
  else
    {
      strcpy(name_str,"node   ");

      // give the node a unique name
      name_str[5] = num_nodes++;    // %%% FIX ME %%%
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
      while(searching)
	{
	  if(s == sptr)
	    return;      // The stimulus is already attached to this node.

	  if(sptr->next == 0)
	    {
	      sptr->next = s;
	      // s->next = 0;  This is done below
	      searching=0;
	    }
	  sptr = sptr->next;
	} 
    }
  else
    stimuli = s;     // This is the first stimulus attached to this node.

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
		  //gi.node_configuration_changed(this);
		  return;
		}

	      sptr = sptr->next;
	    } while(sptr);

	} 
    }
}


int Stimulus_Node::update(unsigned int current_time)
{

  int node_voltage = 0;

  //cout << "getting state of " << name() << '\n';

  if(stimuli != 0)
    {
      stimulus *sptr = stimuli;

      // gpsim assumes that there may be multiple sources attached to
      // the node. Usually  one will be dominant. This works well for
      // pullup resistors, open collector drivers, and bi-directional
      // I/O lines. However, if there is more than one source, then
      // the contention is resolved through ohm's law. This works well
      // for analog stuff too.

      while(sptr)
	{
	  node_voltage += sptr->get_voltage(current_time);
	  //cout << " node " << sptr->name() << " voltage is " << sptr->get_voltage(current_time) <<'\n';

	  sptr = sptr->next;
	}
      //cout << "node voltage " << node_voltage << '\n';

      // 'node_voltage' now represents the most up-to-date value of this node.
      // Now, tell all of the stimuli that are interested:
      sptr = stimuli;
      while(sptr)
	{
	  sptr->put_node_state(node_voltage);
	  sptr = sptr->next;
	}

      state = node_voltage;
      return(node_voltage);

    }
  else
    if(!warned)
      {
	cout << "Warning: No stimulus is attached to node: \"" << name_str << "\"\n";
	warned = 1;
      }

  return(0);

}

stimulus::stimulus(char *n)
{
  strcpy(name_str,"stimulus");

  snode = 0;
  drive = 0;
  state = 0;
  digital_state = 0;
  xref = 0;
  next = 0;


  //cout << "stimulus\n";
  xref = new XrefObject((unsigned int *)&state);
}

void stimulus::put_state_value(int new_state)
{
  put_state(new_state);
  if(xref) xref->update();
}

void stimulus::put_name(const char *n)
{
  cout << "stimulus::put_name  " << n << '\n';

  strncpy(name_str,n, STIMULUS_NAME_LENGTH);
    
}
//========================================================================


square_wave::square_wave(unsigned int p, unsigned int dc, unsigned int ph, char *n)
{
      
  //cout << "creating sqw stimulus\n";

  if(n)
    strcpy(name_str,n);
  else
    {
      strcpy(name_str,"s1_square_wave");
      name_str[1] = num_stimuli++;
    }


  period = p;   // cycles
  duty   = dc;  // # of cycles over the period for which the sq wave is high
  phase  = ph;  // phase of the sq wave wrt the cycle counter
  state  = 0;   // output 
  time   = 0;   // simulation time
  snode = 0;
  next = 0;

  drive  = MAX_DRIVE / 2;

  add_stimulus(this);
}

// Create a square wave given only a (possibly) a name

square_wave::square_wave(char *n)
{
  square_wave(0,0,0,n);
}


int square_wave::get_voltage(guint64 current_time)
{
  if(verbose)
    cout << "Getting new state of the square wave.\n";
  if( ((current_time+phase) % period) <= duty)
    return drive;
  else
    return -drive;
}

//========================================================================
//
// triangle_wave

triangle_wave::triangle_wave(unsigned int p, unsigned int dc, unsigned int ph, char *n)
{
      
  //cout << "creating sqw stimulus\n";

  if(n)
    strcpy(name_str,n);
  else
    {
      strcpy(name_str,"s1_triangle_wave");
      name_str[1] = num_stimuli++;
    }

  if(p==0)  //error
    p = 1;

  // copy the square wave stuff
  period = p;   // cycles
  duty   = dc;  // # of cycles over the period for which the sq wave is high
  phase  = ph;  // phase of the sq wave wrt the cycle counter
  state  = 0;   // output 
  time   = 0;   // simulation time
  drive  = 255*5; // Hard coded for now
  snode = 0;
  next = 0;

  //cout << "duty cycle " << dc << " period " << p << " drive " << drive << '\n';
  // calculate the slope and the intercept for the two lines comprising
  // the triangle wave:

  if(duty)
    m1 = float(drive)/duty;
  else
    m1 = float(drive)/period;   // m1 will not be used if the duty cycle is zero

  b1 = 0;

  if(period != duty)
    m2 = float(drive)/(float(duty) - float(period));
  else
    m2 = float(drive);

  b2 = -float(period) * m2;

  //cout << "m1 = " << m1 << " b1 = " << b1 << '\n';
  //cout << "m2 = " << m2 << " b2 = " << b2 << '\n';

  add_stimulus(this);
}

// Create a triangle wave given only a (possibly) a name

triangle_wave::triangle_wave(char *n)
{
  triangle_wave(0,0,0,n);
}


int triangle_wave::get_voltage(guint64 current_time)
{
  //cout << "Getting new state of the triangle wave.\n";

  int t = (current_time+phase) % period;

  /*
  if( t <= duty)
    return int(b1 + m1 * t);
  else
    return int(b2 + m2 * t);
  */

  // debug stuff:
  int ret_val;

  if( t <= duty)
    ret_val = MAX_ANALOG_DRIVE*int(b1 + m1 * t);
  else
    ret_val = MAX_ANALOG_DRIVE*int(b2 + m2 * t);
  
  //  cout << "Triangle wave: t = " << t << " value = " << ret_val << '\n';
  return ret_val;

}

//========================================================================
//
// Event

Event::Event(void)
{
  current_state = 0;
  drive = MAX_DRIVE/2;
  state = -drive;
}

//========================================================================
//
void Event::callback(void)
{


  // If there's a node attached to this stimulus, then update it.
  if(snode)
    snode->update(cycles.value);

  //
  // If the event is inactive.
  if(current_state == 0) {
    cycles.set_break_delta(1,this);
    //gpsim_set_break_delta(1,this);
    current_state = 1;
    state = drive;
  } else {
    current_state = 0;
    -drive;
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
      // samples somehow got wiped out, we can't want


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

int asynchronous_stimulus::get_voltage(guint64 current_time) 
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
    cpu = (pic_processor*)active_cpu;

  if(cpu && samples) //  && transition_cycles)
    {

      current_index = 0;

      if(digital)
	initial_state = initial_state ? (MAX_DRIVE / 2) : -(MAX_DRIVE / 2);


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
      cout << "Warning: aynchronous stimulus has no cpu\n";
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
    cpu = (pic_processor *)active_cpu;

  if(cpu && samples) //  && transition_cycles)
    {
      guint64 old_future_cycle = future_cycle;

      current_index = 0;
      start_cycle = new_start_time;

      if(digital)
	initial_state = initial_state ? (MAX_DRIVE / 2) : -(MAX_DRIVE / 2);


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
void asynchronous_stimulus::put_data(guint64 data_point)
{
  // On the very first call, create the samples linked list.

  /*  if(!samples) {
      samples = g_slist_append(samples, (void *)(new StimulusDataType) );
      current_sample = samples;
      cout << "  creating samples link list\n";
  }
  */

  if(data_flag) {

    // put data
    if(data_point)
      ((StimulusDataType *)(current_sample->data))->value = MAX_DRIVE/2;
    else
      ((StimulusDataType *)(current_sample->data))->value = -MAX_DRIVE/2;


  } else {


    // put time
    current_sample = g_slist_append(current_sample, (void *)(new StimulusDataType) );
    max_states++;


    if(!samples)
      samples = current_sample;
    else
      current_sample = current_sample->next;

    ((StimulusDataType *)(current_sample->data))->time = data_point;

  }

  data_flag ^= 1;
}

void asynchronous_stimulus::put_data(float data_point)
{
  ((StimulusDataType *)(current_sample->data))->value =
    ((guint64)(MAX_ANALOG_DRIVE * data_point));

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
  initial_state  = 0;
  start_cycle    = 0;

  digital = 1;
  current_sample = 0;
  data_flag = 0;
  samples = 0;

  if(n)
    strcpy(name_str,n);
  else
    {
      strcpy(name_str,"a1_asynchronous_stimulus");
      name_str[1] = num_stimuli++;
    }

  add_stimulus(this);

}

//========================================================================
dc_supply::dc_supply(char *n)
{

  snode = 0;
  next = 0;

  if(n)
    strcpy(name_str,n);
  else
    {
      strcpy(name_str,"v1_supply");
      name_str[1] = num_stimuli++;
    }


  drive = MAX_DRIVE / 2;
  add_stimulus(this);

}

//========================================================================
//

IOPIN::IOPIN(IOPORT *i, unsigned int b,char *opt_name, Register **_iopp)
{
  iop = i;
  iopp = _iopp;
  iobit=b;
  state = 0;
  l2h_threshold = 100;
  h2l_threshold = -100;
  drive = 0;
  snode = 0;

  if(iop) {
    iop->attach_iopin(this,b);

    // assign the name to the I/O pin.
    // If one was passed to us (opt_name), then use it
    // otherwise, derive the name from the I/O port to 
    // which this pin is attached.

    if(opt_name) {
      strncpy(name_str, 
	      iop->name(),
	      STIMULUS_NAME_LENGTH-((strlen(opt_name)>30)?30:strlen(opt_name)));
      strcat(name_str,".");
      strcat(name_str,opt_name);
    }
    else {

      strncpy(name_str, iop->name(),STIMULUS_NAME_LENGTH-2);
      char bs[2];
      bs[0] = iobit+'0';
      bs[1] = 0;
      strcat(name_str,bs);
    }
  }

  add_stimulus(this);

}

IOPIN::IOPIN(void)
{

  cout << "IOPIN default constructor\n";

  iop = 0;
  iopp = 0;
  iobit=0;
  state = 0;
  l2h_threshold = 100;
  h2l_threshold = -100;
  drive = 0;
  snode = 0;

  add_stimulus(this);

}

IOPIN::~IOPIN()
{
    if(snode)
	snode->detach_stimulus(this);

    remove_stimulus(this);
}

void IOPIN::put_state_value(int new_state)
{
  Register *port = get_iop();
  if(port)
    port->setbit_value(iobit, new_state &1);

  if(xref)
    xref->update();
}

int IOPIN::get_state(void)
{
  Register *port = get_iop();

  if(snode) {

    if(state>l2h_threshold) 
      return +1;
    else
      return -1;
    
  } else if(port) {

    if(port->get_bit(iobit))
      return +1;
    else
      return -1;
  } 

  // No I/O pin or node is associated with this I/O pin
  return 0;
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

  if(iop)
    return iop;
}

//========================================================================
//
IO_input::IO_input(IOPORT *i, unsigned int b,char *opt_name, Register **_iopp)
  : IOPIN(i,b,opt_name,_iopp)
{

  state = 0;
  drive = 0;

}

IO_input::IO_input(void)
{
  cout << "IO_input default constructor\n";


}
void IO_input::toggle(void)
{
  Register *port = get_iop();

  if(port) {

    port->setbit(iobit, 1^port->get_bit(iobit));
    if(port->xref)
      port->xref->update();
    state = port->get_bit(iobit);

/*
    iop->setbit(iobit, 1^iop->get_bit(iobit));
    if(iop->xref)
      iop->xref->update();
    state = iop->get_bit(iobit);
*/
  }
  else
    state ^= 1;

  if(xref)
    xref->update();
}

/*************************************
 *  int IO_input::get_voltage(guint64 current_time)
 *
 *  If this iopin has a stimulus attached to it then
 * the voltage will be dictated by the stimulus. Otherwise,
 * the voltage is determined by the state of the ioport register
 * that is inside the pic. For an input (like this), the pic code
 * that is being simulated can not change the state of the I/O pin.
 * However, the user has the ability to modify the state of
 * this register either by writing directly to it in the cli,
 * or by clicking in one of many places in the gui.
 */
int IO_input::get_voltage(guint64 current_time)
{
  // The last time the stimulus to which this node is/maybe attached,
  // the drive was updated.

  if(snode)
    return drive;
  else if(iop)
    return ( (iop->value & (1<<iobit)) ? drive : -drive);

  // this input is not attached to a node or an I/O port
  // Perhaps it's an I/O pin on a module... It doesn't really
  // make a whole lot of sense to returning anything, so just
  // return the value of 'drive'.

  return drive;

}

void IO_input::put_state( int new_digital_state)
{
  //cout << "IO_input::put_state() new_state = " << new_digital_state <<'\n';
  Register *port = get_iop();

  if( (new_digital_state != 0) && (state < h2l_threshold)) {

    //cout << " driving I/O line high \n";
    state = l2h_threshold + 1;

    if(port)
      port->setbit(iobit,1);

  } 
  else if((new_digital_state == 0) && (state > l2h_threshold)) {

    //cout << " driving I/O line low \n";
    state = h2l_threshold - 1;
    if(port)
      port->setbit(iobit,0);

  }
  //else cout << " no change in IO_input state\n";

}

// this IO_input is attached to a node that has just been updated.
// The node is now trying to update this stimulus.

void IO_input::put_node_state( int new_state)
{
  //cout << "IO_input::put_node_state() " << " node = " << name() << " new_state = " << new_state <<'\n';


  // No need to proceed if we already in the new_state.

  if(new_state == state)
    return;

  Register *port = get_iop();

  if(port) {

    // If the I/O pin to which this stimulus is mapped is at a logic 
    // high AND the new state is below the high-to-low threshold
    // then we need to drive the I/O line low.
    //
    // Similarly, if the I/O line is low and the new_state is above
    // the low-to-high threshold, we need to drive it low.

    if(port->get_bit(iobit)) {
      if(new_state < h2l_threshold)
	port->setbit(iobit,0);
    } else {
      if(new_state > l2h_threshold)
	port->setbit(iobit,1);
    }
  } else {

    // No I/O port is associated with this I/O pin.
    // Most probably then this is a pin in a Module.

    if(get_digital_state()) {
      if( state < h2l_threshold) {

	//cout << " driving I/O line low \n";
	state = l2h_threshold + 1;
	put_digital_state(0);

      } 
    } else {
      if(state > l2h_threshold) {

	//cout << " driving I/O line high \n";
	state = h2l_threshold - 1;
	put_digital_state(1);
      }
    }
  }


  state = new_state;
}

//========================================================================
//
IO_bi_directional::IO_bi_directional(IOPORT *i, unsigned int b,char *opt_name, Register **_iopp)
  : IO_input(i,b,opt_name,_iopp)
{
  //  source = new source_stimulus();

  state = 0;
  drive = MAX_DRIVE / 2;
  driving = 0;

  //sprintf(name_str,"%s%n",iop->name_str,iobit);
  //cout << name_str;
  // cout << "IO_bi_directional\n";
}


void IO_bi_directional::put_state( int new_digital_state)
{
  //cout << "IO_bi_directional::put_state() new_state = " << new_digital_state <<'\n';

  // If the bi-directional pin is an output then driving is TRUE.
  if(driving) {

    Register *port = get_iop();

    if(port) {
      // If the new state to which the stimulus is being set is different than
      // the current state of the bit in the ioport (to which this stimulus is
      // mapped), then we need to update the ioport.

      if((new_digital_state!=0) ^ ( port->value & (1<<iobit))) {

	port->setbit(iobit,new_digital_state);

	// If this stimulus is attached to a node, then let the node be updated
	// with the new state as well.
	if(snode)
	  snode->update(0);
	// Note that this will auto magically update
	// the io port.

      }
    } else {

      // a port-less pin, probably an external module

      if((new_digital_state ^ digital_state) & 1) {

	digital_state = new_digital_state & 1;

	if(snode)
	  snode->update(digital_state);
      }

    }

  }
  else {

    // The bi-directional pin is configured as an input. So let the parent
    // input class handle it.
    IO_input::put_state(new_digital_state);

  }

}


IO_bi_directional::IO_bi_directional(void)
{
  cout << "IO_bi_directional constructor shouldn't be called\n";
}

IO_bi_directional_pu::IO_bi_directional_pu(IOPORT *i, unsigned int b,char *opt_name, Register **_iopp)
  : IO_bi_directional(i, b,opt_name,_iopp)
{

  pull_up_resistor = new resistor();
  pull_up_resistor->drive = 10;    // %%% FIX ME %%%


  state = 0;
  drive  = MAX_DRIVE / 2;
  driving = 0;

  //  sprintf(name_str,"%s%n",iop->name_str,iobit);
  //cout << name_str;

}

int IO_bi_directional::get_voltage(guint64 current_time)
{
  //cout << "Getting new state of a bi-di IO pin "<< iobit<<'\n';

  if(driving || !snode)
    {

      if( iop->value & (1<<iobit))
	{
	  //cout << " high\n";
	  return drive;
	}
      else
	{
	  //cout << " low\n";
	  return -drive;
	}
    }
  else
    {
      // This node is not driving (because it's configured
      // as an input). There is a stimulus attached to it, so
      // don't upset the 'node summing'. I guess we could return
      // a input leakage value...
      //cout << " not driving\n";
      return 0;
    }

}


//---------------
//::update_direction(unsigned int new_direction)
//
//  This is called when a new value is written to the tris register
// with which this bi-direction pin is associated.

void IO_bi_directional::update_direction(unsigned int new_direction)
{

  //cout << "IO_bi_direction::update_direction\n";

  if(new_direction)
    driving = 1;
  else
    driving = 0;

}

//---------------
//void IO_bi_directional::change_direction(unsigned int new_direction)
//
//  This is called by the gui to change the direction of an 
// io pin. 

void IO_bi_directional::change_direction(unsigned int new_direction)
{

  //cout << __FUNCTION__ << '\n';
  if(iop)
    iop->change_pin_direction(iobit, new_direction & 1);

  if(xref)
    xref->update();
}

int IO_bi_directional_pu::get_voltage(guint64 current_time)
{
  //cout << "Getting new state of a bi-di-pu IO pin "<< iobit;

  if(driving | !snode)
    {
      if( iop->value & (1<<iobit))
	{
	  //cout << " high\n";
	  return drive;
	}
      else
	{
	  //cout << " low\n";
	  return -drive;
	}
    }
  else
    {
      //cout << " pulled up\n";
      return (pull_up_resistor->get_voltage(current_time));
    }

}


IO_open_collector::IO_open_collector(IOPORT *i, unsigned int b,char *opt_name, Register **_iopp)
  : IO_input(i,b,opt_name,_iopp)
{

  drive = MAX_DRIVE / 2;
  driving = 0;

  state = 0;

  //  sprintf(name_str,"%s%n",iop->name_str,iobit);
  //cout << name_str;
  strcpy(name_str, iop->name());
  char bs[2];
  bs[0] = iobit+'0';
  bs[1] = 0;
  strcat(name_str,bs);
  add_stimulus(this);
}


int IO_open_collector::get_voltage(guint64 current_time)
{
  //cout << "Getting new state of an open collector IO pin port "<< iop->name() << " bit " << iobit<<'\n';

  if(driving )
    {
      if( iop->value & (1<<iobit))
	{
	  //cout << "high\n";
	  return 0;
	}
      else
	{
	  //cout << "low\n";
	  return (-MAX_DRIVE/2);
	}
    }
  else
    {
      //cout << "open collector is configured as an input\n";
      if(snode)
	return 0;
      else
	return ( (iop->value & (1<<iobit)) ? drive : (-drive));
    }
}

void IO_open_collector::update_direction(unsigned int new_direction)
{
  //cout << "IO_open_collector::" << __FUNCTION__ << " to new direction " << new_direction << '\n';
  if(new_direction)
    driving = 1;
  else
    driving = 0;

}

//---------------
//void IO_open_collector::change_direction(unsigned int new_direction)
//
//  This is called by the gui to change the direction of an 
// io pin. 

void IO_open_collector::change_direction(unsigned int new_direction)
{

  //cout << "IO_open_collector::" << __FUNCTION__ << '\n';

  if(iop)
    iop->change_pin_direction(iobit, new_direction & 1);

  //iop->tris->setbit(iobit, new_direction & 1);

  if(xref)
    xref->update();
}

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
