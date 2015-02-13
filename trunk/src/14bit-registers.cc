/*
   Copyright (C) 1998-2000 Scott Dattalo
   Copyright (C) 2013	Roy R. Rankin

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


#include <stdio.h>
#include <iostream>
#include <iomanip>
#include <assert.h>


#include "../config.h"
#include "14bit-processors.h"
#include "14bit-registers.h"
#include "psp.h"     // needed for operator[] on WPU::wpu_gpio (not sure why)
#include "14bit-tmrs.h"

#include <string>
#include "stimuli.h"

#include "xref.h"
#define PCLATH_MASK              0x1f

//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("0x%06X %s() ",cycles.get(),__FUNCTION__); printf arg; }
#else
#define Dprintf(arg) {}
#endif

pic_processor *temp_cpu;
// FIXME file_register::put_value has a useful feature...

//
#if 0
//-----------------------------------------------------------
//  void file_register::put_value(unsigned int new_value)
//
//  put_value is used by the gui to change the contents of
// file registers. We could've let the gui use the normal
// 'put' member function to change the contents, however
// there are instances where 'put' has a cascading affect.
// For example, changing the value of an i/o port's tris
// could cause i/o pins to change states. In these cases,
// we'd like the gui to be notified of all of the cascaded
// changes. So rather than burden the real-time simulation
// with notifying the gui, I decided to create the 'put_value'
// function instead. 
//   Since this is a virtual function, derived classes have
// the option to override the default behavior.
//
// inputs:
//   unsigned int new_value - The new value that's to be
//                            written to this register
// returns:
//   nothing
//
//-----------------------------------------------------------

void file_register::put_value(unsigned int new_value)
{

  // go ahead and use the regular put to write the data.
  // note that this is a 'virtual' function. Consequently,
  // all objects derived from a file_register should
  // automagically be correctly updated.

  put(new_value);

  // Even though we just wrote a value to this register,
  // it's possible that the register did not get fully
  // updated (e.g. porta on many pics has only 5 valid
  // pins, so the upper three bits of a write are meaningless)
  // So we should explicitly tell the gui (if it's
  // present) to update its display.

  if(xref)
    {
      xref->update();

      if(cpu && address == cpu_pic->fsr->value)
	{
	  if(cpu_pic->indf->xref)
	    cpu_pic->indf->xref->update();
	}
    }
}

#endif

//--------------------------------------------------
// member functions for the BORCON class
// currently does not do anything
//--------------------------------------------------
//
BORCON::BORCON(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu,pName,pDesc)
{
}

void  BORCON::put(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  value.put(new_value & 0x80);



}

void  BORCON::put_value(unsigned int new_value)
{
  put(new_value&0x80);
}

//--------------------------------------------------
// member functions for the BSR class
//--------------------------------------------------
//
BSR::BSR(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu,pName,pDesc),
    register_page_bits(0)
{
}

void  BSR::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());

  value.put(new_value & 0x01f);
  //value.put(new_value & 0x01f);
  if(cpu_pic->base_isa() == _14BIT_E_PROCESSOR_)
      cpu_pic->register_bank = &cpu_pic->registers[ value.get() << 7 ];
  else
      cpu_pic->register_bank = &cpu_pic->registers[ value.get() << 8 ];


}

void  BSR::put_value(unsigned int new_value)
{
  put(new_value);

  update();
  cpu_pic->indf->update();
}

void  IOCxF::put(unsigned int new_value)
{
  unsigned int masked_value = new_value & mValidBits;
  Dprintf((" %s value %x add %x\n", name().c_str(), new_value, new_value&valid_bits));

  get_trace().raw(write_trace.get() | value.get());
  value.put(masked_value);
  if (intcon)
  {
    ((INTCON_14_PIR *)intcon)->set_rbif(masked_value != 0);
    ((INTCON_14_PIR *)intcon)->aocxf_val(this, masked_value);
  }
}

// Adjust internal RC oscillator frequency as per 12f675/629
// Spec sheet does not give range so assume +/- 12.5% as per 16f88
// The fact that base_freq is not 0. indicates the RC oscillator is being used
// and thus an adjustment should be made.
//
// This will work for any number of adjustment bits in byte but must be left justified
// and 1000000 centre frequency and 11111111 highest frequency
void  OSCCAL::put(unsigned int new_value)
{
  int   adj = new_value & mValidBits;
  float tune;
  trace.raw(write_trace.get() | value.get());
  value.put(adj);
  if (base_freq > 0.)
  {
    adj  = adj -  0x80;
    // A hook to honour configured frequency - if we're going to change it now
    if(cpu_pic->get_frequency() > base_freq*0.875 && base_freq*1.125 > cpu_pic->get_frequency()) {
      base_freq=cpu_pic->get_frequency();
      if (verbose)
        cout << "Adjusting base frequency for INTOSC calibration: " << base_freq << "\n";
    }
    tune = (1. + 0.125 * adj / 0x80) * base_freq;
    cpu_pic->set_frequency(tune);
    if (verbose)
      cout << "Calibrating INTOSC by " << adj << " to " << tune << "\n";
  }
}

void OSCCAL::set_freq(float new_base_freq)
{
  base_freq = new_base_freq;
  put(value.get());
}

void  OSCTUNE::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  value.put(new_value);
  osccon->set_rc_frequency();
}
// Clock is stable
void OSCCON::callback()
{
    value.put(value.get() | IOFS);
}
bool OSCCON::set_rc_frequency()
{
  double base_frequency = 31.e3;


  if (!cpu_pic->get_int_osc())
     return false;

    unsigned int new_IRCF = (value.get() & ( IRCF0 | IRCF1 | IRCF2)) >> 4;
    switch (new_IRCF)
    {
    case 0:
	base_frequency = 31.e3;
	break;
	
    case 1:
	base_frequency = 125e3;
	break;
	
    case 2:
	base_frequency = 250e3;
	break;
	
    case 3:
	base_frequency = 500e3;
	break;
	
    case 4:
	base_frequency = 1e6;
	break;
	
    case 5:
	base_frequency = 2e6;
	break;
	
    case 6:
	base_frequency = 4e6;
	break;
	
    case 7:
	base_frequency = 8e6;
	break;
   }
   if (osctune)
   {
       int tune;
       unsigned int osctune_value = osctune->value.get();
       tune = osctune_value & (OSCTUNE::TUN5-1);
       tune = (OSCTUNE::TUN5 & osctune_value) ? -tune : tune;
       base_frequency *= 1. + 0.125 * tune / 31.;
   }
   cpu_pic->set_frequency(base_frequency);
   if ((bool)verbose)
   {
	cout << "set_rc_frequency() : osccon=" << hex << value.get();
	if (osctune)
	    cout << " osctune=" << osctune->value.get();
	cout << " new frequency=" << base_frequency << endl;
   }
   return true;
}
void  OSCCON::put(unsigned int new_value)
{
  
  unsigned int new_IRCF = (new_value & ( IRCF0 | IRCF1 | IRCF2)) >> 4;
  unsigned int old_IRCF = (value.get() & ( IRCF0 | IRCF1 | IRCF2)) >> 4;
  trace.raw(write_trace.get() | value.get());
  value.put(new_value);

  if (set_rc_frequency())  // using internal RC Oscillator
  {
	if (old_IRCF == 0 && new_IRCF != 0) // Allow 4 ms to stabalise
	{
	    guint64 settle;

	    settle = (guint64) (get_cycles().instruction_cps() * 4e-3);
	    get_cycles().set_break(get_cycles().get() + settle, this);
	    new_value &= ~ IOFS;
	}
	else
	    new_value |= IOFS;
  }
}
// Clock is stable
void OSCCON_2::callback()
{
    unsigned int add_bits = 0;
    unsigned int val;

    if (!oscstat) return;

    val = oscstat->value.get();
    switch(mode)
    {
    case LF:
	add_bits = OSCSTAT::LFIOFR;
	break;

    case MF:
	add_bits = OSCSTAT::MFIOFR;
	break;

    case HF:
	add_bits = OSCSTAT::HFIOFL | OSCSTAT::HFIOFR  | OSCSTAT::HFIOFS;
	break;

    case PLL:
	add_bits = OSCSTAT::PLLR;
	break;

    case T1OSC:
	break;
    }
    val |= add_bits;
    oscstat->value.put(val);
}
bool OSCCON_2::set_rc_frequency()
{
  double base_frequency = 31.25e3;
  unsigned int sys_clock = value.get() & (SCS0 | SCS1);
  bool osccon_pplx4 = value.get() & SPLLEN;
  bool config_pplx4 = cpu_pic->get_pplx4_osc();



  if (sys_clock == 0 && config_fosc != 4) // Not internal oscillator
  {
	if (config_fosc >= 3) // always run at full speed
	{
  	    unsigned int oscstat_reg = (oscstat->value.get() & 0x1f);
	    oscstat->value.put(oscstat_reg | OSCSTAT::OSTS);
	    mode = EC;
	}
	else if (config_ieso) // internal/external switchover
	{
	    mode = OST;
        }
  
  }


  if((osccon_pplx4 && !config_pplx4) && sys_clock == 0)\
  {
	mode |= PLL;
	return true;
  }
  if (!cpu_pic->get_int_osc() && (sys_clock == 0))
     return false;

  if (sys_clock == 1) // T1OSC
  {
	base_frequency = 32.e3;
	mode = T1OSC;
  }
  else if (sys_clock > 1 || config_fosc == 4)
  {
    unsigned int new_IRCF = (value.get() & ( IRCF0 | IRCF1 | IRCF2 |IRCF3)) >> 3;
    switch (new_IRCF)
    {
    case 0:
    case 1:
	base_frequency = 30.e3;
	mode = LF;
	break;

    case 2:
	mode = MF;
	base_frequency = 31.25e3;
	break;
	
    case 3:
	mode = HF;
	base_frequency = 31.25e3;
	break;
	
    case 4:
	mode = MF;
	base_frequency = 62.5e3;
	break;
	
    case 5:
	mode = MF;
	base_frequency = 125e3;
	break;
	
    case 6:
	mode = MF;
	base_frequency = 250e3;
	break;
	
    case 7:
	mode = MF;
	base_frequency = 500e3;
	break;
	
    case 8:
	mode = HF;
	base_frequency = 125e3;
	break;
	
    case 9:
	mode = HF;
	base_frequency = 250e3;
	break;
	
    case 10:
	mode = HF;
	base_frequency = 500e3;
	break;
	
    case 11:
	mode = HF;
	base_frequency = 1e6;
	break;
	
    case 12:
	mode = HF;
	base_frequency = 2e6;
	break;
	
    case 13:
	mode = HF;
	base_frequency = 4e6;
	break;
	
    case 14:
	// The treatment for PPL based on Fig 5-1 of P12f1822 ref manual
	if (osccon_pplx4 || config_pplx4)
	{
	   mode = PLL;
	   base_frequency = 32e6;
	}
	else
	{
	   mode = HF;
	   base_frequency = 8e6;
	}
	break;
	
    case 15:
	mode = HF;
	base_frequency = 16e6;
	break;
   }
   if (osctune)
   {
       int tune;
       unsigned int osctune_value = osctune->value.get();
       tune = osctune_value & (OSCTUNE::TUN5-1);
       tune = (OSCTUNE::TUN5 & osctune_value) ? -tune : tune;
       base_frequency *= 1. + 0.125 * tune / 31.;
   }
  }
   cpu_pic->set_frequency(base_frequency);
   if ((bool)verbose)
   {
	cout << "set_rc_frequency() : osccon=" << hex << value.get();
	if (osctune)
	    cout << " osctune=" << osctune->value.get();
	cout << " new frequency=" << base_frequency << endl;
   }
   return true;
}
void  OSCCON_2::put_value(unsigned int new_value)
{
	value.put(new_value);
}
void  OSCCON_2::put(unsigned int new_value)
{
  
  unsigned int old_value = value.get();
  unsigned int oscstat_reg = 0;
  unsigned int oscstat_new = 0;
  trace.raw(write_trace.get() | value.get());
  value.put(new_value);

  if (old_value == new_value) return;

  assert(oscstat);
  
  oscstat_reg = oscstat->value.get();
  oscstat_new = oscstat_reg;
	if (((new_value & (SCS0 | SCS1))==0) && !cpu_pic->get_int_osc())
		oscstat_new |= OSCSTAT::OSTS;
	else
		oscstat_new &= ~OSCSTAT::OSTS;

  

  if (set_rc_frequency())  // using internal RC Oscillator
	set_callback();
}
void OSCCON_2::set_config(unsigned int cfg_fosc, bool cfg_ieso)
{
    config_fosc = cfg_fosc;
    config_ieso = cfg_ieso;

}
void OSCCON_2::wake()
{
}
void OSCCON_2::set_callback()
{
	unsigned int oscstat_reg = oscstat->value.get();;
	unsigned int oscstat_new = oscstat_reg;
	guint64 settle = 0;

	switch(mode&~PLL)
	{
	case LF:
		oscstat_new &= ~(OSCSTAT::OSTS | OSCSTAT::PLLR | OSCSTAT::T1OSCR);
		settle = get_cycles().get() + 2;
		break;

	case MF:
		oscstat_new &= ~(OSCSTAT::OSTS | OSCSTAT::PLLR | OSCSTAT::T1OSCR);
		settle = get_cycles().get(2e-6); // 2us settle time
		break;

	case HF:
		oscstat_new &= ~(OSCSTAT::OSTS | OSCSTAT::PLLR | OSCSTAT::T1OSCR);
		settle = get_cycles().get(2e-6); // 2us settle time
		break;

	case T1OSC:
		settle = get_cycles().get() + 1024/4;
		break;
	}
  	if((mode & PLL) && (oscstat_reg & OSCSTAT::PLLR) == 0)
		settle = get_cycles().get(2e-3); // 2ms

	if (settle)
	{
		if (next_callback > get_cycles().get())
		    get_cycles().clear_break(next_callback);

	        get_cycles().set_break(settle, this);
		next_callback = settle;
	}
	if(oscstat && (oscstat_new != oscstat_reg))
		oscstat->put(oscstat_new);
}

void WDTCON::put(unsigned int new_value)
{
  unsigned int masked_value = new_value & valid_bits;

  trace.raw(write_trace.get() | value.get());

  value.put(masked_value);

  if (valid_bits > 1)
      cpu_pic->wdt.set_prescale(masked_value >> 1);
  if (cpu_pic->swdten_active())
      cpu_pic->wdt.swdten((masked_value & SWDTEN) == SWDTEN);
}
void WDTCON::reset(RESET_TYPE r)
{
   putRV(por_value);
}
//
//--------------------------------------------------
// member functions for the FSR class
//--------------------------------------------------
//
FSR::FSR(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc)
{}

void  FSR::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());

  value.put(new_value);
}

void  FSR::put_value(unsigned int new_value)
{
  put(new_value);

  update();
  cpu_pic->indf->update();
}


unsigned int FSR::get()
{
  trace.raw(read_trace.get() | value.get());
  return(value.get());
}

unsigned int FSR::get_value()
{
  return(value.get());
}


//
//--------------------------------------------------
// member functions for the FSR_12 class
//--------------------------------------------------
//
FSR_12::FSR_12(Processor *pCpu, const char *pName, unsigned int _rpb, unsigned int _valid_bits)
  : FSR(pCpu, pName, ""),
    valid_bits(_valid_bits),
    register_page_bits(_rpb)
{}

void  FSR_12::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  value.put(new_value);

  /* The 12-bit core selects the register page using the fsr */
  cpu_pic->register_bank = &cpu_pic->registers[ value.get() & register_page_bits ];
}

