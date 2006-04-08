/*
   Copyright (C) 1998-2002 T. Scott Dattalo
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
along with gpsim; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#ifndef __COMPARATOR_H__
#define __COMPARATOR_H__

/***************************************************************************
 *
 * Include file for: Processors with dual comparators and Voltage Refarence
 *
 * 
 *
 ***************************************************************************/


class CMSignalSource;
class VRSignalSource;
class CMCON;

class VRCON : public sfr_register
{
 public:

 CMCON *_cmcon;

  enum VRCON_bits
    {
      VR0 = 1<<0,
      VR1 = 1<<1,
      VR2 = 1<<2,
      VR3 = 1<<3,

      VRR = 1<<5,
      VROE = 1<<6,
      VREN = 1<<7
    };

  VRCON(void);

  virtual void put(unsigned int new_value);
  virtual void setIOpin(PinModule *);
  virtual double get_Vref(void) { return(vr_Vref); };
                                                                                
protected:
  PinModule 		*vr_PinModule;
  double 		vr_Vref;
  stimulus		*vr_pu;
  stimulus		*vr_pd;
  double		vr_Rhigh;
  double		vr_Rlow;

};

class CM_stimulus : public stimulus
{
   public:

	CM_stimulus(CMCON *arg, const char *n=0,
           double _Vth=0.0, double _Zth=1e12
           );

    CMCON *_cmcon;

     virtual void   set_nodeVoltage(double v);

};
class CMCON : public sfr_register
{
 public:


  VRCON *_vrcon;
  enum CMCON_bits
    {
      CM0 = 1<<0,
      CM1 = 1<<1,
      CM2 = 1<<2,
      CIS = 1<<3,
      C1INV = 1<<4,
      C2INV = 1<<5,
      C1OUT = 1<<6,
      C2OUT = 1<<7,
    };


  virtual void setINpin(int i, PinModule *);
  virtual void setOUTpin(int i, PinModule *);
  virtual void assign_pir_set(PIR_SET *new_pir_set);
  virtual unsigned int get();
  virtual void put(unsigned int);


  CMCON(void);

protected:
  PinModule *cm_input[4];
  PinModule *cm_output[2];
  CMSignalSource *cm_source[2];
  unsigned int m_CMval[2];
  PIR_SET_1 *pir_set;
  CM_stimulus *cm_stimulus[4];
};

class CMCON_1 : public CMCON	// Type 1 Comparator used by 16F62x
{
 public:
  virtual unsigned int get();
  virtual void put(unsigned int);
  CMCON_1(void);
};
class CMCON_2 : public CMCON	// Type 2 Comparator used by 16F87xA
{
 public:
  virtual unsigned int get();
  virtual void put(unsigned int);
  CMCON_2(void);
};
class COMPARATOR_MODULE
{
 public:

  IOPORT *comparator_port;

  CMCON *cmcon;
  VRCON vrcon;

  void initialize( PIR_SET *pir_set, PinModule *pin_vr0, PinModule *pin_cm0, 
	PinModule *pin_cm1, PinModule *pin_cm2,
	PinModule *pin_cm3, PinModule *pin_cm4);

  void initialize( PIR_SET *pir_set, PinModule *pin_vr0, PinModule *pin_cm0, 
	PinModule *pin_cm1, PinModule *pin_cm2,
	PinModule *pin_cm3, PinModule *pin_cm4, PinModule *pin_cm5);

};
#endif // __COMPARATOR_H__
