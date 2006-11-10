/*
   Copyright (C) 1998-2003 Scott Dattalo
                 2003 Mike Durian
                 2006 David Barnett

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

#ifndef PM_RD_H
#define PM_RD_H

#include <assert.h>

#include "gpsim_classes.h"
#include "registers.h"
#include "breakpoints.h"

class pic_processor;
class PM_RD;

//---------------------------------------------------------
// PMCON1 - PM control register 1
//

class PMCON1 : public sfr_register
{
public:
enum
{
 RD    = (1<<0),
};


  PMCON1(void);

  void put(unsigned int new_value);
  unsigned int get(void);

  inline void set_pm(PM_RD *pm) {pm_rd = pm;}
  inline void set_valid_bits(unsigned int vb) { valid_bits = vb; }
  inline unsigned int get_valid_bits(void) { return (valid_bits); }
  inline void set_bits(unsigned int b) { valid_bits |= b; }
  inline void clear_bits(unsigned int b) { valid_bits &= ~b; }

  unsigned int valid_bits;
  PM_RD *pm_rd;
};

const unsigned int PMCON1_VALID_BITS = (PMCON1::RD);

//
// PMDATA - PM data register
//

class PMDATA : public sfr_register
{
public:

  PMDATA(void);

  void put(unsigned int new_value);
  unsigned int get(void);

};

//
// PMADR - PM address register
//

class PMADR : public sfr_register
{
public:

  PMADR(void);

  void put(unsigned int new_value);
  unsigned int get(void);
};


//------------------------------------------------------------------------
//------------------------------------------------------------------------

// For storing callback and cpu ptr and grouping PM regs
class PM_RD :  public TriggerObject
{
public:
  static const unsigned int READ_CYCLES = 2;

  PM_RD(void);
  virtual void set_cpu(pic_processor *p) { cpu = p; }

  virtual void callback(void);
  virtual void start_read(void);  

  inline virtual PMCON1 *get_reg_pmcon1(void) { return (&pmcon1); }
  inline virtual PMDATA *get_reg_pmdata(void) { return (&pmdata); }
  inline virtual PMDATA *get_reg_pmdath(void) { return (&pmdath); }
  inline virtual PMADR *get_reg_pmadr(void) { return (&pmadr); }
  inline virtual PMADR *get_reg_pmadrh(void) { return (&pmadrh); }

  //protected:
  pic_processor *cpu;

  PMCON1 pmcon1;
  PMDATA pmdata;
  PMDATA pmdath;
  PMADR  pmadr;
  PMADR  pmadrh;

  unsigned int rd_adr;          // latched adr

};


#endif /* PM_RD_H */
