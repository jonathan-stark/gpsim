#include <string>
#include <list>
#include <vector>
#include <iostream>

#include "expr.h"
#include "errors.h"

using namespace std;

//------------------------------------------------------------------------

Expression::Expression(void)
{


}


Expression:: ~Expression(void)
{
}

//------------------------------------------------------------------------
Value::Value()
{
  constant = false;
}

Value::~Value()
{
}

Value::Value(bool isConstant)
{
  constant = isConstant;
}

int Value::getAsInt()
{
  throw new Error(showType() +
		  " cannot be converted to an integer ");
}

double Value::getAsDouble()
{
  cout << "Throwing an exception\n";
  throw new Error(showType() +
		  " cannot be converted to a double ");
}

/*****************************************************************
 * The AbstractRange class.
 */
AbstractRange::AbstractRange(unsigned int newLeft, unsigned int newRight)
{
  left = newLeft;
  right = newRight;
}


AbstractRange::AbstractRange(unsigned int newLeft, unsigned int newRight, bool isConstant)
  : Value(isConstant)
{
  left = newLeft;
  right = newRight;
}


AbstractRange::~AbstractRange()
{
}


string AbstractRange::toString()
{
  string str = "";
  
  str = Integer::toString("%02d", left) + ":" + Integer::toString("%02d", right);
  return (str);
}

string AbstractRange::toString(char* format)
{
  char cvtBuf[1024];

  sprintf(cvtBuf, format, left, right);
  return (string(&cvtBuf[0]));
}

unsigned int AbstractRange::get_leftVal()
{
  return(left);
}

unsigned int AbstractRange::get_rightVal()
{
  return(right);
}

AbstractRange* AbstractRange::typeCheck(Value* val, string valDesc)
{
  if (typeid(*val) != typeid(AbstractRange)) {
    throw new TypeMismatch(valDesc, "AbstractRange", val->showType());
  }
  // This static cast is totally safe in light of our typecheck, above.
  return((AbstractRange*)(val));
}

/*****************************************************************
 * The Boolean class.
 */
Boolean::Boolean(bool newValue)
{
  value = newValue;
}

Boolean::Boolean(bool newValue, bool isConstant)
  : Value(isConstant)
{
  value = newValue;
}

Boolean::~Boolean()
{
}

string Boolean::toString()
{
  return (string(value ? "true" : "false"));
}

string Boolean::toString(bool value)
{
  return (string(value ? "true" : "false"));
}

string Boolean::toString(char* format)
{
  char cvtBuf[1024];

  sprintf(cvtBuf, format, value);
  return (string(&cvtBuf[0]));
}

bool Boolean::getVal()
{
  return value;
}

Boolean* Boolean::typeCheck(Value* val, string valDesc)
{
  if (typeid(*val) != typeid(Boolean)) {
    throw new TypeMismatch(valDesc, "Boolean", val->showType());
  }

  // This static cast is totally safe in light of our typecheck, above.
  return((Boolean*)(val));
}

/*****************************************************************
 * The Integer class.
 */

Integer::Integer(int newValue, bool isConstant)
  : Value(isConstant)
{
  value = newValue;
}

string Integer::toString()
{
  return toString("%d");
}


string Integer::toString(char* format)
{
  char cvtBuf[1024];

  snprintf(cvtBuf,sizeof(cvtBuf), format, value);
  return (string(&cvtBuf[0]));
}


string Integer::toString(char* format, int value)
{
  char cvtBuf[1024];

  snprintf(cvtBuf,sizeof(cvtBuf), format, value);
  return (string(&cvtBuf[0]));
}

string Integer::toString(int value)
{
  char cvtBuf[1024];

  snprintf(cvtBuf,sizeof(cvtBuf), "%d", value);
  return (string(&cvtBuf[0]));  
}

int Integer::getVal()
{
  return value;
}

