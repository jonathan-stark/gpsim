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
#include <iostream>
#include <string>
#include <unistd.h>
#include <glib.h>
using namespace std;
#include <list>

#include "gpsim_classes.h"
#include "breakpoints.h"

/* forward references: */
class Stimulus_Node;
class stimulus;
class IOPORT;

/* Support functions */
extern Stimulus_Node * find_node (string name);
extern void add_node(Stimulus_Node * new_node);
extern void add_node(char *node_name);
extern void dump_node_list(void);
extern stimulus * find_stimulus (string name);
extern void add_stimulus(stimulus * );
extern void dump_stimulus_list(void);

/****************************************************************************
 *
 * Include file support stimuli. 
 *
 *       stimulus             BreakCallBack
 *       |  \                      /
 *       |   -----------------+----
 *       |                    |
 *       |- IOPIN             |- source_stimulus
 *          |                              |
 *          |- IO_input                    |- square_wave
 *             |                           |- triangle_wave
 *             |- IO_open_collector        |- asynchronous_stimulus
 *             |- IO_bi_directional        |- dc_supply
 *                |                        |- open_collector
 *                |- IO_bi_directional_pu
 *
 *  A stimulus is used to stimulate stimuli. What's that mean? Well,
 * in gpsim, the pic I/O pins are derived from the stimulus base class
 * (as can be seen from above). The I/O pins are what interface to the
 * 'external' world. In some cases, I/O pins are inputs and others they're
 * outputs. The stimulus base class defines the basic functionality of
 * a stimulus and how this interface to the outside world is to occur.
 *
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
#define MAX_ANALOG_DRIVE 0x1000

#define STIMULUS_NAME_LENGTH   30

extern list <Stimulus_Node *> node_list;

extern list <stimulus *> stimulus_list;


class Stimulus_Node
{
public:
  char name_str[STIMULUS_NAME_LENGTH];
  /*  unsigned int total_conductance; ** amount of impedance seen at this node */
  bool warned;   /* keeps track of node warnings (e.g. floating node, contentiong) */
  int state;   /* The most recent value of this node */

  stimulus *stimuli;            /* Pointer to the first stimulus connected to this node. */

  Stimulus_Node(const char *n = NULL);
  ~Stimulus_Node();
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
  char name_str[STIMULUS_NAME_LENGTH];
  Stimulus_Node *snode;      /* Node to which this stimulus is attached */
  int drive;        /* This defines the strength of the source or the magnitude of the load. */
  int state;                 /* The most recent value of this stimulus */
  bool digital_state;        /* 0/1 digitization of the analog state */
  XrefObject *xref;          /* A link to the gui. */

  stimulus *next;

  stimulus(char *n=NULL);

  // Two different ways to obtain the stimulus state.
  // 'get_voltage' is sort of like an analog representation of the stimulus state.
  // This is what's called when the a node want to fine how much drive this
  // stimulus is contributing.
  virtual int get_voltage(guint64 current_time) {return state;};

  // 'get_state' returns up-to-date state of the stimulus. I/O Pins (which
  // are derived from this class), can be queried through here.
  virtual int get_state(void) {return state;};

  // Three different ways the stimulus is changed:
  virtual void put_state(int new_state) {state=new_state;};      // From simulation
  virtual void put_node_state(int new_state) {state=new_state;}; // From attached node
  virtual void put_state_value(int new_state);                   // From the gui
  void put_name(const char *new_name);

  // interface to the digital state
  virtual bool get_digital_state(void) {return digital_state;};
  virtual void put_digital_state(bool new_dstate) { digital_state = new_dstate;};

  virtual char * name(void){return name_str;};
  virtual void attach(Stimulus_Node *s) { snode = s;};
  virtual void detach(Stimulus_Node *s) { if(snode == s) snode = NULL; else cout<<"errrrrrr\n";};
};

class source_stimulus : public stimulus, public BreakCallBack
{
public:

enum SOURCE_TYPE
{
  DC,
  SQW,
  ASY,
  TRI,
  RESISTOR,
  OPEN_COLLECTOR,
  EVENT
};

  unsigned int 
    period,
    duty,
    phase,
    initial_state;


  guint64
    start_cycle,
    time;

  source_stimulus(void) {
    period = 0;
    duty = 0;
    phase = 0;
    initial_state = 0;
    start_cycle = 0;
    time = 0;
  };

  virtual int get_voltage(guint64 current_time) {return 0;};
  virtual int get_state(void) {return state;};
  virtual SOURCE_TYPE isa(void) {return SQW;};
  char *name(void) { return(name_str);};

  virtual void callback(void);
  virtual void callback_print(void);

  void put_period(unsigned new_period) { period = new_period; };
  void put_duty(unsigned new_duty) { duty = new_duty; };
  void put_phase(unsigned new_phase) { phase = new_phase; };
  void put_initial_state(unsigned new_initial_state) { initial_state = new_initial_state; };
  void put_start_cycle(unsigned new_start_cycle) { 
    phase = start_cycle = new_start_cycle; };
  virtual void put_data(guint64 data_point) {};
  virtual void put_data(float data_point) {};
  virtual void set_digital(void) { };
  virtual void set_analog(void) { };
  virtual void start(void) { };

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

