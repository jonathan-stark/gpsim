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


#ifndef __16BIT_INSTRUCTIONS_H__
#define __16BIT_INSTRUCTIONS_H__

#include "14bit-instructions.h"
#include "16bit-registers.h"

/*---------------------------------------------------------
 * 16bit-instructions.h
 *
 * This .h file contains the definitions for the 16-bit core 
 * instructions (the 16bit core of the 18cxxx processors that
 * is). Most of the instructions are derived from the corresponding
 * 12 and 14 bit core instructions. However, the virtual function
 * 'execute' is replaced. This is because the memory addressing and
 * the status register are  different for the 16bit core. The alternative is
 * is to patch the existing instructions with the 16bit stuff.
 * I feel that this is an unwarranted performance hit. So gpsim
 * is slightly bigger, but it's also slightly faster...
 */

//---------------------------------------------------------
class Branching : public instruction
{
public:
  int destination_index;
  unsigned int absolute_destination_index;

  Branching(Processor *new_cpu, unsigned int new_opcode, unsigned int address);

  virtual void execute(){ };
  virtual void debug(){ };
  virtual char *name(char *,int);
  virtual bool isBase() { return true;}

  void decode(Processor *new_cpu, unsigned int new_opcode);

};

//---------------------------------------------------------
class multi_word_instruction : public instruction
{
 public:
  unsigned int word2_opcode;
  unsigned int PMaddress;
  unsigned int PMindex;
  bool initialized;

  multi_word_instruction(Processor *new_cpu, unsigned int new_opcode, unsigned int address);

  virtual int instruction_size() { return 2;}
  virtual enum INSTRUCTION_TYPES isa() {return MULTIWORD_INSTRUCTION;};
  virtual bool isBase() { return true;}

  virtual void initialize(bool init_state) { initialized = init_state; }
};

//---------------------------------------------------------
class multi_word_branch : public multi_word_instruction
{
 public:
  unsigned int destination_index;

  multi_word_branch(Processor *new_cpu, unsigned int new_opcode, unsigned int address);

  void runtime_initialize();
  virtual void execute(){};
  virtual char *name(char *,int);

};

//---------------------------------------------------------
class ADDULNK : public instruction 
{

public:
  ADDULNK(Processor *new_cpu, unsigned int new_opcode,const char *, unsigned int address);
  virtual bool isBase() { return true;}
  virtual void execute();
  virtual char *name(char *,int);
protected:
  unsigned int m_lit;
};

//---------------------------------------------------------
class ADDFSR : public instruction 
{

public:
  ADDFSR(Processor *new_cpu, unsigned int new_opcode,const char *, unsigned int address);
  virtual bool isBase() { return true;}
  virtual void execute();
  virtual char *name(char *,int);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {
    if (((new_opcode>>6)&3) == 3)
    {
      if (new_opcode & 0x100)
	return new ADDULNK(new_cpu,new_opcode,"subulnk", address);
      else
	return new ADDULNK(new_cpu,new_opcode,"addulnk", address);
    }
    if (new_opcode & 0x100)
      return new ADDFSR(new_cpu,new_opcode,"subfsr", address);
    return new ADDFSR(new_cpu,new_opcode,"addfsr", address);
  }
protected:
  unsigned int m_fsr;
  unsigned int m_lit;
  Indirect_Addressing *ia;
};

//---------------------------------------------------------
class CALLW : public instruction
{
public:
  CALLW(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual bool isBase() { return true;}
  virtual void execute();
  virtual char *name(char *,int);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {
    return new CALLW(new_cpu,new_opcode,address);
  }
};
//---------------------------------------------------------
class MOVSF : public multi_word_instruction
{
public:

