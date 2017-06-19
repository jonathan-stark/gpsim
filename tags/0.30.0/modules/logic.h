/*
   Copyright (C) 1998,1999,2000 T. Scott Dattalo

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


#ifndef __LOGIC_H__
#define __LOGIC_H__

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#include "../src/stimuli.h"
#include "../src/modules.h"

#ifdef HAVE_GUI
#include <gtk/gtk.h>
#endif

class LogicGate;

/*********************************************************
 *
 * Create a class derived from the IO_input class that
 * will allow us to intercept when the I/O input is being
 * driven. (This isn't done for PIC I/O pins because the
 * logic for handling I/O pin changes resides in the IOPORT
 * class.)
 */

class Logic_Input : public IOPIN
{
private:
  LogicGate *LGParent;
  unsigned int m_iobit;

public:

  virtual void setDrivenState( bool new_state);
//RRR  virtual void get(char *return_str, int len);

  Logic_Input (LogicGate *parent, unsigned int b, const char *opt_name=0)
    : IOPIN(opt_name), LGParent(parent), m_iobit(b)
    {
    }



};

class Logic_Output : public IO_bi_directional
{
private:
  LogicGate *LGParent;
  unsigned int m_iobit;
public:

//RRR  virtual void get(char *return_str, int len);

  Logic_Output (LogicGate *parent, unsigned int b,const char *opt_name=0)
    : IO_bi_directional(opt_name), LGParent(parent), m_iobit(b)
    {
    }

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
  unsigned int input_bit_mask;
  unsigned int input_state;
  IOPIN  **pInputPins;
  Logic_Output *pOutputPin;
#ifdef HAVE_GUI
  GdkPixbuf *pixbuf;
#endif
  LogicGate(const char *name, const char * desc);
  ~LogicGate();

  // Inheritances from the Package class
  virtual void create_iopin_map();


  virtual void update_state()=0;
  void update_input_pin(unsigned int pin, bool bValue);
  virtual int get_num_of_pins() {return number_of_pins;};
  void set_number_of_pins(int npins){number_of_pins=npins;};
#ifdef HAVE_GUI
  GtkWidget *create_pixmap(const gchar **pixmap_data);
#endif
};

// 2 input and gate

class ANDGate: public LogicGate
{
public:

  virtual void update_state();
  ANDGate(const char *name, const char * desc);
  ~ANDGate();

};

class AND2Gate: public ANDGate
{
public:

  static Module *construct(const char *new_name);
  const virtual char *type() { return ("and2"); };
  // virtual void update_state(void);
  AND2Gate(const char *);
  ~AND2Gate();

};

class ORGate: public LogicGate
{
public:

  ORGate(const char *name, const char * desc);
  ~ORGate();
  virtual void update_state();

};

// 2 input or gate

class OR2Gate: public ORGate
{
public:

  //virtual void update_state(void);
  static Module *construct(const char *new_name);
  const virtual char *type() { return ("or2"); };
  OR2Gate(const char *name);
  ~OR2Gate();

};

class XORGate: public LogicGate
{
public:

  XORGate(const char *name, const char * desc);
  ~XORGate();
  virtual void update_state();

};

// 2 input or gate

class XOR2Gate: public XORGate
{
public:

  //virtual void update_state(void);
  static Module *construct(const char *new_name);
  const virtual char *type() { return ("xor2"); };
  XOR2Gate(const char *name);
  ~XOR2Gate();

};

class NOTGate: public LogicGate
{
public:

  //virtual void update_state(void);
  static Module *construct(const char *new_name);
  const virtual char *type() { return ("not"); };
  virtual void update_state();
  NOTGate(const char *name);
  ~NOTGate();

};
#endif //  __LOGIC_H__

