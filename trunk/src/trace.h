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

#ifndef  __TRACE_H__
#define  __TRACE_H__

#include <unistd.h>
#include <glib.h>

//---------------------------------------------------------
// Class for trace buffer

class Trace
{
  public:


#define    NOTHING          0
#define    INSTRUCTION      (1<<24)
#define    PROGRAM_COUNTER  (2<<24)
#define    REGISTER_READ    (3<<24)
#define    REGISTER_WRITE   (4<<24)
#define    BREAKPOINT       (5<<24)
#define    INTERRUPT        (6<<24)
#define    READ_W           (7<<24)
#define    WRITE_W          (8<<24)
#define    _RESET           (9<<24)
#define    PC_SKIP          (0x0a<<24)
#define    WRITE_TRIS       (0x0b<<24)
#define    WRITE_OPTION     (0x0c<<24)
#define    OPCODE_WRITE     (0x0d<<24)
#define    MODULE_TRACE1    (0x0e<<24)
#define    MODULE_TRACE2    (0x0f<<24)
#define    CYCLE_COUNTER_LO (0x80<<24)
#define    CYCLE_COUNTER_HI (0x40<<24)


#define    TRACE_BUFFER_SIZE  (1<<12)
#define    TRACE_BUFFER_MASK  (TRACE_BUFFER_SIZE-1)

  unsigned int trace_buffer[TRACE_BUFFER_SIZE];
  unsigned int trace_index;
  pic_processor *cpu;

  Trace (void);

  inline void instruction (unsigned int opcode)
  {
    trace_buffer[trace_index] = INSTRUCTION | opcode;
    trace_index = (trace_index + 1) & TRACE_BUFFER_MASK;
  }

  inline void program_counter (unsigned int pc)
  {
    trace_buffer[trace_index] = PROGRAM_COUNTER | pc;
    trace_index = (trace_index + 1) & TRACE_BUFFER_MASK;
  }

  inline void pc_skip (unsigned int pc)
  {
    trace_buffer[trace_index] = PC_SKIP | pc;
    trace_index = (trace_index + 1) & TRACE_BUFFER_MASK;
  }

  inline void register_read (unsigned int address, unsigned int value)
  {
    trace_buffer[trace_index] = REGISTER_READ | (address << 8) | value;
    trace_index = (trace_index + 1) & TRACE_BUFFER_MASK;
  }

  inline void register_write (unsigned int address, unsigned int value)
  {
    trace_buffer[trace_index] = REGISTER_WRITE | (address << 8) | value;
    trace_index = (trace_index + 1) & TRACE_BUFFER_MASK;
  }

  inline void opcode_write (unsigned int address, unsigned int opcode)
  {
    trace_buffer[trace_index] = OPCODE_WRITE | (address & 0xffffff);
    trace_index = (trace_index + 1) & TRACE_BUFFER_MASK;
    trace_buffer[trace_index] = OPCODE_WRITE | (opcode & 0xffff);
    trace_index = (trace_index + 1) & TRACE_BUFFER_MASK;
  }


  inline void cycle_counter (guint64 cc)
  {
    // The 64 bit cycle counter requires two 32 bit traces.
    trace_buffer[trace_index] = CYCLE_COUNTER_LO | cc & 0xffffffff;
    trace_index = (trace_index + 1) & TRACE_BUFFER_MASK;
    trace_buffer[trace_index] = CYCLE_COUNTER_HI | (cc>>32) | (cc & CYCLE_COUNTER_LO);
    trace_index = (trace_index + 1) & TRACE_BUFFER_MASK;
  }
  inline void breakpoint (unsigned int bp)
  {
    trace_buffer[trace_index] = BREAKPOINT | bp;
    trace_index = (trace_index + 1) & TRACE_BUFFER_MASK;
  }

  void interrupt (void)
  {
    trace_buffer[trace_index] = INTERRUPT;
    trace_index = (trace_index + 1) & TRACE_BUFFER_MASK;
  }

  inline void read_W (unsigned int value)
  {
    trace_buffer[trace_index] = READ_W | value;
    trace_index = (trace_index + 1) & TRACE_BUFFER_MASK;
  }

  inline void write_W (unsigned int value)
  {
    trace_buffer[trace_index] = WRITE_W | value;
    trace_index = (trace_index + 1) & TRACE_BUFFER_MASK;
  }

  inline void reset (RESET_TYPE r)
    {
      trace_buffer[trace_index] = _RESET | ( (int) r);
      trace_index = (trace_index + 1) & TRACE_BUFFER_MASK;
    }

  inline void write_TRIS (unsigned int value)
  {
    trace_buffer[trace_index] = WRITE_TRIS | value;
    trace_index = (trace_index + 1) & TRACE_BUFFER_MASK;
  }

  inline void write_OPTION (unsigned int value)
  {
    trace_buffer[trace_index] = WRITE_OPTION | value;
    trace_index = (trace_index + 1) & TRACE_BUFFER_MASK;
  }

  inline void module1(unsigned int value)
  {
    trace_buffer[trace_index] = MODULE_TRACE1 | value;
    trace_index = (trace_index + 1) & TRACE_BUFFER_MASK;
  }

  inline void module2(unsigned int value1,unsigned int value2)
  {
    trace_buffer[trace_index] = MODULE_TRACE2 | value1;
    trace_index = (trace_index + 1) & TRACE_BUFFER_MASK;
    trace_buffer[trace_index] = MODULE_TRACE2 | value2;
    trace_index = (trace_index + 1) & TRACE_BUFFER_MASK;
  }

  void switch_cpus(pic_processor *new_cpu) {cpu = new_cpu;};

  void dump (unsigned int n=0);
  void dump_last_instruction(void);
  int  dump1(unsigned int);

};

extern Trace trace;

#endif
