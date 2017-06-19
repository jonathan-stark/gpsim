/*
   Copyright (C) 1998-2003 Scott Dattalo

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
#include <sstream>

#include "../config.h"
#include "xref.h"
#include "processor.h"
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

char * RegisterValue::toString(char *str, int len, int regsize) const
{
  if(str && len) {
    RegisterValue rv = *this;

    char hex2ascii[] = "0123456789ABCDEF";
    char undefNibble = '?';
    int i;

    int m = regsize * 2 + 1;
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
// SplitBitString
//
// The purpose of this routine is to convert a string of bitnames into
// an array of names. The string is formatted like:
//
//  b1.b2.b3
//
// In other words, a period is the delimeter between the names.
// This gets converted to:
//
//  b1
//  b2
//  b2
//
// INPUTS
//  n - number of names
//  in - input string formatted as described
//  in2 - if 'in' is NULL, then all 'n' names will be 'in2'
//
// OUTPUTS
//  out - an array to hold the strings.
//
// Note, the input string 'in' will be modified such that all of the '.'s will
// get turned into string terminating 0's.
//

static void SplitBitString(int n, const char **out, char *in, const char *in2)
{

  if(!in) {
    for(int i=0; i<n; i++)
      out[i] = in2;
  } else {

    char *str = in;
    for(int i =0; i<n; i++ ) {

      out[i] = in;

      str = strchr(in, '.');
      if(str) {
	*str = 0;
	in = ++str;
      }
      //cout << "split :" << i << " ==> " << out[i] << endl;
    }
  }
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
			       const char *cByteSeparator,
			       const char *HiBitNames,
			       const char *LoBitNames,
			       const char *UndefBitNames) const
{
  unsigned int i,mask,max;

  if(!s || len<=0)
    return 0;

  max = 32;

  unsigned int nBits = count_bits(BitPos);

  if(nBits >= max)
    nBits = max;

  const char *HiNames[32];
  const char *LoNames[32];
  const char *UndefNames[32];


  char *cHi = HiBitNames ? strdup(HiBitNames) : 0;
  char *cLo = LoBitNames ? strdup(LoBitNames) : 0;
  char *cUn = UndefBitNames ? strdup(UndefBitNames) : 0;

  SplitBitString(nBits, HiNames, cHi, "1");
  SplitBitString(nBits, LoNames, cLo, "0");
  SplitBitString(nBits, UndefNames, cUn, "?");

  char *dest = s;

  int bitNumber=31;
  for(i=0,mask=1<<31; mask; mask>>=1,bitNumber--) {

    if(BitPos & mask) {

      const char *H = HiNames[i];
      const char *L = LoNames[i];
      const char *U = UndefNames[i];

      const char *c = (init & mask) ?  U :
	((data & mask) ? H : L);

      strncpy(dest, c, len);
      int l = strlen(c);
      len -= l;
      dest += l;
      *dest = 0;

      if(i++>nBits || len < 0)
	break;

      if(cByteSeparator && bitNumber && ((bitNumber%8)==0)) {
	strncpy(dest, cByteSeparator, len);
	int l = strlen(cByteSeparator);
	len -= l;
	dest += l;
	*dest = 0;
	if(len < 0)
	  break;
      }

    }

  }

  free(cHi);
  free(cLo);
  free(cUn);

  return s;
}


//--------------------------------------------------
// Member functions for the file_register base class
//--------------------------------------------------
//
// For now, initialize the register with valid data and set that data equal to 0.
// Eventually, the initial value will be marked as 'uninitialized.

Register::Register(Module *_cpu, const char *pName, const char *pDesc)
  : Value(pName,pDesc,_cpu), value(RegisterValue(0, 0)),
    address(AN_INVALID_ADDRESS),
    alias_mask(0), por_value(RegisterValue(0, 0)), m_replaced(0)
{
  set_xref(new XrefObject(this));
  read_access_count=0;
  write_access_count=0;
  mValidBits = 0xFF;

}
Register::~Register()
{
  if (cpu) {
    //cout << "Removing register from ST:" << name_str <<  " addr "<< this << endl;
    cpu->removeSymbol(this);
  }
}

//------------------------------------------------------------
int Register::set_break(ObjectBreakTypes bt, ObjectActionTypes at, Expression *expr)
{
  return get_bp().set_break(bt, at, this, expr);
}

int Register::clear_break()
{
  return -1;
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

unsigned int Register::get()
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
    int set_mask = (1<<bit_number);

  if ( set_mask & mValidBits ) {
    trace.raw(write_trace.get() | value.get());
    value.put((value.get() & ~set_mask) | (new_value ? set_mask : 0));
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

  value.put(new_value);

  // Even though we just wrote a value to this register,
  // it's possible that the register did not get fully
  // updated (e.g. porta on many pics has only 5 valid
  // pins, so the upper three bits of a write are meaningless)
  // So we should explicitly tell the gui (if it's
  // present) to update its display.

  update();

}

///
/// New accessor functions
//////////////////////////////////////////////////////////////

unsigned int Register::register_size () const
{
  Processor *pProc = Value::get_cpu();
  return pProc == 0 ? 1 : pProc->register_size();
}


//-----------------------------------------------------------
// set_write_trace
// set_read_trace
//
// These functions initialize the trace type to be used for
// register reads and writes.
//
void Register::set_write_trace(RegisterValue &rv)
{
  write_trace = rv;
}
void Register::set_read_trace(RegisterValue &rv)
{
  read_trace = rv;
}

//------------------------------------------------------------

char * Register::toString(char *str, int len)
{
  return getRV_notrace().toString(str, len, register_size()*2);
}
char * Register::toBitStr(char *s, int len)
{
  unsigned int bit_length = register_size() * 8;
  unsigned int bits = (1<<bit_length) - 1;

  return getRV_notrace().toBitStr(s,len,bits);
}

//-----------------------------------------------------------

void Register::new_name(const char *s)
{

  if(s) {
    string str(s);
    new_name(str);
  }
}

void Register::new_name(string &new_name)
{
  if (name_str != new_name) {
    if (name_str.empty()) {
      name_str = new_name;
      return;
    }

    name_str = new_name;
    if (cpu) {
      addName(new_name);
      cpu->addSymbol(this, &new_name);
    }

  }
}
//------------------------------------------------------------------------
// set -- assgin the value of some other object to this Register
//
// This is used (primarily) during Register stimuli processing. If
// a register stimulus is attached to this register, then it will
// call ::set() and supply a Value pointer.

void Register::set(Value * pVal)
{
  Register *pReg = dynamic_cast<Register *>(pVal);
  if (pReg) {
    putRV(pReg->getRV());
    return;
  }

  if (pVal) {
    put_value( (unsigned int)*pVal);
  }
}

//------------------------------------------------------------------------
// copy - create a new Value object that's a 'copy' of this object
//
// We really don't perform a true copy. Instead, an Integer object
// is created containing the same numeric value of this object.
// This code is called during expression parsing. *NOTE* this copied
// object can be assigned a new value, however that value will not
// propagate to the Register!

Value *Register::copy()
{
  Value *val = new ValueWrapper(this);
  return val;
}
void Register::get(gint64 &i)
{
  i = get_value();
}
//--------------------------------------------------
//--------------------------------------------------
//--------------------------------------------------
sfr_register::sfr_register(Module *pCpu, const char *pName, const char *pDesc)
  : Register(pCpu,pName,pDesc), wdtr_value(0,0)
{}

void sfr_register::reset(RESET_TYPE r)
{
  switch (r) {

  case POR_RESET:
    putRV(por_value);
    break;

  case WDT_RESET:
    // Most registers simply retain their value across WDT resets.
    //putRV(wdtr_value);
    break;
  default:
    break;
  }

}

//--------------------------------------------------
//--------------------------------------------------

//--------------------------------------------------
// member functions for the InvalidRegister class
//--------------------------------------------------
void InvalidRegister::put(unsigned int new_value)
{
  cout << "attempt write to invalid file register\n";

  if (address != AN_INVALID_ADDRESS)
    cout << "    address 0x" << hex << address << ',';
  cout << "   value 0x" << hex << new_value << endl;

  if(((Processor*)cpu)->getBreakOnInvalidRegisterWrite()) {
    bp.halt();
  }
  trace.raw(write_trace.get() | value.get());

  return;
}

unsigned int InvalidRegister::get()
{
  cout << "attempt read from invalid file register\n";
  if (address != AN_INVALID_ADDRESS)
    cout << "    address 0x" << hex << address << endl;

  trace.raw(read_trace.get() | value.get());
  if(((Processor*)cpu)->getBreakOnInvalidRegisterRead()) {
    bp.halt();
  }

  return(0);
}


InvalidRegister::InvalidRegister(Processor *pCpu, const char *pName, const char *pDesc)
  : Register(pCpu,pName,pDesc)
{}

RegisterCollection::RegisterCollection (Processor   *pProcessor,
					const char  *pC_collection_name,
					Register   **ppRegisters,
					unsigned int uiSize) :
  IIndexedCollection(16), m_ReturnValue(0)
{
  m_pProcessor = pProcessor;
  gpsimObject::new_name(pC_collection_name);
  m_ppRegisters = ppRegisters;
  m_uSize = uiSize;
  pProcessor->addSymbol(this);
}

RegisterCollection:: ~RegisterCollection()
{
  if (m_pProcessor)
    m_pProcessor->removeSymbol(this);
}

// Displays in symbol GUI
void RegisterCollection::get(char *return_str, int len)
{
    if (return_str)
    {
	strncpy(return_str, "", len);
    }
}
unsigned int RegisterCollection::GetSize() {
  return m_uSize;
}

Value &RegisterCollection::GetAt(unsigned int uIndex, Value *) {
  if(uIndex > m_uSize) {
    throw Error("index is out of range");
  }
  m_ReturnValue.set((int)m_ppRegisters[uIndex]->get_value());
  m_ReturnValue.setBitmask(m_pProcessor->register_mask());
  ostringstream sIndex;
  if (m_pProcessor)
      sIndex << m_pProcessor->name() << "." ;

  sIndex << Value::name() << "["
	  << hex << m_szPrefix << uIndex << "]" << '\000';
  m_ReturnValue.new_name(sIndex.str().c_str());
  return m_ReturnValue;
}

void RegisterCollection::SetAt(unsigned int uIndex, Value *pValue)  {
  if(uIndex > m_uSize) {
    throw Error("index is out of range");
  }
  Integer *pInt = dynamic_cast<Integer*>(pValue);
  if(pInt == NULL) {
    throw Error("rValue is not an Integer");
  }
  else {
    m_ppRegisters[uIndex]->put((unsigned int)(int)*pInt);
  }
}

void RegisterCollection::ConsolidateValues(int &iColumnWidth,
                                           vector<string> &aList,
                                           vector<string> &aValue)
{
  unsigned int  uFirstIndex = 0;
  unsigned int  uIndex;
  Register *    pReg = m_ppRegisters[0];
  Integer       uLastValue(pReg->getRV_notrace().data);
  uLastValue.setBitmask(m_pProcessor->register_mask());
  for(uIndex = 0; uIndex < m_uSize; uIndex++) {
    pReg = m_ppRegisters[uIndex];
    RegisterValue rvValue = pReg->getRV_notrace();
    if((unsigned int)uLastValue != rvValue.data) {
      PushValue(uFirstIndex, uIndex,
                &uLastValue, aList, aValue);
      iColumnWidth = max(iColumnWidth, (int)aList.back().size());
      uFirstIndex = uIndex;
      uLastValue = rvValue.data;
    }
  }
  uIndex--;
  // Record the last set of elements
  if(uFirstIndex <= uIndex) {
    PushValue(uFirstIndex, uIndex,
              &uLastValue, aList, aValue);
    iColumnWidth = max(iColumnWidth, (int)aList.back().size());
  }
}

//void RegisterCollection::SetAt(ExprList_t* pIndexers, Expression *pExpr) {
//  throw Error("RegisterCollection::SetAt() not implemented");
//}

unsigned int RegisterCollection::GetLowerBound() {
  return 0;
}

unsigned int RegisterCollection::GetUpperBound() {
  return m_uSize - 1;
}