void  FSR_12::put_value(unsigned int new_value)
{

  put(new_value);

  update();
  cpu_pic->indf->update();

}


unsigned int FSR_12::get()
{
  unsigned int v = get_value();
  trace.raw(read_trace.get() | value.get());
  return(v);
}

unsigned int FSR_12::get_value()
{
  // adjust for missing bits
  //cout << "FSR_12:get_value - valid_bits 0x" << hex << valid_bits << endl;
  return ((value.get() & valid_bits) | (~valid_bits & 0xff));

}

//
//--------------------------------------------------
// member functions for the Status_register class
//--------------------------------------------------
//

//--------------------------------------------------

Status_register::Status_register(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc)
{
  rcon = NULL;
  break_point = 0;
  break_on_z =0;
  break_on_c =0;
  address = 3;
  rp_mask = RP_MASK;
  write_mask = 0xff & ~STATUS_TO & ~STATUS_PD;
  new_name("status");
}

//--------------------------------------------------
void Status_register::reset(RESET_TYPE r)
{
  switch (r) {

  case POR_RESET:
    putRV(por_value);
    put_TO(1);
    put_PD(1);
    break;

  case WDT_RESET:
    put_TO(0);
    break;

  default:
    break;
  }

}

//--------------------------------------------------
// put

void Status_register::put(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());

  value.put((value.get() & ~write_mask) | (new_value & write_mask));

  if(cpu_pic->base_isa() == _14BIT_PROCESSOR_)
    {
      cpu_pic->register_bank = &cpu_pic->registers[(value.get() & rp_mask) << 2];
    }

}


