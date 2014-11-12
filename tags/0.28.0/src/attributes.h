/*
   Copyright (C) 1998-2005 T. Scott Dattalo

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

#if !defined(__ATTRIBUTES_H__)
#define __ATTRIBUTES_H__

#include "value.h"

class Processor;

/// gpsim attributes
///


/// WarnModeAttribute
class WarnModeAttribute : public Boolean
{
protected:
  Processor *cpu;
public:
  WarnModeAttribute(Processor *_cpu);
  virtual void set(Value *v);
  virtual void get(bool &b);
};


/// SafeModeAttribute
class SafeModeAttribute : public Boolean
{
protected:
  Processor *cpu;
public:
  SafeModeAttribute(Processor *_cpu);
  ~SafeModeAttribute();
  virtual void set(Value *v);
  virtual void get(bool &b);
};

/// UnknownModeAttribute
class UnknownModeAttribute : public Boolean
{
protected:
  Processor *cpu;
public:
  UnknownModeAttribute(Processor *_cpu);
  virtual void set(Value *v);
  virtual void get(bool &b);
};


/// BreakOnResetAttribute
class BreakOnResetAttribute : public Boolean
{
protected:
  Processor *cpu;
public:
  BreakOnResetAttribute(Processor *_cpu);
  virtual void set(Value *v);
  virtual void get(bool &b);
};
#endif //if !defined(__ATTRIBUTES_H__)

