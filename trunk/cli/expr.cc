#include <string>
#include <list>
#include <vector>
#include <iostream>

#include "expr.h"


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

}

Value::~Value()
{
  cout << "deleting Value\n";

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

  sprintf(cvtBuf, "%d", value);
  return (string(&cvtBuf[0]));  
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
 
  cout << "deleting LiteralInteger\n";
}