//--------------------------------------------------
// get
//unsigned int Status_register::get()

//--------------------------------------------------
// put_Z

//void Status_register::put_Z(unsigned int new_z)

//--------------------------------------------------
// get_Z
//unsigned int Status_register::get_Z()
//--------------------------------------------------
// put_C

//void Status_register::put_C(unsigned int new_c)

//--------------------------------------------------
// get_C
//unsigned int Status_register::get_C()

//--------------------------------------------------
// put_Z_C_DC

//--------------------------------------------------
// member functions for the INDF class
//--------------------------------------------------
INDF::INDF(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc)
{
  fsr_mask = 0x7f;           // assume a 14bit core
  base_address_mask1 = 0;    //   "          "
  base_address_mask2 = 0xff; //   "          "
}

void INDF::initialize()
{

  switch(cpu_pic->base_isa()) {

  case _12BIT_PROCESSOR_:
    fsr_mask = 0x1f;
    base_address_mask1 = 0x0;
    base_address_mask2 = 0x1f;

    break;

  case _14BIT_PROCESSOR_:
    fsr_mask = 0x7f;
    break;

  case _PIC17_PROCESSOR_:
  case _PIC18_PROCESSOR_:
    cout << "BUG: INDF::"<<__FUNCTION__<<". 16bit core uses a different class for indf.";
    break;
  default:
    cout << " BUG - invalid processor type INDF::initialize\n";
  }
    

}
void INDF::put(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());
  int reg = (cpu_pic->fsr->get_value() + //cpu_pic->fsr->value + 
	     ((cpu_pic->status->value.get() & base_address_mask1)<<1) ) &  base_address_mask2;

  // if the fsr is 0x00 or 0x80, then it points to the indf
  if(reg & fsr_mask){
    cpu_pic->registers[reg]->put(new_value);

    //(cpu_pic->fsr->value & base_address_mask2) + ((cpu_pic->status->value & base_address_mask1)<<1)
  }

}

void INDF::put_value(unsigned int new_value)
{

  // go ahead and use the regular put to write the data.
  // note that this is a 'virtual' function. Consequently,
  // all objects derived from a file_register should
  // automagically be correctly updated (which isn't
  // necessarily true if we just write new_value on top
  // of the current register value).

  put(new_value);

  update();
  int r = (cpu_pic->fsr->get_value() + //cpu_pic->fsr->value + 
	   (((cpu_pic->status->value.get() & base_address_mask1)<<1)& base_address_mask2));
  if(r & fsr_mask) 
    cpu_pic->registers[r]->update();

}


unsigned int INDF::get()
{

  trace.raw(read_trace.get() | value.get());
  //trace.register_read(address,value.get());
  int reg = (cpu_pic->fsr->get_value() +
	     ((cpu_pic->status->value.get() & base_address_mask1)<<1) ) &  base_address_mask2;
  if(reg & fsr_mask)
    return(cpu_pic->registers[reg]->get());
  else
    return(0);   // avoid infinite loop if fsr points to the indf
}

unsigned int INDF::get_value()
{
  int reg = (cpu_pic->fsr->get_value() +
	       ((cpu_pic->status->value.get() & base_address_mask1)<<1) ) &  base_address_mask2;
  if(reg & fsr_mask)
    return(cpu_pic->registers[reg]->get_value());
  else
    return(0);   // avoid infinite loop if fsr points to the indf
}



//--------------------------------------------------
// member functions for the PCL base class
//--------------------------------------------------
PCL::PCL(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc)
{
  por_value = RegisterValue(0,0);
}