  MOVSF(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  virtual char *name(char *,int);
  void runtime_initialize();

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new MOVSF(new_cpu,new_opcode,address);}
protected:
  unsigned int source,destination;

};

//---------------------------------------------------------
class PUSHL : public instruction
{
public:
  PUSHL(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual bool isBase() { return true;}
  virtual void execute();
  virtual char *name(char *,int);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {
    return new PUSHL(new_cpu,new_opcode,address);
  }
protected:
  unsigned int m_lit;
};


//---------------------------------------------------------
class ADDLW16 : public ADDLW 
{

public:
  ADDLW16(Processor *new_cpu, unsigned int new_opcode, unsigned int address) :
    ADDLW(new_cpu,  new_opcode,address)
    {};
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new ADDLW16(new_cpu,new_opcode,address);}
};

//---------------------------------------------------------
class ADDWF16 : public ADDWF
{
public:
  int i;

  ADDWF16(Processor *new_cpu, unsigned int new_opcode, unsigned int address) 
    : ADDWF(new_cpu,new_opcode,address){};
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new ADDWF16(new_cpu,new_opcode,address);}
};

//---------------------------------------------------------
class ADDWFC : public Register_op
{
public:

  ADDWFC(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new ADDWFC(new_cpu,new_opcode,address);}
};

//---------------------------------------------------------
class ANDLW16 : public ANDLW 
{

public:
  ANDLW16(Processor *new_cpu, unsigned int new_opcode, unsigned int address) :
    ANDLW(new_cpu,  new_opcode,address)
    {};
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new ANDLW16(new_cpu,new_opcode,address);}
};

//---------------------------------------------------------
class ANDWF16 : public ANDWF
{
public:

  ANDWF16(Processor *new_cpu, unsigned int new_opcode, unsigned int address) 
    : ANDWF(new_cpu,new_opcode,address){};
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new ANDWF16(new_cpu,new_opcode,address);}
};

//---------------------------------------------------------
class BC : public Branching
{
public:

