/*
   Copyright (C) 1998 T. Scott Dattalo
   Copyright (C) 2013 Roy R. Rankin

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

#include<stdio.h>
#include <iostream>
#include <iomanip>

#include "../config.h"
#include "14bit-processors.h"
#include "14bit-instructions.h"


//#define DEBUG
#if defined(DEBUG)
#define Dprintf(arg) {printf("0x%06X %s() ",cycles.get(),__FUNCTION__); printf arg; }
#else
#define Dprintf(arg) {}
#endif
//---------------------------------------------------------
ADDFSR::ADDFSR(Processor *new_cpu, unsigned int new_opcode, const char *pName, unsigned int address)
  : instruction(new_cpu,  new_opcode,address)
{
  m_fsr = (opcode>>6)&1;
  m_lit = opcode & 0x3f;

  switch(m_fsr) {
  case 0:
    ia = &cpu14e->ind0;
    break;

  case 1:
    ia = &cpu14e->ind1;
    break;

  }

  new_name(pName);

}

char *ADDFSR::name(char *return_str,int len)
{

  snprintf(return_str,len,"%s\t%d,0x%x",
	   gpsimObject::name().c_str(),
	   m_fsr,
	   m_lit);

  return(return_str);
}


void ADDFSR::execute()
{
  ia->put_fsr(ia->get_fsr_value() + m_lit);  //ADDFSR
  cpu_pic->pc->increment();
}
//--------------------------------------------------

ADDLW::ADDLW (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Literal_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("addlw");
}

void ADDLW::execute(void)
{
  unsigned int old_value,new_value;

  new_value = (old_value = cpu14->W->value.get()) + L;

  cpu14->W->put(new_value & 0xff);
  cpu14->status->put_Z_C_DC(new_value, old_value, L);

  cpu14->pc->increment();

}


//---------------------------------------------------------
MOVIW::MOVIW(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : instruction(new_cpu,  new_opcode,address)
{
  if (opcode & 0x3f00)	// Index indirect
  {
      m_fsr = (opcode>>6)&1;
      m_lit = opcode & 0x3f;
      if (m_lit & 0x20) m_lit -= 0x40;
      m_op = DELTA;
      Dprintf((" shift op %x fsr %d data %d raw %d\n", opcode>>6, m_fsr, m_lit, opcode &0x3f));

  }
  else
  {
      m_fsr = (opcode>>2)&1;
      m_op = opcode & 0x3;
  }

  switch(m_fsr) {
  case 0:
    ia = &cpu14e->ind0;
    break;

  case 1:
    ia = &cpu14e->ind1;
    break;
  }

  new_name("moviw");

}

char *MOVIW::name(char *return_str,int len)
{

  switch(m_op)
  {
  case PREINC:
	snprintf(return_str,len,"%s\t++FSR%d",
	   gpsimObject::name().c_str(),
	   m_fsr);
	break;

  case PREDEC:
	snprintf(return_str,len,"%s\t--FSR%d",
	   gpsimObject::name().c_str(),
	   m_fsr);
	break;

  case POSTINC:
	snprintf(return_str,len,"%s\tFSR%d++",
	   gpsimObject::name().c_str(),
	   m_fsr);
	break;

  case POSTDEC:
	snprintf(return_str,len,"%s\tFSR%d--",
	   gpsimObject::name().c_str(),
	   m_fsr);
	break;

  case DELTA:
	snprintf(return_str,len,"%s\t%d[FSR%d]",
	   gpsimObject::name().c_str(), m_lit, 
	   m_fsr);
	break;
  }

  return(return_str);
}


void MOVIW::execute()
{
  if (m_op == PREINC)
  {
    ia->put_fsr(ia->fsr_value + 1);
    cpu14->W->put(ia->indf.get());
  }
  else if (m_op == PREDEC)
  {
    ia->put_fsr(ia->fsr_value - 1);
    cpu14->W->put(ia->indf.get());
  }
  else if (m_op == POSTINC)
  {
    cpu14->W->put(ia->indf.get());
    ia->put_fsr(ia->fsr_value + 1);
  }
  else if (m_op == POSTDEC)
  {
    cpu14->W->put(ia->indf.get());
    ia->put_fsr(ia->fsr_value - 1);
  }
  else if (m_op == DELTA)
  {
	ia->fsr_delta = m_lit;
        cpu14->W->put(ia->indf.get());
  }
  cpu_pic->pc->increment();
}
//---------------------------------------------------------
MOVWI::MOVWI(Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : instruction(new_cpu,  new_opcode,address)
{
  if (opcode & 0x3f00)	// Index indirect
  {
      m_fsr = (opcode>>6)&1;
      m_lit = opcode & 0x3f;
      if (m_lit & 0x20) m_lit -= 0x40;
      m_op = DELTA;
      Dprintf((" shift op %x fsr %d data %d\n", opcode>>6, m_fsr, m_lit));

  }
  else
  {
      m_fsr = (opcode>>2)&1;
      m_op = opcode & 0x3;
  }

  switch(m_fsr) {
  case 0:
    ia = &cpu14e->ind0;
    break;

  case 1:
    ia = &cpu14e->ind1;
    break;
  }

  new_name("movwi");

}

char *MOVWI::name(char *return_str,int len)
{

  switch(m_op)
  {
  case PREINC:
	snprintf(return_str,len,"%s\t++FSR%d",
	   gpsimObject::name().c_str(),
	   m_fsr);
	break;

  case PREDEC:
	snprintf(return_str,len,"%s\t--FSR%d",
	   gpsimObject::name().c_str(),
	   m_fsr);
	break;

  case POSTINC:
	snprintf(return_str,len,"%s\tFSR%d++",
	   gpsimObject::name().c_str(),
	   m_fsr);
	break;

  case POSTDEC:
	snprintf(return_str,len,"%s\tFSR%d--",
	   gpsimObject::name().c_str(),
	   m_fsr);
	break;

  case DELTA:
	snprintf(return_str,len,"%s\t%d[FSR%d]",
	   gpsimObject::name().c_str(), m_lit, 
	   m_fsr);
	break;
  }

  return(return_str);
}


void MOVWI::execute()
{
  if (m_op == PREINC)
  {
    ia->put_fsr(ia->fsr_value + 1);
    ia->indf.put(cpu14->W->get());
  }
  else if (m_op == PREDEC)
  {
    ia->put_fsr(ia->fsr_value - 1);
    ia->indf.put(cpu14->W->get());
  }
  else if (m_op == POSTINC)
  {
    ia->indf.put(cpu14->W->get());
    ia->put_fsr(ia->fsr_value + 1);
  }
  else if (m_op == POSTDEC)
  {
    ia->indf.put(cpu14->W->get());
    ia->put_fsr(ia->fsr_value - 1);
  }
  else if (m_op == DELTA)
  {
    Dprintf((" DELTA fsr %d delta %d\n", m_fsr, m_lit));
    ia->fsr_delta = m_lit;
    ia->indf.put(cpu14->W->get());
  }
  cpu_pic->pc->increment();
}
//--------------------------------------------------

MOVLB::MOVLB (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Literal_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("movlb");
}

void MOVLB::execute()
{
  cpu14e->bsr.put(L);

  cpu_pic->pc->increment();

}


//--------------------------------------------------

RETFIE::RETFIE (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : instruction(new_cpu,new_opcode,address)
{
  decode(new_cpu, new_opcode);
  new_name("retfie");
}

void RETFIE::execute(void)
{
  cpu14->pc->new_address(cpu14->stack->pop());
  cpu14->intcon->set_gie();
  if(cpu_pic->base_isa() == _14BIT_E_PROCESSOR_)
  {
	cpu14e->status->value = cpu14e->status_shad.value;
	cpu14e->W->value = cpu14e->wreg_shad.value;
	cpu14e->bsr.value = cpu14e->bsr_shad.value;
	cpu14e->pclath->value = cpu14e->pclath_shad.value;
	cpu14e->ind0.fsrl.value = cpu14e->fsr0l_shad.value;
	cpu14e->ind0.fsrh.value = cpu14e->fsr0h_shad.value;
	cpu14e->ind1.fsrl.value = cpu14e->fsr1l_shad.value;
	cpu14e->ind1.fsrh.value = cpu14e->fsr1h_shad.value;
  }
}

//--------------------------------------------------

RETURN::RETURN (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : instruction(new_cpu,new_opcode,address)
{
  decode(new_cpu, new_opcode);
  new_name("return");
}

void RETURN::execute(void)
{
  cpu14->pc->new_address(cpu14->stack->pop());
}

//--------------------------------------------------

SUBLW::SUBLW (Processor *new_cpu, unsigned int new_opcode, unsigned int address)
  : Literal_op(new_cpu, new_opcode, address)
{
  decode(new_cpu, new_opcode);
  new_name("sublw");
}

void SUBLW::execute(void)
{
  unsigned int old_value,new_value;

  new_value = L - (old_value = cpu14->W->value.get());

  cpu14->W->put(new_value & 0xff);

  cpu14->status->put_Z_C_DC_for_sub(new_value, old_value, L);

  cpu14->pc->increment();

}

