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
#include "value.h"

extern "C"
{
#include "lxt_write.h"
}

class Processor;
class Trace;



class TraceFrame;
//========================================================================
class TraceObject
{
public:

  TraceObject();
  virtual void print(FILE *)=0;
  virtual void print_frame(TraceFrame *,FILE *);
  virtual void getState(TraceFrame *);

};
class ProcessorTraceObject : public TraceObject
{
public:
  Processor *cpu;

  ProcessorTraceObject(Processor *_cpu) : TraceObject() , cpu(_cpu)
  {
  }

  virtual void print(FILE *)=0;
};

class RegisterWriteTraceObject : public ProcessorTraceObject
{
public:
  Register *reg;
  RegisterValue from;
  RegisterValue to;

  //RegisterTraceObject();
  RegisterWriteTraceObject(Processor *_cpu, Register *_reg, RegisterValue trv);
  virtual void print(FILE *);
  virtual void getState(TraceFrame *);
};

class RegisterReadTraceObject : public RegisterWriteTraceObject
{
public:
  RegisterReadTraceObject(Processor *_cpu, Register *_reg, RegisterValue trv);
  virtual void print(FILE *);
  virtual void getState(TraceFrame *);
};

class PCTraceObject : public ProcessorTraceObject
{
public:
  unsigned int address;

  PCTraceObject(Processor *_cpu, unsigned int _address);
  virtual void print(FILE *);
  virtual void print_frame(TraceFrame *,FILE *);
};

//========================================================================
class TraceType
{
public:

  unsigned int type;		// The integer type is dynamically
				// assigned by the Trace class.
  unsigned int size;		// The number of positions this
				// type occupies

  TraceType(unsigned int t, unsigned int s);

  // Given an index into the trace buffer, decode()
  // will fetch traced items at that trace buffer index
  // and attempt to decode them.

  virtual TraceObject *decode(unsigned int tbi) = 0;

  virtual bool isFrameBoundary() { return false;}
  // Returns true if the trace record starting at index 'tbi' is of the same
  // type as this TraceType
  virtual bool isValid(unsigned int tbi);
  virtual int bitsTraced(void) { return 24; }
  virtual int dump_raw(unsigned tbi, char *buf, int bufsize);
};

class ProcessorTraceType : public TraceType
{
public:
  Processor *cpu;

  ProcessorTraceType(Processor *_cpu, 
		     unsigned int t,
		     unsigned int s)
    : TraceType(t,s), cpu(_cpu)
  {
  }

  virtual TraceObject *decode(unsigned int tbi) = 0;

};

class PCTraceType : public ProcessorTraceType
{
public:
  PCTraceType(Processor *_cpu, unsigned int t, unsigned int s);
  virtual TraceObject *decode(unsigned int tbi);
  virtual bool isFrameBoundary() { return true; }
  virtual int dump_raw(unsigned tbi, char *buf, int bufsize);
};

class RegisterWriteTraceType : public ProcessorTraceType
{
public:

  RegisterWriteTraceType(Processor *_cpu, unsigned int t, unsigned int s);

  virtual TraceObject *decode(unsigned int tbi);
  virtual int dump_raw(unsigned tbi, char *buf, int bufsize);

};

class RegisterReadTraceType : public ProcessorTraceType
{
public:

  RegisterReadTraceType(Processor *_cpu, unsigned int t, unsigned int s);

  virtual TraceObject *decode(unsigned int tbi);
  virtual int dump_raw(unsigned tbi, char *buf, int bufsize);

};

//========================================================================
// TraceFrame
//
// A trace frame collects all trace items that occurred at the same instant
// of time. When the trace buffer is decoded, markers will be examined
// to determine the frame boundaries.

class TraceFrame
{
public:
  list <TraceObject *> traceObjects;
  guint64 cycle_time;

  TraceFrame(); 
  virtual ~TraceFrame();
  virtual void add(TraceObject *to);
  virtual void print(FILE *);
  virtual void update_state(void);
};

//-----------------------------------------------------------
class TraceRawLog
{
public:

  char *log_filename;
  FILE *log_file;

  void log();
  void enable(char*);
  void disable();

  TraceRawLog();
  ~TraceRawLog();

};
//------------------------------------------------------------
class traceValue : public gpsimValue
{
 public:
  traceValue(void);
  virtual void put_value(unsigned int new_value) {};
  virtual unsigned int get_value(void);
};
//---------------------------------------------------------
// Class for trace buffer

class Trace
{
  public:

