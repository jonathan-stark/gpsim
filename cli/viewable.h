/*****************************************************************
 * Viewable.h: All objects subclassed from this are "viewable" in
 * the following sense:
 *
 * 1. Objects subclassed from Viewable must provide a method
 * toString() that will return a string representation of their
 * value.
 *
 * 2. The Viewable class provides a method showType() to get a
 * string representation of their data type (their classname).
 *
 * 3. The Viewable class provides a method show() that uses the
 * showType() and toString() methods to display the object in
 * the string form "type:value"
 *
 */

#if !defined(VIEWABLE_H)
#define VIEWABLE_H

#include <string>
using namespace std;


//*****************************************************************
class Viewable {
 public:
  Viewable();
  virtual ~Viewable();
  string show();
  string showType();
  virtual string toString()=0;
  
 private:

};



#endif
