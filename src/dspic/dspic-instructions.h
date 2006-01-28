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

#if !defined(__DSPIC_INSTRUCTIONS_H__)
#define __DSPIC_INSTRUCTIONS_H__

#include "../pic-instructions.h"

namespace dspic_instructions
{

  //----------------------------------------------------------------------
  class ADD : public instruction
  {
  public:
    ADD(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new ADD(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class ADDC : public instruction
  {
  public:
    ADDC(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new ADDC(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class AND : public instruction
  {
  public:
    AND(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new AND(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class ASR : public instruction
  {
  public:
    ASR(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new ASR(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class BCLR : public instruction
  {
  public:
    BCLR(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new BCLR(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class BRA : public instruction
  {
  public:
    BRA(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new BRA(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class BSET : public instruction
  {
  public:
    BSET(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new BSET(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class BSW : public instruction
  {
  public:
    BSW(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new BSW(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class BTG : public instruction
  {
  public:
    BTG(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new BTG(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class BTS : public instruction
  {
  public:
    BTS(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new BTS(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class BTST : public instruction
  {
  public:
    BTST(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new BTST(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class BTSTS : public instruction
  {
  public:
    BTSTS(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new BTSTS(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class CALL : public instruction
  {
  public:
    CALL(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new CALL(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class CLR : public instruction
  {
  public:
    CLR(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new CLR(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class CLRWDT : public instruction
  {
  public:
    CLRWDT(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new CLRWDT(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class COM : public instruction
  {
  public:
    COM(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new COM(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class CP : public instruction
  {
  public:
    CP(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new CP(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class CP0 : public instruction
  {
  public:
    CP0(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new CP0(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class CPB : public instruction
  {
  public:
    CPB(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new CPB(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class CPS : public instruction
  {
  public:
    CPS(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new CPS(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class DAW : public instruction
  {
  public:
    DAW(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new DAW(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class DEC : public instruction
  {
  public:
    DEC(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new DEC(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class DISI : public instruction
  {
  public:
    DISI(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new DISI(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class DIV : public instruction
  {
  public:
    DIV(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new DIV(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class DO : public instruction
  {
  public:
    DO(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new DO(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class ED : public instruction
  {
  public:
    ED(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new ED(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class EXCH : public instruction
  {
  public:
    EXCH(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new EXCH(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class FB : public instruction
  {
  public:
    FB(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new FB(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class GOTO : public instruction
  {
  public:
    GOTO(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new GOTO(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class INC : public instruction
  {
  public:
    INC(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new INC(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class IOR : public instruction
  {
  public:
    IOR(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new IOR(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class LAC : public instruction
  {
  public:
    LAC(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new LAC(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class LNK : public instruction
  {
  public:
    LNK(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new LNK(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class LSR : public instruction
  {
  public:
    LSR(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new LSR(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class MAC : public instruction
  {
  public:
    MAC(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new MAC(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class MOV : public instruction
  {
  public:
    MOV(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new MOV(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class MOVSAC : public instruction
  {
  public:
    MOVSAC(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new MOVSAC(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class MPY : public instruction
  {
  public:
    MPY(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new MPY(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class MUL : public instruction
  {
  public:
    MUL(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new MUL(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class NEG : public instruction
  {
  public:
    NEG(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new NEG(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class NOP : public instruction
  {
  public:
    NOP(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new NOP(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class POP : public instruction
  {
  public:
    POP(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new POP(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class PUSH : public instruction
  {
  public:
    PUSH(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new PUSH(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class PWRSAV : public instruction
  {
  public:
    PWRSAV(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new PWRSAV(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class RCALL : public instruction
  {
  public:
    RCALL(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new RCALL(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class REPEAT : public instruction
  {
  public:
    REPEAT(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new REPEAT(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class RESET : public instruction
  {
  public:
    RESET(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new RESET(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class RET : public instruction
  {
  public:
    RET(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new RET(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class ROT : public instruction
  {
  public:
    ROT(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new ROT(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class SAC : public instruction
  {
  public:
    SAC(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new SAC(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class SE : public instruction
  {
  public:
    SE(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new SE(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class SETM : public instruction
  {
  public:
    SETM(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new SETM(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class SFTAC : public instruction
  {
  public:
    SFTAC(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new SFTAC(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class SL : public instruction
  {
  public:
    SL(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new SL(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class SUB : public instruction
  {
  public:
    SUB(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new SUB(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class SWAP : public instruction
  {
  public:
    SWAP(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new SWAP(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class TBLRD : public instruction
  {
  public:
    TBLRD(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new TBLRD(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class TBLWT : public instruction
  {
  public:
    TBLWT(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new TBLWT(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class ULNK : public instruction
  {
  public:
    ULNK(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new ULNK(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class XOR : public instruction
  {
  public:
    XOR(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new XOR(new_cpu,new_opcode);}
  };

  //----------------------------------------------------------------------
  class ZE : public instruction
  {
  public:
    ZE(Processor *new_cpu, unsigned int new_opcode);
    virtual void execute();
    virtual bool isBase() { return true;}
    static instruction *construct(Processor *new_cpu, unsigned int new_opcode)
    {return new ZE(new_cpu,new_opcode);}
  };

};

#endif // __DSPIC_INSTRUCTIONS_H__
