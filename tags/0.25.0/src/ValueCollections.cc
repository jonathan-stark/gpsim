/*
   Copyright (C) 1998-2004 Scott Dattalo

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

#include "ValueCollections.h"
#include "symbol.h"
#include "registers.h"

IIndexedCollection::IIndexedCollection(const char *pName, 
				       const char *pDesc,
				       int iAddressRadix) 
  : Value(pName, pDesc)
{
  SetAddressRadix(iAddressRadix);
}

IIndexedCollection::IIndexedCollection(int iAddressRadix) {
  SetAddressRadix(iAddressRadix);
}

void IIndexedCollection::SetAddressRadix(int iRadix) {
  m_iAddressRadix = iRadix;
  if(iRadix == 16) {
    strcpy(m_szPrefix, "$");
  }
  else {
    m_szPrefix[0] = 0;
  }
}

void IIndexedCollection::Set(Value * pValue) {
  unsigned int  uUpper = GetUpperBound() + 1;
  for(unsigned int uIndex = GetLowerBound(); uIndex < uUpper; uIndex++) {
    SetAt(uIndex, pValue);
  }
}

void IIndexedCollection::SetAt(ExprList_t* pIndexers, Expression *pExpr) {
  ExprList_t::iterator it;
  ExprList_t::iterator itEnd = pIndexers->end();
  Value * pValue = pExpr->evaluate();
//    _CT * pCTValue = dynamic_cast<_CT*>(pValue);
//    if(pCTValue != NULL) {
    for(it = pIndexers->begin(); it != itEnd; it++) {
      Value * pIndex = (*it)->evaluate();
      Integer *pIntIndex = dynamic_cast<Integer*>(pIndex);
      if(pIntIndex != NULL) {
        SetAt(int(*pIntIndex), pValue);
      }
      else {
        AbstractRange *pRange = dynamic_cast<AbstractRange*>(pIndex);
        if(pRange) {
          unsigned uEnd = pRange->get_rightVal() + 1;
          for(unsigned int uIndex = pRange->get_leftVal(); uIndex < uEnd; uIndex++) {
            SetAt(uIndex, pValue);
          }
        }
        else {
          /*
          register_symbol *pReg = dynamic_cast<register_symbol*>(pIndex);
          if(pReg) {
            SetAt(pReg->getReg()->address, pValue);
          }
          else {
            throw Error("indexer not valid");
          }
          */
          Register *pReg = dynamic_cast<Register*>(pIndex);
          if (pReg) 
            SetAt(pReg->getAddress(), pValue);
          else
            throw Error("indexer not valid");
        }
      }
      if(pIndex != NULL) {
        delete pIndex;
      }
    }
//    }
//    else {
//    }
  delete pValue;
}

char *IIndexedCollection::toString(char *pBuffer, int len) {
  return strncpy(pBuffer, toString().c_str(), len);
}

string IIndexedCollection::toString() {
  int iColumnWidth = 0;
  vector<string> asIndexes;
  vector<string> asValue;
  ConsolidateValues(iColumnWidth, asIndexes, asValue);
  return toString(iColumnWidth, asIndexes, asValue);
}

string IIndexedCollection::toString(ExprList_t* pIndexerExprs) {
  try {
    ostringstream sOut;
    if(pIndexerExprs==NULL)  {
      sOut << toString() << ends;
      return sOut.str();
    }
    else {
      ExprList_t::iterator it;
      ExprList_t::iterator itEnd = pIndexerExprs->end();
      for(it = pIndexerExprs->begin(); it != itEnd; it++) {
        Value * pIndex = (*it)->evaluate();
        AbstractRange *pRange = dynamic_cast<AbstractRange*>(pIndex);
        if(pRange) {
          unsigned uEnd = pRange->get_rightVal() + 1;
          for(unsigned int uIndex = pRange->get_leftVal(); uIndex < uEnd; uIndex++) {
            Value &Value = GetAt(uIndex);
            sOut << Value.name() << " = " << Value.toString() << endl;
          }
          continue;
        }
        String *pName = dynamic_cast<String*>(pIndex);
        Integer *pInt = pName ? 
          globalSymbolTable().findInteger(pName->getVal()) :
          dynamic_cast<Integer*>(pIndex);

        Integer temp(0);
        if(pInt == NULL) {
          // This is a temp workaround. I (JR) would expect a register symbol
          // evaluate to an Integer object containing the value of the
          // register. It currently returns an object that is a copy 
          // of the register_symbol object.
          /*
          register_symbol *pReg = dynamic_cast<register_symbol*>(pIndex);
          if(pReg) {
            gint64 i;
            pReg->get(i);
            temp.set(i);
            pInt = &temp;
          }
          */

          Register *pReg = dynamic_cast<Register*>(pIndex);
          if(pReg) {
            gint64 i = pReg->get_value();
            temp.set(i);
            pInt = &temp;
          }
        }
        if(pInt) {
          unsigned int uIndex = (unsigned int)pInt->getVal();
          if(bIsIndexInRange(uIndex)) {
            Value &Value = GetAt(uIndex);
            sOut << Value.name() << " = " << Value.toString() << endl;
          }
          else {
            sOut << "Error: Index " << uIndex << " is out of range" << endl;
          }
        }
        else {
          sOut << "Error: The index specified for '"
            << name() << "' does not contain a valid index."
            << endl;
        }
        delete pIndex;
      }
    }
    sOut << ends;
    return sOut.str();
  }
  catch(Error e) {
    return e.toString();
  }
}

void IIndexedCollection::PushValue(int iFirstIndex, int iCurrentIndex,
                                   Value *pValue,
                                   vector<string> &asIndexes, vector<string> &asValue) {
  ostringstream sIndex;
  if(m_iAddressRadix == 16) {
    sIndex << hex; 
  }
  sIndex << Value::name() << "[" << m_szPrefix << iFirstIndex;
  if(iFirstIndex != iCurrentIndex) {
    sIndex << ".." << m_szPrefix << iCurrentIndex;
  }
  sIndex << "]" << ends;
  asIndexes.push_back(string(sIndex.str()));
  asValue.push_back(pValue->toString());
}

string IIndexedCollection::ElementIndexedName(unsigned int iIndex) {
  ostringstream sIndex;
  if(m_iAddressRadix == 16) {
    sIndex << hex; 
  }
  sIndex << Value::name() << "[" << m_szPrefix << iIndex;
  sIndex << "]" << ends;
  return sIndex.str();
}

string IIndexedCollection::toString(int iColumnWidth, vector<string> &asIndexes,
                                    vector<string> &asValue) {
  ostringstream sOut;
  vector<string>::iterator itValue;
  vector<string>::iterator itElement;
  vector<string>::iterator itElementEnd = asIndexes.end();
  // Dump the consolidated element list
  for(itElement = asIndexes.begin(), itValue = asValue.begin();
      itElement != itElementEnd;
      itElement++, itValue++) {
    sOut.width(iColumnWidth);
    sOut.setf(ios_base::left);
    sOut << (*itElement);
    sOut << " = ";
    sOut << (*itValue);
    if(itElement + 1 != itElementEnd)
      sOut << endl;
  }
  sOut << ends;
  return sOut.str();
}

Integer * IIndexedCollection::FindInteger(const char *s)
{
  return globalSymbolTable().findInteger(s);
}

