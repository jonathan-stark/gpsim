#include <string>
#include <list>
#include <vector>
#include <iostream>

#include "expr.h"
#include "operator.h"
#include "errors.h"
#include "symbol.h"

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
  return new Boolean(value->getVal(), true);
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
  return new Integer(value->getVal(), true);
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
  return new Float(value->getVal(), true);
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
  return new String(value->getVal(), true);
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

LiteralSymbol::LiteralSymbol(symbol *_sym)
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

