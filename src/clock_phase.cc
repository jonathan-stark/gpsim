/*
   Copyright (C) 2006 T. Scott Dattalo

This file is part of gpsim.

gpsim is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

gpsim is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with gpsim; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#include "clock_phase.h"
#if defined(CLOCK_EXPERIMENTS)

#include "processor.h"
#include "gpsim_time.h"

//========================================================================
ClockPhase::ClockPhase()
  : m_pNextPhase(this)
{
}

ClockPhase::~ClockPhase()
{
}


//========================================================================

ProcessorPhase::ProcessorPhase(Processor *pcpu)
  : ClockPhase(), 
    m_pcpu(pcpu)
{
}
ProcessorPhase::~ProcessorPhase()
{
}

//========================================================================

phaseExecute1Cycle::phaseExecute1Cycle(Processor *pcpu)
  : ProcessorPhase(pcpu)
{
}
phaseExecute1Cycle::~phaseExecute1Cycle()
{
}

/*
  phaseExecute1Cycle::advance() - advances a processor's time one clock cycle.

 */

ClockPhase *phaseExecute1Cycle::advance()
{
  setNextPhase(this);
  m_pcpu->step_one(false);
  get_cycles().increment();
  return m_pNextPhase;
}

#endif // defined(CLOCK_EXPERIMENTS)
