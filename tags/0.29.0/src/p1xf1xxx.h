#ifndef __P1xF1xxx_H__
#define __P1xF1xxx_H__

#include "14bit-processors.h"
#include "14bit-tmrs.h"
#include "intcon.h"
#include "pir.h"
#include "pie.h"
#include "eeprom.h"
#include "comparator.h"
#include "a2dconverter.h"
#include "pic-ioports.h"

#define FOSC0 (1<<0)
#define FOSC1 (1<<1)
#define FOSC2 (1<<2)
#define IESO (1<<12)


class APFCON : public  sfr_register
{
 public:
  void put(unsigned int new_value);
  void set_pins(unsigned int bit, PinModule *pin0, PinModule *pin1)
  {
	m_bitPin[0][bit] = pin0;
	m_bitPin[1][bit] = pin1;
  }
  void set_usart(USART_MODULE    *_usart) { m_usart = _usart;};
  void set_ssp(SSP1_MODULE    *_ssp) { m_ssp = _ssp;};
  void set_t1gcon(T1GCON    *_t1gcon) { m_t1gcon = _t1gcon;};
  void set_ccpcon(CCPCON    *_ccpcon) { m_ccpcon = _ccpcon;};
  void set_ValidBits(unsigned int _mask){mValidBits = _mask;}

   APFCON(Processor *pCpu, const char *pName, const char *pDesc);
  
  private:

PinModule	*m_bitPin[2][8];
USART_MODULE 	*m_usart;
SSP1_MODULE 	*m_ssp;
T1GCON    	*m_t1gcon;
CCPCON		*m_ccpcon;


};

class P12F1822 : public _14bit_e_processor
{
public:
 ComparatorModule2 comparator;
  PIR_SET_2 pir_set_2_def;
  PIE     pie1;
  PIR    *pir1;
  PIE     pie2;
  PIR    *pir2;
  T2CON	  t2con;
  PR2	  pr2;
  TMR2    tmr2;
  T1CON_G   t1con_g;
  TMRL    tmr1l;
  TMRH    tmr1h;
  CCPCON	ccp1con;
  CCPRL		ccpr1l;
  CCPRH		ccpr1h;
  FVRCON	fvrcon;
  BORCON	borcon;
  ANSEL_P 	ansela;
  ADCON0  	adcon0;
  ADCON1_16F 	adcon1;
  sfr_register  adresh;
  sfr_register  adresl;
  OSCCON_2  	osccon;
  OSCTUNE 	osctune;
  OSCSTAT 	oscstat;
  //OSCCAL  osccal;
  WDTCON  	wdtcon;
  USART_MODULE 	usart;
  SSP1_MODULE 	ssp;
  APFCON	apfcon;
  PWM1CON	pwm1con;
  ECCPAS        ccp1as;
  PSTRCON       pstr1con;
  CPSCON0	cpscon0;
  CPSCON1	cpscon1;
  SR_MODULE	sr_module;
  EEPROM_EXTND *e;


  WPU              *m_wpua;
  IOC              *m_iocap;
  IOC              *m_iocan;
  IOCxF            *m_iocaf;
  PicPortIOCRegister  *m_porta;
  PicTrisRegister  *m_trisa;
  PicLatchRegister *m_lata;
  DACCON0	   *m_daccon0;
  DACCON1	   *m_daccon1;

  virtual PIR *get_pir2() { return (NULL); }
  virtual PIR *get_pir1() { return (pir1); }
  virtual PIR_SET *get_pir_set() { return (&pir_set_2_def); }

  virtual EEPROM_EXTND *get_eeprom() { return ((EEPROM_EXTND *)eeprom); }

  virtual PROCESSOR_TYPE isa(){return _P12F1822_;};

 P12F1822(const char *_name=0, const char *desc=0);
  ~P12F1822();
  static Processor *construct(const char *name);
  virtual void create_sfr_map();
  virtual void create_symbols();
  virtual void set_out_of_range_pm(unsigned int address, unsigned int value);
  virtual void create_iopin_map();
  virtual void create(int ram_top, int eeprom_size);
  virtual unsigned int register_memory_size () const { return 0x1000; }
  virtual void option_new_bits_6_7(unsigned int bits);
  virtual unsigned int program_memory_size() const { return 2048; }
  virtual void enter_sleep();
  virtual void exit_sleep();
  virtual void oscillator_select(unsigned int mode, bool clkout);
  virtual void program_memory_wp(unsigned int mode);


};

