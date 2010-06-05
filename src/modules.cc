/*
   Copyright (C) 1998,1999,2000 T. Scott Dattalo

This file is part of the libgpsim library of gpsim

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


#include "modules.h"

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string>
#include <list>
#include <vector>
#include <typeinfo>

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
#include "symbol.h"
#include "xref.h"
#include "value.h"
#include "packages.h"
#include "cmd_manager.h"


// When a new library is loaded, all of the module types
// it supports are placed into the ModuleTypes map. This
// object is private to this file.

typedef map<string, Module_Types *> ModuleTypeInfo_t;
ModuleTypeInfo_t ModuleTypes;


//------------------------------------------------------------------------
// Add a new type to the ModuleTypes map if the name for that type
// does not exist already.

static void AddModuleType(const char *pName, Module_Types *pType)
{
  string sName(pName);
  ModuleTypeInfo_t::iterator mti = ModuleTypes.find(sName);
  if (mti == ModuleTypes.end())
    ModuleTypes[sName] = pType;
}

DynamicModuleLibraryInfo::DynamicModuleLibraryInfo(string &sCanonicalName,
                           string &sUserSuppliedName,
                           void   *pHandle)
    : m_sCanonicalName(sCanonicalName),
      m_sUserSuppliedName(sUserSuppliedName),
      m_pHandle(pHandle),
      get_mod_list(0)
{
    const char * error;
    if (m_pHandle)
      get_mod_list = (Module_Types_FPTR)get_library_export("get_mod_list", m_pHandle, &error);

    if (!get_mod_list) {
      cout << "WARNING: non-conforming module library\n"
           << "  gpsim libraries should have the get_mod_list() function defined\n";
      fprintf(stderr, "%s\n",error);
      free_error_message(error);
    } else {

      // Get a pointer to the list of modules that this library file supports.
      Module_Types *pLibModList = get_mod_list();

      // Loop through the list of modules supported by the library and an entry
      // ModuleTypes map for each one.

      if(pLibModList)
        for(Module_Types *pModTypes = pLibModList;  pModTypes->names[0]; pModTypes++) {
          AddModuleType(pModTypes->names[0], pModTypes);
          AddModuleType(pModTypes->names[1], pModTypes);
        }

      // If the module has an "initialize" function, then call it now.
      typedef  void * (*void_FPTR)(void);
      void * (*initialize)(void) = (void_FPTR)get_library_export("initialize", m_pHandle, NULL);
      if(initialize)
        initialize();

      /*
      ICommandHandler * pCliHandler = ml->GetCli();
      if (pCliHandler != NULL)
        CCommandManager::GetManager().Register(pCliHandler);
      */
    }

}

ModuleLibraries_t ModuleLibraries;


//========================================================================

void MakeCanonicalName(string &sPath, string &sName) {
#ifdef _WIN32
  sName = sPath;
#else
  GetFileName(sPath, sName);
#endif
}

int ModuleLibrary::LoadFile(string &fName)
{
  void *handle;
  const char *pszError;
  bool bReturn = false;

  string sPath= fName;
  FixupLibraryName(sPath);
  string sName;
  MakeCanonicalName(sPath, sName);
  ModuleLibraries_t::iterator mli = ModuleLibraries.find(sName);

  if (mli == ModuleLibraries.end()) {
    if ((handle = ::load_library(sPath.c_str(), &pszError)) == NULL) {
#ifdef THROW
      ostringstream stream;

      stream << "failed to open library module ";
      stream << sPath;
      stream << ": ";
      stream << pszError;
      stream << endl;

      char cw[_MAX_PATH];
      getcwd(cw, sizeof(cw));
      stream << "current working directory is ";
      stream << cw;
      stream << endl << ends;
      free_error_message(pszError);
      throw new Error(stream.str());
#endif //THROW
    }
    else {

      ModuleLibraries[sName] = new DynamicModuleLibraryInfo(fName, sName, handle);
      bReturn = true;
    }
  }
  /*
  if(verbose)
    DisplayFileList();
  */
  return bReturn;

}
int ModuleLibrary::InstantiateObject(string &sObjectName, string &sInstantiatedName)
{

  ModuleTypeInfo_t::iterator mti = ModuleTypes.find(sObjectName);
  if (mti != ModuleTypes.end()) {

    Module *pModule = mti->second->module_constructor(sInstantiatedName.c_str());
    pModule->set_module_type(sObjectName);
    globalSymbolTable().addModule(pModule);

    // Tell the gui or any modules that are interfaced to gpsim
    // that a new module has been added.
    gi.new_module(pModule);
    return pModule!=0;
  }
  return 0;

}

