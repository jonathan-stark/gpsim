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
along with gpasm; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#ifndef __14_BIT_TMRS_H__
#define __14_BIT_TMRS_H__

#include "pic-processor.h"
#include "trace.h"
#include "14bit-processors.h"

class IOPIN;

class PIR;
class PIR1;
class TMR1L;
class TMR1H;
class TMR2;
class CCPRL;
class ADCON0;

//---------------------------------------------------------
// CCPCON - Capture and Compare registers
//---------------------------------------------------------
class CCPRH : public sfr_register
{
public:

  CCPRL *ccprl;
  bool  pwm_mode;
  unsigned int pwm_value;

  CCPRH(void);
  void put(unsigned int new_value);
  unsigned int get(void);

};


class CCPRL : public sfr_register
{
public:

  CCPRH  *ccprh;
  TMR1L  *tmr1l;

  void put(unsigned int new_value);
  void capture_tmr(void);
  void start_compare_mode(void);
  void stop_compare_mode(void);
  void start_pwm_mode(void);
  CCPRL(void);
};


//---------------------------------------------------------
// CCPCON - Capture and Compare Control register
//---------------------------------------------------------
class CCPCON : public sfr_register
{
public:

  /* Bit definitions for the register */
enum
{
  CCPM0 = 1 << 0,
  CCPM1 = 1 << 1,
  CCPM2 = 1 << 2,
  CCPM3 = 1 << 3,
  CCPY  = 1 << 4,
  CCPX  = 1 << 5
};

  /* Define the Modes (based on the CCPM bits) */
enum
{
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

  int edges;
  CCPRL *ccprl;
  IOPIN *iopin;
  PIR   *pir;
  TMR2  *tmr2;
  ADCON0 *adcon0;

  void new_edge(unsigned int level);
  void compare_match(void);
  void pwm_match(int new_state);
  void put(unsigned int new_value);
  CCPCON(void);

};
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
  T1CKPS1 = 1<<5
};

  TMR1L  *tmr1l;

  T1CON(void);

  unsigned int get(void)
    {
      trace.register_read(address,value);
      return(value);
    }

  unsigned int get_prescale(void)
    {
      return( (value &(T1CKPS0 | T1CKPS1)) >> 4);
    }
  unsigned int get_tmr1cs(void)
    {
      return(value & TMR1CS);
    }
  unsigned int get_tmr1on(void)
    {
      return(value & TMR1ON);
    }
  void put(unsigned int new_value);

};


//---------------------------------------------------------
// TMR1L & TMR1H - Timer 1
class TMR1H : public sfr_register
{
public:

  TMR1L *tmr1l;

  void put(unsigned int new_value);
  unsigned int get(void);
  virtual unsigned int get_value(void);

  TMR1H(void);

};

class TMR1L : public sfr_register, public BreakCallBack
{
public:

  TMR1H *tmr1h;
  T1CON *t1con;
  PIR1  *pir1;
  CCPCON *ccpcon;

  unsigned int 
    prescale,
    prescale_counter,
    break_value,
    compare_value,
    value_16bit;         /* Low and high concatenated */

  guint64
    synchronized_cycle,
    last_cycle,
    future_cycle;

  bool compare_mode;

  virtual void callback(void);

  TMR1L(void);

  virtual void put(unsigned int new_value);
  virtual unsigned int get(void);
  virtual unsigned int get_value(void);
  virtual unsigned int get_low_and_high(void);
  virtual void on_or_off(int new_state);
  virtual void increment(void);   // Used when TMR1 is attached to an external clock
  virtual void current_value(void);
  virtual void new_clock_source(void);
  virtual void update(void);
  virtual void clear_timer(void);
};


//---------------------------------------------------------
// PIE Peripheral Interrupt Enable register base class 
// for PIE1 & PIE2

class PIE : public sfr_register
{
public:
  PIR *pir;

  void put(unsigned int new_value);
  
};

//---------------------------------------------------------
// PIR Peripheral Interrupt register base class for PIR1 & PIR2

class PIR_base
{
public:

  virtual void set_ccpif(void)=0;
  virtual bool interrupt_status(void)=0;
};


class PIR : public sfr_register, public PIR_base
{
public:
  INTCON  *intcon;
  PIE     *pie;

  virtual void set_ccpif(void){cout<<"PIR ccpif\n";};
  virtual bool interrupt_status(void){cout<<"PIR intstat\n";};

};


//---------------------------------------------------------
// PIR1 Peripheral Interrupt register # 1

class PIR1 : public PIR
{
public:

enum
{
    TMR1IF  = 1<<0,
    TMR2IF  = 1<<1,
    CCP1IF  = 1<<2,
    SSPIF   = 1<<3,
    TXIF    = 1<<4,
    RCIF    = 1<<5,
    ADIF    = 1<<6,     // 18cxxx
    PSPIF   = 1<<7
};

//  int VALID_BITS;
  int valid_bits;
 
