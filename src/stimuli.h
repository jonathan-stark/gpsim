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


#ifndef __STIMULI_H__
#define __STIMULI_H__
#include <iostream.h>
#include <string>

#include "breakpoints.h"

/* forward references: */

class _14bit_processor;

class stimulus;
class source_stimulus;
class resistor;
class open_collector;
class Stimulus_Node;

/* Support functions */
extern Stimulus_Node * find_node (string name);
extern void add_node(Stimulus_Node * new_node);
extern void add_node(char *node_name);
extern void dump_node_list(void);
extern stimulus * find_stimulus (string name);
extern void add_stimulus(stimulus * );


/*
 * The 'drive levels' are loosely related to the source impedance 
 * a stimulus has. Large positive drive levels mean that the source
 * has a very low output impedance while driving high. Similarly,
 * a large negative drive level corresponds to a very low output
 * impedance while driving low. An I/O pin is a good example of
 * a stimulus that has low source impedance while driving high and
 * low. An open-collector is a good example of a stimulus that has
 * low output impedance while driving low and high output impedance
 * while 'driving' or floating high.
 *
 * Analog stimuli will generally have higher output impedances than
 * digital stimuli. So for example, if you inadvertantly misconfigure
 * the TRIS bits on an analog port as outputs, then the digital outputs
 * will overwhelm the analog inputs. 
 */

#define MAX_DRIVE        0x100000
#define MAX_ANALOG_DRIVE 0x10

class Stimulus_Node
{
public:
  char name_str[30];
  /*  unsigned int total_conductance; ** amount of impedance seen at this node */
  bool warned;   /* keeps track of node warnings (e.g. floating node, contentiong) */
  int state;   /* The most recent value of this node */

  stimulus *stimuli;            /* Pointer to the first stimulus connected to this node. */

  Stimulus_Node(char *n = NULL);
  char * name(void){return name_str;};
  int get_voltage(void) { return state; };
  int update(unsigned int current_time);
  /*  void put_state(int new_state);*/
  void attach_stimulus(stimulus *);
  void detach_stimulus(stimulus *);
  /* void update_conductance(unsigned int stimulus_old_conductance, unsigned int stimulus_new_conductance);*/

};

class stimulus
{
public:
  char name_str[30];
  Stimulus_Node *snode;      /* Node to which this stimulus is attached */
  unsigned int drive;        /* This defines the strength of the source or the magnitude of the load. */
  int state;                 /* The most recent value of this stimulus */
  XrefObject *xref;          /* A link to the gui. */

  stimulus *next;

  stimulus(char *n=NULL);
  virtual int get_voltage(guint64 current_time) {return state;};
  virtual void put_state(int new_state) {state=new_state;};
  virtual void put_state_value(int new_state);
  virtual char * name(void){return name_str;};
  virtual void attach(Stimulus_Node *s) { snode = s;};
};

class source_stimulus : public stimulus
{
public:

enum SOURCE_TYPE
{
  DC,
  SQW,
  ASY,
  TRI,
  RESISTOR,
  OPEN_COLLECTOR
};

  unsigned int 
    period,
    duty,
    phase,
    initial_state,
    state;

  guint64
    start_cycle,
    time;

  virtual int get_voltage(guint64 current_time) {return 0;};
  virtual SOURCE_TYPE isa(void) {return SQW;};
  char *name(void) { return(name_str);};

};

class IOPIN : public stimulus
{
public:

enum IOPIN_TYPE
{
  INPUT_ONLY,          // e.g. MCLR
  BI_DIRECTIONAL,      // most iopins
  BI_DIRECTIONAL_PU,   // same as bi_directional, but with pullup resistor. e.g. portb
  OPEN_COLLECTOR       // bit4 in porta on the 18 pin midrange devices.

};

enum IOPIN_DIRECTION
{
  DIR_INPUT,
  DIR_OUTPUT
};

  IOPORT *iop;
  unsigned int iobit;
  int l2h_threshold;
  int h2l_threshold;

  IOPIN(void);
  IOPIN(IOPORT *i, unsigned int b);
  attach_to_port(IOPORT *i, unsigned int b) {iop = i; iobit=b;};
  virtual IOPIN_TYPE isa(void) {return INPUT_ONLY;};

  virtual int get_voltage(guint64 current_time) {return state;};
  virtual void put_state(int new_state) {state=new_state;}; 
  virtual void put_state_value(int new_state);
  virtual void toggle(void) {state = !state;}; 
  virtual void attach(Stimulus_Node *s);
  virtual void change_direction(unsigned int){return;};
  virtual void update_direction(unsigned int x){return;};
  virtual IOPIN_DIRECTION  get_direction(void) {return DIR_INPUT; };
};

