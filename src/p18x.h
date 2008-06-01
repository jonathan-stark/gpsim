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

#ifndef __P18X_H__
#define __P18X_H__

#include "16bit-processors.h"
#include "eeprom.h"
#include "psp.h"
#include "comparator.h"

class PicPortRegister;
class PicTrisRegister;
class PicLatchRegister;

class P18C2x2 : public _16bit_processor
{
 public:

  P18C2x2(const char *_name=0, const char *desc=0);

  void create();

  virtual PROCESSOR_TYPE isa(){return _P18Cxx2_;};
  virtual PROCESSOR_TYPE base_isa(){return _PIC18_PROCESSOR_;};
  virtual void create_symbols();

  virtual unsigned int program_memory_size() const { return 0x400; };

  virtual void create_iopin_map();

};

class P18C242 : public P18C2x2
{
 public:
  virtual PROCESSOR_TYPE isa(){return _P18C242_;};
  P18C242(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  void create();

  virtual unsigned int program_memory_size() const { return 0x2000; };
  virtual unsigned int last_actual_register () const { return 0x02FF;};

};

class P18C252 : public P18C242
{
 public:

  virtual PROCESSOR_TYPE isa(){return _P18C252_;};
  P18C252(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  void create();

  virtual unsigned int program_memory_size() const { return 0x4000; };
  virtual unsigned int last_actual_register () const { return 0x05FF;};


};

/*********************************************************************
 *  class definitions for the 18C4x2 family
 */

class P18C4x2 : public _16bit_processor
{
 public:


  PicPSP_PortRegister  *m_portd;
  PicTrisRegister  *m_trisd;
  PicLatchRegister *m_latd;

  PicPortRegister  *m_porte;
  PicPSP_TrisRegister  *m_trise;
  PicLatchRegister *m_late;

  PSP               psp;

  P18C4x2(const char *_name=0, const char *desc=0);

  void create();

  virtual PROCESSOR_TYPE isa(){return _P18Cxx2_;};
  virtual PROCESSOR_TYPE base_isa(){return _PIC18_PROCESSOR_;};
  virtual void create_symbols();

  virtual unsigned int program_memory_size() const { return 0x400; };

