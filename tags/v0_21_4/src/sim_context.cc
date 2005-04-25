/*
   Copyright (C) 1998 T. Scott Dattalo

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
along with gpasm; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include <stdio.h>
#ifdef _WIN32
#include "uxtime.h"
#endif
#ifndef _MSC_VER
#include <sys/time.h>
#endif
#include <time.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <map>

#include "../config.h"

#include "sim_context.h"
#include "breakpoints.h"


//================================================================================
// Global Declarations
//  FIXME -  move these global references somewhere else

// don't print out a bunch of garbage while initializing
extern int     verbose;


//================================================================================
//
// pic_processor
//
// This file contains all (most?) of the code that simulates those features 
// common to all pic microcontrollers.
//
//

CSimulationContext::CSimulationContext() {
  active_cpu_id = 0;
  cpu_ids = 0;
}

CSimulationContext *CSimulationContext::s_SimulationContext = new CSimulationContext();

CSimulationContext *CSimulationContext::GetContext() {
  return s_SimulationContext;
}

void CSimulationContext::SetDefaultProcessor(const char * processor_type,
                                             const char * processor_new_name) {
  m_DefProcessorName    = processor_type;
  m_DefProcessorNameNew = processor_new_name;
}

//-------------------------------------------------------------------
Processor * CSimulationContext::set_processor(const char * processor_type,
                                              const char * processor_new_name)
{
  Processor *p;
  CProcessorList::iterator it = processor_list.find(string(processor_type));
  GetBreakpoints().clear_all(GetActiveCPU());
  GetSymbolTable().clear();
  if(processor_list.end() == it) {
    p = add_processor(processor_type,processor_new_name);
  }
  else {
    p = it->second;
    delete p;
    p = add_processor(processor_type,processor_new_name);
//    p->init
  }
  return p;
}

//-------------------------------------------------------------------
Processor * CSimulationContext::add_processor(const char * processor_type,
                                              const char * processor_new_name)
{
  if(verbose)
    cout << "Trying to add new processor '" << processor_type << "' named '" 
	 << processor_new_name << "'\n";

  ProcessorConstructor *pc = ProcessorConstructorList::GetList()->find(processor_type);
  if(pc) {
    Processor *p = pc->cpu_constructor();
    if(p) {
      p->m_pConstructorObject = pc;
      processor_list.insert(CProcessorList::value_type(p->name(), p));
      p->initializeAttributes();
      active_cpu = p;
      //p->processor_id = 
      active_cpu_id = ++cpu_ids;
      if(verbose) {
        cout << processor_type << '\n';
        cout << "Program Memory size " <<  p->program_memory_size() << '\n';
        cout << "Register Memory size " <<  p->register_memory_size() << '\n';
      }

      // Tell the gui or any modules that are interfaced to gpsim
      // that a new processor has been declared.
      gi.new_processor(p);
      instantiated_modules_list.push_back(p);

      return p;
    }
    else
      cout << " unable to add a processor (BUG?)\n";

  } else
    cout << processor_type << " is not a valid processor.\n(try 'processor list' to see a list of valid processors.\n";

  return(0);
}


//------------------------------------------------------------------------
// dump_processor_list - print out all of the processors a user is 
//                       simulating.

void CSimulationContext::dump_processor_list(void)
{

  cout << "Processor List\n";

  bool have_processors = 0;
  CProcessorList::iterator processor_iterator; 
  for (processor_iterator = processor_list.begin();
       processor_iterator != processor_list.end(); 
       processor_iterator++) {
      CProcessorList::value_type vt = *processor_iterator;
      Processor *p = vt.second;
      cout << p->name() << '\n';
      have_processors = 1;
    }

  if(!have_processors)
    cout << "(empty)\n";

}

void CSimulationContext::Clear() {
  GetBreakpoints().clear_all(GetActiveCPU());
  CProcessorList::iterator processor_iterator; 
  GetSymbolTable().clear_all();
  for (processor_iterator = processor_list.begin();
       processor_iterator != processor_list.end(); 
       processor_iterator++) {
      CProcessorList::value_type vt = *processor_iterator;
      Processor *p = vt.second;
      delete p;
    }
  processor_list.clear();
}

extern Symbol_Table symbol_table;  // There's only one instance of "the" symbol table
Symbol_Table & CSimulationContext::GetSymbolTable() {
  return symbol_table;
}

Breakpoints & CSimulationContext::GetBreakpoints() {
  return get_bp();
}

Processor * CSimulationContext::GetActiveCPU() {
  return get_active_cpu();
}

CSimulationContext::CProcessorList::iterator
CSimulationContext::CProcessorList::find(const key_type& _Keyval) {
  ProcessorConstructorList * pcl = ProcessorConstructorList::GetList();
  ProcessorConstructor * pc = pcl->find(_Keyval.c_str());
  if(pc == NULL)
    return end();

  iterator it;
  iterator itEnd = end();
  for(it = begin(); it != itEnd; it++) {
    if(it->second->m_pConstructorObject == pc) {
      return it;
    }
  }
  return itEnd;
}
