/*
   Copyright (C) 1998-2005 T. Scott Dattalo

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

#include "attributes.h"
#include "processor.h"

//========================================================================
// Attribute wrappers
//
// Attribute wrappers are simple classes that handle specific processor
// attributes. Their primary purpose is to allow for a way to call back
// into the processor classs whenever the attribute changes.



//========================================================================
// WarnMode
//========================================================================

WarnModeAttribute::WarnModeAttribute(Processor *_cpu) :
  Boolean(false) ,cpu(_cpu)
{
  new_name("WarnMode");
  set_description(" enable warning messages when true");
}
void WarnModeAttribute::set(Value *v)
{
  Boolean::set(v);
  bool currentVal;
  get(currentVal);
  cpu->setWarnMode(currentVal);
}
void WarnModeAttribute::get(bool &b)
{
  b = cpu->getWarnMode();
  Boolean::set(&b);
}

//========================================================================
// SafeModeAttribute
//========================================================================

SafeModeAttribute::SafeModeAttribute(Processor *_cpu) :
  Boolean(false) ,cpu(_cpu)
{
  new_name("SafeMode");
  set_description(" Model the processor's specification when true. Model the actual\n"
		  " processor when false (e.g. TRIS instruction for mid range PICs\n"
		  " will emit a warning if SafeMode is true).");
}
void SafeModeAttribute::set(Value *v)
{
  Boolean::set(v);
  bool currentVal;
  get(currentVal);
  cpu->setSafeMode(currentVal);
}
void SafeModeAttribute::get(bool &b)
{
  b = cpu->getSafeMode();
  Boolean::set(b);
}

//========================================================================
// UnknownModeAttribute
//========================================================================
UnknownModeAttribute::UnknownModeAttribute(Processor *_cpu) :
    Boolean(false) ,cpu(_cpu)
{
  new_name("UnknownMode");
  set_description(" Enable three-state register logic. Unknown values are treated \n"
		  " as 0 when this is false.");

}
void UnknownModeAttribute::set(Value *v)
{
  Boolean::set(v);
  bool currentVal;
  get(currentVal);
  cpu->setUnknownMode(currentVal);
}
void UnknownModeAttribute::get(bool &b)
{
  b = cpu->getUnknownMode();
  Boolean::set(b);
}


//########################################################################
void init_attributes()
{
  
}