  virtual void create_sfr_map();
  virtual void create_iopin_map();

};


class P18C442 : public P18C4x2
{
 public:
  virtual PROCESSOR_TYPE isa(){return _P18C442_;};
  P18C442(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  void create();
  virtual unsigned int program_memory_size() const { return 0x2000; };
  virtual unsigned int last_actual_register () const { return 0x02FF;};

};


class P18C452 : public P18C442
{
 public:
  virtual PROCESSOR_TYPE isa(){return _P18C452_;};
  P18C452(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  void create();
  virtual unsigned int program_memory_size() const { return 0x4000; };
  virtual unsigned int last_actual_register () const { return 0x05FF;};

};

class P18F242 : public P18C242
{
 public:
  virtual PROCESSOR_TYPE isa(){return _P18F242_;};
  P18F242(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  void create();
  virtual unsigned int program_memory_size() const { return 0x2000; };

  virtual void set_out_of_range_pm(unsigned int address, unsigned int value);
  virtual void set_eeprom(EEPROM *ep) {
    // Use set_eeprom_pir as the 18Fxxx devices use an EEPROM with PIR
   assert(0);
  }
  virtual void set_eeprom_pir(EEPROM_PIR *ep) { eeprom = ep; }
  virtual EEPROM_PIR *get_eeprom() { return ((EEPROM_PIR *)eeprom); }

};

class P18F252 : public P18F242
{
 public:
  virtual PROCESSOR_TYPE isa(){return _P18F252_;};
  P18F252(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  void create();
  virtual unsigned int program_memory_size() const { return 0x4000; };

};

class P18F442 : public P18C442
{
 public:
  virtual PROCESSOR_TYPE isa(){return _P18F442_;};
  P18F442(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  void create();
  virtual unsigned int program_memory_size() const { return 0x2000; };

  virtual void set_out_of_range_pm(unsigned int address, unsigned int value);
  virtual void set_eeprom(EEPROM *ep) {
    // Use set_eeprom_pir as the 18Fxxx devices use an EEPROM with PIR
   assert(0);
  }
  virtual void set_eeprom_pir(EEPROM_PIR *ep) { eeprom = ep; }
  virtual EEPROM_PIR *get_eeprom() { return ((EEPROM_PIR *)eeprom); }

};

//
// The P18F248 is the same as the P18F242 except it has CAN, one fewer
// CCP module and a 5/10 ADC.  For now just assume it is identical.
class P18F248 : public P18F242
{
 public:
  virtual PROCESSOR_TYPE isa(){return _P18F248_;};
  P18F248(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  void create();
};
 
//
// The P18F448 is the same as the P18F442 except it has CAN, one fewer
// CCP module and a 5/10 ADC.  For now just assume it is identical.
class P18F448 : public P18F442
{
 public:
  virtual PROCESSOR_TYPE isa(){return _P18F448_;};
  P18F448(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  void create();
};
 

class P18F452 : public P18F442
{
 public:
  virtual PROCESSOR_TYPE isa(){return _P18F452_;};
  P18F452(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  void create();

  virtual unsigned int program_memory_size() const { return 0x4000; };

};

class P18F2455 : public P18F442
{
 public:
  virtual PROCESSOR_TYPE isa(){return _P18F2455_;};
  P18F2455(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  void create();

  virtual unsigned int program_memory_size() const { return 0x40000; };

};

class P18Fxx20 : public _16bit_processor
{
public:
  P18Fxx20(const char *_name=0, const char *desc=0);
  virtual PROCESSOR_TYPE base_isa(){return _PIC18_PROCESSOR_;};
};

class P18F1220 : public P18Fxx20
{
 public:
  virtual PROCESSOR_TYPE isa(){return _P18F1220_;};
  P18F1220(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  void create();
  virtual void create_iopin_map();
  virtual unsigned int program_memory_size() const { return 0x1000; };

};


class P18F1320 : public P18F1220
{
 public:
  virtual PROCESSOR_TYPE isa(){return _P18F1320_;};
  P18F1320(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  void create();

  virtual unsigned int program_memory_size() const { return 0x2000; };

};



class P18F2x21 : public _16bit_processor
{
 public:

  PicPortRegister  *m_porte;
  PicPSP_TrisRegister  *m_trise;
  PicLatchRegister *m_late;

//  OSCTUNE      osctune;
  ComparatorModule comparator;

  P18F2x21(const char *_name=0, const char *desc=0);

  void create();

  virtual PROCESSOR_TYPE isa(){return _P18Cxx2_;};
  virtual PROCESSOR_TYPE base_isa(){return _PIC18_PROCESSOR_;};
  virtual void create_symbols();

  virtual unsigned int program_memory_size() const { return 0x400; };

// Setting the correct register memory size breaks things
//  virtual unsigned int register_memory_size () const { return 0x200;};
  virtual unsigned int last_actual_register () const { return 0x01FF;};

  virtual void create_iopin_map();
  virtual void create_sfr_map();


  virtual void set_out_of_range_pm(unsigned int address, unsigned int value);
  virtual void set_eeprom(EEPROM *ep) {
    // Use set_eeprom_pir as the 18Fxxx devices use an EEPROM with PIR
   assert(0);
  }
  virtual void set_eeprom_pir(EEPROM_PIR *ep) { eeprom = ep; }
  virtual EEPROM_PIR *get_eeprom() { return ((EEPROM_PIR *)eeprom); }
};

class P18F2321 : public P18F2x21
{
 public:
  virtual PROCESSOR_TYPE isa(){return _P18F2321_;};
  P18F2321(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  void create();

  virtual unsigned int program_memory_size() const { return 0x1000; };

};


class P18F4x21 : public P18F2x21
{
 public:

  PicPSP_PortRegister  *m_portd;
  PicTrisRegister  *m_trisd;
  PicLatchRegister *m_latd;

  PSP               psp;

  P18F4x21(const char *_name=0, const char *desc=0);

  void create();

  virtual void create_symbols();

  virtual void create_iopin_map();
  virtual void create_sfr_map();

};

class P18F4321 : public P18F4x21
{
 public:
  virtual PROCESSOR_TYPE isa(){return _P18F4321_;};
  P18F4321(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);
  void create();

  virtual unsigned int program_memory_size() const { return 0x1000; };

};


#endif
