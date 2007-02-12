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

//    End of new symbol table design.
//
//************************************************************************

#if defined(USE_OLD_SYMBOL_TABLE)

Symbol_Table symbol_table;  // There's only one instance of "the" symbol table

#if defined(_WIN32)
Symbol_Table &get_symbol_table(void) {
  return symbol_table;
}
#endif

Symbol_Table::Symbol_Table() {
    reserve(500);
}

/*
void Symbol_Table::add_stimulus_node(Stimulus_Node *s, bool bClearable)
{
  node_symbol *sym = findNodeSymbol(s->name());
  // New paradigm is for the named stimulus objects
  // to add themselves and remove them selves.
  // Since there is a lot of code that adds a stimulus
  // object we will ignore them if the stimulus already
  // exists unless it is a different object with the
  // same name.
  if(sym == NULL) {
    node_symbol *ns = new node_symbol(s);
    ns->setClearableSymbol(bClearable);
    if(!add(ns)) {
      delete ns;
    }
  }
  else if(sym->getNode() != s) {
    GetUserInterface().DisplayMessage("Warning: Attempt to add symbol %s that already exists\n",
      s->name().c_str());
  }
  else {
    // use this code to capture calls to add_stimulus_node() that are not needed
    GetUserInterface().DisplayMessage("Warning: Attempt to add symbol object '%s' twice\n",
      s->name().c_str());
  }
}

void Symbol_Table::add_stimulus(stimulus *s, bool bClearable)
{
  stimulus_symbol *sym = findStimulusSymbol(s->name());
  // New paradigm is for the named stimulus objects
  // to add themselves and remove them selves.
  // Since there is a lot of code that adds a stimulus
  // object we will ignore them if the stimulus already
  // exists unless it is a different object with the
  // same name.
  if(sym == NULL) {
    stimulus_symbol *ss = new stimulus_symbol(s);
    ss->setClearableSymbol(bClearable);
    if(!add(ss)) {
      delete ss;
    }
  }
  else if(sym->getStimulus() != s) {
    GetUserInterface().DisplayMessage("Warning: Attempt to add symbol %s that already exists\n",
      s->name().c_str());
  }
  else {
    // use this code to capture calls to add_stimulus() that are not needed
    GetUserInterface().DisplayMessage("Warning: Attempt to add symbol object '%s' twice\n",
      s->name().c_str());
  }
}
*/

// This is an experiment to have the symbol table ordered
// case insensitive unless their is a match for a case insensitive
// compare, then a case sensitive compare is made.
int SymbolCompare(const char *pLeft, const char *pRight) {
#if 0
  int iResult = stricmp(pLeft, pRight);
  if(iResult < 0) {
    return -1;
  }
  else if(iResult > 0) {
    return 1;
  }
  else {
    return strcmp(pLeft, pRight);
  }
#else
    return strcmp(pLeft, pRight);
#endif
}