// %%% FIX ME %%% breaks are different
void PCL::put(unsigned int new_value)
{

  trace.raw(write_trace.get() | value.get());
  cpu_pic->pc->computed_goto(new_value);
  //trace.register_write(address,value.get());
}

void PCL::put_value(unsigned int new_value)
{

  value.put(new_value & 0xff);
  cpu_pic->pc->put_value( (cpu_pic->pc->get_value() & 0xffffff00) | value.get());

  // The gui (if present) will be updated in the pc->put_value call.
}

unsigned int PCL::get()
{
  return((value.get()+1) & 0xff);
}

unsigned int PCL::get_value()
{
  value.put(cpu_pic->pc->get_value() & 0xff);
  return(value.get());
}
//------------------------------------------------------------
// PCL reset
// 
void PCL::reset(RESET_TYPE r)
{
  trace.raw(write_trace.get() | value.get());
  putRV_notrace(por_value);
}

//--------------------------------------------------
// member functions for the PCLATH base class
//--------------------------------------------------

PCLATH::PCLATH(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc)
{
    mValidBits = PCLATH_MASK;
}

void PCLATH::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());
  value.put(new_value & mValidBits);
}

void PCLATH::put_value(unsigned int new_value)
{
  cout << "PCLATH::put_value(" << new_value << ")\n";
  value.put(new_value & mValidBits);

  // RP - I cannot think of a single possible reason I'd want to affect the real PC here!
  //  cpu_pic->pc->put_value( (cpu_pic->pc->get_value() & 0xffff00ff) | (value.get()<<8) );

  // The gui (if present) will be updated in the pc->put_value call.
}

unsigned int PCLATH::get()
{
  trace.raw(read_trace.get() | value.get());
  //trace.register_read(address,value.get());
  return(value.get() & mValidBits);
}

//--------------------------------------------------
// member functions for the PCON base class
//--------------------------------------------------
//
PCON::PCON(Processor *pCpu, const char *pName, const char *pDesc, 
	unsigned int bitMask)
  : sfr_register(pCpu, pName, pDesc)
{
  valid_bits = bitMask;
}

void PCON::put(unsigned int new_value)
{
  Dprintf((" value %x add %x\n", new_value, new_value&valid_bits));
  trace.raw(write_trace.get() | value.get());
  //trace.register_write(address,value.get());
  value.put(new_value&valid_bits);
}


//------------------------------------------------

Indirect_Addressing14::Indirect_Addressing14(pic_processor *pCpu, const string &n)
  : fsrl(pCpu, (string("fsrl")+n).c_str(), "FSR Low", this),
    fsrh(pCpu, (string("fsrh")+n).c_str(), "FSR High", this),
    indf(pCpu, (string("indf")+n).c_str(), "Indirect Register", this)
{
  current_cycle = (guint64)(-1);   // Not zero! See bug #3311944
  fsr_value = 0;
  fsr_state = 0;
  fsr_delta = 0;
  cpu = pCpu;

}

/*
 * put - Each of the indirect registers associated with this
 * indirect addressing class will call this routine to indirectly
 * write data.
 */
void Indirect_Addressing14::put(unsigned int new_value)
{
    unsigned int fsr_adj = fsr_value + fsr_delta;

    if (fsr_adj < 0x1000) 	// Traditional Data Memory
    {
	if(is_indirect_register(fsr_adj))
	    return;
	cpu_pic->registers[fsr_adj]->put(new_value);
    }
    else if (fsr_adj >= 0x2000 && fsr_adj < 0x29b0) // Linear GPR region
    {
	unsigned int bank = (fsr_adj & 0xfff) / 0x50;
	unsigned int low_bits = ((fsr_adj & 0xfff) % 0x50) + 0x20;
        Dprintf(("fsr_adj %x bank %x low_bits %x add %x\n", fsr_adj, bank, low_bits, (bank*0x80 + low_bits)));
        cpu_pic->registers[bank * 0x80 + low_bits]->put(new_value); 
    }
    else if (fsr_adj >= 0x8000 && fsr_adj <= 0xffff) // program memory
    {
	cout << "WARNING cannot write via FSR/INDF to program memory address 0x"
		<<hex << fsr_adj << endl;
	return;	// Not writable
    }

}

/*
 * get - Each of the indirect registers associated with this
 * indirect addressing class will call this routine to indirectly
 * retrieve data.
 */
unsigned int Indirect_Addressing14::get()
{
    unsigned int fsr_adj = fsr_value + fsr_delta;

    if (fsr_adj < 0x1000) // Traditional Data Memory
    {
	if(is_indirect_register(fsr_adj))
	    return 0;
	return cpu_pic->registers[fsr_adj]->get();
    }
    else if (fsr_adj >= 0x2000 && fsr_adj < 0x29b0) // Linear GPR region
    {
	unsigned int bank = (fsr_adj & 0xfff) / 0x50;
	unsigned int low_bits = ((fsr_adj & 0xfff) % 0x50) + 0x20;
        return(cpu_pic->registers[bank * 0x80 + low_bits]->get()); 
    }
    else if (fsr_adj >= 0x8000 && fsr_adj <= 0xffff) // program memory
    {
	unsigned int pm;
	unsigned address = fsr_adj - 0x8000;

  	if (address <= cpu_pic->program_memory_size())
        {
	  pm = cpu_pic->get_program_memory_at_address(address);
          Dprintf((" address %x max %x value %x\n",address, cpu_pic->program_memory_size(), pm));
	    return pm & 0xff;
        }
    }
    return 0;
}

/*
 * get - Each of the indirect registers associated with this
 * indirect addressing class will call this routine to indirectly
 * retrieve data.
 */
unsigned int Indirect_Addressing14::get_value()
{
    unsigned int fsr_adj = fsr_value + fsr_delta;
    if (fsr_adj < 0x1000)	// Traditional Data Memory
    {
	if(is_indirect_register(fsr_adj))
	    return 0;
	
	return cpu_pic->registers[fsr_adj]->get_value();
    }
    else if (fsr_adj >= 0x2000 && fsr_adj < 0x29b0) // Linear GPR region
    {
	unsigned int bank = (fsr_adj & 0xfff) / 0x50;
	unsigned int low_bits = ((fsr_adj & 0xfff) % 0x50) + 0x20;

        return(cpu_pic->registers[bank * 0x80 + low_bits]->get_value()); 
    }
    else if (fsr_adj >= 0x8000 && fsr_adj <= 0xffff) // program memory
    {
	unsigned int pm;
	unsigned address = fsr_adj - 0x8000;

  	if (address <= cpu_pic->program_memory_size())
        {
	  pm = cpu_pic->get_program_memory_at_address(address);
	    return pm & 0xff;
        }
    }
    return 0;
}

void Indirect_Addressing14::put_fsr(unsigned int new_fsr)
{

  fsrl.put(new_fsr & 0xff);
  fsrh.put((new_fsr>>8) & 0xff);

}


/*
 * update_fsr_value - This routine is called by the FSRL and FSRH
 * classes. It's purpose is to update the 16-bit 
 * address formed by the concatenation of FSRL and FSRH.
 *
 */

