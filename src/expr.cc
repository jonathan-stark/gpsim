#include <string>
#include <list>
#include <vector>
#include <iostream>
#include <sstream>

#include "expr.h"
#include "operator.h"
#include "errors.h"
#include "symbol.h"
#include "ValueCollections.h"

using namespace std;

//------------------------------------------------------------------------

Expression::Expression(void)
{


}


Expression:: ~Expression(void)
{
}

/*****************************************************************
 * The LiteralBoolean class.
 */
LiteralBoolean::LiteralBoolean(Boolean* value_)
{
  //if (value_==0) {
  //  throw new Internal ("LiteralBoolean::LiteralBoolean(): NULL value ptr");
  //}
  value = value_;
}

LiteralBoolean::~LiteralBoolean()
{
}

Value* LiteralBoolean::evaluate()
{
  return new Boolean(value->getVal());
}

string LiteralBoolean::toString()
{
  return value->toString();
}


//------------------------------------------------------------------------

LiteralInteger::LiteralInteger(Integer* newValue)
  : Expression()
{
  value = newValue;
}

 LiteralInteger::~LiteralInteger()
{
  delete value;
 
}

Value* LiteralInteger::evaluate()
{
  return new Integer(value->getVal());
}

string LiteralInteger::toString()
{
  return value->toString();
}

/*****************************************************************
 * The LiteralFloat class.
 */
LiteralFloat::LiteralFloat(Float* value_)
{
  //if (value_==0) {
  //  throw new Internal ("LiteralFloat::LiteralFloat(): NULL value ptr");
  //}
  value = value_;
}

LiteralFloat::~LiteralFloat()
{
}

Value* LiteralFloat::evaluate()
{
  return new Float(value->getVal());
}

string LiteralFloat::toString()
{
  return value->toString();
}


/*****************************************************************
 * The LiteralString class.
 */
LiteralString::LiteralString(String* value_)
{
  //if (value_==0) {
  //  throw new Internal ("LiteralString::LiteralString(): NULL value ptr");
  //}
  value = value_;
}

LiteralString::~LiteralString()
{
}

Value* LiteralString::evaluate()
{
  return new String(value->getVal());
}

string LiteralString::toString()
{
  return value->toString();
}


/*****************************************************************
 * The LiteralSymbol class
 *
 * The literal symbol is a thin 'literal' wrapper for the symbol class.
 * The command line parser uses LiteralSymbol whenever an expression
 * encounters a symbol. 
 */

LiteralSymbol::LiteralSymbol(Value *_sym)
  : sym(_sym)
{
}

LiteralSymbol::~LiteralSymbol()
{
}

Value* LiteralSymbol::evaluate()
{
  return  sym->copy();
}

string LiteralSymbol::toString()
{
  if(sym)
    return sym->name();

  return string("");
}

IndexedSymbol::IndexedSymbol(Value *pSymbol, ExprList_t*pExprList)
: m_pSymbol(pSymbol), m_pExprList(pExprList) {
}

IndexedSymbol::~IndexedSymbol() {
}

Value* IndexedSymbol::evaluate() {
  // Indexed symbols with more than one index expression
  // cannot be evaluated
  if(m_pExprList->size() > 1) {
    // Could return an AbstractRange
    throw Error("Indexed variable evaluates to more than one value");
  }
  else {
    return m_pExprList->front()->evaluate();
  }
}

string IndexedSymbol::toString() {
  IIndexedCollection *pIndexedCollection =
    dynamic_cast<IIndexedCollection *>(m_pSymbol);
  if(pIndexedCollection == NULL) {
    return string("The symbol ") + m_pSymbol->name() + " is not an indexed variable";
  }
  else {
    try {
      ostringstream sOut;
      if(m_pExprList==NULL)  {
        sOut << pIndexedCollection->toString() << '\000';
        return sOut.str();
      }
      else {
        unsigned int uUpperBound = pIndexedCollection->GetUpperBound();
        ExprList_t::iterator it;
        ExprList_t::iterator itEnd = m_pExprList->end();
        for(it = m_pExprList->begin(); it != itEnd; it++) {
          Value * pIndex = (*it)->evaluate();
          AbstractRange *pRange = dynamic_cast<AbstractRange*>(pIndex);
          if(pRange) {
            cout << "indexing using a range is not implemented" << endl;
          }
          Integer *pInt;
          String *pName = dynamic_cast<String*>(pIndex);
          if(pName) {
            pInt = get_symbol_table().findInteger(pName->getVal());
          }
          else {
            pInt = dynamic_cast<Integer*>(pIndex);
          }
          Integer temp(0);
          if(pInt == NULL) {
            // This is a temp workaround. I would expect a register symbol
            // evaluate to an Integer object containing the value of the
            // register. It currently returns an object that is a copy 
            // of the register_symbol object.
            register_symbol *pReg = dynamic_cast<register_symbol*>(pIndex);
            if(pReg) {
              gint64 i;
              pReg->get(i);
              temp.set(i);
              pInt = &temp;
            }
          }
          if(pInt) {
            unsigned int uIndex = (unsigned int)pInt->getVal();
            if(uIndex <= uUpperBound) {
              Value &Value = pIndexedCollection->GetAt(uIndex);
              sOut << Value.name() << " = " << Value.toString() << endl;
            }
            else {
              sOut << "Error: Index " << uIndex << " is out of range" << endl;
            }
          }
          else {
            cout << "Error: The index specified for '"
              << m_pSymbol->name() << "' does not contain a valid index."
              << endl;
          }
          delete pIndex;
        }
      }
      sOut << '\000';
      return sOut.str();
    }
    catch(Error e) {
      return e.toString();
    }
  }
  return string("IndexedSymbol not initialized");
}

