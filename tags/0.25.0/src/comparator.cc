/*
        
   Copyright (C) 1998 T. Scott Dattalo
   Copyright (C) 2006,2010 Roy R. Rankin

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
#include <string>
#include <cstdio>

#include "../config.h"

#include "pic-ioports.h"
#include "trace.h"
#include "processor.h"
#include "pir.h"
#include "stimuli.h"
#include "14bit-tmrs.h"
#include "comparator.h"

ComparatorModule::ComparatorModule(Processor *pCpu)
  : cmcon(pCpu,"cmcon", "Comparator Module Control"),
    cmcon1(pCpu,"cmcon1", "Comparator Configure Register"),
    vrcon(pCpu,"vrcon", "Voltage Reference Control")
{
}
void ComparatorModule::initialize( PIR_SET *pir_set,
        PinModule *pin_vr0, PinModule *pin_cm0,
        PinModule *pin_cm1, PinModule *pin_cm2, PinModule *pin_cm3,
        PinModule *pin_cm4, PinModule *pin_cm5)
{
  //  cmcon = new CMCON;
  cmcon.assign_pir_set(pir_set);
  cmcon.setINpin(0, pin_cm0);
  cmcon.setINpin(1, pin_cm1);
  cmcon.setINpin(2, pin_cm2);
  cmcon.setINpin(3, pin_cm3);
  cmcon.setOUTpin(0, pin_cm4);
  cmcon.setOUTpin(1, pin_cm5);
  vrcon.setIOpin(pin_vr0);
  cmcon._vrcon = &vrcon;
  vrcon._cmcon = &cmcon;
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
  ~CMSignalSource()
  {
    cout << "deleting CMsignal source " << this << endl;
  }
  virtual void release()
  {
    cout << "CMSignalSource:" << this << endl;
    delete this;
  }
  virtual char getState()
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
        _cmcon->get();  // recalculate comparator values
        nodeVoltage = v;
}


/*
    Setup the configuration for the comparators. Must be called
    for each comparator and each mode(CN2:CM0) that can be used.
        il1 = input Vin- when CIS == 0
        ih1 = input Vin+ when CIS == 0
        il2 = input Vin- when CIS == 1
        ih2 = input Vin+ when CIS == 1

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

//------------------------------------------------------------------------
CMCON::CMCON(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc),
    _vrcon(0),
    pir_set(0), m_tmrl(0), m_eccpas(0)
{
  value.put(0);
  cm_input[0]=cm_input[1]=cm_input[2]=cm_input[3]=0;
  cm_output[0] = cm_output[1] = 0;
  cm_input_pin[0]=cm_input_pin[1]=cm_input_pin[2]=cm_input_pin[3]=0;
  cm_output_pin[0]=cm_output_pin[1]=0;
  cm_source[0]=cm_source[1]=0;
  cm_stimulus[0]=cm_stimulus[1]=cm_stimulus[2]=cm_stimulus[3]=0;
  
}

CMCON::~CMCON()
{
  free(cm_input_pin[0]);  free(cm_input_pin[1]);
  free(cm_input_pin[2]);  free(cm_input_pin[3]);
  free(cm_output_pin[0]); free(cm_output_pin[1]);
  free(cm_output_pin[2]); free(cm_output_pin[3]);

}
void CMCON::setINpin(int i, PinModule *newPinModule)
{
    if (newPinModule == NULL) return;
    cm_input[i] = newPinModule;
    cm_input_pin[i] = strdup(newPinModule->getPin().name().c_str());
}
void CMCON::setOUTpin(int i, PinModule *newPinModule)
{
    if (newPinModule == NULL) return;
    cm_output[i] = newPinModule;
    cm_output_pin[i] = strdup(newPinModule->getPin().name().c_str());
}
void CMCON::assign_pir_set(PIR_SET *new_pir_set)
{
    pir_set = new_pir_set;
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
**      get()
**              read the comparator inputs and set C2OUT and C1OUT
**              as required. Also drive output pins if required.
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
                (m_configuration_bits[i][mode] >> (shift + CFG_SHIFT)) & CFG_MASK,
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
        else                    // Don't care about inputs, register value 0
            cmcon_val &= ~output_bit;
   }

   if (value.get() ^ cmcon_val) // change of state
   {
	// Signal ECCPAS ?
	if (m_eccpas)
	{
	    int diff = value.get() ^ cmcon_val;
	    if (diff & C1OUT)
		m_eccpas->c1_output(cmcon_val & C1OUT);
	    if (diff & C2OUT)
		m_eccpas->c2_output(cmcon_val & C2OUT);
	}
        // Generate interupt ?
        if (pir_set)
                pir_set->set_cmif();
   }
   if (m_tmrl)
	m_tmrl->compare_gate((cmcon_val & C1OUT) == C1OUT);
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

  if ((mode != 0) && (mode != 7) && ! cm_stimulus[0])   // initialize stimulus
  {
        cm_stimulus[0] = new CM_stimulus(this, "cm_stimulus_1", 0, 1e12);
        cm_stimulus[1] = new CM_stimulus(this, "cm_stimulus_2", 0, 1e12);
        cm_stimulus[2] = new CM_stimulus(this, "cm_stimulus_3", 0, 1e12);
        cm_stimulus[3] = new CM_stimulus(this, "cm_stimulus_4", 0, 1e12);
  }
  //
  // setup outputs
  //
  for( i = 0; i < 2 && cm_output[i]; i++)
  {
      if (out_mask & (1<<i))
      {
	  char name[20];
          if ( ! cm_source[i])
                cm_source[i] = new CMSignalSource();
	  sprintf(name, "c%dout", i+1);
	  cm_output[i]->getPin().newGUIname(name);
          cm_output[i]->setSource(cm_source[i]);
      }
      else if (cm_source[i])
      {
	    cm_output[i]->getPin().newGUIname(cm_output[i]->getPin().name().c_str());
            cm_output[i]->setSource(0);
      }
  }
  //
  // setup inputs
  for(i = 0; i < 4 && cm_input[i]; i++)
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
                cm_input[i]->AnalogReq(this, true, newname);
            }
        }
        else
        {

          if (!strncmp(name, "an", 2))
	    cm_input[i]->AnalogReq(this, false, cm_input[i]->getPin().name().c_str());

        }

   }

  // if only one comparator,  mask C2INV
  if (!cm_output[1]) new_value &= 0x1f;
  value.put(new_value);
  if (verbose)
      cout << "CMCON::put() val=0x" << hex << new_value <<endl;
  get();        // update comparator values

}
//------------------------------------------------------------------------
CMCON1::CMCON1(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc), m_tmrl(0)
{
}
CMCON1::~CMCON1() {}
void CMCON1::put(unsigned int new_value)
{

  if (verbose)
      cout << "CMCON1::put(new_value) =" << hex << new_value << endl;

  assert(m_tmrl);
  m_tmrl->set_T1GSS((new_value & T1GSS) == T1GSS);

  trace.raw(write_trace.get() | value.get());
  value.put(new_value);
}
//------------------------------------------------------------------------
SRCON::SRCON(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc)
{
    writable_bits = SR1 | SR0 | C1SEN | C2REN | FVREN;
   SR_Q = FALSE;
}
void SRCON::put(unsigned int new_value)
{
  // PULSR and PULSS should be only settable using bsf
  // it is not clear if these bits read anything other than 0,
  // but this is what I am assuming RRR
  if (new_value & PULSR)
	SR_Q = FALSE;
  else if (new_value & PULSS && ! reset)
	SR_Q = TRUE;
  trace.raw(write_trace.get() | value.get());
  value.put(new_value & writable_bits);

}
CM2CON1::CM2CON1(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc)
{
    writable_bits = C1RSEL | C2RSEL | T1GSS | C2SYNC;
}
void CM2CON1::link_cm12con0(CM1CON0 *_cm1con0, CM2CON0 *_cm2con0)
{
	m_cm1con0 = _cm1con0;
	m_cm2con0 = _cm2con0;
}
void CM2CON1::put(unsigned int new_value)
{
  unsigned int old_value = value.get();
  trace.raw(write_trace.get() | value.get());
  value.put(new_value & writable_bits);
  if (((new_value ^ old_value) & C1RSEL) && m_cm1con0)
	m_cm1con0->get();
  if (((new_value ^ old_value) & C2RSEL) && m_cm2con0)
	m_cm2con0->get();

  if (m_cm2con0->m_tmrl)
	m_cm2con0->m_tmrl->set_T1GSS((new_value & T1GSS) == T1GSS);


}


void CM1CON0::state_change(unsigned int cmcon_val)
{
   if (!cm_stimulus[0])
   {
        cm_stimulus[0] = new CM_stimulus((CMCON *)this, "cm1_stimulus_1", 0, 1e12);
        cm_stimulus[1] = new CM_stimulus((CMCON *)this, "cm1_stimulus_2", 0, 1e12);
   }
   if (value.get() ^ cmcon_val) // change of state
   {

	// Mirror C1OUT in CM2CON1::MC1OUT
	if (cmcon_val & OUT)
	{
	    m_cm2con1->value.put(m_cm2con1->value.get() | CM2CON1::MC1OUT);
	    if (m_srcon->value.get() & SRCON::C1SEN)
	    {
		m_srcon->set = TRUE;
		if (!m_srcon->reset)
		    m_srcon->SR_Q = TRUE;
	    }
	}
	else
	{
	    m_cm2con1->value.put(m_cm2con1->value.get() & ~CM2CON1::MC1OUT);
	    if (m_srcon->value.get() & SRCON::C1SEN)
	    {
		m_srcon->set = FALSE;
	    }
	}
	if (m_eccpas) m_eccpas->c1_output(cmcon_val & OUT);

        // Generate interupt ?
        if (pir_set)
	{
                pir_set->set_c1if();
	}
   }
   if (cmcon_val & OE)	// output pin enabled
   {
    if (!(m_srcon->value.get() & SRCON::SR0)) //SRCON select comparator output
    {
        cm_source->putState((cmcon_val & OUT) != 0);
     }
     else	// RS latch output
     {
        cm_source->putState(m_srcon->SR_Q);
     }
     cm_output->updatePinModule();
     update();
    }
}
double CM1CON0::CVref()
{

	if (m_cm2con1->value.get() & CM2CON1::C1RSEL)
	    return(m_vrcon->get_Vref());
	else
	    return(0.6);
}

void CM2CON0::state_change(unsigned int cmcon_val)
{
   if (value.get() ^ cmcon_val) // change of state
   {
	// Mirror C2OUT in CM2CON1::MC2OUT
	if (cmcon_val & OUT)
	    m_cm2con1->value.put(m_cm2con1->value.get() | CM2CON1::MC2OUT);
	else
	    m_cm2con1->value.put(m_cm2con1->value.get() & ~CM2CON1::MC2OUT);
        // Generate interupt ?
        if (pir_set)
                pir_set->set_c2if();
   }
   if (m_tmrl)
   {
	m_tmrl->compare_gate((cmcon_val & OUT) == OUT);
   }
  
   if (m_eccpas) m_eccpas->c2_output(cmcon_val & OUT);

   if (cmcon_val & OE)	// output pin enabled
   {
    if (!(m_srcon->value.get() & SRCON::SR1)) //SRCON select comparator output
    {
        cm_source->putState((cmcon_val & OUT) != 0);
     }
     else	// RS latch output
     {
        cm_source->putState(!m_srcon->SR_Q);
     }
     cm_output->updatePinModule();
     update();
    }
}
double CM2CON0::CVref()
{

	if (m_cm2con1->value.get() & CM2CON1::C2RSEL)
	    return(m_vrcon->get_Vref());
	else
	    return(0.6);
}

CM12CON0::CM12CON0(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc),
    m_vrcon(0), m_cm2con1(0), m_srcon(0),
    pir_set(0), m_tmrl(0)
{
  value.put(0);
  cm_input[0]=cm_input[1]=cm_input[2]=cm_input[3]=cm_input[4]=0;
  cm_output = 0;
  cm_source=0;
  cm_stimulus[0]=cm_stimulus[1]=0;
  cm_snode[0]=cm_snode[1]=0;
}

CM12CON0::~CM12CON0()
{

}
void CM12CON0::put(unsigned int new_value)
{
  unsigned int old_val = value.get();

  if (verbose)
      cout << "CM12CON0::put(new_value) =" << hex << new_value << endl;

  // ON/OFF or channel change
  if ((old_val ^ (new_value & 0xf7)) & ( ON | R | CH1 | CH0))
  {
	if (new_value & ON)
	{
	    int channel = new_value & (CH1 | CH0);
	    if ( !(new_value & R))	// Monitor input for CIN+
	    {
		if (cm_snode[0] == NULL)
		{
		    cm_snode[0] = cm_input[4]->getPin().snode;
		    if (cm_snode[0]) cm_snode[0]->attach_stimulus(cm_stimulus[0]);
		}
			
	    }
	    else if (cm_snode[0] != NULL) // Using CVref so turn of monitoring
	    {
		cm_snode[0]->detach_stimulus(cm_stimulus[0]);
		cm_snode[0] = NULL;
	    }
	    if (cm_snode[1] == NULL) // No CIN- monitoring active
	    {
		cm_snode[1] = cm_input[channel]->getPin().snode;
		if (cm_snode[1]) cm_snode[1]->attach_stimulus(cm_stimulus[1]);
	    }
	    // Change CIN- monitoring if on differnt pin
	    else if (cm_snode[1] != cm_input[channel]->getPin().snode)
	    {
		cm_snode[1]->detach_stimulus(cm_stimulus[1]);
		cm_snode[1] = cm_input[channel]->getPin().snode;
		if (cm_snode[1]) cm_snode[1]->attach_stimulus(cm_stimulus[1]);
	    }
	    
	}
	else // turning off, stop all monitoring of pins
	{
	    if (cm_snode[0] != NULL)
	    {
		cm_snode[0]->detach_stimulus(cm_stimulus[0]);
                cm_snode[0] = NULL;
            }
	    if (cm_snode[1] != NULL)
	    {
		cm_snode[1]->detach_stimulus(cm_stimulus[1]);
                cm_snode[1] = NULL;
	    }
	}

  }

  trace.raw(write_trace.get() | value.get());
  value.put(new_value & 0xf7);

  get();        // update comparator values

}
/*
**      get()
**              read the comparator inputs and set C2OUT and C1OUT
**              as required. Also drive output pins if required.
*/
unsigned int CM12CON0::get()
{
    unsigned int cmcon_val = value.get();
    bool out_true;
    if (cmcon_val & ON)
    {
        double Vhigh;
        double Vlow;
	Vlow = cm_input[cmcon_val & 0x3]->getPin().get_nodeVoltage();
	if (cmcon_val & R) // use Cvref
	{
		Vhigh = CVref();
	}
	else	// use cin+
	{
	    Vhigh = cm_input[4]->getPin().get_nodeVoltage();
	}
	out_true = Vhigh > Vlow;

	out_true ^= ((cmcon_val & POL) == POL);
    }
    else	// If not on, output same as POL
    {
	out_true = ((cmcon_val & POL) == POL);
    }
    if (out_true)
	cmcon_val |= OUT;
    else
	cmcon_val &= ~OUT;

   state_change(cmcon_val);
   value.put(cmcon_val);
   return(cmcon_val);
}
void CM12CON0::setpins(PinModule * c12in0, PinModule * c12in1,
        PinModule * c12in2, PinModule * c12in3,
        PinModule * cinPlus, PinModule * cout)
{
   // Multiplexed in - pins
   cm_input[0] = c12in0;
   cm_input[1] = c12in1;
   cm_input[2] = c12in2;
   cm_input[3] = c12in3;

   // one in + pin
   cm_input[4] = cinPlus;

   // one output pin
   cm_output = cout;
   if (! cm_source)
	cm_source = new CMSignalSource();
   cm_output->setSource(cm_source);
} 
void CM12CON0::link_registers(PIR_SET *new_pir_set, CM2CON1 *_cm2con1,
        VRCON *_vrcon, SRCON *_srcon, ECCPAS *_eccpas)
{
	pir_set = new_pir_set;
	m_cm2con1 = _cm2con1;
	m_vrcon = _vrcon;
	m_srcon = _srcon;
	m_eccpas = _eccpas;
}

