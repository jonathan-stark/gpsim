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

NUMERICALLY CONTROLLED OSCILLATOR (NCO) MODULE

*/

#ifndef __NCO_h__
#define __NCO_h__
#include "registers.h"

class NCO;
class NCO_Interface;
class ncoCLKSignalSink;
class NCOSigSource;
class PIR;
class CLC;

class NCOxCON : public sfr_register
{
public:
	NCOxCON(NCO *pt, Processor *pCpu, const char *pName, const char *pDesc);
	virtual void put(unsigned int new_value);
        unsigned int con_mask;
private:
	NCO *pt_nco;
};

class NCOxCLK : public sfr_register
{
public:
        NCOxCLK(NCO *pt, Processor *pCpu, const char *pName, const char *pDesc);
        virtual void put(unsigned int new_value);
        unsigned int clk_mask;
private:
        NCO *pt_nco;
};

class NCOxACCL : public sfr_register
{
public:
        NCOxACCL(NCO *pt, Processor *pCpu, const char *pName, const char *pDesc);
        virtual void put(unsigned int new_value);
private:
        NCO *pt_nco;
};
class NCOxACCH : public sfr_register
{
public:
        NCOxACCH(NCO *pt, Processor *pCpu, const char *pName, const char *pDesc);
        virtual void put(unsigned int new_value);
private:
        NCO *pt_nco;
};
class NCOxACCU : public sfr_register
{
public:
        NCOxACCU(NCO *pt, Processor *pCpu, const char *pName, const char *pDesc);
        virtual void put(unsigned int new_value);
private:
        NCO *pt_nco;
};
class NCOxINCH : public sfr_register
{
public:
        NCOxINCH(NCO *pt, Processor *pCpu, const char *pName, const char *pDesc);
        virtual void put(unsigned int new_value);
private:
        NCO *pt_nco;
};
class NCOxINCL : public sfr_register
{
public:
        NCOxINCL(NCO *pt, Processor *pCpu, const char *pName, const char *pDesc);
        virtual void put(unsigned int new_value);
private:
        NCO *pt_nco;
};



class NCO : public TriggerObject
{
public:
	enum {
	// NCOxCON
	NxEN	= 1<<7,
	NxOE	= 1<<6,
	NxOUT	= 1<<5,
	NxPOL	= 1<<4,
	NxPFM	= 1<<0,
	// NCOxCLK
	NxPW_mask = 0xe0,
	NxCLKS_mask = 0x03,
	// the follow are pseudo values returned from clock_src()
	HFINTOSC = 0,
	FOSC = 1,
	LC1_OUT = 2,
	NCO1CLK = 3

	};

    NCOxCON  nco1con;
    NCOxCLK  nco1clk;
    NCOxACCH nco1acch;
    NCOxACCL nco1accl;
    NCOxACCU nco1accu;
    NCOxINCH nco1inch;
    NCOxINCL nco1incl;
    NCO(Processor *pCpu);
    ~NCO();
    virtual int clock_src();
    void current_value();
    void set_acc_buf();
    void set_inc_buf();
    void update_ncocon(unsigned int);
    void update_ncoclk(unsigned int);
    void setIOpins(PinModule *pIN, PinModule *pOUT);
    void setNCOxPin(PinModule *pNCOx);
    void link_nco(bool level, char index);
    void setState(char new3State);
    void oeNCO1(bool on);
    void outputNCO1(bool level);
    void enableCLKpin(bool on);
    void releasePinSource(PinModule *pin);
    void newINCL();
    void NCOincrement();
    void simulate_clock(bool on);
    void callback();
    void set_hold_acc(unsigned int acc_val, int index) { acc_hold[index] = acc_val; }
    void set_accFlag(bool newValue) { accFlag = newValue;}
    bool get_accFlag() { return accFlag;}
    void set_clc(CLC *_clc, int i) { m_clc[i] = _clc; }
    PIR		    *pir;
    InterruptSource *m_NCOif;
private:
    Processor       *cpu;
    PinModule       *pinNCOclk;
    string	    CLKgui;
    PinModule       *pinNCO1;
    string	    NCO1gui;
    NCOSigSource    *NCO1src;
    bool	    srcNCO1active;
    int		    inc_load;
    unsigned int    inc;
    gint32	    acc;
    unsigned int    acc_hold[3];
    guint64	    future_cycle;
    guint64	    last_cycle;		// Time of last acc update
    NCO_Interface   *nco_interface;
    ncoCLKSignalSink   *CLKsink;
    bool	    CLKstate;
    bool	    NCOoverflow;
    bool	    accFlag;		// acc buffer needs updating
    unsigned int    pulseWidth;
    CLC		    *m_clc[4];
};

// NCO with clock layout ala 10f320
class NCO2 : public NCO
{
public:
    NCO2(Processor *pCpu);
    virtual int clock_src();

};

#endif //__NCO_h__

