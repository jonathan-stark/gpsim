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
#include "xref.h"

#define MODE "0x" << hex

Trace trace;     // Instantiate the trace buffer class
TraceLog trace_log;
ProfileKeeper profile_keeper;


#define TRACE_INSTRUCTION       (1<< (INSTRUCTION >> 24))
#define TRACE_PROGRAM_COUNTER   (1<< (PROGRAM_COUNTER >> 24))
#define TRACE_CYCLE_INCREMENT   (1<< (CYCLE_INCREMENT >> 24))
#define TRACE_ALL (0xffffffff)

Trace::Trace(void)
{

  for(trace_index = 0; trace_index < TRACE_BUFFER_SIZE; trace_index++)
    trace_buffer[trace_index] = NOTHING;

  trace_index = 0;
  string_cycle = 0;

  xref = new XrefObject(&trace_index);

}

Trace::~Trace(void)
{
  if(xref)
    delete xref;

}

//--------------------------------------------------------------
// is_cycle_trace(unsigned int index)
//
//  Given an index into the trace buffer, this function determines
// if the trace is a cycle counter trace.
//
// INPUT: index - index into the trace buffer
// RETURN: 0 - trace is not a cycle counter
//         1 - trace is the high integer of a cycle trace
//         2 - trace is the low integer of a cycle trace

int Trace::is_cycle_trace(unsigned int index)
{

  if(!(trace_buffer[index] & (CYCLE_COUNTER_LO | CYCLE_COUNTER_HI)))
    return 0;


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
	  int i = (index - 1) &  TRACE_BUFFER_MASK;   // previous index
	  if( (trace_buffer[i] & (CYCLE_COUNTER_HI | CYCLE_COUNTER_LO)) &&
	      (((trace_buffer[k] - trace_buffer[i]) & 0x7fffffff) == 1) )
	    return 1;
	}

      // The current index points to the low int and the next entry is the high int
      return 2;

    }

  //printf("trace error??? in cycle trace\n");

  return 1;
}

int Trace::dump1(unsigned index, char *buffer, int bufsize)
{
  char a_string[50];
  unsigned int i;
  file_register *r;

  int return_value = is_cycle_trace(index);

  if(bufsize)
    buffer[0] = 0;   // NULL terminate just in case no string is created

  if(return_value == 2) {
    
    int k = (index + 1) & TRACE_BUFFER_MASK;
    if(trace_flag & (CYCLE_COUNTER_LO | CYCLE_COUNTER_HI))
      snprintf(buffer, bufsize,"  cycle: 0x%x%x" ,
	     (trace_buffer[k]&0x3fffffff),
	     ((trace_buffer[index]&0x7fffffff) | (trace_buffer[k]&0x80000000 )));

    return(return_value);

  }

  
  return_value = 1;

  switch (trace_buffer[index] & 0xff000000)
    {
    case NOTHING:
      //snprintf("  empty trace cycle\n");
      break;
    case INSTRUCTION:
      if(trace_flag & TRACE_INSTRUCTION)
	snprintf(buffer, bufsize," instruction: 0x%04x",
		 trace_buffer[index]&0xffff);
      break;
    case PROGRAM_COUNTER:
      if(trace_flag & TRACE_PROGRAM_COUNTER) {
	i = trace_buffer[index]&0xffff;
	snprintf(buffer, bufsize,"  pc: 0x%04x %s", 
		 i ,cpu->program_memory[i]->name(a_string));
      }
      break;
    case PC_SKIP:
      i = trace_buffer[index]&0xffff;
      snprintf(buffer, bufsize,"  skipped: %04x %s",
	       i, cpu->program_memory[i]->name(a_string));
      break;
    case REGISTER_READ:
      r = cpu->registers[(trace_buffer[index]>>8) & 0xfff];
      snprintf(buffer, bufsize,"   read: 0x%02x from %s",
	       trace_buffer[index]&0xff, r->name());
      break;
    case REGISTER_WRITE:
      r = cpu->registers[(trace_buffer[index]>>8) & 0xfff];
      snprintf(buffer, bufsize,"  wrote: 0x%02x to %s",
	       trace_buffer[index]&0xff, r->name());
      break;
    case READ_W:
      snprintf(buffer, bufsize,"   read: 0x%02x from W",
	       trace_buffer[index]&0xff);
      break;
    case WRITE_W:
      snprintf(buffer, bufsize,"  wrote: 0x%02x to W",
	       trace_buffer[index]&0xff);
      break;
    case WRITE_TRIS:
      snprintf(buffer, bufsize,"  wrote: 0x%02x to TRIS",
	       trace_buffer[index]&0xff);
      break;
    case WRITE_OPTION:
      snprintf(buffer, bufsize,"  wrote: 0x%02x to OPTION",
	       trace_buffer[index]&0xff);
      break;
    case BREAKPOINT:
      snprintf(buffer, bufsize,"BREAK: ");
      bp.dump_traced(trace_buffer[index] & 0xffffff);
      break;
    case INTERRUPT:
      snprintf(buffer, bufsize,"interrupt");
      break;
    case _RESET:
      switch( (RESET_TYPE) (trace_buffer[index]&0xff))
	{
	case POR_RESET:
	  snprintf(buffer, bufsize,"POR");
	  break;
	case WDT_RESET:
	  snprintf(buffer, bufsize,"WDT reset");
	  break;
	default:
	  snprintf(buffer, bufsize,"unknown reset");
	}
      break;

    case OPCODE_WRITE:
      if((trace_buffer[(index-1)&TRACE_BUFFER_MASK] & 0xff000000) == OPCODE_WRITE)
	snprintf(buffer, bufsize,"wrote opcode: 0x%04 to pgm memory: 0x%05x",
	       trace_buffer[index]&0xffff,
	       trace_buffer[(index-1)&TRACE_BUFFER_MASK] & 0xffffff);

      break;

    case CYCLE_INCREMENT:
      if(trace_flag & TRACE_CYCLE_INCREMENT)
	snprintf(buffer, bufsize,"cycle increment");
      break;
    case MODULE_TRACE2:
      return_value = 2;
    case MODULE_TRACE1:
      snprintf(buffer, bufsize," module trace  0x%lx",
	       trace_buffer[index]&0xffffff);
      break;

      //default:
      //snprintf("*** INVALID TRACE ***\n");
    }

  return return_value;

}

