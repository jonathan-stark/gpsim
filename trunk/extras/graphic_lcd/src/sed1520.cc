/*
   Copyright (C) 2005 T. Scott Dattalo

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


#define IN_MODULE

#include <time.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <stdlib.h>
#include <string>
#include <iostream>

#include "../config.h"

#include <gtk/gtk.h>


#include <gpsim/packages.h>
#include <gpsim/stimuli.h>
#include <gpsim/symbol.h>
#include <gpsim/gpsim_interface.h>


#include "sed1520.h"


#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("%s:%d",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

//------------------------------------------------------------------------
SED1520::SED1520()
  : m_bE(true), m_controlState(0),
    m_page(0), m_columnAddress(0)
{

}

//------------------------------------------------------------------------
void SED1520::setA0(bool newA0)
{
  m_controlState &= ~eA0;
  m_controlState |= (newA0 ? eA0 : 0);
}

void SED1520::setRW(bool newRW)
{
  m_controlState &= ~eRW;
  m_controlState |= (newRW ? eRW : 0);
}

void SED1520::setE(bool newE)
{

  if (m_bE != newE && newE) {

    switch (m_controlState) {
    case eDataRead:
      driveDataBus(getData());
      advanceColumnAddress();
      break;
    case eDataWrite:
      storeData();
      advanceColumnAddress();
      break;
    case eStatusRead:
      driveDataBus(getStatus());
      break;
    case eCommandWrite:
      executeCommand();
      break;
    }
  }

  m_bE = newE;

}



void SED1520::setData(unsigned int d)
{
  m_dataBus = d;
}

void SED1520::driveDataBus(unsigned int d)
{
  m_dataBus = d;
}

void SED1520::advanceColumnAddress()
{
  m_columnAddress += (m_columnAddress < eNCOLUMNS_PER_PAGE) ? 1 : 0;
}

void SED1520::storeData()
{
  m_ram[eNCOLUMNS_PER_PAGE*m_page + m_columnAddress] = m_dataBus;
}

unsigned int SED1520::getData()
{
  m_dataBus = m_ram[eNCOLUMNS_PER_PAGE*m_page + m_columnAddress];
  return m_dataBus;
}

unsigned int SED1520::getStatus()
{
  return 0;
}

void SED1520::executeCommand()
{

  // decode the command 

  if ( (m_dataBus & 0x80) == 0) {
    // Set column address

    m_columnAddress = (m_dataBus & 0x7f) % eNCOLUMNS_PER_PAGE;

  } else {

    if ((m_dataBus & 0x7C) == 0x38)
      // set Page Address
      m_page = m_dataBus & 3;

  }
}

//------------------------------------------------------------------------
bool SED1520::dataBusDirection()
{
  return (m_controlState & (eE | eRW)) == (eE | eRW);
}
//------------------------------------------------------------------------
unsigned int SED1520::getDataBus()
{
  return m_dataBus;
}
//------------------------------------------------------------------------

void SED1520::randomizeRAM()
{
  for (unsigned int i=0; i < eNCOLUMNS_PER_PAGE * eNPAGES; i++)
    m_ram[i] =  (rand()>>8) & 0xff;

}
//------------------------------------------------------------------------
// prBadRam - private function called when ever an illegal access is made
// to the RAM
unsigned int &SED1520::prBadRam(unsigned int index)
{
  static unsigned int si;
  printf("WARNING SED1520 - illegal RAM access index=%d\n",index);
  return si;
}