  BC(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new BC(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class BN : public Branching
{
public:

  BN(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new BN(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class BNC : public Branching
{
public:

  BNC(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new BNC(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class BNN : public Branching
{
public:

  BNN(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new BNN(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class BNOV : public Branching
{
public:

  BNOV(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new BNOV(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class BNZ : public Branching
{
public:

  BNZ(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new BNZ(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class BOV : public Branching
{
public:

  BOV(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new BOV(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class BRA : public instruction
{
public:
  int destination_index;
  unsigned int absolute_destination_index;

  BRA(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  virtual char *name(char *,int);
  virtual bool isBase() { return true;}
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new BRA(new_cpu,new_opcode,address);}
};

//---------------------------------------------------------
class BTG : public Bit_op
{
public:

  BTG(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new BTG(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class BZ : public Branching
{
public:

  BZ(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new BZ(new_cpu,new_opcode,address);}

};


//---------------------------------------------------------
class CALL16 : public multi_word_branch
{
public:
  bool fast;

  CALL16(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new CALL16(new_cpu,new_opcode,address);}
  virtual char *name(char *,int);

};


//---------------------------------------------------------
class COMF16 : public COMF
{
public:

  COMF16(Processor *new_cpu, unsigned int new_opcode, unsigned int address) 
    : COMF(new_cpu,new_opcode,address){};
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new COMF16(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class CPFSEQ : public Register_op
{
public:

  CPFSEQ(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new CPFSEQ(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class CPFSGT : public Register_op
{
public:

  CPFSGT(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new CPFSGT(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class CPFSLT : public Register_op
{
public:

  CPFSLT(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new CPFSLT(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class DAW : public instruction
{
public:

  DAW(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  virtual bool isBase() { return true;}
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new DAW(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class DECF16 : public DECF
{
public:

  DECF16(Processor *new_cpu, unsigned int new_opcode, unsigned int address) 
    : DECF(new_cpu,new_opcode,address){};
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new DECF16(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class DECFSZ16 : public DECFSZ
{
public:

  DECFSZ16(Processor *new_cpu, unsigned int new_opcode, unsigned int address) 
    : DECFSZ(new_cpu,new_opcode,address){};
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new DECFSZ16(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class DCFSNZ : public Register_op
{
public:

  DCFSNZ(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new DCFSNZ(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class GOTO16 : public multi_word_branch
{
public:

  GOTO16(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new GOTO16(new_cpu,new_opcode,address);}
};


//---------------------------------------------------------
class INCF16 : public INCF
{
public:

  INCF16(Processor *new_cpu, unsigned int new_opcode, unsigned int address) 
    : INCF(new_cpu,new_opcode,address){};
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new INCF16(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class INCFSZ16 : public INCFSZ
{
public:

  INCFSZ16(Processor *new_cpu, unsigned int new_opcode, unsigned int address) 
    : INCFSZ(new_cpu,new_opcode,address){};
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new INCFSZ16(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class INFSNZ : public Register_op
{
public:

  INFSNZ(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new INFSNZ(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------

class IORLW16 : public IORLW 
{

public:
  IORLW16(Processor *new_cpu, unsigned int new_opcode, unsigned int address) :
    IORLW(new_cpu,  new_opcode,address)
    {};
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new IORLW16(new_cpu,new_opcode,address);}
};

//---------------------------------------------------------
class IORWF16 : public IORWF
{
public:

  IORWF16(Processor *new_cpu, unsigned int new_opcode, unsigned int address) 
    : IORWF(new_cpu,new_opcode,address){};
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new IORWF16(new_cpu,new_opcode,address);}
};

//---------------------------------------------------------
class LCALL16 : public multi_word_branch
{
public:
  bool fast;

  LCALL16(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new LCALL16(new_cpu,new_opcode,address);}
  virtual char *name(char *,int);

};

//---------------------------------------------------------
class LFSR : public multi_word_instruction
{
public:
  unsigned int fsr,k;
  Indirect_Addressing *ia;

  LFSR(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  virtual char *name(char *,int);
  void runtime_initialize();

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new LFSR(new_cpu,new_opcode,address);}
};


//---------------------------------------------------------
class MOVF16 : public MOVF
{
public:

  MOVF16(Processor *new_cpu, unsigned int new_opcode, unsigned int address) 
    : MOVF(new_cpu,new_opcode,address){};
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new MOVF16(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class MOVFF : public multi_word_instruction
{
public:
  unsigned int source,destination;

  MOVFF(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  virtual char *name(char *,int);
  void runtime_initialize();

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new MOVFF(new_cpu,new_opcode,address);}
};


//---------------------------------------------------------
class MOVFP : public multi_word_instruction
{
public:
  unsigned int source,destination;

  MOVFP(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  virtual char *name(char *,int);
  void runtime_initialize();

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new MOVFP(new_cpu,new_opcode,address);}
};


//---------------------------------------------------------

class MOVLB : public Literal_op
{
public:
  MOVLB(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new MOVLB(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------

class MOVLR : public Literal_op
{
public:
  MOVLR(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new MOVLR(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class MOVPF : public multi_word_instruction
{
public:
  unsigned int source,destination;

  MOVPF(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  virtual char *name(char *,int);
  void runtime_initialize();

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new MOVPF(new_cpu,new_opcode,address);}
};


//---------------------------------------------------------
class MOVWF16 : public MOVWF
{
public:

  MOVWF16(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new MOVWF16(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------

class MOVWF16a : public MOVWF
{
public:

  MOVWF16a(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new MOVWF16a(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------

class MULLW : public Literal_op
{
public:
  MULLW(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new MULLW(new_cpu,new_opcode,address);}

};


//---------------------------------------------------------
class MULWF : public Register_op
{
public:

  MULWF(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new MULWF(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class NEGF : public Register_op
{
public:

  NEGF(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new NEGF(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class NEGW : public Register_op
{
public:

  NEGW(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new NEGW(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class POP : public instruction
{
public:

  POP(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  virtual bool isBase() { return true;}
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new POP(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class PUSH : public instruction
{
public:

  PUSH(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  virtual bool isBase() { return true;}
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new PUSH(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class RCALL : public instruction
{
public:
  int destination_index;
  unsigned int absolute_destination_index;

  RCALL(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  virtual char *name(char *,int);
  virtual bool isBase() { return true;}
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new RCALL(new_cpu,new_opcode,address);}
};

//---------------------------------------------------------
class RESET : public instruction
{
public:

  RESET(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  virtual bool isBase() { return true;}
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new RESET(new_cpu,new_opcode,address);}

};



//---------------------------------------------------------
class RETFIE16 : public RETFIE
{
public:
  bool fast;

  RETFIE16(Processor *new_cpu, unsigned int new_opcode, unsigned int address) : 
    RETFIE(new_cpu,new_opcode,address)
    {
      fast = (new_opcode & 1);
    };
  virtual void execute();
  virtual char *name(char *,int);

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new RETFIE16(new_cpu,new_opcode,address);}

};


//---------------------------------------------------------
class RETURN16 : public RETURN
{
public:
  bool fast;

  RETURN16(Processor *new_cpu, unsigned int new_opcode, unsigned int address) : 
    RETURN(new_cpu,new_opcode,address)
    {
      fast = (new_opcode & 1);
    };
  virtual void execute();
  virtual char *name(char *,int);

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new RETURN16(new_cpu,new_opcode,address);}

};


//---------------------------------------------------------
class RLCF : public Register_op
{
public:

  RLCF(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new RLCF(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class RLNCF : public Register_op
{
public:

  RLNCF(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new RLNCF(new_cpu,new_opcode,address);}

};


//---------------------------------------------------------
class RRCF : public Register_op
{
public:

  RRCF(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new RRCF(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class RRNCF : public Register_op
{
public:

  RRNCF(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new RRNCF(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class SETF : public Register_op
{
public:

  SETF(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new SETF(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class SLEEP16 : public SLEEP
{
public:

  SLEEP16(Processor *new_cpu, unsigned int new_opcode, unsigned int address) : 
    SLEEP(new_cpu,new_opcode,address) { };
  virtual void execute();

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new SLEEP16(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class SUBFWB : public Register_op
{
public:

  SUBFWB(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new SUBFWB(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------

class SUBLW16 : public SUBLW
{

public:

  SUBLW16(Processor *new_cpu, unsigned int new_opcode, unsigned int address) : 
    SUBLW(new_cpu,new_opcode,address) { };
  virtual void execute();

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new SUBLW16(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------

class SUBWF16 : public SUBWF
{

public:

  SUBWF16(Processor *new_cpu, unsigned int new_opcode, unsigned int address) : 
    SUBWF(new_cpu,new_opcode,address) { };
  virtual void execute();

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new SUBWF16(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class SUBWFB : public Register_op
{
public:

  SUBWFB(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new SUBWFB(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class TBLRD : public instruction
{
public:

  TBLRD(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  virtual char *name(char *,int);
  virtual bool isBase() { return true;}

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new TBLRD(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class TBLWT : public instruction
{
public:

  TBLWT(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  virtual char *name(char *,int);
  virtual bool isBase() { return true;}

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new TBLWT(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class TLRD : public instruction
{
public:

  TLRD(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  virtual char *name(char *,int);
  virtual bool isBase() { return true;}

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new TLRD(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class TLWT : public instruction
{
public:

  TLWT(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();
  virtual char *name(char *,int);
  virtual bool isBase() { return true;}

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new TLWT(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class TSTFSZ : public Register_op
{
public:

  TSTFSZ(Processor *new_cpu, unsigned int new_opcode, unsigned int address);
  virtual void execute();

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new TSTFSZ(new_cpu,new_opcode,address);}

};

//---------------------------------------------------------
class XORLW16 : public XORLW 
{

public:
  XORLW16(Processor *new_cpu, unsigned int new_opcode, unsigned int address) :
    XORLW(new_cpu,  new_opcode, address)
    {};
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new XORLW16(new_cpu,new_opcode,address);}
};

//---------------------------------------------------------
class XORWF16 : public XORWF
{
public:

  XORWF16(Processor *new_cpu, unsigned int new_opcode, unsigned int address) 
    : XORWF(new_cpu,new_opcode, address){};
  virtual void execute();
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  {return new XORWF16(new_cpu,new_opcode,address);}
};


#endif  /*  __12BIT_INSTRUCTIONS_H__ */
