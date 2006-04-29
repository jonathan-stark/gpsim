/*
   Copyright (C) 1998 T. Scott Dattalo
   Copyright (C) 2006 Roy R. Rankin

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



#include <stdio.h>
#include <iostream>
#include <string>

#include "pic-ioports.h"
#include "trace.h"
#include "processor.h"
#include "pir.h"
#include "stimuli.h"
#include "comparator.h"

void COMPARATOR_MODULE::initialize( PIR_SET *pir_set,
	PinModule *pin_vr0, PinModule *pin_cm0, 
	PinModule *pin_cm1, PinModule *pin_cm2, PinModule *pin_cm3, 
	PinModule *pin_cm4)
{
    cmcon = new CMCON_1;
    cmcon->assign_pir_set(pir_set);
    cmcon->setINpin(0, pin_cm0);
    cmcon->setINpin(1, pin_cm1);
    cmcon->setINpin(2, pin_cm2);
    cmcon->setINpin(3, pin_cm3);
    cmcon->setOUTpin(0, pin_cm3);
    cmcon->setOUTpin(1, pin_cm4);
    vrcon.setIOpin(pin_vr0);
    cmcon->_vrcon = &vrcon;
    vrcon._cmcon = cmcon;
    cmcon->rename_pins(0);
}

void COMPARATOR_MODULE::initialize( PIR_SET *pir_set,
	PinModule *pin_vr0, PinModule *pin_cm0, 
	PinModule *pin_cm1, PinModule *pin_cm2, PinModule *pin_cm3, 
	PinModule *pin_cm4, PinModule *pin_cm5)
{
    cmcon = new CMCON_2;
    cmcon->assign_pir_set(pir_set);
    cmcon->setINpin(0, pin_cm0);
    cmcon->setINpin(1, pin_cm1);
    cmcon->setINpin(2, pin_cm2);
    cmcon->setINpin(3, pin_cm3);
    cmcon->setOUTpin(0, pin_cm4);
    cmcon->setOUTpin(1, pin_cm5);
    vrcon.setIOpin(pin_vr0);
    cmcon->_vrcon = &vrcon;
    vrcon._cmcon = cmcon;
    cmcon->rename_pins(7);
}

//--------------------------------------------------
//
//--------------------------------------------------
class CMSignalSource : public SignalControl
{
public:
  CMSignalSource()
    : m_state('0')
  {
  }
  char getState()
  {
    return m_state;
  }
  void putState(bool new_val)
  {
	m_state = new_val?'1':'0';
  }
private:
  char m_state;
};

CM_stimulus::CM_stimulus(CMCON * arg, const char *cPname,double _Vth, double _Zth)
  : stimulus(cPname, _Vth, _Zth)
{
	_cmcon = arg;
}

void   CM_stimulus::set_nodeVoltage(double v)
{
        _cmcon->get();	// recalculate comparator values
        nodeVoltage = v;
}


CMCON_1::CMCON_1(void)
{
}
CMCON_2::CMCON_2(void)
{
}
CMCON::CMCON(void)
{
  value.put(0);
}
void CMCON::setINpin(int i, PinModule *newPinModule)
{
    cm_input[i] = newPinModule;
    cm_input_pin[i] = strdup(newPinModule->getPin().name().c_str());
}
void CMCON::setOUTpin(int i, PinModule *newPinModule)
{
    cm_output[i] = newPinModule;
    cm_output_pin[i] = strdup(newPinModule->getPin().name().c_str());
}
void CMCON::assign_pir_set(PIR_SET *new_pir_set)
{
    pir_set = (PIR_SET_1 *)new_pir_set;
}
/*
**	get()
**		read the comparator inputs and set C2OUT and C1OUT
**		as required. Also drive output pins if required.
**		(CMCON version dummy, real function type dependant)
*/
unsigned int CMCON::get() 
{ 
	cout << "CMCON:: get() should not be called\n";
	return 0;
}
/*
**	Type 1 as used in P16F62x
*/
unsigned int CMCON_1::get()
{
    unsigned int cmcon_val = value.get();
    bool out1_true;
    bool out2_true;
    int i;
    double Vhigh1, Vlow1;
    double Vhigh2, Vlow2;

    Vhigh1 = Vhigh2 = 5.0;	// just to avoid compiler warning
    Vlow1 = Vlow2 = 0.0;	// just to avoid compiler warning
    switch (cmcon_val & 0x07)
    {
    case 0:	// Comparitor reset 
    case 7:	// Comparators off
	if (cmcon_val & (C1OUT|C2OUT) && pir_set)	// change output
	    pir_set->set_cmif();
	cmcon_val &= ~(C1OUT|C2OUT);
   	value.put(cmcon_val);
   	return(cmcon_val);
	break;

    case 1:	// 3 imputs muxed 2 comparitors
	Vhigh1 = Vhigh2 = cm_input[2]->getPin().get_nodeVoltage();
	i = (cmcon_val & CIS)?3:0;
	Vlow1 = cm_input[i]->getPin().get_nodeVoltage();
	Vlow2 = cm_input[1]->getPin().get_nodeVoltage();

	break;

    case 2:	// 4 inputs muxed vs Vref
	Vhigh1 = Vhigh2 = _vrcon->get_Vref();
	i = (cmcon_val & CIS)?3:0;
	Vlow1 = cm_input[i]->getPin().get_nodeVoltage();
	i = (cmcon_val & CIS)?2:1;
	Vlow2 = cm_input[i]->getPin().get_nodeVoltage();
	    
	break;

    case 3:	// Two Common Reference Comparators
    case 6:	// Two common reference Comparitors with output
	Vhigh1 = Vhigh2 = cm_input[2]->getPin().get_nodeVoltage();
	Vlow1 = cm_input[0]->getPin().get_nodeVoltage();
	Vlow2 = cm_input[1]->getPin().get_nodeVoltage();

	break;
   case 4:	// two independant comparators
	Vhigh1 = cm_input[3]->getPin().get_nodeVoltage();
	Vhigh2 = cm_input[2]->getPin().get_nodeVoltage();
	Vlow1 = cm_input[0]->getPin().get_nodeVoltage();
	Vlow2 = cm_input[1]->getPin().get_nodeVoltage();
	break;

   case 5:	// One independant comparator
	if (cmcon_val & C1INV)
	{
 	    Vhigh1 = 5.;
	    Vlow1 = 0.;
	}
	else
	{
 	    Vhigh1 = 0.;
	    Vlow1 = 5.;
	}
	Vhigh2 = cm_input[2]->getPin().get_nodeVoltage();
	Vlow2 = cm_input[1]->getPin().get_nodeVoltage();
	break;

   }
    if (Vhigh1 > Vlow1)
	out1_true = (cmcon_val & C1INV)?false:true;
     else
	out1_true = (cmcon_val & C1INV)?true:false;

    if (Vhigh2 > Vlow2)
	out2_true = (cmcon_val & C2INV)?false:true;
    else
	out2_true = (cmcon_val & C2INV)?true:false;

   if (out1_true)  
	cmcon_val |= C1OUT;
   else
	cmcon_val &= ~C1OUT;
   if (out2_true)
	cmcon_val |= C2OUT;
   else
	cmcon_val &= ~C2OUT;

   if (value.get() ^ cmcon_val) // change of state
   {
   	if ((cmcon_val & 0x07) == 6)	// drive 2 outputs
   	{
	    cm_source[0]->putState(out1_true);
	    cm_source[1]->putState(out2_true);
	    cm_output[0]->updatePinModule();
	    cm_output[1]->updatePinModule();
	    update();
   	}
	// Generate interupt ?
	if (pir_set)
		pir_set->set_cmif();
   }

   if (verbose)
   {
	cout << "CMCON_1::get() Vhigh1=" << Vhigh1 << " Vlow1=" << Vlow1 
     	    << " out1_true=" << out1_true << endl;
	cout << "               Vhigh2=" << Vhigh2 << " Vlow2=" << Vlow2 
     	    << " out2_true=" << out2_true << endl;
	cout <<"               cmcon =0x" << hex << cmcon_val <<endl;
   }
   value.put(cmcon_val);
   return(cmcon_val);
}
/*
**	Type 2 as used in P16F87xA
*/
unsigned int CMCON_2::get()
{
    unsigned int cmcon_val = value.get();
    bool out1_true;
    bool out2_true;
    int i;
    double Vhigh1, Vlow1;
    double Vhigh2, Vlow2;

    Vhigh1 = Vhigh2 = 5.0;	// just to avoid compiler warning
    Vlow1 = Vlow2 = 0.0;	// just to avoid compiler warning
    switch (cmcon_val & 0x07)
    {
    case 0:	// Comparitor reset 
    case 7:	// Comparators off
	if (cmcon_val & (C1OUT|C2OUT) && pir_set)	// change output
	    pir_set->set_cmif();
	cmcon_val &= ~(C1OUT|C2OUT);
   	value.put(cmcon_val);
   	return(cmcon_val);
	break;


    case 6:	// 4 inputs muxed vs Vref
	Vhigh1 = Vhigh2 = _vrcon->get_Vref();
	i = (cmcon_val & CIS)?3:0;
	Vlow1 = cm_input[i]->getPin().get_nodeVoltage();
	i = (cmcon_val & CIS)?2:1;
	Vlow2 = cm_input[i]->getPin().get_nodeVoltage();
	    
	break;

    case 4:	// Two Common Reference Comparators
    case 5:	// Two common reference Comparitors with output
	Vhigh1 = Vhigh2 = cm_input[3]->getPin().get_nodeVoltage();
	Vlow1 = cm_input[0]->getPin().get_nodeVoltage();
	Vlow2 = cm_input[1]->getPin().get_nodeVoltage();

	break;

   case 2:	// two independant comparators
   case 3:	// two independant comparators with output
	Vhigh1 = cm_input[3]->getPin().get_nodeVoltage();
	Vhigh2 = cm_input[2]->getPin().get_nodeVoltage();
	Vlow1 = cm_input[0]->getPin().get_nodeVoltage();
	Vlow2 = cm_input[1]->getPin().get_nodeVoltage();
	break;

   case 1:	// One independant comparator with output
	if (cmcon_val & C2INV)
	{
 	    Vhigh2 = 5.;
	    Vlow2 = 0.;
	}
	else
	{
 	    Vhigh2 = 0.;
	    Vlow2 = 5.;
	}
	Vhigh1 = cm_input[3]->getPin().get_nodeVoltage();
	Vlow1 = cm_input[0]->getPin().get_nodeVoltage();
	break;

   }
    if (Vhigh1 > Vlow1)
	out1_true = (cmcon_val & C1INV)?false:true;
     else
	out1_true = (cmcon_val & C1INV)?true:false;

    if (Vhigh2 > Vlow2)
	out2_true = (cmcon_val & C2INV)?false:true;
    else
	out2_true = (cmcon_val & C2INV)?true:false;

   if (out1_true)  
	cmcon_val |= C1OUT;
   else
	cmcon_val &= ~C1OUT;
   if (out2_true)
	cmcon_val |= C2OUT;
   else
	cmcon_val &= ~C2OUT;

   if (value.get() ^ cmcon_val) // change of state
   {
   	switch ((cmcon_val & 0x07) )	// drive  outputs
   	{
	case 3:
	case 5:
	    cm_source[0]->putState(out1_true);
	    cm_source[1]->putState(out2_true);
	    cm_output[0]->updatePinModule();
	    cm_output[1]->updatePinModule();
	    update();
	    break;

	case 1:
	    cm_source[0]->putState(out1_true);
	    cm_output[0]->updatePinModule();
	    update();
	    break;
   	}
	// Generate interupt ?
	if (pir_set)
		pir_set->set_cmif();
   } 

   if (verbose)
   {
	cout << "CMCON_2::get() Vhigh1=" << Vhigh1 << " Vlow1=" << Vlow1 
     	    << " out1_true=" << out1_true << endl;
	cout << "             Vhigh2=" << Vhigh2 << " Vlow2=" << Vlow2 
     	    << " out2_true=" << out2_true << endl;
	cout <<"             cmcon =0x" << hex << cmcon_val <<endl;
   }
   value.put(cmcon_val);
   return(cmcon_val);
}

