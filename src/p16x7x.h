/*
   Copyright (C) 1998 T. Scott Dattalo

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

#ifndef __P16X7X_H__
#define __P16X7X_H__


#include "p16x6x.h"    /* The '7x stuff is like '6x stuff with a/d converters */
#include "pir.h"
#include "a2dconverter.h"
#include "pm_rd.h"

//---------------------------------------------------------

class P16C71 :  public P16C61
{
 public:

  ADCON0 adcon0;
  ADCON1 adcon1;
  sfr_register  adres;

  virtual PROCESSOR_TYPE isa(){return _P16C71_;};
  virtual void create_symbols();

  virtual unsigned int program_memory_size() const { return 0x400; };
  virtual void create_sfr_map();


  P16C71(const char *_name=0, const char *desc=0);
  void create();
  static Processor *construct(const char *name);
private:
  // This is not a real PIR register, but only one that allows the A2D Interrupt
  // flag be processed in manner similar to other processors.
  class PIR_16C71;
  PIR_16C71 *m_pir;
};

class P16C712 :  public P16C62
{
 public:

  ADCON0 adcon0;
  ADCON1 adcon1;
  sfr_register  adres;

  virtual PROCESSOR_TYPE isa(){return _P16C712_;};

  virtual void create_sfr_map();


  P16C712(const char *_name=0, const char *desc=0);
  void create();
  static Processor *construct(const char *name);

  virtual bool hasSSP() { return false; }
};

class P16C716 :  public P16C712
{
 public:

  virtual PROCESSOR_TYPE isa(){return _P16C716_;};

  virtual unsigned int program_memory_size() const { return 0x800; };

  P16C716(const char *_name=0, const char *desc=0);
  static Processor *construct(const char *name);

};

class P16C72 : public P16C62 
{
 public:
  // XXX
  // This pir1_2, pir2_2 stuff is not particularly pretty.  It would be
  // better to just tell C++ to redefine pir1 and pir2 and PIR1v2 and
  // PIR2v2, but C++ only supports covariance in member function return
  // values.
  PIR1v2 *pir1_2_reg;
  PIR2v2 *pir2_2_reg;
  PIR_SET_2 pir_set_2_def;
  ADCON0 adcon0;
  ADCON1 adcon1;
  sfr_register  adres;

  virtual PROCESSOR_TYPE isa(){return _P16C72_;};
  virtual void create_symbols();
  void create_sfr_map();
  virtual PIR *get_pir1() { return (pir1_2_reg); }
  virtual PIR *get_pir2() { return (pir2_2_reg); }
  virtual PIR_SET *get_pir_set() { return (&pir_set_2_def); }


  P16C72(const char *_name=0, const char *desc=0);
  void create();
  static Processor *construct(const char *name);

};

class P16C73 : public P16C63
{
 public:
  // XXX
  // This pir1_2, pir2_2 stuff is not particularly pretty.  It would be
  // better to just tell C++ to redefine pir1 and pir2 and PIR1v2 and
  // PIR2v2, but C++ only supports covariance in member function return
  // values.
  PIR1v2 *pir1_2_reg;
  PIR2v2 *pir2_2_reg;
  PIR_SET_2 pir_set_2_def;
  ADCON0 adcon0;
  ADCON1 adcon1;
  sfr_register  adres;

  virtual PROCESSOR_TYPE isa(){return _P16C73_;};
  virtual void create_symbols();
  void create_sfr_map();
  virtual PIR *get_pir1() { return (pir1_2_reg); }
  virtual PIR *get_pir2() { return (pir2_2_reg); }
  virtual PIR_SET *get_pir_set() { return (&pir_set_2_def); }

  P16C73(const char *_name=0, const char *desc=0);
  void create();
  static Processor *construct(const char *name);

};

class P16F73 : public P16C73
{
public:

  virtual PROCESSOR_TYPE isa(){return _P16F73_;};
  virtual unsigned int register_memory_size () const { return 0x200;};
  virtual void create_symbols();
  void create_sfr_map();
  P16F73(const char *_name=0, const char *desc=0);
  void create();
  static Processor *construct(const char *name);

protected:
  PM_RD pm_rd;
};

//---------------------------------------------------------

class P16C74 : public P16C65 // Not a typo, a 'c74 is more like a 'c65 then a 'c64!
{
 public:
  // XXX
  // This pir1_2, pir2_2 stuff is not particularly pretty.  It would be
  // better to just tell C++ to redefine pir1 and pir2 and PIR1v2 and
  // PIR2v2, but C++ only supports covariance in member function return
  // values.
  PIR1v2 *pir1_2_reg;
  PIR2v2 *pir2_2_reg;
  PIR_SET_2 pir_set_2_def;
  ADCON0 adcon0;
  ADCON1 adcon1;
  sfr_register  adres;

  virtual PROCESSOR_TYPE isa(){return _P16C74_;};
  virtual void create_symbols();
  void create_sfr_map();
  virtual PIR *get_pir1() { return (pir1_2_reg); }
  virtual PIR *get_pir2() { return (pir2_2_reg); }
  virtual PIR_SET *get_pir_set() { return (&pir_set_2_def); }

  virtual unsigned int program_memory_size() const { return 0x1000; };


  P16C74(const char *_name=0, const char *desc=0);
  void create();
  static Processor *construct(const char *name);

};

class P16F74 : public P16C74
{
public:

  virtual PROCESSOR_TYPE isa(){return _P16F74_;};
  virtual unsigned int register_memory_size () const { return 0x200;};
  virtual void create_symbols();
  void create_sfr_map();
  P16F74(const char *_name=0, const char *desc=0);
  void create();
  static Processor *construct(const char *name);

protected:
  PM_RD pm_rd;
};


#endif