  IOPORT *iop;    // Two ways to access parent port
  Register **iopp;   // this later one is used to set break points.
  unsigned int iobit;

  int l2h_threshold;
  int h2l_threshold;

  IOPIN(void);
  IOPIN(IOPORT *i, unsigned int b,char *opt_name=NULL, Register **_iop=NULL);
  ~IOPIN();
  void attach_to_port(IOPORT *i, unsigned int b) {iop = i; iobit=b;};
  virtual IOPIN_TYPE isa(void) {return INPUT_ONLY;};

  virtual int get_voltage(guint64 current_time) {return state;};
  virtual int get_state(void);
  virtual Register *get_iop(void);
  virtual void put_state(int new_state) {state=new_state;}; 
  virtual void put_node_state(int new_state) {state=new_state;}; // From attached node
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
  IO_input(IOPORT *i, unsigned int b,char *opt_name=NULL, Register **_iop=NULL);
  IO_input(void);
  virtual int get_voltage(guint64 current_time); //{return drive;};
  virtual void toggle(void);
  virtual void put_state( int new_state);
  virtual void put_node_state(int new_state); // From attached node
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
  IO_bi_directional(IOPORT *i, unsigned int b,char *opt_name=NULL, Register **_iop=NULL);
  virtual void put_state( int new_state);
  virtual int get_voltage(guint64 current_time);
  virtual void update_direction(unsigned int);
  virtual void change_direction(unsigned int);
  virtual IOPIN_DIRECTION  get_direction(void) {return ((driving) ? DIR_OUTPUT : DIR_INPUT);};
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


class IO_bi_directional_pu : public IO_bi_directional
{
public:

  resistor *pull_up_resistor;

  virtual IOPIN_TYPE isa(void) {return BI_DIRECTIONAL_PU;};
  IO_bi_directional_pu(IOPORT *i, unsigned int b,char *opt_name=NULL, Register **_iop=NULL);
  virtual int get_voltage(guint64 current_time);

};

class IO_open_collector : public IO_input
{
public:

  bool driving;
  
  virtual IOPIN_TYPE isa(void) {return OPEN_COLLECTOR;};
  IO_open_collector(IOPORT *i, unsigned int b,char *opt_name=NULL, Register **_iop=NULL);
  virtual int get_voltage(guint64 current_time);
  virtual void update_direction(unsigned int);
  virtual void change_direction(unsigned int);
  virtual IOPIN_DIRECTION  get_direction(void) {return ((driving) ? DIR_OUTPUT : DIR_INPUT);};

};

class square_wave : public source_stimulus
{
public:

  square_wave(unsigned int _period, unsigned int _duty, unsigned int _phase, char *n=NULL); 
  square_wave(char *n=NULL);

  virtual int get_voltage(guint64 current_time);
      
  char *name(void) { return(name_str);};
  virtual SOURCE_TYPE isa(void) {return SQW;};

};

class triangle_wave : public source_stimulus
{
public:

  float m1,b1,m2,b2;

  virtual int get_voltage(guint64 current_time);
  triangle_wave(unsigned int _period, unsigned int _duty, unsigned int _phase, char *n=NULL); 
  triangle_wave(char *n=NULL);

  virtual SOURCE_TYPE isa(void) {return TRI;};

};

typedef struct StimulusData {

  guint64 time;
  int value;

} StimulusDataType;

class asynchronous_stimulus : public source_stimulus
{
public:

  unsigned int
    max_states,
    current_index,
    digital,
    current_state,
    next_state;

  guint64
    //*transition_cycles,
    future_cycle;

  GSList *current_sample;
  GSList *samples;

  //int
  //*values;


  pic_processor *cpu;
    

  virtual void callback(void);
  virtual int get_voltage(guint64 current_time);
  virtual void start(void);
  virtual void re_start(guint64 new_start_time);
  virtual void put_data(guint64 data_point);
  virtual void put_data(float data_point);
  virtual void set_digital(void) { digital = 1; };
  virtual void set_analog(void) { digital = 0; };
  asynchronous_stimulus(char *n=NULL);
  virtual SOURCE_TYPE isa(void) {return ASY;};

 private:
  bool data_flag;
};


class dc_supply : public source_stimulus {
 public:

  dc_supply(char *n=NULL);
  virtual SOURCE_TYPE isa(void) {return DC;};
  virtual int get_voltage(guint64 current_time) { return drive;};

};

class open_collector : public source_stimulus
{
public:

  unsigned int current_state;
  virtual SOURCE_TYPE isa(void) {return OPEN_COLLECTOR;};

};

/*
 * An "Event" is a special stimulus that will assert for a single clock
 * cycle.
 *
 * Since Events are derived from the source_stimulus class, they can
 * be either single shot or repetitive.
 *
 */

class Event : public source_stimulus
{
public:

  unsigned int current_state;
  virtual SOURCE_TYPE isa(void) {return EVENT;};
  virtual void callback(void);
  Event(void);
};
//--------------------------------------------------------------------------
/*
class BUS : public Stimulus_Node
{
public:


};
*/
#endif  // __STIMULI_H__