void CMCON::put(unsigned int new_value)
{
	cout << "CMCON::put should not be called\n";
}
/*
**	CMCON::put()
**		attach stimuli to appropriate pins so when an input changes,
**		the comparator value can be updated. Also if mode is 6 setup 
**		driving of output pins.
*/
void CMCON_1::put(unsigned int new_value)
{
                                                                                
  unsigned int mode = new_value & 0x7;
                                                                                
  trace.raw(write_trace.get() | value.get());
                                                                                
  if (mode ^ (value.get() & 0x7))
      rename_pins(mode);

  if (mode != 6)
  {
	if ( cm_source[0])
	    cm_output[0]->setSource(0);
	if ( cm_source[1])
	    cm_output[1]->setSource(0);
  }
  if ((mode != 0) && (mode != 7) && ! cm_stimulus[0])	// initialize stimulus
  {
	cm_stimulus[0] = new CM_stimulus(this, "cm_stimulus_1", 0, 1e12);
	cm_stimulus[1] = new CM_stimulus(this, "cm_stimulus_2", 0, 1e12);
	cm_stimulus[2] = new CM_stimulus(this, "cm_stimulus_3", 0, 1e12);
	cm_stimulus[3] = new CM_stimulus(this, "cm_stimulus_4", 0, 1e12);
  }

  switch(mode)
  {
    case 6:			// Start outputs
	if (! cm_source[0])
		cm_source[0] = new CMSignalSource();
	if (! cm_source[1])
		cm_source[1] = new CMSignalSource();
	cm_output[0]->setSource(cm_source[0]);
	cm_output[1]->setSource(cm_source[1]);

   case 3:			// ports 0, 1 and 2
	if (cm_input[0]->getPin().snode)
	    cm_input[0]->getPin().snode->attach_stimulus(cm_stimulus[0]);
	if (cm_input[1]->getPin().snode)
	    cm_input[1]->getPin().snode->attach_stimulus(cm_stimulus[1]);
	if (cm_input[2]->getPin().snode)
	    cm_input[2]->getPin().snode->attach_stimulus(cm_stimulus[2]);
	if (cm_input[3]->getPin().snode)
	    cm_input[3]->getPin().snode->detach_stimulus(cm_stimulus[3]);
	break;

    case 1:
    case 2:
    case 4:	// stimulus ports 0, 1, 2, and 3
	if (cm_input[0]->getPin().snode)
	    cm_input[0]->getPin().snode->attach_stimulus(cm_stimulus[0]);
	if (cm_input[1]->getPin().snode)
	    cm_input[1]->getPin().snode->attach_stimulus(cm_stimulus[1]);
	if (cm_input[2]->getPin().snode)
	    cm_input[2]->getPin().snode->attach_stimulus(cm_stimulus[2]);
	if (cm_input[3]->getPin().snode)
	    cm_input[3]->getPin().snode->attach_stimulus(cm_stimulus[3]);
	break;

    case 5:	// Stimulus ports 1 and 2
	if (cm_input[0]->getPin().snode)
	    cm_input[0]->getPin().snode->detach_stimulus(cm_stimulus[0]);
	if (cm_input[1]->getPin().snode)
	    cm_input[1]->getPin().snode->attach_stimulus(cm_stimulus[1]);
	if (cm_input[2]->getPin().snode)
	    cm_input[2]->getPin().snode->attach_stimulus(cm_stimulus[2]);
	if (cm_input[3]->getPin().snode)
	    cm_input[3]->getPin().snode->detach_stimulus(cm_stimulus[3]);
	break;

    case 0:
    case 7:		// detach stimulus
	if (cm_input[0]->getPin().snode && cm_stimulus[0])
	    cm_input[0]->getPin().snode->detach_stimulus(cm_stimulus[0]);
	if (cm_input[1]->getPin().snode && cm_stimulus[1])
	    cm_input[1]->getPin().snode->detach_stimulus(cm_stimulus[1]);
	if (cm_input[2]->getPin().snode && cm_stimulus[2])
	    cm_input[2]->getPin().snode->detach_stimulus(cm_stimulus[2]);
	if (cm_input[3]->getPin().snode && cm_stimulus[3])
	    cm_input[3]->getPin().snode->detach_stimulus(cm_stimulus[3]);
	break;

  }

  value.put(new_value);
  if (verbose)
      cout << "CMCON_1::put() val=0x" << hex << new_value <<endl;
  get();	// update comparator values
}
void CMCON_2::put(unsigned int new_value)
{
                                                                                
  unsigned int mode = new_value & 0x7;
                                                                                
  trace.raw(write_trace.get() | value.get());
                                                                                
  if (mode ^ (value.get() & 0x7))
      rename_pins(mode);	

  // Turn off outputs not being used

  switch(mode)
  {
  case 1:
	if (! cm_source[0])
		cm_source[0] = new CMSignalSource();
	if ( cm_source[1])
	    	cm_output[1]->setSource(0);
	cm_output[0]->setSource(cm_source[0]);
	break;

  case 3:
  case 5:
	if (! cm_source[0])
		cm_source[0] = new CMSignalSource();
	if (! cm_source[1])
		cm_source[1] = new CMSignalSource();
	cm_output[0]->setSource(cm_source[0]);
	cm_output[1]->setSource(cm_source[1]);
	break;

  default:
	if ( cm_source[0])
	    cm_output[0]->setSource(0);
	if ( cm_source[1])
	    cm_output[1]->setSource(0);
	break;
  }
  if ((mode != 0) && (mode != 7) && ! cm_stimulus[0])	// initialize stimulus
  {
	cm_stimulus[0] = new CM_stimulus(this, "cm_stimulus_1", 0, 1e12);
	cm_stimulus[1] = new CM_stimulus(this, "cm_stimulus_2", 0, 1e12);
	cm_stimulus[2] = new CM_stimulus(this, "cm_stimulus_3", 0, 1e12);
	cm_stimulus[3] = new CM_stimulus(this, "cm_stimulus_4", 0, 1e12);
  }

  switch(mode)
  {
    case 3:			// Start outputs
	cm_output[0]->setSource(cm_source[0]);
	cm_output[1]->setSource(cm_source[1]);
	// Fall Through
    case 6:
    case 2:	// stimulus ports 0, 1, 2, and 3
	if (cm_input[0]->getPin().snode)
	    cm_input[0]->getPin().snode->attach_stimulus(cm_stimulus[0]);
	if (cm_input[1]->getPin().snode)
	    cm_input[1]->getPin().snode->attach_stimulus(cm_stimulus[1]);
	if (cm_input[2]->getPin().snode)
	    cm_input[2]->getPin().snode->attach_stimulus(cm_stimulus[2]);
	if (cm_input[3]->getPin().snode)
	    cm_input[3]->getPin().snode->attach_stimulus(cm_stimulus[3]);
	break;

    case 5:	
	cm_output[0]->setSource(cm_source[0]);
	cm_output[1]->setSource(cm_source[1]);
	// Fall Through
    case 4:	// Stimulus ports 0, 1 and 3
	if (cm_input[0]->getPin().snode)
	    cm_input[0]->getPin().snode->attach_stimulus(cm_stimulus[0]);
	if (cm_input[1]->getPin().snode)
	    cm_input[1]->getPin().snode->attach_stimulus(cm_stimulus[1]);
	if (cm_input[2]->getPin().snode)
	    cm_input[2]->getPin().snode->detach_stimulus(cm_stimulus[2]);
	if (cm_input[3]->getPin().snode)
	    cm_input[3]->getPin().snode->attach_stimulus(cm_stimulus[3]);
	break;

    case 1:	// stimulus ports 0 and 3 only
	cm_output[0]->setSource(cm_source[0]);
	cm_output[1]->setSource(0);
	if (cm_input[0]->getPin().snode)
	    cm_input[0]->getPin().snode->attach_stimulus(cm_stimulus[0]);
	if (cm_input[1]->getPin().snode)
	    cm_input[1]->getPin().snode->detach_stimulus(cm_stimulus[1]);
	if (cm_input[2]->getPin().snode)
	    cm_input[2]->getPin().snode->detach_stimulus(cm_stimulus[2]);
	if (cm_input[3]->getPin().snode)
	    cm_input[3]->getPin().snode->attach_stimulus(cm_stimulus[3]);
	break;

    case 0:
    case 7:		// detach stimulus
	if (cm_input[0]->getPin().snode && cm_stimulus[0])
	    cm_input[0]->getPin().snode->detach_stimulus(cm_stimulus[0]);
	if (cm_input[1]->getPin().snode && cm_stimulus[1])
	    cm_input[1]->getPin().snode->detach_stimulus(cm_stimulus[1]);
	if (cm_input[2]->getPin().snode && cm_stimulus[2])
	    cm_input[2]->getPin().snode->detach_stimulus(cm_stimulus[2]);
	if (cm_input[3]->getPin().snode && cm_stimulus[3])
	    cm_input[3]->getPin().snode->detach_stimulus(cm_stimulus[3]);
	break;

  }

  value.put(new_value);
  if (verbose)
  	cout << "CMCON_2::put() val=0x" << hex << new_value <<endl;
  get();	// update comparator values
}

