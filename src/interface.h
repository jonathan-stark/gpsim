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
#include "gpsim_def.h"
#include <glib.h>
//#include "symbol.h"
enum SYMBOL_TYPE
{
  SYMBOL_BASE_CLASS,
  SYMBOL_IOPORT,
  SYMBOL_STIMULUS_NODE,
  SYMBOL_STIMULUS,
  SYMBOL_LINE_NUMBER,
  SYMBOL_CONSTANT,
  SYMBOL_REGISTER,
  SYMBOL_ADDRESS
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

char *gpsim_get_register_name(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number);
unsigned int gpsim_get_register_value(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number);
void  gpsim_put_register_value(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number, unsigned int register_value);

#define SYMBOL_NAME_LEN 32
typedef struct _sym
{
    enum SYMBOL_TYPE type;
    char name[SYMBOL_NAME_LEN];
    int value;
} sym;

gboolean gpsim_register_is_alias(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number);
gboolean gpsim_register_is_sfr(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number);
void gpsim_symbol_rewind(unsigned int processor_id);
sym *gpsim_symbol_iter(unsigned int processor_id); // NULL on end

//  char *gpsim_get_register_name(unsigned int processor_id, unsigned int register_number);
//  unsigned int gpsim_get_register_value(unsigned int processor_id, unsigned int register_number);
//  void  gpsim_put_register_value(unsigned int processor_id, unsigned int register_number, unsigned int register_value);
  char *gpsim_processor_get_name(unsigned int processor_id);
  unsigned int gpsim_get_pc_value(unsigned int processor_id);
  void  gpsim_put_pc_value(unsigned int processor_id, unsigned int pc_value);
  unsigned int gpsim_get_status(unsigned int processor_id);
  unsigned int gpsim_get_w(unsigned int processor_id);
  unsigned int gpsim_get_cycles_lo(unsigned int processor_id);
  guint64  gpsim_get_cycles(unsigned int processor_id);
  guint64  gpsim_get_update_rate(void);
  void     gpsim_set_update_rate(guint64);
  unsigned int gpsim_get_program_memory_size(unsigned int processor_id);
  unsigned int gpsim_get_register_memory_size(unsigned int processor_id,REGISTER_TYPE type);
  unsigned int gpsim_reg_has_breakpoint(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number);
  unsigned int gpsim_reg_set_read_breakpoint(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number);
  unsigned int gpsim_reg_set_write_breakpoint(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number);
  unsigned int gpsim_reg_set_read_value_breakpoint(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number, unsigned int value);
  unsigned int gpsim_reg_set_write_value_breakpoint(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number, unsigned int value);
  unsigned int gpsim_reg_clear_breakpoints(unsigned int processor_id, REGISTER_TYPE type, unsigned int register_number);
  unsigned int gpsim_address_has_breakpoint(unsigned int processor_id, unsigned int address);
  void  gpsim_assign_program_xref(unsigned int processor_id, unsigned int address, gpointer xref);
  void  gpsim_clear_register_xref(unsigned int processor_id, REGISTER_TYPE type, unsigned int reg_number, gpointer xref);
  void  gpsim_assign_register_xref(unsigned int processor_id, REGISTER_TYPE type, unsigned int reg_number, gpointer xref);
  void gpsim_assign_pc_xref(unsigned int processor_id, gpointer xref);
  void gpsim_step(unsigned int processor_id, unsigned int steps);
  void gpsim_step_over(unsigned int processor_id);
  void gpsim_run(unsigned int processor_id);
  void gpsim_run_to_address(unsigned int processor_id, unsigned int address);
  void gpsim_toggle_break_at_address(unsigned int processor_id, unsigned int address);
  void gpsim_toggle_break_at_line(unsigned int processor_id, unsigned int file_id, unsigned int line);
  unsigned int  gpsim_find_closest_address_to_line(unsigned int processor_id, unsigned int file_id, unsigned int line);
  unsigned int gpsim_get_file_id(unsigned int processor_id, unsigned int address);
  struct file_context * gpsim_get_file_context(unsigned int processor_id, unsigned int file_id);
  unsigned int gpsim_get_src_line(unsigned int processor_id, unsigned int address);
  unsigned int gpsim_get_number_of_source_files(unsigned int processor_id);
  char *gpsim_get_opcode_name(unsigned int processor_id, unsigned int address, char *buffer);
  unsigned int gpsim_get_opcode(unsigned int processor_id, unsigned int address);
  gpointer gpsim_set_cyclic_break_point( unsigned int processor_id, 
				   void (*interface_callback)(gpointer), 
				   gpointer interface_callback_data,
				   guint64 cycle);

  //---------------------------------------------------------------------------
  // pin interface functions
  //---------------------------------------------------------------------------
  void  gpsim_assign_pin_xref(unsigned int processor_id, unsigned int pin, gpointer xref);
  unsigned int  gpsim_package_pin_count(unsigned int processor_id);
  char *gpsim_pin_get_name(unsigned int processor_id, unsigned int pin);
  unsigned int  gpsim_pin_get_value(unsigned int processor_id, unsigned int pin);
  void  gpsim_pin_set_value(unsigned int processor_id, unsigned int pin);
  unsigned int  gpsim_pin_get_dir(unsigned int processor_id, unsigned int pin);
  void  gpsim_pin_set_dir(unsigned int processor_id, unsigned int pin, unsigned int new_dir);


  //---------------------------------------------------------------------------
  //
  //          callback functions
  //
  //---------------------------------------------------------------------------
void gpsim_interface_init(void);

/*
 * update_object - pointer to the function that is invoked when an object changes
 *
 * If an object, like the contents of a register, changes then this function
 * will be called (if it's non-null). There are two parameters:
 *  xref - this is a pointer to some structure in the client's data space.
 *  new_value - this is the new value to which the object has changed.
 *
 *
 */

extern void (*update_object) (gpointer xref,int new_value);



/*
 * simulation_has_stopped - pointer to the function that is invoked when gpsim has
 *                          stopped simulating.
 */

extern void (*simulation_has_stopped) (void);


/*
 * new_processor - pointer to the function that is invoked when a new processor is
 *                 added to gpsim
 */

extern void (*new_processor) (unsigned int processor_id);

/*
 * new_program - pointer to the function that is invoked when a new program is
 *               loaded into gpsim
 */

  /* extern void (*new_program)  (pic_processor *p);*/

extern void (*new_program)  (unsigned int processor_id);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __INTERFACE_H__ */
