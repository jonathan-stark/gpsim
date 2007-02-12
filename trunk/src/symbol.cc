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

//
// symbol.cc
//
//  The file contains the code that controls all of the symbol
// stuff for gpsim. Most of the work is handled by the C++ map
// container class.
//
#include "symbol.h"

#include <iostream>
#include <iomanip>
#include <sstream>

#include <string>
#include <vector>
#include <algorithm>

#include "value.h"
#include "modules.h"

SymbolTable   gSymbolTable;
SymbolTable_t globalSymbols;
static SymbolTable_t *currentSymbolTable=0;


#if defined(_WIN32)
SymbolTable &globalSymbolTable()
{
  return gSymbolTable;
}
#endif


//-------------------------------------------------------------------
//-------------------------------------------------------------------
SymbolTable_t::~SymbolTable_t()
{
}

int SymbolTable_t::addSymbol(gpsimObject *pSym, string *ps_AliasedName)
{
  if (pSym) {
    ps_AliasedName = (ps_AliasedName && !ps_AliasedName->empty()) ? ps_AliasedName : &pSym->name();
    SymbolTable_t::iterator sti = find(*ps_AliasedName);
    if (sti == end()) {
      operator[](*ps_AliasedName) = pSym;
      return 1;
    }
  }
  return 0;
}

int SymbolTable_t::removeSymbol(gpsimObject *pSym, bool bDeleteObject)
{
  if (pSym) {
    SymbolTable_t::iterator sti = find(pSym->name());
    if (sti != end()) {
      if (bDeleteObject)
        delete (*sti).second;
      erase(sti);
      return 1;
    }
  }
  return -1;
}

gpsimObject *SymbolTable_t::findSymbol(const string &searchString)
{
  SymbolTable_t::iterator sti = find(searchString);

  return sti != end() ? sti->second : 0;
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------


SymbolTable::SymbolTable()
{
  MSymbolTables[string("__global__")] = &globalSymbols;
  currentSymbolTable = &globalSymbols;
}

SymbolTable::~SymbolTable()
{
}

int SymbolTable::addSymbol(gpsimObject *pSym)
{
  if (pSym)
    cout << "Adding " << pSym->name() << " to the global symbol table\n";
  return globalSymbols.addSymbol(pSym);

}
int SymbolTable::removeSymbol(gpsimObject *pSym, bool bDeleteObject)
{
  if (pSym) 
    cout << "Removing " << pSym->name() << " from the global symbol table\n";
  return globalSymbols.removeSymbol(pSym, bDeleteObject);
}


void SymbolTable::addModule(Module *pModule)
{
  if (pModule)
    MSymbolTables[pModule->name()] = &pModule->getSymbolTable();
}
void SymbolTable::removeModule(Module *pModule)
{
  /*
  if (pModule) {
    ModuleList_t::iterator mi = gModuleList.find(pModule->name().c_str());
    if (mi->second == pModule)
      gModuleList.erase(mi);
  }
  */
}

void SymbolTable::listModules()
{
  cout << "list modules -- implement\n";
}


/*
class SymbolFinder
{
  string searchString;
public:
  gpsimObject *pFound;
  SymbolFinder(string s)
    : searchString(s)
  {}

  bool tpred(const pair<const string, SymbolTable_t *> &st)
  {
    cout << "searching " << st.first << endl;
    SymbolTable_t::iterator sti = st.second->find(searchString);
    pFound = sti != st.second->end() ? sti->second : 0;
    
    return pFound != 0;
  }
};
*/

static  string searchString;
static  gpsimObject *pFound=0;
bool tpred(const pair<const string, SymbolTable_t *> &st)
{
  cout << "searching " << st.first << endl;
  pFound = st.second->findSymbol(searchString);
  return pFound != 0;
}

gpsimObject *SymbolTable::find(string s)
{
  // First check scoping
  //
  //   SymbolTableName.SymbolName
  //                  ^
  //                  |
  //                  +---  scoping operator
  //
  // If the search string contains the scoping operator (i.e. '.') 
  // then the symbol table specified by the scope. In other words,
  // if the search string begins with a period, then search the
  // current symbol table.
  // If the search string contains a period, then search for the
  // symbol table named with the string to the left of the period
  // and if that table is found search in it for the string to 
  // the right of the period.

  const char scopeOperator = '.';
  int scopeOperatorPosition = s.find_first_of(scopeOperator);
  if (scopeOperatorPosition != string::npos) {
    SymbolTable_t *searchTable = &globalSymbols;
    if (scopeOperatorPosition == 0) {   // Select the current symbol table
      searchTable = currentSymbolTable;
      scopeOperatorPosition++;
    }
    else {
      // Find the symbol table with the scoped name:
      string moduleName = s.substr(0, scopeOperatorPosition); //,string::npos);
      MSymbolTable_t::iterator mti = MSymbolTables.find(moduleName);
      if (mti != MSymbolTables.end()) {
        searchTable = mti->second;
        scopeOperatorPosition++;
      }
    }
    SymbolTable_t::iterator sti = searchTable->find(s.substr(scopeOperatorPosition,string::npos));
    if (sti != searchTable->end())
      return sti->second;
  }

  pFound = 0;  // assume the symbol is not found.
  searchString = s;
  find_if (MSymbolTables.begin(), MSymbolTables.end(), tpred);

  return pFound;
}

//------------------------------------------------------------------------
// Convenience find functions
// All these do is call SymbolTable::find and cast the found symbol into 
// another type

gpsimObject *SymbolTable::findObject(gpsimObject *pObj)
{
  return pObj ? find(pObj->name()) : 0;
}

Integer *SymbolTable::findInteger(string s)
{
  return dynamic_cast<Integer *>(find(s));
}
Value *SymbolTable::findValue(string s)
{
  return dynamic_cast<Value *>(find(s));
}
Module *SymbolTable::findModule(string s)
{
  return dynamic_cast<Module *>(find(s));
}

void SymbolTable::ForEachModule(PFN_ForEachModule forEach)
{
  
  for_each(MSymbolTables.begin(), MSymbolTables.end(),  forEach);
}

