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
along with gpasm; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */


#ifndef __PIC_INSTRUCTIONS_H__
#define __PIC_INSTRUCTIONS_H__


#ifdef HAVE_GUI
#include <unistd.h>
#include <glib.h>
#endif

class XrefObject;
class pic_processor;
class Register;

/*
 *  base class for an instruction
 */

class instruction
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
    MULTIWORD_INSTRUCTION
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


  char name_str[20];      /* %%% FIX ME %%% dyanmically allocate? */
  unsigned int opcode;
  Processor *cpu;     /* A pointer to the microcontroller to which this
                           *  instruction belongs  */
  int file_id;            /* The source file that declared this instruction
			   * (The file_id is an index into an array of files) */
  int hll_file_id;        /* The hll source file that declared this instruction */
  int src_line;           /* The line number within the source file */
  int lst_line;           /* The line number within the list file */
  int hll_src_line;       /* The line number within the HLL source file */

  instruction(void);
  virtual void execute(void){ };
  virtual void debug(void){ };
  virtual char *name(char *str){ return(name_str);};
  virtual int instruction_size(void) { return 1;};
  virtual unsigned int get_opcode(void) { return opcode; };
  virtual int get_src_line(void) { return(src_line); };
  virtual int get_hll_src_line(void) { return(hll_src_line); };
  virtual int get_lst_line(void) { return(lst_line); };
  virtual int get_file_id(void) {return(file_id); };
  virtual int get_hll_file_id(void) {return(hll_file_id); };
  virtual INSTRUCTION_TYPES isa(void) {return NORMAL_INSTRUCTION;};
  void decode(Processor *new_cpu, unsigned int new_opcode){cpu = new_cpu;opcode=new_opcode;};
  void add_line_number_symbol(int address);
  void update_line_number(int file, int sline, int lline, int hllfile, int hllsline);

  // Some instructions require special initialization after they've
  // been instantiated. For those that do, the instruction base class
  // provides a way to control the initialization state (see the 16-bit
  // PIC instructions).
  virtual void initialize(bool init_state) {};

  int is_modified; // flag indicating if this instruction has
                   // changed since start.

  guint64 cycle_count; // Nr of cycles used up by this instruction


  // xref - a Cross reference object.
  // External applications like the gui can register call back functions
  // through the xref object. 

  XrefObject *xref;

};


//---------------------------------------------------------
class invalid_instruction : public instruction
{
public:

  virtual void execute(void);

  virtual void debug(void)
  {
    //cout << "*** INVALID INSTRUCTION ***\n";
  };

  invalid_instruction(pic_processor *new_cpu=NULL,unsigned int new_opcode=0);
  virtual INSTRUCTION_TYPES isa(void) {return INVALID_INSTRUCTION;};
  virtual char *name(char *str){return("INVALID");};
  static instruction *construct(pic_processor *new_cpu, unsigned int new_opcode)
    {return new invalid_instruction(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class Literal_op : public instruction
{
public:
  unsigned int L;

  virtual void execute(void){ };
  virtual void debug(void){ };
  virtual char *name(char *);

  void decode(Processor *new_cpu, unsigned int new_opcode)
    {
      opcode = new_opcode;
      cpu = new_cpu;
      L = opcode & 0xff;
    }
};


//---------------------------------------------------------
class Bit_op : public instruction
{
public:
  unsigned int mask,register_address;
  bool access;
  Register *reg;

  virtual void execute(void){ };
  virtual void debug(void){ };
  virtual char *name(char *);

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

  virtual void execute(void){ };
  virtual void debug(void){ };
  virtual char *name(char *);

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
  instruction * (*inst_constructor) (pic_processor *cpu, unsigned int inst);
};


#endif  /*  __PIC_INSTRUCTIONS_H__ */
