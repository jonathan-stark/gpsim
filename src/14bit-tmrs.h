/*
   Copyright (C) 1998 T. Scott Dattalo
   Copyright (C) 2009,2010,2013 Roy R. Rankin

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

#ifndef __14_BIT_TMRS_H__
#define __14_BIT_TMRS_H__

#include "gpsim_classes.h"
#include "registers.h"
#include "breakpoints.h"
#include "ioports.h"
#include "ssp.h"
#include "interface.h"

class TMRL;
class TMRH;
class TMR2;
class CCPRL;
class CCPCON;
class PWM1CON;
class ADCON0;
class PIR_SET;
class InterruptSource;
class TMR1_Interface;

class _14bit_processor;

//---------------------------------------------------------
// Todo
// 
// The timer base classes need to be abstracted one more 
// layer. The 18fxxx parts have a new timer, TMR3, that's
// almost but not quite, identical to the 16fxx's TMR1.


//---------------------------------------------------------
// CCPCON - Capture and Compare registers
//---------------------------------------------------------
class CCPRH : public sfr_register
{
public:

  CCPRL *ccprl;
  bool  pwm_mode;
  unsigned int pwm_value;

  CCPRH(Processor *pCpu, const char *pName, const char *pDesc=0);
  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  unsigned int get();

};


class CCPRL : public sfr_register
{
public:

  CCPRH  *ccprh;
  CCPCON *ccpcon;
  TMRL   *tmrl;

  void put(unsigned int new_value);
  void capture_tmr();
  void start_compare_mode(CCPCON *ref=0);
  void stop_compare_mode();
  bool test_compare_mode();
  void start_pwm_mode();
  void stop_pwm_mode();
  void assign_tmr(TMRL *ptmr);
  CCPRL(Processor *pCpu, const char *pName, const char *pDesc=0);
};

class INT_SignalSink;    // I/O pin interface
class PinModule;
//
// Enhanced Capture/Compare/PWM Auto-Shutdown control register
//
class ECCPAS : public sfr_register
{
public:

  /* Bit definitions for the register */
  enum {
	PSSBD0  = 1 << 0,	// Pins P1B and P1D Shutdown control bits
	PSSBD1  = 1 << 1,
	PSSAC0  = 1 << 2,	// Pins P1A and P1C Shutdown control bits
	PSSAC1  = 1 << 3,
	ECCPAS0 = 1 << 4,	// ECCP Auto-shutdown Source Select bits
	ECCPAS1 = 1 << 5,
	ECCPAS2 = 1 << 6,
	ECCPASE = 1 << 7	// ECCP Auto-Shutdown Event Status bit
  };


  ECCPAS(Processor *pCpu, const char *pName, const char *pDesc=0);
  ~ECCPAS();

  void put(unsigned int new_value);
  void put_value(unsigned int new_value);
  void setBitMask(unsigned int bm) { mValidBits = bm; }
  void setIOpin(PinModule *p0, PinModule *p1, PinModule *p2);
  void c1_output(int value);
  void c2_output(int value);
  void set_trig_state(int index, bool state);
  bool shutdown_trigger(int);
  void link_registers(PWM1CON *_pwm1con, CCPCON *_ccp1con);

private:
  PWM1CON 	*pwm1con;
  CCPCON  	*ccp1con;
  PinModule 	*m_PinModule;
  INT_SignalSink  *m_sink;
  bool		trig_state[3];
  int		c1_state;
  int		c2_state;
  int		int_state;
  
};
//
// Enhanced PWM control register
//
class PWM1CON : public sfr_register
{
public:

  /* Bit definitions for the register */
  enum {
	PDC0    = 1 << 0,	// PWM delay count bits
	PDC1    = 1 << 1,
	PDC2    = 1 << 2,
	PDC3    = 1 << 3,
	PDC4    = 1 << 4,
	PDC5    = 1 << 5,
	PDC6    = 1 << 6,
	PRSEN   = 1 << 7	// PWM Restart Enable bit
  };


  PWM1CON(Processor *pCpu, const char *pName, const char *pDesc=0);
  ~PWM1CON();

