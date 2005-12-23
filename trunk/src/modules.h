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
#include <vector>
#include <assert.h>

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

class Module;
class Processor;

template<class _Type>
class OrderedVector : public vector<_Type*> {

  struct NameLessThan : binary_function<_Type*, _Type*, bool> {
    bool operator()(const _Type* left, const _Type* right) const {
      return strcmp(left->m_pName, right->m_pName) < 0;
    }
  };

public:
  typedef typename vector<_Type*>::iterator iterator;

  OrderedVector() {
  }
  bool  Exists(const char *pName) {
    return Get(pName) != NULL;
  }
  iterator FindIt(const char *pName) {
    _Type KeyValue(pName);
    iterator sti = lower_bound(vector<_Type*>::begin( ), vector<_Type*>::end( ),
      &KeyValue, NameLessThan());
    if( sti != vector<_Type*>::end() && strcmp((*sti)->m_pName, pName) == 0) {
      return sti;
    }
    return vector<_Type*>::end();
  }
  _Type *Get(const char *pName) {
    _Type KeyValue(pName);
    iterator sti = lower_bound(vector<_Type*>::begin( ), vector<_Type*>::end( ),
      &KeyValue, NameLessThan());
    if( sti != vector<_Type*>::end() && strcmp((*sti)->m_pName, pName) == 0) {
      return *sti;
    }
    return NULL;
  }
  bool Add(_Type *pObject) {
    iterator it = lower_bound(vector<_Type*>::begin( ), vector<_Type*>::end( ),
      pObject, NameLessThan());
    if(it == vector<_Type*>::end() || strcmp((*it)->m_pName, pObject->m_pName) != 0) {
      insert(it, pObject);
      return true;
    }
    return false;
  }
};

typedef Module * (*FNMODULECONSTRUCTOR) (const char *);

class ModuleLibrary {
public:
//  static ModuleLibrary & GetSingleton() { return *m_pLibrary;};

  static void         LoadFile(const char *pFilename);
  static void         FreeFile(const char *pFilename);
  static Module *     NewObject(const char *pTypeName, const char *pName = NULL);
  static void         Delete(Module *);

  static ICommandHandler * GetCommandHandler(const char *pName);

  static string       DisplayFileList();
  static string       DisplayModuleTypeList();
  static string       DisplayModuleList();
  static string       DisplayProcessorTypeList();
  static string       DisplayModulePins(char *pName);
#if 0
  static Processor *  NewProcessorFromFile(const char *pName);
  static Processor *  NewProcessorFromType(const char *pType,
                                           const char *pName);
  static void         DeleteProcessor(Processor *);
#endif

private:
  static void         MakeCanonicalName(string &sPath, string &sName);
  static bool         FileExists(const string &sName);
  static bool         AddFile(const char *library_name,
                              void *library_handle);

public:
#ifndef SWIG
  // Module file refers to a dynamically loaded program library. (dll or so)
  class File {
  public:
    File(const char * pName, void * pHandle = NULL) {
      m_pName = strdup(pName);
      m_pHandle = pHandle;
    }
    ~File() {
      free((void*)m_pName);
    }

    ICommandHandler *GetCli();
    const char *name() {
      return(m_pName);
    }

    const char * m_pName;
    void * m_pHandle;
    Module_Types * (*get_mod_list)(void);
  };
  /*
    FileList tracks loaded library files. (i.e .dll and .so files)
  */
  typedef OrderedVector<File>     FileList;
  static  FileList &              GetFileList();

  // Module Type refers to each Module_Type exposed a module file.
  // This includes aliased names.
  class Type {
  public:
    Type(const char * pName, FNMODULECONSTRUCTOR pConstructor = NULL) {
      m_pName = pName;
      m_pConstructor = pConstructor;
    }
    const char *        m_pName;
    FNMODULECONSTRUCTOR m_pConstructor;
  };

  /*
    TypeList is a consolidated list of all module type names
    from all loaded library files.
  */
  class TypeList : public OrderedVector<Type> {
      Module *NewObject(const char *pName);
  };
  static  TypeList &          GetTypeList();
#endif

private:
  static  FileList            m_FileList;
  static  TypeList            m_TypeList;

  ModuleLibrary() {};
  ~ModuleLibrary() {};
  static  int             m_iSequenceNumber;
  /*
    ModuleList is a list of all allocated modules.
    JRH - I'm not convinced that his is needed.
  */
  typedef vector<Module*> ModuleList;
  static  ModuleList      m_ModuleList;
};


//------------------------------------------------------------------------
//
/// Module - Base class for all gpsim behavior models. 

class Module : public gpsimObject {
public:
  friend class ModuleLibrary;

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

  virtual Value *get_attribute(char *attr, bool bWarnIfNotFound=true);
  virtual string  DisplayAttributes(bool show_values=true);
  virtual void initializeAttributes();

  /// Reset 

  virtual void reset(RESET_TYPE r);

  /// Version
  virtual char *get_version(void) { return version;}

  /// gui
  virtual void set_widget(void * a_widget) {widget = a_widget;}
  virtual void *get_widget(void) {return widget;}

  /// cli
  /// Modules can have gpsim CLI scripts associated with them. 
  /// add_command will add a single CLI command to a script
  void add_command(string &script_name, string &command);

  /// run_script will pass a script to the gpsim CLI. This script
  /// executes immediately (i.e. it'll execute before any commands
  /// that may already be queued).
  void run_script(string &script_name);


  void SetType(ModuleLibrary::Type *pType);
#ifndef SWIG
  const virtual char *GetTypeName() { return m_pType->m_pName; }
  // deprecated
  const virtual char *type(void) { return m_pType->m_pName; }
#endif

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
    void concatenate(ModuleScript *);
  private:
    string name;
    list<string *> m_commands;
  };

  map<string ,ModuleScript *> m_scripts;

protected:
  char *version;
  ModuleLibrary::Type *m_pType;
};

class Module_Types
{
public:

  char *names[2];
  Module * (*module_constructor) (const char *module_name);
};

#ifndef SWIG
const int Module_Types_Name_Count = sizeof(((Module_Types*)NULL)->names) / sizeof(char*);


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
#endif


#endif // __MODULES_H__