//------------------------------------------------------------------------
// add(Value *) - add a symbol to the symbol table
bool Symbol_Table::add(Value *s)
{
  if(s) {
    if(s->name().empty()) {
      printf("Symbol_Table::add() attempt to add a symbol with no name: %s\n",
        s->toString().c_str());
    }
    else {
      iterator it = lower_bound(begin( ), end( ),
        s, NameLessThan());
      if (it != end() &&
        (*it)->name() == s->name()) {
        GetUserInterface().DisplayMessage(
            "Symbol_Table::add(): Warning: failed to add symbol "
            "because a symbol by the name '%s' already exists, new object is type %s\n",
            s->name().c_str(), s->showType().c_str());
        return false;
      }
      if(verbose)
	cout << "Adding '" << s->name() << "' to the symbol table\n";
      insert(it, s);

      return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------
// add_register
register_symbol *Symbol_Table::add_register(Register *new_reg, const char *symbol_name)
{
  // mask of zero lets the register_symbol calculate a default mask
  return add_register(new_reg, symbol_name, 0);
}

register_symbol * Symbol_Table::add_register(Register *new_reg, 
					     const char *symbol_name,
					     unsigned int uMask)
{
  if(!new_reg)
    return 0;

  if(symbol_name) {
    string sName(symbol_name);
    if((new_reg->name() == sName && find(new_reg->name()) ) ||
       new_reg->baseName() == sName && find(new_reg->baseName())) {
      if(verbose)
        GetUserInterface().DisplayMessage(
        "Symbol_Table::add_register(): Warning: Not adding register symbol '%s'"
	        " to symbol table\n because it already exists.\n",
	        symbol_name);
      return 0;
     }
  }

  register_symbol *rs = new register_symbol(symbol_name, new_reg, uMask);

  add(rs);
  return rs;
}

void Symbol_Table::add_w(WREG *new_w)
{

  if(!new_w)
    return;

  w_symbol *ws = new w_symbol((char *)0, new_w);

  add(ws);

}

void Symbol_Table::add_constant(const char *_name, int value, bool bClearable)
{
  Integer *i = new Integer(value);
  i->new_name(_name);
  i->setClearableSymbol(bClearable);
  add(i);
}
/*
void Symbol_Table::add_address(Processor *pCpu, const char *new_name, int value)
{
  add(new AddressSymbol(pCpu, new_name,value));
}

void Symbol_Table::add_line_number(Processor *pCpu, int address, const char *symbol_name)
{
  add(new LineNumberSymbol(pCpu, symbol_name,  address));
}
*/
void Symbol_Table::add_module(Module * m, const char *cPname)
{
  module_symbol *ms = new module_symbol(m,cPname);

  if(!add(ms)) {
    delete ms;
  }
}

void Symbol_Table::remove_module(Module * m)
{
  iterator sti = FindIt(m->name());
  Value *sym;

  while( sti != end()) {
    sym = *sti;
    if((typeid(sym) == typeid(module_symbol)) &&
       sym->name() == m->name()) {
      erase(sti);
      return;
    }
    sti++;
  }
}

Value *Symbol_Table::remove(string &s)
{
  iterator it = FindIt(s);
  if(it != end() && (*it)->name() == s) {
    Value *pValue = *it;
    erase(it);
    return pValue;
  }
  return NULL;
}

void Symbol_Table::rename(const char *pOldName, const char *pNewName)
{
  // First make sure the old and new names are both valid.
  if (pNewName && pOldName && *pOldName && *pNewName) {
    iterator it = FindIt(pOldName);
    if(it != end() && (*it)->name() == pOldName) {
      Value *pValue = *it;
      erase(it);
      pValue->new_name(pNewName);
      add(pValue);
    }
  }
}

Value * Symbol_Table::find(const char *str)
{
  string s(str);
  return(find(s));
}

Value * Symbol_Table::find(type_info const &symt, const char *str)
{
  string s(str);
  iterator sti = FindIt(str);
  while( sti != end()) {
    Value *val = *sti;
    int iResult = val->name().compare(s);
    if(iResult == 0) {
      return val;
    }
    else if(iResult > 0) {
      // leave early for efficiency
      return NULL;
    }
    sti++;
  }
  return 0;
}

Register * Symbol_Table::findRegister(unsigned int address)
{
  iterator sti = begin();
  while( sti != end()) {
    register_symbol *pSymbol = dynamic_cast<register_symbol*>(*sti);
    if(pSymbol != 0) {
      Register * pReg = pSymbol->getReg();
      if(pReg->address == address &&
        pSymbol->getBitmask() == pReg->get_cpu()->register_mask())
        // This function will find the first symbol that
        // uses the entire bit field of the register.
        return(pReg);
    }
    sti++;
  }
  return NULL;
}

register_symbol * Symbol_Table::findRegisterSymbol(const char *pName)
{
  iterator sti;
  register_symbol *pRegSym;
  for( sti = FindIt(pName) ;sti != end(); sti++) {
    Value *val = *sti;
    if(val->name() == pName) {
      if((pRegSym = dynamic_cast<register_symbol*>(val)) != NULL) {
        return pRegSym;
      }
    }
  }
  return 0;
}

register_symbol * Symbol_Table::findRegisterSymbol(unsigned int uAddress)
{
  iterator sti = begin();
  ostringstream sDumbLabel;
  sDumbLabel << "R" << hex << uppercase << uAddress;
  while( sti != end()) {
    register_symbol *pRegSymbol = dynamic_cast<register_symbol*>(*sti);
    if(pRegSymbol != 0) {
      Register * pReg = pRegSymbol->getReg();
      if (0 && pReg && pReg->get_cpu() == NULL) {
	// hmmm  we need to fix this... It's possible for 
	// modules (which are not processors) to have registers.
        //cout << " Null cpu for reg named:"<<pReg->name()<<endl;

      }
      //assert(pReg->get_cpu() != NULL);
      if(pReg && pReg->get_cpu() && pRegSymbol->getAddress() == uAddress &&
        pRegSymbol->getBitmask() == pReg->get_cpu()->register_mask() &&
        // This function will find the first symbol that
        // uses the entire bit field of the register.
        sDumbLabel.str() != pRegSymbol->name()) {
          return(pRegSymbol);
        }
    }
    sti++;
  }
  return NULL;
}

register_symbol * Symbol_Table::findRegisterSymbol(unsigned int uAddress,
                                                   unsigned int uBitmask)
{
  if(uBitmask == 0) {
    uBitmask = get_active_cpu()->register_mask();
  }
  iterator sti = begin();
  ostringstream sDumbLabel;
  sDumbLabel << "R" << hex << uppercase << uAddress;
  while( sti != end()) {
    register_symbol *pRegSymbol = dynamic_cast<register_symbol*>(*sti);
    if(pRegSymbol != 0) {
      if(pRegSymbol->getAddress() == uAddress &&
        pRegSymbol->getBitmask() == uBitmask &&
        sDumbLabel.str() != pRegSymbol->name()) {
          return(pRegSymbol);
        }
    }
    sti++;
  }
  return NULL;
}

Register * Symbol_Table::findRegister(const char *s)
{
  iterator sti = FindIt(s); // .begin();
  while( sti != end()) {
    Value *val = *sti;
    if(val && typeid(*val) == typeid(register_symbol)) {
      if(val->name() == s) {
        return ((register_symbol*)val)->getReg();
      }
    }
    sti++;
  }
  return NULL;
}

const char * Symbol_Table::findProgramAddressLabel(unsigned int address) {
  iterator sti = begin();
  while( sti != end()) {
    Value *val = *sti;
    AddressSymbol * pAddSym = dynamic_cast<AddressSymbol*>(val);
    if(pAddSym != NULL) {
      gint64 iSymbolAddress;
      pAddSym->get(iSymbolAddress);
      if(iSymbolAddress == address &&
        strncmp(pAddSym->name().c_str(), "line_", sizeof("line_") - 1) != 0) {
        return pAddSym->name().c_str();
      }
    }
    sti++;
  }
  return "";
}

const char * Symbol_Table::findConstant(unsigned int uValue,
                                        unsigned int uReferencedFromAddress)
{
  // regarding uReferencingAddress
  // see comment in the header.
  iterator sti = begin();
  while( sti != end()) {
    Integer *val = dynamic_cast<Integer*>(*sti);
    if(val != NULL) {
      gint64 uSymValue;
      val->get(uSymValue);
      if(uValue == (unsigned int)uSymValue)
        return(val->name().c_str());
    }
    sti++;
  }
  return NULL;
}

Integer * Symbol_Table::findInteger(const char *s)
{
  return findSymbol(s, (Integer*)NULL);
}

Boolean * Symbol_Table::findBoolean(const char *s)
{
  return findSymbol(s, (Boolean*)NULL);
}

module_symbol *Symbol_Table::findModuleSymbol(const char *s)
{
  return findSymbol(s, (module_symbol*)NULL);
}

Module * Symbol_Table::findModule(const char *s)
{
  module_symbol * pNodeSym = findModuleSymbol(s);
  if( pNodeSym != NULL) {
    return pNodeSym->get_module();
  }
  return ((Module *)0);
}
/*
node_symbol * Symbol_Table::findNodeSymbol(const char *s)
{
  return findSymbol(s, (node_symbol*)NULL);
}

Stimulus_Node * Symbol_Table::findNode(const char *s)
{
  node_symbol * pNodeSym = findNodeSymbol(s);
  if( pNodeSym != NULL) {
    return pNodeSym->getNode();
  }
  return ((Stimulus_Node *)0);
}
*/
String * Symbol_Table::findString(const char *s)
{
  return findSymbol(s, (String *)NULL);
}

template<class _symbol_iterator_t, class _symbol_t>
_symbol_iterator_t Symbol_Table::beginSymbol(_symbol_iterator_t *pit, _symbol_t*psym) {
  iterator it;
  iterator itEnd = _Myt::end();
  for(it = _Myt::begin(); itEnd != it; it++) {
    if(dynamic_cast<_symbol_t*>(*it) != NULL) {
      return _symbol_iterator_t(this, it);
    }
  }
  return _symbol_iterator_t(this, itEnd);
}

template<class _symbol_iterator_t, class _symbol_t>
_symbol_iterator_t Symbol_Table::endSymbol(_symbol_iterator_t *pit, _symbol_t*psym) {
  return _symbol_iterator_t(this,_Myt::end());
}

Symbol_Table::node_symbol_iterator Symbol_Table::beginNodeSymbol()
{
  return (node_symbol_iterator)beginSymbol((node_symbol_iterator*)0,
					   (node_symbol*)0);
}

Symbol_Table::node_symbol_iterator Symbol_Table::endNodeSymbol()
{
    return endSymbol((node_symbol_iterator*) 0, (node_symbol*)0);
}
/*
stimulus_symbol * Symbol_Table::findStimulusSymbol(const char *s)
{
  return findSymbol(s, (stimulus_symbol*)NULL);
}

stimulus * Symbol_Table::findStimulus(const char *s)
{
  stimulus_symbol * pNodeSym = findStimulusSymbol(s);
  if( pNodeSym != NULL) {
    return pNodeSym->getStimulus();
  }
  attribute_symbol *pSymbol = findAttributeSymbol(s);
  if(pSymbol != NULL) {
    Value *pValue;
    pSymbol->get(&pValue);
    return dynamic_cast<stimulus*>(pValue);
  }
  return ((stimulus *)0);
}
*/
attribute_symbol * Symbol_Table::findAttributeSymbol(const char *s)
{
  return findSymbol(s, (attribute_symbol*)NULL);
}


Symbol_Table::stimulus_symbol_iterator Symbol_Table::beginStimulusSymbol() {
  return (stimulus_symbol_iterator)beginSymbol(
    (stimulus_symbol_iterator*)NULL,
    (stimulus_symbol*)NULL);
}

Symbol_Table::stimulus_symbol_iterator Symbol_Table::endStimulusSymbol() {
  return endSymbol((stimulus_symbol_iterator*) NULL,
    (stimulus_symbol*)NULL);
}


Symbol_Table::module_symbol_iterator Symbol_Table::beginModuleSymbol() {
  iterator it;
  iterator itEnd = _Myt::end();
  for(it = _Myt::begin(); itEnd != it; it++) {
    if(dynamic_cast<module_symbol*>(*it) != NULL &&
      dynamic_cast<attribute_symbol*>(*it) == NULL) {
      return module_symbol_iterator(this, it);
    }
  }
  return module_symbol_iterator(this, itEnd);
//  return (module_symbol_iterator)beginSymbol(
//    (module_symbol_iterator*)NULL,
//    (module_symbol*)NULL);
}

Symbol_Table::module_symbol_iterator Symbol_Table::endModuleSymbol() {
  return endSymbol((module_symbol_iterator*) NULL,
    (module_symbol*)NULL);
}

Symbol_Table::module_symbol_iterator
Symbol_Table::module_symbol_iterator::operator++(int) {
  // postincrement
  Symbol_Table::iterator & it = (Symbol_Table::iterator&)*this;
  for(it++; it != m_pSymbolTable->end(); it++) {
    if(dynamic_cast<module_symbol*>(*it) != NULL &&
      dynamic_cast<attribute_symbol*>(*it) == NULL) {
      return (*this);
    }
  }
  return (*this);
}


bool Symbol_Table::Exist(const char *s) {
  return this->FindIt(s) != end();
}

void Symbol_Table::dump_one(string *s)
{
  Value *val = find(*s);
  if(val)
    cout << val->name() << " = " << val->toString() << endl;
}

void Symbol_Table::dump_one(const char *str)
{
  string s =  string(str);
  dump_one(&s);
}


void Symbol_Table::dump_all(void)
{
  cout << "  Symbol Table\n";
  bool bUserCanceled = false;
  CSimulationContext::GetContext()->SetUserCanceledFlag(&bUserCanceled);
  iterator sti = begin();
  iterator last;
  while( sti != end()) {
    Value *val = *sti;
    if(val && (typeid(*val) != typeid(LineNumberSymbol))) {
      if(dynamic_cast<IIndexedCollection*>(val) == NULL) {
        cout << val->name() << " = " ;
      }
      cout << val->toString() << endl;
    }
    last = sti;
    sti++;
    if(sti != end() && (*last)->name() == (*sti)->name()) {
      cout << "***************** Duplicate Found ***********" << endl;
    }
    if(bUserCanceled) {
      cout << endl << "Symbol dump canceled." << endl;
      break;
    }
  }
  CSimulationContext::GetContext()->SetUserCanceledFlag(NULL);


}

bool beginsWith(string &sTarget, string &sBeginsWith) {
  string sT;
  sT = sTarget.substr(0, sBeginsWith.size());
  return sT == sBeginsWith;
}

void Symbol_Table::dump_filtered(const string & sSymbol)
{
  string sBeginsWith;
  int nLastCharPos = sSymbol.size() - 1;
  if(nLastCharPos < 1) {
    dump_all();
    return;
  }
  bool bUserCanceled = false;
  CSimulationContext::GetContext()->SetUserCanceledFlag(&bUserCanceled);
  if(sSymbol[nLastCharPos] == '.') {
    sBeginsWith = sSymbol.substr(0, nLastCharPos);
  }
  else {
    dump_one(sSymbol.c_str());
  }
  Value KeyValue(sBeginsWith.c_str(), "key value");
  iterator sti = lower_bound(begin( ), end( ),
    &KeyValue, NameLessThan());
  iterator last;
  while( sti != end()) {
    Value *val = *sti;
    if(val && (typeid(*val) != typeid(LineNumberSymbol)) &&
      beginsWith(val->name(), sBeginsWith)) {
      if(dynamic_cast<IIndexedCollection*>(val) == NULL) {
        cout << val->name() << " = " ;
      }
      cout << val->toString() << endl;
      }
    last = sti;
    sti++;
    if(bUserCanceled) {
      cout << endl << "Symbol dump canceled." << endl;
      break;
    }
  }
  CSimulationContext::GetContext()->SetUserCanceledFlag(NULL);
}

void Symbol_Table::dump_type(type_info const &symt)
{
  cout << DisplayType(symt);
}

string Symbol_Table::DisplayType(type_info const &symt)
{
  ostringstream stream;
  // Now loop through the whole table and display all instances of the type of interest
  int first=1;     // On the first encounter of one, display the title
  iterator sti = begin();
  while( sti != end()) {
    Value *sym = *sti;
    if(sym && (typeid(*sym) == symt)) {
      if(first) {
        first = 0;
        stream << "Symbol Table for \"" << sym->showType() << "\"" << endl;
      }

      stream << sym->toString() << endl;
    }
    sti++;
  }
  if(first)
    stream << "No symbols found" << endl << ends;
  return string(stream.str());
}

bool IsClearable(Value* value)
{
  return value->isClearable();
}

//------------------------------------------------------------------------
//
// 
void Symbol_Table::clear(const char *cpBelongsTo)
{
  iterator it;

  Value *pParent = find(cpBelongsTo);

  for(it = begin(); it != end();) {
    Value *value = *it;

    if(value && value->isClearable()
       // The following test needs to be implemented.
       // TSD & JRH 10JAN07 are discussing some design issues here.
       /* && (!pParent || value->belongsTo(pParent)) */
       ) {

      delete value;
      erase(it);
    }
    else {
      ++it;
    }
  }
//  remove_if(begin(), end(), IsClearable);
}

void Symbol_Table::Initialize() 
{
#if 0
  PopulateWithCommandLineSymbols();
#endif
}

void Symbol_Table::Reinitialize()
{
  clear(0);
}

void Symbol_Table::clear_all() 
{
  iterator it;

  // There's a design flaw here. A symbol may have been created
  // either globally or as a member of a class. In other words,
  // symbols don't have to be created via 'new'. Thus it's not
  // safe to assume that a symbol can be deleted with delete!
  /*
  for(it = begin(); it != end(); ++it)
    delete *it;
  */
  _Myt::clear();
}

Value * Symbol_Table::find(const string &s)
{
  const bool findDuplicates=false;
  iterator sti = FindIt(s); // .begin();

  Value *ret=0;
  while( sti != end()) {
      Value *val = *sti;
      if(val && val->name() == s) {
        if(!findDuplicates)
          return val;

        if(!ret) {
          ret = val;
        } else
          cout << "Found duplicate:" << val->show()<<endl;
      }

      ++sti;
    }

  //// --- Search the New Symbol Table
  if (!ret) {
    ret = gSymbolTable.findValue(s);
    if (ret) {
      cout << "Found " << ret->name() << " in new symbol table\n";
    }
  }
  return ret;

}


Symbol_Table::iterator
Symbol_Table::FindIt(const char *pszKey) {
  Value KeyValue(pszKey, "key value");
  return FindIt(&KeyValue);
}

Symbol_Table::iterator
Symbol_Table::FindIt(const string &sKey) {
  Value KeyValue(sKey.c_str(), "key value");
  return FindIt(&KeyValue);
}

Symbol_Table::iterator
Symbol_Table::FindIt(Value *key) {
  iterator it = lower_bound(begin( ), end( ),
    key, NameLessThan());
  if (it != end() &&
      (*it)->name() == key->name()) {
    return it;
  }
  return end();
}


//------------------------------------------------------------------------
// symbols
//
//

symbol::symbol(const char *_name)
{
  new_name(_name);
}
symbol::symbol(string &_name)
{
  new_name(_name);
}

symbol::~symbol(void)
{
}

Value* symbol::evaluate()
{
  string msg("symbol '");
  msg.append(this->name());
  msg.append("' of type '");
  msg.append(showType());
  msg.append("' cannot not be evaluated in an expression");
  throw Error( msg );
}

//------------------------------------------------------------------------
string symbol::toString()
{
  return showType();
}

//------------------------------------------------------------------------
node_symbol::node_symbol(Stimulus_Node *_sn)
  : symbol(0) , stimulus_node(_sn)
{
  if(stimulus_node)
    new_name(stimulus_node->name());
}

string node_symbol::toString(void)
{
  return string("node:")+name();
}

// Count LSB that are off
static int BitShiftCount( unsigned int uMask ) {
  unsigned int uBitMaskCount = 0;
  if( uMask != 0 ) {
    for ( int i = 0; i < 16; i++ ) {
      if( (uMask & (0x1 << i)) == 0 )
        uBitMaskCount++;
      else
        break;
    }
  }
  return uBitMaskCount;
}

//------------------------------------------------------------------------
// register_symbol
//
register_symbol::register_symbol(const register_symbol & regsym)
  : symbol(regsym.name_str.c_str()), reg(regsym.reg)
{
  m_uMask = regsym.m_uMask;
  m_uMaskShift = regsym.m_uMaskShift;

  if (name_str.empty()) {
    name_str = regsym.reg->name();
  }
}

register_symbol::register_symbol(const char *_name, Register *_reg)
  : symbol(_name), reg(_reg)
{
  setMask(reg);
  if (_name == NULL && reg != NULL) {
    name_str = _reg->name();
  }
}

register_symbol::register_symbol(const char *_name, Register *_reg,
                                 unsigned int uMask)
  : symbol(_name), reg(_reg)
{
  if(uMask == 0) {
    setMask(reg);
  }
  else {
    m_uMask = uMask;
    m_uMaskShift = BitShiftCount(m_uMask);
  }
  if (_name == NULL && reg != NULL) {
    name_str = _reg->name();
  }
}

register_symbol::register_symbol(Register *_reg)
  : symbol(_reg->name()), reg(_reg)
{
  setMask(reg);
}

void register_symbol::setMask(Register *pReg) {
  m_uMask = 0xff;
  for(unsigned int i = 1; i < pReg->register_size(); i++) {
    m_uMask <<= 8;
    m_uMask |= 0xff;
  }
  m_uMaskShift = BitShiftCount(m_uMask);
}

unsigned int register_symbol::getAddress()
{
  return reg ? reg->address : 0xffffffff;
}

unsigned int register_symbol::getBitmask(void) {
  return m_uMask;
}

string &register_symbol::name() const 
{
  return (string &)name_str;
}

char *register_symbol::name(char *buf, int len) {
  return symbol::name(buf, len);
}

string register_symbol::toString()
{
  if(reg) {
    char buff[256];
    char bits[256];

    reg->toBitStr(bits,sizeof(bits));

    int iDigits = reg->register_size() * 2;
    // turn off masked bits in dwRead
    unsigned int uValue = reg->get_value() & m_uMask;
    uValue  = uValue >> m_uMaskShift;
    if ( (unsigned int)((1<<(4*iDigits))-1) != m_uMask)
      snprintf(buff,sizeof(buff),"[0x%x] BITS 0x%0*x = 0x%0*x = 0b",
	       reg->address, iDigits, m_uMask,
	       iDigits, uValue);
    else
      snprintf(buff,sizeof(buff),"[0x%x] = 0x%0*x = 0b",
	       reg->address, 
	       iDigits, uValue);

    return string(buff) + string(bits);
  }
  return string("");
}

char *register_symbol::toString(char *return_str, int len)
{
  if(!return_str)
    return 0;

  if(reg)
    return reg->toString(return_str,len);

  *return_str=0;
  return return_str;
}
char *register_symbol::toBitStr(char *return_str, int len)
{
  if(!return_str)
    return 0;

  if(reg)
    return reg->toBitStr(return_str,len);

  *return_str=0;
  return return_str;
}


void  register_symbol::get(int &i)
{
  if(reg) {
    i = reg->get_value() & m_uMask;
    i >>= m_uMaskShift;
  }
  else
    i = 0;
}
void  register_symbol::get(gint64 &i)
{
  if(reg) {
    i = reg->get_value() & m_uMask;
    i >>= m_uMaskShift;
  }
  else
    i = 0;
}
void register_symbol::get(char *buffer, int buf_size)
{
  if(buffer) {
    int v;
    get(v);
    snprintf(buffer,buf_size,"%d",v);
  }

}

void register_symbol::get(Packet &p)
{
  if(reg) {
    int i;
    get(i);
    p.EncodeUInt32(i);
  }

}


void register_symbol::set(int new_value)
{
  if(reg)
    reg->putRV(RegisterValue(SetMaskedValue(new_value),0));
}

void register_symbol::set(Value *v)
{
  if(reg && v) {
    int i;
    v->get(i);
    reg->putRV(RegisterValue(SetMaskedValue(i),0));
  }
}

void register_symbol::set(const char *buffer, int buf_size)
{
  if(buffer) {

    int i;
    int converted=0;

    // if a straight decimal conversion fails, then try hexadecimal.

    converted = sscanf(buffer, "0x%x",  &i);

    if(!converted)
      converted = sscanf(buffer, "%d",  &i);

    if(!converted)
      converted = sscanf(buffer, "$%x",  &i);

    if(converted)
      set(i);

  }

}

unsigned int register_symbol::SetMaskedValue(unsigned int uValue) {
  
  Register *reg = getReg();
  unsigned int uCurrentValue = reg ? reg->get_value() & m_uMask: 0;
  // turn off masked bits in uCurrentValue
  uCurrentValue &= ~m_uMask;
  // turn off non-mask bits in uValue
  uValue = uValue << m_uMaskShift;
  uValue &= m_uMask;
  // map the passed in value into the target
  uCurrentValue |= ((long)uValue);
  return uCurrentValue;
}

void register_symbol::set(Packet &p)
{
  unsigned int i;
  if(p.DecodeUInt32(i)) {
    set((int)i);
  }

}

bool register_symbol::compare(ComparisonOperator *compOp, Value *rvalue)
{
  if(!compOp || !rvalue)
    return false;

  gint64 i,r;

  get(i);
  rvalue->get(r);

  if(i < r)
    return compOp->less();

  if(i > r)
    return compOp->greater();

  return compOp->equal();
}

void register_symbol::update(void) {
  reg->update();
}


symbol *register_symbol::copy()
{
  return new register_symbol(*this);
}
Register *register_symbol::getReg()
{
  return reg;
}

Value* register_symbol::evaluate() {
  gint64 v;
  get(v);
  return new Integer(v);
}

int register_symbol::set_break(ObjectBreakTypes bt, ObjectActionTypes at,
			       Expression *pExpr)
{
  return get_bp().set_break(bt,at, reg,pExpr);
}

int register_symbol::clear_break()
{
  cout << showType() << " objects breakpoints can only be cleared by 'clear #'\n   where # is the breakpoint number\n";
  return -1;
}

//------------------------------------------------------------------------
w_symbol::w_symbol(const char *_name, Register *_reg)
  : register_symbol(_name, _reg)
{
}
string w_symbol::toString()
{
  if(reg) {
    char buff[256];
    char bits[256];

    reg->toBitStr(bits,sizeof(bits));

    snprintf(buff,sizeof(buff)," = 0x%02x = 0b", reg->get_value() & 0xff);

    return string(buff) + string(bits);
  }
  return string("");
}

//------------------------------------------------------------------------
ioport_symbol::ioport_symbol(PortRegister *_ioport)
  : register_symbol(_ioport->name().c_str(), _ioport)
{
}

//------------------------------------------------------------------------
AddressSymbol::AddressSymbol(Processor *pCpu, const char *_name, unsigned int _val)
  :  Integer(_val)

{
  new_name(_name);
  cpu = pCpu;
}

string AddressSymbol::toString()
{
  char buf[256];
  int i = (int)getVal();
  snprintf(buf,sizeof(buf), " at address %d = 0x%X",i,i);
  
  return string(buf);
}

Value* AddressSymbol::evaluate()
{
  return copy();
}
int AddressSymbol::set_break(ObjectBreakTypes bt, ObjectActionTypes at,
                             Expression *pExpr)
{
  if (bt == gpsimObject::eBreakExecute && cpu)
    return get_bp().set_execution_break((Processor*)cpu,getVal(),pExpr);

  return -1;
}

LineNumberSymbol::LineNumberSymbol(Processor *pCpu,
                                   const char *_name, 
                                   unsigned int _val)
  :  AddressSymbol(pCpu,_name,_val)
{
  if(!_name) {
    char buf[64];
    snprintf(buf,sizeof(buf), "line_%04x",_val);
    new_name(buf);
  }

}
//------------------------------------------------------------------------
module_symbol::module_symbol(Module *_module, const char *_name)
  : symbol(_name), module(_module)
{
}

Value *module_symbol::copy()
{
  cout << "copying module symbol: " << name() << endl;

  return new module_symbol(module,name().c_str());
}
void module_symbol::set(const char *cP,int len)
{
  throw new Error("object cannot be assigned a value\n");
}

void module_symbol::get(char *cP, int len)
{
  if(cP) {
    *cP = 0;
  }
}

string module_symbol::description()
{
  return module ? module->description() : string("no description");
}

string module_symbol::toString()
{
  return module ? module->toString() : name();
}


//------------------------------------------------------------------------

attribute_symbol::attribute_symbol(Module *_module, Value *_attribute)
  : module_symbol(_module, 0) , attribute(_attribute)
{
  if(module && attribute) {
    char buf[256];

    snprintf(buf,sizeof(buf),"%s.%s",module->name().c_str(), attribute->name().c_str());
    if(verbose)
      cout << "creating attribute symbol named: " << buf << endl;
    new_name(buf);
    attribute->new_name(buf);
  } 
}

string attribute_symbol::toString()
{

  if(attribute)
    return attribute->toString();
  else
    return string("(null)");
}

char *attribute_symbol::toString(char *return_str, int len)
{
  if(attribute)
    return attribute->toString(return_str,len);
  else if(return_str)
    *return_str=0;

  return return_str;
}
char *attribute_symbol::toBitStr(char *return_str, int len)
{
  if(attribute)
    return attribute->toBitStr(return_str,len);
  else if(return_str)
    *return_str=0;

  return return_str;
}

string attribute_symbol::description()
{

  if(attribute)
    return attribute->description();
  else
    return string("no attribute");  // <-- this has to be an error
}

Value *attribute_symbol::copy()
{
  if (attribute)
    return attribute->copy();
  return copy();
}

Value* attribute_symbol::evaluate()
{
  return copy();
}

void attribute_symbol::set(double d)
{
  if(attribute)
    attribute->set(d);
}
void attribute_symbol::set(gint64 i)
{
  if(attribute)
    attribute->set(i);
}
void attribute_symbol::set(int i)
{
  if(attribute)
    attribute->set(i);
}
void attribute_symbol::set(Value *v)
{
  if(attribute)
    attribute->set(v);
}
void attribute_symbol::set(const char *cp,int len)
{
  if(attribute)
    attribute->set(cp,len);
}
void attribute_symbol::set(Expression *e)
{
  if(attribute)
    attribute->set(e);
}
void attribute_symbol::set(Packet &p)
{
  if(attribute)
    attribute->set(p);
}

void attribute_symbol::get(int &i)
{
  if(attribute)
    attribute->get(i);
}
void attribute_symbol::get(gint64 &i)
{
  if(attribute)
    attribute->get(i);
}
void attribute_symbol::get(double &d)
{
  if(attribute)
    attribute->get(d);
}
void attribute_symbol::get(char *c, int len)
{
  if(attribute)
    attribute->get(c,len);
}

void attribute_symbol::get(Packet &p)
{
  if(attribute)
    attribute->get(p);
}

void attribute_symbol::get(Value **v)
{
  if(attribute && v)
    *v = attribute;
}


void attribute_symbol::set_xref(Value *v)
{
  cout << "Warning function:" << __FUNCTION__ << " no longer exists\n";
  //if(attribute)
  //  attribute->set_xref(v);
  //Value::set_xref(v);
}

Value *attribute_symbol::get_xref()
{
  cout << "Warning function:" << __FUNCTION__ << " no longer exists\n";
  //if(attribute)
  //  return attribute->get_xref();
  //return Value::get_xref();
  return 0;
}

//------------------------------------------------------------------------
stimulus_symbol::stimulus_symbol(stimulus *_s)
  : symbol(_s->name().c_str()), s(_s)
{
}

//------------------------------------------------------------------------
string &stimulus_symbol::name() const
{
  if(s)
    return s->name();

  return Value::name();
}

char *stimulus_symbol::name(char *pName, int len) {
  if(s)
    return s->name(pName, len);

  return Value::name(pName, len);
}

void stimulus_symbol::new_name(const char *pNewName) {
  if(s)
    return s->new_name(pNewName);

  return Value::new_name(pNewName);
}

void stimulus_symbol::new_name(string &sNewName) {
  if(s)
    return s->new_name(sNewName);

  return Value::new_name(sNewName);
}


string stimulus_symbol::toString()
{
  if(s)
    return s->toString();
  return name();
}

//------------------------------------------------------------------------
/*
val_symbol::val_symbol(gpsimValue *v)
  : symbol((char*)0)
{
  if(!v)
    throw string(" val_symbol");

  val = v;
}
string val_symbol::toString()
{
  return val->toString();
}

void val_symbol::get(int &i)
{
  if (val)
    i = val->get_value();
  else
    i = 0;
}

void val_symbol::get(gint64 &i)
{
  if (val)
    i = val->get_value();
  else
    i = 0;
}
void val_symbol::set(int new_value)
{
  if(val)
    val->put_value(new_value);
}
void val_symbol::set(gint64 new_value)
{
  if(val) 
    val->put_value((int)new_value);
}
string &val_symbol::name(void) const
{
  return val->name();
}
symbol *val_symbol::copy()
{
  return new val_symbol(val);
}
bool val_symbol::compare(ComparisonOperator *compOp, Value *rvalue)
{
  if(!compOp || !rvalue)
    return false;

  gint64 i,r;

  get(i);
  rvalue->get(r);

  if(i < r)
    return compOp->less();

  if(i > r)
    return compOp->greater();

  return compOp->equal();
}
*/

#endif