  void put(unsigned int new_value);
  void setBitMask(unsigned int bm) { mValidBits = bm; }

private:
};
//
// Enhanced PWM Pulse Steering control register
//
class PSTRCON : public sfr_register
{
public:

  /* Bit definitions for the register */
  enum {
	STRA    = 1 << 0,	// Steering enable bit A
	STRB    = 1 << 1,	// Steering enable bit B
	STRC    = 1 << 2,	// Steering enable bit C
	STRD    = 1 << 3,	// Steering enable bit D
	STRSYNC = 1 << 4,	// Steering Sync bit
  };


  PSTRCON(Processor *pCpu, const char *pName, const char *pDesc=0);
  ~PSTRCON();

  void put(unsigned int new_value);

private:
};

//---------------------------------------------------------
// CCPCON - Capture and Compare Control register
//---------------------------------------------------------
class CCPSignalSource;  // I/O pin interface
class CCPSignalSink;    // I/O pin interface
class Tristate;		// I/O pin high impedance
class CCP12CON;

class CCPCON : public sfr_register, public TriggerObject
{
public:

  /* Bit definitions for the register */
  enum {
    CCPM0 = 1 << 0,
    CCPM1 = 1 << 1,
    CCPM2 = 1 << 2,
    CCPM3 = 1 << 3,
    CCPY  = 1 << 4,
    CCPX  = 1 << 5,
    P1M0  = 1 << 6,	// CCP1 EPWM Output config bits 16f88x
    P1M1  = 1 << 7
  };

  /* Define the Modes (based on the CCPM bits) */
  enum {
    ALL_OFF0 = 0,
    ALL_OFF1 = 1,
    ALL_OFF2 = 2,
    ALL_OFF3 = 3,
    CAP_FALLING_EDGE = 4,
    CAP_RISING_EDGE  = 5,
    CAP_RISING_EDGE4 = 6,
    CAP_RISING_EDGE16 = 7,
    COM_SET_OUT = 8,
    COM_CLEAR_OUT = 9,
    COM_INTERRUPT = 10,
    COM_TRIGGER = 11,
    PWM0 = 12,
    PWM1 = 13,
    PWM2 = 14,
    PWM3 = 15
  };

  void setBitMask(unsigned int bv) { mValidBits = bv; }
  void new_edge(unsigned int level);
  void compare_match();
  void pwm_match(int new_state);
  void drive_bridge(int level, int new_value);
  void shutdown_bridge(int eccpas);
  void put(unsigned int new_value);
  char getState();
  bool test_compare_mode();
  void callback();
  void releasePins(int);
  void releaseSink();

  void setCrosslinks(CCPRL *, PIR *, unsigned int _mask, TMR2 *, ECCPAS *_eccpas=0);
  void setADCON(ADCON0 *);
  CCPCON(Processor *pCpu, const char *pName, const char *pDesc=0);
  ~CCPCON();
  void setIOpin(PinModule *p1, PinModule *p2=0, PinModule *p3=0, PinModule *p4=0);
  void setIOPin1(PinModule *p1);
  void setIOPin2(PinModule *p2);
  void set_tmr2(TMR2 *_tmr2) { tmr2 = _tmr2;}

  PSTRCON *pstrcon;
  PWM1CON *pwm1con;
  ECCPAS  *eccpas;

protected:
  PinModule 	*m_PinModule[4];
  CCPSignalSource *m_source[4];
  bool		source_active[4];
  CCPSignalSink *m_sink;
  Tristate	*m_tristate;
  bool  	m_bInputEnabled;    // Input mode for capture/compare
  bool  	m_bOutputEnabled;   // Output mode for PWM
  char  	m_cOutputState;
  int   	edges;
  guint64 	future_cycle;
  bool 		delay_source0, delay_source1;
  bool 		bridge_shutdown;


  CCPRL   *ccprl;
  PIR     *pir;
  TMR2    *tmr2;
  ADCON0  *adcon0;
  unsigned int pir_mask;

};


class TMR1_Freq_Attribute;
//---------------------------------------------------------
// T1CON - Timer 1 control register