void Indirect_Addressing14::update_fsr_value()
{

  if(current_cycle != get_cycles().get())
    {
      fsr_value = (fsrh.value.get() << 8) |  fsrl.value.get();
      fsr_delta = 0;
    }
}
//--------------------------------------------------
// member functions for the FSR class
//--------------------------------------------------
//
FSRL14::FSRL14(Processor *pCpu, const char *pName, const char *pDesc, Indirect_Addressing14 *pIAM)
  : sfr_register(pCpu,pName,pDesc),
    iam(pIAM)
{
}

void  FSRL14::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  value.put(new_value & 0xff);
  iam->fsr_delta = 0;
  iam->update_fsr_value();
}

void  FSRL14::put_value(unsigned int new_value)
{

  value.put(new_value & 0xff);
  iam->fsr_delta = 0;
  iam->update_fsr_value();

  update();
  cpu14->indf->update();

}

FSRH14::FSRH14(Processor *pCpu, const char *pName, const char *pDesc, Indirect_Addressing14 *pIAM)
  : sfr_register(pCpu,pName,pDesc),
    iam(pIAM)
{
}

void  FSRH14::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  value.put(new_value & 0xff);

  iam->update_fsr_value();

}

void  FSRH14::put_value(unsigned int new_value)
{

  value.put(new_value & 0xff);
  iam->update_fsr_value();

  update();
  cpu14->indf->update();
}

// INDF14 used by 14bit enhanced indirect addressing
INDF14::INDF14(Processor *pCpu, const char *pName, const char *pDesc, Indirect_Addressing14 *pIAM)
  : sfr_register(pCpu,pName,pDesc),
    iam(pIAM)
{
}

void INDF14::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  if(iam->fsr_value & 0x8000) // extra cycle for program memory access
      get_cycles().increment();

  iam->put(new_value);
  iam->fsr_delta = 0;
}

void INDF14::put_value(unsigned int new_value)
{
  iam->put(new_value);
  iam->fsr_delta = 0;
  update();
}

unsigned int INDF14::get()
{
  unsigned int ret;

  Dprintf((" get val %x delta %x \n", iam->fsr_value, iam->fsr_delta));
  trace.raw(read_trace.get() | value.get());
  if(iam->fsr_value & 0x8000)
      get_cycles().increment();

  ret = iam->get();
  iam->fsr_delta = 0;
  return ret;
}

unsigned int INDF14::get_value()
{
  return(iam->get_value());
}
//--------------------------------------------------
Stack::Stack(Processor *pCpu) : cpu(pCpu)
{

  stack_warnings_flag = 0;   // Do not display over/under flow stack warnings
  break_on_overflow = 0;     // Do not break if the stack over flows
  break_on_underflow = 0;    // Do not break if the stack under flows
  stack_mask = 7;            // Assume a 14 bit core.
  pointer = 0;

  for(int i=0; i<31; i++)
    contents[i] = 0;

  STVREN = 0;

}

//
// Stack::push
//
// push the passed address onto the stack by storing it at the current
// 

bool Stack::push(unsigned int address)
{
  Dprintf(("pointer=%d address0x%x\n",pointer,address));
  // Write the address at the current point location. Note that the '& stack_mask'
  // implicitly handles the stack wrap around.

  
  contents[pointer & stack_mask] = address;

  // If the stack pointer is too big, then the stack has definitely over flowed.
  // However, some pic programs take advantage of this 'feature', so provide a means
  // for them to ignore the warnings.

  if(pointer++ >= (int)stack_mask) {
	stack_overflow();
	return false;
  }
  return true;

}
bool Stack::stack_overflow()
{
    if(stack_warnings_flag || break_on_overflow)
      cout << "stack overflow ";
    if(break_on_overflow)
      bp.halt();
    return true;
}

//
// Stack::pop
//

unsigned int Stack::pop()
{

  // First decrement the stack pointer.

  if(--pointer < 0)  {
	stack_underflow();
	return 0;
  }


  Dprintf(("pointer=%d address0x%x\n",pointer,contents[pointer & stack_mask]));

  return(contents[pointer & stack_mask]);
}
bool Stack::stack_underflow()
{
    pointer = 0;
    if(stack_warnings_flag || break_on_underflow)
      cout << "stack underflow ";
    if(break_on_underflow) 
      bp.halt();
    return true;
}

//
//  bool Stack::set_break_on_overflow(bool clear_or_set)
//
//  Set or clear the break on overflow flag


bool Stack::set_break_on_overflow(bool clear_or_set)
{
  if(break_on_overflow == clear_or_set)
    return 0;

  break_on_overflow = clear_or_set;

  return 1;

}

//
//  bool Stack::set_break_on_underflow(bool clear_or_set)
//
//  Set or clear the break on underflow flag


bool Stack::set_break_on_underflow(bool clear_or_set)
{
  if(break_on_underflow == clear_or_set)
    return 0;

  break_on_underflow = clear_or_set;

  return 1;

}

// Read value at top of stack
//
unsigned int Stack::get_tos()
{
  if (pointer > 0)
    return (contents[pointer-1]);
  else
    return (0);
}

// Modify value at top of stack;
//
void Stack::put_tos(unsigned int new_tos)
{

  if (pointer > 0)
      contents[pointer-1] = new_tos;

}

// Stack14E for extended 14bit processors
// This stack implementation differs from both the other 14bit
// and 16bit stacks as a dummy empty location is used so a
// stack with 16 slots can hold 16 values. The other implementaion
// of the stack hold n-1 values for an n slot stack.
// This stack also supports stkptr, tosl, and tosh like the 16bit
// (p18) processors 
Stack14E::Stack14E(Processor *pCpu) : Stack(pCpu),
    stkptr(pCpu, "stkptr", "Stack pointer"),
    tosl(pCpu, "tosl", "Top of Stack low byte"),
    tosh(pCpu, "tosh", "Top of Stack high byte")
{
  stkptr.stack = this;
  tosl.stack = this;
  tosh.stack = this;


  STVREN = 1;
}

Stack14E::~Stack14E()
{
  pic_processor *pCpu = dynamic_cast<pic_processor *>(cpu);
  if (pCpu) 
  {
    pCpu->remove_sfr_register(&stkptr);
    pCpu->remove_sfr_register(&tosl);
    pCpu->remove_sfr_register(&tosh);
  }
}
void Stack14E::reset(RESET_TYPE r)
{
    pointer = NO_ENTRY;
    if (STVREN)
	contents[stack_mask] = 0;
    else
	contents[pointer-1] = contents[stack_mask];
  
    Dprintf((" pointer %x\n", pointer));
    stkptr.put(pointer-1);
}

