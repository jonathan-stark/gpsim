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

#ifndef __INTERFACE_H__
#define __INTERFACE_H__

/*
 * interface.h
 *
 * Here are (hopefully) all of the definitions needed for an
 * external program (such as a gui) to interface with gpsim
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <glib.h>
#include "modules.h"
//#include "symbol.h"
enum SYMBOL_TYPE
{
  SYMBOL_INVALID,
  SYMBOL_BASE_CLASS,
  SYMBOL_IOPORT,
  SYMBOL_STIMULUS_NODE,
  SYMBOL_STIMULUS,
  SYMBOL_LINE_NUMBER,
  SYMBOL_CONSTANT,
  SYMBOL_REGISTER,
  SYMBOL_ADDRESS,
  SYMBOL_SPECIAL_REGISTER,   // like W
  SYMBOL_PROCESSOR,
  SYMBOL_MODULE
};

typedef enum _REGISTER_TYPE
{
    REGISTER_RAM,
    REGISTER_EEPROM
} REGISTER_TYPE;

struct file_context {
  char *name;           /* file name */
  FILE *file_ptr;
  int *line_seek;       /* an array of offsets into the file that point to
			 *  the start of the source lines. */
  int max_line;
};

extern int verbose;
extern unsigned int gpsim_is_initialized;
void  initialization_is_complete(void);

#define INVALID_VALUE 0xffffffff

  /*********************************************************************
   * 
   * What follows are a whole bunch of functions that provide access to
   * gpsim's innards. This is what the gui uses to interface to gpsim.
   * All of the functions here have "C" signatures, which is to say
   * they may be called from "C" code as well as from C++
   */


