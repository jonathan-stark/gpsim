/*
   Copyright (C) 1998 T. Scott Dattalo

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


#ifndef __PIC_INSTRUCTIONS_H__
#define __PIC_INSTRUCTIONS_H__


#ifdef HAVE_GUI
#include <glib.h>
#endif

#include "value.h"

class Register;


// FIXME get rid of AddressSymbol and LineNumberSymbol classes

class AddressSymbol : public Integer
{
public:

  AddressSymbol(Processor *pCpu, const char *, unsigned int);
  virtual string toString();
  virtual Value* evaluate();
  virtual int set_break(ObjectBreakTypes bt=eBreakAny,
                        ObjectActionTypes at=eActionHalt, 
                        Expression *expr=0);
};

class LineNumberSymbol : public AddressSymbol
{
protected:
  int src_id,src_line,lst_id,lst_line,lst_page;
 public:

  LineNumberSymbol(Processor *pCpu, const char *, unsigned int);
  void put_address(int new_address) {set(new_address);}
  void put_src_line(int new_src_line) {src_line = new_src_line;}
  void put_lst_line(int new_lst_line) {lst_line = new_lst_line;}
  void put_lst_page(int new_lst_page) {lst_page = new_lst_page;}
};




/*
 *  base class for an instruction
 */

class instruction : public Value
{
public:

  enum INSTRUCTION_TYPES
  {
    NORMAL_INSTRUCTION,
    INVALID_INSTRUCTION,
    BREAKPOINT_INSTRUCTION,
    NOTIFY_INSTRUCTION,
    PROFILE_START_INSTRUCTION,
    PROFILE_STOP_INSTRUCTION,
    MULTIWORD_INSTRUCTION,
    ASSERTION_INSTRUCTION
  };

  /*
   * Not all instructions derived from the instruction
   * class use these constants... 
   */

  enum
  {
    REG_MASK_12BIT = 0x1f,
    REG_MASK_14BIT = 0x7f,
    REG_MASK_16BIT = 0xff,
    DESTINATION_MASK_12BIT = 0x20,
    DESTINATION_MASK_14BIT = 0x80,
    DESTINATION_MASK_16BIT = 0x200,
    ACCESS_MASK_16BIT = 0x100,
  };


  instruction(Processor *pProcessor, unsigned int uOpCode, unsigned int uAddrOfInstr);
  virtual ~instruction();

  virtual void execute() = 0;
  virtual void debug(){ }
  virtual int instruction_size() { return 1;}
  virtual unsigned int get_opcode() { return opcode; }
  virtual unsigned int get_value() { return opcode; }
  virtual void put_value(unsigned int new_value) { }
  virtual unsigned int getAddress() { return m_uAddrOfInstr;}
  virtual void setAddress(unsigned int addr) { m_uAddrOfInstr = addr;}
  virtual int get_src_line() { return(src_line); }
  virtual int get_hll_src_line() { return(hll_src_line); }
  virtual void set_hll_src_line(int line) { hll_src_line=line; }
  virtual int get_lst_line() { return(lst_line); }
  virtual int get_file_id() {return(file_id); }
  virtual int get_hll_file_id() {return(hll_file_id); }
  virtual void set_hll_file_id(int file_id) {hll_file_id=file_id; }
  virtual enum INSTRUCTION_TYPES isa() {return NORMAL_INSTRUCTION;}
  virtual guint64 getCyclesUsed() { return cycle_count;}
  virtual bool isBase() = 0;
  void decode(Processor *new_cpu, unsigned int new_opcode);
  virtual void update_line_number(int file, int sline, int lline, int hllfile, int hllsline);

  virtual char *ReadSrcLine(char *buf, int nBytes);
  virtual char *ReadLstLine(char *buf, int nBytes);
  virtual char *ReadHLLLine(char *buf, int nBytes);

  virtual int set_break(ObjectBreakTypes bt=eBreakAny, ObjectActionTypes at=eActionHalt, Expression *expr=0);
  virtual void addLabel(string &rLabel);

  // Some instructions require special initialization after they've
  // been instantiated. For those that do, the instruction base class
  // provides a way to control the initialization state (see the 16-bit
  // PIC instructions).
  virtual void initialize(bool init_state) {};

  bool bIsModified() { return m_bIsModified; }
  void setModified(bool b) { m_bIsModified=b; }
  gpsimObject *getLineSymbol() { return pLineSymbol; }
protected:
  bool m_bIsModified; // flag indicating if this instruction has
                      // changed since start.
  guint64 cycle_count; // Nr of cycles used up by this instruction