  enum eTraceTypes {
    NOTHING =  0,
    //INSTRUCTION        = (1<<24),
    BREAKPOINT         = (2<<24),
    INTERRUPT          = (3<<24),
    _RESET             = (4<<24),
    WRITE_TRIS         = (5<<24),
    WRITE_OPTION       = (6<<24),
    OPCODE_WRITE       = (7<<24),
    LAST_TRACE_TYPE       = (8<<24),

    CYCLE_COUNTER_LO   = (0x80<<24),
    CYCLE_COUNTER_HI   = (0x40<<24)
  };


#define    TRACE_BUFFER_SIZE  (1<<12)
#define    TRACE_BUFFER_MASK  (TRACE_BUFFER_SIZE-1)
#define    TRACE_BUFFER_NEAR_FULL  (TRACE_BUFFER_SIZE * 3 /4)
#define    TRACE_STRING_BUFFER 50

  unsigned int trace_buffer[TRACE_BUFFER_SIZE];
  unsigned int trace_index;
  unsigned int trace_flag;
  bool bLogging;
  TraceRawLog logger;

  traceValue trace_value;

  // When interfaced with a gui, the contents of the trace
  // buffer are decoded one line-at-a-time, copied to the string_buffer
  // and sent to the gui via xref interface (actually, the gui
  // is notified that new data is available in the string_buffer).
  XrefObject *xref;
  char  string_buffer[TRACE_STRING_BUFFER];
  guint64 string_cycle;          // The cycle corresponding to the decoded string
  unsigned int string_index;     // The trace buffer index corresponding "   "

  Processor *cpu;

  TraceFrame *current_frame;
  guint64 current_cycle_time;      // used when decoding the trace buffer.
  list <TraceFrame *> traceFrames;
  unsigned int lastTraceType;
  unsigned int lastSubTraceType;

  Trace (void);
  ~Trace(void);

  // trace raw allows any value to be written to the trace buffer.
  // This is useful for modules that wish to trace things, but do
  // not wish to modify the Trace class.

  inline void raw (unsigned int ui)
  {
    trace_buffer[trace_index] = ui;
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
    trace_buffer[trace_index] = (unsigned int)(CYCLE_COUNTER_LO | (cc & 0xffffffff));
    trace_index = (trace_index + 1) & TRACE_BUFFER_MASK;
    trace_buffer[trace_index] = (unsigned int)(CYCLE_COUNTER_HI | (cc>>32) | (cc & CYCLE_COUNTER_LO));
    trace_index = (trace_index + 1) & TRACE_BUFFER_MASK;
    if(bLogging && near_full()) 
      logger.log();
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

  inline bool near_full(void) {
    return (trace_index > TRACE_BUFFER_NEAR_FULL);
  }

  void switch_cpus(Processor *new_cpu) {cpu = new_cpu;};

  int  dump (int n=0, FILE *out_stream=0);
  void dump_last_instruction(void);
  int  dump1(unsigned int,char *, int);
  void dump_raw(int n);

  // tbi - trace buffer index masking.
  inline unsigned int tbi(unsigned int index)
  {
    return index & TRACE_BUFFER_MASK;
  }

  // inRange - returns true if the trace index i is between the 
  // indices of low and high. 
  // It's assumed that the range does not exceed half of the trace buffer
  bool inRange(unsigned int i, unsigned int low, unsigned int high)
  {
    i = tbi(i);

    if( low < high) 
      return (i >= low && i <= high);
    // Looks like the range straddles the roll over boundary.
    return (i >= low || i <= high);
  }


  // get() return the trace entry at 'index'
  inline unsigned int operator [] (unsigned int index)
  {
    return trace_buffer[tbi(index)];
  }

  unsigned int get(unsigned int index)
  {
    return trace_buffer[tbi(index)];
  }

  // type() - return the trace type at 'index'
  unsigned int type(unsigned int index)
  {
    return operator[](index) & 0xff000000;
  }

  // A gpsim clock cycle takes two consecutive trace buffer entries.
  // The is_cycle_trace() member function will examine the trace
  // buffer to determine if the two traces starting at 'index' are
  // a cycle trace.
  int is_cycle_trace(unsigned int index, guint64 *cvt_cycle);

  // When logging is enabled, the entire trace buffer will be copied to a file.
  void enableLogging(char *fname);
  void disableLogging();

  unsigned int allocateTraceType(TraceType *,int nSlots=1);

  // Trace frame manipulation
  void addFrame(TraceFrame *newFrame);
  void addToCurrentFrame(TraceObject *to);
  void deleteTraceFrame(void);
  void printTraceFrame(FILE *);


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
  guint64  *buffer;             // Where the time is stored
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
