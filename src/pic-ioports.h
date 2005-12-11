/*
   Copyright (C) 1998 T. Scott Dattalo

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

#ifndef __PIC_IOPORTS_H__
#define __PIC_IOPORTS_H__

#include "ioports.h"

///------------------------------------------------------------
class PicTrisRegister;
class PicPortRegister : public PortRegister
{
public:
  PicPortRegister(const char *port_name, unsigned int numIopins, unsigned int enableMask);
  void setTris(PicTrisRegister *new_tris);
  virtual void setEnableMask(unsigned int nEnableMask);
  Register *getTris();
protected:
  PicTrisRegister *m_tris;
};

class PicTrisRegister : public sfr_register
{

public:

  PicTrisRegister(const char *tris_name, PicPortRegister *);
  virtual void put(unsigned int new_value);
  virtual unsigned int get();
  
protected:
  PicPortRegister *m_port;
};

class PicPortBRegister : public PicPortRegister
{
public:
  PicPortBRegister(const char *port_name, unsigned int numIopins, unsigned int enableMask);

  virtual void put(unsigned int new_value);
  virtual unsigned int get();
  virtual void setbit(unsigned int bit_number, char new_value);
  void setRBPU(bool);
  void setIntEdge(bool);
private:
  enum {
    eIntEdge = 1<<6,
    eRBPU    = 1<<7
  };
  bool m_bRBPU;
  bool m_bIntEdge;

};

#endif  // __PIC_IOPORTS_H__
