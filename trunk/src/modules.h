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

#include <list>
#include <string>
#include <map>

#include "gpsim_object.h"
#include "gpsim_classes.h"

class Module;
class Module_Types;
class ModuleInterface;
class IOPIN;
class XrefObject;
class Value;
class Package;
class ICommandHandler;

typedef  Module * (*Module_FPTR)();
typedef  Module_Types * (*Module_Types_FPTR)();

enum SIMULATION_MODES
{
  eSM_INITIAL,
  eSM_STOPPED,
  eSM_RUNNING,
  eSM_SLEEPING,
  eSM_SINGLE_STEPPING,
  eSM_STEPPING_OVER,
  eSM_RUNNING_OVER
};


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

  ICommandHandler *GetCli();
};

ICommandHandler * module_get_command_handler(const char *name);

extern list <Module_Library *> module_list;

extern list <Module *> instantiated_modules_list;


//------------------------------------------------------------------------
//
/// Module - Base class for all gpsim behavior models. 

class Module : public gpsimObject {
public:

  list<Value *> attributes;         // A list of attributes that pertain to the Module
  Package  *package;                // A package for the module
  ModuleInterface *interface;       // An interface to the module.
  SIMULATION_MODES simulation_mode; // describes the simulation state for this module

  XrefObject *xref;                 // Updated when the module changes
  

  //! I/O pin specific
  /*
    
  */

  virtual int get_pin_count(void);
  virtual string &get_pin_name(unsigned int pin_number);
  virtual int get_pin_state(unsigned int pin_number);
  virtual IOPIN *get_pin(unsigned int pin_number);
  virtual void assign_pin(unsigned int pin_number, IOPIN *pin);
  virtual void create_pkg(unsigned int number_of_pins);

  /// Attributes:

  void add_attribute(Value *);

  virtual void set(const char *cP,int len=0);
  virtual void get(char *, int len);
  virtual Value *get_attribute(char *attr, bool bWarnIfNotFound=true);
  virtual void dump_attributes(int show_values=1);

  /// Reset 

  virtual void reset(RESET_TYPE r);

  /// Version
  virtual char *get_version(void) { return version;}

  /// gui
  virtual void set_widget(void * a_widget) {widget = a_widget;}
  virtual void *get_widget(void) {return widget;}

  /// cli
  /// Modules can have special scripts associated with them.
  void add_command(string &script_name, string &command);
  void run_script(string &script_name);

  const virtual char *type(void) { return (name_str.c_str()); };

  static Module *construct(char *);
  Module(void);
  virtual ~Module();

  /// Functions to support actual hardware
  virtual bool isHardwareOnline() { return true; }

private:
  void *widget;   // GtkWidget * that is put in the breadboard.

  // Storage for scripts specifically associated with this module.
  class ModuleScript {
  public:
    ModuleScript(string &name_);
    ~ModuleScript();
    void add_command(string &command);
    void run(ICommandHandler *);
  private:
    string name;
    list<string *> m_commands;
  };

  map<string ,ModuleScript *> m_scripts;

protected:
  char *version;

};

class Module_Types
{
public:

  char *names[2];
  Module * (*module_constructor) (const char *module_name);
};


/**
  * CFileSearchPath
  * Implemented in os_dependent.cc
  */
class CFileSearchPath : public list<string> {
public:
  CFileSearchPath() {}
  void AddPathFromFilePath(string &sFolder, string &sFile);
  const char * Find(string &path);
};

/*****************************************************************************
 *
 * Helper functions
 *
 *****************************************************************************/
void GetFileName(string &sPath, string &sName);
void GetFileNameBase(string &sPath, string &sName);
void FixupLibraryName(string &sPath);
void * load_library(const char *library_name, char **pszError);
void * get_library_export(const char *name, void *library_handle, char **pszError);
void free_library(void *handle);
void free_error_message(char * pszError);
void module_display_available(void);
void module_list_modules(void);
bool module_load_library(const char *library_name);
void module_load_module(const char * module_type, const char * module_new_name=0);
void module_reset_all(RESET_TYPE r);

void module_pins(char *module_name);
void module_update(char *module_name);
#endif // __MODULES_H__
