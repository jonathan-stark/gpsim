/*
   Copyright (C) 2006 T. Scott Dattalo

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
  class PCL : public dsPicRegister
  {
  public:
    virtual void put(unsigned int new_value);
    virtual void put_value(unsigned int new_value);
    virtual unsigned int get();
    virtual unsigned int get_value();

    PCL();
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
    Stack();
    void push();
    void pop();
    void init(dspic::dsPicProcessor *);
  protected:
    dspic::dsPicProcessor *m_cpu;
  };
}
#endif // !defined(__DSPIC_REGISTERS_H__)
