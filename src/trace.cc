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

#include <iostream.h>
#include <iomanip.h>

#include "../config.h"
#include "pic-processor.h"
#include "14bit-processors.h"
#include "trace.h"
#include "trace_orb.h"

#define MODE "0x" << hex

Trace trace;     // Instantiate the trace buffer class

static unsigned int trace_flag=0xffffffff;  // Trace everything

#define TRACE_INSTRUCTION       (1<< (INSTRUCTION >> 24))
#define TRACE_PROGRAM_COUNTER   (1<< (PROGRAM_COUNTER >> 24))

Trace::Trace(void)
{

  for(trace_index = 0; trace_index < TRACE_BUFFER_SIZE; trace_index++)
    trace_buffer[trace_index] = NOTHING;

  trace_index = 0;

}

int Trace::dump1(unsigned index)
{
  char a_string[50];
  unsigned int i;
  file_register *r;

  int return_value = 1;

  if(trace_buffer[index] & (CYCLE_COUNTER_LO | CYCLE_COUNTER_HI))
    {
      // Cycle counter

      // A cycle counter occupies two consecutive trace buffer entries.
      // We have to determine if the current entry (pointed to by index) is
      // the high or low integer of the cycle counter.
      //
      // The upper two bits of the trace are used to decode the two 32-bit
      // integers that comprise the cycle counter. The encoding algorithm is
      // optimized for speed:
      // CYCLE_COUNTER_LO is defined as 1<<31
      // CYCLE_COUNTER_HI is defined as 1<<30
      //
      //   trace[i] = low 32 bits of cycle counter | CYCLE_COUNTER_LO
      //   trace[i+1] = upper 32 bits of "    " | CYCLE_COUNTER_HI | bit 31 of cycle counter
      //
      // The low 32-bits are always saved in the trace buffer with the msb (CYCLE_COUNTER_LO)
      // set. However, notice that this bit may've already been set prior to calling trace().
      // So we need to make sure that we don't lose it. This is done by copying it along
      // with the high 32-bits of the cycle counter into the next trace buffer location. The
      // upper 2 bits of the cycle counter are assumed to always be zero (if they're not, gpsim
      // has been running for a loooonnnggg time!). Bit 30 (CYCLE_COUNTER_HIGH) is always
      // set in the high 32 bit trace. While bit 31 gets the copy of bit 31 that was over
      // written in the low 32 bit trace.
      //
      // Here are some examples:
      //                                                upper 2 bits 
      //    cycle counter    |  trace[i]    trace[i+1]    [i]   [i+1]
      //---------------------+----------------------------------------
      //         0x12345678  |  0x92345678  0x40000000    10     01
      //         0x44445555  |  0xc4445555  0x40000000    11     01
      // 0x1111222233334444  |  0xb3334444  0x51112222    10     01
      //         0x9999aaaa  |  0x9999aaaa  0xc0000000    10     11
      //         0xccccdddd  |  0xccccdddd  0xc0000000    11     11
      //         0xccccddde  |  0xccccddde  0xc0000000    11     11
      //
      // Looking at the upper two bits of trace buffer, we can make these
      // observations:
      //
      // 00 - not a cycle counter trace
      // 10 - current index points at the low int of a cycle counter
      // 01 - current index points at the high int of a cycle counter
      // 11 - if traces on either side of the current index are the same
      //      then the current index points to a low int else it points to a high int

      int j = index;                         // Assume that the index is pointing to the low int.
      int k = (j + 1) & TRACE_BUFFER_MASK;   // and that the next entry is the high int.

      if( (trace_buffer[j] & CYCLE_COUNTER_LO) &&
	  (trace_buffer[k] & CYCLE_COUNTER_HI) )
	{
	  if(trace_buffer[j] & CYCLE_COUNTER_HI)
	    {
	      // The upper two bits of the current trace are set. This means that
	      // the trace is either the high 32 bits or the low 32 bits of the cycle 
	      // counter. This ambiguity is resolved by examining the trace buffer on
	      // either side of the current index. If the entry immediately proceeding
	      // this one is not a cycle counter trace, then we know that we're pointing
	      // at the low 32 bits. If the proceeding entry IS a cycle counter trace then
	      // we have two consecutive cycle traces (we already know that the entry
	      // immediately following the current trace index is a cycle counter trace).
	      // Now we know that if  have consecutive cycle traces, then they differ by one
	      // count. We only need to look at the low 32 bits of these consecutive
	      // traces to ascertain this.
	      i = (index - 1) &  TRACE_BUFFER_MASK;   // previous index
	      if( (trace_buffer[i] & (CYCLE_COUNTER_HI | CYCLE_COUNTER_LO)) &&
		     (((trace_buffer[k] - trace_buffer[i]) & 0x7fffffff) == 1) )
		return return_value;
	    }

	  // The current index points to the low int and the next entry is the high int
	  printf("  cycle: 0x%x%x\n" ,
		 (trace_buffer[k]&0x3fffffff),
		 ((trace_buffer[j]&0x7fffffff) | (trace_buffer[k]&0x80000000 )));
	  return_value = 2;

	}

    }

  else
    {
      switch (trace_buffer[index] & 0xff000000)
	{
	case NOTHING:
	  printf("  empty trace cycle\n");
	  break;
	case INSTRUCTION:
	  if(trace_flag & TRACE_INSTRUCTION)
	    printf("instruction: 0x%04x\n", trace_buffer[index]&0xffff);
	  break;
	case PROGRAM_COUNTER:
	  if(trace_flag & TRACE_PROGRAM_COUNTER) {
	    i = trace_buffer[index]&0xffff;
	    printf("  pc: 0x%04x %s\n", i ,cpu->program_memory[i]->name(a_string));
	  }
	  break;
	case PC_SKIP:
	  i = trace_buffer[index]&0xffff;
	  printf("  skipped: %04x %s\n", i, cpu->program_memory[i]->name(a_string));
	  break;
	case REGISTER_READ:
	  r = cpu->registers[(trace_buffer[index]>>8) & 0xfff];
	  printf("   read: 0x%02x from %s\n",trace_buffer[index]&0xff, r->name());
	  break;
	case REGISTER_WRITE:
	  r = cpu->registers[(trace_buffer[index]>>8) & 0xfff];
	  printf("  wrote: 0x%02x to %s\n",trace_buffer[index]&0xff, r->name());
	  break;
	case READ_W:
	  printf("   read: 0x%02x from W\n",trace_buffer[index]&0xff);
	  break;
	case WRITE_W:
	  printf("  wrote: 0x%02x to W\n",trace_buffer[index]&0xff);
	  break;
	case WRITE_TRIS:
	  printf("  wrote: 0x%02x to TRIS\n",trace_buffer[index]&0xff);
	  break;
	case WRITE_OPTION:
	  printf("  wrote: 0x%02x to OPTION\n",trace_buffer[index]&0xff);
	  break;
	case BREAKPOINT:
	  printf("BREAK: ");
	  bp.dump_traced(trace_buffer[index] & 0xffffff);
	  break;
	case INTERRUPT:
	  printf("interrupt:\n");
	  break;
	case _RESET:
	  switch( (RESET_TYPE) (trace_buffer[index]&0xff))
	    {
	    case POR_RESET:
	      printf("POR");
	      break;
	    case WDT_RESET:
	      printf("WDT");
	      break;
	    default:
	      printf("unknown");
	    }
	  printf(" reset\n");
	  break;

	case OPCODE_WRITE:
	  if((trace_buffer[(index-1)&TRACE_BUFFER_MASK] & 0xff000000) == OPCODE_WRITE)
	    printf("wrote opcode: 0x%04 to pgm memory: 0x%05x\n",
		   trace_buffer[index]&0xffff,
		   trace_buffer[(index-1)&TRACE_BUFFER_MASK] & 0xffffff);

	  break;

	case MODULE_TRACE2:
	  return_value = 2;
	case MODULE_TRACE1:
	  printf(" module trace  0x%lx\n",trace_buffer[index]&0xffffff);
	  break;

	default:
	  printf("*** INVALID TRACE ***\n");
	}
    }

  return return_value;

}