class T1CON : public sfr_register
{
public:

enum
{
  TMR1ON  = 1<<0,
  TMR1CS  = 1<<1,
  T1SYNC  = 1<<2,
  T1OSCEN = 1<<3,
  T1CKPS0 = 1<<4,
  T1CKPS1 = 1<<5,
  T1RD16  = 1<<6,
  TMR1GE  = 1<<6,  // TMR1 Gate Enable used if TMR1L::setGatepin() has been called
  T1GINV  = 1<<7
};

  TMRL  *tmrl;
  TMR1_Freq_Attribute *freq_attribute;

  T1CON(Processor *pCpu, const char *pName, const char *pDesc=0);
  ~T1CON();

  unsigned int get();

  // For (at least) the 18f family, there's a 4X PLL that effects the
  // the relative timing between gpsim's cycle counter (which is equivalent
  // to the cumulative instruction count) and the external oscillator. In
  // all parts, the clock source for the timer is fosc, the external oscillator.
  // However, for the 18f parts, the instructions execute 4 times faster when
  // the PLL is selected.

  virtual unsigned int get_prescale();

  virtual unsigned int get_tmr1cs()
    {
      return((value.get() & TMR1CS)?2:0);
    }
  virtual bool get_tmr1on()
    {
      return(value.get() & TMR1ON);
    }
  virtual bool  get_t1oscen()
    {
      return(value.get() & T1OSCEN);
    }
  virtual bool get_tmr1GE()
    {
      return(value.get() & TMR1GE);
    }
  virtual bool get_t1GINV()
    {
      return(value.get() & T1GINV);
    }
  virtual bool get_t1sync()
    {
      return(value.get() & T1SYNC);
    }
  virtual void put(unsigned int new_value);

};



class T1GCon_GateSignalSink;
class T1CON_G;

// Timer 1 gate control Register
class T1GCON : public sfr_register
{
public:

  enum
  {
	T1GSS0	= 1<<0,		// Gate source select bits
	T1GSS1	= 1<<1,
	T1GVAL  = 1<<2,		// Current state bit
	T1GGO	= 1<<3,		// Gate Single-Pulse Acquisition Status bit
	T1GSPM	= 1<<4, 	// Gate Single-Pulse Mode bit
	T1GTM	= 1<<5,		// Gate Toggle Mode bit
	T1GPOL	= 1<<6,		// Gate Polarity bit
	TMR1GE  = 1<<7,		// Gate Enable bit
  };

  
  void put(unsigned int new_value);
  virtual void setGatepin(PinModule *);
  virtual void PIN_gate(bool);
  virtual void T0_gate(bool);
  virtual void T2_gate(bool);
  virtual void CM1_gate(bool);
  virtual void CM2_gate(bool);
  virtual void new_gate(bool);
  void set_WRmask(unsigned int mask) { write_mask = mask;}
  void set_tmrl(TMRL *_tmrl) { tmrl = _tmrl;}
  virtual void setInterruptSource(InterruptSource * _int) 
	{ m_Interrupt = _int;}
  virtual InterruptSource * getInterruptSource() { return m_Interrupt; }
  virtual bool get_tmr1GE()
    {
      return(value.get() & TMR1GE);
    }
  bool get_t1GPOL() { return (value.get() & T1GPOL); }
  virtual bool tmr1_isON();
  

  T1GCON(Processor *pCpu, const char *pName, const char *pDesc=0, T1CON_G *t1con_g=0);
  ~T1GCON();

private:

  T1GCon_GateSignalSink *sink;
  unsigned int 	write_mask;
  TMRL		*tmrl;
  T1CON_G	*t1con_g;
  InterruptSource *m_Interrupt;
  bool 		PIN_gate_state;
  bool 		T0_gate_state; // can also be Tx = PRx where x=2,4,6
  bool 		CM1_gate_state;
  bool 		CM2_gate_state;
  bool 		last_t1g_in;
  PinModule	*gate_pin;
};

//
// T1CON_G	combines T1CON and T1GCON into one virtual register
//
class T1CON_G : public T1CON
{
public:

  enum
  {
    TMR1ON  = 1<<0,
    T1SYNC  = 1<<2,
    T1OSCEN = 1<<3,
    T1CKPS0 = 1<<4,
    T1CKPS1 = 1<<5,
    TMR1CS0  = 1<<6,
    TMR1CS1  = 1<<7,
  };