void CMCON_1::rename_pins(unsigned int new_value)
{
   if (verbose)
	cout << "CMCON_1::rename_pins new_value=" << new_value << endl;
    switch(new_value & 7)
    {
      case 7:
	cm_input[0]->getPin().newGUIname(cm_input_pin[0]);
	cm_input[1]->getPin().newGUIname(cm_input_pin[1]);
	cm_input[2]->getPin().newGUIname(cm_input_pin[2]);
	cm_input[3]->getPin().newGUIname(cm_input_pin[3]);
	cm_output[1]->getPin().newGUIname(cm_output_pin[1]);
	break;

      case 0:
      case 1:
      case 2:
      case 4:
	cm_input[0]->getPin().newGUIname("an0");
	cm_input[1]->getPin().newGUIname("an1");
	cm_input[2]->getPin().newGUIname("an2");
	cm_input[3]->getPin().newGUIname("an3");
	cm_output[1]->getPin().newGUIname(cm_output_pin[1]);
	break;

      case 3:
	cm_input[0]->getPin().newGUIname("an0");
	cm_input[1]->getPin().newGUIname("an1");
	cm_input[2]->getPin().newGUIname("an2");
	cm_output[0]->getPin().newGUIname(cm_output_pin[0]);
	cm_output[1]->getPin().newGUIname(cm_output_pin[1]);
	break;

      case 5:
	cm_input[0]->getPin().newGUIname(cm_input_pin[0]);
	cm_input[1]->getPin().newGUIname("an1");
	cm_input[2]->getPin().newGUIname("an2");
	cm_output[0]->getPin().newGUIname(cm_output_pin[0]);
	cm_output[1]->getPin().newGUIname(cm_output_pin[1]);
	break;

      case 6:
	cm_input[0]->getPin().newGUIname("an0");
	cm_input[1]->getPin().newGUIname("an1");
	cm_input[2]->getPin().newGUIname("an2");
	cm_output[0]->getPin().newGUIname("cmp1");
	cm_output[1]->getPin().newGUIname("cmp2");
	break;

    }
}
void CMCON_2::rename_pins(unsigned int new_value)
{
   if (verbose)
	cout << "CMCON_2::rename_pins new_value=" << new_value << endl;
    switch(new_value & 7)
    {
      case 7:
	cm_input[0]->getPin().newGUIname(cm_input_pin[0]);
	cm_input[1]->getPin().newGUIname(cm_input_pin[1]);
	cm_input[2]->getPin().newGUIname(cm_input_pin[2]);
	cm_input[3]->getPin().newGUIname(cm_input_pin[3]);
	cm_output[0]->getPin().newGUIname(cm_output_pin[0]);
	cm_output[1]->getPin().newGUIname(cm_output_pin[1]);
	break;

      case 0:
      case 2:
      case 6:
	cm_input[0]->getPin().newGUIname("an0");
	cm_input[1]->getPin().newGUIname("an1");
	cm_input[2]->getPin().newGUIname("an2");
	cm_input[3]->getPin().newGUIname("an3");
	cm_output[0]->getPin().newGUIname(cm_output_pin[0]);
	cm_output[1]->getPin().newGUIname(cm_output_pin[1]);
	break;

      case 1:
	cm_input[0]->getPin().newGUIname("an0");
	cm_input[1]->getPin().newGUIname(cm_input_pin[1]);
	cm_input[2]->getPin().newGUIname(cm_input_pin[2]);
	cm_input[3]->getPin().newGUIname("an3");
	cm_output[0]->getPin().newGUIname("c1out");
	cm_output[1]->getPin().newGUIname(cm_output_pin[1]);
	break;

      case 3:
	cm_input[0]->getPin().newGUIname("an0");
	cm_input[1]->getPin().newGUIname("an1");
	cm_input[2]->getPin().newGUIname("an2");
	cm_input[3]->getPin().newGUIname("an3");
	cm_output[0]->getPin().newGUIname("c1out");
	cm_output[1]->getPin().newGUIname("c2out");
	break;

      case 4:
	cm_input[0]->getPin().newGUIname("an0");
	cm_input[1]->getPin().newGUIname("an1");
	cm_input[2]->getPin().newGUIname(cm_input_pin[2]);
	cm_input[3]->getPin().newGUIname("an3");
	cm_output[0]->getPin().newGUIname(cm_output_pin[0]);
	cm_output[1]->getPin().newGUIname(cm_output_pin[1]);
	break;

      case 5:
	cm_input[0]->getPin().newGUIname("an0");
	cm_input[1]->getPin().newGUIname("an1");
	cm_input[2]->getPin().newGUIname(cm_input_pin[2]);
	cm_input[3]->getPin().newGUIname("an3");
	cm_output[0]->getPin().newGUIname("c1out");
	cm_output[1]->getPin().newGUIname("c2out");
	break;

    }
}
//--------------------------------------------------
//	Voltage reference
//--------------------------------------------------