//--------------------------------------------------
//      Voltage reference
//--------------------------------------------------

VRCON::VRCON(Processor *pCpu, const char *pName, const char *pDesc)
  : sfr_register(pCpu, pName, pDesc),
    vr_PinModule(0),
    pin_name(0)
{
  valid_bits = VR0|VR1|VR2|VR3|VRR|VROE|VREN;
  value.put(0);
}

VRCON::~VRCON()
{
  delete pin_name;
}

void VRCON::setIOpin(PinModule *newPinModule)
{
    if (newPinModule == NULL) return;
    vr_PinModule = newPinModule;
    pin_name = strdup(newPinModule->getPin().name().c_str());
}

double VRCON::get_Vref()
{
	unsigned int new_value = value.get();
        Vref_high =  ((Processor *)cpu)->get_Vdd();
   	Vref_low = 0.0;
        vr_Rhigh = (8 + (16 - (new_value & 0x0f))) * 2000.;
        vr_Rlow = (new_value & 0x0f) * 2000.;
        if (! (new_value & VRR))        // High range ?
            vr_Rlow += 16000.;

        vr_Vref = (Vref_high - Vref_low) * vr_Rlow / (vr_Rhigh + vr_Rlow) + Vref_low;
        if (verbose)
        {
            cout << "VRCON::put Rhigh=" <<vr_Rhigh << " Rlow=" << vr_Rlow
                << " Vout=" << vr_Vref << endl;
        }
	return(vr_Vref);
}
void VRCON::put(unsigned int new_value)
{

  new_value &= valid_bits;
  unsigned int old_value = value.get();
  unsigned int diff = new_value ^ old_value;

  trace.raw(write_trace.get() | value.get());

  if (verbose & 2)
        cout << "VRCON::put old=" << hex << old_value << " new=" << new_value << endl;
  if (!diff)
        return;

  // if no PinModule clear VROE bit
  if (!vr_PinModule) new_value &= ~VROE;
  value.put(new_value);
  if (new_value & VREN)         // Vreference enable set
  {
	get_Vref();
        if (new_value & VROE)   // output voltage to pin
        {

            if (! vr_pu)
            {
                vr_pu = new stimulus("vref_pu", Vref_high, vr_Rhigh);
            }

            if (! vr_pd)
                vr_pd = new stimulus("vref_pd", Vref_low, vr_Rlow);
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
        else if (vr_PinModule)    // not outputing voltage to pin
        {
            if (!strcmp("Vref", vr_PinModule->getPin().name().c_str()))
                vr_PinModule->getPin().newGUIname(pin_name);
            if (diff & 0x0f)    // did value of vreference change ?
                _cmcon->get();
            if(vr_PinModule && vr_PinModule->getPin().snode)
            {
                vr_PinModule->getPin().snode->detach_stimulus(vr_pu);
                vr_PinModule->getPin().snode->detach_stimulus(vr_pd);
                vr_PinModule->getPin().snode->update();
            }
        }
  }
  else  // vref disable
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