  TMRL  *tmrl;
  TMR1_Freq_Attribute *freq_attribute;

  T1CON_G(Processor *pCpu, const char *pName, const char *pDesc=0);
  ~T1CON_G();

  void t1_cap_increment();

  // RRR unsigned int get();

  // For (at least) the 18f family, there's a 4X PLL that effects the
  // the relative timing between gpsim's cycle counter (which is equivalent
  // to the cumulative instruction count) and the external oscillator. In
  // all parts, the clock source for the timer is fosc, the external oscillator.
  // However, for the 18f parts, the instructions execute 4 times faster when
  // the PLL is selected.

  virtual unsigned int get_prescale() 
	{ return( ((value.get() &(T1CKPS0 | T1CKPS1)) >> 4) );}

  virtual unsigned int get_tmr1cs()
    {
      return((value.get() & (TMR1CS1 | TMR1CS0))>>6);
    }
  virtual bool get_tmr1on()
    {
      return(value.get() & TMR1ON);
    }
  virtual bool get_t1oscen()
    {
      return(value.get() & T1OSCEN);
    }
  virtual bool get_t1sync()
    {
      return(value.get() & T1SYNC);
    }
  virtual void put(unsigned int new_value);
  virtual bool get_tmr1GE() 
  {
    return t1gcon.get_tmr1GE();
  }
  virtual bool get_t1GINV() { return true;}

  T1GCON t1gcon;

};

//---------------------------------------------------------
// TMRL & TMRH - Timer 1
class TMRH : public sfr_register
{
public:

  TMRL *tmrl;

  void put(unsigned int new_value);
  unsigned int get();
  virtual unsigned int get_value();

  TMRH(Processor *pCpu, const char *pName, const char *pDesc=0);

};

class TMR1CapComRef;

class TMRL : public sfr_register, public TriggerObject, public SignalSink
{
public:

  TMRH  *tmrh;
  T1CON *t1con;

  unsigned int 
    prescale,
    prescale_counter,
    break_value,
    value_16bit;         /* Low and high concatenated */

  double ext_scale;

  TMR1CapComRef * compare_queue;

  guint64
    synchronized_cycle,
    future_cycle;
  gint64 last_cycle;  // last_cycle can be negative for small cycle counts


  virtual void callback();
  virtual void callback_print();

  TMRL(Processor *pCpu, const char *pName, const char *pDesc=0);
  ~TMRL();

  void set_ext_scale();

  virtual void release();

  virtual void put(unsigned int new_value);
  virtual unsigned int get();
  virtual unsigned int get_value();
  virtual unsigned int get_low_and_high();
  virtual void on_or_off(int new_state);
  virtual void current_value();
  virtual void new_clock_source();
  virtual void update();
  virtual void clear_timer();
  virtual void setSinkState(char);
  virtual void setIOpin(PinModule *);
  virtual void setGatepin(PinModule *);
  virtual void IO_gate(bool);
  virtual void compare_gate(bool);
  virtual void setInterruptSource(InterruptSource *);
  virtual InterruptSource * getInterruptSource() { return m_Interrupt; }
  virtual void sleep();
  virtual void wake();

  void set_compare_event ( unsigned int value, CCPCON *host );
  void clear_compare_event ( CCPCON *host );

  void set_T1GSS(bool arg);
  virtual void increment();   // Used when TMR1 is attached to an external clock

private:
  TMR1_Interface *tmr1_interface;
  char m_cState;
  bool m_GateState;		// Only changes state if setGatepin() has been called
  bool m_compare_GateState;
  bool m_io_GateState;
  bool m_bExtClkEnabled;
  bool m_sleeping;
  bool m_t1gss;			// T1 gate source
				// true - IO pin controls gate, 
				// false - compare controls gate
  InterruptSource *m_Interrupt;
};






class PR2 : public sfr_register
{
public:

  TMR2 *tmr2;

  PR2(Processor *pCpu, const char *pName, const char *pDesc=0);
  void put(unsigned int new_value);

};

//---------------------------------------------------------
// T2CON - Timer 2 control register

class T2CON : public sfr_register
{
public:

enum
{
  T2CKPS0 = 1<<0,
  T2CKPS1 = 1<<1,
  TMR2ON  = 1<<2,
  TOUTPS0 = 1<<3,
  TOUTPS1 = 1<<4,
  TOUTPS2 = 1<<5,
  TOUTPS3 = 1<<6
};