//------------------------------------------------------------------
// find_previous_cycle
//
//  Starting at the trace index passed to it, this routine will search
// backwards in the trace buffer for the next instruction.

int Trace::find_previous_cycle(int index)
{

  int cycles = 0;

  index &= TRACE_BUFFER_MASK;

  do {

      switch (trace_buffer[index] & 0xff000000) {

      case INSTRUCTION:
	return ((cycles << 16) | index);

      case CYCLE_INCREMENT:
	cycles++;
	break;
	
      }

      index = (index - 1) & TRACE_BUFFER_MASK;
    
  } while (index != trace_index);

  return 0;
}

//------------------------------------------------------------------
// find_cycle
//
//  This routine will search for the n'th cycle in the trace buffer
// BEFORE the one at "in_index". It will then find the instruction 
// and pc indices after this cycle.
//
// INPUT
//    n - number of cycles to search back into the trace buffer.
//
// INPUTS/OUTPUTS
//    instruction_index
//    pc_index
//    cycle_index
//
// These three input parameters are actually outputs. They are passed by
// reference and will get assigned the trace buffer index at which they're
// found. More specifically, they're assigned the trace buffer index after
// the n'th cycle has been found


guint64 Trace::find_cycle(int n, int in_index, int &instruction_index, int &pc_index, int &cycle_index)
{
  unsigned int i;
  guint64 cycle=0;
  bool 
    found_instruction = 0,
    found_pc =0,
    found_cycle = 0;

  // -1 means the index wasn't found.
  instruction_index=-1;
  pc_index=-1;
  cycle_index=-1;

  if(in_index>0) {
    i = in_index & TRACE_BUFFER_MASK;
  } else {

    // The trace_index is pointing to the next available slot,
    // but we need to start looking at the last traced thing.
    i = (trace_index-1) & TRACE_BUFFER_MASK;

  }

  // search backwards first for an instruction and then for a program_counter

  do
  {
    if(is_cycle_trace(i) == 2) {
      found_cycle = 1;
      cycle_index = i;
      int k = (i+1) & TRACE_BUFFER_MASK;
      cycle = trace_buffer[k]&0x3fffffff;
      cycle = (cycle << 32) | 
	   ((trace_buffer[i]&0x7fffffff) | (trace_buffer[k]&0x80000000 ));

    } else {

      switch (trace_buffer[i] & 0xff000000) {

      case INSTRUCTION:
	instruction_index = i;
	n--;
	if(n == 0)
	  found_instruction = 1;
	if(found_cycle)
	  cycle--;
	break;

      case PROGRAM_COUNTER:
	if(found_instruction) {
	  found_pc = 1;
	}
	pc_index = i;

	break;

      case CYCLE_INCREMENT:
	if(found_cycle)
	  cycle--;
	break;
	
      }

    }

    i = (i-1) & TRACE_BUFFER_MASK;

 
  // i == trace_index then we looped all the way around without finding anything
  // that should NEVER happen.
  
  } while( (i != trace_index) && (!found_pc));

  return cycle;
}