class P16F178x : public _14bit_e_processor
{
public:
 ComparatorModule2 comparator;
  PIR_SET_2 pir_set_2_def;
  PIE     pie1;
  PIR    *pir1;
  PIE     pie2;
  PIR    *pir2;
  PIE     pie3;
  PIR    *pir3;
  T2CON	  t2con;
  PR2	  pr2;
  TMR2    tmr2;
  T1CON_G   t1con_g;
  TMRL    tmr1l;
  TMRH    tmr1h;
  CCPCON	ccp1con;
  CCPRL		ccpr1l;
  CCPRH		ccpr1h;
  FVRCON	fvrcon;
  BORCON	borcon;
  ANSEL_P 	ansela;
  ANSEL_P   	anselb;
  ANSEL_P 	anselc;
  ADCON0_DIF  	adcon0;
  ADCON1_16F 	adcon1;
  ADCON2_DIF	adcon2;
  sfr_register  adresh;
  sfr_register  adresl;
  OSCCON_2  	osccon;
  OSCTUNE 	osctune;
  OSCSTAT 	oscstat;
  //OSCCAL  osccal;
  WDTCON  	wdtcon;
  USART_MODULE 	usart;
  SSP1_MODULE 	ssp;
  APFCON	apfcon1;
  APFCON	apfcon2;
  PWM1CON	pwm1con;
  ECCPAS        ccp1as;
  PSTRCON       pstr1con;
  EEPROM_EXTND *e;


  WPU              *m_wpua;
  IOC              *m_iocap;
  IOC              *m_iocan;
  IOCxF            *m_iocaf;
  PicPortIOCRegister  *m_porta;
  PicTrisRegister  *m_trisa;
  PicLatchRegister *m_lata;
  IOC              *m_iocep;
  IOC              *m_iocen;
  IOCxF            *m_iocef;
  PicPortIOCRegister  *m_porte;
  PicTrisRegister  *m_trise;
  WPU              *m_wpue;
  DACCON0	   *m_daccon0;
  DACCON1	   *m_daccon1;
  DACCON0	   *m_dac2con0;
  DACCON1	   *m_dac2con1;
  DACCON0	   *m_dac3con0;
  DACCON1	   *m_dac3con1;
  DACCON0	   *m_dac4con0;
  DACCON1	   *m_dac4con1;
  IOC              *m_iocbp;
  IOC              *m_iocbn;
  IOCxF            *m_iocbf;
  PicPortBRegister  *m_portb;
  PicTrisRegister  *m_trisb;
  PicLatchRegister *m_latb;
  WPU              *m_wpub;

  IOC              *m_ioccp;
  IOC              *m_ioccn;
  IOCxF            *m_ioccf;
  PicPortBRegister  *m_portc;
  PicTrisRegister  *m_trisc;
  PicLatchRegister *m_latc;
  WPU              *m_wpuc;

  virtual PIR *get_pir2() { return (NULL); }
  virtual PIR *get_pir1() { return (pir1); }
  virtual PIR_SET *get_pir_set() { return (&pir_set_2_def); }

  virtual EEPROM_EXTND *get_eeprom() { return ((EEPROM_EXTND *)eeprom); }

 P16F178x(const char *_name=0, const char *desc=0);
  ~P16F178x();
  virtual void create_sfr_map();
  virtual void create_symbols();
  virtual void set_out_of_range_pm(unsigned int address, unsigned int value);
  virtual void create(int ram_top, int eeprom_size);
  virtual void option_new_bits_6_7(unsigned int bits);
  virtual void enter_sleep();
  virtual void exit_sleep();
  virtual void oscillator_select(unsigned int mode, bool clkout);
  virtual void program_memory_wp(unsigned int mode);

  unsigned int ram_size;


};

class P16F1788 : public P16F178x
{
public:
  virtual PROCESSOR_TYPE isa(){return _P16F1788_;};

 P16F1788(const char *_name=0, const char *desc=0);
  ~P16F1788();
  static Processor *construct(const char *name);
  virtual void create_sfr_map();
  virtual void create_iopin_map();
  virtual void create(int ram_top, int eeprom_size);
  virtual unsigned int program_memory_size() const { return 16384; }
  virtual unsigned int register_memory_size () const { return 0x1000; }

};
class P16F1823 : public P12F1822
{
public:
  ANSEL_P   anselc;
  virtual PROCESSOR_TYPE isa(){return _P16F1823_;};

 P16F1823(const char *_name=0, const char *desc=0);
  ~P16F1823();
  static Processor *construct(const char *name);
  virtual void create_sfr_map();
  virtual void create_iopin_map();
  virtual void create(int ram_top, int eeprom_size);

  PicPortBRegister  *m_portc;
  PicTrisRegister  *m_trisc;
  PicLatchRegister *m_latc;
  WPU              *m_wpuc;
};
#endif //__P1xF1xxx_H__
