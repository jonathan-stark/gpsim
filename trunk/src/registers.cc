/*
   Copyright (C) 1998-2003 Scott Dattalo

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


#include <stdio.h>
#include <iostream>
#include <iomanip>


#include "../config.h"
#include "xref.h"
#include "registers.h"
#include "trace.h"

unsigned int count_bits(unsigned int ui)
{
  unsigned int bits=0;

  while (ui) {
    ui &= (ui-1);
    bits++;
  }
  return bits;
}
//========================================================================
// toString
//
// Convert a RegisterValue type to a string.
//
// A RegisterValue type allows the bits of a register to take on three
// values: High, Low, or undefined. If all of the bits are defined,
// then this routine will convert register value to a hexadecimal string.
// Any undefined bits within a nibble will cause the associated nibble to
// be undefined and will get converted to a question mark.
//

char * RegisterValue::toString(char *str, int len, int regsize)
{
  if(str && len) {
    RegisterValue rv = *this;

    char hex2ascii[] = "0123456789ABCDEF";
    char undefNibble = '?';
    int i;

    int m = regsize+1;
    if(len < m)
      m = len;

    m--;

    for(i=0; i < m; i++) {
      if(rv.init & 0x0f)
	str[m-i-1] = undefNibble;
      else
	str[m-i-1] = hex2ascii[rv.data & 0x0f];
      rv.init >>= 4;
      rv.data >>= 4;
    }
    str[m] = 0;

  }
  return str;
}

//========================================================================
// toBitStr
//
// Convert a RegisterValue type to a bit string
//
// Given a pointer to a string, this function will convert a register
// value into a string of ASCII characters. If no names are given
// for the bits, then the default values of 'H', 'L', and '?' are 
// used for high, low and undefined.
//
// The input 'BitPos' is a bit mask that has a bit set for each bit that
// the user wishes to display.
//

char * RegisterValue::toBitStr(char *s, int len, unsigned int BitPos, 
			     char *HiBitNames,
			     char *LoBitNames,
			     char *UndefBitNames)
{
  unsigned int j,mask,max;
  int i;

  if(!s || len<=0)
    return 0;

  max = 32;

  int nBits = count_bits(BitPos);

  if(nBits >= len)
    nBits = len-1;

  for(i=nBits-1,j=0,mask=1; j<max; j++, mask<<=1) {

    if(BitPos & mask) {

      char H = HiBitNames ? HiBitNames[i] : '1';
      char L = LoBitNames ? LoBitNames[i] : '0';
      char U = UndefBitNames ? UndefBitNames[i] : '?';

      s[i] = (init & mask) ?  U : 
	((data & mask) ? H : L);

      if(--i < 0)
	break;
    }

  }

  s[nBits] = 0;

  return s;
}


//--------------------------------------------------
// Member functions for the file_register base class
//--------------------------------------------------
//

Register::Register(void)
{
  cpu = 0;
  new_name("file_register");

  // For now, initialize the register with valid data and set that data equal to 0.
  // Eventually, the initial value will be marked as 'uninitialized.

  por_value = RegisterValue(0,0);
  putRV(por_value);
  _xref.assign_data(this);
  read_access_count=0;
  write_access_count=0;
  bit_mask = 7;
  replacedBy = 0;

}
//ugh duplication in constructors...
Register::Register(Processor *_cpu)
  : gpsimValue(_cpu)
{

  new_name("file_register");

  // For now, initialize the register with valid data and set that data equal to 0.
  // Eventually, the initial value will be marked as 'uninitialized.

  por_value = RegisterValue(0,0);
  putRV(por_value);
  _xref.assign_data(this);
  read_access_count=0;
  write_access_count=0;
  bit_mask = 7;
  replacedBy = 0;

}
Register::~Register(void)
{

}

//------------------------------------------------------------
// get()
//
//  Return the contents of the file register.
// (note - breakpoints on file register reads
//  are not checked here. Instead, a breakpoint
//  object replaces those instances of file 
//  registers for which we wish to monitor.
//  So a file_register::get call will invoke
//  the breakpoint::get member function. Depending
//  on the type of break point, this get() may
//  or may not get called).

unsigned int Register::get(void)
{
  trace.raw(read_trace.get() | value.get());
  return(value.get());
}

//------------------------------------------------------------
// put()
//
//  Update the contents of the register.
//  See the comment above in file_register::get()
//  with respect to break points
//

void Register::put(unsigned int new_value)
{
  trace.raw(write_trace.get() | value.get());
  value.put(new_value);
}


bool Register::get_bit(unsigned int bit_number)
{

  return  (value.get() & (1<<bit_number) ) ? true : false;

}

double Register::get_bit_voltage(unsigned int bit_number)
{
  if(get_bit(bit_number))
    return 5.0;
  else
    return 0.0;
}
//--------------------------------------------------
// set_bit
//
//  set a single bit in a register. Note that this
// is really not intended to be used on the file_register
// class. Instead, setbit is a place holder for high level
// classes that overide this function
void Register::setbit(unsigned int bit_number, bool new_value)
{
  if(bit_number < bit_mask) {
    trace.raw(write_trace.get() | value.get());
    value.put((value.get() & ~(1<<bit_number)) | (1<<bit_number));
  }
}

//-----------------------------------------------------------
//  void Register::put_value(unsigned int new_value)
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

void Register::put_value(unsigned int new_value)
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

  update();

}

//-----------------------------------------------------------
// set_write_trace
// set_read_trace
//
// These functions initialize the trace type to be used for 
// register reads and writes.
//
void Register::set_write_trace(unsigned int wt)
{
  write_trace.data = wt;
  write_trace.init = wt + (1<<24); //(Trace::REGISTER_WRITE_INIT - Trace::REGISTER_WRITE);
}
void Register::set_read_trace(unsigned int rt)
{
  read_trace.data = rt;
  read_trace.init = rt + (1<<24); //(Trace::REGISTER_READ_INIT - Trace::REGISTER_READ);
}

//------------------------------------------------------------

char * Register::toString(char *str, int len)
{
  return value.toString(str, len, register_size()*2);
}
char * Register::toBitStr(char *s, int len)
{
  unsigned int bit_length = register_size() * 8;
  unsigned int bits = (1<<bit_length) - 1;

  return value.toBitStr(s,len,bits);
}


//--------------------------------------------------
//--------------------------------------------------
//--------------------------------------------------
sfr_register::sfr_register(Processor *_cpu)
  : Register(_cpu)
{

}

//--------------------------------------------------
// member functions for the InvalidRegister class
//--------------------------------------------------
void InvalidRegister::put(unsigned int new_value)
{
  cout << "attempt write to invalid file register: address 0x" << 
    hex << address<< ", value 0x" << new_value << '\n';

  bp.halt();
  trace.raw(write_trace.get() | value.get());

  return;
}

unsigned int InvalidRegister::get(void)
{
  cout << "attempt read from invalid file register\n";

  trace.raw(read_trace.get() | value.get());

  return(0);
}


InvalidRegister::InvalidRegister(unsigned int at_address)
{

  char name_str[100];
  sprintf (name_str, "invalid fr  0x%02x", at_address);
  new_name(name_str);
}

InvalidRegister::InvalidRegister(void)
{
  new_name("INVALID_REGISTER");
}



