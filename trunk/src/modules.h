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


/*
  modules.h

  The base class for modules is defined here.

  Include this file into yours for creating custom modules.
 */


#ifndef __MODULES_H__
#define __MODULES_H__

#include "gpsim_classes.h"



/*

port
 pin
  type
  number
  position in port

struct Module_Pin {
 char *port_name,
 char *pin_name,
 PIN_TYPE pin_type,
 int  pin_number
 int  pin_position
} 
*/


class Module {
public:

  char * name_str;


  unsigned int interface_id;

  // I/O pin specific

  virtual int get_pin_count(void){return 0;};
  virtual char *get_pin_name(unsigned int pin_number) {return NULL;};
  virtual int get_pin_state(unsigned int pin_number) {return 0;};
  virtual IOPIN *get_pin(unsigned int pin_number) {return NULL;};


  const virtual char *type(void) { return (name_str); };
  char *name(void) {return name_str;};
  virtual void new_name(char *);

  static Module *construct(char *name);
  Module(void);

};


class Module_Types
{
public:

  char *names[2];
  Module * (*module_constructor) (char *module_name=NULL);
};


//--------------------------------------
//
// non-class helper functions.

void module_display_available(void);
void module_list_modules(void);
//int find_in_available_module_list(char * module_type);
void module_load_library(char *library_name);
void module_load_module(char * module_type, char * module_new_name=NULL);
void module_pins(char *module_name);
#endif // __MODULES_H__
