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

#ifndef _WIN32
#include <dlfcn.h>
#else
#include <windows.h>
/*
 * interface is a Module class member variable in gpsim,
 * in WIN32 Platform SDK it is a macro, defined in BaseTypes.h
 * the WIN32 Platform SDK definition should be undefined
 */
#undef interface
#endif

#include "modules.h"
#include "pic-processor.h"
#include "stimuli.h"
#include "stimulus_orb.h"
#include "symbol.h"
#include "xref.h"


static int module_sequence_number = 0;


list <Module *> instantiated_modules_list;


/*****************************************************************************
 *
 * Module.cc
 *
 * Here's where much of the infrastructure of gpsim is defined.
 *
 * A Module is define to be something that gpsim knows how to simulate.
 * When gpsim was originally designed, a module was simple a pic processor.
 * This concept was expanded to accomodate devices like LEDs, switches,
 * LCDs and so on. 
 */

Module::Module(void)
{

  name_str = 0;
  widget=0;
  package = 0;
  interface = 0;

  // FIXME - remove these gui references:
  x=-1; // -1 means automatic positioning
  y=-1;

  xref = new XrefObject;

}

Module::~Module(void)
{

  symbol_table.remove_module(this,name_str);

  instantiated_modules_list.remove(this);

  delete package;
  delete xref;
}


Module * Module::construct(char * name)
{

  cout << " Can't create a generic Module\n";

  return 0;

}


//-------------------------------------------------------------------
//-------------------------------------------------------------------
void Module::new_name(const char *s)
{
  if(name_str)
    delete name_str;

  if(s)
    name_str = strdup(s);
  else
    name_str = 0;

}





//-------------------------------------------------------------------
//-------------------------------------------------------------------

void  Module::dump_attributes(int show_values)
{

  list <Attribute *> :: iterator attribute_iterator;

  cout << __FUNCTION__ << endl;

  for (attribute_iterator = attributes.begin();  
       attribute_iterator != attributes.end(); 
       attribute_iterator++) {

    Attribute *locattr = *attribute_iterator;

    cout << locattr->get_name();
    if(show_values) {

      char buf[50];

      cout << " = " << locattr->sGet(buf,50);
    }
    cout << endl;
  }


}


//-------------------------------------------------------------------
//-------------------------------------------------------------------

Attribute * Module::get_attribute(char *attribute_name)
{

  if(!attribute_name)
    return 0;

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

  cout << "Could not find attribute named " << attribute_name  << '\n';

  return 0;
}


//-------------------------------------------------------------------
//-------------------------------------------------------------------

