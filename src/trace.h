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
#include <stdio.h>
#include <glib.h>

#include "gpsim_classes.h"
#include "breakpoints.h"

extern "C"
{
#include "lxt_write.h"
}

class Processor;

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
#define    CYCLE_INCREMENT    (0x10<<24)
#define    REGISTER_READ_VAL  (0x11<<24)
#define    REGISTER_WRITE_VAL (0x12<<24)
#define    REGISTER_READ_16BITS  (0x13<<24)
#define    REGISTER_WRITE_16BITS (0x14<<24)

#define    CYCLE_COUNTER_LO   (0x80<<24)
#define    CYCLE_COUNTER_HI   (0x40<<24)


#define    TRACE_BUFFER_SIZE  (1<<12)
#define    TRACE_BUFFER_MASK  (TRACE_BUFFER_SIZE-1)
#define    TRACE_BUFFER_NEAR_FULL  (TRACE_BUFFER_SIZE * 3 /4)
#define    TRACE_STRING_BUFFER 50

  unsigned int trace_buffer[TRACE_BUFFER_SIZE];
  unsigned int trace_index;
  unsigned int trace_flag;

  // When interfaced with a gui, the contents of the trace
  // buffer are decoded one line-at-a-time, copied to the string_buffer
  // and sent to the gui via xref interface (actually, the gui
  // is notified that new data is available in the string_buffer).
  XrefObject *xref;
  char  string_buffer[TRACE_STRING_BUFFER];
  guint64 string_cycle;          // The cycle corresponding to the decoded string
  unsigned int string_index;     // The trace buffer index corresponding "   "

  Processor *cpu;

  Trace (void);
  ~Trace(void);

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

  inline void register_read_16bits (unsigned int address, unsigned int value)
  {
    trace_buffer[trace_index] = REGISTER_READ_16BITS | (address << 16) | (value & 0xffff);
    trace_index = (trace_index + 1) & TRACE_BUFFER_MASK;
  }

  inline void register_write_16bits (unsigned int address, unsigned int value)
  {
    trace_buffer[trace_index] = REGISTER_WRITE_16BITS | (address << 16) | (value & 0xffff);
    trace_index = (trace_index + 1) & TRACE_BUFFER_MASK;
  }

  inline void register_read_value (unsigned int address, unsigned int value)
  {
    trace_buffer[trace_index] = REGISTER_READ_VAL | (address << 8) | value;
    trace_index = (trace_index + 1) & TRACE_BUFFER_MASK;
  }

  inline void register_write_value (unsigned int address, unsigned int value)
  {
    trace_buffer[trace_index] = REGISTER_WRITE_VAL | (address << 8) | value;
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
  inline void cycle_increment (void)
  {
    // For those instructions that advance the cycle counter by 2,
    // we have to record the extra cycle.
    trace_buffer[trace_index] = CYCLE_INCREMENT;
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

  inline bool near_full(void) {
    return (trace_index > TRACE_BUFFER_NEAR_FULL);
  }

  void switch_cpus(Processor *new_cpu) {cpu = new_cpu;};

  int  dump (unsigned int n=0, FILE *out_stream=0, int watch_reg=-1);
  void dump_last_instruction(void);
  int  dump1(unsigned int,char *, int);
  int  dump_instruction(unsigned int instruction_index);
  void dump_raw(int n);

  int is_cycle_trace(unsigned int index);
  guint64 find_cycle(int n, int in_index, int &instruction_index, 
		     int &pc_index, int &cycle_index);
  int find_previous_cycle(int index);
};

#ifdef IN_MODULE
// we are in a module: don't access trace object directly!
Trace &get_trace(void);
#else
// we are in gpsim: use of get_trace() is recommended,
// even if trace object can be accessed directly.
extern Trace trace;

inline Trace &get_trace(void)
{
  return trace;
}
#endif


//-----------------------------------------------------------
#define TRACE_FILE_FORMAT_ASCII 0
#define TRACE_FILE_FORMAT_LXT 1
class TraceLog : public BreakpointObject
{
public:
  bool logging;
  bool lograw;
  int items_logged;
  char *log_filename;
  FILE *log_file;
  Processor *cpu;
  unsigned int last_trace_index;
  Trace buffer;
  int file_format;
  struct lt_trace *lxtp;
  struct lt_symbol *symp;

  TraceLog(void);
  ~TraceLog(void);

  virtual void callback(void);
  void enable_logging(const char *new_filename=0, int format=TRACE_FILE_FORMAT_ASCII);
  void disable_logging(void);
  void switch_cpus(Processor *new_cpu);
  void open_logfile(const char *new_fname, int format);
  void close_logfile(void);
  void write_logfile(void);
  void status(void);

  void lxt_trace(unsigned int address, unsigned int value, guint64 cc);

  void register_read(unsigned int address, unsigned int value, guint64 cc);
  void register_write(unsigned int address, unsigned int value, guint64 cc);
  void register_read_value(unsigned int address, unsigned int value, guint64 cc);
  void register_write_value(unsigned int address, unsigned int value, guint64 cc);

};

extern TraceLog trace_log;

//-----------------------------------------------------------
class ProfileKeeper : public BreakpointObject
{
public:
  bool enabled;
  Processor *cpu;
  unsigned int last_trace_index;
  unsigned int instruction_address;
  unsigned int trace_pc_value;

  ProfileKeeper(void);
  ~ProfileKeeper(void);

  void catchup(void);
  virtual void callback(void);
  void enable_profiling(void);
  void disable_profiling(void);
  void switch_cpus(Processor *new_cpu);

};

extern ProfileKeeper profile_keeper;


/**********************************************************************
 * boolean event logging
 *
 * The boolean event logger is a class for logging the time
 * of boolean (i.e. 0/1) events.
 *
 * The class is designed to be efficient for both logging events and
 * for accessing events that have already been logged. The events
 * are stored in several small buffers that are linked together with
 * binary trees. Each small buffer is linear, i.e. an array. Each
 * element of the array stores the time when the event occurred.
 * The state of the event is encoded in the position of the array.
 * In other words, "high" events are at the odd indices of the array
 * and "low" ones at the even ones.
 *
 * Each small buffer is associated with a contiguous time span. The
 * start and end of this span is recorded so that one can quickly
 * ascertain if a certain time instant resideds in the buffer. 
 *
 * The binary tree is fairly standard. A single top node records three
 * numbers: the start time for the left child, the end time for the
 * left child (which by default is the start time for the right child)
 * and the end time for the right child. The nodes of left and right
 * children are similar to the parents. To find which small buffer
 * contains an event for a certain time, one simply starts at the
 * top of the tree and traverses the nodes until a leaf is reached.
 * A leaf, of course, is where the data is stored.
 *
 * The time for the event comes from gpsim's global cycle counter.
 * This counter is 64-bits wide. The buffers that store the time however,
 * are only 32-bits wide. There are two simple tricks employed to get
 * around this problem. First, full 64-bit time for the first event
 * is recorded. All subsequent events are 32-bit offsets from this.
 * Second, to ensure that the 32-bit offset does not wrap around, the
 * boolean event logger will set a cycle counter break point that is
 * less than 2^32 cycles in the future. If this break point is encountered
 * before the buffer fills, then this buffer is closed and added to the
 * binary and a new buffer is started.
 *
 * Repeated events are not logged. E.g.. if two 1's are logged, the 
 * second one is ignored.
 * 
 */

class BoolEventBuffer : public BreakpointObject
{
public:

  guint32  index;               // Index into the buffer
  guint32  *buffer;             // Where the time is stored
  guint32  max_events;          // Size of the event buffer
  guint64  start_time;          // time of the first event
  guint64  future_cycle;        // time at which the buffer can store no more data.
  bool     bInitialState;       // State when started.
  bool     bActive;             // True if the buffer is enabled for storing.
  bool     bFull;               // True if the buffer has been filled.


  BoolEventBuffer(bool _initial_state, guint32 _max_events = 4096);
  ~BoolEventBuffer(void);
  unsigned int get_index(guint64 event_time);
  void activate(bool _initial_state);
  void deactivate(void);
  void callback(void);
  void callback_print(void);
  inline bool event(bool state);


  inline bool isActive(void)
  {
    return bActive;
  }

  inline bool isFull(void)
  {
    return (index < max_events);
  }

  /*
    get_index - return the current index

    This is used by the callers to record where in the event
    buffer a specific event is stored. (e.g. The start bit
    of a usart bit stream.)
   */
  inline unsigned int get_index(void) {
    return index;
  }


  bool get_event(int index) {

    return (index & 1)^bInitialState;
  }

  bool get_state(guint64 event_time) {
    return get_event(get_index(event_time));
  }

  int get_edges(guint64 start_time, guint64 end_time) {
    return ( get_index(end_time) - get_index(start_time) );
  }

};

#endif
