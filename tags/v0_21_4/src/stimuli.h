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
class symbol;

/* typedefs */
typedef list<Value*> SymbolList_t;
typedef list<string> StringList_t;


/* Support functions */
extern Stimulus_Node * find_node (string name);
extern void add_node(Stimulus_Node * new_node);
extern void add_node(char *node_name);
extern void dump_node_list(void);
extern stimulus * find_stimulus (string name);
extern void add_stimulus(stimulus * );
void remove_stimulus(stimulus * stimulus);
extern void dump_stimulus_list(void);

extern void stimuli_attach(StringList_t *);
extern void stimuli_attach(SymbolList_t *);

/****************************************************************************
 *
 * Include file support stimuli. 
 *
 *       stimulus             TriggerObject
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
 */

#define MAX_DRIVE        0x100000
#define MAX_ANALOG_DRIVE 0x1000

extern list <Stimulus_Node *> node_list;

extern list <stimulus *> stimulus_list;


class Stimulus_Node : public gpsimObject, public TriggerObject
{
public:
  bool warned;         // keeps track of node warnings (e.g. floating node, contention)
  double voltage;      // The most recent target voltage of this node
  double Cth;          // The most recent capacitance (to ground) measured on this node.
  double Zth;          // The most recent thevenin resistance computed on this node.

  double current_time_constant; // The most recent time constant for the attached stimuli.
  double delta_voltage;     // Amplitude of initial change
  double initial_voltage;   // node voltage at the instant of change
  double finalVoltage;      // Target voltage when settling

  double min_time_constant; // time constants longer than this induce settling
  bool bSettling;           // true when the voltage is settling 
  stimulus *stimuli;        // Pointer to the first stimulus connected to this node.
  int nStimuli;             // number of stimuli attached to this node.
  guint64 future_cycle;     // next simulation cycle for updating the node voltage

  Stimulus_Node(const char *n = 0);
  virtual ~Stimulus_Node();

  double get_nodeVoltage() { return voltage; }
  double get_nodeZth() { return Zth;}
  double get_nodeCth() { return Cth; }

  void update(guint64 current_time);
  void update();
  void refresh();

  void attach_stimulus(stimulus *);
  void detach_stimulus(stimulus *);

  void time_constant(double new_tc);

  // When the node is settling (due to RC charging/discharging)
  // it's voltage is periodically updated by invoking callback()
  virtual void callback(void);
  virtual void callback_print(void);

};


//========================================================================
//
// stimulus
//
// The stimulus class is the base class for all of the analog interfaces
// between modules. A stimulus is a 1-node device that has a characteristic
// impedance and voltage. If you're familiar with circuit analysis, these
// are the Thevenin voltage and impedance.
//
// gpsim is not a spice simulator. So complex devices like transistors or
// opamps are not modeled. In fact, even simple devices like capacitors and
// inductors are not modeled.
//
class stimulus : public gpsimValue
{
public:

  Stimulus_Node *snode;      // Node to which this stimulus is attached

  bool bDrivingState;        // 0/1 digitization of the analog state we're driving
  bool bDrivenState;         // 0/1 digitization of the state we're being driven to
  bool bDriving;             // True if this stimulus is a driver


  double Vth;                // Open-circuit or Thevenin voltage
  double Zth;                // Input or Thevenin resistance
  double Cth;                // Stimulus capacitance.

  double nodeVoltage;        // The voltage driven on to this stimulus by the snode
  stimulus *next;            // next stimulus that's on the snode

  stimulus(const char *n=0);
  virtual ~stimulus();


  // Functions for accessing/manipulating the thevenin voltage and impedance.
  virtual double get_Vth() { return Vth; }
  virtual void   set_Vth(double v) { Vth = v; }
  virtual double get_Zth() { return Zth; }
  virtual void   set_Zth(double z) { Zth = z; }
  virtual double get_Cth() { return Cth; }
  virtual void   set_Cth(double c) { Cth = c; }

  virtual double get_nodeVoltage() { return nodeVoltage; }
  virtual void   set_nodeVoltage(double v) { nodeVoltage = v; }

  virtual bool getDriving() { return bDriving; }
  virtual void setDriving(bool bNewDriving) { bDriving=bNewDriving; }

  // A way of writing digital values to the stimulus.
  // The difference between the 'put' and the 'set' methods is that
  // a stimulus that wants to drive it's own digital state will
  // call 'put' where as when an external node wishes to drive this
  // stimulus it'll call set.
  virtual bool getDrivingState(void) {return bDrivingState;};
  virtual void setDrivingState(bool new_dstate) { bDrivingState = new_dstate;};

  virtual bool getDrivenState(void) {return bDrivenState;};
  virtual void setDrivenState(bool new_dstate) { bDrivenState = new_dstate;};

  virtual void putDrivingState(bool new_dstate) { bDrivingState = new_dstate;};

  // getBitChar - this complements the Register class' getBitStr function
  virtual char getBitChar() { return '0'; }
  virtual void attach(Stimulus_Node *s) { snode = s;};
  virtual void detach(Stimulus_Node *s) { if(snode == s) snode = 0; };

  // If a stimulus changes its state, it can signal this change to
  // any other stimuli that are connected to it.
  virtual void updateNode(void) { if(snode) snode->update(0);}

  // Display info about the stimulus.
  virtual void show();

  // These are only here because they're pure virtual functions in the parent class.
  virtual unsigned int get_value(void) { return 0;}
  virtual void put_value(unsigned int new_value) {}

};