void Module::set_attribute(char *attr, char *val)
{

  if(attr) {
    Attribute *locattr = get_attribute(attr);

    if(locattr)
      locattr->set(val);
  } else {

    // Go ahead and dump the attribute list:
    dump_attributes();
  }

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------

void Module::set_attribute(char *attr, char *val, int val2)
{

  if(attr) {
    Attribute *locattr = get_attribute(attr);

    if(locattr)
      locattr->set(val,val2);
  } else {

    // Go ahead and dump the attribute list:
    dump_attributes();
  }

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------

void Module::set_attribute(char *attr, double val)
{

  if(attr) {
    Attribute *locattr = get_attribute(attr);

    if(locattr)
      locattr->set(val);
  } else {

    // Go ahead and dump the attribute list:
    dump_attributes();

  }
 

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void Module::add_attribute(Attribute *new_attribute)
{

  attributes.push_back(new_attribute);

  cout << "add_attribute  name = " << new_attribute->get_name() << '\n';

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void Module::create_pkg(unsigned int number_of_pins)
{
  if(package)
    delete package;

  package = new Package(number_of_pins);

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void Module::assign_pin(unsigned int pin_number, IOPIN *pin)
{
  if(package)
    package->assign_pin(pin_number, pin);

}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
int Module::get_pin_count(void)
{
  if(package)
    return package->get_pin_count();

  return 0;

}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
char *Module::get_pin_name(unsigned int pin_number)
{
  if(package)
    return package->get_pin_name(pin_number);
  return 0;

}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
int Module::get_pin_state(unsigned int pin_number)
{
  if(package)
    return package->get_pin_state(pin_number);

  return 0;
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
IOPIN *Module::get_pin(unsigned int pin_number)
{
  if(package)
    return package->get_pin(pin_number);

  return 0;
}


//-------------------------------------------------------------------
//-------------------------------------------------------------------
Module_Library::Module_Library(const char *new_name, void *library_handle)
{
#ifndef _WIN32
  const char * error;

  if(new_name)
    _name = strdup(new_name);
  else
    _name = 0;

  _handle = library_handle;

  get_mod_list = (Module_Types_FPTR) dlsym(_handle, "get_mod_list");

  if ((error = dlerror()) != 0)  {
    cout << "WARNING: non-conforming module library\n"
	 << "  gpsim libraries should have the mod_list() function defined\n";
    fputs(error, stderr);
    module_list = 0;
  } else {

    // Get a pointer to the list of modules that this library supports.
    module_list = get_mod_list();

    // If the module has an "initialize" function, then call it now.
    //
    typedef  void * (*void_FPTR)(void);
    void * (*initialize)(void) = (void_FPTR) dlsym(_handle,"initialize");
    if(initialize)
      initialize();

  }
#else
//  module_list = 0;
//  _name = 0;
//  _handle = 0;

    if(new_name)
      _name = strdup(new_name);
    else
      _name = NULL;

    _handle = library_handle;

    get_mod_list = (Module_Types_FPTR) GetProcAddress((HMODULE)_handle, "get_mod_list");
    if (NULL == get_mod_list) {
      char *error = g_win32_error_message(GetLastError());

      cout << "WARNING: non-conforming module library\n"
	   << "  gpsim libraries should have the mod_list() function defined\n";
      fprintf(stderr, "%s\n", error);
      g_free(error);
      module_list = NULL;
    } else {

      // Get a pointer to the list of modules that this library supports.
      module_list = get_mod_list();
    }

#endif
}

//------------------------------------------------------------------------
// As module libraries are loaded, they're placed into the following list:
list <Module_Library *> module_list;
list <Module_Library *> :: iterator module_iterator;

//------------------------------------------------------------------------
// Each time a new module is instantiated from a module library, the 
// reference designator counter is incremented. (This will change later
// to accomodate different reference designator types).
static int  ref_des_count = 1;

void module_add_library(const char *library_name, void *library_handle)
{


  if(library_name) {

    Module_Library *ml = new Module_Library(library_name, library_handle);

    module_list.push_back(ml);

  } else 
    cout << "BUG: " << __FUNCTION__ << " called with NULL library_name";

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


void module_load_library(const char *library_name)
{
#ifndef _WIN32
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
//  cout << "  -- gpsim on WIN32 doesn't support modules yet\n";
  void *handle;

  handle = (void *)LoadLibrary(library_name);
  if (NULL == handle) {
    char *error = g_win32_error_message(GetLastError());

    fprintf (stderr, "%s\n", error);
    g_free(error);
    return;
  }

  module_add_library(library_name,handle);

  module_display_available();
#endif
}

void module_load_module(const char *module_type, const char *module_name)
{


  if(!module_type) {
    cout << "WARNING: module type is 0\n";
    return;
  }

  if(module_name==0)
  {
      char *p = (char*)malloc(128);
      sprintf(p, "%s%d",module_type,module_sequence_number);
      module_name = p;
  }

  {
    cout << "Searching for module:  " << module_type;

    if(module_name)
      cout << " named " << module_name;

//    cout  << '\n';
  }


  for (module_iterator = module_list.begin();  
       module_iterator != module_list.end(); 
       module_iterator++) {

    Module_Library *t = *module_iterator;
    if(verbose)
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

	      symbol_table.add_module(new_module,module_name);

              instantiated_modules_list.push_back(new_module);

	      // Tell the gui or any modules that are interfaced to gpsim
	      // that a new module has been added.
	      gi.new_module(new_module);


	      return;
	    }
	i++;

      }

    }
  }

  cout << " NOT FOUND\n";

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

//    cout << "Module " << mP->cpu->name() << '\n';
    return mP->cpu;
  }
  else {
    cout << "module `" << module_name << "' wasn't found\n";
    return 0;
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
// Set module attribute

void module_set_attr(char *module_name,char *attr, char *val)
{

  Module *cpu=module_check_cpu(module_name);


  if(!cpu)
    return;

  cpu->set_attribute(attr,val);

}

//--------------------------------------------------
// module_set_attr
// Set module attribute

void module_set_attr(char *module_name,char *attr, char *val, int val2)
{

  Module *cpu=module_check_cpu(module_name);


  if(!cpu)
    return;

  cpu->set_attribute(attr,val,val2);

}
//--------------------------------------------------
// module_set_attr
// Set module attribute

void module_set_attr(char *module_name,char *attr, double val)
{

  Module *cpu=module_check_cpu(module_name);

  if(!cpu)
    return;

  cpu->set_attribute(attr,val);

}


void module_set_position(char *module_name, int x, int y)
{

  Module *cpu=module_check_cpu(module_name);

  if(!cpu)
    return;

  cpu->x=x;
  cpu->y=y;
  if(cpu->xref)
      cpu->xref->update();
}
