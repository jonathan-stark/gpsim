/*
   Copyright (C) 2006 T. Scott Dattalo

This file is part of the libgpsim_dspic library of gpsim

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

#if !defined(__DSPIC_REGISTERS_H__)
#define __DSPIC_REGISTERS_H__

#include "../registers.h"
#include "../trace.h"

namespace dspic {
  extern Trace *gTrace;
  class dsPicProcessor;
};

namespace dspic_registers {

  //------------------------------------------------------------
  // dspic Registers
  //
  // The dspic registers are natively 16-bit wide. We'll overide
  // most of the Register base class methods because of this.

  class dsPicRegister : public Register
  {
  public:

    /// The 'iMask' member is used to inhibit/enable three state logic
    /// (the i in iMask stands for init, which is the name of the variable
    /// holding the stated of the initialized bits).

    static unsigned int iMask;

    dsPicRegister(Processor *, const char *pName, const char *pDesc=0);
    /*
    dsPicRegister()
    {
      value.data = 0;
      value.init = 0xffff;
      por_value.data = 0;
      por_value.init = 0xffff;
    }

    dsPicRegister(RegisterValue &rv)
    {
      value = rv;
    }
    */
    virtual bool get_bit(unsigned int bit_number);

    virtual void put(unsigned int new_value)
    {
      RegisterValue rv = getRV_notrace();
      rv.data = new_value&0xffff;
      putRV(rv);
    }

    virtual unsigned int get()
    {
      RegisterValue rv = getRV();
      return rv.data;
    }

    virtual unsigned int get_value()
    {
      RegisterValue rv = getRV_notrace();
      return rv.data;
    }

    virtual void putRV_notrace(RegisterValue rv)
    {
#if defined(PROPAGATE_UNKNOWNS)
      if(gbPropagateUnknown) 
	{
	  unsigned int diff = value.data ^ rv.data;
	  value.init |= (rv.init | diff);
	  value.data &= ~diff;
	}
      else
#endif
	{
	  value.data = rv.data;
	  value.init = (rv.init & iMask);
	}

    }

    virtual void putRV(RegisterValue rv)
    {
      dspic::gTrace->raw(write_trace.get() | value.get());
      dspic::gTrace->raw( write_trace.geti() | value.geti());

      putRV_notrace(rv);
    }

    virtual RegisterValue getRV(void)
    {
      dspic::gTrace->raw(read_trace.get() | value.get());
      dspic::gTrace->raw(read_trace.geti() | value.geti());
      return getRV_notrace();
    }

    virtual RegisterValue getRV_notrace(void)
    {
      return RegisterValue(value.get(),value.geti()&iMask);
    }

    // getRVN is the same as getRV and is only overidden by the 
    // status register(s). This method is used by certain instructions
    // that might have the status register as the destination.
    // The 'N' means that the flags will be cleared.

    virtual RegisterValue getRVN(void)
    {
      dspic::gTrace->raw(read_trace.get() | value.get());
      dspic::gTrace->raw(read_trace.geti() | value.geti());
      return getRVN_notrace();
    }
    virtual RegisterValue getRVN_notrace(void)
    {
      return getRV_notrace();
    }

    virtual unsigned int register_size () const
    { 
      return 2; // bytes
    }
  };


  //------------------------------------------------------------
  //
  class Status : public dsPicRegister
  {
  public:

    enum {
      eC    = 1<<0,
      eZ    = 1<<1,
      eOV   = 1<<2,
      eN    = 1<<3,
      eRA   = 1<<4,
      eIPLD = 1<<5,
      eIPL1 = 1<<6,
      eIPL2 = 1<<7,
      eDC   = 1<<8,
      eDA   = 1<<9,
      eSAB  = 1<<10,
      eOAB  = 1<<11,
      eSB   = 1<<12,
      eSA   = 1<<13,
      eOB   = 1<<14,
      eOA   = 1<<15
    };

    Status(Processor *, const char *pName, const char *pDesc=0);

    inline void traceWrite()
    {
      dspic::gTrace->raw(write_trace.get() | value.get());
      dspic::gTrace->raw(write_trace.geti() | value.geti());
    }

    inline void putFlags(unsigned int flags, 
			 unsigned int mask,
			 unsigned int uninit)
    {
      traceWrite();
      value.data = (value.data & ~mask) | flags;
      value.init &= ~mask;
      value.init |= uninit;
    }

  };
  //------------------------------------------------------------
  //
  class WRegister : public dsPicRegister
  {
  public:
    WRegister();
  };
  //------------------------------------------------------------
  //
  class PCL : public dsPicRegister
  {
  public:
    virtual void put(unsigned int new_value);
    virtual void put_value(unsigned int new_value);
    virtual unsigned int get();
    virtual unsigned int get_value();

    PCL(Processor *, const char *pName, const char *pDesc=0);
  };

  //------------------------------------------------------------
  // dspic Program Counter
  //

  class dsPicProgramCounter : public Program_Counter
  {
  public:
    dsPicProgramCounter(dspic::dsPicProcessor *, PCL *);

    virtual void increment();
    //virtual void skip();
    virtual void jump(unsigned int new_address);
    //virtual void interrupt(unsigned int new_value);
    virtual void computed_goto(unsigned int new_value);
    //virtual void new_address(unsigned int new_value);
    virtual void put_value(unsigned int new_value);
    virtual unsigned int get_value();
    //virtual unsigned int get_next();

  protected:
    PCL  *m_pcl;
    dspic::dsPicProcessor *m_cpu;
  };

  //------------------------------------------------------------
  // dspic Stack
  //
  class Stack
  {
  public:
    Stack(dspic::dsPicProcessor *pCpu);
    void push();
    void pop();
  protected:
    dspic::dsPicProcessor *m_cpu;
  };
}
#endif // !defined(__DSPIC_REGISTERS_H__)
