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
#include <glib.h>
#endif

class XrefObject;
class pic_processor;
class file_register;

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
  pic_processor *cpu;     /* A pointer to the microcontroller to which this
                           *  instruction belongs  */
  int file_id;            /* The source file that declared this instruction
			   * (The file_id is an index into an array of files) */
  int src_line;           /* The line number within the source file */
  int lst_line;           /* The line number within the list file */

  instruction(void);
  virtual void execute(void){ };
  virtual void debug(void){ };
  virtual char *name(char *str){ return(name_str);};
  virtual int instruction_size(void) { return 1;};
  virtual unsigned int get_opcode(void) { return opcode; };
  virtual int get_src_line(void) { return(src_line); };
  virtual int get_lst_line(void) { return(lst_line); };
  virtual int get_file_id(void) {return(file_id); };
  virtual INSTRUCTION_TYPES isa(void) {return NORMAL_INSTRUCTION;};
  void decode(pic_processor *new_cpu, unsigned int new_opcode){cpu = new_cpu;opcode=new_opcode;};
  void add_line_number_symbol(int address);
  void update_line_number(int file, int sline, int lline);

  //#ifdef HAVE_GUI
  // If we are linking with a gui, then here are a
  // few declarations that are used to send data to it.
  // This is essentially a singly-linked list of pointers
  // to structures. The structures provide information
  // such as where the instruction is located (in the gui), 
  // the type of window it's in, and also the way in which
  // the data is presented

  XrefObject *xref;
  
//    GSList *gui_xref;
//    virtual void assign_xref(gpointer);
//    virtual GSList *get_gui_xref(void) {return gui_xref;};
  //#endif

};


//---------------------------------------------------------
class invalid_instruction : public instruction
{
public:

  virtual void execute(void)
  { 
    //cout << "*** INVALID INSTRUCTION ***\n";
    #ifdef __DEBUG_VERBOSE__
      debug();
    #endif
  };
  virtual void debug(void)
  {
    //cout << "*** INVALID INSTRUCTION ***\n";
  };

  invalid_instruction(pic_processor *new_cpu=NULL,unsigned int new_opcode=0)
    {
      cpu=new_cpu;
      opcode=new_opcode;
    }
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

  void decode(pic_processor *new_cpu, unsigned int new_opcode)
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
  file_register *reg;

  virtual void execute(void){ };
  virtual void debug(void){ };
  virtual char *name(char *);

  void decode(pic_processor *new_cpu, unsigned int new_opcode);

};


//---------------------------------------------------------
class Register_op : public instruction
{
public:

  static file_register *source;
  unsigned int register_address;
  bool destination, access;

  /*  file_register *destination;*/

  virtual void execute(void){ };
  virtual void debug(void){ };
  virtual char *name(char *);

  void decode(pic_processor *new_cpu, unsigned int new_opcode);

};




//---------------------------------------------------------
extern invalid_instruction bad_instruction;



#endif  /*  __PIC_INSTRUCTIONS_H__ */
