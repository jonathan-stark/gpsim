/*
   Copyright (C) 1998-2003 Scott Dattalo
                 2003 Mike Durian
                 2006 Roy Rankin
                 2006 David Barnett

This file is part of the libgpsim library of gpsim

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see 
<http://www.gnu.org/licenses/lgpl-2.1.html>.
*/

#include <assert.h>

#include <iostream>
#include <iomanip>
using namespace std;

#include <glib.h>

#include "trace.h"
#include "pic-processor.h"
#include "pm_rd.h"


//------------------------------------------------------------------------
//
// PM-related registers


void PMCON1::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());

  new_value &= valid_bits;
  
  bool rd_rise = (bool)(new_value & ~value.get() & RD);
  value.put((value.get() & RD) | new_value);

  if (rd_rise)
    pm_rd->start_read();
}

unsigned int PMCON1::get()
{
  trace.raw(read_trace.get() | value.get());

  return(value.get());
}

PMCON1::PMCON1(Processor *pCpu, PM_RD *pRd)
  : sfr_register(pCpu, "pmcon1", "Program Memory Read Write Control"),
    pm_rd(pRd)
{
  valid_bits = PMCON1_VALID_BITS;
}

unsigned int PMDATA::get()
{
  trace.raw(read_trace.get() | value.get());
  return(value.get());
}

void PMDATA::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  value.put(new_value);
}

PMDATA::PMDATA(Processor *pCpu, const char *pName)
  : sfr_register(pCpu, pName, "Program Memory Data")
{}


unsigned int PMADR::get()
{
  trace.raw(read_trace.get() | value.get());

  return(value.get());
}

void PMADR::put(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  value.put(new_value);

}


PMADR::PMADR(Processor *pCpu, const char *pName)
  : sfr_register(pCpu, pName, "Program Memory Address")
{}

// ----------------------------------------------------------

PM_RD::PM_RD(pic_processor *pCpu)
  : cpu(pCpu),
    pmcon1(pCpu,this),
    pmdata(pCpu,"pmdata"),
    pmdath(pCpu,"pmdath"),
    pmadr(pCpu,"pmadr"),
    pmadrh(pCpu,"pmadrh")
{
}

void PM_RD::start_read()
{
  rd_adr = pmadr.value.get() | (pmadrh.value.get() << 8);

  get_cycles().set_break(get_cycles().get() + READ_CYCLES, this);
}

void PM_RD::callback()
{
  // read program memory
  if(pmcon1.value.get() & PMCON1::RD) {
    int opcode = cpu->pma->get_opcode(rd_adr);
    pmdata.value.put(opcode & 0xff);
    pmdath.value.put((opcode>>8) & 0xff);
    pmcon1.value.put(pmcon1.value.get() & (~PMCON1::RD));
  }
}

