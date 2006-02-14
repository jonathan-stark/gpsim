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

  virtual void execute(void){ };
  virtual void debug(void){ };
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

  virtual int instruction_size(void) { return 2;}
  virtual enum INSTRUCTION_TYPES isa(void) {return MULTIWORD_INSTRUCTION;};
  virtual bool isBase() { return true;}

  virtual void initialize(bool init_state) { initialized = init_state; }
};

//---------------------------------------------------------
class multi_word_branch : public multi_word_instruction
{
 public:
  unsigned int destination_index;

  void runtime_initialize(void);
  virtual void execute(void){};
  virtual char *name(char *,int);

};

//---------------------------------------------------------
class ADDULNK : public instruction 
{

public:
  ADDULNK(Processor *new_cpu, unsigned int new_opcode,const char *);
  virtual bool isBase() { return true;}
  virtual void execute(void);
  virtual char *name(char *,int);
protected:
  unsigned int m_lit;
};

//---------------------------------------------------------
class ADDFSR : public instruction 
{

public:
  ADDFSR(Processor *new_cpu, unsigned int new_opcode,const char *);
  virtual bool isBase() { return true;}
  virtual void execute(void);
  virtual char *name(char *,int);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
  {
    if (((new_opcode>>6)&3) == 3)
      if (new_opcode & 0x100)
	return new ADDULNK(new_cpu,new_opcode,"subulnk");
      else
	return new ADDULNK(new_cpu,new_opcode,"addulnk");
    if (new_opcode & 0x100)
      return new ADDFSR(new_cpu,new_opcode,"subfsr");
    return new ADDFSR(new_cpu,new_opcode,"addfsr");
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
  CALLW(Processor *new_cpu, unsigned int new_opcode);
  virtual bool isBase() { return true;}
  virtual void execute(void);
  virtual char *name(char *,int);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
  {
    return new CALLW(new_cpu,new_opcode);
  }
};
//---------------------------------------------------------
class MOVSF : public multi_word_instruction
{
public:

  MOVSF(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  virtual char *name(char *,int);
  void runtime_initialize(void);

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new MOVSF(new_cpu,new_opcode);}
protected:
  unsigned int source,destination;

};

//---------------------------------------------------------
class PUSHL : public instruction
{
public:
  PUSHL(Processor *new_cpu, unsigned int new_opcode);
  virtual bool isBase() { return true;}
  virtual void execute(void);
  virtual char *name(char *,int);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
  {
    return new PUSHL(new_cpu,new_opcode);
  }
protected:
  unsigned int m_lit;
};


//---------------------------------------------------------
class ADDLW16 : public ADDLW 
{

public:
  ADDLW16(Processor *new_cpu, unsigned int new_opcode) :
    ADDLW(new_cpu,  new_opcode)
    {};
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new ADDLW16(new_cpu,new_opcode);}
};

//---------------------------------------------------------
class ADDWF16 : public ADDWF
{
public:
  int i;

  ADDWF16(Processor *new_cpu, unsigned int new_opcode) : ADDWF(new_cpu,new_opcode){};
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new ADDWF16(new_cpu,new_opcode);}
};

//---------------------------------------------------------
class ADDWFC : public Register_op
{
public:

  ADDWFC(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new ADDWFC(new_cpu,new_opcode);}
};

//---------------------------------------------------------
class ANDLW16 : public ANDLW 
{

public:
  ANDLW16(Processor *new_cpu, unsigned int new_opcode) :
    ANDLW(new_cpu,  new_opcode)
    {};
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new ANDLW16(new_cpu,new_opcode);}
};

//---------------------------------------------------------
class ANDWF16 : public ANDWF
{
public:

  ANDWF16(Processor *new_cpu, unsigned int new_opcode) : ANDWF(new_cpu,new_opcode){};
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new ANDWF16(new_cpu,new_opcode);}
};

//---------------------------------------------------------
class BC : public Branching
{
public:

