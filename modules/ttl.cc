/*
   Copyright (C) 2006T. Scott Dattalo

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

/*
  ttl.cc - gpsim's TTL library

*/


#include "../config.h"

#ifdef HAVE_GUI
#include <gtk/gtk.h>
#endif

#include "ttl.h"
#include "../src/stimuli.h"
#include "../src/ioports.h"
#include "../src/symbol.h"
#include "../src/value.h"
#include "../src/packages.h"

namespace TTL
{

//------------------------------------------------------------------------
// TTL base class

TTLbase::TTLbase(const char *_name, const char *desc)
  : Module(_name,desc), m_dVdd(0)
{
}

TTLbase::~TTLbase()
{
}

//------------------------------------------------------------------------
// TTL377 - Octal Latch
//
// 

class Clock : public IOPIN
{
public:
  Clock(const char *n, TTLbase *pParent);

  virtual void setDrivenState(bool);

private:
  TTLbase *m_pParent;
};

//------------------------------------------------------------
// Enable
//
class Enable : public IOPIN
{
public:
  Enable(const char *n, TTLbase *pParent);

  virtual void setDrivenState(bool);

private:
  TTLbase *m_pParent;
};


//------------------------------------------------------------------------
Clock::Clock(const char *n, TTLbase *pParent)
  : IOPIN(n), m_pParent(pParent)
{
}
void Clock::setDrivenState(bool bNewState)
{
  IOPIN::setDrivenState(bNewState);
  if (m_pParent)
    m_pParent->setClock(bNewState);
}

//------------------------------------------------------------------------
Enable::Enable(const char *n, TTLbase *pParent)
  : IOPIN(n), m_pParent(pParent)
{
}
void Enable::setDrivenState(bool bNewState)
{
  IOPIN::setDrivenState(bNewState);
  if (m_pParent)
    m_pParent->setEnable(bNewState);
}

//------------------------------------------------------------------------

Module *TTL377::construct(const char *_new_name)
{
  TTL377 *pTTL377 = new TTL377(_new_name);

  pTTL377->new_name(_new_name);
  pTTL377->create_iopin_map();


  return pTTL377;
}


TTL377::TTL377(const char *_name)
  : TTLbase(_name, "TTL377 - Octal Latch")
{

  m_D = new IOPIN *[8];
  m_Q = new IO_bi_directional *[8];

  char pName[4];
  pName[0] = '.';
  pName[3] = 0;
  int i;
  string sPinName;
  for (i=0; i<8; i++) {
    pName[1] = 'D';
    pName[2] = '0' + i;
    sPinName = name() + pName;
    m_D[i] = new IOPIN(sPinName.c_str());

    pName[1] = 'Q';
    sPinName = name() + pName;
    m_Q[i] = new IO_bi_directional(sPinName.c_str());
    m_Q[i]->setDriving(true);
  }

  sPinName = name() + ".E";
  m_enable = new Enable(sPinName.c_str(),this);
  sPinName = name() + ".CP";
  m_clock  = new Clock(sPinName.c_str(),this);
}

void TTL377::setClock(bool bNewClock)
{
  if (bNewClock && !m_bClock && !m_bEnable) {
    update_state();
  }
  m_bClock = bNewClock;
}

void TTL377::setEnable(bool bNewEnable)
{
  m_bEnable = bNewEnable;
}

void TTL377::update_state()
{
  // Copy the inputs to the outputs
  for (int i=0; i<8; i++) 
    m_Q[i]->putState(m_D[i]->getDrivenState());
}

void TTL377::create_iopin_map()
{
  package = new Package(20);

  package->assign_pin( 1, m_enable);
  package->assign_pin( 2, m_Q[0]);
  package->assign_pin( 3, m_D[0]);
  package->assign_pin( 4, m_D[1]);
  package->assign_pin( 5, m_Q[1]);
  package->assign_pin( 6, m_Q[2]);
  package->assign_pin( 7, m_D[2]);
  package->assign_pin( 8, m_D[3]);
  package->assign_pin( 9, m_Q[3]);
  package->assign_pin(11, m_clock);
  package->assign_pin(12, m_Q[4]);
  package->assign_pin(13, m_D[4]);
  package->assign_pin(14, m_D[5]);
  package->assign_pin(15, m_Q[5]);
  package->assign_pin(16, m_Q[6]);
  package->assign_pin(17, m_D[6]);
  package->assign_pin(18, m_D[7]);
  package->assign_pin(19, m_Q[7]);
  
}


}
