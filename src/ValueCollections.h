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
/*
This file originated by J.R. Heisey

*/

#ifndef __VALUECOLLECTIONS_H__
#define __VALUECOLLECTIONS_H__

#include <glib.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdio>

#include "value.h"
#include "expr.h"
#include "operator.h"
#include "errors.h"

/*
    The IIndexedCollection class is an abstract class used to
    expose an array of values of a given name to the command
    prompt interface. These arrays are not dynamically resizable.
    The scripting language does not provide a way of allocating
    an array at gpsim runtime. These arrays are only definable
    at gpsim compile time.

    Values based on the class Value type are implemented via the 
    template class IndexedCollection. The first class Value based
    array is implemented by CIndexedIntegerCollection.
    
    The CIndexedIntegerCollection class provides its own storage
    for the Integer values and is best used for Integer array based
    attributes.

    To derive your own arrays you will primarily need to derive from
    IIndexedCollection and define implementations for all the pure
    virutal functions. 
    
    I (JR) have provided an example of an array implementation as an example.
    The class RegisterCollection is declared in register.h and is 
    an example of how to expose data values that are not based on
    the Value class. The RegisterCollection exposes a gpsim command
    prompt array variable called ramData that exposes the data value
    in simulated RAM memory. When you type 'ramData' from the command
    prompt, all memory contents will be displayed. To display one data
    value type 'ramData[expr]' were expr may be an integer value or
    and expression that evaluates to an integer value. (i.e. To see
    the contents of the RAM where X contains the value of the RAM
    address type 'ramData[X] at the command prompt.)

    You may also type 'ramData[expr_list]' where expr_list are one or
    more expressions delimited by commas.
    all of the 
      ConsolidateValues()



*/
class IIndexedCollection : public Value {
public:
  IIndexedCollection(const char *pName, const char *pDesc, int iAddressRadix = 10);
  IIndexedCollection(int iAddressRadix = 10);
  virtual unsigned int GetSize() = 0;
  virtual Value &GetAt(unsigned int uIndex, Value *pValue=0) = 0;
  virtual void SetAt(unsigned int uIndex, Value *pValue) = 0;
  virtual void Set(Value *pValue);
  virtual void SetAt(ExprList_t* pIndexers, Expression *pExpr);
  virtual unsigned int GetLowerBound() = 0;
  virtual unsigned int GetUpperBound() = 0;
  virtual bool bIsIndexInRange(unsigned int uIndex) {
    return uIndex >= GetLowerBound() &&  uIndex <= GetUpperBound();
  }
  void          SetAddressRadix(int iRadix);

  virtual string toString(ExprList_t* pIndexerExprs);
  virtual char * toString(char *pBuffer, int len);
  virtual string toString();

  inline Value & operator[](unsigned int uIndex) {
    return GetAt(uIndex);
  }


protected:
  virtual void ConsolidateValues(int &iColumnWidth,
                                 vector<string> &aList,
                                 vector<string> &aValue) = 0;

  template<class _ValueType>
  void ConsolidateValues(int &iColumnWidth,
                         vector<string> &aList,
                         vector<string> &aValue,
                         _ValueType *_V = 0) {
    unsigned int  uFirstIndex = GetLowerBound();
    unsigned int  uIndex;
    unsigned int  uUpper = GetUpperBound() + 1;
    _ValueType    LastValue((_ValueType&)GetAt(uFirstIndex));
    for(uIndex = uFirstIndex + 1; uIndex < uUpper; uIndex++) {
      _ValueType &curValue = (_ValueType&)GetAt(uIndex);
      if(LastValue != curValue) {
        PushValue(uFirstIndex, uIndex - 1,
                  &LastValue, aList, aValue);
        iColumnWidth = max(iColumnWidth, (int)aList.back().size());
        uFirstIndex = uIndex;
        LastValue = curValue;
      }
    }
    uIndex--;
    // Record the last set of elements
    if(uFirstIndex <= uIndex) {
      PushValue(uFirstIndex, uIndex,
                &LastValue, aList, aValue);
      iColumnWidth = max(iColumnWidth, (int)aList.back().size());
    }
  }
  void PushValue(int iFirstIndex, int iCurrentIndex,
                 Value *pValue,
                 vector<string> &asIndexes, vector<string> &asValue);
  virtual string toString(int iColumnWidth, vector<string> &asIndexes,
                          vector<string> &asValue);
//  virtual string toString(ExprList_t* pIndexers, Expression *pExpr);
  virtual string ElementIndexedName(unsigned int iIndex);

  Integer *     FindInteger(const char *s);

protected:
  char  m_szPrefix[3];
  int   m_iAddressRadix;
};



template<class _CT, class _ST>
class IndexedCollection : public IIndexedCollection {
protected:
  typedef vector<_CT*> VectorType;
  typedef _CT* _ElementType;

public:
  explicit IndexedCollection(
    const char * pName = NULL, const char *pDesc = 0) {
    m_uLower = 0;
    if(pName == NULL)
      pName = "unnamed";
    new_name(pName);
    set_description(pDesc);
  }

  explicit IndexedCollection(unsigned int uSize, _ST stDefValue,
    const char * pName = NULL, const char *pDesc = 0) {
    m_uLower = 0;
    if(pName == NULL)
      pName = "unnamed";
    Value::new_name(pName);
    set_description(pDesc);
    m_Array.reserve(uSize);
    string sName;
    char szIndex[12];
    for(unsigned int uIndex = 0; uIndex < uSize; uIndex++) {
      sName = pName;
      sprintf(szIndex, "[%d]", uIndex + m_uLower);
      sName.append(szIndex);
      // Hmm... Do we really want to create new object for every array entry?
      m_Array.push_back(new _CT(sName.c_str(), stDefValue, pDesc));
    }
  }

