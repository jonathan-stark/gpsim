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

#ifndef __P16X8X_H__
#define __P16X8X_H__

#include "14bit-processors.h"
#include "intcon.h"

class P16C84 : public P16C8x
{
public:


  virtual PROCESSOR_TYPE isa(void){return _P16C84_;};

  virtual unsigned int program_memory_size(void) const { return 0x400; };

  P16C84(void);
  static Processor *construct(void);
};

class P16F84 : public P16C8x
{
public:

  virtual PROCESSOR_TYPE isa(void){return _P16F84_;};

  virtual unsigned int program_memory_size(void) const { return 0x400; };

  P16F84(void);
  static Processor *construct(void);
};

class P16CR84 : public P16F84
{
public:

  virtual PROCESSOR_TYPE isa(void){return _P16CR84_;};

  P16CR84(void) {  name_str = "p16cr84"; };
  static Processor *construct(void);
};



class P16F83 : public P16C8x
{
public:

  virtual PROCESSOR_TYPE isa(void){return _P16F83_;};

  virtual unsigned int program_memory_size(void) const { return 0x200; };

  P16F83(void);
  static Processor *construct(void);
};

class P16CR83 : public P16F83
{
public:

  virtual PROCESSOR_TYPE isa(void){return _P16CR83_;};

  P16CR83(void) {  name_str = "p16cr83"; };
  static Processor *construct(void);
};


#endif