bool Stack14E::push(unsigned int address)
{
  Dprintf(("pointer=%d address 0x%x\n",pointer,address));
  // Write the address at the current point location. Note that the '& stack_mask'
  // implicitly handles the stack wrap around.

  if(pointer == NO_ENTRY)
	pointer = 0;
  
  contents[pointer & stack_mask] = address;

  // If the stack pointer is too big, then the stack has definitely over flowed.
  // However, some pic programs take advantage of this 'feature', so provide a means
  // for them to ignore the warnings.

  if(pointer++ > (int)stack_mask) {
	return stack_overflow();
  }
  stkptr.put(pointer-1);
  return true;

}
unsigned int  Stack14E::pop()
{
    unsigned int ret = 0;

    if (pointer == NO_ENTRY)
    {
	return stack_underflow();
    }
    pointer--;
    ret = contents[pointer];
    if (pointer <= 0)
	pointer = NO_ENTRY;
    
    stkptr.put(pointer-1);
    return(ret);
}
bool Stack14E::stack_overflow()
{
    cpu14e->pcon.put(cpu14e->pcon.get() | PCON::STKOVF);
    if(STVREN) 
    {
	cpu->reset(STKOVF_RESET);
	return false;
    }
    else
    {
	cout << "Stack overflow\n";
    }
    return true;
}
bool Stack14E::stack_underflow()
{
    Dprintf((" cpu %p STVREN %d\n", cpu, STVREN));
    cpu14e->pcon.put(cpu14e->pcon.get() | PCON::STKUNF);
    if(STVREN) 
    {
	cpu->reset(STKUNF_RESET);
    	return false;
    }
    else
    {
	cout << "Stack underflow\n";
    }
    return true;

}

//------------------------------------------------
// TOSL
TOSL::TOSL(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu,pName,pDesc)
{}

unsigned int TOSL::get()
{
  value.put(stack->get_tos() & 0xff);
  trace.raw(read_trace.get() | value.get());
  return(value.get());
}

unsigned int TOSL::get_value()
{
  value.put(stack->get_tos() & 0xff);
  return(value.get());
}

void TOSL::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  stack->put_tos( (stack->get_tos() & 0xffffff00) | (new_value & 0xff));
  value.put(new_value & 0xff);
}

void TOSL::put_value(unsigned int new_value)
{
  stack->put_tos( (stack->get_tos() & 0xffffff00) | (new_value & 0xff));
  value.put(new_value & 0xff);
  update();
}


//------------------------------------------------
// TOSH
TOSH::TOSH(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu,pName,pDesc)
{}

unsigned int TOSH::get()
{
  value.put((stack->get_tos() >> 8) & 0xff);
  trace.raw(read_trace.get() | value.get());
  return(value.get());

}

unsigned int TOSH::get_value()
{

  value.put((stack->get_tos() >> 8) & 0xff);
  return(value.get());

}

void TOSH::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  stack->put_tos( (stack->get_tos() & 0xffff00ff) | ( (new_value & 0xff) << 8));
  value.put(new_value & 0xff);
}

void TOSH::put_value(unsigned int new_value)
{

  stack->put_tos( (stack->get_tos() & 0xffff00ff) | ( (new_value & 0xff) << 8));
  value.put(new_value & 0xff);

  update();

}
STKPTR::STKPTR(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu,pName,pDesc)
{}
void STKPTR::put_value(unsigned int new_value)
{
  stack->pointer = (new_value & 0x1f) + 1;
  value.put(new_value);
  update();
}

void STKPTR::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  put_value(new_value);
}


//========================================================================
class WReadTraceObject : public RegisterReadTraceObject
{
public:
  WReadTraceObject(Processor *_cpu, RegisterValue trv);
  virtual void print(FILE *fp);
};

class WWriteTraceObject : public RegisterWriteTraceObject
{
public:
  WWriteTraceObject(Processor *_cpu, RegisterValue trv);
  virtual void print(FILE *fp);
};


class WTraceType : public ProcessorTraceType
{
public:
  WTraceType(Processor *_cpu, 
	     unsigned int s)
    : ProcessorTraceType(_cpu,s,"W reg")
  {}

  TraceObject *decode(unsigned int tbi);
};



//========================================================================
WWriteTraceObject::WWriteTraceObject(Processor *_cpu, RegisterValue trv) 
  : RegisterWriteTraceObject(_cpu,0,trv)
{
  pic_processor *pcpu = dynamic_cast<pic_processor *>(cpu);

  if(pcpu) {
    to = cpu_pic->Wreg->trace_state;
    cpu_pic->Wreg->trace_state = from;
  }

}

void WWriteTraceObject::print(FILE *fp)
{
  char sFrom[16];
  char sTo[16];

  fprintf(fp, "  Wrote: 0x%s to W was 0x%s\n",
	  to.toString(sTo,sizeof(sTo)),
	  from.toString(sFrom,sizeof(sFrom)));

}

//========================================================================
WReadTraceObject::WReadTraceObject(Processor *_cpu, RegisterValue trv) 
  : RegisterReadTraceObject(_cpu,0,trv)
{
  pic_processor *pcpu = dynamic_cast<pic_processor *>(cpu);

  if(pcpu) {
    to = cpu_pic->Wreg->trace_state;
    cpu_pic->Wreg->trace_state = from;
  }

}

void WReadTraceObject::print(FILE *fp)
{
  char sFrom[16];

  fprintf(fp, "  Read: 0x%s from W\n",
	  from.toString(sFrom,sizeof(sFrom)));

}

//========================================================================
TraceObject * WTraceType::decode(unsigned int tbi)
{

  unsigned int tv = trace.get(tbi);

  RegisterValue rv = RegisterValue(tv & 0xff,0);
  TraceObject *wto;

  if (tv & (1<<22)) 
    wto = new WReadTraceObject(cpu, rv);
  else
    wto = new WWriteTraceObject(cpu, rv);

  return wto;
}


WREG::WREG(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu,pName,pDesc)
{
  if(cpu) {
    unsigned int trace_command = trace.allocateTraceType(m_tt = new WTraceType(get_cpu(),1));
    RegisterValue rv(trace_command+(0<<22), trace_command+(2<<22));
    set_write_trace(rv);
    rv = RegisterValue(trace_command+(1<<22), trace_command+(3<<22));
    set_read_trace (rv);
  }

}

WREG::~WREG()
{
  delete m_tt;
}


void WPU::put(unsigned int new_value)
{
    unsigned int masked_value = new_value & mValidBits;
    int i;

    trace.raw(write_trace.get() | value.get());

    value.put(masked_value);
    for(i = 0; i < 8; i++)
    {
	if((1<<i) & mValidBits)
	{
	    (&(*wpu_gpio)[i])->getPin().update_pullup((((1<<i) & masked_value) && wpu_pu )? '1' : '0', true);
	}
    }
}
/*
 * enable/disable all WPU  pullups
 */
void WPU::set_wpu_pu(bool pullup_enable)
{
    if (pullup_enable != wpu_pu)
    {
	wpu_pu = pullup_enable;
        put(value.get());	// change pull-ups based on value of WPU and gpio_pu
    }
}

CPSCON0::CPSCON0(Processor *pCpu, const char *pName, const char *pDesc)
    : sfr_register(pCpu, pName, pDesc), m_tmr0(0), chan(0),
        future_cycle(0), cps_stimulus(0)
{
      mValidBits = 0xcf;
      for(int i =0; i < 16; i++)
        pin[i] = 0;
}