  virtual unsigned int GetSize() {
    return (unsigned int)m_Array.size();
  }

//  void SetSize(unsigned int) {
//  }

  virtual Value &GetAt(unsigned int uIndex, Value * /*placeholder*/) {
    return GetAt(uIndex);
  }

  _CT &GetAt(unsigned int uIndex) {
    if(uIndex <= GetUpperBound() && uIndex >= m_uLower) {
      return static_cast<_CT&>(*(m_Array.at(uIndex - m_uLower)));
    }
    else {
      throw Error(string("Error: index out of range"));
    }
  }

  virtual void SetAt(unsigned int uIndex, Value *pValue) {
    _CT * pCTValue = dynamic_cast<_CT*>(pValue);
    if(pCTValue != NULL) {
      SetAt(uIndex, pCTValue);
    }
    else {
    }
  }

  void SetAt(unsigned int uIndex, _CT *pValue)
  {
    if((uIndex + 1 - m_uLower) < m_Array.size() && uIndex >= m_uLower) {

      _ST stValue;
      pValue->get(stValue);

      _CT * pElement = static_cast<_CT*>(m_Array.at(uIndex - m_uLower));
      if (pElement)
	pElement->set(stValue);

    } else {

      char szIndex[10];
      sprintf(szIndex, "%d", uIndex);
      string sMsg("invalid array index of ");
      sMsg.append(szIndex);
      throw Error(sMsg);
    }
  }

  inline _CT & operator[](unsigned int uIndex) {
    return GetAt(uIndex);
  }

  _CT & operator[](Expression *pIndexExpr) {
    Value *pIndex = pIndexExpr->evaluate();
    String *pStr;
    if((pStr = dynamic_cast<String*>(pIndex)) != NULL) {
      // pIndex = get_symbol_table().findInteger(pStr->getVal());
      pIndex = FindInteger(pStr->getVal());
    }
    if(dynamic_cast<Integer*>(pIndex) != NULL) {
      unsigned int uIndex = (unsigned int)*pIndex;
      return GetAt(uIndex);
    }
//    else if(dynamic_cast<String*>(pIndex) != NULL) {
      // Future: Implement a indexed collection that accepts
      // strings where the string is used for a hashed based
      // collection.
//    }
    else {
      string sMsg;
      sMsg = "Indexer expression does not evaluate to an Integer for " + name(); 
      throw sMsg.c_str();
    }
  }

  virtual void ConsolidateValues(int &iColumnWidth,
                                 vector<string> &aList,
                                 vector<string> &aValue) {
    typename VectorType::iterator it;
    typename VectorType::iterator itLastEqualed;
    typename VectorType::iterator itEnd = m_Array.end();
    unsigned int iCurrentIndex = m_uLower, iFirstIndex = m_uLower;
    itLastEqualed = itEnd;
    // The purpose of the two loops it to collapse consecutive 
    // elements of equal value onto one display line.
    // This loop examines every element's value and records
    // the value in aValue. aList is used to record an
    // appropriate label for one or more elements.
    for(it = itLastEqualed = m_Array.begin(); it != itEnd; ++it) {
      ostringstream sIndex;
      if(*(_CT*)(*itLastEqualed) != *(_CT*)(*it)) {
        PushValue(iFirstIndex, iCurrentIndex - 1,
                  *itLastEqualed, aList, aValue);
        iFirstIndex = iCurrentIndex;
        iColumnWidth = max(iColumnWidth, (int)aList.back().size());
        itLastEqualed = it;
      }
      iCurrentIndex++;
    }
    iCurrentIndex--;
    // Record the last set of elements
    if(iFirstIndex <= iCurrentIndex) {
      PushValue(iFirstIndex, iCurrentIndex,
                *itLastEqualed, aList, aValue);
      iColumnWidth = max(iColumnWidth, (int)aList.back().size());
    }
  }
  
  unsigned int GetLowerBound() {
     return m_uLower;
  }

  unsigned int GetUpperBound() {
     return (unsigned int)m_Array.size();
  }
  /* --- This isn't used and I (Scott) don't understand the intent...
  void SetLowerBound(unsigned int uIndex) {
    m_uLower = uIndex;
    // Now iterate through the elements and change
    // and pre-formatted element names.
    typename VectorType::iterator it;
    typename VectorType::iterator itEnd = m_Array.end();
    unsigned int iNameBufferLen = 256;
    char *pNameBuffer = (char*)malloc(iNameBufferLen);
    unsigned int uLoopIndex = m_uLower;
    for(it = m_Array.begin(); it != itEnd; it++) {
      if(iNameBufferLen < (*it)->name().size() + 2) {
        pNameBuffer = (char*)realloc(pNameBuffer, iNameBufferLen += 100);
      }
      sprintf(pNameBuffer, "%s[%d]", name().c_str(), uLoopIndex);
      (*it)->new_name(pNameBuffer);
      uLoopIndex++;
    }
    free(pNameBuffer);
  }
  */

protected:
  void push_back(_CT *p) {
    m_Array.push_back(p);
  }
  unsigned int m_uLower;

private:
  VectorType m_Array;
};

class CIndexedIntegerCollection : public IndexedCollection<Integer, gint64> {
public:
  CIndexedIntegerCollection(unsigned int uSize, gint64 stDefValue,
    const char * pName = NULL, const char *pDesc = 0)
    : IndexedCollection<Integer, gint64>(uSize, stDefValue,
                                         pName, pDesc ) {
    }
};

#endif // __VALUECOLLECTIONS_H__
