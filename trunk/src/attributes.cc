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
#include "symbol.h"
#include "gpsim_time.h"
#include "protocol.h"
#include "../config.h"

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
  Boolean::set(b);
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
  Boolean::get(currentVal);
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
  Boolean::get(currentVal);
  cpu->setUnknownMode(currentVal);
}
void UnknownModeAttribute::get(bool &b)
{
  b = cpu->getUnknownMode();
  Boolean::set(b);
}

//========================================================================
// BreakOnResetAttribute
//========================================================================
BreakOnResetAttribute::BreakOnResetAttribute(Processor *_cpu) :
    Boolean(false) ,cpu(_cpu)
{
  new_name("BreakOnReset");
  set_description(" If true, halt simulation when reset occurs \n");

}
void BreakOnResetAttribute::set(Value *v)
{
  Boolean::set(v);
  bool currentVal;
  Boolean::get(currentVal);
  cpu->setBreakOnReset(currentVal);
}
void BreakOnResetAttribute::get(bool &b)
{
  b = cpu->getBreakOnReset();
  Boolean::set(b);
}

//========================================================================
//
// Cycle_Counter attribute
//
// The Cycle_counter attribute exposes the cycle counter through the 
// gpsim attribute interface. This allows for it to be queried from
// the command line or sockets.

class CycleCounterAttribute : public Integer
{
public:
  CycleCounterAttribute() :
    Integer(0) 
  {
    m_bClearableSymbol = false;
    new_name("cycles");
    set_description(" Simulation time in terms of cycles.");
  }
  void set(gint64 i)
  {
    static bool warned = false;
    if(!warned)
      cout << "cycle counter is read only\n";
    warned = true;
  }
  void get(gint64 &i)
  {
    i = cycles.get();
  }
  void get(Packet &p)
  {
    p.EncodeUInt64(cycles.get());
  }
  virtual string toString()
  {
    char buf[256];
    gint64 i;
    get(i);
    long long int j = i;
    snprintf(buf,sizeof(buf),"%" PRINTF_INT64_MODIFIER
	     "d = 0x%08" PRINTF_INT64_MODIFIER "X",j,j);
    return string(buf);
  }

};

//========================================================================
//
// GUI update rate attribute
#ifdef HAVE_GUI
class GUIUpdateRateAttribute : public Integer
{
public:
  GUIUpdateRateAttribute() :
    Integer(0) 
  {
    m_bClearableSymbol = false;
    new_name("sim.gui_update_rate");
    set_description(" Specifies the number of cycles between gui updates");
  }
  void set(gint64 i)
  {
    gi.set_update_rate(i);
  }
  void get(gint64 &i)
  {
    i = gi.get_update_rate();
  }
};
#endif


//========================================================================
//
Integer *verbosity;


//########################################################################
void init_attributes()
{
  
  // Define internal simulator attributes .
  verbosity = new Integer("sim.verbosity",1,"gpsim's verboseness 0=nothing printed 0xff=very verbose");
  verbosity->setClearableSymbol(false);
  get_symbol_table().add(verbosity);
  get_symbol_table().add(new CycleCounterAttribute());
  stop_watch.init();
#ifdef HAVE_GUI
  get_symbol_table().add(new GUIUpdateRateAttribute());
#endif

  get_symbol_table().add_constant("POR_RESET",     POR_RESET);          // Power-on reset
  get_symbol_table().add_constant("WDT_RESET",     WDT_RESET);          // Watch Dog timer timeout reset
  get_symbol_table().add_constant("EXTERNAL_RESET",EXTERNAL_RESET);     // I/O pin (e.g. MCLR going low) reset
  get_symbol_table().add_constant("SOFT_RESET",    SOFT_RESET);         // Software initiated reset
  get_symbol_table().add_constant("BOD_RESET",     BOD_RESET);          // Brown out detection reset
  get_symbol_table().add_constant("SIM_RESET",     SIM_RESET);          // Simulation Reset

}