void ModuleLibrary::ListLoadableModules()
{
  ModuleTypeInfo_t::iterator mti = ModuleTypes.begin();
  while (mti != ModuleTypes.end()) {

    cout << " " << mti->first << endl;
    ++mti;
  }

}

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

Module::Module(const char *_name, const char *desc)
  : gpsimObject(_name, desc)
{

  package = 0;
  interface = 0;

  // Derived modules should assign more reasonable values for this.
  version = 0;

  xref = new XrefObject;

  simulation_mode = eSM_STOPPED;
  widget = 0;

  if (_name) {
    // If there is a module symbol already with this
    // name, then print a warning before deleting.

    gpsimObject *pOldModule = globalSymbolTable().find(name());
    if (pOldModule) {
      cout << "Warning: There already is a symbol in the symbol table named " << _name << endl;
      return;
    }
  }
  globalSymbolTable().addModule(this);

  // Create position attribute place holders if we're not using the gui
  if(!get_interface().bUsingGUI()) {
    addSymbol(new Float("xpos",80.0));
    addSymbol(new Float("ypos",80.0));
  }

}

#if 0 // warning: 'void dumpOneSymbol(const SymbolEntry_t&)' defined but not used
static void dumpOneSymbol(const SymbolEntry_t &sym)
{
  cout << "  " << sym.second
       << " stored as " << sym.first
       << endl;
}
#endif


Module::~Module(void)
{
  map<string ,ModuleScript *>::iterator si;

  for (si = m_scripts.begin();
       si != m_scripts.end();
       ++si) {
    ModuleScript *pMS = (*si).second;
    delete pMS;
  }
  m_scripts.clear();

  deleteSymbol("xpos");
  deleteSymbol("ypos");

  /*
  cout << "Stuff still in the symbol table:\n";
  mSymbolTable.ForEachSymbolTable(dumpOneSymbol);
  */
  delete package;
  delete xref;

  globalSymbolTable().removeModule(this);

}

void Module::reset(RESET_TYPE r)
{
  cout << " resetting module " << name() << endl;
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
int Module::addSymbol(gpsimObject *pSym, string *ps_AliasedName)
{
  return mSymbolTable.addSymbol(pSym, ps_AliasedName);
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
int Module::removeSymbol(gpsimObject *pSym)
{
  return mSymbolTable.removeSymbol(pSym);
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
int Module::removeSymbol(const string &s)
{
  return mSymbolTable.removeSymbol(s);
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
int Module::deleteSymbol(const string &s)
{
  return mSymbolTable.deleteSymbol(s);
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
int Module::deleteSymbol(gpsimObject *pSym)
{
  if (!pSym)
    return 0;

  if (!removeSymbol(pSym))
    return 0;

  delete pSym;
  return 1;
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------
gpsimObject *Module::findSymbol(const string &searchString)
{
  return mSymbolTable.findSymbol(searchString);
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
    if(pCli) {
      script->run(pCli);
    }
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
  list <string *> :: iterator command_iterator;

  for (command_iterator = m_commands.begin();
       command_iterator != m_commands.end();
       ++command_iterator)
    delete *command_iterator;

  m_commands.clear();
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