#define SYMBOL_NAME_LEN 32
typedef struct _sym
{
    enum SYMBOL_TYPE type;
    char *name;
    int value;
} sym;

  //---------------------------------------------------------------------------
  // processor functions
  //---------------------------------------------------------------------------
  char *gpsim_processor_get_name(unsigned int processor_id);
  unsigned int gpsim_get_pc_value(unsigned int processor_id);
  void  gpsim_put_pc_value(unsigned int processor_id, unsigned int pc_value);
  unsigned int gpsim_get_status(unsigned int processor_id);
  void gpsim_put_status(unsigned int processor_id, unsigned int status_value);
  unsigned int gpsim_get_w(unsigned int processor_id);
  void gpsim_put_w(unsigned int processor_id, unsigned int w_value);
  unsigned int gpsim_get_cycles_lo(unsigned int processor_id);
  guint64  gpsim_get_cycles(unsigned int processor_id);
  double gpsim_get_inst_clock(unsigned int processor_id);
  guint64  gpsim_get_update_rate(void);
  void     gpsim_set_update_rate(guint64);
  void gpsim_assign_pc_xref(unsigned int processor_id, gpointer xref);
  void gpsim_assign_trace_xref(gpointer xref);
  void gpsim_get_current_trace(guint64 *current_cycle, int *trace_index,
			       char *current_trace, int bufsize);
  void gpsim_trace_dump_to_file(int number_of_instructions, FILE *f);
  void gpsim_step(unsigned int processor_id, unsigned int steps);
  void gpsim_step_over(unsigned int processor_id);
  void gpsim_hll_step(unsigned int processor_id);
  void gpsim_hll_step_over(unsigned int processor_id);
  void gpsim_run(unsigned int processor_id);
  void gpsim_stop(unsigned int processor_id);
  void gpsim_reset(unsigned int processor_id);
  void gpsim_finish(unsigned int processor_id);
  void gpsim_run_to_address(unsigned int processor_id, unsigned int address);
  unsigned int gpsim_get_stack_size(unsigned int processor_id);
  unsigned int gpsim_get_stack_value(unsigned int processor_id, unsigned int address);
  guint64 gpsim_get_current_time(void);


  //---------------------------------------------------------------------------
  // Profiling
  //---------------------------------------------------------------------------
  void gpsim_enable_profiling(unsigned int processor_id);
  void gpsim_disable_profiling(unsigned int processor_id);
  guint64 gpsim_get_cycles_used(unsigned int processor_id, unsigned int address);
  guint64 gpsim_get_register_read_accesses(unsigned int processor_id, REGISTER_TYPE type, unsigned int address);
  guint64 gpsim_get_register_write_accesses(unsigned int processor_id, REGISTER_TYPE type, unsigned int address);


  //---------------------------------------------------------------------------
  // misc gpsim functions
  //---------------------------------------------------------------------------
  gpointer gpsim_set_cyclic_break_point( unsigned int processor_id, 
				   void (*interface_callback)(gpointer), 
				   gpointer interface_callback_data,
				   guint64 cycle);
  gpointer gpsim_set_cyclic_break_point2( void (*interface_callback)(gpointer), 
					  gpointer interface_callback_data,
					  guint64 cycle);
  void gpsim_clear_break(gpointer b);
  int gpsim_open(unsigned int processor_id, char *file);
  unsigned int gpsim_get_number_of_source_files(unsigned int processor_id);
  unsigned int gpsim_get_hll_file_id(unsigned int processor_id,
			  	 unsigned int address);
  unsigned int gpsim_get_file_id(unsigned int processor_id,
				 unsigned int address);
  struct file_context * gpsim_get_file_context(unsigned int processor_id,
					       unsigned int file_id);
  char *gpsim_get_version(char *dest, int max_len);
  int gpsim_get_hll_mode(unsigned int processor_id);
  int gpsim_set_hll_mode(unsigned int processor_id, int mode);
  void gpsim_set_bulk_mode(int flag);
  
  //---------------------------------------------------------------------------
  // symbol functions
  //---------------------------------------------------------------------------
  void gpsim_symbol_rewind(unsigned int processor_id);
  sym *gpsim_symbol_iter(unsigned int processor_id); // NULL on end


  //---------------------------------------------------------------------------
  // RAM/EEPROM memory functions
  //---------------------------------------------------------------------------
  unsigned int gpsim_get_register_memory_size(unsigned int processor_id,
					      REGISTER_TYPE type);
  char *gpsim_get_register_name(unsigned int processor_id,
				REGISTER_TYPE type,
				unsigned int register_number);
  unsigned int gpsim_get_register_value(unsigned int processor_id,
					REGISTER_TYPE type,
					unsigned int register_number);
  void  gpsim_put_register_value(unsigned int processor_id,
				 REGISTER_TYPE type,
				 unsigned int register_number,
				 unsigned int register_value);
  unsigned int gpsim_reg_has_breakpoint(unsigned int processor_id,
					REGISTER_TYPE type,
					unsigned int register_number);
  unsigned int gpsim_reg_set_read_breakpoint(unsigned int processor_id,
					     REGISTER_TYPE type,
					     unsigned int register_number);
  unsigned int gpsim_reg_set_write_breakpoint(unsigned int processor_id,
					      REGISTER_TYPE type,
					      unsigned int register_number);
  unsigned int gpsim_reg_set_read_value_breakpoint(unsigned int processor_id,
						   REGISTER_TYPE type,
						   unsigned int register_number,
						   unsigned int value);
  unsigned int gpsim_reg_set_write_value_breakpoint(unsigned int processor_id,
						    REGISTER_TYPE type,
						    unsigned int register_number,
						    unsigned int value);
  unsigned int gpsim_reg_clear_breakpoints(unsigned int processor_id,
					   REGISTER_TYPE type,
					   unsigned int register_number);
  unsigned int gpsim_set_log_name(unsigned int processor_id, char *filename, int format);
  unsigned int gpsim_reg_set_read_logging(unsigned int processor_id,
					  REGISTER_TYPE type,
					  unsigned int register_number);
  unsigned int gpsim_reg_set_write_logging(unsigned int processor_id,
					   REGISTER_TYPE type,
					   unsigned int register_number);
  unsigned int gpsim_reg_set_read_value_logging(unsigned int processor_id,
						REGISTER_TYPE type,
						unsigned int register_number,
						unsigned int value,
						unsigned int mask);
  unsigned int gpsim_reg_set_write_value_logging(unsigned int processor_id,
						 REGISTER_TYPE type,
						 unsigned int register_number,
						 unsigned int value,
						 unsigned int mask);
  gboolean gpsim_register_is_alias(unsigned int processor_id,
				   REGISTER_TYPE type,
				   unsigned int register_number);
  gboolean gpsim_register_is_sfr(unsigned int processor_id,
				 REGISTER_TYPE type,
				 unsigned int register_number);
  gboolean gpsim_register_is_valid(unsigned int processor_id,
				   REGISTER_TYPE type,
				   unsigned int register_number);
  void  gpsim_clear_register_xref(unsigned int processor_id,
				  REGISTER_TYPE type,
				  unsigned int reg_number,
				  gpointer xref);
  void  gpsim_assign_register_xref(unsigned int processor_id,
				   REGISTER_TYPE type,
				   unsigned int reg_number,
				   gpointer xref);

  
  //---------------------------------------------------------------------------
  // program memory functions
  //---------------------------------------------------------------------------
  void gpsim_set_read_break_at_address(unsigned int processor_id,
					  unsigned int address);
  void gpsim_set_write_break_at_address(unsigned int processor_id,
					  unsigned int address);
  void gpsim_set_execute_break_at_address(unsigned int processor_id,
					  unsigned int address);
  void gpsim_set_notify_point_at_address(unsigned int processor_id,
					 unsigned int address,
					 void (*cb)(gpointer),
					 gpointer data);
  void gpsim_set_profile_start_at_address(unsigned int processor_id,
					  unsigned int address,
					  void (*cb)(gpointer),
					  gpointer data);
  void gpsim_set_profile_stop_at_address(unsigned int processor_id,
					 unsigned int address,
					 void (*cb)(gpointer),
					 gpointer data);
  void gpsim_clear_profile_start_at_address(unsigned int processor_id,
					    unsigned int address);
  void gpsim_clear_profile_stop_at_address(unsigned int processor_id,
					   unsigned int address);
  void gpsim_clear_breakpoints_at_address(unsigned int processor_id,
					  unsigned int address);
  void gpsim_toggle_break_at_address(unsigned int processor_id,
				     unsigned int address);
  void gpsim_toggle_break_at_line(unsigned int processor_id,
				  unsigned int file_id,
				  unsigned int line);
  void gpsim_toggle_break_at_hll_line(unsigned int processor_id,
				  unsigned int file_id,
				  unsigned int line);
  unsigned int  gpsim_find_closest_address_to_line(unsigned int processor_id,
						   unsigned int file_id,
						   unsigned int line);
  unsigned int  gpsim_find_closest_address_to_hll_line(unsigned int processor_id,
						   unsigned int file_id,
						   unsigned int line);
  char *gpsim_get_opcode_name(Processor *cpu,
			      unsigned int address,
			      char *buffer);
  unsigned int gpsim_get_opcode(unsigned int processor_id,
				unsigned int address);
  void gpsim_put_opcode(unsigned int processor_id,
			unsigned int address,
			unsigned int opcode);
  unsigned int gpsim_get_src_line(unsigned int processor_id,
				  unsigned int address);
  unsigned int gpsim_get_hll_src_line(unsigned int processor_id,
                                    unsigned int address);
#endif /* __INTERFACE_H__ */
