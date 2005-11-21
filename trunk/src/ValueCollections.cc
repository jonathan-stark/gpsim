/*
   Copyright (C) 1998-2004 Scott Dattalo

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

#include "ValueCollections.h"
#include "symbol.h"

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

void IIndexedCollection::set(Value * pValue) {
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

Integer * IIndexedCollection::findInteger(const char *s) {
  return get_symbol_table().findInteger(s);
}

