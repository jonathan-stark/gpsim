/*
   Copyright (C) 1999 T. Scott Dattalo

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


#include <iostream>
#include <iomanip>
#include <string>

#include "command.h"
#include "cmd_module.h"

#include "../src/pic-processor.h"
#include "../src/modules.h"
#include "../src/symbol.h"
#include "../src/cmd_manager.h"



cmd_module c_module;

#define CMD_MOD_LIST    1
#define CMD_MOD_LOAD    2
#define CMD_MOD_DUMP    3
#define CMD_MOD_LIB     4
#define CMD_MOD_PINS    5


static cmd_options cmd_module_options[] =
{
  {"list",      CMD_MOD_LIST,    OPT_TT_BITFLAG},
  {"load",      CMD_MOD_LOAD,    OPT_TT_STRING},
  {"dump",      CMD_MOD_DUMP,    OPT_TT_BITFLAG},
  {"pins",      CMD_MOD_PINS,    OPT_TT_STRING},
  {"library",   CMD_MOD_LIB ,    OPT_TT_STRING},
  {"lib",       CMD_MOD_LIB ,    OPT_TT_STRING},
  {0,0,0}
};


cmd_module::cmd_module()
  : command("module","mod")
{ 

  brief_doc = string("Select & Display modules");

  long_doc = string (
    "module [ [load module_type [module_name]] | [lib lib_name] | [list] | \n"
    "[[dump | pins] module_name] ] \n"
    "\tIf no options are specified, then the currently defined module(s)\n"
    "\twill be displayed. This is the same as the `module list' command.\n"
    "\tThe `module load lib_name' tells gpsim to search for the module\n"
    "\tlibrary called `lib_name' and to load it. (Note that the format of\n"
    "\tmodule libraries is exactly the same as a Linux shared library. This\n"
    "\tmeans that the module library should reside in a path available to\n"
    "\tdlopen(). Please see the README.MODULES in the gpsim distribution.\n"
    "\tTo instantiate a new module, then type\n"
    "\t  module module_type module_name\n"
    "\twhere module_type refers to a specific module in a module library\n"
    "\tand module_name is the user name assigned to it.\n"
    "\tInformation about a module can be displayed by the command\n"
    "\t  module module_name [dump | pins]\n"
    "\twhere module_name is the name that you assigned when the module\n"
    "\twas instantiated. The optional dump and pins identifiers specify\n"
    "\tthe information you wish to display (dump is the default).\n"
    "\n"
    "\tDevelopers of gpsim and developers building libraries for use with\n"
    "\tgpsim may find it useful to set the GPSIM_MODULE_PATH environment variable\n"
    "\tto the target folder of the library module that is under development.\n"
    "\tMultiple folders may be delimited with a ':' for Linux and ';' for\n"
    "\tWindows.\n"
    "\n"
    "\texamples:\n"
    "\n"
    "\tmodule                      // Display the modules you've already defined.\n"
    "\tmodule lib my_mods.so       // Load the module library called my_mods.\n"
    "\tmodule list                 // Display the list of modules supported.\n"
    "\tmodule load lcd my_lcd      // Create an instance of an 'lcd'\n"
    "\tmodule pins my_lcd          // Display the pin states of an instantiated module\n"
    "\tmodule load lcd lcd2x20     // Create a new module.\n"
    "\tmodule load pullup R1       // and another.\n"
);

  op = cmd_module_options; 
}

static void dumpModules(const SymbolTableEntry_t &st)
{
  cout << " Module: " << st.first << endl;
}

void cmd_module::module(void)
{

  if(verbose)
    cout << "cmd_module: display modules\n";
#ifdef OLD_MODULE_LIBRARY
  cout << ModuleLibrary::DisplayModuleList();
#else
  globalSymbolTable().ForEachModule(dumpModules);
#endif

}

void cmd_module::module(cmd_options *opt)
{
  if(!opt)
    return;

  switch(opt->value)
    {

    case CMD_MOD_LIST:
#ifdef OLD_MODULE_LIBRARY
      cout << ModuleLibrary::DisplayFileList();
#else
      ModuleLibrary::ListLoadableModules();
#endif
      break;

    default:
      cout << "cmd_module error:";
      if(opt->name)
        cout << " no parameters with option: " << opt->name;
      cout << endl;
    }

}

void cmd_module::module(cmd_options_str *cos, 
			list <string> *strs)
{
  //  const int cMAX_PARAMETERS=2;
  //  int nParameters=cMAX_PARAMETERS;
  //  guint64 parameters[cMAX_PARAMETERS] = {0,0};

  //  evaluate(eList, parameters, &nParameters);

  list <string> :: iterator si;
  string s1, s2;
  int nStrings = 0;

  if (strs) {
    nStrings = strs->size();

    si = strs->begin();
    if(strs->size() >= 1) {
      s1 = *si;
    
      if(strs->size() >= 2) {
        ++si;
        s2 = *si;
      }
    }

  }

  // Now choose the specific command based on the input parameters
  
  if(nStrings==0)
    module(cos);
  else if(nStrings==1)
    module(cos, s1.c_str());
  //else if(nParameters==0 && nStrings==2)
  //  module(cos, (char*)s1.c_str(), (char*)s2.c_str());
  //else if(nParameters==1 && nStrings==1)
  //  module(cos, (char*)s1.c_str(), parameters[0]);
  //else if(nParameters==2 && nStrings==0)
  //  module(cos, parameters[0], parameters[1]);
  //else if(nParameters==1 && nStrings==2)
  //  module(cos, (char*)s1.c_str(), (char*)s2.c_str(), parameters[0]);
  else
    cout << "module command error\n";

}


void cmd_module::module(cmd_options_str *cos)
{

  if(!cos)
    return;

  switch(cos->co->value)
    {
    case CMD_MOD_LIB:
      if(verbose)
        cout << "module command got the library " << cos->str << endl;

      try {
        string fname(cos->str);
        ModuleLibrary::LoadFile(fname);
      }
      catch(Error *pError) {
        cout << pError->get_errMsg();
      }

      break;
    case CMD_MOD_LOAD:
      // Load a module from (an already loaded) library and let
      // gpsim assign the name.
      if(verbose)
        cout << "module command got the module " << cos->str << '\n';
#ifdef OLD_MODULE_LIBRARY
      if(ModuleLibrary::NewObject(cos->str) == NULL) {
        GetUserInterface().DisplayMessage("module type %s not created\n", cos->str);
      }
#else
      {
        cout << "Fixme -- module NewObject\n";
      }
#endif

      break;

    case CMD_MOD_DUMP:
      cout <<  " is not supported yet\n";
      break;

    case CMD_MOD_PINS:
      cout << "Fixme: display module pins is not supported...\n";
      //ModuleLibrary::DisplayModulePins(cos->str);
      break;

    default:
      cout << "cmd_module error\n";
    }

}


void  cmd_module::module(cmd_options_str *cos, const char *op1)
{

  switch(cos->co->value)
    {

    case CMD_MOD_LOAD:
      // Load a module from (an already loaded) library 
#ifdef OLD_MODULE_LIBRARY
      if(ModuleLibrary::NewObject(cos->str,  op1) == NULL) {
        GetUserInterface().DisplayMessage("module type %s not created\n", cos->str);
      }
#else
      {
        string mName(cos->str);
        string refDes(op1);
        if(!ModuleLibrary::InstantiateObject(mName,refDes))
          GetUserInterface().DisplayMessage("module type %s not created\n", cos->str);
      }        
#endif

      break;

    default:
      cout << "Warning, ignoring module command\n";
    }
}

