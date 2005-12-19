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


#include "modules.h"

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
#if !defined(_MAX_PATH)
  #define _MAX_PATH 1024
#endif
#include <unistd.h>  // for getcwd 

#else
#include <direct.h>
#include <windows.h>
/*
 * interface is a Module class member variable in gpsim,
 * in WIN32 Platform SDK it is a macro, defined in BaseTypes.h
 * the WIN32 Platform SDK definition should be undefined
 */
#undef interface
#endif

#include "pic-processor.h"
#include "stimuli.h"
#include "stimulus_orb.h"
#include "symbol.h"
#include "xref.h"
#include "value.h"
#include "packages.h"
#include "cmd_manager.h"

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

  package = 0;
  interface = 0;

  // Derived modules should assign more reasonable values for this.
  version = 0;

  xref = new XrefObject;

  simulation_mode = eSM_STOPPED;
  widget = 0;
}

Module::~Module(void)
{

  symbol_table.remove_module(this);

  instantiated_modules_list.remove(this);

  delete package;
  delete xref;
}


Module * Module::construct(char * name)
{

  cout << " Can't create a generic Module\n";

  return 0;

}

void Module::reset(RESET_TYPE r)
{
  cout << " resetting module " << name() << endl;
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------

void  Module::dump_attributes(int show_values)
{

  list <Value *> :: iterator attribute_iterator;

  for (attribute_iterator = attributes.begin();  
       attribute_iterator != attributes.end(); 
       attribute_iterator++) {

    Value *locattr = *attribute_iterator;

    cout << locattr->name();
    if(show_values)
      cout << " = " << locattr->toString();
    cout << endl;
  }


}


//-------------------------------------------------------------------
//-------------------------------------------------------------------

Value *Module::get_attribute(char *attribute_name, bool bWarnIfNotFound)
{
  if(!attribute_name)
    return 0;

  string full_name = name() + "." + attribute_name;
  list <Value *> :: iterator attribute_iterator;

  for (attribute_iterator = attributes.begin();  
       attribute_iterator != attributes.end(); 
       attribute_iterator++) {

    Value *locattr = *attribute_iterator;

    if (locattr->name() == full_name)
      return locattr;
  }

  if (bWarnIfNotFound)
    cout << "Could not find attribute named " << attribute_name  << '\n';

  return 0;
}


void Module::set(const char *cP,int len)
{
  cout << "Module:" <<name() << " doesn't support set()\n"; 
}

void Module::get(char *pBuf, int len)
{
  if(pBuf != 0) {
    *pBuf = 0;
  }
  // cout << "Module:" <<name() << " doesn't support get()\n"; 
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void Module::initializeAttributes()
{
  // Create position attribute place holders if we're not using the gui
  if(!get_interface().bUsingGUI()) {
    add_attribute(new Float("xpos",80.0));
    add_attribute(new Float("ypos",80.0));
  }
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
void Module::add_attribute(Value *new_attribute)
{

  attributes.push_back(new_attribute);

  symbol_table.add(new attribute_symbol(this,new_attribute));

  if(verbose)
    cout << "add_attribute  name = " << new_attribute->name() << '\n';

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
string &Module::get_pin_name(unsigned int pin_number)
{
  static string invalid("");
  if(package)
    return package->get_pin_name(pin_number);
  return invalid;  //FIXME

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
// Module Scripts
//
// Module command line scripts are named scripts created by symbol 
// files. For example, with PIC cod files, it's possible to
// create assertions and simulation commands using the '.assert'
// and '.sim' directives. These commands are ASCII strings that
// are collected together. 
//

//-------------------------------------------------------------------
// Module::add_command
//
// Add a command line command to a Module Script.
//-------------------------------------------------------------------
void Module::add_command(string &script_name, string &command)
{
  ModuleScript *script = m_scripts[script_name];
  if (!script) {
    script = new ModuleScript(script_name);
    m_scripts[script_name] = script;
  }

  script->add_command(command);
}

//-------------------------------------------------------------------
// Module::run_script - execute a gpsim command line script
// 
//-------------------------------------------------------------------
void Module::run_script(string &script_name)
{
  ModuleScript *script = m_scripts[script_name];
  if (script) {
    ICommandHandler *pCli = CCommandManager::GetManager().find("gpsimCLI");
    script->run(pCli);
  }
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
Module::ModuleScript::ModuleScript(string &name_)
  : name(name_)
{
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
Module::ModuleScript::~ModuleScript()
{
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
void Module::ModuleScript::add_command(string &command)
{
  string *new_command = new string(command);
  m_commands.push_back(new_command);
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void Module::ModuleScript::run(ICommandHandler *pCommandHandler)
{
  if (!pCommandHandler)
    return;

  pCommandHandler->ExecuteScript(m_commands,0);
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
void Module::ModuleScript::concatenate(ModuleScript *pOtherScript)
{
  if (!pOtherScript)
    return;

  list <string *> :: iterator command_iterator;

  for (command_iterator = pOtherScript->m_commands.begin();
       command_iterator != pOtherScript->m_commands.end(); 
       ++command_iterator)
    m_commands.push_back(*command_iterator);
}
//-------------------------------------------------------------------
//-------------------------------------------------------------------
Module_Library::Module_Library(const char *new_name, void *library_handle)
{
  char * error;

  if(new_name)
    _name = strdup(new_name);
  else
    _name = 0;

  _handle = library_handle;

  get_mod_list = (Module_Types_FPTR)get_library_export(
    "get_mod_list", _handle, &error);

  if (NULL == get_mod_list) {
    cout << "WARNING: non-conforming module library\n"
	 << "  gpsim libraries should have the get_mod_list() function defined\n";
    fputs(error, stderr);
    fputs ("\n", stderr);
    module_list = 0;
  } else {

    // Get a pointer to the list of modules that this library supports.
    module_list = get_mod_list();

    if(!module_list)
      cout << "no modules were found in " << name() << endl;

    // If the module has an "initialize" function, then call it now.
    typedef  void * (*void_FPTR)(void);
    void * (*initialize)(void) = (void_FPTR)get_library_export(
      "initialize", _handle, NULL);
    if(initialize)
      initialize();
  }
}

Module_Library::~Module_Library(void) {
  if(_handle) {
    free_library(_handle);
  }
  if(_name)
    delete _name;
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
ICommandHandler *Module_Library::GetCli() {

  PFNGETCOMMANDHANDLER pGetCli = (PFNGETCOMMANDHANDLER)get_library_export(
    GPSIM_GETCOMMANDHANDLER, _handle, NULL);
  if (pGetCli != NULL)
    return pGetCli();
  return NULL;
}


//------------------------------------------------------------------------
// As module libraries are loaded, they're placed into the following list:
typedef list <Module_Library *> ModuleLibraryList;
ModuleLibraryList module_list;
ModuleLibraryList :: iterator module_iterator;

bool ModuleLibraryExists(string sName) {
  ModuleLibraryList::iterator it;
  ModuleLibraryList::iterator itEnd = module_list.end();
  for(it = module_list.begin(); it != itEnd; it++) {
    if(sName.compare((*it)->name()) == 0) {
      return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------
// Each time a new module is instantiated from a module library, the 
// reference designator counter is incremented. (This will change later
// to accomodate different reference designator types).
//static int  ref_des_count = 1;

void module_canonical_name(string &sPath, string &sName) {
  GetFileName(sPath, sName);
#ifdef _WIN32
  for(unsigned int i = 0; i < sName.size(); i++) {
    sName[i] = ::toupper(sName[i]);
  }
#endif
}

static bool module_add_library(const char *library_name, void *library_handle)
{

  if(library_name) {
    string sName(library_name);
    module_canonical_name(sName, sName);
    Module_Library *ml = new Module_Library(sName.c_str(), library_handle);
    module_list.push_back(ml);
    return true;
  } else 
    cout << "BUG: " << __FUNCTION__ << " called with NULL library_name";
  return false;
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

bool module_load_library(const char *library_name)
{
  void *handle;
  char *pszError;
  bool bReturn = false;

  string sPath(library_name);
  FixupLibraryName(sPath);
  string sName;
  module_canonical_name(sPath, sName);
  if(!ModuleLibraryExists(sName)) {
    if ((handle = load_library(sPath.c_str(), &pszError)) == NULL) {
      char cw[_MAX_PATH];
  //    GetCurrentDirectory(_MAX_PATH, cw);
      getcwd(cw, _MAX_PATH);
      //fprintf(stderr, "%s in module_load_library(%s)\n", pszError, library_name);
      cerr << "failed to open library module ";
      cerr << sPath;
      cerr << ": ";
      cerr << pszError;
      cerr << endl;
      cerr << "current working directory is ";
      cerr << cw;
      cerr << endl;
      free_error_message(pszError);
    }
    else {

      if(module_add_library(sPath.c_str(),handle)) {
        bReturn = true;
      }
    }
  }
  if(verbose)
    module_display_available();
  return bReturn;
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
    if(verbose) {
      cout << "Searching for module:  " << module_type;

      if(module_name)
	cout << " named " << module_name;
      cout  << '\n';
    }
  }


  for (module_iterator = module_list.begin();  
       module_iterator != module_list.end(); 
       ++module_iterator) {

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
	      if(verbose)
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

  
  cout << "Warning: Module '" << module_type <<  "' was not found\n";

}

Module_Library * module_get_library(const char* name) {
  string sPath(name);
  FixupLibraryName(sPath);
  string sName;
  module_canonical_name(sPath, sName);

  for (module_iterator = module_list.begin();  
       module_iterator != module_list.end(); 
       ++module_iterator) {

    Module_Library *t = *module_iterator;
    if (strcmp(t->name(), sName.c_str()) == 0)
      return t;
  }
  return NULL;
}

void module_free_library(const char* name) {
  string sPath(name);
  FixupLibraryName(sPath);
  string sName;
  module_canonical_name(sPath, sName);
  for (module_iterator = module_list.begin();  
       module_iterator != module_list.end(); 
       ++module_iterator) {

    Module_Library *t = *module_iterator;
    if (strcmp(t->name(), sName.c_str()) == 0) {
      module_list.erase(module_iterator);
      delete t;
    }
  }
}

ICommandHandler * module_get_command_handler(const char *name) {
  Module_Library * lib = module_get_library(name);
  if (lib != NULL)
    return lib->GetCli();
  return NULL;
}

//------------------------------------------------------------------------
void module_reset_all(RESET_TYPE r)
{
  list <Module *> :: iterator mi;
  //list <Module *> instantiated_modules_list;

  for (mi = instantiated_modules_list.begin();  
       mi != instantiated_modules_list.end(); 
       ++mi)
    (*mi)->reset(r);

}
//--------------------------------------------------
// Print out all of the symbols that are of type 'SYMBOL_MODULE'

void module_list_modules(void)
{

 symbol_table.dump_type( typeid(module_symbol));
}

//--------------------------------------------------
// Module *module_check_cpu(symbol *mP)
//
// Given a pointer to a symbol, check to see if the cpu is valid.
//

Module *module_check_cpu(char *module_name)
{

  Value *mP = symbol_table.find(typeid(module_symbol),module_name);

  module_symbol *ms = dynamic_cast<module_symbol *>(mP);

  if(ms)
    return ms->get_module();
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
// module_update
// calls the update method of the xref object. 
// The purpose is (most probably) to tell the gui that
// a module needs to be updated in some way (e.g. an 
// attribute may've changed and the gui needs to reflect
// this).

void module_update(char *module_name)
{

  Module *cpu=module_check_cpu(module_name);

  if(cpu && cpu->xref)
    cpu->xref->_update();

}
