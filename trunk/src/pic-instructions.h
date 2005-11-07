/*
   Copyright (C) 1998 T. Scott Dattalo

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


#ifndef __PIC_INSTRUCTIONS_H__
#define __PIC_INSTRUCTIONS_H__


#ifdef HAVE_GUI
#include <unistd.h>
#include <glib.h>
#endif

#include "value.h"

class Register;

/*
 *  base class for an instruction
 */

class instruction : public gpsimValue
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


  instruction();
  instruction(Processor *pProcessor, unsigned int uOpCode, unsigned int uAddrOfInstr);
  void Initialize(Processor *pProcessor, unsigned int uOpCode, unsigned int uAddrOfInstr);

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
  virtual int get_lst_line() { return(lst_line); }
  virtual int get_file_id() {return(file_id); }
  virtual int get_hll_file_id() {return(hll_file_id); }
  virtual INSTRUCTION_TYPES isa() {return NORMAL_INSTRUCTION;}
  virtual guint64 getCyclesUsed() { return cycle_count;}
  virtual bool isBase() = 0;
  void decode(Processor *new_cpu, unsigned int new_opcode);
  void add_line_number_symbol(int address);
  void update_line_number(int file, int sline, int lline, int hllfile, int hllsline);

  virtual char *ReadSrcLine(char *buf, int nBytes);
  virtual char *ReadLstLine(char *buf, int nBytes);
  virtual char *ReadHLLLine(char *buf, int nBytes);

  // Some instructions require special initialization after they've
  // been instantiated. For those that do, the instruction base class
  // provides a way to control the initialization state (see the 16-bit
  // PIC instructions).
  virtual void initialize(bool init_state) {};

  bool bIsModified() { return m_bIsModified; }
  void setModified(bool b) { m_bIsModified=b; }
protected:
  bool m_bIsModified; // flag indicating if this instruction has
                      // changed since start.
  guint64 cycle_count; // Nr of cycles used up by this instruction

  unsigned int opcode;
  unsigned int m_uAddrOfInstr;
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
  virtual INSTRUCTION_TYPES isa();
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

  invalid_instruction(Processor *new_cpu=0,unsigned int new_opcode=0);
  invalid_instruction(Processor *new_cpu,unsigned int address, 
    unsigned int new_opcode);
  virtual INSTRUCTION_TYPES isa() {return INVALID_INSTRUCTION;};
  //virtual char *name(char *str){return("INVALID");};
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new invalid_instruction(new_cpu,new_opcode);}

  virtual bool isBase() { return true; }

};

//---------------------------------------------------------
class Literal_op : public instruction
{
public:
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

  static Register *source;
  unsigned int register_address;
  bool destination, access;

  /*  Register *destination;*/

  virtual void debug(){ };
  virtual char *name(char *,int);
  virtual bool isBase() { return true; }

  void decode(Processor *new_cpu, unsigned int new_opcode);

};




//---------------------------------------------------------
extern invalid_instruction bad_instruction;

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
  instruction * (*inst_constructor) (Processor *cpu, unsigned int inst);
};


#endif  /*  __PIC_INSTRUCTIONS_H__ */
