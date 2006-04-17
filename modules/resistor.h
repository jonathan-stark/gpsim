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

/* IN_MODULE should be defined for modules */
#define IN_MODULE

#ifdef HAVE_GUI
#include <gtk/gtk.h>
#endif


#include <glib.h>

#include "../src/modules.h"
#include "../src/packages.h"
#include "../src/stimuli.h"
#include "../src/symbol.h"
#include "../src/trace.h"
#include "../src/ioports.h"

# if 0
class Resistor;
class Resistor_IOPORT : public IOPORT
{
public:

  virtual void trace_register_write(void);

  Resistor_IOPORT (unsigned int _num_iopins=2) : IOPORT(_num_iopins) {}

};



class Resistor : public Module 
{
public:

  Resistor_IOPORT  *port;

  Resistor(void);
  ~Resistor(void);

  // Inheritances from the Package class
  virtual void create_iopin_map(void);

  static Module *construct(const char *new_name=NULL);

};
#endif

class PullupResistor : public Module 
{
public:

  IO_bi_directional_pu res;

#ifdef MANAGING_GUI

  GtkWidget *pu_window;

#endif

  PullupResistor(const char *init_name=NULL);
  ~PullupResistor();

  // Inheritances from the Package class
  virtual void create_iopin_map(void);

  static Module *pu_construct(const char *new_name=NULL);
  static Module *pd_construct(const char *new_name=NULL);

#ifdef MANAGING_GUI
  void build_window(void);
#endif

};
#endif //  __RESISTOR_H__
