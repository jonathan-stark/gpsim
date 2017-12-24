/*
   Copyright (C) 2017 Roy R Rankin

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

// CONFIGURABLE LOGIC CELL (CLC)

#ifndef __CLC_h__
#define __CLC_h__

#include "registers.h"
class CLC;
class INxSignalSink;
class CLCSigSource;

class CLCxCON : public sfr_register
{
  public:
    CLCxCON(CLC *_clc, Processor *pCpu, const char *pName, const char *pDesc) :
	sfr_register(pCpu, pName, pDesc), m_clc(_clc), write_mask(0xdf)
    {
    }
    void put(unsigned int);

  private:
    CLC *m_clc;
    unsigned int write_mask;
};

class CLCxPOL : public sfr_register
{
  public:
    CLCxPOL(CLC *_clc, Processor *pCpu, const char *pName, const char *pDesc) :
	sfr_register(pCpu, pName, pDesc), m_clc(_clc), write_mask(0x8f)

    {
    }

  private:
    CLC *m_clc;
    unsigned int write_mask;
};

class CLCxSEL0 : public sfr_register
{
  public:
    CLCxSEL0(CLC *_clc, Processor *pCpu, const char *pName, const char *pDesc) :
	sfr_register(pCpu, pName, pDesc), m_clc(_clc), write_mask(0x77)
    {
    }

    void put(unsigned int);

  private:
   CLC *m_clc;
    unsigned int write_mask;
    
};

class CLCxSEL1 : public sfr_register
{
  public:
    CLCxSEL1(CLC *_clc, Processor *pCpu, const char *pName, const char *pDesc) :
	sfr_register(pCpu, pName, pDesc), m_clc(_clc), write_mask(0x77)
    {
    }

    void put(unsigned int);

  private:
    CLC *m_clc;
    unsigned int write_mask;
};

class CLCxGLS0 : public sfr_register
{
  public:
    CLCxGLS0(CLC *_clc, Processor *pCpu, const char *pName, const char *pDesc) :
	sfr_register(pCpu, pName, pDesc), m_clc(_clc)
    {
    }

  private:
    CLC *m_clc;
};

class CLCxGLS1 : public sfr_register
{
  public:
    CLCxGLS1(CLC *_clc, Processor *pCpu, const char *pName, const char *pDesc) :
	sfr_register(pCpu, pName, pDesc), m_clc(_clc)

    {
    }
  private:
    CLC *m_clc;
};

class CLCxGLS2 : public sfr_register
{
  public:
    CLCxGLS2(CLC *_clc, Processor *pCpu, const char *pName, const char *pDesc) :
	sfr_register(pCpu, pName, pDesc), m_clc(_clc)

    {
    }
  private:
    CLC *m_clc;
};

class CLCxGLS3 : public sfr_register
{
  public:
    CLCxGLS3(CLC *_clc, Processor *pCpu, const char *pName, const char *pDesc) :
	sfr_register(pCpu, pName, pDesc), m_clc(_clc)

    {
    }
  private:
    CLC *m_clc;
};

class CLCDATA : public sfr_register
{
  public:
    CLCDATA(Processor *pCpu, const char *pName, const char *pDesc) :
	sfr_register(pCpu, pName, pDesc)
    {
    }
};


class CLC
{
  public:

    enum {
	// CLCxCON
	LCxEN   = (1<<7),
	LCxOE   = (1<<6),
	LCxOUT  = (1<<5),
	LCxINTP = (1<<4),
	LCxINTN = (1<<3),
	LCxMODE = 0x7,

	// CLCxPOL
	LCxPOL  = (1<<7),
    };

    CLC(Processor *_cpu, unsigned int _index);
    ~CLC();
    void setCLCxPin(PinModule *alt_pin);
    void enableINxpin(int, bool);
    void D1S(int select);
    void D2S(int select);
    void D3S(int select);
    void D4S(int select);
    void t0_overflow();
    void t1_overflow();
    void t2_match();
    void out_pwm(bool level, int id);
    void NCO_out(bool level);
    void CxOUT_sync(bool output, int cm);
    void set_clcPins(PinModule *IN0, PinModule *IN1, PinModule *_CLCx)
	{ pinCLCxIN[0] = IN0; pinCLCxIN[1] = IN1, pinCLCx = _CLCx;}
    void setState(char new3State, int index);
    void releasePinSource(PinModule *pin);
    void oeCLCx(bool on);
    void update_clccon(unsigned int diff);
    void scan_gates(bool on);

    CLCxCON  clcxcon;
    CLCxPOL  clcxpol;
    CLCxSEL0 clcxsel0;
    CLCxSEL1 clcxsel1;
    CLCxGLS0 clcxgls0;
    CLCxGLS1 clcxgls1;
    CLCxGLS2 clcxgls2;
    CLCxGLS3 clcxgls3;
    CLCDATA  *clcdata;

  private:
    PinModule     *pinCLCx;
    CLCSigSource  *CLCxsrc;
    string        CLCxgui;
    bool          srcCLCxactive;
    unsigned int  index;
    int           D1Sselect;
    int           D2Sselect;
    int           D3Sselect;
    int           D4Sselect;
    INxSignalSink *INxsink[2];
    int		  INxactive[2];
    bool	  INxstate[2];
    PinModule     *pinCLCxIN[2];
    string        INxgui[2];
    bool	  pwmx_level[4];
    bool	  CMxOUT_level[4];
};

#endif // __CLC_h__
