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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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
   * What follows is a whole bunch of functions that provide access to
   * gpsim's innards. This is what the gui uses to interface to gpsim.
   * All of the functions here have "C" signatures, which is to say
   * they may be called from "C" code as well as from C++
   */


#define SYMBOL_NAME_LEN 32
typedef struct _sym
{
    enum SYMBOL_TYPE type;
    char name[SYMBOL_NAME_LEN];
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
  guint64  gpsim_get_update_rate(void);
  void     gpsim_set_update_rate(guint64);
  void gpsim_assign_pc_xref(unsigned int processor_id, gpointer xref);
  void gpsim_assign_trace_xref(gpointer xref);
  void gpsim_get_current_trace(guint64 *current_cycle, int *trace_index,
			       char *current_trace, int bufsize);
  void gpsim_trace_dump_to_file(int number_of_instructions, FILE *f);
  void gpsim_step(unsigned int processor_id, unsigned int steps);
  void gpsim_step_over(unsigned int processor_id);
  void gpsim_run(unsigned int processor_id);
  void gpsim_stop(unsigned int processor_id);
  void gpsim_reset(unsigned int processor_id);
  void gpsim_finish(unsigned int processor_id);
  void gpsim_run_to_address(unsigned int processor_id, unsigned int address);
  unsigned int gpsim_get_stack_size(unsigned int processor_id);
  unsigned int gpsim_get_stack_value(unsigned int processor_id, unsigned int address);


  //---------------------------------------------------------------------------
  // Profiling
  //---------------------------------------------------------------------------
  void gpsim_enable_profiling(unsigned int processor_id);
  void gpsim_disable_profiling(unsigned int processor_id);
  guint64 gpsim_get_cycles_used(unsigned int processor_id, unsigned int address);


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
  int gpsim_open(unsigned int processor_id, char *file);
  unsigned int gpsim_get_number_of_source_files(unsigned int processor_id);
  unsigned int gpsim_get_file_id(unsigned int processor_id,
				 unsigned int address);
  struct file_context * gpsim_get_file_context(unsigned int processor_id,
					       unsigned int file_id);
  char *gpsim_get_version(char *dest, int max_len);

  
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
  gboolean gpsim_register_is_alias(unsigned int processor_id,
				   REGISTER_TYPE type,
				   unsigned int register_number);
  gboolean gpsim_register_is_sfr(unsigned int processor_id,
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
  unsigned int gpsim_get_program_memory_size(unsigned int processor_id);
  unsigned int gpsim_address_has_breakpoint(unsigned int processor_id,
					    unsigned int address);
  unsigned int gpsim_address_has_changed(unsigned int processor_id,
					 unsigned int address);
  void  gpsim_assign_program_xref(unsigned int processor_id,
				  unsigned int address,
				  gpointer xref);
  void gpsim_set_read_break_at_address(unsigned int processor_id,
					  unsigned int address);
  void gpsim_set_write_break_at_address(unsigned int processor_id,
					  unsigned int address);
  void gpsim_set_execute_break_at_address(unsigned int processor_id,
					  unsigned int address);
  void gpsim_clear_breakpoints_at_address(unsigned int processor_id,
					  unsigned int address);
  void gpsim_toggle_break_at_address(unsigned int processor_id,
				     unsigned int address);
  void gpsim_toggle_break_at_line(unsigned int processor_id,
				  unsigned int file_id,
				  unsigned int line);
  unsigned int  gpsim_find_closest_address_to_line(unsigned int processor_id,
						   unsigned int file_id,
						   unsigned int line);
  char *gpsim_get_opcode_name(unsigned int processor_id,
			      unsigned int address,
			      char *buffer);
  unsigned int gpsim_get_opcode(unsigned int processor_id,
				unsigned int address);
  void gpsim_put_opcode(unsigned int processor_id,
			unsigned int address,
			unsigned int opcode);
  unsigned int gpsim_get_src_line(unsigned int processor_id,
				  unsigned int address);


  //---------------------------------------------------------------------------
  // pin interface functions
  //---------------------------------------------------------------------------
  void  gpsim_assign_pin_xref(unsigned int processor_id,
			      unsigned int pin,
			      gpointer xref);
  unsigned int  gpsim_package_pin_count(unsigned int processor_id);
  char *gpsim_pin_get_name(unsigned int processor_id,
			   unsigned int pin);
  unsigned int  gpsim_pin_get_value(unsigned int processor_id,
				    unsigned int pin);
  void  gpsim_pin_toggle(unsigned int processor_id,
			 unsigned int pin);
  unsigned int  gpsim_pin_get_dir(unsigned int processor_id,
				  unsigned int pin);
  void  gpsim_pin_set_dir(unsigned int processor_id,
			  unsigned int pin,
			  unsigned int new_dir);


  //---------------------------------------------------------------------------
  //
  //          callback function registration
  //
  //---------------------------------------------------------------------------
  unsigned int gpsim_register_interface(gpointer _new_object);
  void gpsim_register_update_object(unsigned int interface_id,
				    void (*update_object) (gpointer xref,int new_value) );
  void gpsim_register_remove_object(unsigned int interface_id, 
				    void (*remove_object) (gpointer xref));
  void gpsim_register_new_processor(unsigned int interface_id, 
				    void (*new_processor) (unsigned int processor_id));
  void gpsim_register_simulation_has_stopped(unsigned int interface_id, 
					     void (*simulation_has_stopped) (gpointer));
  void gpsim_register_new_program(unsigned int interface_id, 
				  void (*new_program)  (unsigned int processor_id));



#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __INTERFACE_H__ */
