#ifndef PIE_H
#define PIE_H

class PIR;

#include "gpsim_classes.h"
#include "registers.h"
#include "breakpoints.h"

//---------------------------------------------------------
// PIE Peripheral Interrupt Enable register base class 
// for PIE1 & PIE2

class PIE : public sfr_register
{
public:
  PIR *pir;

  void put(unsigned int new_value);

};

#endif /* PIE_H */