CPSCON0::~CPSCON0()
{
    if (cps_stimulus)
	delete cps_stimulus;
}

void CPSCON0::put(unsigned int new_value)
{
    unsigned int masked_value = (new_value & mValidBits) & ~CPSOUT;
    unsigned int diff = masked_value ^ value.get();

    trace.raw(write_trace.get() | value.get());

    value.put(masked_value);
    if (diff & T0XCS)
	m_tmr0->set_t0xcs(masked_value & T0XCS);
    calculate_freq();
}

#define p_cpu ((Processor *)cpu)

void CPSCON0::calculate_freq()
{

    if (!(value.get() & CPSON)) // not active, return
	return;

    if (!pin[chan] || !pin[chan]->getPin().snode)
    {
	return;
    }

    double cap = pin[chan]->getPin().snode->Cth;
    double current = 0;
    double deltat;

    switch((value.get() & (CPSRNG0 | CPSRNG1)) >> 2)
    {
    case 1:
	current = (value.get() & CPSRM) ? 9e-6 : 0.1e-6;
	break;

    case 2:
	current = (value.get() & CPSRM) ? 30e-6 : 1.2e-6;
	break;

    case 3:
	current = (value.get() & CPSRM) ? 100e-6 : 18e-6;
	break;
    };

    if (current < 1e-12)
	return;
    // deltat is the time required to charge the capacitance on the pin
    // from a constant current source for the specified voltage swing.
    // The voltage swing for the internal reference is not specified
    // and it is just a guess that it is Vdd - 2 diode drops.
    //
    // This implimentation does not work if capacitor oscillator
    // runs faster than Fosc/4
    //
    if (value.get() & CPSRM)
    {
        deltat = (FVR_voltage - DAC_voltage)*cap/current;
	if (deltat <= 0.)
	{
	    cout << "CPSCON FVR must be greater than DAC for high range to work\n";
	    return;
	}
    }
    else
    {
	deltat = (p_cpu->get_Vdd() - 1.2) * cap / current;
    }
 
    period = (p_cpu->get_frequency() * deltat + 2) / 4;

    if (period <= 0)
    {
	cout << "CPSCON Oscillator > Fosc/4, setting to Fosc/4\n";
	period = 1;
    }
    guint64 fc = get_cycles().get() + period;
    if (future_cycle > get_cycles().get())
    {
	get_cycles().reassign_break(future_cycle, fc, this);
    }
    else
        get_cycles().set_break(fc, this);

    future_cycle = fc;
}

void CPSCON0::callback()
{
    Dprintf(("now=0x%"PRINTF_GINT64_MODIFIER"x\n",get_cycles().get()));

    if (!(value.get() & CPSON))
	return;

    if (value.get() & CPSOUT) // High to low transition
    {
	value.put(value.get() & ~CPSOUT);
	if (m_tmr0 && (value.get() & T0XCS) && 
	    m_tmr0->get_t0se() && m_tmr0->get_t0cs())
	{
		m_tmr0->increment();
	}
		
    }
    else			// Low to high transition
    {
	value.put(value.get() | CPSOUT);
	if (m_tmr0 && (value.get() & T0XCS) && 
	    !m_tmr0->get_t0se() && m_tmr0->get_t0cs())
	{
		m_tmr0->increment();
	}
	if (m_t1con_g)
	    m_t1con_g->t1_cap_increment();
    }


    calculate_freq();
}

void CPSCON0::set_chan(unsigned int _chan)
{
    if (_chan == chan)
	return;

    if (!pin[_chan])
    {
	cout << "CPSCON Channel " << _chan << " reserved\n";
	return;
    }
    if (!pin[_chan]->getPin().snode)
    {
	cout << "CPSCON Channel " << pin[_chan]->getPin().name() << " requires a node attached\n";
	chan = _chan;
	return;
    }
    if (!cps_stimulus)
	cps_stimulus = new CPS_stimulus(this, "cps_stimulus");
    else if (pin[_chan]->getPin().snode)
    {
	(pin[_chan]->getPin().snode)->detach_stimulus(cps_stimulus);
    }

    chan = _chan;
    (pin[_chan]->getPin().snode)->attach_stimulus(cps_stimulus);
    calculate_freq();
}

void CPSCON0::set_DAC_volt(double volt)
{
    DAC_voltage = volt;
    if ((value.get() & (CPSON|CPSRM)) == (CPSON|CPSRM))
	calculate_freq();
}
void CPSCON0::set_FVR_volt(double volt)
{
    FVR_voltage = volt;
    if ((value.get() & (CPSON|CPSRM)) == (CPSON|CPSRM))
	calculate_freq();
}

CPS_stimulus::CPS_stimulus(CPSCON0 * arg, const char *cPname,double _Vth, double _Zth)
  : stimulus(cPname, _Vth, _Zth)
{
    m_cpscon0 = arg;
}

// Thisvis also called when the capacitance chages,
// not just when the voltage changes
void   CPS_stimulus::set_nodeVoltage(double v)
{
	Dprintf(("set_nodeVoltage =%.1f\n", v));
 	nodeVoltage = v;;
	m_cpscon0->calculate_freq();
}
	
void CPSCON1::put(unsigned int new_value)
{
    unsigned int masked_value = new_value & mValidBits;

    trace.raw(write_trace.get() | value.get());

    value.put(masked_value);
    assert(m_cpscon0);
    m_cpscon0->set_chan(masked_value);
}

SRCON0::SRCON0(Processor *pCpu, const char *pName, const char *pDesc, SR_MODULE *_sr_module)
    : sfr_register(pCpu, pName, pDesc),
	m_sr_module(_sr_module)
{
}
void SRCON0::put(unsigned int new_value)
{
    unsigned int diff = new_value ^ value.get();

    if (!diff)
	return;

    trace.raw(write_trace.get() | value.get());

    value.put(new_value  & ~(SRPR|SRPS)); // SRPR AND SRPS not saved

    if ((diff & SRPS) && (new_value & SRPS))
	m_sr_module->pulse_set();
    if ((diff & SRPR) && (new_value & SRPR))
	m_sr_module->pulse_reset();

    if (diff & CLKMASK)
	m_sr_module->clock_diff((new_value & CLKMASK) >> CLKSHIFT);

    if (diff & (SRQEN | SRLEN))
	m_sr_module->Qoutput();
    if (diff & (SRNQEN | SRLEN))
	m_sr_module->NQoutput();

    m_sr_module->update();
}

SRCON1::SRCON1(Processor *pCpu, const char *pName, const char *pDesc, SR_MODULE *_sr_module)
    : sfr_register(pCpu, pName, pDesc),
	m_sr_module(_sr_module), mValidBits(0xdd)
{
}

