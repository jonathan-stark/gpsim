/*****************************************************************
 *
 */
#include <typeinfo>
#include "viewable.h"

#include <ctype.h>

/*****************************************************************
 * The primoridal Viewable class.
 */
Viewable::Viewable()
{
}

Viewable::~Viewable()
{
}


// The 'type' of any Viewable object is equivalent to the class name.
string Viewable::showType()
{
  const char* name;
  
  name = typeid(*this).name();
  
  /* Unfortunately, the class name string returned by typeid() is
   * implementation-specific.  If a particular compiler produces
   * ugly output, this is your chance to clean it up. */
#if defined __GNUC__
  /* GNU C++ puts the length of the class name in front of the
     actual class name.  We will skip over it for clarity. */
  while (isdigit(*name)) {
    name++;
  }
#elif defined _MSC_VER
  #pragma message("--->You might want to clean up the result of typeid() here...")
#else
  #warning --->You might want to clean up the result of typeid() here...
#endif

  return string(name);
}


string Viewable::show()
{
  return showType() + ":" + toString();
}