Integer* Integer::typeCheck(Value* val, string valDesc)
{
  if (typeid(*val) != typeid(Integer)) {
    throw new TypeMismatch(valDesc, "Integer", val->showType());
  }

  // This static cast is totally safe in light of our typecheck, above.
  return((Integer*)(val));
}

Integer* Integer::assertValid(Value* val, string valDesc, int valMin)
{
  Integer* iVal;
  int i;

  iVal = Integer::typeCheck(val, valDesc);
  i = iVal->getVal();
  
  if (i < valMin) {
    throw new Error(valDesc +
                    " must be greater than " + Integer::toString(valMin) + 
                    ", saw " + Integer::toString(i)
                    );
  }
  
  return(iVal);
}

Integer* Integer::assertValid(Value* val, string valDesc, int valMin, int valMax)
{
  Integer* iVal;
  int i;
  
  iVal = (Integer::typeCheck(val, valDesc));

  i = iVal->getVal();
  
  if ((i < valMin) || (i>valMax)) {
    throw new Error(valDesc +
                    " must be be in the range [" + Integer::toString(valMin) + ".." + 
                    Integer::toString(valMax) + "], saw " + Integer::toString(i)
                    );
  }
  
  return(iVal);
}


/*****************************************************************
 * The Float class.
 */
Float::Float(double newValue)
{
  value = newValue;
}

Float::Float(double newValue, bool isConstant)
  : Value(isConstant)
{
  value = newValue;
}

Float::~Float()
{
}

string Float::toString()
{
  return toString("%#-16.16g");
}


string Float::toString(char* format)
{
  char cvtBuf[1024];

  sprintf(cvtBuf, format, value);
  return (string(&cvtBuf[0]));
}

double Float::getVal()
{
  return(value);
}


Float* Float::typeCheck(Value* val, string valDesc)
{
  if (typeid(*val) != typeid(Float)) {
    throw new TypeMismatch(valDesc, "Float", val->showType());
  }

  // This static cast is totally safe in light of our typecheck, above.
  return((Float*)(val));
}



/*****************************************************************
 * The String class.
 */
String::String(string newValue)
{
  value = newValue;
}

String::String(string newValue, bool isConstant)
  : Value(isConstant)
{
  value = newValue;
}


String::~String()
{
}


// -----------------------------------------------------------------
// The string toString is a bit more complicated than you might
// expect.  The complication comes from the fact that we want to
// display the string in a fashion that will allow it to be
// reparsed.  This really means that we need to reformat all
// non-printing chars that we encounter in the string.
string String::toString()
{
  unsigned int i;
  string msg;
  unsigned char c;

  msg = "\"";
  for (i=0; i<value.size(); ++i) {
    c = value.at(i);

    if (isprint((int)c)) {
      /* Look for the printable chars that need to be escaped
         so that the string could be easily reparsed by other
         tools. */
      if (c=='\\') msg += "\\";
      if (c=='"') msg += "\\";
      
      msg += c;
    }
    else {
      /* Display non-printing chars in a C-like fashion */
      switch (c) {
      case '\a': msg += "\\a"; break;
      case '\b': msg += "\\b"; break;
      case '\f': msg += "\\f"; break;
      case '\n': msg += "\\n"; break;
      case '\r': msg += "\\r"; break;
      case '\t': msg += "\\t"; break;
      case '\v': msg += "\\v"; break;
      default:   msg += "\\" + Integer::toString("%02x", c);
      }
    }
  }
  msg += '"';

  return(msg);
}

string String::toString(char* format)
{
  char cvtBuf[1024];

  sprintf(cvtBuf, format, value.c_str());
  return (string(&cvtBuf[0]));
}


string String::getVal()
{
  return(value);
}


String* String::typeCheck(Value* val, string valDesc)
{
  if (typeid(*val) != typeid(String)) {
    throw new TypeMismatch(valDesc, "String", val->showType());
  }

  // This static cast is totally safe in light of our typecheck, above.
  return((String*)(val));
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


