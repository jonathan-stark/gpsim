
#include "errors.h"

/*****************************************************************
 * The primordial Assembler Error class.
 */
AnError::AnError(string severity, string errMsg)
{
  // Pretty gross, but the lexer makes sure that this global
  // always describes the current source line.
  //this->sourceRef = thisLine;

  this->severity = severity;
  this->errMsg = errMsg;

}

AnError::~AnError()
{
}



string AnError::toString()
{
  return string("\"" + errMsg + "\"");
}


string AnError::get_errMsg()
{
  return errMsg;
}

/*****************************************************************
 * Generate assembler errors of severity "ERROR"
 */
int Error::count;

Error::Error(string errMsg)
  : AnError(string("ERROR"), errMsg)
{
}

Error::~Error()
{
}


/*****************************************************************
 * Generate assembler errors of severity "FATAL_ERROR"
 */
FatalError::FatalError(string errMsg)
  : AnError(string("FATAL_ERROR"), errMsg)
{
}

FatalError::~FatalError()
{
}


/*****************************************************************
 * Generate a generic Type Mismatch error of the "expected xx,
 * observed yy" variety.
 */
TypeMismatch::TypeMismatch(string theOperator, string expectedType, string observedType)
  : Error(" Type mismatch for " + theOperator + " operator. Type expected " + expectedType
          + ", found " + observedType)
{
}


/*****************************************************************
 * Generate a generic Type Mismatch error of the "operator x
 * cannot be applied to type y" variety.
 */
TypeMismatch::TypeMismatch(string theOperator, string observedType)
  : Error("Operator <" + theOperator + "> cannot be applied to type "
          + observedType)
{
}

TypeMismatch::~TypeMismatch()
{
}