void Trace::dump(unsigned int n=0)
{
  int i;

  trace_flag = 0xffffffff;    // Trace everything.

  if(n)
    i = (trace_index - n) & TRACE_BUFFER_MASK;
  else
    i = (trace_index - 10) & TRACE_BUFFER_MASK;

  do
    {
      
      i = (i + dump1(i)) & TRACE_BUFFER_MASK;
    } while( (i != trace_index) && (i != ( (trace_index+1) & TRACE_BUFFER_MASK)));
}



void Trace::dump_last_instruction(void)
{
  char a_string[50];
  unsigned int i;
  unsigned int
    instruction_index,
    pc_index;
  bool 
    found_instruction = 0,
    found_pc =0;
  file_register *r;


  i = (trace_index-1) & TRACE_BUFFER_MASK;

  // Starting at the latest trace, search backwards first for 
  // an instruction and then for a program_counter
  do
  {
    switch (trace_buffer[i] & 0xff000000)
    {

    case INSTRUCTION:
      instruction_index = i;
      found_instruction = 1;
      break;

    case PROGRAM_COUNTER:
      if(found_instruction) 
	{
	  pc_index = i;
	  found_pc = 1;
	}
      break;

    }

    i = (i-1) & TRACE_BUFFER_MASK;

 
  // i == trace_index then we looped all the way around without finding anything
  // that should NEVER happen.
  
  } while( (i != trace_index) && (!found_pc));

  if(found_pc)
    {
      i = trace_buffer[pc_index]&0xffff;

      printf("%s  0x%04X  0x%04X  %s\n",cpu->name_str,i,
	     trace_buffer[instruction_index]&0xffff,
	     cpu->program_memory[i]->name(a_string) );

      trace_flag &= ~(TRACE_INSTRUCTION | TRACE_PROGRAM_COUNTER);

      if(instruction_index != trace_index) 
	{
	  // Get the trace buffer index just past the point where the instruction was traced
	  i = (instruction_index  + 1) & TRACE_BUFFER_MASK;

	  // Now search the trace buffer for all register accesses
	  do
	    {
	      i = (i + dump1(i)) & TRACE_BUFFER_MASK;
	    } while( (i != trace_index) && (i != ( (trace_index+1) & TRACE_BUFFER_MASK)));

	} 
      else
	{
	  printf("nothing past the instruction   trace_index %x  inst index %x\n ",trace_index, instruction_index);
	}

    }
  else
    {
      printf("Error while dumping the trace (couldn't find any instructions)\n");
    }
}


/*****************************************************************
 *
 *         Logging
 */
TraceLog::TraceLog(void)
{
  logging = 0;
  log_filename = NULL;
  //  log_file = (FILE)NULL;
}

TraceLog::~TraceLog(void)
{

  disable_logging();
    
  if(log_filename)
    free(log_filename);

}

void TraceLog::callback(void)
{

  return;

}

void TraceLog::enable_logging(char *new_fname)
{

  if(logging)
    return;

  logging = 1;

}

void TraceLog::disable_logging(void)
{

  if(!logging)
    return;

  logging = 0;


}
//*****************************************************************
// *** KNOWN CHANGE ***
//  Support functions that will get replaced by the CORBA interface.
//  

//--------------------------------------------
void trace_dump_all(void)
{
  trace.dump();

}
//--------------------------------------------
void trace_dump_n(int numberof)
{
  trace.dump(numberof);
}