class IO_input : public IOPIN
{
public:

  virtual IOPIN_TYPE isa(void) {return INPUT_ONLY;};
  IO_input(IOPORT *i, unsigned int b);
  IO_input(void);
  virtual int get_voltage(guint64 current_time); //{return drive;};
  virtual void toggle(void);
  virtual void put_state( int new_state);
    /* {
      if(new_state>threshold) {
	iop->setbit( iobit, 1);
	state = 1;
      }	else {
	iop->setbit( iobit, 0);
	state = 0;
      }
    };
    */ 
  virtual void change_direction(unsigned int){return;};
  virtual void update_direction(unsigned int){return;};
  virtual IOPIN_DIRECTION  get_direction(void) {return DIR_INPUT;};
};

class IO_bi_directional : public IO_input
{
public:

  //  source_stimulus *source;
  bool driving;
  
  virtual IOPIN_TYPE isa(void) {return BI_DIRECTIONAL;};
  IO_bi_directional(void);
  IO_bi_directional(IOPORT *i, unsigned int b);
  virtual int get_voltage(guint64 current_time);
  virtual void update_direction(unsigned int);
  virtual void change_direction(unsigned int);
  virtual IOPIN_DIRECTION  get_direction(void) {return ((driving) ? DIR_OUTPUT : DIR_INPUT);};
};

class IO_bi_directional_pu : public IO_bi_directional
{
public:

  resistor *pull_up_resistor;

  virtual IOPIN_TYPE isa(void) {return BI_DIRECTIONAL_PU;};
  IO_bi_directional_pu(IOPORT *i, unsigned int b);
  virtual int get_voltage(guint64 current_time);

};

class IO_open_collector : public IO_input
{
public:

  //  source_stimulus *source;
  bool driving;
  
  virtual IOPIN_TYPE isa(void) {return OPEN_COLLECTOR;};
  IO_open_collector(IOPORT *i, unsigned int b);
  virtual int get_voltage(guint64 current_time);
  virtual void update_direction(unsigned int);
  virtual void change_direction(unsigned int);
  virtual IOPIN_DIRECTION  get_direction(void) {return ((driving) ? DIR_OUTPUT : DIR_INPUT);};

};

class square_wave : public source_stimulus
{
public:

  square_wave(unsigned int _period, unsigned int _duty, unsigned int _phase, char *n=NULL); 

  virtual int get_voltage(guint64 current_time);
  /*    {
      if( ((current_time+phase) % period) <= duty)
	return 1;
      else
	return 0;
    };
    */
      
  char *name(void) { return(name_str);};
  virtual SOURCE_TYPE isa(void) {return SQW;};

};

class triangle_wave : public source_stimulus
{
public:

  float m1,b1,m2,b2;

  virtual int get_voltage(guint64 current_time);
  triangle_wave(unsigned int _period, unsigned int _duty, unsigned int _phase, char *n=NULL); 

  virtual SOURCE_TYPE isa(void) {return TRI;};

};

class asynchronous_stimulus : public source_stimulus, public BreakCallBack
{
public:

  unsigned int
    max_states,
    current_index,
    digital,
    current_state,
    next_state;

  guint64
    *transition_cycles,
    future_cycle;

  int
    *values;

  pic_processor *cpu;
    

  virtual void callback(void);
  virtual int get_voltage(guint64 current_time);
  virtual void start(void);
  asynchronous_stimulus(char *n=NULL);
  virtual SOURCE_TYPE isa(void) {return ASY;};

};


class dc_supply : public source_stimulus {
 public:

  dc_supply(char *n=NULL);
  virtual SOURCE_TYPE isa(void) {return DC;};
  virtual int get_voltage(guint64 current_time) { return drive;};

};

/* For now, a resistor has one end attached to either ground or vcc and the
** other end is attached to a node. As we get more ambitious, this may change.
**/

class resistor : public source_stimulus
{
public:

  virtual int get_voltage(guint64 current_time) {return drive;};
  virtual SOURCE_TYPE isa(void) {return RESISTOR;};
};

class open_collector : public source_stimulus
{
public:

  unsigned int current_state;
  virtual SOURCE_TYPE isa(void) {return OPEN_COLLECTOR;};

};
#endif  // __STIMULI_H__
