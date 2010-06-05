/*
   Copyright (C) 1998 T. Scott Dattalo

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

#ifndef _WIN32
#if !defined(_MAX_PATH)
  #define _MAX_PATH 1024
#endif
#include <unistd.h>
#else
#include <direct.h>
#endif


#include "../config.h"

#include "errors.h"
#include "fopen-path.h"
#include "program_files.h"
#include "sim_context.h"
#include "breakpoints.h"
#include "trace.h"
#include "symbol.h"

//================================================================================
// Global Declarations
//  FIXME -  move these global references somewhere else

// don't print out a bunch of garbage while initializing


//================================================================================
//
// pic_processor
//
// This file contains all (most?) of the code that simulates those features 
// common to all pic microcontrollers.
//
//

CSimulationContext::CSimulationContext() 
  :  m_bEnableLoadSource(*new Boolean("EnableSourceLoad", true,
                                      "Enables and disables loading of source code")) 
{
  active_cpu_id = 0;
  cpu_ids = 0;
  m_pbUserCanceled = NULL;
  globalSymbolTable().addSymbol(&m_bEnableLoadSource);
}

CSimulationContext::~CSimulationContext() 
{
  globalSymbolTable().deleteSymbol("EnableSourceLoad");
}

void CSimulationContext::Initialize()
{
}

//------------------------------------------------------------
// The static pointer 's_SimulationContext' in CSimulationContext
// is initialized to 0 here, but initialized to something valid 
// when GetContext is called.

CSimulationContext *CSimulationContext::s_SimulationContext = 0;

CSimulationContext *CSimulationContext::GetContext()
{
  // Declare static CSimulationContext and return a pointer to it.
  // Note, we could 'new' an object, but there's no way to call the
  // destructor in that case.
  static CSimulationContext sContext;
  s_SimulationContext = &sContext;
  return s_SimulationContext;
}

bool CSimulationContext::SetDefaultProcessor(const char * processor_type,
                                             const char * processor_new_name) 
{
  if (processor_type) {
    ProcessorConstructor *pc = ProcessorConstructorList::GetList()->findByType(processor_type);

    if (pc) {
      m_DefProcessorName    = processor_type;
      if(processor_new_name == NULL)
	m_DefProcessorNameNew.clear();
      else
	m_DefProcessorNameNew = processor_new_name;
      return true;
    }
  } else {

    m_DefProcessorNameNew = processor_new_name;

  }

  return false;
}

//-------------------------------------------------------------------
Processor * CSimulationContext::SetProcessorByType(const char * processor_type,
                                                   const char * processor_new_name)
{
  Processor *p;
  CProcessorList::iterator it = processor_list.findByType(string(processor_type));
  GetBreakpoints().clear_all(GetActiveCPU());

  cout << __FUNCTION__ << " FIXME \n";
  // GetSymbolTable().Reinitialize();

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

  ProcessorConstructor *pc = ProcessorConstructorList::GetList()->findByType(processor_type);
  if(pc) {
    return add_processor(pc,processor_new_name ? processor_new_name : m_DefProcessorNameNew.c_str());
  } else
    cout << processor_type << " is not a valid processor.\n"
      "(try 'processor list' to see a list of valid processors.\n";
  return 0;
}

Processor * CSimulationContext::add_processor(ProcessorConstructor *pc,
					      const char * processor_new_name)
{
  Processor *  p = pc->ConstructProcessor(processor_new_name);
  if(p) {
    add_processor(p);
    p->m_pConstructorObject = pc;
  }
  else
    cout << " unable to add a processor (BUG?)\n";
  return p;
}

Processor * CSimulationContext::add_processor(Processor *p)
{
    processor_list.insert(CProcessorList::value_type(p->name(), p));
    //p->initializeAttributes();
    active_cpu = p;
    //p->processor_id = 
    active_cpu_id = ++cpu_ids;
    if(verbose) {
      cout << p->name() << '\n';
      cout << "Program Memory size " <<  p->program_memory_size() << " words\n";
      cout << "Register Memory size " <<  p->register_memory_size() << '\n';
    }

    trace.switch_cpus(p);
    // Tell the gui or any modules that are interfaced to gpsim
    // that a new processor has been declared.
    gi.new_processor(p);

    return p;

  return 0;
}

int CSimulationContext::LoadProgram(const char *filename,
                                    const char *pProcessorType,
                                    Processor **ppProcessor,
				    const char *pProcessorName)
{
  bool bReturn = false;
  Processor *pProcessor;
  FILE * pFile = fopen_path (filename, "rb");
  if(pFile == NULL) {
    char cw[_MAX_PATH];

    perror((string("failed to open program file ") + filename).c_str());
    getcwd(cw, sizeof(cw));
    cerr << "current working directory is ";
    cerr << cw;
    cerr << endl;
    return false;
  }
  if(pProcessorType != NULL) {
    pProcessor = SetProcessorByType(pProcessorType, NULL);
    if(pProcessor != NULL) {
      bReturn  = pProcessor->LoadProgramFile(filename, pFile, pProcessorName);
    }
  }
  else if(!m_DefProcessorName.empty()) {
    pProcessor = SetProcessorByType(m_DefProcessorName.c_str(), NULL);
    if(pProcessor != NULL) {
      bReturn  = pProcessor->LoadProgramFile(filename, pFile, pProcessorName);
    }
  }
  else {
    pProcessor = NULL;
    if (!m_DefProcessorNameNew.empty())
      pProcessorName = m_DefProcessorNameNew.c_str();
    // use processor defined in program file
    bReturn  = ProgramFileTypeList::GetList().LoadProgramFile(
			   &pProcessor, filename, pFile, pProcessorName);

  }

  fclose(pFile);
  if(bReturn) {
    // Tell all of the interfaces that a new program exists.
    gi.new_program(pProcessor);
  }
  if(ppProcessor != NULL) {
    *ppProcessor = pProcessor;
  }
  return bReturn;
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

void CSimulationContext::Clear() 
{

  CProcessorList::iterator processor_iterator; 
  for (processor_iterator = processor_list.begin();
       processor_iterator != processor_list.end(); 
       ++processor_iterator) {
    CProcessorList::value_type vt = *processor_iterator;
    Processor *p = vt.second;
    GetBreakpoints().clear_all(p);
    delete p;
  }
  processor_list.clear();
}

void CSimulationContext::Reset(RESET_TYPE r)
{
  /*
  Symbol_Table &ST = get_symbol_table();
  Symbol_Table::module_symbol_iterator it;
  Symbol_Table::module_symbol_iterator itEnd = ST.endModuleSymbol();
  for(it = ST.beginModuleSymbol(); it != itEnd; it++) {
      Module *m = (*it)->get_module();
      if(m) {
        m->reset(r);
      }
  }
  */
  cout << __FUNCTION__ << " FIXME \n";
}