//------------------------------------------------------------------
// int Trace::dump_instruction(unsigned int instruction_index)
//
//  Starting at the `instruction_index', this function will decode the
// trace buffer upto the next instruction.
//

int Trace::dump_instruction(unsigned int instruction_index)
{

  FILE *out_stream=NULL;
  char a_string[50];

  instruction_index &= TRACE_BUFFER_MASK;

  // make sure the instruction index really does point to a traced instruction.
  if(INSTRUCTION != trace_buffer[instruction_index] & 0xff000000)
    return -1;

  int i = (instruction_index + 1) & TRACE_BUFFER_MASK;


  bool 
    found_instruction = 0,
    found_pc =0,
    found_cycle = 0;

  // -1 means the index wasn't found.
  int pc_index=-1;
  int cycle_index=-1;
  int cycle =0;

  // Starting at the latest trace, search backwards first for 
  // an instruction and then for a program_counter
  do
  {
    switch (trace_buffer[i] & 0xff000000) {

    case INSTRUCTION:
      found_instruction = 1;
      break;

    case PROGRAM_COUNTER:
      found_pc = 1;
      pc_index = i;

      break;

    case CYCLE_INCREMENT:
      cycle++;
      break;
	
    }


    i = (i+1) & TRACE_BUFFER_MASK;

 
  } while( (i != trace_index) && (!found_pc));


  do {

    i = trace_buffer[pc_index]&0xffff;

    if(string_cycle && out_stream) 
      fprintf(out_stream,"0x%016LX  ",string_cycle);

    snprintf(string_buffer, sizeof(string_buffer),
	     "%s  0x%04X  0x%04X  %s",cpu->name_str,i,
	     trace_buffer[instruction_index]&0xffff,
	     cpu->program_memory[i]->name(a_string) );
    if(out_stream)
      fprintf(out_stream,"%s\n",string_buffer);
    string_index = instruction_index;

    if(xref)
      xref->update();

    found_pc = 0;
    found_instruction = 0;

    if(instruction_index != trace_index) {

      // Get the trace buffer index just past the point where the instruction was traced
      i = (instruction_index  + 1) & TRACE_BUFFER_MASK;

      // Now dump all of the trace information that occurred along with this
      // instruction.
      do  {
	switch (trace_buffer[i] & 0xff000000) {

	case INSTRUCTION:
	  instruction_index = i;
	  string_cycle++;
	  found_instruction = 1;
	  break;

	case PROGRAM_COUNTER:
	  pc_index = i;
	  found_pc = 1;

	  break;

	case CYCLE_INCREMENT:
	  string_cycle++;
	  break;
	
	}


	string_index = i;

	i = (i + dump1(i,string_buffer, sizeof(string_buffer))) & TRACE_BUFFER_MASK;

	if(string_buffer[0]) {

	  if(out_stream)
	    fprintf(out_stream,"%s\n",string_buffer);

	  if(xref)
	    xref->update();
	}

      } while( (i != trace_index) && (i != ( (trace_index+1) & TRACE_BUFFER_MASK))
	       &&
	       (!found_pc || !found_instruction)
	       );

    } else
      i = trace_index;  // break out of the loop

  } while( (i != trace_index) && (i != ( (trace_index+1) & TRACE_BUFFER_MASK)));


}

//------------------------------------------------------------------
// int Trace::dump(unsigned int n=0)
//

