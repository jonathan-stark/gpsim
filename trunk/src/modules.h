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
#include "packages.h"
#include "attribute.h"


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

/*****************************************************************************
 *
 * Helper functions
 *
 *****************************************************************************/
typedef  Module * (*Module_FPTR)();
typedef  Module_Types * (*Module_Types_FPTR)();

//----------------------------------------------------------
// An instance of the Module_Library class is created each
// time a library of modules is opened.

class Module_Library {
  char *_name;
  void *_handle;
  Module_Types * (*get_mod_list)(void);

public:

  Module_Types *module_list;

  Module_Library(const char *new_name, void *library_handle);

  ~Module_Library(void) {
    if(_name)
      delete _name;
  };

  char *name(void) {
    return(_name);
  }

  void *handle(void) {
    return _handle;
  }

};

extern list <Module_Library *> module_list;

extern list <Module *> instantiated_modules_list;

class Module {
public:

  char * name_str;                // A unique name to describe the Module
  list<Attribute *> attributes;   // A list of attributes that pertain to the Module
  Package  *package;              // A package for the module
  ModuleInterface *interface;     // An interface to the module.

  unsigned int interface_id;

  void *widget; // GtkWidget * that is put in the breadboard.

  int x,y;
  XrefObject *xref; // Updated when the module changes position

  // I/O pin specific

  virtual int get_pin_count(void){return 0;};
  virtual char *get_pin_name(unsigned int pin_number) {return NULL;};
  virtual int get_pin_state(unsigned int pin_number) {return 0;};
  virtual IOPIN *get_pin(unsigned int pin_number) {return NULL;};

  void add_attribute(Attribute *);

  virtual void set_attribute(char *attr, char *val, int val2);
  virtual void set_attribute(char *attr, char *val);
  virtual void set_attribute(char *attr, double val);

  virtual Attribute *get_attribute(char *attr);

  virtual void dump_attributes(int show_values=1);
  const virtual char *type(void) { return (name_str); };
  char *name(void) {return name_str;};
  virtual void new_name(const char *);

  static Module *construct(char *name);
  Module(void);
  virtual ~Module();

};


/**************************************************************
* External Modules
*  The class for external modules is a combination of the internal
* module class and the package class. 
*
*/
class ExternalModule : public Module, public Package
{
 public:

  // Define a set of virtual functions to redirect overloaded functions
  // to the proper place. There probably is a better way to do this...

  virtual int get_pin_count(void)
    {
      return Package::get_pin_count();
    };

  virtual char *get_pin_name(unsigned int pin_number) 
    {
      return Package::get_pin_name(pin_number);
    };

  virtual int get_pin_state(unsigned int pin_number) 
    {
      return Package::get_pin_state(pin_number);
    };

  virtual IOPIN *get_pin(unsigned int pin_number) 
    {
      return Package::get_pin(pin_number);
    };

};
class Module_Types
{
public:

  char *names[2];
  ExternalModule * (*module_constructor) (const char *module_name);
};


//--------------------------------------
//
// non-class helper functions.

void module_display_available(void);
void module_list_modules(void);
//int find_in_available_module_list(char * module_type);
void module_load_library(const char *library_name);
void module_load_module(const char * module_type, const char * module_new_name=NULL);
void module_pins(char *module_name);
void module_set_attr(char *module_name,char *attr, char *val);
void module_set_attr(char *module_name,char *attr, char *val, int val2);
void module_set_attr(char *module_name,char *attr, double val);
void module_set_position(char *module_name, int x, int y);
#endif // __MODULES_H__
