#if !defined(_ERRORS_H_)
#define _ERRORS_H_

#include <string>

#include "gpsim_object.h"
using namespace std;

//*****************************************************************
class AnError : public gpsimObject {
 public:
  AnError(string severity, string errMsg);
  virtual ~AnError();

  string toString();
  string get_errMsg();

 private:
  string severity;
  string errMsg;
};

//*****************************************************************
class Error : public AnError {
 public:
  Error(string errMsg);
  virtual ~Error();

  static int count;
};

//*****************************************************************
class FatalError : public AnError {
 public:
  FatalError(string errMsg);
  virtual ~FatalError();
};

//*****************************************************************
class TypeMismatch : public Error {
 public:
  TypeMismatch(string theOperator, string expectedType, string observedType);
  TypeMismatch(string theOperator, string observedType);
  virtual ~TypeMismatch();
};

#endif // _ERRORS_
