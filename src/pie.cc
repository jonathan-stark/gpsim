#include <glib.h>	// for guint64
#include <iostream>		// for cout used in breakpoints.h
using namespace std;
#include "breakpoints.h"
#include "trace.h"
#include "gpsim_classes.h"	// for RESET_TYPE

#include "pie.h"
#include "pir.h"

void PIE::put(unsigned int new_value)
{
  value = new_value;
  trace.register_write(address,value);

  if( pir->interrupt_status())
    {
      pir->intcon->peripheral_interrupt();
    }
}
