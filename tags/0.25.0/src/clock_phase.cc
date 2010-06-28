/*
   Copyright (C) 2006 T. Scott Dattalo

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


#include "clock_phase.h"

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
  if (!bp.global_break)
      get_cycles().increment();
  return m_pNextPhase;
}

//========================================================================

phaseIdle::phaseIdle(Processor *pcpu)
  : ProcessorPhase(pcpu)
{
}
phaseIdle::~phaseIdle()
{
}

/*
  phaseIdle::advance() - advances a processor's time one clock cycle,
  but does not execute code.
 */

ClockPhase *phaseIdle::advance()
{
  setNextPhase(this);
  get_cycles().increment();
  return m_pNextPhase;
}