  inline void set_tmr1if(void)
    {
      put(get() | TMR1IF);
    }

  inline void set_tmr2if(void)
    {
      put(get() | TMR2IF);
    }

  void set_ccpif(void)
    {
      put(get() | CCP1IF);
    }

  inline void set_sspif(void)
    {
      put(get() | SSPIF);
    }

  inline void set_txif(void)
    {
      put(get() | TXIF);
    }

  inline void set_rcif(void)
    {
      put(get() | RCIF);
    }

  inline void set_adif(void)
    {
      put(get() | ADIF);
    }

  inline void set_pspif(void)
    {
      put(get() | PSPIF);
    }

 unsigned int get_txif(void)
   {
     return value & TXIF;
   }
 void clear_txif(void)
   {
     value &= ~TXIF;
     trace.register_write(address,value);
   }
 void clear_rcif(void)
   {
     value &= ~RCIF;
     trace.register_write(address,value);
   }
 
  bool interrupt_status(void)
    {
      if( value & valid_bits & pie->value)
	return(1);
      else
	return(0);

    }

  void put(unsigned int new_value)
    {
      value = new_value;
      trace.register_write(address,value);

      if( value & valid_bits & pie->value )
	{
	  intcon->peripheral_interrupt();
	}

    }

  PIR1(void);
};

//---------------------------------------------------------
// PIR2 Peripheral Interrupt register # 2

class PIR2 : public PIR
{
public:

  /*
  INTCON  *intcon;
  PIE     *pie;
  */

#define  CCP2IF   1<<0

  //  const int
  //  VALID_BITS = CCP2IF;

  void set_ccpif(void)
    {
      put(get() | CCP2IF);
    }

  void put(unsigned int new_value)
    {
      value = new_value;
      trace.register_write(address,value);

      if( value & CCP2IF & pie->value )
	{
	  intcon->peripheral_interrupt();
	}

    }

  bool interrupt_status(void)
    {
      if( value & CCP2IF & pie->value)
	return(1);
      else
	return(0);

    }


  PIR2(void);
};




class PR2 : public sfr_register
{
public:

  TMR2 *tmr2;

  PR2(void);
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

  T2CON(void);

  inline unsigned int get_t2ckps0(void)
    {
      return(value & T2CKPS0);
    }

  inline unsigned int get_t2ckps1(void)
    {
      return(value & T2CKPS1);
    }

  inline unsigned int get_tmr2on(void)
    {
      return(value & TMR2ON);
    }

  inline unsigned int get_post_scale(void)
    {
      return( (value & (TOUTPS0 | TOUTPS1 | TOUTPS2 | TOUTPS3)) >> 3 );
    }

  inline unsigned int get_pre_scale(void)
    {
      //  ps1:ps0 prescale
      //   0   0     1
      //   0   1     4
      //   1   x     16

      if(value & T2CKPS1)
	return 16;
      else
	if(value & T2CKPS0)
	  return 4;
	else
	  return 1;

    }

  void put(unsigned int new_value);

};



//---------------------------------------------------------
// TMR2 - Timer
class TMR2 : public sfr_register, public BreakCallBack
{
public:
  /* Define the way in which the tmr2 callback function may be updated. */
  enum TMR2_UPDATE_TYPES
  {
    TMR2_PWM1_UPDATE = 1<<0,       // wrt ccp1
    TMR2_PWM2_UPDATE = 1<<1,       // wrt ccp2
    TMR2_PR2_UPDATE  = 1<<2,       // update pr2 match
    TMR2_DONTCARE_UPDATE = 7       // whatever comes next
  };

  int pwm_mode;
  int update_state;
  int last_update;

  unsigned int 
    prescale,
    prescale_counter,
    break_value,
    duty_cycle1,     /* for ccp1 */
    duty_cycle2;     /* for ccp2 */
  int
    post_scale;
  guint64
    synchronized_cycle,
    last_cycle,
    future_cycle;

  PR2  *pr2;
  PIR1 *pir1;
  T2CON *t2con;
  CCPCON *ccp1con;
  CCPCON *ccp2con;

  virtual void callback(void);

  TMR2(void);

  void put(unsigned int new_value);
  unsigned int get(void);
  void start(void);
  void new_pre_post_scale(void);
  void new_pr2(void);
  void current_value(void);
  void update(int ut = TMR2_DONTCARE_UPDATE);
  void pwm_dc(unsigned int dc, unsigned int ccp_address);
  void stop_pwm(unsigned int ccp_address);

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

  TMR2_MODULE(void);
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
  PIR1  *pir1;

  TMR1_MODULE(void);
  void initialize(T1CON *t1con, PIR1 *pir1);

};

#endif