  BC(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new BC(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class BN : public Branching
{
public:

  BN(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new BN(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class BNC : public Branching
{
public:

  BNC(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new BNC(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class BNN : public Branching
{
public:

  BNN(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new BNN(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class BNOV : public Branching
{
public:

  BNOV(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new BNOV(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class BNZ : public Branching
{
public:

  BNZ(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new BNZ(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class BOV : public Branching
{
public:

  BOV(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new BOV(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class BRA : public instruction
{
public:
  int destination_index;
  unsigned int absolute_destination_index;

  BRA(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  virtual char *name(char *,int);
  virtual bool isBase() { return true;}
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new BRA(new_cpu,new_opcode);}
};

//---------------------------------------------------------
class BTG : public Bit_op
{
public:

  BTG(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new BTG(new_cpu,new_opcode);}


};

//---------------------------------------------------------
class BZ : public Branching
{
public:

  BZ(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new BZ(new_cpu,new_opcode);}

};


//---------------------------------------------------------
class CALL16 : public multi_word_branch
{
public:
  bool fast;

  CALL16(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new CALL16(new_cpu,new_opcode);}
  virtual char *name(char *,int);

};


//---------------------------------------------------------
class COMF16 : public COMF
{
public:

  COMF16(Processor *new_cpu, unsigned int new_opcode) : COMF(new_cpu,new_opcode){};
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new COMF16(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class CPFSEQ : public Register_op
{
public:

  CPFSEQ(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new CPFSEQ(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class CPFSGT : public Register_op
{
public:

  CPFSGT(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new CPFSGT(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class CPFSLT : public Register_op
{
public:

  CPFSLT(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new CPFSLT(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class DAW : public instruction
{
public:

  DAW(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  virtual bool isBase() { return true;}
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new DAW(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class DECF16 : public DECF
{
public:

  DECF16(Processor *new_cpu, unsigned int new_opcode) : DECF(new_cpu,new_opcode){};
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new DECF16(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class DECFSZ16 : public DECFSZ
{
public:

  DECFSZ16(Processor *new_cpu, unsigned int new_opcode) : DECFSZ(new_cpu,new_opcode){};
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new DECFSZ16(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class DCFSNZ : public Register_op
{
public:

  DCFSNZ(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new DCFSNZ(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class GOTO16 : public multi_word_branch
{
public:

  GOTO16(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new GOTO16(new_cpu,new_opcode);}
};


//---------------------------------------------------------
class INCF16 : public INCF
{
public:

  INCF16(Processor *new_cpu, unsigned int new_opcode) : INCF(new_cpu,new_opcode){};
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new INCF16(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class INCFSZ16 : public INCFSZ
{
public:

  INCFSZ16(Processor *new_cpu, unsigned int new_opcode) : INCFSZ(new_cpu,new_opcode){};
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new INCFSZ16(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class INFSNZ : public Register_op
{
public:

  INFSNZ(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new INFSNZ(new_cpu,new_opcode);}

};

//---------------------------------------------------------

class IORLW16 : public IORLW 
{

public:
  IORLW16(Processor *new_cpu, unsigned int new_opcode) :
    IORLW(new_cpu,  new_opcode)
    {};
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new IORLW16(new_cpu,new_opcode);}
};

//---------------------------------------------------------
class IORWF16 : public IORWF
{
public:

  IORWF16(Processor *new_cpu, unsigned int new_opcode) : IORWF(new_cpu,new_opcode){};
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new IORWF16(new_cpu,new_opcode);}
};

//---------------------------------------------------------
class LCALL16 : public multi_word_branch
{
public:
  bool fast;

  LCALL16(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new LCALL16(new_cpu,new_opcode);}
  virtual char *name(char *,int);

};

//---------------------------------------------------------
class LFSR : public multi_word_instruction
{
public:
  unsigned int fsr,k;
  Indirect_Addressing *ia;

  LFSR(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  virtual char *name(char *,int);
  void runtime_initialize(void);

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new LFSR(new_cpu,new_opcode);}
};


//---------------------------------------------------------
class MOVF16 : public MOVF
{
public:

  MOVF16(Processor *new_cpu, unsigned int new_opcode) : MOVF(new_cpu,new_opcode){};
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new MOVF16(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class MOVFF : public multi_word_instruction
{
public:
  unsigned int source,destination;

  MOVFF(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  virtual char *name(char *,int);
  void runtime_initialize(void);

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new MOVFF(new_cpu,new_opcode);}
};


//---------------------------------------------------------
class MOVFP : public multi_word_instruction
{
public:
  unsigned int source,destination;

  MOVFP(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  virtual char *name(char *,int);
  void runtime_initialize(void);

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new MOVFP(new_cpu,new_opcode);}
};


//---------------------------------------------------------

class MOVLB : public Literal_op
{
public:
  MOVLB(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new MOVLB(new_cpu,new_opcode);}

};

//---------------------------------------------------------

class MOVLR : public Literal_op
{
public:
  MOVLR(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new MOVLR(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class MOVPF : public multi_word_instruction
{
public:
  unsigned int source,destination;

  MOVPF(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  virtual char *name(char *,int);
  void runtime_initialize(void);

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new MOVPF(new_cpu,new_opcode);}
};


//---------------------------------------------------------
class MOVWF16 : public MOVWF
{
public:

  MOVWF16(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new MOVWF16(new_cpu,new_opcode);}

};

//---------------------------------------------------------

class MOVWF16a : public MOVWF
{
public:

  MOVWF16a(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new MOVWF16a(new_cpu,new_opcode);}

};

//---------------------------------------------------------

class MULLW : public Literal_op
{
public:
  MULLW(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new MULLW(new_cpu,new_opcode);}

};


//---------------------------------------------------------
class MULWF : public Register_op
{
public:

  MULWF(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new MULWF(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class NEGF : public Register_op
{
public:

  NEGF(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new NEGF(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class NEGW : public Register_op
{
public:

  NEGW(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new NEGW(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class POP : public instruction
{
public:

  POP(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  virtual bool isBase() { return true;}
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new POP(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class PUSH : public instruction
{
public:

  PUSH(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  virtual bool isBase() { return true;}
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new PUSH(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class RCALL : public instruction
{
public:
  int destination_index;
  unsigned int absolute_destination_index;

  RCALL(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  virtual char *name(char *,int);
  virtual bool isBase() { return true;}
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new RCALL(new_cpu,new_opcode);}
};

//---------------------------------------------------------
class RESET : public instruction
{
public:

  RESET(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  virtual bool isBase() { return true;}
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new RESET(new_cpu,new_opcode);}

};



//---------------------------------------------------------
class RETFIE16 : public RETFIE
{
public:
  bool fast;

  RETFIE16(Processor *new_cpu, unsigned int new_opcode) : 
    RETFIE(new_cpu,new_opcode)
    {
      fast = (new_opcode & 1);
    };
  virtual void execute(void);
  virtual char *name(char *,int);

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new RETFIE16(new_cpu,new_opcode);}

};


//---------------------------------------------------------
class RETURN16 : public RETURN
{
public:
  bool fast;

  RETURN16(Processor *new_cpu, unsigned int new_opcode) : 
    RETURN(new_cpu,new_opcode)
    {
      fast = (new_opcode & 1);
    };
  virtual void execute(void);
  virtual char *name(char *,int);

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new RETURN16(new_cpu,new_opcode);}

};


//---------------------------------------------------------
class RLCF : public Register_op
{
public:

  RLCF(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new RLCF(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class RLNCF : public Register_op
{
public:

  RLNCF(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new RLNCF(new_cpu,new_opcode);}

};


//---------------------------------------------------------
class RRCF : public Register_op
{
public:

  RRCF(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new RRCF(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class RRNCF : public Register_op
{
public:

  RRNCF(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new RRNCF(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class SETF : public Register_op
{
public:

  SETF(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new SETF(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class SLEEP16 : public SLEEP
{
public:

  SLEEP16(Processor *new_cpu, unsigned int new_opcode) : 
    SLEEP(new_cpu,new_opcode) { };
  virtual void execute(void);

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new SLEEP16(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class SUBFWB : public Register_op
{
public:

  SUBFWB(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new SUBFWB(new_cpu,new_opcode);}

};

//---------------------------------------------------------

class SUBLW16 : public SUBLW
{

public:

  SUBLW16(Processor *new_cpu, unsigned int new_opcode) : 
    SUBLW(new_cpu,new_opcode) { };
  virtual void execute(void);

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new SUBLW16(new_cpu,new_opcode);}

};

//---------------------------------------------------------

class SUBWF16 : public SUBWF
{

public:

  SUBWF16(Processor *new_cpu, unsigned int new_opcode) : 
    SUBWF(new_cpu,new_opcode) { };
  virtual void execute(void);

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new SUBWF16(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class SUBWFB : public Register_op
{
public:

  SUBWFB(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new SUBWFB(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class TBLRD : public instruction
{
public:

  TBLRD(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  virtual char *name(char *,int);
  virtual bool isBase() { return true;}

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new TBLRD(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class TBLWT : public instruction
{
public:

  TBLWT(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  virtual char *name(char *,int);
  virtual bool isBase() { return true;}

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new TBLWT(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class TLRD : public instruction
{
public:

  TLRD(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  virtual char *name(char *,int);
  virtual bool isBase() { return true;}

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new TLRD(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class TLWT : public instruction
{
public:

  TLWT(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);
  virtual char *name(char *,int);
  virtual bool isBase() { return true;}

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new TLWT(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class TSTFSZ : public Register_op
{
public:

  TSTFSZ(Processor *new_cpu, unsigned int new_opcode);
  virtual void execute(void);

  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new TSTFSZ(new_cpu,new_opcode);}

};

//---------------------------------------------------------
class XORLW16 : public XORLW 
{

public:
  XORLW16(Processor *new_cpu, unsigned int new_opcode) :
    XORLW(new_cpu,  new_opcode)
    {};
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new XORLW16(new_cpu,new_opcode);}
};

//---------------------------------------------------------
class XORWF16 : public XORWF
{
public:

  XORWF16(Processor *new_cpu, unsigned int new_opcode) : XORWF(new_cpu,new_opcode){};
  virtual void execute(void);
  static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new XORWF16(new_cpu,new_opcode);}
};


#endif  /*  __12BIT_INSTRUCTIONS_H__ */