void SRCON1::put(unsigned int new_value)
{
    unsigned int masked_value = new_value & mValidBits;
    unsigned int diff = masked_value ^ value.get();

    trace.raw(write_trace.get() | value.get());

    value.put(masked_value);

    if (!diff)
	return;

    if (diff & (SRRCKE | SRSCKE))
    {
	if (!(new_value & (SRRCKE | SRSCKE)))	// all clocks off
	    m_sr_module->clock_disable(); // turn off clock
	else 
	    m_sr_module->clock_enable(); // turn on clock
    }
    m_sr_module->update();
}

class SRinSink : public SignalSink
{
public:
  SRinSink(SR_MODULE *_sr_module)
     : sr_module(_sr_module)
  {}

  virtual void setSinkState(char new3State)
  {
	sr_module->setState(new3State);
  }
  virtual void release() { delete this; }
private:
  SR_MODULE *sr_module;
};

SR_MODULE::SR_MODULE(Processor *_cpu) :
    srcon0(_cpu, "srcon0", "SR Latch Control 0 Register", this),
    srcon1(_cpu, "srcon1", "SR Latch Control 1 Register", this),
    cpu(_cpu), future_cycle(0), state_set(false), state_reset(false),
    state_Q(false), srclk(0), syncc1out(false), syncc2out(false),
    SRI_pin(0), SRQ_pin(0), SRNQ_pin(0), m_SRinSink(0),
    m_SRQsource(0), m_SRNQsource(0)
{
}
SR_MODULE::~SR_MODULE()
{
    if ( m_SRQsource)
	delete  m_SRQsource;
    if ( m_SRNQsource)
	delete  m_SRNQsource;
}

// determine output state of RS flip-flop
// If both state_set and state_reset are true, Q output is 0
// SPR[SP] and clocked inputs maybe set outside the update
// function prior to its call.
void SR_MODULE::update()
{

    if ((srcon1.value.get() & SRCON1::SRSC1E) && syncc1out)
	state_set = true;

    if ((srcon1.value.get() & SRCON1::SRSC2E) && syncc2out)
	state_set = true;

    if ((srcon1.value.get() & SRCON1::SRSPE) && SRI_pin->getPin().getState())
	state_set = true;

    if ((srcon1.value.get() & SRCON1::SRRC1E) && syncc1out)
	state_reset = true;

    if ((srcon1.value.get() & SRCON1::SRRC2E) && syncc2out)
	state_reset = true;

    if ((srcon1.value.get() & SRCON1::SRRPE) && SRI_pin->getPin().getState())
	state_reset = true;
    if (state_set)
	state_Q = true;

    // reset overrides a set
    if (state_reset)
	state_Q = false;


    state_set = state_reset = false;

    if (!(srcon0.value.get() & SRCON0::SRLEN))
	return;

    if ((srcon0.value.get() & SRCON0::SRQEN))
	m_SRQsource->putState(state_Q ? '1' : '0');

    if ((srcon0.value.get() & SRCON0::SRNQEN))
	m_SRNQsource->putState(!state_Q ? '1' : '0');

}

// Stop clock if currently running
void SR_MODULE::clock_disable()
{
    if (future_cycle> get_cycles().get())
    {
	get_cycles().clear_break(this);
	future_cycle = 0;
    }
    future_cycle = 0;
}

// Start clock if not running
// As break works on instruction cycles, clock runs every 1-128
// instructions which is 1 << srclk
//
void SR_MODULE::clock_enable()
{
    if (!future_cycle)
    {
	future_cycle = get_cycles().get() + (1 << srclk);
	get_cycles().set_break(future_cycle, this);
    }
}

// Called for clock rate change
void SR_MODULE::clock_diff(unsigned int _srclk)
{
    srclk = _srclk;

    clock_disable();

    if (srcon1.value.get() & (SRCON1::SRSCKE | SRCON1::SRRCKE)) 
    {
	clock_enable();
    }
}

void SR_MODULE::callback()
{
    bool active = false;

    if (srcon1.value.get() & (SRCON1::SRSCKE)) //Set clock enabled
    {
	active = true;
	pulse_set();
    }

    if (srcon1.value.get() & (SRCON1::SRRCKE)) //Reset clock enabled
    {
	active = true;
	pulse_reset();
    }
    if (active)
    {
	future_cycle = 0;
	clock_enable();
    }
    update();

}
void SR_MODULE::setPins(PinModule *sri, PinModule *srq, PinModule *srnq)
{

    if(!SRI_pin)
    {
	m_SRinSink = new SRinSink(this);
	sri->addSink(m_SRinSink);
    }
    else if (SRI_pin != sri)
    {
	SRI_pin->removeSink(m_SRinSink);
	sri->addSink(m_SRinSink);
    }
    SRI_pin = sri;
	SRQ_pin = srq;
	SRNQ_pin = srnq;

}

// If pin chnages and we are looking at it, call update
void SR_MODULE::setState(char IOin)
{
    if (srcon1.value.get() & (SRCON1::SRSPE | SRCON1::SRRPE))
	update();
}

void SR_MODULE::syncC1out(bool val)
{
    if (syncc1out != val)
    {
	syncc1out = val;
	if (srcon1.value.get() & (SRCON1::SRSC1E | SRCON1::SRRC1E))
	{
		update();
	}
    }
}
void SR_MODULE::syncC2out(bool val)
{
    if (syncc2out != val)
    {
	syncc2out = val;
	if (srcon1.value.get() & (SRCON1::SRSC2E | SRCON1::SRRC2E))
	{
		update();
	}
    }
}

// Setup or tear down RSQ output pin
// This is only call if SRLEN OR SRQEN has changed
void SR_MODULE::Qoutput()
{
    if ((srcon0.value.get() & (SRCON0::SRLEN | SRCON0::SRQEN)) ==
	(SRCON0::SRLEN | SRCON0::SRQEN))
    {
	if (!m_SRQsource)
	    m_SRQsource = new PeripheralSignalSource(SRQ_pin);

	SRQ_pin->setSource(m_SRQsource);
	SRQ_pin->getPin().newGUIname("SRQ");
    }
    else
    {
	SRQ_pin->setSource(0);
	if (strcmp("SRQ", SRQ_pin->getPin().GUIname().c_str()) == 0)
	{
	    SRQ_pin->getPin().newGUIname(SRQ_pin->getPin().name().c_str());
	}
    }
}
// Setup or tear down RSNQ output pin
// This is only call if SRLEN OR SRNQEN has changed
void SR_MODULE::NQoutput()
{
    if ((srcon0.value.get() & (SRCON0::SRLEN | SRCON0::SRNQEN)) == 
	(SRCON0::SRLEN | SRCON0::SRNQEN))
    {
	if (!m_SRNQsource)
	    m_SRNQsource = new PeripheralSignalSource(SRNQ_pin);

	SRNQ_pin->setSource(m_SRNQsource);
	SRNQ_pin->getPin().newGUIname("SRNQ");
    }
    else
    {
	SRNQ_pin->setSource(0);
	if (strcmp("SRNQ", SRNQ_pin->getPin().GUIname().c_str()) == 0)
	{
	    SRNQ_pin->getPin().newGUIname(SRNQ_pin->getPin().name().c_str());
	}
    }
}