  unsigned int opcode;
  unsigned int m_uAddrOfInstr;
  gpsimObject *pLineSymbol;
  int file_id;            /* The source file that declared this instruction
			   * (The file_id is an index into an array of files) */
  int hll_file_id;        /* The hll source file that declared this instruction */
  int src_line;           /* The line number within the source file */
  int lst_line;           /* The line number within the list file */
  int hll_src_line;       /* The line number within the HLL source file */
};


//---------------------------------------------------------
// An AliasedInstruction is a class that is designed to replace an
// instruction in program memory. (E.g. breakpoint instructions are an
// example).
class AliasedInstruction : public instruction
{
public:
  AliasedInstruction(instruction *);
  AliasedInstruction();
  AliasedInstruction(Processor *pProcessor, 
		     unsigned int uOpCode, 
		     unsigned int uAddrOfInstr);
  ~AliasedInstruction();
  void setReplaced(instruction *);
  virtual instruction *getReplaced();

  virtual void execute();
  virtual void debug();
  virtual int instruction_size();
  virtual unsigned int get_opcode();
  virtual unsigned int get_value();
  virtual void put_value(unsigned int new_value);
  virtual int get_src_line();
  virtual int get_hll_src_line();
  virtual int get_lst_line();
  virtual int get_file_id();
  virtual int get_hll_file_id();
  virtual void update_line_number(int file, int sline, int lline, int hllfile, int hllsline);
  virtual enum INSTRUCTION_TYPES isa();
  virtual void initialize(bool init_state);
  virtual char *name(char *,int len);
  virtual bool isBase();

  virtual void update(void);
  virtual void add_xref(void *xref);
  virtual void remove_xref(void *xref);

protected:
  instruction *m_replaced;

};

//---------------------------------------------------------
class invalid_instruction : public instruction
{
public:

  virtual void execute();

  virtual void debug()
  {
    //cout << "*** INVALID INSTRUCTION ***\n";
  };

  //invalid_instruction(Processor *new_cpu=0,unsigned int new_opcode=0);
  invalid_instruction(Processor *new_cpu,
                      unsigned int new_opcode,
                      unsigned int address);
  virtual enum INSTRUCTION_TYPES isa() {return INVALID_INSTRUCTION;};
  //virtual char *name(char *str){return("INVALID");};
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode,unsigned int address)
  {return new invalid_instruction(new_cpu,new_opcode,address);}
  virtual void addLabel(string &rLabel);
  virtual bool isBase() { return true; }

};

//---------------------------------------------------------
class Literal_op : public instruction
{
public:
  Literal_op(Processor *pProcessor, unsigned int uOpCode, unsigned int uAddrOfInstr);

  unsigned int L;

  virtual void debug(){ };
  virtual char *name(char *,int);
  virtual bool isBase() { return true; }

  void decode(Processor *new_cpu, unsigned int new_opcode);
};


//---------------------------------------------------------
class Bit_op : public instruction
{
public:
  Bit_op(Processor *pProcessor, unsigned int uOpCode, unsigned int uAddrOfInstr);

  unsigned int mask,register_address;
  bool access;
  Register *reg;

  virtual void debug(){ };
  virtual char *name(char *,int);
  virtual bool isBase() { return true; }

  void decode(Processor *new_cpu, unsigned int new_opcode);

};


//---------------------------------------------------------
class Register_op : public instruction
{
public:

  Register_op(Processor *pProcessor, unsigned int uOpCode, unsigned int uAddrOfInstr);

  static Register *source;
  unsigned int register_address;
  bool destination, access;

  /*  Register *destination;*/

  virtual void debug(){ };
  virtual char *name(char *,int);
  virtual bool isBase() { return true; }

  void decode(Processor *new_cpu, unsigned int new_opcode);

};




//-----------------------------------------------------------------
//
// instruction_constructor - a class used to create the PIC instructions
//
// The way it works is the 'instruction_constructor' structure
// contains three pieces of info for each instruction:
//   inst_mask - a bit mask indicating which bits uniquely
//               identify an instruction
//   opcode    - What those unique bits should be
//   inst_constructor - A pointer to the static member function
//                      'construct' in the instruction class.
//
// An instruction is decoded by finding a matching entry in
// the instruction_constructor array. A match is defined to
// be:
//    inst_mask & passed_opcode == opcode
// which means that the opcode that is passed to the decoder
// is ANDed with the instruction mask bits and compared to
// the base bits of the opcode. If this test passes, then the
// 'inst_constructor' will be called.

struct instruction_constructor {
  unsigned int inst_mask;
  unsigned int opcode;
  instruction * (*inst_constructor) (Processor *cpu, unsigned int inst, unsigned int address);
};


#endif  /*  __PIC_INSTRUCTIONS_H__ */