int Trace::dump(unsigned int n=0, FILE *out_stream=NULL)
{

  char a_string[50];
  unsigned int i;
  int
    instruction_index=-1,
    pc_index=-1,
    cycle_index=-1;
  bool 
    found_instruction = 0,
    found_pc =0,
    found_cycle = 0;

  file_register *r;

  if(!cpu)
      return 0;

  if(!n)
    n = 5;

  string_cycle = find_cycle(n,-1,instruction_index, pc_index, cycle_index);

  if(pc_index>=0) {

    // We are going to print the cycle (if it was found) along with the
    // instruction so turn off the trace flags associated with the cycle
    // counter. This way, the cycle traces will not get printed when we
    // call dump1()

    trace_flag = TRACE_ALL & ~(TRACE_CYCLE_INCREMENT | 
			       CYCLE_COUNTER_LO      | 
			       CYCLE_COUNTER_HI      |
			       TRACE_INSTRUCTION     |
			       TRACE_PROGRAM_COUNTER);
    do {

      i = trace_buffer[pc_index]&0xffff;

      if(string_cycle && out_stream) 
	fprintf(out_stream,"0x%016LX  ",string_cycle);

      snprintf(string_buffer, sizeof(string_buffer),
	       "%s  0x%04X  0x%04X  %s",cpu->name_str,i,
	       trace_buffer[instruction_index]&0xffff,
	       cpu->program_memory[i]->name(a_string) );
      if(out_stream)
	fprintf(out_stream,"%s\n",string_buffer);
      string_index = instruction_index;

      if(xref)
	xref->update();

      found_pc = 0;
      found_instruction = 0;

      if(instruction_index != trace_index) {

	// Get the trace buffer index just past the point where the instruction was traced
	i = (instruction_index  + 1) & TRACE_BUFFER_MASK;

	// Now dump all of the trace information that occurred along with this
	// instruction.
	do  {
	  switch (trace_buffer[i] & 0xff000000) {

	  case INSTRUCTION:
	    instruction_index = i;
	    string_cycle++;
	    found_instruction = 1;
	    break;

	  case PROGRAM_COUNTER:
	    pc_index = i;
	    found_pc = 1;

	    break;

	  case CYCLE_INCREMENT:
	    string_cycle++;
	    break;
	
	  }


	  string_index = i;

	  i = (i + dump1(i,string_buffer, sizeof(string_buffer))) & TRACE_BUFFER_MASK;

	  if(string_buffer[0]) {

	    if(out_stream)
	      fprintf(out_stream,"%s\n",string_buffer);

	    if(xref)
	      xref->update();
	  }

	} while( (i != trace_index) && (i != ( (trace_index+1) & TRACE_BUFFER_MASK))
		 &&
		 (!found_pc || !found_instruction)
		 );

      } else
	i = trace_index;  // break out of the loop

    } while( (i != trace_index) && (i != ( (trace_index+1) & TRACE_BUFFER_MASK)));

  }

  return n;

}

//---------------------------------------------------------
// dump_raw
// mostly for debugging, 
void Trace::dump_raw(int n)
{
  if(!n)
    return;

  const int BUFFER_SIZE = 50;

  char buffer[BUFFER_SIZE];

  int i = (trace_index - n)  & TRACE_BUFFER_MASK;

  trace_flag = TRACE_ALL;

  do {
    if(is_cycle_trace(i))
      printf("0x%016LX:%016LX",trace_buffer[i], trace_buffer[(i+1) & TRACE_BUFFER_MASK]);
    else
      printf("0x%016LX",trace_buffer[i]);

    i = (i + dump1(i,buffer,BUFFER_SIZE)) & TRACE_BUFFER_MASK;

    if(buffer[0]) 
      printf("%s",buffer);
    putc('\n',stdout);

  } while((i!=trace_index) && (i!=((trace_index+1)&TRACE_BUFFER_MASK)));
    putc('\n',stdout);
    putc('\n',stdout);
}

//
// dump_last_instruction(void)

void Trace::dump_last_instruction(void)
{
  dump(1,stdout);
}


/*****************************************************************
 *
 *         Logging
 */
TraceLog::TraceLog(void)
{
  logging = 0;
  log_filename = NULL;
  cpu = NULL;
  log_file = NULL;
  last_trace_index = 0;
}

