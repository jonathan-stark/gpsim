/*
	out = output
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
	PinModule *pin_cm4, PinModule *pin_cm5)
{
    cmcon = new CMCON;
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


/*
    Setup the configuration for the comparators. Must be called
    for each comparator and each mode(CN2:CM0) that can be used.
	il1 = input Vin- when CIS == 0
	ih1 = input Vin+ when CIS == 0
	il2 = input Vin- when CIS == 1
	ih2 = input Vin+ when CIS == 1
	out = output

	if input == VREF, reference voltage is used.
*/

void CMCON::set_configuration(int comp, int mode, int il1, int ih1, int il2,
int ih2, int out)
{
    if (comp > cMaxComparators || comp < 1 )
    {
	cout << "CMCON::set_configuration comp=" << comp << " out of range\n";
	return;
    }
    if (mode > cMaxConfigurations)
    {
	cout << "CMCON::set_configuration mode too large\n";
	return;
    }
    m_configuration_bits[comp-1][mode] = (il1 << CFG_SHIFT*4) | 
	(ih1 << CFG_SHIFT*3) |
	(il2 << CFG_SHIFT*2) | (ih2 << CFG_SHIFT) | out;
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

double CMCON::comp_voltage(int ind, int invert)
{
    double Voltage;

    switch(ind)
    {
    case VREF:
	Voltage = _vrcon->get_Vref();
	break;

    case NO_IN:
	Voltage = invert ? 5. : 0.;
	break;

    default:
	Voltage = cm_input[ind]->getPin().get_nodeVoltage();
	break;
    }
    return Voltage;
}
/*
**	get()
**		read the comparator inputs and set C2OUT and C1OUT
**		as required. Also drive output pins if required.
*/
unsigned int CMCON::get() 
{ 
    unsigned int cmcon_val = value.get();
    int mode = cmcon_val & 0x07;
    int i;
                                                                                
    for (i = 0; i < 2; i++)
    {
	double Vhigh;
	double Vlow;
	bool out_true;
	int out;
	int invert_bit = (i == 0) ? C1INV : C2INV;
	int output_bit = (i == 0) ? C1OUT : C2OUT;
	int shift = (cmcon_val & CIS) ? CFG_SHIFT : CFG_SHIFT*3;

	if ((m_configuration_bits[i][mode] & CFG_MASK) != ZERO)
        {
	    Vhigh = comp_voltage( 
		(m_configuration_bits[i][mode] >> shift) & CFG_MASK,
		cmcon_val & invert_bit);
	    Vlow = comp_voltage( 
		(m_configuration_bits[i][mode] >> shift+CFG_SHIFT) & CFG_MASK,
		(cmcon_val & invert_bit) == 0);
		
	    if (Vhigh > Vlow)
	    	out_true = (cmcon_val & invert_bit)?false:true;
            else
	    	out_true = (cmcon_val & invert_bit)?true:false;

   	    if (out_true)  
	    	cmcon_val |= output_bit;
   	    else
	    	cmcon_val &= ~output_bit;

	    if ( (out = m_configuration_bits[i][mode] & CFG_MASK) < 2)
	    {
	    	cm_source[out]->putState(out_true);
	    	cm_output[out]->updatePinModule();
	    	update();
	    }
	}
	else			// Don't care about inputs, register value 0
	    cmcon_val &= ~output_bit;
   }

   if (value.get() ^ cmcon_val) // change of state
   {
	// Generate interupt ?
	if (pir_set)
		pir_set->set_cmif();
   }
   value.put(cmcon_val);
   return(cmcon_val);
}

void CMCON::put(unsigned int new_value)
{
  unsigned int mode = new_value & 0x7;
  unsigned int in_mask = 0;
  unsigned int out_mask = 0;
  unsigned int configuration;
  int i;
                                                                                
  if (verbose)
      cout << "CMCON::put(new_value) =" << hex << new_value << endl;

  trace.raw(write_trace.get() | value.get());


  // Determine used input and output pins
  for(i = 0; i < 2; i++)
  {
     configuration = m_configuration_bits[i][mode];
     if ((configuration & CFG_MASK) < 2)
	out_mask |= (1 << (configuration & CFG_MASK));
     for(int j = 0; j < 4; j++)
     {
	configuration >>= CFG_SHIFT;
	if ((configuration & CFG_MASK) < 4)
		in_mask |= (1 << (configuration & CFG_MASK));
     }
  }

  if (verbose)
      cout << "CMCON::put in_mask=" << in_mask << " out_mask=" << out_mask << endl;

  if ((mode != 0) && (mode != 7) && ! cm_stimulus[0])	// initialize stimulus
  {
	cm_stimulus[0] = new CM_stimulus(this, "cm_stimulus_1", 0, 1e12);
	cm_stimulus[1] = new CM_stimulus(this, "cm_stimulus_2", 0, 1e12);
	cm_stimulus[2] = new CM_stimulus(this, "cm_stimulus_3", 0, 1e12);
	cm_stimulus[3] = new CM_stimulus(this, "cm_stimulus_4", 0, 1e12);
  }
  //
  // setup outputs
  //
  for( i = 0; i < 2; i++)
  {
      if (out_mask & (1<<i))
      {
          if ( ! cm_source[i])
		cm_source[i] = new CMSignalSource();
	  cm_output[i]->setSource(cm_source[i]);
      }
      else if (cm_source[i])
      {
	    cm_output[i]->setSource(0);
      }
  }
  //
  // setup inputs
  for(i = 0; i < 4; i++)
  {
	const char *name = cm_input[i]->getPin().GUIname().c_str();

	if (cm_input[i]->getPin().snode)
        {
	    if (in_mask & (1 << i))
		(cm_input[i]->getPin().snode)->attach_stimulus(cm_stimulus[i]);
	    else
		(cm_input[i]->getPin().snode)->detach_stimulus(cm_stimulus[i]);
	}
	// rewrite GUI name as required
	if (in_mask & (1 << i) ) 
	{
	    char newname[20];

	    if (strncmp(name, "an", 2))
	    {
	    	sprintf(newname, "an%d", i);
	    	cm_input[i]->getPin().newGUIname(newname);
	    }
	}
	else
	{
	    if (!strncmp(name, "an", 2))
		cm_input[i]->getPin().newGUIname(cm_input[i]->getPin().name().c_str());
	}
	 
   }

  value.put(new_value);
  if (verbose)
      cout << "CMCON_1::put() val=0x" << hex << new_value <<endl;
  get();	// update comparator values
      
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


