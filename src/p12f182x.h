#ifndef __P12F182x_H__
#define __P12F182x_H__

#include "14bit-processors.h"
#include "14bit-tmrs.h"
#include "intcon.h"
#include "pir.h"
#include "pie.h"
#include "eeprom.h"
#include "comparator.h"
#include "a2dconverter.h"
#include "pic-ioports.h"

class P12F1822 : public _14bit_e_processor
{
public:
 ComparatorModule comparator;
  PIR_SET_2 pir_set_2_def;
  PIE     pie1;
  PIR    *pir1;
  PIE     pie2;
  PIR	  *pir2;
  T2CON	  t2con;
  PR2	  pr2;
  TMR2    tmr2;
  T1CON   t1con;
  TMRL    tmr1l;
  TMRH    tmr1h;
  CCPCON  ccp1con;
  CCPRL   ccpr1l;
  CCPRH   ccpr1h;
  FVRCON  fvrcon;
  BORCON  borcon;
  ANSEL_P ansela;
  ADCON0  adcon0;
  ADCON1_16F adcon1;
  sfr_register  adresh;
  sfr_register  adresl;
  OSCCON  osccon;
  OSCTUNE osctune;
  WDTCON  wdtcon;
  EEPROM_EXTND *e;


  WPU              *m_wpua;
  IOC              *m_iocap;
  IOC              *m_iocan;
  IOC              *m_iocaf;
  PicPortIOCRegister  *m_porta;
  PicTrisRegister  *m_trisa;
  PicLatchRegister *m_lata;

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
#endif //__P12F182x_H__
