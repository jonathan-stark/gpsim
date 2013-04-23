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
  ANSEL   ansela;
  ADCON0_12F adcon0;
  ADCON1_16F adcon1;
  sfr_register  adresh;
  sfr_register  adresl;
  OSCCAL  osccal;
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
//RRR  virtual void create_config_memory();
//RRR  virtual bool set_config_word(unsigned int address,unsigned int cfg_word);
  virtual void enter_sleep();
  virtual void exit_sleep();
  virtual void oscillator_select(unsigned int mode, bool clkout);
  virtual void program_memory_wp(unsigned int mode);


};

#endif //__P12F182x_H__
