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

#define DEBUG 0

#if defined(_WIN32)
SymbolTable &globalSymbolTable()
{
  return gSymbolTable;
}
#endif


//-------------------------------------------------------------------
//-------------------------------------------------------------------
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


static gpsimObject *pSearchObject=0;
static gpsimObject *pFoundObject=0;
static bool spred(const SymbolEntry_t &se)
{
  pFoundObject = se.second == pSearchObject ? pSearchObject : 0;
  return pFoundObject != 0;
}

int SymbolTable_t::removeSymbol(gpsimObject *pSym)
{
  if (pSym) {

    pSearchObject = pSym;
    pFoundObject = 0;
    SymbolTable_t::iterator it = find_if (begin(), end(), spred);
    if (it != end()) {

      if (DEBUG) {
        cout << __FUNCTION__ <<':' << __LINE__ << " removing symbol ";

        if (pSym) 
          cout << pSym->name() << endl;
      }
      erase (it);
      return 1;
    }
  }
  return 0;
}

int SymbolTable_t::removeSymbol(const string &s)
{
  SymbolTable_t::iterator sti = find(s);
  if (sti != end()) {

    if (DEBUG)
      cout << __FUNCTION__ <<':' << __LINE__ << " Removing symbol " << s << endl;

    erase(sti);
    return 1;
  }

  return 0;
}

int SymbolTable_t::deleteSymbol(const string &s)
{
  SymbolTable_t::iterator sti = find(s);
  if (sti != end()) {

    if (DEBUG)
      cout << __FUNCTION__ <<':' << __LINE__ << "  Deleting symbol " << s << endl;

    delete sti->second;
    erase(sti);
    return 1;
  }

  return 0;
}


gpsimObject *SymbolTable_t::findSymbol(const string &searchString)
{
  stiFound = find(searchString);
  return stiFound != end() ? stiFound->second : 0;
}

//-------------------------------------------------------------------
//-------------------------------------------------------------------


SymbolTable::SymbolTable()
{
  MSymbolTables[string("__global__")] = &globalSymbols;
  currentSymbolTable = &globalSymbols;
}


static void dumpOneSymbol(const SymbolEntry_t &sym)
{
  cout << "  " //<< sym.second->name()  // name may not be valid.
       << " stored as " << sym.first 
       << " pointer:" << sym.second
       << "  Type:" << sym.second->showType()
       << endl;
}

static void dumpSymbolTables(const SymbolTableEntry_t &st)
{
  cout << " Symbol Table: " << st.first << endl;
  (st.second)->ForEachSymbolTable(dumpOneSymbol);
}

SymbolTable::~SymbolTable()
{
  if (DEBUG) {
    cout << "Deleting the symbol table, here's what is still left in it:\n";
    ForEachModule(dumpSymbolTables);
  }
}

int SymbolTable::addSymbol(gpsimObject *pSym)
{
  /*
  if (pSym)
    cout << "Adding " << pSym->name() << " to the global symbol table\n";
  */
  return globalSymbols.addSymbol(pSym);

}
int SymbolTable::removeSymbol(gpsimObject *pSym)
{
  /*
  cout << __FUNCTION__ <<':' << __LINE__ << "  Removing symbol ";
  if (pSym) 
    cout << " -- " << pSym->name() << " from the global symbol table\n";
  */
  return globalSymbols.removeSymbol(pSym);
}


void SymbolTable::addModule(Module *pModule)
{
  if (pModule) {
    MSymbolTables[pModule->name()] = &pModule->getSymbolTable();
    globalSymbols.addSymbol(pModule);
  }
}
void SymbolTable::removeModule(Module *pModule)
{

  if (pModule) {
    //cout << "Removing " << pModule->name() << " from the global symbol table\n";
    MSymbolTable_t::iterator mi = MSymbolTables.find(pModule->name());
    if (mi != MSymbolTables.end())
      MSymbolTables.erase(mi);
    globalSymbols.removeSymbol(pModule);
  }

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


static  SymbolTable_t *searchTable=0;
static  string searchString;
static  gpsimObject *pFound=0;

bool tpred(const pair<const string, SymbolTable_t *> &st)
{
  //cout << "searching " << st.first << endl;
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
  size_t scopeOperatorPosition = s.find_first_of(scopeOperator);
  if (scopeOperatorPosition != string::npos) {
    searchTable = &globalSymbols;
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
  MSymbolTable_t::iterator mti = find_if (MSymbolTables.begin(), MSymbolTables.end(), tpred);
  if (mti != MSymbolTables.end())
    searchTable = mti->second;
  
  return pFound;
}

int SymbolTable::removeSymbol(const string &s)
{

  gpsimObject *pObj = find(s);
  if (pObj && searchTable) {
    if (searchTable->stiFound != searchTable->end()) {

      //cout << "Removing symbol " << s << endl;

      searchTable->erase(searchTable->stiFound);
      return 1;
    }
  }

  return 0;
}
int SymbolTable::deleteSymbol(const string &s)
{
  gpsimObject *pObj = find(s);
  if (pObj && searchTable) {
    if (searchTable->stiFound != searchTable->end()) {

      //cout << "Deleting symbol " << s << endl;

      searchTable->erase(searchTable->stiFound);
      delete pObj;
      return 1;
    }
  }

  /*
  gpsimObject *pObj = find(s);
  if (pObj && searchTable) {
    if (*stiFound != searchTable->end()) {
      searchTable->erase(*stiFound);
      delete pObj;
      return 1;
    }
  }
  */
  return 0;

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


static void dumpModules(const SymbolTableEntry_t &st)
{
  cout << " Module: " << st.first << endl;
}

void SymbolTable::listModules()
{
  ForEachModule(dumpModules);
}


void SymbolTable::ForEachModule(PFN_ForEachModule forEach)
{
  
  for_each(MSymbolTables.begin(), MSymbolTables.end(),  forEach);
}

