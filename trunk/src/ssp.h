/*
   Copyright (C) 1998,1999 T. Scott Dattalo

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

#include <iostream.h>
#include <stdio.h>


class invalid_file_register;   // Forward reference


#ifndef __SSP_H__
#define __SSP_H__

#include "pic-processor.h"
#include "14bit-registers.h"


class PIR1;
class  _14bit_processor;
class IOPORT;


class _SSPCON : public sfr_register
{
 public:

  // Register bit definitions

  enum {
    SSPM0  = 1<<0,
    SSPM1  = 1<<1,
    SSPM2  = 1<<2,
    SSPM3  = 1<<3,
    CKP    = 1<<4,
    SSPEN  = 1<<5,
    SSPOV  = 1<<6,
    WCOL   = 1<<7
  };

  virtual void put(unsigned int);
  virtual void put_value(unsigned int);
};

class _SSPSTAT : public sfr_register
{
 public: 

  // Register bit definitions

  enum {
    BF = 1<<0,  // Buffer Full
    UA = 1<<1,  // Update Address
    RW = 1<<2,  // Read/Write info
    S  = 1<<3,  // Start bit (I2C mode)
    P  = 1<<4,  // Stop bit (I2C mode)
    DA = 1<<5   // Data/Address bit (I2C mode)
  };

  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
};


class _SSPBUF : public sfr_register
{
 public: 

  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
};

class _SSPADD : public sfr_register
{
 public: 

  virtual void put(unsigned int new_value);
  virtual void put_value(unsigned int new_value);
};

class SSP_MODULE
{
 public:
  _SSPBUF   *sspbuf;
  _SSPCON   *sspcon;
  _SSPSTAT  *sspstat;
  _SSPADD   *sspadd;

  SSP_MODULE(void);
  void initialize(IOPORT *ssp_port);

};

class SSP_MODULE14 : public SSP_MODULE
{
 public:
  _14bit_processor *cpu;

  SSP_MODULE14(void);

  virtual void initialize(_14bit_processor *new_cpu);
};

#endif  // __SSP_H__
