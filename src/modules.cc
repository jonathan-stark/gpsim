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


#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string>
#include <list>
#include <vector>

#include "../config.h"

#ifdef HAVE_GUI
#include <dlfcn.h>
#endif

#include "modules.h"
#include "pic-processor.h"
#include "stimuli.h"
#include "stimulus_orb.h"
#include "symbol.h"
#include "xref.h"


static int module_sequence_number = 0;

/*****************************************************************************
 *
 * Module.cc
 *
 * Here's where much of the infrastructure of gpsim is defined.
 *
 * A Module is define to be something that gpsim knows how to simulate.
 * When gpsim was originally designed, a module was simple a pic processor.
 * This concept was expanded to accomodate addition devices like LEDs, switches,
 * LCDs and so on. 
 */

Module::Module(void)
{

  name_str = NULL;
  interface_id = 0;

}


Module * Module::construct(char * name)
{

  cout << " Can't create a generic Module\n";

  return NULL;

}

#if 0
void Module::create_pkg(unsigned int _number_of_pins)
{
  if(number_of_pins)
    {
      cout << "error: Module::create_pkg. Module appears to already exist.\n";
      return;
    }


  number_of_pins = _number_of_pins;

  pins = (IOPIN **) new char[sizeof( IOPIN *) * number_of_pins];

  for(int i=0; i<number_of_pins; i++)
    pins[i] = NULL;

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
int Module::pin_existance(unsigned int pin_number)
{

  if(!number_of_pins)
    {
      cout << "error: Module::assign_pin. No module.\n";
      return E_NO_PACKAGE;
    }


  if((pin_number > number_of_pins) || (pin_number == 0))
    {
      cout << "error: Module::assign_pin. Pin number is out of range.\n";
      cout << "Max pins " << number_of_pins << ". Trying to add " << pin_number <<".\n";
      return E_PIN_OUT_OF_RANGE;
    }

  if(pins[pin_number-1])
    return E_PIN_EXISTS;
  
  return E_NO_PIN;

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
IOPIN *Module::get_pin(unsigned int pin_number)
{

  if(E_PIN_EXISTS == pin_existance(pin_number))
    return pins[pin_number-1];
  else
    return NULL;

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void Module::assign_pin(unsigned int pin_number, IOPIN *pin)
{

  switch(pin_existance(pin_number)) {

  case E_PIN_EXISTS:
    if(pins[pin_number-1])
      cout << "warning: Module::assign_pin. Pin number " << pin_number << " already exists.\n";

  case E_NO_PIN:
    pins[pin_number-1] = pin;
    // Tell the I/O pin its number
    //pin->new_pin_number(pin_number);
    break;

  }


}

void Module::create_iopin_map(void)
{

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------

char *Module::get_pin_name(unsigned int pin_number)
{

  if(pin_existance(pin_number) == E_PIN_EXISTS)
    return pins[pin_number-1]->name();
  else
    return NULL;

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
int Module::get_pin_state(unsigned int pin_number)
{

  if(pin_existance(pin_number) == E_PIN_EXISTS)
    return pins[pin_number-1]->get_voltage(0);
  else
    return 0;

}

#endif

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void Module::new_name(char *s)
{
  if(name_str)
    delete name_str;

  if(s)
    name_str = strdup(s);
  else
    name_str = NULL;

}




Attribute * Module::get_attribute(char *attribute_name)
{

  list <Attribute *> :: iterator attribute_iterator;

  for (attribute_iterator = attributes.begin();  
       attribute_iterator != attributes.end(); 
       attribute_iterator++) {

    Attribute *locattr = *attribute_iterator;

    if(attribute_name) {
      if(strcmp(locattr->get_name(), attribute_name) == 0) {
	cout << "Found attribute: " << locattr->get_name() << '\n';

	return locattr;
      }
    } else {
      char buf[50];

      cout << locattr->get_name() << " = " << locattr->sGet(buf,50) << '\n';
    }
  }

  if(attribute_name)
    cout << "Could not find attribute named " << attribute_name  << '\n';

  return NULL;
}


//-------------------------------------------------------------------
//-------------------------------------------------------------------

void Module::set_attribute(char *attr, char *val)
{

  Attribute *locattr = get_attribute(attr);

  if(locattr)
    locattr->set(val);

  //cout << " warning " << name_str << " does not have any changable attributes\n";
/*
  if(!attr)
    return;

  list <Attribute *> :: iterator attribute_iterator;

  for (attribute_iterator = attributes.begin();  
       attribute_iterator != attributes.end(); 
       attribute_iterator++) {

    Attribute *locattr = *attribute_iterator;

    if(strcmp(locattr->get_name(), attr) == 0) {
      cout << "Found attribute: " << locattr->get_name() << '\n';

      locattr->set(val);

      return;
    }
  }

  cout << "Could not find (attr = " << attr << " , val = " << val << ")\n";
*/
}

void Module::set_attribute(char *attr, double val)
{

  Attribute *locattr = get_attribute(attr);

  if(locattr)
    locattr->set(val);

  // cout << " warning " << name_str << " does not have any changable attributes\n";
/*
  if(!attr)
    return;

  list <Attribute *> :: iterator attribute_iterator;

  for (attribute_iterator = attributes.begin();  
       attribute_iterator != attributes.end(); 
       attribute_iterator++) {

    Attribute *locattr = *attribute_iterator;

    if(strcmp(locattr->get_name(), attr) == 0) {
      cout << "Found attribute: " << locattr->get_name() << '\n';

      locattr->set(val);

      return;
    }
  }

  cout << "Could not find (attr = " << attr << " , val = " << val << ")\n";
*/
}

void Module::add_attribute(Attribute *new_attribute)
{

  attributes.push_back(new_attribute);

  cout << "add_attribute  name = " << new_attribute->get_name() << '\n';

}

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

  Module_Library(char *new_name, void *library_handle) {
    #ifdef HAVE_GUI
    const char * error;

    if(new_name)
      _name = strdup(new_name);
    else
      _name = NULL;

    _handle = library_handle;

    get_mod_list =   (Module_Types_FPTR) dlsym(_handle, "get_mod_list");

    if ((error = dlerror()) != NULL)  {
      cout << "WARNING: non-conforming module library\n"
	   << "  gpsim libraries should have the mod_list() function defined\n";
      fputs(error, stderr);
      module_list = NULL;
    } else {

      // Get a pointer to the list of modules that this library supports.
      module_list = get_mod_list();
    }
    #else

    module_list = NULL;
    _name = NULL;
    _handle = NULL;

    #endif

  };

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

//------------------------------------------------------------------------
// As module libraries are loaded, they're placed into the following list:
list <Module_Library *> module_list;
list <Module_Library *> :: iterator module_iterator;

//------------------------------------------------------------------------
// Each time a new module is instantiated from a module library, the 
// reference designator counter is incremented. (This will change later
// to accomodate different reference designator types).
static int  ref_des_count = 1;

void module_add_library(char *library_name, void *library_handle)
{


  if(library_name) {

    Module_Library *ml = new Module_Library(library_name, library_handle);

    module_list.push_back(ml);

  } else 
    cout << "BUG: " << __FUNCTION__ << " called with library_name == NULL";

}

// dump_available_libraries
// ...

void module_display_available(void)
{

  cout << "Module Libraries\n";
  

  for (module_iterator = module_list.begin();  
       module_iterator != module_list.end(); 
       module_iterator++) {

    Module_Library *t = *module_iterator;
    cout << t->name() << '\n';

    if(t->module_list) {
      // Loop through the list and display all of the modules.
      int i=0;

      while(t->module_list[i].names[0]) {
	cout << "   " << t->module_list[i++].names[0] << '\n';
      }
    }
  }
}


void module_load_library(char *library_name)
{

#ifdef HAVE_GUI

  void *handle;
  char *error;

  handle = dlopen (library_name, RTLD_LAZY);
  if (!handle) {

    fputs (dlerror(), stderr);
    return;
  }

  module_add_library(library_name,handle);

  module_display_available();
#else

  cout << "  -- gpsim doesn't support modules in the cli-only mode yet\n";

#endif

}

void module_load_module(char *module_type, char *module_name=NULL)
{


  if(!module_type) {
    cout << "WARNING: module type is NULL\n";
    return;
  }

  if(module_name==NULL)
  {
      module_name = (char*)malloc(128);
      sprintf(module_name, "%s%d",module_type,module_sequence_number);
  }

  {
    cout << "Searching for module:  " << module_type;

    if(module_name)
      cout << " named " << module_name;

    cout  << '\n';
  }


  for (module_iterator = module_list.begin();  
       module_iterator != module_list.end(); 
       module_iterator++) {

    Module_Library *t = *module_iterator;
    cout << t->name() << '\n';

    if(t->module_list) {
      // Loop through the list and search for the module.
      int i=0,j;

      while(t->module_list[i].names[0]) {

	// Look at all of the possible names for a module

	for(j=0; j<2; j++)
	  if(strcmp(module_type,t->module_list[i].names[j]) == 0)
	    {
	      // We found a module that matches.
	      // Now, let's create an instance of it and throw it into
	      // the symbol library

	      cout << " Found it!\n";
	      Module *new_module = t->module_list[i].module_constructor(module_name);
	      new_module->interface_id = ++module_sequence_number;  // fixme make this a member function.
	      symbol_table.add_module(new_module,module_name);

	      // Tell the gui or any modules that are interfaced to gpsim
	      // that a new module has been added.
	      gi.new_module(new_module);


	      return;
	    }
	i++;

      }

    }
  }

  cout << "NOT FOUND\n";

}

//--------------------------------------------------
// Print out all of the symbols that are of type 'SYMBOL_MODULE'

void module_list_modules(void)
{

 symbol_table.dump_type( SYMBOL_MODULE);
}

//--------------------------------------------------
// Module *module_check_cpu(symbol *mP)
//
// Given a pointer to a symbol, check to see if the cpu is valid.
//

Module *module_check_cpu(char *module_name)
{

  symbol *mP = symbol_table.find(SYMBOL_MODULE,module_name);

  if(mP && mP->cpu) {

    cout << "Module " << mP->cpu->name() << '\n';
    return mP->cpu;
  }
  else {
    cout << "module `" << module_name << "' wasn't found\n";
    return NULL;
  }

}

//--------------------------------------------------
// module_pins
// Display the states of the pins of a module

void module_pins(char *module_name)
{

  Module *cpu=module_check_cpu(module_name);

  if(!cpu)
    return;

  for(int i=1; i<=cpu->get_pin_count(); i++) {

    cout << " Pin number " << i << " named " << cpu->get_pin_name(i) 
	 << " is " << ( (cpu->get_pin_state(i)>0) ? "high\n" : "low\n");
  }

}

//--------------------------------------------------
// module_set_attr
// Display the states of the pins of a module

void module_set_attr(char *module_name,char *attr, char *val)
{

  Module *cpu=module_check_cpu(module_name);


  if(!cpu)
    return;

  cpu->set_attribute(attr,val);

}
//--------------------------------------------------
// module_set_attr
// Display the states of the pins of a module

void module_set_attr(char *module_name,char *attr, double val)
{

  Module *cpu=module_check_cpu(module_name);

  if(!cpu)
    return;

  cpu->set_attribute(attr,val);

}
