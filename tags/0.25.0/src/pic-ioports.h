/*
   Copyright (C) 1998 T. Scott Dattalo
   Copyright (C) 2009 Roy R. Rankin

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

#ifndef __PIC_IOPORTS_H__
#define __PIC_IOPORTS_H__

#include "ioports.h"

///------------------------------------------------------------
class PicTrisRegister;
class PicPortRegister : public PortRegister
{
public:
  PicPortRegister(Processor *pCpu, const char *pName, const char *pDesc,
                  /*const char *port_name, */
                  unsigned int numIopins, unsigned int enableMask=0xff);
  void setTris(PicTrisRegister *new_tris);
  Register *getTris();
protected:
  PicTrisRegister *m_tris;
};

class PicTrisRegister : public sfr_register
{

public:

  PicTrisRegister(Processor *pCpu, const char *pName, const char *pDesc,
                  /*const char *tris_name, */
                  PicPortRegister *,bool bIgnoreWDTResets, unsigned int nEnableMask=0xff);
  virtual void put(unsigned int new_value);
  virtual unsigned int get();
  virtual char get3StateBit(unsigned int bitMask);
  void setEnableMask(unsigned int);
  unsigned int getEnableMask() { return m_EnableMask; }
  void reset(RESET_TYPE r);
protected:
  PicPortRegister *m_port;
  unsigned int m_EnableMask;
  bool m_bIgnoreWDTResets;
};

class INTCON;
//  PicPortBRegister is usually used for portb and interrupts on selected edge
//  of bit 0 and sleep wakeup and interrupt on level changes for bits 4-7. 
class PicPortBRegister : public PicPortRegister
{
public:
  PicPortBRegister(Processor *pCpu, const char *pName, const char *pDesc,
                   INTCON *pIntcon,
                   unsigned int numIopins, unsigned int enableMask=0xff);
  ~PicPortBRegister();

  virtual void put(unsigned int new_value);
  virtual unsigned int get();
  virtual void setbit(unsigned int bit_number, char new_value);
  void setRBPU(bool);
  void setIntEdge(bool);
  void assignRBPUSink(unsigned int bitPos, sfr_register *);
private:
  enum {
    eIntEdge = 1<<6,
    eRBPU    = 1<<7
  };
  bool m_bRBPU;
  bool m_bIntEdge;

  BitSink *m_bsRBPU;
  INTCON  *m_pIntcon;
};

class IOC;
//	Like PicPortBRegister, PicPortGRegister allows wakeup from sleep
//	and interrupt on pin level change. However, PicPortGRegister
//	uses IOC to determine which of any of the bits will do this.
//	Note: as GPIF,GPIE are the same bits as RBIF,RBIE in INTCON we can
//	use the existing set_rbif function to set the GPIF bit.
//
class PicPortGRegister : public PicPortBRegister
{
 
public:
  INTCON *m_pIntcon;
  IOC	*m_pIoc;

  PicPortGRegister(Processor *pCpu, const char *pName, const char *pDesc,
                   INTCON *pIntcon, IOC *pIoc,
                   unsigned int numIopins, unsigned int enableMask=0x3f)
	: PicPortBRegister(pCpu, pName, pDesc, pIntcon, numIopins, enableMask),
	m_pIntcon(pIntcon), m_pIoc(pIoc)
  {
  }

  virtual void setbit(unsigned int bit_number, char new3State);
  void setIntEdge(bool);

private:
  bool m_bIntEdge;
};
class PSP;

class PicPSP_PortRegister : public PortRegister
{
public:
  PicPSP_PortRegister(Processor *pCpu, const char *pName, const char *pDesc,
                      /*const char *port_name, */
                      unsigned int numIopins, unsigned int enableMask);
  virtual void put(unsigned int new_value);
  virtual unsigned int get();
  void setPSP(PSP *pspReg) { m_psp = pspReg;}
  void setTris(PicTrisRegister *new_tris);
  Register *getTris();
protected:
  PicTrisRegister *m_tris;
  PSP           *m_psp;
};

class PicPSP_TrisRegister : public PicTrisRegister
{

public:

  PicPSP_TrisRegister(Processor *pCpu, const char *pName, const char *pDesc,
                      /*const char *tris_name, */
                      PicPortRegister *,bool bIgnoreWDTResets);
  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  virtual unsigned int get();
};

//------------------------------------------------------------------------
// PicLatchRegister - 16bit-core devices
class PicLatchRegister : public sfr_register
{
public:
  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
  virtual unsigned int get();
  virtual void setbit(unsigned int bit_number, char new_value);

  virtual void setEnableMask(unsigned int nEnableMask);

  PicLatchRegister(Processor *pCpu, const char *pName, const char *pDesc,
                   /*const char *, */
                   PortRegister *,unsigned int nEnableMask=0xff);

protected:
  PortRegister *m_port;
  unsigned int m_EnableMask;
};

#endif  // __PIC_IOPORTS_H__
