/*
   Copyright (C) 1998,1999,2000 T. Scott Dattalo

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


#ifndef __LOGIC_H__
#define __LOGIC_H__

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include "../src/stimuli.h"
#include "../src/ioports.h"
#include "../src/symbol.h"
#include "../src/modules.h"

#include <gtk/gtk.h>

class LogicGate;
class ANDGate;
class AND2Gate;


/*********************************************************
 *
 * Create a class derived from the IO_input class that
 * will allow us to intercept when the I/O input is being
 * driven. (This isn't done for PIC I/O pins because the
 * logic for handling I/O pin changes resides in the IOPORT
 * class.)
 */

class LogicPin
{
 public:
  LogicGate *LGParent;

  void new_logic_gate(class LogicGate *lg) {LGParent=lg;};
};

class Logic_Input : public IO_input, public LogicPin
{
public:

  virtual void put_digital_state( bool new_state);

  Logic_Input (IOPORT *i, unsigned int b, char *opt_name=NULL) 
    : IO_input(i,b,opt_name) 
    { 
      LGParent = NULL;
    };

};

class Logic_Output : public IO_bi_directional, public LogicPin
{
public:

  //virtual void put_node_state( int new_state);

  Logic_Output (IOPORT *i, unsigned int b,char *opt_name=NULL) 
    : IO_bi_directional(i,b,opt_name)
    { 
      LGParent = NULL;
    };

};

/*************************************************************
 *
 * LogicGate class.
 *
 * Here's the definition for the LogicGate class and all of its
 * children
 *
 *    Module
 *      |
 *      --------
 *             |
 *             |-LogicGate
 *                 |
 *                 |- ANDGate
 *                 |     |
 *                 |     |- AND2Gate
 *                 |- ORGate
 *                       |
 *                       |- OR2Gate
 */

class LogicGate : public Module
{
public:

  int number_of_pins;
  int output_bit_mask;
  unsigned int input_bit_mask;

  IOPORT  *port;

  LogicGate(void);
  ~LogicGate(void);

  // Inheritances from the Package class
  virtual void create_iopin_map(void);


  virtual void update_state(void){};
  virtual int get_num_of_pins(void) {return number_of_pins;};
  void set_number_of_pins(int npins){number_of_pins=npins;};
  GtkWidget *create_pixmap(char **pixmap_data);
};

// 2 input and gate

class ANDGate: public LogicGate
{
public:

  virtual void update_state(void);

};

class AND2Gate: public ANDGate
{
public:

  static Module *construct(const char *new_name);
  const virtual char *type(void) { return ("and2"); };
  // virtual void update_state(void);
  AND2Gate(void);
  ~AND2Gate(void);

};

class ORGate: public LogicGate
{
public:

  virtual void update_state(void);

};

// 2 input or gate

class OR2Gate: public ORGate
{
public:

  //virtual void update_state(void);
  static Module *construct(const char *new_name);
  const virtual char *type(void) { return ("or2"); };
  OR2Gate(void);
  ~OR2Gate(void);
  
};

class XORGate: public LogicGate
{
public:

  virtual void update_state(void);

};

// 2 input or gate

class XOR2Gate: public XORGate
{
public:

  //virtual void update_state(void);
  static Module *construct(const char *new_name);
  const virtual char *type(void) { return ("xor2"); };
  XOR2Gate(void);
  ~XOR2Gate(void);
  
};

class NOTGate: public LogicGate
{
public:

  //virtual void update_state(void);
  static Module *construct(const char *new_name);
  const virtual char *type(void) { return ("not"); };
  virtual void update_state(void);
  NOTGate(void);
  ~NOTGate(void);
  
};
#endif //  __LOGIC_H__

