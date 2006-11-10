#include <glib.h>	// for guint64
#include <iostream>		// for cout used in breakpoints.h
using namespace std;
//#include "breakpoints.h"
#include "trace.h"

#include "intcon.h"
#include "pie.h"
#include "pir.h"

void PIE::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  value.put(new_value);

  assert(pir);

  if(pir->interrupt_status())
    pir->setPeripheralInterrupt();
}