  TMR2 *tmr2;

  T2CON(Processor *pCpu, const char *pName, const char *pDesc=0);

  inline unsigned int get_t2ckps0()
    {
      return(value.get() & T2CKPS0);
    }

  inline unsigned int get_t2ckps1()
    {
      return(value.get() & T2CKPS1);
    }

  inline unsigned int get_tmr2on()
    {
      return(value.get() & TMR2ON);
    }

  inline unsigned int get_post_scale()
    {
      return( (value.get() & (TOUTPS0 | TOUTPS1 | TOUTPS2 | TOUTPS3)) >> 3 );
    }

  inline unsigned int get_pre_scale()
    {
      //  ps1:ps0 prescale
      //   0   0     1
      //   0   1     4
      //   1   x     16

      if(value.get() & T2CKPS1)
	return 16;
      else
	if(value.get() & T2CKPS0)
	  return 4;
	else
	  return 1;

    }

  void put(unsigned int new_value);

};

#define MAX_PWM_CHANS   5

//---------------------------------------------------------
// TMR2 - Timer
class TMR2 : public sfr_register, public TriggerObject
{
protected:
  CCPCON * ccp[MAX_PWM_CHANS];

public:
  /* Define the way in which the tmr2 callback function may be updated. */
  enum TMR2_UPDATE_TYPES
  {
    TMR2_WRAP        = 1<<0,	   // wrap TMR2
    TMR2_PR2_UPDATE  = 1<<1,       // update pr2 match
    TMR2_PWM1_UPDATE = 1<<2,       // wrt ccp1
//    TMR2_PWM2_UPDATE = 1<<3,       // wrt ccp2
    // PWM must be last as a variable number of channels follows
    TMR2_ANY_PWM_UPDATE = 0xfc,    // up to six PWM channels
    TMR2_DONTCARE_UPDATE = 0xff    // whatever comes next
  };

  int pwm_mode;
  int update_state;
  int last_update;

  unsigned int 
    prescale,
    prescale_counter,
    break_value,
    duty_cycle[MAX_PWM_CHANS];     /* for ccp channels */
  int 		post_scale;
  guint64	last_cycle;
  guint64	future_cycle;

  PR2  		*pr2;
  PIR_SET 	*pir_set;
  T2CON 	*t2con;
  SSP_MODULE	*ssp_module[2];
  T1GCON	*m_txgcon;

  virtual void callback();
  virtual void callback_print();
  TMR2(Processor *pCpu, const char *pName, const char *pDesc=0);
  ~TMR2();

  void put(unsigned int new_value);
  unsigned int get();
  void on_or_off(int new_state);
  void start();
  void new_pre_post_scale();
  void new_pr2(unsigned int new_value);
  void current_value();
  void update(int ut = TMR2_DONTCARE_UPDATE);
  void pwm_dc(unsigned int dc, unsigned int ccp_address);
  void stop_pwm(unsigned int ccp_address);
  virtual unsigned int get_value();
  virtual void setInterruptSource(InterruptSource * _int) 
	{ m_Interrupt = _int;}
  virtual InterruptSource * getInterruptSource() { return m_Interrupt; }

  bool add_ccp ( CCPCON * _ccp );
  bool rm_ccp(CCPCON *_ccp);
  InterruptSource *m_Interrupt;
};


//---------------------------------------------------------
//
// TMR2_MODULE
//
// 

class TMR2_MODULE
{
public:

  _14bit_processor *cpu;
  char * name_str;


  T2CON *t2con;
  PR2   *pr2;
  TMR2  *tmr2;

  TMR2_MODULE();
  void initialize(T2CON *t2con, PR2 *pr2, TMR2  *tmr2);

};

//---------------------------------------------------------
//
// TMR1_MODULE
//
// 

class TMR1_MODULE
{
public:

  _14bit_processor *cpu;
  char * name_str;

  T1CON *t1con;
  PIR_SET  *pir_set;

  TMR1_MODULE();
  void initialize(T1CON *t1con, PIR_SET *pir_set);

};

#endif
