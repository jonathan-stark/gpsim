#include <string>
#include <list>
#include <vector>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <assert.h>

#include "expr.h"
#include "operator.h"
#include "errors.h"
#include "symbol.h"
#include "ValueCollections.h"
#include "processor.h"

using namespace std;

//------------------------------------------------------------------------

Expression::Expression(void)
{


}


Expression:: ~Expression(void)
{
}

/*****************************************************************
 * The LiteralArray class.
 */
LiteralArray::LiteralArray(ExprList_t *pExprList)
{
  m_pExprList = pExprList;
}

LiteralArray::~LiteralArray()
{
}

Value *LiteralArray::evaluate()
{
  return new Boolean(true);
}

string LiteralArray::toString()
{
  return string("FixMe");
}

/*****************************************************************
 * The LiteralBoolean class.
 */
LiteralBoolean::LiteralBoolean(Boolean* value_)
{
  //if (value_==0) {
  //  throw new Internal ("LiteralBoolean::LiteralBoolean(): NULL value ptr");
  //}
  assert(value_ != 0);
  value = value_;
}

LiteralBoolean::~LiteralBoolean()
{
}

Value* LiteralBoolean::evaluate()
{
  bool b;
  value->get(b);
  return new Boolean(b);
}

string LiteralBoolean::toString()
{
  return value->toString();
}


//------------------------------------------------------------------------

LiteralInteger::LiteralInteger(Integer* newValue)
  : Expression()
{
  assert(newValue != 0);
  value = newValue;
}

 LiteralInteger::~LiteralInteger()
{
  delete value;
}

Value* LiteralInteger::evaluate()
{
  gint64 i;
  value->get(i);
  return new Integer(i);
}

string LiteralInteger::toString()
{
  return value->toString();
}

int LiteralInteger::set_break(ObjectBreakTypes bt, ObjectActionTypes at, Expression *expr)
{
  return value ? value->set_break(bt,at,expr) : -1;
}

/*****************************************************************
 * The LiteralFloat class.
 */
LiteralFloat::LiteralFloat(Float* value_)
{
  //if (value_==0) {
  //  throw new Internal ("LiteralFloat::LiteralFloat(): NULL value ptr");
  //}
  assert(value_ != 0);
  value = value_;
}

LiteralFloat::~LiteralFloat()
{
  delete value;
}

Value* LiteralFloat::evaluate()
{
  double d;
  value->get(d);
  return new Float(d);
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
  delete value;
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

LiteralSymbol::LiteralSymbol(gpsimObject *_sym)
{
  sym = dynamic_cast<Value *>(_sym);
  assert(sym != 0);
}

LiteralSymbol::~LiteralSymbol()
{
}

Value* LiteralSymbol::evaluate()
{
  return  sym->evaluate();
}

Value *LiteralSymbol::GetSymbol()
{
  return sym;
}

string LiteralSymbol::toString()
{
  if(sym)
    return sym->name();

  return string("");
}

int LiteralSymbol::set_break(ObjectBreakTypes bt, ObjectActionTypes at, Expression *expr)
{
  return sym ? sym->set_break(bt,at,expr) : -1;
}

int LiteralSymbol::clear_break()
{
  return sym ? sym->clear_break() : -1;
}
/*****************************************************************
 * The LiteralSymbol class
 */
IndexedSymbol::IndexedSymbol(gpsimObject *pSymbol, ExprList_t*pExprList)
  : m_pExprList(pExprList)
{
  m_pSymbol = dynamic_cast<Value *>(pSymbol);
  assert(m_pSymbol != 0);
  assert(pExprList != 0);
}

IndexedSymbol::~IndexedSymbol()
{
}

Value* IndexedSymbol::evaluate()
{
  // Indexed symbols with more than one index expression
  // cannot be evaluated
  if(m_pExprList->size() > 1) {
    // Could return an AbstractRange
    throw Error("Indexed variable evaluates to more than one value");
  }

  IIndexedCollection *pIndexedCollection =
    dynamic_cast<IIndexedCollection *>(m_pSymbol);
  if (!pIndexedCollection)
    throw Error("Cannot index this variable");
  else {
    Value *pV=m_pExprList->front()->evaluate();
    unsigned int ui = *pV;

    return pIndexedCollection->GetAt(ui).copy();
  }
}

string IndexedSymbol::toString() {
  IIndexedCollection *pIndexedCollection =
    dynamic_cast<IIndexedCollection *>(m_pSymbol);
  if(pIndexedCollection == NULL) {
    return string("The symbol ") + m_pSymbol->name() + " is not an indexed variable";
  }
  else {
    ostringstream sOut;
    sOut << pIndexedCollection->toString(m_pExprList) << ends;
    return sOut.str();
  }
  return string("IndexedSymbol not initialized");
}


/*****************************************************************
 * The RegisterExpression class
 *
 * The literal symbol is a thin 'literal' wrapper for the symbol class.
 * The command line parser uses RegisterExpression whenever an expression
 * encounters a symbol. 
 */

RegisterExpression::RegisterExpression(unsigned int uAddress)
  : m_uAddress(uAddress)
{
}

RegisterExpression::~RegisterExpression()
{
}

Value* RegisterExpression::evaluate()
{
  Register *pReg = get_active_cpu()->rma.get_register(m_uAddress);
  if(pReg) {
    return new Integer(pReg->get_value());
  }
  else {
    static char sFormat[] = "reg(%d) is not a valid register";
    char sBuffer[sizeof(sFormat) + 10];
    sprintf(sBuffer, sFormat, m_uAddress);
    throw Error(string(sBuffer));
  }
}

string RegisterExpression::toString()
{
  char sBuffer[10];
  sprintf(sBuffer, "%d", m_uAddress);
  return string(sBuffer);
}