TraceLog::~TraceLog(void)
{

  disable_logging();
    
  close_logfile();

}

void TraceLog::callback(void)
{
  if(log_file) {
    
    if(last_trace_index < trace.trace_index) {
      fwrite(&trace.trace_buffer[last_trace_index],
	     sizeof(unsigned int),
	     trace.trace_index - last_trace_index,
	     log_file);
    } else {
      fwrite(&trace.trace_buffer[last_trace_index],
	     sizeof(unsigned int),
	     TRACE_BUFFER_MASK - last_trace_index,
	     log_file);
      fwrite(&trace.trace_buffer[0],
	     sizeof(unsigned int),
	     trace.trace_index,
	     log_file);
    }

    //for(int i=last_trace_index; i!=trace.trace_index; i = (i+1)& TRACE_BUFFER_MASK) 
    //  fprintf(log_file,"%08x\n",trace.trace_buffer[i]);

    last_trace_index = trace.trace_index;
    cpu->cycles.set_break(cpu->cycles.value + 1000,this);

  }

}

void TraceLog::open_logfile(char *new_fname)
{

  if(!new_fname)
    new_fname = "gpsim.log";

  if(log_filename) {
    //
    // Looks like there's a log file open and now we
    // want to open a different one.
    //

    if(strcmp(new_fname, log_filename) == 0 ) 
      return;  // the file with this name is already opened


    close_logfile();

  }

  log_file = fopen(new_fname, "w");
  log_filename = strdup(new_fname);

}

void TraceLog::close_logfile(void)
{

  if(log_filename) {
    fclose(log_file);
    free(log_filename);
  }

}

void TraceLog::enable_logging(char *new_fname)
{

  if(logging)
    return;

  if(!cpu) {
    if(active_cpu)
      cpu = active_cpu;
    else
      cout << "Warning: Logging can't be enabled until a cpu has been selected.";
  }

  open_logfile(new_fname);

  last_trace_index = trace.trace_index;
  cpu->cycles.set_break(cpu->cycles.value + 1000,this);
  logging = 1;

}

void TraceLog::disable_logging(void)
{

  if(!logging)
    return;

  logging = 0;


}

void TraceLog::switch_cpus(pic_processor *pcpu)
{
  cpu = pcpu;
}


/*****************************************************************
 *
 *         Profiling
 */
ProfileKeeper::ProfileKeeper(void)
{
  enabled = 0;
  cpu = NULL;
  last_trace_index = 0;
}

ProfileKeeper::~ProfileKeeper(void)
{

  disable_profiling();
}

void ProfileKeeper::catchup(void)
{
    if(!enabled)
        return;
    for(int i=last_trace_index; i!=trace.trace_index; i = (i+1)& TRACE_BUFFER_MASK)
    {
	switch (trace.trace_buffer[i] & 0xff000000)
	{
	case INSTRUCTION:
	    instruction_address=trace_pc_value;
	    cpu->program_memory[instruction_address]->cycle_count++;
	    trace_pc_value++;
	    break;

	case PROGRAM_COUNTER:
	case PC_SKIP:
	    trace_pc_value=trace.trace_buffer[i]&0xffff;
	    break;

	case CYCLE_INCREMENT:
	    cpu->program_memory[instruction_address]->cycle_count++;
	    break;
	}
    }

    last_trace_index = trace.trace_index;
}

void ProfileKeeper::callback(void)
{
    if(enabled)
    {
        catchup();
	cpu->cycles.set_break(cpu->cycles.value + 1000,this);
    }
}

void ProfileKeeper::enable_profiling(void)
{

    if(enabled)
	return;

    if(!cpu) {
	if(active_cpu)
	    cpu = active_cpu;
	else
	    cout << "Warning: Profiling can't be enabled until a cpu has been selected.";
    }

    last_trace_index = trace.trace_index;
    cpu->cycles.set_break(cpu->cycles.value + 1000,this);
    enabled = 1;
}

void ProfileKeeper::disable_profiling(void)
{

    if(!enabled)
	return;

    enabled = 0;
}

void ProfileKeeper::switch_cpus(pic_processor *pcpu)
{
    cpu = pcpu;
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
  trace.dump(numberof,stdout);
}
//--------------------------------------------
void trace_dump_raw(int numberof)
{
  trace.dump_raw(numberof);
}