VRCON::VRCON(void)
{
  value.put(0);
}
void VRCON::setIOpin(PinModule *newPinModule)
{
    vr_PinModule = newPinModule;
    pin_name = strdup(newPinModule->getPin().name().c_str());
}

void VRCON::put(unsigned int new_value)
{
                                                                                
  new_value &= 0xef;	// Bit 4 always 0
  unsigned int old_value = value.get();
  unsigned int diff = new_value ^ old_value;
                                                                                
  trace.raw(write_trace.get() | value.get());
                                                                                
  if (verbose & 2)
  	cout << "VRCON::put old=" << hex << old_value << " new=" << new_value << endl;
  if (!diff)
	return;

  value.put(new_value);
  if (new_value & VREN)		// Vreference enable set
  {
    	double VDD =  ((Processor *)cpu)->get_Vdd();
	vr_Rhigh = (8 + (16 - new_value & 0x0f)) * 2000.;
	vr_Rlow = (new_value & 0x0f) * 2000.;
	if (! (new_value & VRR))	// High range ?
	    vr_Rlow += 16000.;
	
	vr_Vref = VDD * vr_Rlow / (vr_Rhigh + vr_Rlow);
	if (verbose)
	{
	    cout << "VRCON::put Rhigh=" <<vr_Rhigh << " Rlow=" << vr_Rlow 
		<< " Vout=" << vr_Vref << endl;
	}
	if (new_value & VROE)	// output voltage to pin
	{

	    if (! vr_pu)
	    {
		vr_pu = new stimulus("vref_pu", VDD, vr_Rhigh);
	    }

	    if (! vr_pd)
		vr_pd = new stimulus("vref_pd", 0.0, vr_Rlow);
	    if (strcmp("Vref", vr_PinModule->getPin().name().c_str()))
 	    	vr_PinModule->getPin().newGUIname("Vref");

	    if (vr_PinModule->getPin().snode)
	    {
		vr_pu->set_Zth(vr_Rhigh);
		vr_pd->set_Zth(vr_Rlow);
	    	vr_PinModule->getPin().snode->attach_stimulus(vr_pu);
	    	vr_PinModule->getPin().snode->attach_stimulus(vr_pd);
		vr_PinModule->getPin().snode->update();
	    }
	}
	else 	// not outputing voltage to pin
	{
	    if (!strcmp("Vref", vr_PinModule->getPin().name().c_str()))
 	    	vr_PinModule->getPin().newGUIname(pin_name);
	    if (diff & 0x0f)	// did value of vreference change ?
		_cmcon->get();
	    if(vr_PinModule && vr_PinModule->getPin().snode)
	    {
                vr_PinModule->getPin().snode->detach_stimulus(vr_pu);
                vr_PinModule->getPin().snode->detach_stimulus(vr_pd);
                vr_PinModule->getPin().snode->update();
	    }
	}
  }
  else	// vref disable
  {
    if (vr_PinModule && !strcmp("Vref", vr_PinModule->getPin().name().c_str()))
 	  vr_PinModule->getPin().newGUIname(pin_name);

    if(vr_PinModule && vr_PinModule->getPin().snode)
    {
          vr_PinModule->getPin().snode->detach_stimulus(vr_pu);
          vr_PinModule->getPin().snode->detach_stimulus(vr_pd);
          vr_PinModule->getPin().snode->update();
    }
  }
}