class source_stimulus : public stimulus, public TriggerObject
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



  source_stimulus(void) {
    period = 0;
    duty = 0;
    phase = 0;
    initial_state = 0;
    start_cycle = 0;
    time = 0;
    digital = true;
  };

  virtual void callback(void);
  virtual void callback_print(void);

  void put_period(guint64 new_period) { period = new_period; };
  void put_duty(guint64 new_duty) { duty = new_duty; };
  void put_phase(guint64 new_phase) { phase = new_phase; };
  void put_initial_state(double new_initial_state) { initial_state = new_initial_state; };
  void put_start_cycle(guint64 new_start_cycle) { 
    phase = start_cycle = new_start_cycle; };
  virtual void set_digital(void) { digital = true;}
  virtual void set_analog(void) { digital = false;}
  virtual void start(void) { };

  virtual void show();

protected:
  bool digital;


  guint64
    start_cycle,
    time,
    period,
    duty,
    phase;
  double
    initial_state;
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

  IOPORT *iop;         // Two ways to access parent port
  Register **iopp;     // this second one is used to set break points.
  unsigned int iobit;  // 

  // These are the low to high and high to low input thresholds. The
  // units are volts.
  double l2h_threshold;
  double h2l_threshold;

  // When connect to a node, these are thresholds used to determine whether 
  // we're being driven by a weak driver or not.
  double ZthWeak;
  double ZthFloating;

  IOPIN(void);
  IOPIN(IOPORT *i, unsigned int b, const char *opt_name=0, Register **_iop=0);
  ~IOPIN();

  void attach_to_port(IOPORT *i, unsigned int b);
  void disconnect_from_port();

  virtual void set_nodeVoltage(double v);
  virtual bool getDrivingState(void);
  virtual void setDrivingState(bool new_dstate);
  virtual bool getDrivenState(void);
  virtual void setDrivenState(bool new_dstate);

  virtual void putDrivingState(bool new_dstate);

  virtual Register *get_iop(void);
  virtual void toggle(void);
  virtual void attach(Stimulus_Node *s);

  // These functions don't apply to Inputs, but provide an
  // interface for the derived classes.
  virtual void update_direction(unsigned int x){ };
  virtual IOPIN_DIRECTION  get_direction(void) {return DIR_INPUT; };
  virtual void update_pullup(bool new_state) { }

  virtual double get_Vth();

  virtual char getBitChar();
  virtual void show();

};

class IO_bi_directional : public IOPIN
{
public:

  // Impedance of the IOPIN when it's not driving.
  double ZthIn;

  // Voltage of the IOPIN when it's not driving
  // (this is the voltage the I/O pin floats to when there's
  // nothing connected to it)
  double VthIn;


  IO_bi_directional(void);
  IO_bi_directional(IOPORT *i, unsigned int b,const char *opt_name=0, Register **_iop=0);

  virtual double get_Zth();
  virtual double get_Vth();
  virtual char getBitChar();

  virtual void set_nodeVoltage(double new_nodeVoltage);

  virtual void update_direction(unsigned int);
  virtual IOPIN_DIRECTION  get_direction(void)
  {return ((getDriving()) ? DIR_OUTPUT : DIR_INPUT);}
};




class IO_bi_directional_pu : public IO_bi_directional
{
public:

  IO_bi_directional_pu(IOPORT *i, unsigned int b,const char *opt_name=0, Register **_iop=0);
  ~IO_bi_directional_pu();
  virtual double get_Vth();
  virtual double get_Zth();

  virtual void set_Zpullup(double Z) { Zpullup = Z; }
  virtual double get_Zpullup() { return Zpullup; }
  virtual void set_Vpullup(double V) { Vpullup = V; }
  virtual double get_Vpullup() { return Vpullup; }

  virtual char getBitChar();
  virtual void update_pullup(bool new_state) { bPullUp = new_state; }
protected:
  bool bPullUp;    // True when pullup is enable
  double Zpullup;  // resistance of the pullup
  double Vpullup;  // voltage of the pullup resistor is tied to.
};


class IO_open_collector : public IO_bi_directional_pu
{
public:

  IO_open_collector(IOPORT *i, unsigned int b,const char *opt_name=0, Register **_iop=0);

  virtual double get_Vth();
  virtual double get_Zth();
  virtual char getBitChar();
};

class square_wave : public source_stimulus
{
public:

  square_wave(unsigned int _period, unsigned int _duty, unsigned int _phase, const char *n=0); 

  virtual double get_Vth();


};

class triangle_wave : public source_stimulus
{
public:

  double m1,b1,m2,b2;

  triangle_wave(unsigned int _period, unsigned int _duty, unsigned int _phase, const char *n=0); 

  virtual double get_Vth();

};

class StimulusData {
public:
  guint64 time;
  double value;

};

class ValueStimulusData {
public:
  guint64 time;
  Value  *v;
};

/// ValueStimulus
///

class ValueStimulus : public source_stimulus
{
protected:
  Value             *initial;
  Value             *current;
  guint64            future_cycle;
  ValueStimulusData  next_sample;

  list<ValueStimulusData> samples;
  list<ValueStimulusData>::iterator sample_iterator;

public:

  virtual void callback();
  virtual void put_data(ValueStimulusData &data_point);
  virtual double get_Vth();
  virtual void start();

  ValueStimulus(const char*n=0);
  virtual ~ValueStimulus();
  virtual void show();

protected:
  ValueStimulusData *getNextSample();
};

class AttributeStimulus : public ValueStimulus
{
  Value    *attr;
public:

  AttributeStimulus(const char *n=0);
  // virtual ~AttributeStimulus();

  virtual void callback();
  void setClientAttribute(Value *);
  virtual void show();
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
  virtual void callback(void);
  Event(void);
};
#endif  // __STIMULI_H__