void CSimulationContext::NotifyUserCanceled() {
  if(m_pbUserCanceled != NULL) {
    *m_pbUserCanceled = true;
    m_pbUserCanceled = NULL;
    return;
  }
  if(CSimulationContext::GetContext()->GetActiveCPU()->simulation_mode
    == eSM_RUNNING) {
    // If we get a CTRL->C while processing a command file
    // we should probably stop the command file processing.
    CSimulationContext::GetContext()->GetBreakpoints().halt();
  }
}

SymbolTable & CSimulationContext::GetSymbolTable() 
{
  return gSymbolTable;
}

Breakpoints & CSimulationContext::GetBreakpoints() {
  return get_bp();
}

Processor * CSimulationContext::GetActiveCPU() {
  return get_active_cpu();
}

Cycle_Counter * CSimulationContext::GetCycleCounter() {
  return &cycles;
}

CSimulationContext::CProcessorList::iterator
CSimulationContext::CProcessorList::findByType(const key_type& _Keyval) {
  // First find a ProcessorConstructor that matches the
  // processor type we are looking for. This should handle
  // naming variations.
  ProcessorConstructorList * pcl = ProcessorConstructorList::GetList();
  ProcessorConstructor * pc = pcl->findByType(_Keyval.c_str());
  if(pc == NULL)
    return end();
  // Now find the specific allocated processor that
  // was created with the ProcessorConstructor object 
  // we found above.
  iterator it;
  iterator itEnd = end();
  for(it = begin(); it != itEnd; it++) {
    if(it->second->m_pConstructorObject == pc) {
      return it;
    }
  }
  return itEnd;
}
