/*
   Copyright (C) 1998,1999,2000,2001 T. Scott Dattalo

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


#ifndef __RESISTOR_H__
#define __RESISTOR_H__

#include "../config.h"

#ifdef HAVE_GUI
#include <gtk/gtk.h>
#endif


#include <glib.h>

#include "../src/modules.h"
#include "../src/packages.h"
#include "../src/stimuli.h"
#include "../src/symbol.h"
#include "../src/trace.h"

class Resistor;

// Resistor_IO - IO pin definition for a resistor.

class Resistor_IO : public IO_bi_directional
{
public:

  bool driving;
  class Resistor *res;

  //virtual SOURCE_TYPE isa(void) {return RESISTOR;};

  Resistor_IO(void);
  Resistor_IO(IOPORT *i, unsigned int b,char *opt_name=NULL);

  // put_node_state is called when the node to which this
  // IO pin is attached is updated. We'll intercept this call
  // and adjust the current through the resistor.
  virtual void put_node_state(int new_state); // From attached node


  //virtual void put_state( int new_state);
  virtual int get_voltage(guint64 current_time);
  //virtual void update_direction(unsigned int);
  //virtual void change_direction(unsigned int);
  //virtual IOPIN_DIRECTION  get_direction(void) {return ((driving) ? DIR_OUTPUT : DIR_INPUT);};
};

class PUResistor_IO : public IOPIN
{
public:

  resistor *res;

  PUResistor_IO(void);
  //  PUResistor_IO(resistor *res);
  //PUResistor_IO(IOPORT *i, unsigned int b,char *opt_name=NULL);

  // put_node_state is called when the node to which this
  // IO pin is attached is updated. We'll intercept this call
  // and adjust the current through the resistor.
  virtual void put_node_state(int new_state); // From attached node


  //virtual void put_state( int new_state);
  virtual int get_voltage(guint64 current_time);
  //virtual void update_direction(unsigned int);
  //virtual void change_direction(unsigned int);
  //virtual IOPIN_DIRECTION  get_direction(void) {return ((driving) ? DIR_OUTPUT : DIR_INPUT);};
};



// Create a few classes from which an LED may be constructed

class Resistor_IOPORT : public IOPORT
{
public:

  virtual void trace_register_write(void);

  Resistor_IOPORT (unsigned int _num_iopins=2);

};



class Resistor : public ExternalModule 
{
public:

  Resistor_IOPORT  *port;

  Resistor(void);
  ~Resistor(void);

  // Inheritances from the Package class
  virtual void create_iopin_map(void);

  static ExternalModule *construct(char *new_name=NULL);

};


class PullupResistor : public ExternalModule 
{
public:
  resistor *res;
#ifdef HAVE_GUI

  GtkWidget *pu_window;

#endif

  PullupResistor(char *init_name=NULL);
  ~PullupResistor();

  // Inheritances from the Package class
  virtual void create_iopin_map(void);

  // Inheritances from the Module class
  virtual void set_attribute(char *attr, char *val);

  static ExternalModule *pu_construct(char *new_name=NULL);
  static ExternalModule *pd_construct(char *new_name=NULL);

#ifdef HAVE_GUI
  void build_window(void);
#endif

};
#endif //  __RESISTOR_H__
