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

#ifndef __P17C75X_H__
#define __P17C75X_H__

#include "16bit-processors.h"
#include "p16x6x.h"


class P17C7xx : public  _16bit_processor
{
public:

  CPUSTA cpusta;
  
  P17C7xx();

  static Processor *construct(const char *name);
  virtual PROCESSOR_TYPE isa(){return _P17C7xx_;};
  virtual void create_symbols();
  virtual void create(int ram_top);

  virtual void create_sfr_map();
  virtual unsigned int program_memory_size() const { return 0x400; };

};

class P17C75x : public P17C7xx
{
 public:

  P17C75x();
  static Processor *construct(const char *name);
  virtual void create(int ram_top);
  virtual void create_sfr_map();
  
  virtual PROCESSOR_TYPE isa(){return _P17C75x_;};
  virtual void create_symbols();
  
  virtual unsigned int program_memory_size() const { return 0x4000; };

};

class P17C752 : public P17C75x
{
 public:
  virtual PROCESSOR_TYPE isa(){return _P17C752_;};
  P17C752();
  static Processor *construct(const char *name);
  void create();
  //  void create_sfr_map();

  void create_sfr_map();
  void create_symbols();
  virtual unsigned int program_memory_size() const { return 0x2000; };
  virtual unsigned int register_memory_size() const { return 0x800; };

};

class P17C756 : public P17C75x
{
 public:

  virtual PROCESSOR_TYPE isa(){return _P17C756_;};
  void create_sfr_map();
  void create_symbols();

  P17C756();
  static Processor *construct(const char *name);
  void create();

  virtual unsigned int program_memory_size() const { return 0x4000; };
  virtual unsigned int register_memory_size() const { return 0x800; };


};

class P17C756A : public P17C75x
{
 public:

  virtual PROCESSOR_TYPE isa(){return _P17C756A_;};
  void create_sfr_map();
  void create_symbols();

  P17C756A();
  static Processor *construct(const char *name);
  void create();

  virtual unsigned int program_memory_size() const { return 0x4000; };
  virtual unsigned int register_memory_size() const { return 0x800; };


};

class P17C762 : public P17C75x
{
 public:

  virtual PROCESSOR_TYPE isa(){return _P17C762_;};
  void create_sfr_map();
  void create_symbols();

  P17C762();
  static Processor *construct(const char *name);
  void create();

  virtual unsigned int program_memory_size() const { return 0x4000; };
  virtual unsigned int register_memory_size() const { return 0x800; };


};

class P17C766 : public P17C75x
{
 public:

  virtual PROCESSOR_TYPE isa(){return _P17C766_;};
  void create_sfr_map();
  void create_symbols();

  P17C766();
  static Processor *construct(const char *name);
  void create();

  virtual unsigned int program_memory_size() const { return 0x4000; };
  virtual unsigned int register_memory_size() const { return 0x800; };


};

#endif
